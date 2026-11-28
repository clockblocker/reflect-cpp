#ifndef RFL_PARSING_TABLESTRUCTPARSER_HPP_
#define RFL_PARSING_TABLESTRUCTPARSER_HPP_

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
#include "../CamelCaseToSnakeCase.hpp"
#include "../DefaultIfMissing.hpp"
#include "../DurationsAsCount.hpp"
#include "../EnumNamesOnly.hpp"
#include "../FailFast.hpp"
#include "../Flatten.hpp"
#include "../Metadata.hpp"
#include "../NamedTuple.hpp"
#include "../NoExtraFields.hpp"
#include "../NoFieldNames.hpp"
#include "../NoOptionals.hpp"
#include "../Processors.hpp"
#include "../Result.hpp"
#include "../SnakeCaseToCamelCase.hpp"
#include "../SnakeCaseToKebabCase.hpp"
#include "../SnakeCaseToPascalCase.hpp"
#include "../ToAllCaps.hpp"
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
#include "find_field_index.hpp"
#include "schema/Type.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "to_single_error_message.hpp"
#include "wrap_error.hpp"

namespace rfl::parsing {

namespace detail {

template <class T>
struct is_table_supported_processor : std::false_type {};

#define RFL_TABLE_SUPPORTED_PROCESSOR(Type) \
  template <>                               \
  struct is_table_supported_processor<Type> : std::true_type {}

RFL_TABLE_SUPPORTED_PROCESSOR(DefaultIfMissing);
RFL_TABLE_SUPPORTED_PROCESSOR(NoExtraFields);
RFL_TABLE_SUPPORTED_PROCESSOR(NoFieldNames);
RFL_TABLE_SUPPORTED_PROCESSOR(NoOptionals);
RFL_TABLE_SUPPORTED_PROCESSOR(FailFast);
RFL_TABLE_SUPPORTED_PROCESSOR(UnderlyingEnums);
RFL_TABLE_SUPPORTED_PROCESSOR(EnumNamesOnly);
RFL_TABLE_SUPPORTED_PROCESSOR(AddTagsToVariants);
RFL_TABLE_SUPPORTED_PROCESSOR(AddNamespacedTagsToVariants);
RFL_TABLE_SUPPORTED_PROCESSOR(DurationsAsCount);
RFL_TABLE_SUPPORTED_PROCESSOR(AllowRawPtrs);
RFL_TABLE_SUPPORTED_PROCESSOR(SnakeCaseToCamelCase);
RFL_TABLE_SUPPORTED_PROCESSOR(SnakeCaseToPascalCase);
RFL_TABLE_SUPPORTED_PROCESSOR(CamelCaseToSnakeCase);
RFL_TABLE_SUPPORTED_PROCESSOR(SnakeCaseToKebabCase);
RFL_TABLE_SUPPORTED_PROCESSOR(ToAllCaps);

#undef RFL_TABLE_SUPPORTED_PROCESSOR

template <class T>
struct is_table_supported_processor_pack : is_table_supported_processor<T> {};

template <class... Ps>
struct is_table_supported_processor_pack<Processors<Ps...>>
    : std::bool_constant<
          (is_table_supported_processor_pack<Ps>::value && ...)> {};

template <class R>
concept ReaderSupportsUnsizedObjectReader =
    requires { typename R::UnsizedObjectReader; };

template <class R, class W, class ProcessorsType>
concept HasNamedTupleFormatPolicy = requires {
  {
    Parser<R, W, NamedTuple<>, ProcessorsType>::ignore_empty_containers_
  } -> std::convertible_to<bool>;
  {
    Parser<R, W, NamedTuple<>, ProcessorsType>::all_required_
  } -> std::convertible_to<bool>;
  {
    Parser<R, W, NamedTuple<>, ProcessorsType>::no_field_names_
  } -> std::convertible_to<bool>;
};

template <class T>
consteval bool table_fields_are_supported();

template <class FieldT>
consteval bool table_single_field_supported() {
  using U = std::remove_cvref_t<FieldT>;
  if constexpr (internal::is_field_v<U>) {
    return false;
  } else if constexpr (internal::is_flatten_field_v<U>) {
    using Sub = std::remove_cvref_t<typename U::Type>;
    if constexpr (std::is_pointer_v<Sub> || !std::is_class_v<Sub> ||
                  !std::is_aggregate_v<Sub> || std::is_union_v<Sub>) {
      return false;
    } else {
      return table_fields_are_supported<Sub>();
    }
  } else {
    return true;
  }
}

template <class T>
consteval bool table_fields_are_supported() {
  using Result = decltype(internal::apply_to_fields(
      std::declval<T&>(), []<class... Fields>(Fields&...) {
        return std::bool_constant<
            (true && ... && table_single_field_supported<Fields>())>{};
      }));
  return Result::value;
}

template <class... Fields>
struct TableFieldTypeList {};

template <class A, class B>
struct concat_type_lists;

template <class... As, class... Bs>
struct concat_type_lists<TableFieldTypeList<As...>, TableFieldTypeList<Bs...>> {
  using type = TableFieldTypeList<As..., Bs...>;
};

template <class... Lists>
struct concat_all_type_lists;

template <>
struct concat_all_type_lists<> {
  using type = TableFieldTypeList<>;
};

template <class L>
struct concat_all_type_lists<L> {
  using type = L;
};

template <class L1, class L2, class... Rest>
struct concat_all_type_lists<L1, L2, Rest...> {
  using type = typename concat_all_type_lists<
      typename concat_type_lists<L1, L2>::type, Rest...>::type;
};

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
  using type = decltype(internal::apply_to_fields(
      std::declval<T&>(), []<class... Fields>(Fields&...) {
        return typename concat_all_type_lists<
            typename single_field_type_list<Fields>::type...>::type{};
      }));
};

template <class T>
using flattened_field_type_list_t = typename flattened_field_type_list<T>::type;

template <class FieldList>
struct extra_fields_info;

template <class... Fields>
struct extra_fields_info<TableFieldTypeList<Fields...>> {
  static consteval int count() {
    return (0 + ... +
            (internal::is_extra_fields_v<std::remove_cvref_t<Fields>> ? 1 : 0));
  }

  static consteval int pos() {
    int result = -1;
    int idx = 0;
    ((internal::is_extra_fields_v<std::remove_cvref_t<Fields>>
          ? (result = idx)
          : 0,
      ++idx),
     ...);
    return result;
  }

  static constexpr int count_ = count();
  static constexpr int pos_ = pos();
};

template <class T>
inline void collect_flattened_mutable_ptrs_impl(T& _t, void** _out,
                                                std::size_t& _pos) noexcept {
  internal::apply_to_fields(_t, [_out, &_pos](auto&... _refs) noexcept {
    const auto push = [_out, &_pos](auto& _ref) noexcept {
      using U = std::remove_cvref_t<decltype(_ref)>;
      if constexpr (internal::is_flatten_field_v<U>) {
        collect_flattened_mutable_ptrs_impl(_ref.value_, _out, _pos);
      } else {
        _out[_pos++] =
            const_cast<void*>(static_cast<const void*>(std::addressof(_ref)));
      }
    };
    (push(_refs), ...);
  });
}

template <class T>
inline void collect_flattened_const_ptrs_impl(const T& _t, const void** _out,
                                              std::size_t& _pos) noexcept {
  internal::apply_to_fields(_t, [_out, &_pos](const auto&... _refs) noexcept {
    const auto push = [_out, &_pos](const auto& _ref) noexcept {
      using U = std::remove_cvref_t<decltype(_ref)>;
      if constexpr (internal::is_flatten_field_v<U>) {
        collect_flattened_const_ptrs_impl(_ref.value_, _out, _pos);
      } else {
        _out[_pos++] = static_cast<const void*>(std::addressof(_ref));
      }
    };
    (push(_refs), ...);
  });
}

template <class T>
inline auto collect_flattened_mutable_ptrs(T& _t) noexcept {
  std::array<void*, internal::direct_flattened_field_count_v<T>> ptrs{};
  std::size_t pos = 0;
  collect_flattened_mutable_ptrs_impl(_t, ptrs.data(), pos);
  return ptrs;
}

template <class T>
inline auto collect_flattened_const_ptrs(const T& _t) noexcept {
  std::array<const void*, internal::direct_flattened_field_count_v<T>> ptrs{};
  std::size_t pos = 0;
  collect_flattened_const_ptrs_impl(_t, ptrs.data(), pos);
  return ptrs;
}

/// Shared lookup helper operating on descriptor table slices (never templated
/// on `T` or `N`).
inline int find_in_table(const FieldNameEntry* _lookup, const std::size_t _size,
                         const std::string_view _name) noexcept {
  if (_size <= 8) {
    for (std::size_t i = 0; i < _size; ++i) {
      if (_lookup[i].name == _name) {
        return _lookup[i].field_idx;
      }
    }
    return -1;
  }
  std::size_t lo = 0;
  std::size_t hi = _size;
  while (lo < hi) {
    const std::size_t mid = lo + (hi - lo) / 2;
    if (_lookup[mid].name < _name) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  return (lo < _size && _lookup[lo].name == _name) ? _lookup[lo].field_idx : -1;
}

/// Type-erased per-field thunks parameterized ONLY by individual field type
/// (never by enclosing struct `T`, `NamedTuple`, or `Tuple`).

template <class R, class W, class FieldT, class ProcessorsType>
void read_uninit_field_erased(const R& _r,
                              const typename R::InputVarType& _var,
                              const std::string_view _name, void* _field_ptr,
                              bool& _is_set,
                              FieldErrors<ProcessorsType>* _errors,
                              const std::size_t _reserve_errors) {
  auto* ptr = static_cast<FieldT*>(_field_ptr);
  detail::assign_uninitialized_view_field<R, W, FieldT*, ProcessorsType>(
      _r, _var, _name, ptr, _is_set, _errors, _reserve_errors);
}

template <class R, class W, class ExtraFieldsT, class ProcessorsType>
void read_extra_uninit_erased(const R& _r, const std::string_view _current_name,
                              const typename R::InputVarType& _var,
                              void* _extra_ptr, bool& _is_found, bool& _is_set,
                              FieldErrors<ProcessorsType>* _errors) {
  static_assert(!std::is_const_v<ExtraFieldsT>,
                "rfl::ExtraFields field cannot be const.");
  using CleanExtra = std::remove_cvref_t<ExtraFieldsT>;
  auto* extra = static_cast<CleanExtra*>(_extra_ptr);
  detail::assign_extra_uninitialized_field<R, W, CleanExtra, ProcessorsType>(
      _r, _current_name, _var, extra, _is_found, _is_set, _errors);
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

template <class R, class W, class ExtraFieldsT, class ProcessorsType>
void read_extra_default_erased(const R& _r,
                               const std::string_view _current_name,
                               const typename R::InputVarType& _var,
                               void* _extra_ptr,
                               FieldErrors<ProcessorsType>* _errors) {
  static_assert(!std::is_const_v<ExtraFieldsT>,
                "rfl::ExtraFields field cannot be const.");
  using CleanExtra = std::remove_cvref_t<ExtraFieldsT>;
  auto* extra = static_cast<CleanExtra*>(_extra_ptr);
  detail::assign_extra_default_field<R, W, CleanExtra, ProcessorsType>(
      _r, _current_name, _var, extra, _errors);
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

template <class ExtraFieldsT>
std::size_t extra_fields_size_erased(const void* _field_ptr) noexcept {
  using CleanExtra = std::remove_cvref_t<ExtraFieldsT>;
  return static_cast<const CleanExtra*>(_field_ptr)->size();
}

template <class R, class W, bool _no_field_names, class ProcessorsType,
          class FieldT, class SchemaType>
void build_schema_field_erased(
    std::map<std::string, schema::Type>* _definitions, SchemaType* _schema,
    const std::string_view _wire_name, const std::string_view _raw_name,
    const void* _default_field_ptr,
    const std::span<const FieldMeta> _field_meta) {
  using U = std::remove_cvref_t<FieldT>;
  const U* ptr = static_cast<const U*>(_default_field_ptr);
  detail::build_named_tuple_field_schema<R, W, _no_field_names, ProcessorsType,
                                         U>(_definitions, _schema, _wire_name,
                                            _raw_name, ptr, _field_meta);
}

template <class R, class W, class ExtraFieldsT, class ProcessorsType>
schema::Type build_extra_schema_erased(
    std::map<std::string, schema::Type>* _definitions) {
  using CleanExtra = std::remove_cvref_t<ExtraFieldsT>;
  using U = std::remove_cvref_t<typename CleanExtra::Type>;
  return Parser<R, W, U, ProcessorsType>::to_schema(_definitions);
}

template <int _pos, class FieldList>
struct nth_type_in_list;

template <int _pos, class Head, class... Tail>
struct nth_type_in_list<_pos, TableFieldTypeList<Head, Tail...>>
    : nth_type_in_list<_pos - 1, TableFieldTypeList<Tail...>> {};

template <class Head, class... Tail>
struct nth_type_in_list<0, TableFieldTypeList<Head, Tail...>> {
  using type = Head;
};

}  // namespace detail

template <class R, class W, class T, class ProcessorsType>
inline constexpr bool can_use_table_struct_parser_v = [] {
  using Type = std::remove_cvref_t<T>;
  if constexpr (!detail::HasNamedTupleFormatPolicy<R, W, ProcessorsType> ||
                !std::is_class_v<Type> || !std::is_aggregate_v<Type> ||
                std::is_union_v<Type> ||
                !detail::is_table_supported_processor_pack<
                    ProcessorsType>::value) {
    return false;
  } else if constexpr (!detail::table_fields_are_supported<Type>()) {
    return false;
  } else {
    return internal::direct_flattened_field_count_v<Type> > 0;
  }
}();

// ----------------------------------------------------------------------------
// Non-`T`-parameterized descriptor-table view readers.
// Instantiated ONCE per `(R, W, ProcessorsType)` across the entire TU!
// ----------------------------------------------------------------------------

template <class R, class W, class ProcessorsType>
class TableViewReader {
 public:
  using InputVarType = typename R::InputVarType;
  using ReadUninitFn = void (*)(const R&, const InputVarType&, std::string_view,
                                void*, bool&, FieldErrors<ProcessorsType>*,
                                std::size_t);
  using ReadExtraUninitFn = void (*)(const R&, std::string_view,
                                     const InputVarType&, void*, bool&, bool&,
                                     FieldErrors<ProcessorsType>*);

  struct Desc {
    const std::string_view* names;
    const detail::FieldNameEntry* lookup;
    const ReadUninitFn* read_fns;
    std::size_t size;
    int pos_extra_fields;
    ReadExtraUninitFn read_extra_fn;
  };

  TableViewReader(const R* _r, const Desc* _desc, void* const* _field_ptrs,
                  bool* _found, bool* _set, std::size_t* _found_count,
                  FieldErrors<ProcessorsType>* _errors) noexcept
      : r_(_r),
        desc_(_desc),
        field_ptrs_(_field_ptrs),
        found_(_found),
        set_(_set),
        found_count_(_found_count),
        errors_(_errors),
        next_idx_(0) {}

  ~TableViewReader() = default;

  std::span<const std::string_view> field_names() const noexcept {
    return {desc_->names, desc_->size};
  }

  RFL_ALWAYS_INLINE void read(const std::string_view& _name,
                              const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    bool already_assigned = false;
    int idx = -1;
    if (next_idx_ < desc_->size &&
        static_cast<int>(next_idx_) != desc_->pos_extra_fields &&
        desc_->names[next_idx_] == _name) [[likely]] {
      idx = static_cast<int>(next_idx_);
      ++next_idx_;
    } else {
      const int found_idx =
          detail::find_in_table(desc_->lookup, desc_->size, _name);
      if (found_idx >= 0 && found_idx != desc_->pos_extra_fields) {
        idx = found_idx;
        next_idx_ = static_cast<std::size_t>(found_idx) + 1;
      }
    }
    if (idx >= 0) {
      const auto u = static_cast<std::size_t>(idx);
      if (!found_[u]) {
        found_[u] = true;
        ++(*found_count_);
        already_assigned = true;
        desc_->read_fns[u](*r_, _var, desc_->names[u], field_ptrs_[u], set_[u],
                           errors_, desc_->size);
      }
    }
    if (desc_->pos_extra_fields >= 0) {
      if (!already_assigned) {
        const auto pos = static_cast<std::size_t>(desc_->pos_extra_fields);
        const bool was_found = found_[pos];
        desc_->read_extra_fn(*r_, _name, _var, field_ptrs_[pos], found_[pos],
                             set_[pos], errors_);
        if (!was_found && found_[pos]) {
          ++(*found_count_);
        }
      }
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
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
    if (_index >= 0 && static_cast<std::size_t>(_index) < desc_->size &&
        _index != desc_->pos_extra_fields) {
      const auto u = static_cast<std::size_t>(_index);
      if (!found_[u]) {
        found_[u] = true;
        ++(*found_count_);
        already_assigned = true;
        desc_->read_fns[u](*r_, _var, desc_->names[u], field_ptrs_[u], set_[u],
                           errors_, desc_->size);
      }
    }
    if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      if (desc_->pos_extra_fields < 0 && !already_assigned) {
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

 protected:
  const R* r_;
  const Desc* desc_;
  void* const* field_ptrs_;
  bool* found_;
  bool* set_;
  std::size_t* found_count_;
  FieldErrors<ProcessorsType>* errors_;
  mutable std::size_t next_idx_;
};

/// Thin static-size adapter for positional/schemaful/external readers that
/// require `ObjectReader::size()` as a compile-time constant.
template <class R, class W, class ProcessorsType, std::size_t _size,
          bool _has_extra_fields>
class SizedTableViewReader : public TableViewReader<R, W, ProcessorsType> {
 public:
  using Base = TableViewReader<R, W, ProcessorsType>;
  using InputVarType = typename Base::InputVarType;
  using Base::Base;
  using Base::read;

  void read(const int _index, const InputVarType& _var) const {
    static_assert(!_has_extra_fields,
                  "rfl::ExtraFields are not supported for positional reads.");
    Base::read(_index, _var);
  }

  template <int i>
  void read(std::integral_constant<int, i>, const InputVarType& _var) const {
    static_assert(!_has_extra_fields,
                  "rfl::ExtraFields are not supported for positional reads.");
    Base::read(i, _var);
  }

  static constexpr std::size_t size() noexcept { return _size; }
};

template <class R, class W, class ProcessorsType>
class TableViewReaderWithDefault {
 public:
  using InputVarType = typename R::InputVarType;
  using ReadDefaultFn = void (*)(const R&, const InputVarType&,
                                 std::string_view, void*,
                                 FieldErrors<ProcessorsType>*);
  using ReadExtraDefaultFn = void (*)(const R&, std::string_view,
                                      const InputVarType&, void*,
                                      FieldErrors<ProcessorsType>*);

  struct Desc {
    const std::string_view* names;
    const detail::FieldNameEntry* lookup;
    const ReadDefaultFn* read_fns;
    std::size_t size;
    int pos_extra_fields;
    ReadExtraDefaultFn read_extra_fn;
  };

  TableViewReaderWithDefault(const R* _r, const Desc* _desc,
                             void* const* _field_ptrs, bool* _found,
                             std::size_t* _found_count,
                             FieldErrors<ProcessorsType>* _errors) noexcept
      : r_(_r),
        desc_(_desc),
        field_ptrs_(_field_ptrs),
        found_(_found),
        found_count_(_found_count),
        errors_(_errors),
        next_idx_(0) {}

  ~TableViewReaderWithDefault() = default;

  std::span<const std::string_view> field_names() const noexcept {
    return {desc_->names, desc_->size};
  }

  RFL_ALWAYS_INLINE void read(const std::string_view& _name,
                              const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    bool already_assigned = false;
    int idx = -1;
    if (next_idx_ < desc_->size &&
        static_cast<int>(next_idx_) != desc_->pos_extra_fields &&
        desc_->names[next_idx_] == _name) [[likely]] {
      idx = static_cast<int>(next_idx_);
      ++next_idx_;
    } else {
      const int found_idx =
          detail::find_in_table(desc_->lookup, desc_->size, _name);
      if (found_idx >= 0 && found_idx != desc_->pos_extra_fields) {
        idx = found_idx;
        next_idx_ = static_cast<std::size_t>(found_idx) + 1;
      }
    }
    if (idx >= 0) {
      const auto u = static_cast<std::size_t>(idx);
      if (!found_[u]) {
        found_[u] = true;
        ++(*found_count_);
        already_assigned = true;
        desc_->read_fns[u](*r_, _var, desc_->names[u], field_ptrs_[u], errors_);
      }
    }
    if (desc_->pos_extra_fields >= 0) {
      if (!already_assigned) {
        const auto pos = static_cast<std::size_t>(desc_->pos_extra_fields);
        desc_->read_extra_fn(*r_, _name, _var, field_ptrs_[pos], errors_);
      }
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
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
    if (_index >= 0 && static_cast<std::size_t>(_index) < desc_->size &&
        _index != desc_->pos_extra_fields) {
      const auto u = static_cast<std::size_t>(_index);
      if (!found_[u]) {
        found_[u] = true;
        ++(*found_count_);
        already_assigned = true;
        desc_->read_fns[u](*r_, _var, desc_->names[u], field_ptrs_[u], errors_);
      }
    }
    if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      if (desc_->pos_extra_fields < 0 && !already_assigned) {
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

 protected:
  const R* r_;
  const Desc* desc_;
  void* const* field_ptrs_;
  bool* found_;
  std::size_t* found_count_;
  FieldErrors<ProcessorsType>* errors_;
  mutable std::size_t next_idx_;
};

template <class R, class W, class ProcessorsType, std::size_t _size,
          bool _has_extra_fields>
class SizedTableViewReaderWithDefault
    : public TableViewReaderWithDefault<R, W, ProcessorsType> {
 public:
  using Base = TableViewReaderWithDefault<R, W, ProcessorsType>;
  using InputVarType = typename Base::InputVarType;
  using Base::Base;
  using Base::read;

  void read(const int _index, const InputVarType& _var) const {
    static_assert(!_has_extra_fields,
                  "rfl::ExtraFields are not supported for positional reads.");
    Base::read(_index, _var);
  }

  template <int i>
  void read(std::integral_constant<int, i>, const InputVarType& _var) const {
    static_assert(!_has_extra_fields,
                  "rfl::ExtraFields are not supported for positional reads.");
    Base::read(i, _var);
  }

  static constexpr std::size_t size() noexcept { return _size; }
};

template <class R, class W, class ProcessorsType>
class TableViewReaderStripped {
 public:
  using InputVarType = typename R::InputVarType;
  using Desc = typename TableViewReader<R, W, ProcessorsType>::Desc;

  TableViewReaderStripped(const R* _r, const Desc* _desc,
                          void* const* _field_ptrs, bool* _found, bool* _set,
                          std::size_t* _found_count,
                          FieldErrors<ProcessorsType>* _errors) noexcept
      : i_(0),
        r_(_r),
        desc_(_desc),
        field_ptrs_(_field_ptrs),
        found_(_found),
        set_(_set),
        found_count_(_found_count),
        errors_(_errors) {}

  ~TableViewReaderStripped() = default;

  std::optional<Error> read(const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) {
      return std::nullopt;
    }
    if (i_ == desc_->size) {
      std::stringstream stream;
      stream << "Expected a maximum of " << std::to_string(desc_->size)
             << " fields, but got at least one more.";
      return Error(stream.str());
    }
    found_[i_] = true;
    ++(*found_count_);
    desc_->read_fns[i_](*r_, _var, desc_->names[i_], field_ptrs_[i_], set_[i_],
                        errors_, desc_->size);
    ++i_;
    return std::nullopt;
  }

 private:
  mutable std::size_t i_;
  const R* r_;
  const Desc* desc_;
  void* const* field_ptrs_;
  bool* found_;
  bool* set_;
  std::size_t* found_count_;
  FieldErrors<ProcessorsType>* errors_;
};

template <class R, class W, class ProcessorsType>
class TableViewReaderWithDefaultStripped {
 public:
  using InputVarType = typename R::InputVarType;
  using Desc = typename TableViewReaderWithDefault<R, W, ProcessorsType>::Desc;

  TableViewReaderWithDefaultStripped(
      const R* _r, const Desc* _desc, void* const* _field_ptrs, bool* _found,
      std::size_t* _found_count,
      FieldErrors<ProcessorsType>* _errors) noexcept
      : i_(0),
        r_(_r),
        desc_(_desc),
        field_ptrs_(_field_ptrs),
        found_(_found),
        found_count_(_found_count),
        errors_(_errors) {}

  ~TableViewReaderWithDefaultStripped() = default;

  std::optional<Error> read(const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) {
      return std::nullopt;
    }
    if (i_ == desc_->size) {
      std::stringstream stream;
      stream << "Expected a maximum of " << std::to_string(desc_->size)
             << " fields, but got at least one more.";
      return Error(stream.str());
    }
    found_[i_] = true;
    ++(*found_count_);
    desc_->read_fns[i_](*r_, _var, desc_->names[i_], field_ptrs_[i_], errors_);
    ++i_;
    return std::nullopt;
  }

 private:
  mutable std::size_t i_;
  const R* r_;
  const Desc* desc_;
  void* const* field_ptrs_;
  bool* found_;
  std::size_t* found_count_;
  FieldErrors<ProcessorsType>* errors_;
};

// ----------------------------------------------------------------------------
// Non-`T`-parameterized shared driver functions.
// Instantiated ONCE per `(R, W, ProcessorsType)` per TU!
// ----------------------------------------------------------------------------

namespace detail {

using DestroyFn = void (*)(void*) noexcept;

inline void destroy_set_fields_table(const DestroyFn* _destroy_fns,
                                     const bool* _set,
                                     void* const* _field_ptrs,
                                     const std::size_t _size) noexcept {
  for (std::size_t i = 0; i < _size; ++i) {
    if (_set[i] && _destroy_fns[i] != nullptr) {
      _destroy_fns[i](_field_ptrs[i]);
    }
  }
}

/// RAII scope guard guaranteeing every placement-constructed field in `buf` is
/// destroyed on error return, normal move return, or exception unwind (e.g. if
/// `T`'s move constructor or a field constructor throws).
struct UninitFieldsGuard {
  const DestroyFn* destroy_fns;
  const bool* set;
  void* const* field_ptrs;
  std::size_t size;

  ~UninitFieldsGuard() noexcept {
    destroy_set_fields_table(destroy_fns, set, field_ptrs, size);
  }
};

template <class R, class W, bool _no_field_names, class ProcessorsType,
          class ReaderObj>
std::optional<Error> read_table_uninit_common(
    const R& _r, const typename R::InputVarType& _var,
    const typename TableViewReader<R, W, ProcessorsType>::Desc& _desc,
    void (*const* _missing_fns)(std::string_view, bool, void*, bool*,
                                FieldErrors<ProcessorsType>*, bool) noexcept,
    void* const* _field_ptrs, bool* _found, bool* _set) {
  FieldErrors<ProcessorsType> errors;
  std::size_t found_count = 0;
  if constexpr (_no_field_names) {
    auto arr = _r.to_array(_var);
    if (!arr) [[unlikely]] {
      return arr.error();
    }
    const auto reader = TableViewReaderStripped<R, W, ProcessorsType>(
        &_r, &_desc, _field_ptrs, _found, _set, &found_count, &errors);
    if (auto err = _r.read_array(reader, *arr)) [[unlikely]] {
      return err;
    }
  } else {
    auto obj = _r.to_object(_var);
    if (!obj) [[unlikely]] {
      return obj.error();
    }
    const auto reader = ReaderObj(&_r, &_desc, _field_ptrs, _found, _set,
                                  &found_count, &errors);
    if (auto err = _r.read_object(reader, *obj)) [[unlikely]] {
      return err;
    }
  }

  if (found_count < _desc.size &&
      !should_stop_on_field_error<ProcessorsType>(errors)) {
    for (std::size_t i = 0; i < _desc.size; ++i) {
      if (!_found[i]) {
        _missing_fns[i](_desc.names[i], false, _field_ptrs[i], &_set[i],
                        &errors, /*_value_initialize_default_vals=*/false);
      }
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
          class ReaderObj>
std::optional<Error> read_table_with_default_common(
    const R& _r, const typename R::InputVarType& _var,
    const typename TableViewReaderWithDefault<R, W, ProcessorsType>::Desc&
        _desc,
    void (*const* _missing_fns)(std::string_view, bool, void*, bool*,
                                FieldErrors<ProcessorsType>*, bool) noexcept,
    void* const* _field_ptrs, bool* _found) {
  FieldErrors<ProcessorsType> errors;
  std::size_t found_count = 0;
  if constexpr (_no_field_names) {
    auto arr = _r.to_array(_var);
    if (!arr) [[unlikely]] {
      return arr.error();
    }
    const auto reader =
        TableViewReaderWithDefaultStripped<R, W, ProcessorsType>(
            &_r, &_desc, _field_ptrs, _found, &found_count, &errors);
    if (auto err = _r.read_array(reader, *arr)) [[unlikely]] {
      return err;
    }
  } else {
    auto obj = _r.to_object(_var);
    if (!obj) [[unlikely]] {
      return obj.error();
    }
    const auto reader =
        ReaderObj(&_r, &_desc, _field_ptrs, _found, &found_count, &errors);
    if (auto err = _r.read_object(reader, *obj)) [[unlikely]] {
      return err;
    }
  }

  if (_missing_fns != nullptr && found_count < _desc.size &&
      !should_stop_on_field_error<ProcessorsType>(errors)) {
    for (std::size_t i = 0; i < _desc.size; ++i) {
      if (!_found[i]) {
        _missing_fns[i](_desc.names[i], false, _field_ptrs[i], nullptr,
                        &errors, /*_value_initialize_default_vals=*/false);
      }
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

template <class W, bool _no_field_names, class OutputObjectOrArrayType, class P>
void write_table_common(
    const W& _w, const std::string_view* _names,
    void (*const* _write_fns)(const W&, std::string_view, const void*,
                              OutputObjectOrArrayType*),
    const void* const* _field_ptrs, const std::size_t _size,
    const int _pos_extra_fields,
    std::size_t (*_extra_size_fn)(const void*) noexcept, const P& _parent) {
  using ParentType = Parent<W>;
  const std::size_t total_fields =
      (_pos_extra_fields >= 0 && _extra_size_fn != nullptr)
          ? (_size - 1 +
             _extra_size_fn(
                 _field_ptrs[static_cast<std::size_t>(_pos_extra_fields)]))
          : _size;
  if constexpr (_no_field_names) {
    auto arr = ParentType::add_array(_w, total_fields, _parent);
    for (std::size_t i = 0; i < _size; ++i) {
      _write_fns[i](_w, _names[i], _field_ptrs[i], &arr);
    }
    _w.end_array(&arr);
  } else {
    auto obj = ParentType::add_object(_w, total_fields, _parent);
    for (std::size_t i = 0; i < _size; ++i) {
      _write_fns[i](_w, _names[i], _field_ptrs[i], &obj);
    }
    _w.end_object(&obj);
  }
}

template <bool _no_field_names, class ProcessorsType, class SchemaType>
schema::Type to_schema_table_common(
    std::map<std::string, schema::Type>* _definitions,
    const std::string_view* _wire_names, const std::string_view* _raw_names,
    void (*const* _schema_fns)(std::map<std::string, schema::Type>*,
                               SchemaType*, std::string_view, std::string_view,
                               const void*, std::span<const FieldMeta>),
    const void* const* _default_ptrs, const std::size_t _size,
    const std::span<const FieldMeta> _field_meta,
    schema::Type (*_extra_schema_fn)(std::map<std::string, schema::Type>*)) {
  SchemaType schema;
  for (std::size_t i = 0; i < _size; ++i) {
    const std::string_view raw =
        _raw_names != nullptr ? _raw_names[i] : _wire_names[i];
    _schema_fns[i](_definitions, &schema, _wire_names[i], raw,
                   _default_ptrs ? _default_ptrs[i] : nullptr, _field_meta);
  }
  if constexpr (!_no_field_names) {
    if (_extra_schema_fn != nullptr) {
      schema.additional_properties_ =
          std::make_shared<schema::Type>(_extra_schema_fn(_definitions));
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      schema.additional_properties_ = false;
    }
  }
  return schema::Type{schema};
}

}  // namespace detail

/// Type-erased descriptor-table aggregate parser. Bypasses `rfl::NamedTuple`,
/// `rfl::Tuple`, and per-struct `ViewReader` / `NamedTupleParser` templates
/// completely: each struct `T` emits only `static constexpr` data tables and
/// delegates all loops to non-`T`-parameterized shared drivers.
template <class R, class W, class T, class ProcessorsType>
struct TableStructParser {
  using InputVarType = typename R::InputVarType;
  using CasePipeline = internal::direct_case_pipeline_t<ProcessorsType>;
  using Names = internal::DirectFieldNames<T, CasePipeline>;
  using FieldList = detail::flattened_field_type_list_t<T>;
  using ExtraInfo = detail::extra_fields_info<FieldList>;

  static constexpr std::size_t size_ = Names::size_;
  static constexpr int pos_extra_fields_ = ExtraInfo::pos_;

  static_assert(ExtraInfo::count_ <= 1,
                "There can be at most one rfl::ExtraFields field per struct.");

  using FormatNTParser = Parser<R, W, NamedTuple<>, ProcessorsType>;
  static constexpr bool ignore_empty_containers_ =
      FormatNTParser::ignore_empty_containers_;
  static constexpr bool all_required_ = FormatNTParser::all_required_;
  static constexpr bool no_field_names_ = FormatNTParser::no_field_names_;

  static_assert(pos_extra_fields_ == -1 || !no_field_names_,
                "You cannot use the rfl::NoFieldNames processor if you are "
                "including rfl::ExtraFields.");

  using OutputObjectOrArrayType =
      std::conditional_t<no_field_names_, typename W::OutputArrayType,
                         typename W::OutputObjectType>;
  using SchemaType = std::conditional_t<no_field_names_, schema::Type::Tuple,
                                        schema::Type::Object>;

  using UninitReader = std::conditional_t<
      detail::ReaderSupportsUnsizedObjectReader<R>,
      TableViewReader<R, W, ProcessorsType>,
      SizedTableViewReader<R, W, ProcessorsType, size_,
                           (pos_extra_fields_ != -1)>>;
  using DefaultReader = std::conditional_t<
      detail::ReaderSupportsUnsizedObjectReader<R>,
      TableViewReaderWithDefault<R, W, ProcessorsType>,
      SizedTableViewReaderWithDefault<R, W, ProcessorsType, size_,
                                      (pos_extra_fields_ != -1)>>;

  using ReadUninitFn =
      typename TableViewReader<R, W, ProcessorsType>::ReadUninitFn;
  using ReadExtraUninitFn =
      typename TableViewReader<R, W, ProcessorsType>::ReadExtraUninitFn;
  using ReadDefaultFn =
      typename TableViewReaderWithDefault<R, W, ProcessorsType>::ReadDefaultFn;
  using ReadExtraDefaultFn = typename TableViewReaderWithDefault<
      R, W, ProcessorsType>::ReadExtraDefaultFn;
  using MissingFn = void (*)(std::string_view, bool, void*, bool*,
                             FieldErrors<ProcessorsType>*, bool) noexcept;
  using DestroyFn = detail::DestroyFn;
  using WriteFn = void (*)(const W&, std::string_view, const void*,
                           OutputObjectOrArrayType*);
  using SchemaFn = void (*)(std::map<std::string, schema::Type>*, SchemaType*,
                            std::string_view, std::string_view, const void*,
                            std::span<const FieldMeta>);

 private:
  template <class FieldT>
  static consteval ReadUninitFn make_read_uninit_fn() noexcept {
    if constexpr (internal::is_extra_fields_v<std::remove_cvref_t<FieldT>>) {
      return nullptr;
    } else {
      return &detail::read_uninit_field_erased<R, W, FieldT, ProcessorsType>;
    }
  }

  template <class... Fields>
  static consteval auto make_read_uninit_table(
      detail::TableFieldTypeList<Fields...>) noexcept {
    return std::array<ReadUninitFn, sizeof...(Fields)>{
        make_read_uninit_fn<Fields>()...};
  }

  template <class FieldT>
  static consteval ReadDefaultFn make_read_default_fn() noexcept {
    if constexpr (internal::is_extra_fields_v<std::remove_cvref_t<FieldT>>) {
      return nullptr;
    } else {
      return &detail::read_default_field_erased<R, W, FieldT, ProcessorsType>;
    }
  }

  template <class... Fields>
  static consteval auto make_read_default_table(
      detail::TableFieldTypeList<Fields...>) noexcept {
    return std::array<ReadDefaultFn, sizeof...(Fields)>{
        make_read_default_fn<Fields>()...};
  }

  template <bool _defaults_constructed, class... Fields>
  static consteval auto make_missing_table(
      detail::TableFieldTypeList<Fields...>) noexcept {
    return std::array<MissingFn, sizeof...(Fields)>{
        &detail::handle_missing_field_erased<
            ignore_empty_containers_, all_required_, _defaults_constructed,
            ProcessorsType, Fields>...};
  }

  template <class FieldT>
  static consteval DestroyFn make_destroy_fn() noexcept {
    if constexpr (std::is_trivially_destructible_v<
                      std::remove_all_extents_t<FieldT>>) {
      return nullptr;
    } else {
      return &detail::destroy_field_erased<FieldT>;
    }
  }

  template <class... Fields>
  static consteval bool has_any_non_trivial_dtor(
      detail::TableFieldTypeList<Fields...>) noexcept {
    return (false || ... ||
            !std::is_trivially_destructible_v<
                std::remove_all_extents_t<Fields>>);
  }

  template <class... Fields>
  static consteval auto make_destroy_table(
      detail::TableFieldTypeList<Fields...>) noexcept {
    return std::array<DestroyFn, sizeof...(Fields)>{
        make_destroy_fn<Fields>()...};
  }

  template <class... Fields>
  static consteval auto make_write_table(
      detail::TableFieldTypeList<Fields...>) noexcept {
    return std::array<WriteFn, sizeof...(Fields)>{
        &detail::write_field_erased<R, W, ignore_empty_containers_,
                                    all_required_, no_field_names_,
                                    ProcessorsType, Fields,
                                    OutputObjectOrArrayType>...};
  }

  template <class... Fields>
  static consteval auto make_schema_table(
      detail::TableFieldTypeList<Fields...>) noexcept {
    return std::array<SchemaFn, sizeof...(Fields)>{
        &detail::build_schema_field_erased<R, W, no_field_names_,
                                           ProcessorsType, Fields,
                                           SchemaType>...};
  }

  static consteval ReadExtraUninitFn make_read_extra_uninit_fn() noexcept {
    if constexpr (pos_extra_fields_ >= 0) {
      using ExtraT =
          typename detail::nth_type_in_list<pos_extra_fields_, FieldList>::type;
      return &detail::read_extra_uninit_erased<R, W, ExtraT, ProcessorsType>;
    } else {
      return nullptr;
    }
  }

  static consteval ReadExtraDefaultFn make_read_extra_default_fn() noexcept {
    if constexpr (pos_extra_fields_ >= 0) {
      using ExtraT =
          typename detail::nth_type_in_list<pos_extra_fields_, FieldList>::type;
      return &detail::read_extra_default_erased<R, W, ExtraT, ProcessorsType>;
    } else {
      return nullptr;
    }
  }

  static consteval std::size_t (*make_extra_size_fn() noexcept)(
      const void*) noexcept {
    if constexpr (pos_extra_fields_ >= 0) {
      using ExtraT =
          typename detail::nth_type_in_list<pos_extra_fields_, FieldList>::type;
      return &detail::extra_fields_size_erased<ExtraT>;
    } else {
      return nullptr;
    }
  }

  static consteval schema::Type (*make_extra_schema_fn() noexcept)(
      std::map<std::string, schema::Type>*) {
    if constexpr (pos_extra_fields_ >= 0) {
      using ExtraT =
          typename detail::nth_type_in_list<pos_extra_fields_, FieldList>::type;
      return &detail::build_extra_schema_erased<R, W, ExtraT, ProcessorsType>;
    } else {
      return nullptr;
    }
  }

 public:
  static Result<T> read(const R& _r, const InputVarType& _var) {
    static_assert(pos_extra_fields_ == -1 || !schemaful::IsSchemafulReader<R>,
                  "rfl::ExtraFields are not supported for schemaful formats, "
                  "because schemaful formats cannot have extra fields.");
    static_assert(!internal::no_extra_fields_v<ProcessorsType> ||
                      pos_extra_fields_ != -1 ||
                      !schemaful::IsSchemafulReader<R>,
                  "Passing rfl::NoExtraFields to a schemaful format does not "
                  "make sense, because schemaful formats cannot have extra "
                  "fields.");

    static constexpr auto read_fns = make_read_uninit_table(FieldList{});
    static constexpr auto missing_fns = make_missing_table<false>(FieldList{});
    static constexpr bool kHasNonTrivialDtor =
        has_any_non_trivial_dtor(FieldList{});
    static constexpr typename TableViewReader<R, W, ProcessorsType>::Desc desc{
        Names::names_.data(), Names::lookup_.data(), read_fns.data(),
        size_,                pos_extra_fields_,     make_read_extra_uninit_fn()};

    alignas(T) unsigned char buf[sizeof(T)]{};
    auto* ptr = internal::ptr_cast<T*>(&buf);
    const auto field_ptrs = detail::collect_flattened_mutable_ptrs(*ptr);
    std::array<bool, size_> found{};
    std::array<bool, size_> set{};

    if constexpr (kHasNonTrivialDtor) {
      static constexpr auto destroy_fns = make_destroy_table(FieldList{});
      const detail::UninitFieldsGuard guard{destroy_fns.data(), set.data(),
                                            field_ptrs.data(), size_};
      auto err = detail::read_table_uninit_common<R, W, no_field_names_,
                                                  ProcessorsType, UninitReader>(
          _r, _var, desc, missing_fns.data(), field_ptrs.data(), found.data(),
          set.data());
      if (err) [[unlikely]] {
        return error(std::move(*err));
      }
      return Result<T>(std::move(*ptr));
    } else {
      auto err = detail::read_table_uninit_common<R, W, no_field_names_,
                                                  ProcessorsType, UninitReader>(
          _r, _var, desc, missing_fns.data(), field_ptrs.data(), found.data(),
          set.data());
      if (err) [[unlikely]] {
        return error(std::move(*err));
      }
      return Result<T>(std::move(*ptr));
    }
  }

  static std::optional<Error> read_with_default(const R& _r,
                                                const InputVarType& _var,
                                                T* _target) {
    static_assert(pos_extra_fields_ == -1 || !schemaful::IsSchemafulReader<R>,
                  "rfl::ExtraFields are not supported for schemaful formats, "
                  "because schemaful formats cannot have extra fields.");
    static_assert(!internal::no_extra_fields_v<ProcessorsType> ||
                      pos_extra_fields_ != -1 ||
                      !schemaful::IsSchemafulReader<R>,
                  "Passing rfl::NoExtraFields to a schemaful format does not "
                  "make sense, because schemaful formats cannot have extra "
                  "fields.");

    static constexpr auto read_fns = make_read_default_table(FieldList{});
    static constexpr typename TableViewReaderWithDefault<
        R, W, ProcessorsType>::Desc desc{
        Names::names_.data(), Names::lookup_.data(), read_fns.data(),
        size_,                pos_extra_fields_,     make_read_extra_default_fn()};

    const auto field_ptrs = detail::collect_flattened_mutable_ptrs(*_target);
    std::array<bool, size_> found{};

    if constexpr (internal::has_default_val_v<T> &&
                  !internal::default_if_missing_v<ProcessorsType>) {
      static constexpr auto missing_fns = make_missing_table<true>(FieldList{});
      return detail::read_table_with_default_common<
          R, W, no_field_names_, ProcessorsType, DefaultReader>(
          _r, _var, desc, missing_fns.data(), field_ptrs.data(), found.data());
    } else {
      return detail::read_table_with_default_common<
          R, W, no_field_names_, ProcessorsType, DefaultReader>(
          _r, _var, desc, nullptr, field_ptrs.data(), found.data());
    }
  }

  template <class P>
  static void write(const W& _w, const T& _var, const P& _parent) {
    static constexpr auto write_fns = make_write_table(FieldList{});
    const auto field_ptrs = detail::collect_flattened_const_ptrs(_var);
    detail::write_table_common<W, no_field_names_, OutputObjectOrArrayType, P>(
        _w, Names::names_.data(), write_fns.data(), field_ptrs.data(), size_,
        pos_extra_fields_, make_extra_size_fn(), _parent);
  }

  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions,
      const T* _defaults = nullptr,
      const std::span<const FieldMeta> _field_meta = {}) {
    static constexpr auto schema_fns = make_schema_table(FieldList{});
    const std::string_view* raw_names_ptr = nullptr;
    if (!_field_meta.empty()) {
      if constexpr (!std::is_same_v<CasePipeline,
                                    internal::DirectCasePipeline<>>) {
        using RawNames =
            internal::DirectFieldNames<T, internal::DirectCasePipeline<>>;
        raw_names_ptr = RawNames::names_.data();
      }
    }
    if (_defaults != nullptr) {
      const auto default_ptrs = detail::collect_flattened_const_ptrs(*_defaults);
      return detail::to_schema_table_common<no_field_names_, ProcessorsType,
                                            SchemaType>(
          _definitions, Names::names_.data(), raw_names_ptr, schema_fns.data(),
          default_ptrs.data(), size_, _field_meta, make_extra_schema_fn());
    }
    return detail::to_schema_table_common<no_field_names_, ProcessorsType,
                                          SchemaType>(
        _definitions, Names::names_.data(), raw_names_ptr, schema_fns.data(),
        nullptr, size_, _field_meta, make_extra_schema_fn());
  }
};

}  // namespace rfl::parsing

#endif
