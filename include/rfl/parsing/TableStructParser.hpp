#ifndef RFL_PARSING_TABLESTRUCTPARSER_HPP_
#define RFL_PARSING_TABLESTRUCTPARSER_HPP_

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Result.hpp"
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
#include "FieldErrors.hpp"
#include "NamedTupleParser.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "ViewReader.hpp"
#include "ViewReaderWithDefault.hpp"
#include "call_destructors_where_necessary.hpp"
#include "find_field_index.hpp"
#include "schema/Type.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "to_single_error_message.hpp"
#include "wrap_error.hpp"

#ifndef RFL_TABLE_PARSER_MIN_FIELDS
#define RFL_TABLE_PARSER_MIN_FIELDS 0
#endif

namespace rfl {
struct AddNamespacedTagsToVariants;
struct AddTagsToVariants;
struct AllowRawPtrs;
struct CamelCaseToSnakeCase;
struct DefaultIfMissing;
struct DurationsAsCount;
struct EnumNamesOnly;
struct FailFast;
struct NoExtraFields;
struct NoOptionals;
struct SnakeCaseToCamelCase;
struct SnakeCaseToKebabCase;
struct SnakeCaseToPascalCase;
struct ToAllCaps;
struct UnderlyingEnums;
template <class... FieldTypes>
class NamedTuple;
template <class... Ps>
struct Processors;
}  // namespace rfl

namespace rfl::internal::parsing {

template <class T>
inline constexpr bool is_table_supported_processor_v =
    std::is_same_v<T, DefaultIfMissing> || std::is_same_v<T, NoExtraFields> ||
    std::is_same_v<T, NoOptionals> || std::is_same_v<T, FailFast> ||
    std::is_same_v<T, UnderlyingEnums> || std::is_same_v<T, EnumNamesOnly> ||
    std::is_same_v<T, AddTagsToVariants> ||
    std::is_same_v<T, AddNamespacedTagsToVariants> ||
    std::is_same_v<T, DurationsAsCount> || std::is_same_v<T, AllowRawPtrs> ||
    std::is_same_v<T, SnakeCaseToCamelCase> ||
    std::is_same_v<T, SnakeCaseToPascalCase> ||
    std::is_same_v<T, CamelCaseToSnakeCase> ||
    std::is_same_v<T, SnakeCaseToKebabCase> || std::is_same_v<T, ToAllCaps>;

template <class T>
struct is_table_supported_processor_pack
    : std::bool_constant<is_table_supported_processor_v<T>> {};

template <class... Ps>
struct is_table_supported_processor_pack<Processors<Ps...>>
    : std::bool_constant<
          (is_table_supported_processor_pack<Ps>::value && ...)> {};

template <class R>
concept ReaderSupportsUnsizedObjectReader =
    requires { typename R::UnsizedObjectReader; };

/// Requires explicit `use_table_struct_parser_ = true` on
/// `Parser<R, W, NamedTuple<>, ProcessorsType>` (not inherited from
/// `NamedTupleParser`) so custom `NamedTupleParser` subclasses are never
/// silently bypassed.
template <class R, class W, class ProcessorsType>
concept HasNamedTupleFormatPolicy =
    requires {
      {
        rfl::parsing::Parser<R, W, NamedTuple<>,
                             ProcessorsType>::use_table_struct_parser_
      } -> std::convertible_to<bool>;
      {
        rfl::parsing::Parser<R, W, NamedTuple<>,
                             ProcessorsType>::ignore_empty_containers_
      } -> std::convertible_to<bool>;
      {
        rfl::parsing::Parser<R, W, NamedTuple<>, ProcessorsType>::all_required_
      } -> std::convertible_to<bool>;
      {
        rfl::parsing::Parser<R, W, NamedTuple<>,
                             ProcessorsType>::no_field_names_
      } -> std::convertible_to<bool>;
    } &&
    rfl::parsing::Parser<R, W, NamedTuple<>,
                         ProcessorsType>::use_table_struct_parser_;

template <class T>
consteval bool table_fields_are_supported();

template <class FieldT>
consteval bool table_single_field_supported() {
  using U = std::remove_cvref_t<FieldT>;
  // Non-pointer-stripping traits keep raw pointers to wrappers (AllowRawPtrs)
  // on the ordinary pointer path; rfl::Field / rfl::ExtraFields fall back to
  // NamedTupleParser.
  if constexpr (is_field<U>::value || is_extra_fields<U>::value) {
    return false;
  } else if constexpr (is_flatten_field<U>::value) {
    using Sub = std::remove_cvref_t<typename U::Type>;
    return !std::is_pointer_v<Sub> && std::is_class_v<Sub> &&
           std::is_aggregate_v<Sub> && !std::is_union_v<Sub> &&
           table_fields_are_supported<Sub>();
  } else {
    return true;
  }
}

template <class T>
consteval bool table_fields_are_supported() {
  return decltype(apply_to_fields(
      std::declval<T&>(), []<class... Fields>(Fields&...) {
        return std::bool_constant<
            (true && ... && table_single_field_supported<Fields>())>{};
      }))::value;
}

template <class... Fields>
struct TableFieldTypeList {};

template <class... As, class... Bs>
consteval TableFieldTypeList<As..., Bs...> operator+(
    TableFieldTypeList<As...>, TableFieldTypeList<Bs...>) noexcept {
  return {};
}

template <class T>
struct flattened_field_type_list;

template <class FieldT>
struct single_field_type_list {
  using type = TableFieldTypeList<FieldT>;
};
template <class Sub>
struct single_field_type_list<Flatten<Sub>> {
  using type =
      typename flattened_field_type_list<typename Flatten<Sub>::Type>::type;
};
template <class Sub>
struct single_field_type_list<const Flatten<Sub>> {
  using type = typename flattened_field_type_list<
      const typename Flatten<Sub>::Type>::type;
};

template <class T>
struct flattened_field_type_list {
  using type = decltype(apply_to_fields(
      std::declval<T&>(), []<class... Fields>(Fields&...) {
        return (TableFieldTypeList<>{} + ... +
                typename single_field_type_list<Fields>::type{});
      }));
};

template <class T>
using flattened_field_type_list_t = typename flattened_field_type_list<T>::type;

template <class T>
inline void collect_flattened_offsets_impl(const T& _t,
                                           const unsigned char* _base,
                                           std::size_t* _out,
                                           std::size_t& _pos) noexcept {
  apply_to_fields(_t, [_base, _out, &_pos](const auto&... _refs) noexcept {
    const auto push = [_base, _out, &_pos](const auto& _ref) noexcept {
      using U = std::remove_cvref_t<decltype(_ref)>;
      if constexpr (is_flatten_field<U>::value) {
        collect_flattened_offsets_impl(_ref.value_, _base, _out, _pos);
      } else {
        const auto* p =
            reinterpret_cast<const unsigned char*>(std::addressof(_ref));
        _out[_pos++] = static_cast<std::size_t>(p - _base);
      }
    };
    (push(_refs), ...);
  });
}

/// Computes fixed byte offsets for all flattened fields of aggregate `T` once
/// per type (aggregates have no virtual bases, so offsets are invariant across
/// all instances of `T`), avoiding per-call `std::array<void*, N>` rebuilds.
template <class T>
inline const std::array<std::size_t, direct_flattened_field_count_v<T>>&
flattened_field_offsets() noexcept {
  static const auto offsets = []() noexcept {
    alignas(T) unsigned char dummy[sizeof(T)];
    const auto* obj = ptr_cast<const T*>(&dummy);
    std::array<std::size_t, direct_flattened_field_count_v<T>> out{};
    std::size_t pos = 0;
    collect_flattened_offsets_impl(*obj, dummy, out.data(), pos);
    return out;
  }();
  return offsets;
}

/// Type-erased per-field thunks parameterized ONLY by individual field type.
template <class R, class W, class FieldT, class ProcessorsType>
void read_uninit_field_erased(const R& _r, const typename R::InputVarType& _var,
                              std::string_view _name, void* _ptr, bool& _is_set,
                              rfl::parsing::FieldErrors<ProcessorsType>* _err,
                              std::size_t _reserve) {
  auto* p = static_cast<FieldT*>(_ptr);
  assign_uninitialized_view_field<R, W, FieldT*, ProcessorsType>(
      _r, _var, _name, p, _is_set, _err, _reserve);
}

template <class R, class W, class FieldT, class ProcessorsType>
void read_default_field_erased(const R& _r,
                               const typename R::InputVarType& _var,
                               std::string_view _name, void* _ptr,
                               rfl::parsing::FieldErrors<ProcessorsType>* _err) {
  auto* p = static_cast<FieldT*>(_ptr);
  assign_default_view_field<R, W, FieldT*, ProcessorsType>(_r, _var, _name, p,
                                                           _err);
}

template <bool _ignore_empty, bool _all_req, bool _defaults,
          class ProcessorsType, class FieldT>
void handle_missing_field_erased(
    std::string_view _name, bool _found, void* _ptr, bool* _set,
    rfl::parsing::FieldErrors<ProcessorsType>* _err, bool _val_init) noexcept {
  handle_missing_named_tuple_field<_ignore_empty, _all_req, _defaults,
                                   ProcessorsType>(
      _name, _found, static_cast<FieldT*>(_ptr), _set, _err, _val_init);
}

template <class FieldT>
void destroy_field_erased(void* _ptr) noexcept {
  destroy_field_if_set(true, static_cast<FieldT*>(_ptr));
}

template <class R, class W, bool _ignore_empty, bool _all_req,
          class ProcessorsType, class FieldT>
void write_field_erased(const W& _w, std::string_view _name, const void* _ptr,
                        typename W::OutputObjectType* _out) {
  write_named_tuple_field<R, W, _ignore_empty, _all_req, false, ProcessorsType>(
      _w, _name, static_cast<const std::remove_cvref_t<FieldT>*>(_ptr), _out);
}

template <class R, class W, class ProcessorsType, class FieldT>
void build_schema_field_erased(
    std::map<std::string, rfl::parsing::schema::Type>* _defs,
    rfl::parsing::schema::Type::Object* _schema, std::string_view _name,
    const void* _def_ptr) {
  using U = std::remove_cvref_t<FieldT>;
  build_named_tuple_field_schema<R, W, false, ProcessorsType, U>(
      _defs, _schema, _name, static_cast<const U*>(_def_ptr));
}

using DestroyFn = void (*)(void*) noexcept;
struct EmptyStorage {};

inline constexpr std::uint64_t make_all_bits_mask(std::size_t _size) noexcept {
  return (_size >= 64) ? ~std::uint64_t{0} : ((std::uint64_t{1} << _size) - 1);
}

/// RAII scope guard destroying every placement-constructed field in `buf`.
template <bool _use_bitmask>
struct UninitFieldsGuard {
  const DestroyFn* destroy_fns;
  std::conditional_t<_use_bitmask, const std::uint64_t*, const bool*> set;
  unsigned char* base_ptr;
  const std::size_t* offsets;
  std::size_t size;

  ~UninitFieldsGuard() noexcept {
    if constexpr (_use_bitmask) {
      for (std::uint64_t mask = *set; mask != 0; mask &= (mask - 1)) {
        const auto i = static_cast<std::size_t>(std::countr_zero(mask));
        if (destroy_fns[i]) {
          destroy_fns[i](static_cast<void*>(base_ptr + offsets[i]));
        }
      }
    } else {
      for (std::size_t i = 0; i < size; ++i) {
        if (set[i] && destroy_fns[i]) {
          destroy_fns[i](static_cast<void*>(base_ptr + offsets[i]));
        }
      }
    }
  }
};

template <class W, class P>
void write_table_common(
    const W& _w, const std::string_view* _names,
    void (*const* _write_fns)(const W&, std::string_view, const void*,
                              typename W::OutputObjectType*),
    const unsigned char* _base_ptr, const std::size_t* _offsets,
    std::size_t _size, const P& _parent) {
  auto obj = rfl::parsing::Parent<W>::add_object(_w, _size, _parent);
  for (std::size_t i = 0; i < _size; ++i) {
    _write_fns[i](_w, _names[i],
                  static_cast<const void*>(_base_ptr + _offsets[i]), &obj);
  }
  _w.end_object(&obj);
}

template <class ProcessorsType>
rfl::parsing::schema::Type to_schema_table_common(
    std::map<std::string, rfl::parsing::schema::Type>* _defs,
    const std::string_view* _names,
    void (*const* _schema_fns)(
        std::map<std::string, rfl::parsing::schema::Type>*,
        rfl::parsing::schema::Type::Object*, std::string_view, const void*),
    const unsigned char* _def_base, const std::size_t* _offsets,
    std::size_t _size) {
  rfl::parsing::schema::Type::Object schema;
  for (std::size_t i = 0; i < _size; ++i) {
    const void* const p =
        _def_base ? static_cast<const void*>(_def_base + _offsets[i]) : nullptr;
    _schema_fns[i](_defs, &schema, _names[i], p);
  }
  if constexpr (no_extra_fields_v<ProcessorsType>) {
    schema.additional_properties_ = false;
  }
  return rfl::parsing::schema::Type{schema};
}

}  // namespace rfl::internal::parsing

namespace rfl::parsing {

template <class R, class W, class T, class ProcessorsType>
inline constexpr bool can_use_table_struct_parser_v = [] {
  using Type = std::remove_cvref_t<T>;
  if constexpr (!internal::parsing::HasNamedTupleFormatPolicy<
                    R, W, ProcessorsType> ||
                !std::is_class_v<Type> || !std::is_aggregate_v<Type> ||
                std::is_union_v<Type> ||
                !internal::parsing::is_table_supported_processor_pack<
                    ProcessorsType>::value) {
    return false;
  } else if constexpr (Parser<R, W, NamedTuple<>,
                              ProcessorsType>::no_field_names_ ||
                       !internal::parsing::table_fields_are_supported<Type>()) {
    return false;
  } else {
    constexpr std::size_t count =
        internal::direct_flattened_field_count_v<Type>;
    return count > 0 &&
           count >= static_cast<std::size_t>(RFL_TABLE_PARSER_MIN_FIELDS);
  }
}();

// Consolidated non-`T`-parameterized descriptor-table view reader.
// `ViewType` is intentionally omitted here and on `SizedTableViewReader` to
// keep the reader independent of `rfl::NamedTuple` and `T`. External readers
// needing field metadata should query `field_names()` / `size()`, or
// specialize `Parser<R, W, NamedTuple<...>>` to opt out of `TableStructParser`.
template <class R, class W, class ProcessorsType, bool _with_default = false,
          bool _use_bitmask = true>
class TableViewReader {
 public:
  using InputVarType = typename R::InputVarType;
  using ReadFn = std::conditional_t<
      _with_default,
      void (*)(const R&, const InputVarType&, std::string_view, void*,
               FieldErrors<ProcessorsType>*),
      void (*)(const R&, const InputVarType&, std::string_view, void*, bool&,
               FieldErrors<ProcessorsType>*, std::size_t)>;
  using FoundPtr = std::conditional_t<_use_bitmask, std::uint64_t*, bool*>;
  using SetPtr = std::conditional_t<
      _with_default, internal::parsing::EmptyStorage,
      std::conditional_t<_use_bitmask, std::uint64_t*, bool*>>;
  using FoundCountPtr =
      std::conditional_t<_use_bitmask, internal::parsing::EmptyStorage,
                         std::size_t*>;

  struct Desc {
    const std::string_view* names;
    const internal::parsing::FieldNameEntry* lookup;
    const ReadFn* read_fns;
    std::size_t size;
  };

  TableViewReader(const R* _r, const Desc* _desc, unsigned char* _base_ptr,
                  const std::size_t* _offsets, FoundPtr _found, SetPtr _set,
                  FoundCountPtr _found_count,
                  FieldErrors<ProcessorsType>* _errors) noexcept
      : r_(_r), desc_(_desc), base_ptr_(_base_ptr), offsets_(_offsets),
        found_(_found), set_(_set), found_count_(_found_count),
        errors_(_errors), next_idx_(0) {}

  ~TableViewReader() = default;

  std::span<const std::string_view> field_names() const noexcept {
    return {desc_->names, desc_->size};
  }

  RFL_ALWAYS_INLINE void read(const std::string_view& _name,
                              const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    const int idx = lookup_index(_name);
    const bool assigned =
        idx >= 0 && assign_if_unseen(static_cast<std::size_t>(idx), _var);
    if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      if (!assigned) {
        std::stringstream stream;
        if constexpr (_with_default) {
          stream << "Value named '" << std::string(_name)
                 << "' not used. Remove the rfl::NoExtraFields processor or "
                    "add rfl::ExtraFields to avoid this error message.";
        } else {
          stream << "Value named '" << _name << "' not used.";
        }
        errors_->emplace_back(
            make_nested_error(std::string(_name), stream.str()));
      }
    }
  }

  void read(int _index, const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    const bool assigned =
        _index >= 0 && static_cast<std::size_t>(_index) < desc_->size &&
        assign_if_unseen(static_cast<std::size_t>(_index), _var);
    if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      if (!assigned) {
        std::stringstream stream;
        stream << "Value named '" << _index << "' not used.";
        errors_->emplace_back(
            make_nested_error(static_cast<std::size_t>(_index), stream.str()));
      }
    }
  }

  template <int i>
  void read(std::integral_constant<int, i>, const InputVarType& _var) const {
    read(i, _var);
  }

 private:
  RFL_ALWAYS_INLINE int lookup_index(std::string_view _name) const noexcept {
    if (next_idx_ < desc_->size && desc_->names[next_idx_] == _name)
        [[likely]] {
      return static_cast<int>(next_idx_++);
    }
    const int found_idx =
        internal::parsing::find_in_table(desc_->lookup, desc_->size, _name);
    if (found_idx >= 0) next_idx_ = static_cast<std::size_t>(found_idx) + 1;
    return found_idx;
  }

  RFL_ALWAYS_INLINE bool assign_if_unseen(std::size_t _u,
                                          const InputVarType& _var) const {
    if constexpr (_use_bitmask) {
      const std::uint64_t bit = std::uint64_t{1} << _u;
      if ((*found_ & bit) != 0) return false;
      *found_ |= bit;
    } else {
      if (found_[_u]) return false;
      found_[_u] = true;
      ++(*found_count_);
    }
    void* const field_ptr = static_cast<void*>(base_ptr_ + offsets_[_u]);
    if constexpr (_with_default) {
      desc_->read_fns[_u](*r_, _var, desc_->names[_u], field_ptr, errors_);
    } else if constexpr (_use_bitmask) {
      bool is_set = false;
      desc_->read_fns[_u](*r_, _var, desc_->names[_u], field_ptr, is_set,
                          errors_, desc_->size);
      if (is_set) [[likely]] *set_ |= (std::uint64_t{1} << _u);
    } else {
      desc_->read_fns[_u](*r_, _var, desc_->names[_u], field_ptr, set_[_u],
                          errors_, desc_->size);
    }
    return true;
  }

 protected:
  const R* r_;
  const Desc* desc_;
  unsigned char* base_ptr_;
  const std::size_t* offsets_;
  FoundPtr found_;
  [[no_unique_address]] SetPtr set_;
  [[no_unique_address]] FoundCountPtr found_count_;
  FieldErrors<ProcessorsType>* errors_;
  mutable std::size_t next_idx_;
};

/// Thin static-size adapter for readers requiring compile-time `size()`.
/// `ViewType` is intentionally omitted; use `field_names()` / `size()` instead.
template <class R, class W, class ProcessorsType, std::size_t _size,
          bool _with_default = false, bool _use_bitmask = true>
class SizedTableViewReader
    : public TableViewReader<R, W, ProcessorsType, _with_default,
                             _use_bitmask> {
 public:
  using TableViewReader<R, W, ProcessorsType, _with_default,
                        _use_bitmask>::TableViewReader;
  static constexpr std::size_t size() noexcept { return _size; }
};

}  // namespace rfl::parsing

namespace rfl::internal::parsing {

template <class R, class W, class ProcessorsType, bool _with_default,
          bool _use_bitmask, class ReaderObj>
std::optional<Error> read_table_common(
    const R& _r, const typename R::InputVarType& _var,
    const typename rfl::parsing::TableViewReader<
        R, W, ProcessorsType, _with_default, _use_bitmask>::Desc& _desc,
    void (*const* _missing_fns)(std::string_view, bool, void*, bool*,
                                rfl::parsing::FieldErrors<ProcessorsType>*,
                                bool) noexcept,
    unsigned char* _base_ptr, const std::size_t* _offsets,
    typename rfl::parsing::TableViewReader<R, W, ProcessorsType, _with_default,
                                           _use_bitmask>::FoundPtr _found,
    typename rfl::parsing::TableViewReader<R, W, ProcessorsType, _with_default,
                                           _use_bitmask>::SetPtr _set) {
  rfl::parsing::FieldErrors<ProcessorsType> errors;
  auto obj = _r.to_object(_var);
  if (!obj) [[unlikely]] return obj.error();

  std::conditional_t<_use_bitmask, EmptyStorage, std::size_t> found_count{};
  const auto reader = [&] {
    if constexpr (_use_bitmask) {
      return ReaderObj(&_r, &_desc, _base_ptr, _offsets, _found, _set,
                       EmptyStorage{}, &errors);
    } else {
      return ReaderObj(&_r, &_desc, _base_ptr, _offsets, _found, _set,
                       &found_count, &errors);
    }
  }();
  if (auto err = _r.read_object(reader, *obj)) [[unlikely]] return err;

  if (_missing_fns != nullptr &&
      !rfl::parsing::should_stop_on_field_error<ProcessorsType>(errors)) {
    const auto handle_one = [&](std::size_t i) {
      void* const fptr = static_cast<void*>(_base_ptr + _offsets[i]);
      if constexpr (_with_default) {
        _missing_fns[i](_desc.names[i], false, fptr, nullptr, &errors, false);
      } else if constexpr (_use_bitmask) {
        bool is_set = false;
        _missing_fns[i](_desc.names[i], false, fptr, &is_set, &errors, false);
        if (is_set) *_set |= (std::uint64_t{1} << i);
      } else {
        _missing_fns[i](_desc.names[i], false, fptr, &_set[i], &errors, false);
      }
    };
    if constexpr (_use_bitmask) {
      for (std::uint64_t missing = make_all_bits_mask(_desc.size) & ~(*_found);
           missing != 0 &&
           !rfl::parsing::should_stop_on_field_error<ProcessorsType>(errors);
           missing &= (missing - 1)) {
        handle_one(static_cast<std::size_t>(std::countr_zero(missing)));
      }
    } else if (found_count < _desc.size) {
      for (std::size_t i = 0; i < _desc.size; ++i) {
        if (!_found[i]) handle_one(i);
      }
    }
  }

  if constexpr (fail_fast_v<ProcessorsType>) {
    return std::move(errors).error();
  } else {
    return errors.size() != 0
               ? std::optional<Error>(
                     rfl::parsing::to_single_error(std::move(errors)))
               : std::nullopt;
  }
}

}  // namespace rfl::internal::parsing

namespace rfl::parsing {

/// Type-erased descriptor-table aggregate parser bypassing `rfl::NamedTuple`,
/// `rfl::Tuple`, and per-struct `ViewReader` / `NamedTupleParser` templates.
template <class R, class W, class T, class ProcessorsType>
struct TableStructParser {
  using InputVarType = typename R::InputVarType;
  using Names = internal::DirectFieldNames<
      T, internal::direct_case_pipeline_t<ProcessorsType>>;
  using FieldList = internal::parsing::flattened_field_type_list_t<T>;

  static constexpr std::size_t size_ = Names::size_;
  static constexpr bool use_bitmask_ = (size_ <= 64);
  using FormatNTParser = Parser<R, W, NamedTuple<>, ProcessorsType>;
  static constexpr bool ignore_empty_ = FormatNTParser::ignore_empty_containers_;
  static constexpr bool all_required_ = FormatNTParser::all_required_;

  template <bool _with_default>
  using ViewReaderFor =
      TableViewReader<R, W, ProcessorsType, _with_default, use_bitmask_>;
  template <bool _with_default>
  using ReaderFor = std::conditional_t<
      internal::parsing::ReaderSupportsUnsizedObjectReader<R>,
      ViewReaderFor<_with_default>,
      SizedTableViewReader<R, W, ProcessorsType, size_, _with_default,
                           use_bitmask_>>;

  using MissingFn = void (*)(std::string_view, bool, void*, bool*,
                             FieldErrors<ProcessorsType>*, bool) noexcept;
  using DestroyFn = internal::parsing::DestroyFn;
  using FlagsT =
      std::conditional_t<use_bitmask_, std::uint64_t, std::array<bool, size_>>;

 private:
  static constexpr void check_schemaful_processors() noexcept {
    static_assert(!internal::no_extra_fields_v<ProcessorsType> ||
                      !schemaful::IsSchemafulReader<R>,
                  "Passing rfl::NoExtraFields to a schemaful format does not "
                  "make sense, because schemaful formats cannot have extra "
                  "fields.");
  }

  template <class Fn, class MakeFn, class... Fields>
  static consteval auto make_table(
      internal::parsing::TableFieldTypeList<Fields...>, MakeFn _make) noexcept {
    return std::array<Fn, sizeof...(Fields)>{
        _make.template operator()<Fields>()...};
  }

  template <class... Fields>
  static consteval bool has_non_trivial_dtor(
      internal::parsing::TableFieldTypeList<Fields...>) noexcept {
    return (false || ... ||
            !std::is_trivially_destructible_v<
                std::remove_all_extents_t<Fields>>);
  }

  static auto flags_ptr(FlagsT& _f) noexcept {
    if constexpr (use_bitmask_) return &_f;
    else return _f.data();
  }

 public:
  static Result<T> read(const R& _r, const InputVarType& _var) {
    check_schemaful_processors();
    static constexpr auto read_fns =
        make_table<typename ViewReaderFor<false>::ReadFn>(
            FieldList{}, []<class F>() consteval {
              return &internal::parsing::read_uninit_field_erased<
                  R, W, F, ProcessorsType>;
            });
    static constexpr auto missing_fns = make_table<MissingFn>(
        FieldList{}, []<class F>() consteval {
          return &internal::parsing::handle_missing_field_erased<
              ignore_empty_, all_required_, false, ProcessorsType, F>;
        });
    static constexpr typename ViewReaderFor<false>::Desc desc{
        Names::names_.data(), Names::lookup_.data(), read_fns.data(), size_};

    const auto& offsets = internal::parsing::flattened_field_offsets<T>();
    alignas(T) unsigned char buf[sizeof(T)];
    FlagsT found{}, set{};

    const auto guard = [&] {
      if constexpr (has_non_trivial_dtor(FieldList{})) {
        static constexpr auto destroy_fns = make_table<DestroyFn>(
            FieldList{}, []<class F>() consteval -> DestroyFn {
              return std::is_trivially_destructible_v<
                         std::remove_all_extents_t<F>>
                         ? nullptr
                         : &internal::parsing::destroy_field_erased<F>;
            });
        return internal::parsing::UninitFieldsGuard<use_bitmask_>{
            destroy_fns.data(), flags_ptr(set), buf, offsets.data(), size_};
      } else {
        return internal::parsing::EmptyStorage{};
      }
    }();
    (void)guard;

    auto err = internal::parsing::read_table_common<
        R, W, ProcessorsType, false, use_bitmask_, ReaderFor<false>>(
        _r, _var, desc, missing_fns.data(), buf, offsets.data(),
        flags_ptr(found), flags_ptr(set));
    if (err) [[unlikely]] return error(std::move(*err));
    return Result<T>(std::move(*internal::ptr_cast<T*>(&buf)));
  }

  static std::optional<Error> read_with_default(const R& _r,
                                                const InputVarType& _var,
                                                T* _target) noexcept {
    check_schemaful_processors();
    static constexpr auto read_fns =
        make_table<typename ViewReaderFor<true>::ReadFn>(
            FieldList{}, []<class F>() consteval {
              return &internal::parsing::read_default_field_erased<
                  R, W, F, ProcessorsType>;
            });
    static constexpr typename ViewReaderFor<true>::Desc desc{
        Names::names_.data(), Names::lookup_.data(), read_fns.data(), size_};

    try {
      const auto& offsets = internal::parsing::flattened_field_offsets<T>();
      FlagsT found{};
      const MissingFn* const missing_ptr = []() -> const MissingFn* {
        if constexpr (internal::has_default_val_v<T> &&
                      !internal::default_if_missing_v<ProcessorsType>) {
          static constexpr auto missing_fns = make_table<MissingFn>(
              FieldList{}, []<class F>() consteval {
                return &internal::parsing::handle_missing_field_erased<
                    ignore_empty_, all_required_, true, ProcessorsType, F>;
              });
          return missing_fns.data();
        } else {
          return nullptr;
        }
      }();
      return internal::parsing::read_table_common<
          R, W, ProcessorsType, true, use_bitmask_, ReaderFor<true>>(
          _r, _var, desc, missing_ptr, reinterpret_cast<unsigned char*>(_target),
          offsets.data(), flags_ptr(found), internal::parsing::EmptyStorage{});
    } catch (const std::exception& _e) {
      return Error(_e.what());
    } catch (...) {
      return Error("Unknown exception while reading struct with defaults.");
    }
  }

  template <class P>
  static void write(const W& _w, const T& _var, const P& _parent) {
    using WriteFn = void (*)(const W&, std::string_view, const void*,
                             typename W::OutputObjectType*);
    static constexpr auto write_fns =
        make_table<WriteFn>(FieldList{}, []<class F>() consteval {
          return &internal::parsing::write_field_erased<
              R, W, ignore_empty_, all_required_, ProcessorsType, F>;
        });
    const auto& offsets = internal::parsing::flattened_field_offsets<T>();
    internal::parsing::write_table_common(
        _w, Names::names_.data(), write_fns.data(),
        reinterpret_cast<const unsigned char*>(std::addressof(_var)),
        offsets.data(), size_, _parent);
  }

  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions,
      const T* _defaults = nullptr) {
    using SchemaFn = void (*)(std::map<std::string, schema::Type>*,
                              schema::Type::Object*, std::string_view,
                              const void*);
    static constexpr auto schema_fns =
        make_table<SchemaFn>(FieldList{}, []<class F>() consteval {
          return &internal::parsing::build_schema_field_erased<
              R, W, ProcessorsType, F>;
        });
    const auto& offsets = internal::parsing::flattened_field_offsets<T>();
    return internal::parsing::to_schema_table_common<ProcessorsType>(
        _definitions, Names::names_.data(), schema_fns.data(),
        _defaults ? reinterpret_cast<const unsigned char*>(_defaults) : nullptr,
        offsets.data(), size_);
  }
};

}  // namespace rfl::parsing

#endif
