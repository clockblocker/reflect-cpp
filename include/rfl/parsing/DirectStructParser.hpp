#ifndef RFL_PARSING_DIRECTSTRUCTPARSER_HPP_
#define RFL_PARSING_DIRECTSTRUCTPARSER_HPP_

#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../AddTagsToVariants.hpp"
#include "../AllowRawPtrs.hpp"
#include "../DefaultIfMissing.hpp"
#include "../DurationsAsCount.hpp"
#include "../EnumNamesOnly.hpp"
#include "../FailFast.hpp"
#include "../Metadata.hpp"
#include "../NamedTuple.hpp"
#include "../NoExtraFields.hpp"
#include "../NoFieldNames.hpp"
#include "../NoOptionals.hpp"
#include "../Processors.hpp"
#include "../Result.hpp"
#include "../UnderlyingEnums.hpp"
#include "../internal/DirectFieldNames.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/bind_to_tuple.hpp"
#include "../internal/default_if_missing_v.hpp"
#include "../internal/has_default_val_v.hpp"
#include "../internal/is_extra_fields.hpp"
#include "../internal/is_field.hpp"
#include "../internal/is_flatten_field.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "../internal/ptr_cast.hpp"
#include "../num_fields.hpp"
#include "FieldErrors.hpp"
#include "NamedTupleParser.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "ViewReader.hpp"
#include "ViewReaderWithDefault.hpp"
#include "call_destructors_where_necessary.hpp"
#include "schema/Type.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "to_single_error_message.hpp"
#include "wrap_error.hpp"

namespace rfl {
namespace json {
struct Reader;
}
namespace generic {
struct Reader;
}
namespace xml {
struct Reader;
}
namespace env {
struct Reader;
}
namespace yaml {
struct Reader;
}
namespace toml {
struct Reader;
}
namespace cli {
struct Reader;
}
namespace cbor {
struct Reader;
}
namespace bson {
struct Reader;
}
namespace msgpack {
struct Reader;
}
namespace ubjson {
struct Reader;
}
namespace flexbuf {
struct Reader;
}
}  // namespace rfl

namespace rfl::parsing {

namespace detail {

template <class T>
struct is_direct_identity_processor : std::false_type {};

#define RFL_DIRECT_IDENTITY_PROCESSOR(Type) \
  template <>                               \
  struct is_direct_identity_processor<Type> : std::true_type {}

RFL_DIRECT_IDENTITY_PROCESSOR(DefaultIfMissing);
RFL_DIRECT_IDENTITY_PROCESSOR(NoExtraFields);
RFL_DIRECT_IDENTITY_PROCESSOR(NoFieldNames);
RFL_DIRECT_IDENTITY_PROCESSOR(NoOptionals);
RFL_DIRECT_IDENTITY_PROCESSOR(FailFast);
RFL_DIRECT_IDENTITY_PROCESSOR(UnderlyingEnums);
RFL_DIRECT_IDENTITY_PROCESSOR(EnumNamesOnly);
RFL_DIRECT_IDENTITY_PROCESSOR(AddTagsToVariants);
RFL_DIRECT_IDENTITY_PROCESSOR(AddNamespacedTagsToVariants);
RFL_DIRECT_IDENTITY_PROCESSOR(DurationsAsCount);
RFL_DIRECT_IDENTITY_PROCESSOR(AllowRawPtrs);

#undef RFL_DIRECT_IDENTITY_PROCESSOR

template <class T>
struct is_direct_identity_processor_pack : is_direct_identity_processor<T> {};

template <class... Ps>
struct is_direct_identity_processor_pack<Processors<Ps...>>
    : std::bool_constant<
          (is_direct_identity_processor_pack<Ps>::value && ...)> {};

/// Known named-field readers that never query `ObjectReader::size()` as a
/// `static constexpr` expression. For these readers we pass the non-`T`,
/// non-`N` `DirectStructViewReader<R, W, ProcessorsType>`, so
/// `R::read_object` is instantiated once per `(R, W, ProcessorsType)` per TU.
/// Unknown/positional readers default to `SizedDirectStructViewReader<..., N>`.
template <class R>
struct reader_needs_static_view_size : std::true_type {};

template <>
struct reader_needs_static_view_size<rfl::json::Reader> : std::false_type {};
template <>
struct reader_needs_static_view_size<rfl::generic::Reader> : std::false_type {};
template <>
struct reader_needs_static_view_size<rfl::xml::Reader> : std::false_type {};
template <>
struct reader_needs_static_view_size<rfl::env::Reader> : std::false_type {};
template <>
struct reader_needs_static_view_size<rfl::yaml::Reader> : std::false_type {};
template <>
struct reader_needs_static_view_size<rfl::toml::Reader> : std::false_type {};
template <>
struct reader_needs_static_view_size<rfl::cli::Reader> : std::false_type {};
template <>
struct reader_needs_static_view_size<rfl::cbor::Reader> : std::false_type {};
template <>
struct reader_needs_static_view_size<rfl::bson::Reader> : std::false_type {};
template <>
struct reader_needs_static_view_size<rfl::msgpack::Reader> : std::false_type {};
template <>
struct reader_needs_static_view_size<rfl::ubjson::Reader> : std::false_type {};
template <>
struct reader_needs_static_view_size<rfl::flexbuf::Reader> : std::false_type {};

template <class R>
inline constexpr bool reader_needs_static_view_size_v =
    reader_needs_static_view_size<std::remove_cvref_t<R>>::value;

template <class T>
consteval bool direct_fields_are_supported() {
  using Result = decltype(internal::apply_to_fields(
      std::declval<T&>(), []<class... Fields>(Fields&...) {
        return std::bool_constant<
            ((!internal::is_field_v<Fields> &&
              !internal::is_flatten_field_v<Fields> &&
              !internal::is_extra_fields_v<Fields>) &&
             ...)>{};
      }));
  return Result::value;
}

template <class... Fields>
struct DirectFieldTypeList {};

template <class T>
using direct_field_type_list_t = decltype(internal::apply_to_fields(
    std::declval<T&>(), []<class... Fields>(Fields&...) {
      return DirectFieldTypeList<Fields...>{};
    }));

template <class T>
inline auto collect_mutable_field_ptrs(T& _t) noexcept {
  return internal::apply_to_fields(_t, [](auto&... _refs) noexcept {
    return std::array<void*, sizeof...(_refs)>{const_cast<void*>(
        static_cast<const void*>(std::addressof(_refs)))...};
  });
}

template <class T>
inline auto collect_const_field_ptrs(const T& _t) noexcept {
  return internal::apply_to_fields(_t, [](const auto&... _refs) noexcept {
    return std::array<const void*, sizeof...(_refs)>{
        static_cast<const void*>(std::addressof(_refs))...};
  });
}

/// Type-erased per-field thunks parameterized ONLY by individual field type
/// (never by the enclosing struct `T`, `NamedTuple`, or `Tuple`).

template <class R, class W, class FieldT, class ProcessorsType>
void read_uninit_field_erased(const R& _r,
                              const typename R::InputVarType& _var,
                              const std::string_view _name, void* _field_ptr,
                              bool& _is_set,
                              FieldErrors<ProcessorsType>* _errors,
                              const size_t _reserve_errors) {
  auto* ptr = static_cast<FieldT*>(_field_ptr);
  detail::assign_uninitialized_view_field<R, W, FieldT*, ProcessorsType>(
      _r, _var, _name, ptr, _is_set, _errors, _reserve_errors);
}

template <class R, class W, class FieldT, class ProcessorsType>
void read_default_field_erased(const R& _r,
                               const typename R::InputVarType& _var,
                               const std::string_view _name, void* _field_ptr,
                               FieldErrors<ProcessorsType>* _errors) {
  auto* ptr = static_cast<FieldT*>(_field_ptr);
  detail::assign_default_view_field<R, W, FieldT*, ProcessorsType>(
      _r, _var, _name, ptr, _errors);
}

template <bool _ignore_empty_containers, bool _all_required,
          bool _defaults_constructed, class ProcessorsType, class FieldT>
void handle_missing_field_erased(
    const std::string_view _name, const bool _is_found, void* _field_ptr,
    bool* _is_set_ptr, FieldErrors<ProcessorsType>* _errors,
    const bool _value_initialize_default_vals) noexcept {
  auto* ptr = static_cast<FieldT*>(_field_ptr);
  detail::handle_missing_named_tuple_field<
      _ignore_empty_containers, _all_required, _defaults_constructed,
      ProcessorsType>(_name, _is_found, ptr, _is_set_ptr, _errors,
                      _value_initialize_default_vals);
}

template <class FieldT>
void destroy_field_erased(void* _field_ptr) noexcept {
  auto* ptr = static_cast<FieldT*>(_field_ptr);
  detail::destroy_field_if_set(true, ptr);
}

template <class R, class W, bool _ignore_empty_containers, bool _all_required,
          bool _no_field_names, class ProcessorsType, class FieldT,
          class OutputObjectOrArrayType>
void write_field_erased(const W& _w, const std::string_view _name,
                        const void* _field_ptr,
                        OutputObjectOrArrayType* _ptr) {
  using CleanFieldT = std::remove_cvref_t<FieldT>;
  const auto* ptr = static_cast<const CleanFieldT*>(_field_ptr);
  detail::write_named_tuple_field<R, W, _ignore_empty_containers, _all_required,
                                  _no_field_names, ProcessorsType>(_w, _name,
                                                                   ptr, _ptr);
}

template <class R, class W, bool _no_field_names, class ProcessorsType,
          class FieldT, class SchemaType>
void build_schema_field_erased(
    std::map<std::string, schema::Type>* _definitions, SchemaType* _schema,
    const std::string_view _wire_name, const std::string_view _raw_name,
    const void* _default_field_ptr,
    const std::span<const FieldMeta> _field_meta) noexcept {
  using U = std::remove_cvref_t<FieldT>;
  const U* ptr = static_cast<const U*>(_default_field_ptr);
  detail::build_named_tuple_field_schema<R, W, _no_field_names, ProcessorsType,
                                         U>(_definitions, _schema, _wire_name,
                                            _raw_name, ptr, _field_meta);
}

using DirectDestroyFn = void (*)(void*) noexcept;

inline void destroy_direct_fields(const bool* _set, void* const* _field_ptrs,
                                  const DirectDestroyFn* _destroy_fns,
                                  const size_t _size) noexcept {
  for (size_t i = 0; i < _size; ++i) {
    if (_set[i] && _destroy_fns[i] != nullptr) {
      _destroy_fns[i](_field_ptrs[i]);
    }
  }
}

}  // namespace detail

template <class T, class ProcessorsType>
inline constexpr bool can_use_direct_struct_parser_v = [] {
  using Type = std::remove_cvref_t<T>;
  if constexpr (!std::is_class_v<Type> || !std::is_aggregate_v<Type> ||
                std::is_union_v<Type> || num_fields<Type> == 0 ||
                !detail::is_direct_identity_processor_pack<
                    ProcessorsType>::value) {
    return false;
  } else {
    return detail::direct_fields_are_supported<Type>();
  }
}();

/// Non-`T`, non-`N` direct view reader for uninitialized aggregate storage.
/// Parameterized only by `(R, W, ProcessorsType)` so `R::read_object` is
/// instantiated once per `(R, W, ProcessorsType)` across all structs in a TU.
template <class R, class W, class ProcessorsType>
class DirectStructViewReader {
 public:
  using InputVarType = typename R::InputVarType;

  using ReadUninitFn = void (*)(const R&, const InputVarType&, std::string_view,
                                void*, bool&, FieldErrors<ProcessorsType>*,
                                size_t);

  DirectStructViewReader(const R* _r,
                         const internal::DirectFieldNamesDesc* _names,
                         void* const* _field_ptrs,
                         const ReadUninitFn* _read_fns, bool* _found,
                         bool* _set,
                         FieldErrors<ProcessorsType>* _errors) noexcept
      : r_(_r),
        names_(_names),
        field_ptrs_(_field_ptrs),
        read_fns_(_read_fns),
        found_(_found),
        set_(_set),
        errors_(_errors) {}

  ~DirectStructViewReader() = default;

  std::span<const std::string_view> field_names() const noexcept {
    return names_->span();
  }

  void read(const std::string_view& _name, const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    bool already_assigned = false;
    if (const int idx = names_->find(_name); idx >= 0) {
      const auto u = static_cast<size_t>(idx);
      if (!found_[u]) {
        found_[u] = true;
        already_assigned = true;
        read_fns_[u](*r_, _var, names_->names[u], field_ptrs_[u], set_[u],
                     errors_, names_->size);
      }
    }
    if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      static_assert(
          !schemaful::IsSchemafulReader<R>,
          "Passing rfl::NoExtraFields to a schemaful format does not make "
          "sense, because schemaful formats cannot have extra fields.");
      if (!already_assigned) {
        std::stringstream stream;
        stream << "Value named '" << _name << "' not used.";
        errors_->emplace_back(
            make_nested_error(std::string(_name), stream.str()));
      }
    }
  }

  void read(const int _index, const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    bool already_assigned = false;
    if (_index >= 0 && static_cast<size_t>(_index) < names_->size) {
      const auto u = static_cast<size_t>(_index);
      if (!found_[u]) {
        found_[u] = true;
        already_assigned = true;
        read_fns_[u](*r_, _var, names_->names[u], field_ptrs_[u], set_[u],
                     errors_, names_->size);
      }
    }
    if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      static_assert(
          !schemaful::IsSchemafulReader<R>,
          "Passing rfl::NoExtraFields to a schemaful format does not make "
          "sense, because schemaful formats cannot have extra fields.");
      if (!already_assigned) {
        std::stringstream stream;
        stream << "Value named '" << _index << "' not used.";
        errors_->emplace_back(
            make_nested_error(static_cast<size_t>(_index), stream.str()));
      }
    }
  }

  template <int i>
  void read(std::integral_constant<int, i>, const InputVarType& _var) const {
    read(i, _var);
  }

 private:
  const R* r_;
  const internal::DirectFieldNamesDesc* names_;
  void* const* field_ptrs_;
  const ReadUninitFn* read_fns_;
  bool* found_;
  bool* set_;
  FieldErrors<ProcessorsType>* errors_;
};

/// Thin static-size wrapper for positional/external readers requiring
/// `static constexpr size_t ObjectReader::size()`.
template <class R, class W, class ProcessorsType, std::size_t _size>
class SizedDirectStructViewReader
    : public DirectStructViewReader<R, W, ProcessorsType> {
 public:
  using Base = DirectStructViewReader<R, W, ProcessorsType>;
  using Base::Base;
  static constexpr std::size_t size() noexcept { return _size; }
};

/// Non-`T`, non-`N` direct view reader for default-constructed storage.
template <class R, class W, class ProcessorsType>
class DirectStructViewReaderWithDefault {
 public:
  using InputVarType = typename R::InputVarType;

  using ReadDefaultFn = void (*)(const R&, const InputVarType&,
                                 std::string_view, void*,
                                 FieldErrors<ProcessorsType>*);

  DirectStructViewReaderWithDefault(
      const R* _r, const internal::DirectFieldNamesDesc* _names,
      void* const* _field_ptrs, const ReadDefaultFn* _read_fns, bool* _found,
      FieldErrors<ProcessorsType>* _errors) noexcept
      : r_(_r),
        names_(_names),
        field_ptrs_(_field_ptrs),
        read_fns_(_read_fns),
        found_(_found),
        errors_(_errors) {}

  ~DirectStructViewReaderWithDefault() = default;

  std::span<const std::string_view> field_names() const noexcept {
    return names_->span();
  }

  void read(const std::string_view& _name, const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    bool already_assigned = false;
    if (const int idx = names_->find(_name); idx >= 0) {
      const auto u = static_cast<size_t>(idx);
      if (!found_[u]) {
        found_[u] = true;
        already_assigned = true;
        read_fns_[u](*r_, _var, names_->names[u], field_ptrs_[u], errors_);
      }
    }
    if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      if (!already_assigned) {
        std::stringstream stream;
        stream << "Value named '" << std::string(_name)
               << "' not used. Remove the rfl::NoExtraFields processor or add "
                  "rfl::ExtraFields to avoid this error message.";
        errors_->emplace_back(
            make_nested_error(std::string(_name), stream.str()));
      }
    }
  }

  void read(const int _index, const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    bool already_assigned = false;
    if (_index >= 0 && static_cast<size_t>(_index) < names_->size) {
      const auto u = static_cast<size_t>(_index);
      if (!found_[u]) {
        found_[u] = true;
        already_assigned = true;
        read_fns_[u](*r_, _var, names_->names[u], field_ptrs_[u], errors_);
      }
    }
    if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      if (!already_assigned) {
        std::stringstream stream;
        stream << "Value named '" << _index << "' not used.";
        errors_->emplace_back(
            make_nested_error(static_cast<size_t>(_index), stream.str()));
      }
    }
  }

  template <int i>
  void read(std::integral_constant<int, i>, const InputVarType& _var) const {
    read(i, _var);
  }

 private:
  const R* r_;
  const internal::DirectFieldNamesDesc* names_;
  void* const* field_ptrs_;
  const ReadDefaultFn* read_fns_;
  bool* found_;
  FieldErrors<ProcessorsType>* errors_;
};

template <class R, class W, class ProcessorsType, std::size_t _size>
class SizedDirectStructViewReaderWithDefault
    : public DirectStructViewReaderWithDefault<R, W, ProcessorsType> {
 public:
  using Base = DirectStructViewReaderWithDefault<R, W, ProcessorsType>;
  using Base::Base;
  static constexpr std::size_t size() noexcept { return _size; }
};

/// Non-`T`, non-`N` stripped-field-names array reader for uninitialized storage.
template <class R, class W, class ProcessorsType>
class DirectStructViewReaderStripped {
 public:
  using InputVarType = typename R::InputVarType;

  using ReadUninitFn = void (*)(const R&, const InputVarType&, std::string_view,
                                void*, bool&, FieldErrors<ProcessorsType>*,
                                size_t);

  DirectStructViewReaderStripped(const R* _r,
                                 const internal::DirectFieldNamesDesc* _names,
                                 void* const* _field_ptrs,
                                 const ReadUninitFn* _read_fns, bool* _found,
                                 bool* _set,
                                 FieldErrors<ProcessorsType>* _errors) noexcept
      : i_(0),
        r_(_r),
        names_(_names),
        field_ptrs_(_field_ptrs),
        read_fns_(_read_fns),
        found_(_found),
        set_(_set),
        errors_(_errors) {}

  ~DirectStructViewReaderStripped() = default;

  std::optional<Error> read(const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) {
      return std::nullopt;
    }
    if (i_ == names_->size) {
      std::stringstream stream;
      stream << "Expected a maximum of " << std::to_string(names_->size)
             << " fields, but got at least one more.";
      return Error(stream.str());
    }
    found_[i_] = true;
    read_fns_[i_](*r_, _var, names_->names[i_], field_ptrs_[i_], set_[i_],
                  errors_, names_->size);
    ++i_;
    return std::nullopt;
  }

 private:
  mutable size_t i_;
  const R* r_;
  const internal::DirectFieldNamesDesc* names_;
  void* const* field_ptrs_;
  const ReadUninitFn* read_fns_;
  bool* found_;
  bool* set_;
  FieldErrors<ProcessorsType>* errors_;
};

template <class R, class W, class ProcessorsType, std::size_t _size>
class SizedDirectStructViewReaderStripped
    : public DirectStructViewReaderStripped<R, W, ProcessorsType> {
 public:
  using Base = DirectStructViewReaderStripped<R, W, ProcessorsType>;
  using Base::Base;
  static constexpr std::size_t size() noexcept { return _size; }
};

/// Non-`T`, non-`N` stripped-field-names array reader for default storage.
template <class R, class W, class ProcessorsType>
class DirectStructViewReaderWithDefaultStripped {
 public:
  using InputVarType = typename R::InputVarType;

  using ReadDefaultFn = void (*)(const R&, const InputVarType&,
                                 std::string_view, void*,
                                 FieldErrors<ProcessorsType>*);

  DirectStructViewReaderWithDefaultStripped(
      const R* _r, const internal::DirectFieldNamesDesc* _names,
      void* const* _field_ptrs, const ReadDefaultFn* _read_fns, bool* _found,
      FieldErrors<ProcessorsType>* _errors) noexcept
      : i_(0),
        r_(_r),
        names_(_names),
        field_ptrs_(_field_ptrs),
        read_fns_(_read_fns),
        found_(_found),
        errors_(_errors) {}

  ~DirectStructViewReaderWithDefaultStripped() = default;

  std::optional<Error> read(const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) {
      return std::nullopt;
    }
    if (i_ == names_->size) {
      std::stringstream stream;
      stream << "Expected a maximum of " << std::to_string(names_->size)
             << " fields, but got at least one more.";
      return Error(stream.str());
    }
    found_[i_] = true;
    read_fns_[i_](*r_, _var, names_->names[i_], field_ptrs_[i_], errors_);
    ++i_;
    return std::nullopt;
  }

 private:
  mutable size_t i_;
  const R* r_;
  const internal::DirectFieldNamesDesc* names_;
  void* const* field_ptrs_;
  const ReadDefaultFn* read_fns_;
  bool* found_;
  FieldErrors<ProcessorsType>* errors_;
};

template <class R, class W, class ProcessorsType, std::size_t _size>
class SizedDirectStructViewReaderWithDefaultStripped
    : public DirectStructViewReaderWithDefaultStripped<R, W, ProcessorsType> {
 public:
  using Base = DirectStructViewReaderWithDefaultStripped<R, W, ProcessorsType>;
  using Base::Base;
  static constexpr std::size_t size() noexcept { return _size; }
};

namespace detail {

/// Non-`T`, non-`N` free runtime drivers — instantiated once per
/// `(R, W, _no_field_names, ProcessorsType)` across all structs in a TU.

template <class R, class W, bool _no_field_names, class ProcessorsType,
          class ReaderType, class ReadUninitFn, class MissingFn>
std::optional<Error> run_direct_read_uninit(
    const R& _r, const typename R::InputVarType& _var,
    const internal::DirectFieldNamesDesc* _names, void* const* _field_ptrs,
    const ReadUninitFn* _read_fns, const MissingFn* _missing_fns, bool* _found,
    bool* _set) {
  FieldErrors<ProcessorsType> errors;
  if constexpr (_no_field_names) {
    auto arr = _r.to_array(_var);
    if (!arr) [[unlikely]] {
      return arr.error();
    }
    const ReaderType reader(&_r, _names, _field_ptrs, _read_fns, _found, _set,
                            &errors);
    auto err = _r.read_array(reader, *arr);
    if (err) [[unlikely]] {
      return err;
    }
  } else {
    auto obj = _r.to_object(_var);
    if (!obj) [[unlikely]] {
      return obj.error();
    }
    const ReaderType reader(&_r, _names, _field_ptrs, _read_fns, _found, _set,
                            &errors);
    auto err = _r.read_object(reader, *obj);
    if (err) [[unlikely]] {
      return err;
    }
  }

  if (!should_stop_on_field_error<ProcessorsType>(errors)) {
    for (size_t i = 0; i < _names->size; ++i) {
      _missing_fns[i](_names->names[i], _found[i], _field_ptrs[i], &_set[i],
                      &errors, /*_value_initialize_default_vals=*/false);
    }
  }

  if constexpr (internal::fail_fast_v<ProcessorsType>) {
    return std::move(errors).error();
  } else {
    if (errors.size() != 0) [[unlikely]] {
      return to_single_error(std::move(errors));
    }
    return std::nullopt;
  }
}

template <class R, class W, bool _no_field_names, class ProcessorsType,
          class ReaderType, class ReadDefaultFn, class MissingFn>
std::optional<Error> run_direct_read_default(
    const R& _r, const typename R::InputVarType& _var,
    const internal::DirectFieldNamesDesc* _names, void* const* _field_ptrs,
    const ReadDefaultFn* _read_fns, const MissingFn* _missing_fns,
    bool* _found) {
  FieldErrors<ProcessorsType> errors;
  if constexpr (_no_field_names) {
    auto arr = _r.to_array(_var);
    if (!arr) [[unlikely]] {
      return arr.error();
    }
    const ReaderType reader(&_r, _names, _field_ptrs, _read_fns, _found,
                            &errors);
    auto err = _r.read_array(reader, *arr);
    if (err) [[unlikely]] {
      return err;
    }
  } else {
    auto obj = _r.to_object(_var);
    if (!obj) [[unlikely]] {
      return obj.error();
    }
    const ReaderType reader(&_r, _names, _field_ptrs, _read_fns, _found,
                            &errors);
    auto err = _r.read_object(reader, *obj);
    if (err) [[unlikely]] {
      return err;
    }
  }

  if (_missing_fns != nullptr &&
      !should_stop_on_field_error<ProcessorsType>(errors)) {
    for (size_t i = 0; i < _names->size; ++i) {
      _missing_fns[i](_names->names[i], _found[i], _field_ptrs[i], nullptr,
                      &errors, /*_value_initialize_default_vals=*/false);
    }
  }

  if constexpr (internal::fail_fast_v<ProcessorsType>) {
    return std::move(errors).error();
  } else {
    if (errors.size() != 0) [[unlikely]] {
      return to_single_error(std::move(errors));
    }
    return std::nullopt;
  }
}

template <class W, bool _no_field_names, class P, class WriteFn>
void run_direct_write(const W& _w, const P& _parent,
                      const internal::DirectFieldNamesDesc* _names,
                      const void* const* _field_ptrs,
                      const WriteFn* _write_fns) {
  using ParentType = Parent<W>;
  if constexpr (_no_field_names) {
    auto arr = ParentType::add_array(_w, _names->size, _parent);
    for (size_t i = 0; i < _names->size; ++i) {
      _write_fns[i](_w, _names->names[i], _field_ptrs[i], &arr);
    }
    _w.end_array(&arr);
  } else {
    auto obj = ParentType::add_object(_w, _names->size, _parent);
    for (size_t i = 0; i < _names->size; ++i) {
      _write_fns[i](_w, _names->names[i], _field_ptrs[i], &obj);
    }
    _w.end_object(&obj);
  }
}

template <bool _no_field_names, class ProcessorsType, class SchemaType,
          class SchemaFn>
schema::Type run_direct_to_schema(
    std::map<std::string, schema::Type>* _definitions,
    const internal::DirectFieldNamesDesc* _names,
    const void* const* _default_ptrs, const SchemaFn* _schema_fns,
    const std::span<const FieldMeta> _field_meta) {
  SchemaType schema;
  for (size_t i = 0; i < _names->size; ++i) {
    _schema_fns[i](_definitions, &schema, _names->names[i], _names->names[i],
                   _default_ptrs != nullptr ? _default_ptrs[i] : nullptr,
                   _field_meta);
  }
  if constexpr (internal::no_extra_fields_v<ProcessorsType> &&
                !_no_field_names) {
    schema.additional_properties_ = false;
  }
  return schema::Type{schema};
}

struct DirectDestroyGuard {
  const bool* set;
  void* const* field_ptrs;
  const DirectDestroyFn* destroy_fns;
  size_t size;

  ~DirectDestroyGuard() noexcept {
    destroy_direct_fields(set, field_ptrs, destroy_fns, size);
  }
};

}  // namespace detail

/// Direct aggregate parser that bypasses `rfl::NamedTuple` and `rfl::Tuple`
/// completely, dispatching through pay-as-you-go `static constexpr` tables of
/// type-erased per-field function pointers and shared non-`T` runtime drivers.
template <class R, class W, class T, class ProcessorsType>
struct DirectStructParser {
  using InputVarType = typename R::InputVarType;
  using Names = internal::DirectFieldNames<T>;
  static constexpr size_t size_ = Names::size_;

  using FormatNTParser = Parser<R, W, NamedTuple<>, ProcessorsType>;
  static constexpr bool ignore_empty_containers_ =
      FormatNTParser::ignore_empty_containers_;
  static constexpr bool all_required_ = FormatNTParser::all_required_;
  static constexpr bool no_field_names_ = FormatNTParser::no_field_names_;

  using OutputObjectOrArrayType =
      std::conditional_t<no_field_names_, typename W::OutputArrayType,
                         typename W::OutputObjectType>;
  using SchemaType = std::conditional_t<no_field_names_, schema::Type::Tuple,
                                        schema::Type::Object>;

  using ReadUninitFn = void (*)(const R&, const InputVarType&, std::string_view,
                                void*, bool&, FieldErrors<ProcessorsType>*,
                                size_t);
  using ReadDefaultFn = void (*)(const R&, const InputVarType&,
                                 std::string_view, void*,
                                 FieldErrors<ProcessorsType>*);
  using MissingFn = void (*)(std::string_view, bool, void*, bool*,
                             FieldErrors<ProcessorsType>*, bool) noexcept;
  using WriteFn = void (*)(const W&, std::string_view, const void*,
                           OutputObjectOrArrayType*);
  using SchemaFn = void (*)(std::map<std::string, schema::Type>*, SchemaType*,
                            std::string_view, std::string_view, const void*,
                            std::span<const FieldMeta>) noexcept;

 private:
  static constexpr bool needs_static_size_ =
      detail::reader_needs_static_view_size_v<R>;

  using UninitReaderType = std::conditional_t<
      no_field_names_,
      std::conditional_t<
          needs_static_size_,
          SizedDirectStructViewReaderStripped<R, W, ProcessorsType, size_>,
          DirectStructViewReaderStripped<R, W, ProcessorsType>>,
      std::conditional_t<
          needs_static_size_,
          SizedDirectStructViewReader<R, W, ProcessorsType, size_>,
          DirectStructViewReader<R, W, ProcessorsType>>>;

  using DefaultReaderType = std::conditional_t<
      no_field_names_,
      std::conditional_t<
          needs_static_size_,
          SizedDirectStructViewReaderWithDefaultStripped<R, W, ProcessorsType,
                                                         size_>,
          DirectStructViewReaderWithDefaultStripped<R, W, ProcessorsType>>,
      std::conditional_t<
          needs_static_size_,
          SizedDirectStructViewReaderWithDefault<R, W, ProcessorsType, size_>,
          DirectStructViewReaderWithDefault<R, W, ProcessorsType>>>;

  template <class... Fields>
  static consteval auto make_read_uninit_table(
      detail::DirectFieldTypeList<Fields...>) noexcept {
    return std::array<ReadUninitFn, sizeof...(Fields)>{
        &detail::read_uninit_field_erased<R, W, Fields, ProcessorsType>...};
  }

  template <class... Fields>
  static consteval auto make_read_default_table(
      detail::DirectFieldTypeList<Fields...>) noexcept {
    return std::array<ReadDefaultFn, sizeof...(Fields)>{
        &detail::read_default_field_erased<R, W, Fields, ProcessorsType>...};
  }

  template <bool _defaults_constructed, class... Fields>
  static consteval auto make_missing_table(
      detail::DirectFieldTypeList<Fields...>) noexcept {
    return std::array<MissingFn, sizeof...(Fields)>{
        &detail::handle_missing_field_erased<
            ignore_empty_containers_, all_required_, _defaults_constructed,
            ProcessorsType, Fields>...};
  }

  template <class FieldT>
  static consteval detail::DirectDestroyFn make_destroy_fn() noexcept {
    if constexpr (std::is_trivially_destructible_v<
                      std::remove_all_extents_t<FieldT>>) {
      return nullptr;
    } else {
      return &detail::destroy_field_erased<FieldT>;
    }
  }

  template <class... Fields>
  static consteval auto make_destroy_table(
      detail::DirectFieldTypeList<Fields...>) noexcept {
    return std::array<detail::DirectDestroyFn, sizeof...(Fields)>{
        make_destroy_fn<Fields>()...};
  }

  template <class... Fields>
  static consteval auto make_write_table(
      detail::DirectFieldTypeList<Fields...>) noexcept {
    return std::array<WriteFn, sizeof...(Fields)>{
        &detail::write_field_erased<R, W, ignore_empty_containers_,
                                    all_required_, no_field_names_,
                                    ProcessorsType, Fields,
                                    OutputObjectOrArrayType>...};
  }

  template <class... Fields>
  static consteval auto make_schema_table(
      detail::DirectFieldTypeList<Fields...>) noexcept {
    return std::array<SchemaFn, sizeof...(Fields)>{
        &detail::build_schema_field_erased<R, W, no_field_names_,
                                           ProcessorsType, Fields,
                                           SchemaType>...};
  }

 public:
  static RFL_ALWAYS_INLINE Result<T> read(const R& _r,
                                          const InputVarType& _var) {
    static constexpr auto read_fns =
        make_read_uninit_table(detail::direct_field_type_list_t<T>{});
    static constexpr auto missing_fns =
        make_missing_table<false>(detail::direct_field_type_list_t<T>{});
    static constexpr auto destroy_fns =
        make_destroy_table(detail::direct_field_type_list_t<T>{});

    alignas(T) unsigned char buf[sizeof(T)]{};
    auto* ptr = internal::ptr_cast<T*>(&buf);
    const auto field_ptrs = detail::collect_mutable_field_ptrs(*ptr);
    std::array<bool, size_> found{};
    std::array<bool, size_> set{};
    const detail::DirectDestroyGuard guard{set.data(), field_ptrs.data(),
                                           destroy_fns.data(), size_};

    auto err = detail::run_direct_read_uninit<R, W, no_field_names_,
                                              ProcessorsType, UninitReaderType>(
        _r, _var, &Names::desc_, field_ptrs.data(), read_fns.data(),
        missing_fns.data(), found.data(), set.data());

    if (err) [[unlikely]] {
      return error(std::move(*err));
    }
    return Result<T>(std::move(*ptr));
  }

  static std::optional<Error> read_with_default(const R& _r,
                                                const InputVarType& _var,
                                                T* _target) {
    static constexpr auto read_fns =
        make_read_default_table(detail::direct_field_type_list_t<T>{});
    const auto field_ptrs = detail::collect_mutable_field_ptrs(*_target);
    std::array<bool, size_> found{};

    const MissingFn* missing_ptr = nullptr;
    if constexpr (internal::has_default_val_v<T> &&
                  !internal::default_if_missing_v<ProcessorsType>) {
      static constexpr auto missing_fns =
          make_missing_table<true>(detail::direct_field_type_list_t<T>{});
      missing_ptr = missing_fns.data();
    }

    return detail::run_direct_read_default<R, W, no_field_names_,
                                           ProcessorsType, DefaultReaderType>(
        _r, _var, &Names::desc_, field_ptrs.data(), read_fns.data(),
        missing_ptr, found.data());
  }

  template <class P>
  static void write(const W& _w, const T& _var, const P& _parent) {
    static constexpr auto write_fns =
        make_write_table(detail::direct_field_type_list_t<T>{});
    const auto field_ptrs = detail::collect_const_field_ptrs(_var);
    detail::run_direct_write<W, no_field_names_>(_w, _parent, &Names::desc_,
                                                 field_ptrs.data(),
                                                 write_fns.data());
  }

  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions,
      const T* _defaults = nullptr,
      const std::span<const FieldMeta> _field_meta = {}) {
    static constexpr auto schema_fns =
        make_schema_table(detail::direct_field_type_list_t<T>{});
    if (_defaults != nullptr) {
      const auto default_ptrs = detail::collect_const_field_ptrs(*_defaults);
      return detail::run_direct_to_schema<no_field_names_, ProcessorsType,
                                          SchemaType>(
          _definitions, &Names::desc_, default_ptrs.data(), schema_fns.data(),
          _field_meta);
    }
    return detail::run_direct_to_schema<no_field_names_, ProcessorsType,
                                        SchemaType>(
        _definitions, &Names::desc_, nullptr, schema_fns.data(), _field_meta);
  }
};

}  // namespace rfl::parsing

#endif
