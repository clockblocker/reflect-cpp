#ifndef RFL_PARSING_NAMEDTUPLEPARSER_HPP_
#define RFL_PARSING_NAMEDTUPLEPARSER_HPP_

#include <array>
#include <cstdint>
#include <exception>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../Generic.hpp"
#include "../Metadata.hpp"
#include "../NamedTuple.hpp"
#include "../Result.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/default_if_missing_v.hpp"
#include "../internal/has_default_val_v.hpp"
#include "../internal/has_reflection_method_v.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/is_attribute.hpp"
#include "../internal/is_default_val_v.hpp"
#include "../internal/is_extra_fields.hpp"
#include "../internal/is_skip.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "../internal/nth_element_t.hpp"
#include "../internal/ptr_cast.hpp"
#include "../num_fields.hpp"
#include "../to_view.hpp"
#include "AreReaderAndWriter.hpp"
#include "FieldErrors.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "ViewReader.hpp"
#include "ViewReaderWithDefault.hpp"
#include "ViewReaderWithDefaultAndStrippedFieldNames.hpp"
#include "ViewReaderWithStrippedFieldNames.hpp"
#include "call_destructors_where_necessary.hpp"
#include "is_empty.hpp"
#include "is_required.hpp"
#include "schema/Type.hpp"
#include "to_single_error_message.hpp"
#include "wrap_error.hpp"

namespace rfl {

/// forward declaration
template <class... Ps>
Generic to_generic(const auto& _t);

namespace internal {

/// Satisfied by a type that carries a nested `ReflectionType` and can be
/// unwrapped by decomposing it, like `rfl::Validator`.
template <class T>
concept IsReflectionTypeWrapper =
    // We use `std::is_aggregate_v` and `num_fields` rather than `requires`
    // because `requires`, well, requires an expression, but checking this via
    // syntax would require a statement `const auto& [_] = _t`, which is not
    // allowed in a `requires` clause.
    has_reflection_type_v<T> && !has_reflection_method_v<T> &&
    std::is_aggregate_v<T> && num_fields<T> == 1;

/// Satisfied when the generic writer can serialize `T`, i.e. when a schema
/// `default` can be produced for it. Every type is writable, except those that
/// can be constructed from `ReflectionType`, but not converted back to it.
template <class T>
concept CanWriteGeneric =
    !has_reflection_type_v<std::remove_cvref_t<T>> ||
    has_reflection_method_v<std::remove_cvref_t<T>> ||
    IsReflectionTypeWrapper<std::remove_cvref_t<T>>;

/// Satisfied when a schema `default` can be emitted for the field type `T`:
/// it must be a `DefaultVal` that satisfies `CanWriteGeneric`.
template <class T>
concept CanEmitDefault =
    is_default_val_v<T> && CanWriteGeneric<typename T::Type>;

}  // namespace internal
}  // namespace rfl

namespace rfl::parsing {

namespace detail {

template <class T>
struct is_std_optional_for_default : std::false_type {};

template <class T>
struct is_std_optional_for_default<std::optional<T>> : std::true_type {};

template <class T>
constexpr bool is_plain_integer_for_default_v =
    std::is_integral_v<T> && !std::is_same_v<T, bool> &&
    !std::is_same_v<T, char> && !std::is_same_v<T, wchar_t> &&
    !std::is_same_v<T, char8_t> && !std::is_same_v<T, char16_t> &&
    !std::is_same_v<T, char32_t>;

/// Fast-path conversion of a default field value to `rfl::Generic` for schema
/// emission. Basic scalars construct `rfl::Generic` directly without
/// instantiating `rfl::to_generic<ProcessorsType>` / `rfl::generic::Writer`.
template <class ProcessorsType, class U>
inline Generic to_default_generic(const U& _val) {
  using T = std::remove_cvref_t<U>;
  if constexpr (std::is_same_v<T, bool>) {
    return Generic(_val);
  } else if constexpr (is_plain_integer_for_default_v<T>) {
    if constexpr (std::is_unsigned_v<T> && sizeof(T) >= sizeof(int64_t)) {
      if (_val <= static_cast<T>(std::numeric_limits<int64_t>::max())) {
        return Generic(static_cast<int64_t>(_val));
      }
      return rfl::to_generic<ProcessorsType>(_val);
    } else {
      return Generic(static_cast<int64_t>(_val));
    }
  } else if constexpr (std::is_floating_point_v<T>) {
    return Generic(static_cast<double>(_val));
  } else if constexpr (std::is_same_v<T, std::string>) {
    return Generic(_val);
  } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
    Generic::Array arr;
    arr.reserve(_val.size());
    for (const auto& s : _val) {
      arr.emplace_back(s);
    }
    return Generic(std::move(arr));
  } else if constexpr (is_std_optional_for_default<T>::value) {
    if (_val.has_value()) {
      return detail::to_default_generic<ProcessorsType>(*_val);
    }
    return Generic();
  } else {
    return rfl::to_generic<ProcessorsType>(_val);
  }
}

/// Sets the default value of the DefaultVal node in `_schema`.
inline void set_schema_default_value(Generic _default, schema::Type* _schema) {
  _schema->variant_.visit([&](auto& _s) {
    if constexpr (std::is_same_v<std::remove_cvref_t<decltype(_s)>,
                                 schema::Type::DefaultVal>) {
      _s.default_value_ = std::move(_default);
    }
  });
}

/// Applies out-of-band `rfl::FieldMeta` (description / deprecation) to a field
/// schema node without embedding long StringLiteral NTTPs in field types.
inline void apply_field_metadata(schema::Type* _schema,
                                 const FieldMeta& _meta) {
  if (!_meta.deprecation_message.empty()) {
    *_schema = schema::Type{schema::Type::Deprecated{
        .deprecation_message_ = std::string(_meta.deprecation_message),
        .description_ = std::string(_meta.description),
        .type_ = Ref<schema::Type>::make(std::move(*_schema))}};
  } else if (!_meta.description.empty()) {
    *_schema = schema::Type{schema::Type::Description{
        .description_ = std::string(_meta.description),
        .type_ = Ref<schema::Type>::make(std::move(*_schema))}};
  }
}

/// Free per-field serialization helper parameterized only by ValueType (never
/// the enclosing NamedTuple's FieldTypes... pack).
template <class R, class W, bool _ignore_empty_containers, bool _all_required,
          bool _no_field_names, class ProcessorsType, class ValueType,
          class OutputObjectOrArrayType>
void write_named_tuple_field(const W& _w, const std::string_view _name,
                             const ValueType& _value,
                             OutputObjectOrArrayType* _ptr) {
  using ParentType = Parent<W>;
  const auto make_parent = [](const std::string_view _n,
                              OutputObjectOrArrayType* _p) {
    if constexpr (_no_field_names) {
      return typename ParentType::Array{_p};
    } else {
      return typename ParentType::Object{_n, _p};
    }
  };
  if constexpr (internal::is_extra_fields_v<ValueType>) {
    for (const auto& [k, v] : *_value) {
      const auto new_parent = make_parent(k, _ptr);
      Parser<R, W, std::remove_cvref_t<decltype(v)>, ProcessorsType>::write(
          _w, v, new_parent);
    }
  } else if constexpr (!_all_required && !_no_field_names &&
                       !is_required<ValueType, _ignore_empty_containers>()) {
    const auto new_parent = make_parent(_name, _ptr);
    if (!is_empty(_value)) {
      if constexpr (internal::is_attribute_v<ValueType>) {
        Parser<R, W, ValueType, ProcessorsType>::write(
            _w, _value, new_parent.as_attribute());
      } else {
        Parser<R, W, ValueType, ProcessorsType>::write(_w, _value, new_parent);
      }
    }
  } else {
    const auto new_parent = make_parent(_name, _ptr);
    if constexpr (internal::is_attribute_v<ValueType>) {
      Parser<R, W, ValueType, ProcessorsType>::write(_w, _value,
                                                     new_parent.as_attribute());
    } else {
      Parser<R, W, ValueType, ProcessorsType>::write(_w, _value, new_parent);
    }
  }
}

/// Extracts a pointer to a view slot for schema default inspection.
/// Pointer slots (including `T*` and C-array pointers `T (*)[N]`) are returned
/// directly without dereferencing; non-pointer processor-added slots (such as
/// `rfl::AddStructName`'s `rfl::Literal<...>`) return `nullptr`.
template <class SlotT>
RFL_ALWAYS_INLINE auto extract_schema_field_ptr(const SlotT& _slot) noexcept {
  if constexpr (std::is_pointer_v<SlotT>) {
    return _slot;
  } else {
    return static_cast<const void*>(nullptr);
  }
}

/// Free per-field schema helper parameterized only by field type U and its
/// view-slot pointer type (never the enclosing NamedTuple's FieldTypes...
/// pack). Deduces `FieldValPtrT` independently of `U` so both raw C-array
/// pointers (`T (*)[N]` vs `rfl::internal::Array<T[N]>`) and non-pointer
/// processor-added slots (`const void*` / `nullptr`) work cleanly.
template <class R, class W, bool _no_field_names, class ProcessorsType, class U,
          class SchemaType, class FieldValPtrT>
void build_named_tuple_field_schema(
    std::map<std::string, schema::Type>* _definitions, SchemaType* _schema,
    const std::string_view _wire_name, const std::string_view _raw_name,
    const FieldValPtrT _field_val,
    const std::span<const FieldMeta> _field_meta) noexcept {
  using PtrT = std::remove_cvref_t<FieldValPtrT>;
  if constexpr (!internal::is_skip_v<U> && !internal::is_extra_fields_v<U>) {
    auto s = Parser<R, W, U, ProcessorsType>::to_schema(_definitions);
    if constexpr (!std::is_same_v<PtrT, const void*> &&
                  !std::is_same_v<PtrT, void*> &&
                  !std::is_same_v<PtrT, std::nullptr_t> &&
                  internal::CanEmitDefault<U>) {
      if (_field_val != nullptr) {
        detail::set_schema_default_value(
            detail::to_default_generic<ProcessorsType>(
                internal::get_default_val(*_field_val).get()),
            &s);
      }
    }
    if (!_field_meta.empty()) {
      if (const auto meta =
              internal::find_field_metadata(_field_meta, _wire_name, _raw_name);
          meta.has_value()) {
        detail::apply_field_metadata(&s, *meta);
      }
    }
    if constexpr (_no_field_names) {
      _schema->types_.emplace_back(std::move(s));
    } else {
      _schema->types_[std::string(_wire_name)] = std::move(s);
    }
  }
}

/// Whether a missing rfl::DefaultVal field can be value-initialized. Only
/// an rfl::NamedTuple does this at runtime, and it cannot have const fields,
/// so const fields of structs need no instantiation.
template <class ValueType>
constexpr bool can_value_initialize_default_val() {
  using T = typename internal::default_val_t<ValueType>::Type;
  return !std::is_const_v<ValueType> && std::is_default_constructible_v<T> &&
         std::is_default_constructible_v<ValueType>;
}

/// Free per-field missing-field handler parameterized only by FieldSlotType
/// (never the enclosing NamedTuple's FieldTypes... pack).
/// Precondition: `_is_set_ptr` is non-null whenever `_defaults_constructed` is
/// false (the uninitialized-memory read path).
template <bool _ignore_empty_containers, bool _all_required,
          bool _defaults_constructed, class ProcessorsType, class FieldSlotType>
RFL_ALWAYS_INLINE void handle_missing_named_tuple_field(
    const std::string_view _name, const bool _is_found,
    const FieldSlotType& _field_slot, bool* _is_set_ptr,
    FieldErrors<ProcessorsType>* _errors,
    const bool _value_initialize_default_vals) noexcept {
  using SlotT = std::remove_cvref_t<FieldSlotType>;
  using ValueType = std::remove_reference_t<std::remove_pointer_t<SlotT>>;
  constexpr bool is_default_val_field = internal::is_default_val_v<ValueType>;
  constexpr bool is_required_field =
      !is_default_val_field && !internal::is_extra_fields_v<ValueType> &&
      (_all_required || is_required<ValueType, _ignore_empty_containers>());

  // When the struct was default-constructed, missing optional or DefaultVal
  // fields already hold their default values, so nothing is needed.
  if constexpr (_defaults_constructed && !is_required_field) {
    return;
  } else {
    if (should_stop_on_field_error<ProcessorsType>(*_errors)) return;
    if (!_is_found) {
      if constexpr (std::is_pointer_v<SlotT> && !_defaults_constructed &&
                    is_default_val_field) {
        if constexpr (detail::can_value_initialize_default_val<ValueType>()) {
          if (_value_initialize_default_vals) {
            // The value's constructor can throw, for instance a Validator
            // whose zero value is invalid.
            try {
              ::new (_field_slot) ValueType();
            } catch (std::exception& e) {
              _errors->emplace_back(make_nested_error(
                  std::string(_name),
                  "Field named '" + std::string(_name) +
                      "' not found, and its default value could not be "
                      "constructed: " +
                      e.what()));
              return;
            }
            *_is_set_ptr = true;
            return;
          }
        }
      }

      if constexpr (is_required_field ||
                    (is_default_val_field && !_defaults_constructed)) {
        std::stringstream stream;
        stream << "Field named '" << std::string(_name) << "' not found.";
        _errors->emplace_back(
            make_nested_error(std::string(_name), stream.str()));
        return;
      }

      if constexpr (std::is_pointer_v<SlotT> && !_defaults_constructed &&
                    !is_required_field && !is_default_val_field) {
        using NonConstT = std::remove_const_t<ValueType>;
        const auto ptr = const_cast<NonConstT*>(_field_slot);
        if constexpr (std::is_nothrow_default_constructible_v<NonConstT>) {
          ::new (ptr) NonConstT();
        } else {
          // The default value can throw, for instance a Validator whose
          // default value is invalid.
          try {
            ::new (ptr) NonConstT();
          } catch (std::exception& e) {
            _errors->emplace_back(make_nested_error(
                std::string(_name),
                "Field named '" + std::string(_name) +
                    "' not found, and its default value could not be "
                    "constructed: " +
                    e.what()));
            return;
          }
        }
        *_is_set_ptr = true;
      }
    }
  }
}

}  // namespace detail

template <class R, class W, bool _ignore_empty_containers, bool _all_required,
          bool _no_field_names, class ProcessorsType, class... FieldTypes>
  requires AreReaderAndWriter<R, W, NamedTuple<FieldTypes...>>
struct NamedTupleParser {
  using InputVarType = typename R::InputVarType;

  using ParentType = Parent<W>;

  using NamedTupleType = NamedTuple<FieldTypes...>;

  using ViewReaderType = std::conditional_t<
      _no_field_names,
      ViewReaderWithStrippedFieldNames<R, W, NamedTupleType, ProcessorsType>,
      ViewReader<R, W, NamedTupleType, ProcessorsType>>;

  using ViewReaderWithDefaultType = std::conditional_t<
      _no_field_names,
      ViewReaderWithDefaultAndStrippedFieldNames<R, W, NamedTupleType,
                                                 ProcessorsType>,
      ViewReaderWithDefault<R, W, NamedTupleType, ProcessorsType>>;

  using InputObjectOrArrayType =
      std::conditional_t<_no_field_names, typename R::InputArrayType,
                         typename R::InputObjectType>;
  using OutputObjectOrArrayType =
      std::conditional_t<_no_field_names, typename W::OutputArrayType,
                         typename W::OutputObjectType>;

  using SchemaType = std::conditional_t<_no_field_names, schema::Type::Tuple,
                                        schema::Type::Object>;

  static constexpr size_t size_ = NamedTupleType::size();

  static_assert(NamedTupleType::pos_extra_fields() == -1 || !_no_field_names,
                "You cannot use the rfl::NoFieldNames processor if you are "
                "including rfl::ExtraFields.");

 public:
  /**
   * @brief Reads a named tuple from the input.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @return A Result containing the parsed named tuple or an error.
   */
  static Result<NamedTuple<FieldTypes...>> read(
      const R& _r, const InputVarType& _var) noexcept {
    alignas(NamedTuple<FieldTypes...>) unsigned char
        buf[sizeof(NamedTuple<FieldTypes...>)];
    auto ptr = internal::ptr_cast<NamedTuple<FieldTypes...>*>(&buf);
    auto view = rfl::to_view(*ptr);
    using ViewType = std::remove_cvref_t<decltype(view)>;
    // A NamedTuple has no in-class initializers, so a missing bare
    // rfl::DefaultVal field is value-initialized, which is what DefaultVal<T>
    // means without an initializer.
    auto [set, err] = Parser<R, W, ViewType, ProcessorsType>::read_view(
        _r, _var, &view, /*_value_initialize_default_vals=*/true);
    if (err) [[unlikely]] {
      call_destructors_where_necessary(set, &view);
      return error(std::move(*err));
    }
    auto res = Result<NamedTuple<FieldTypes...>>(std::move(*ptr));
    call_destructors_where_necessary(set, &view);
    return res;
  }

  /**
   * @brief Reads the data into a view assuming no default values.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @param _view The view to read into.
   * @param _value_initialize_default_vals Whether a missing rfl::DefaultVal
   *         field is value-initialized. Otherwise it is required, because
   *         the default value in the struct's in-class initializer is not
   *         available.
   * @return A pair containing a boolean array indicating which fields were
   *         found and an optional error.
   */
  static RFL_ALWAYS_INLINE
      std::pair<std::array<bool, NamedTupleType::size()>, std::optional<Error>>
      read_view(const R& _r, const InputVarType& _var,
                NamedTuple<FieldTypes...>* _view,
                const bool _value_initialize_default_vals = false) noexcept {
    if constexpr (_no_field_names) {
      auto arr = _r.to_array(_var);
      if (!arr) [[unlikely]] {
        auto set = std::array<bool, NamedTupleType::size()>{};
        return std::make_pair(set, arr.error());
      }
      return read_object_or_array(_r, *arr, _view,
                                  _value_initialize_default_vals);
    } else {
      auto obj = _r.to_object(_var);
      if (!obj) [[unlikely]] {
        auto set = std::array<bool, NamedTupleType::size()>{};
        return std::make_pair(set, obj.error());
      }
      return read_object_or_array(_r, *obj, _view,
                                  _value_initialize_default_vals);
    }
  }

  /**
   * @brief Reads the data into a view assuming default values.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @param _view The view to read into.
   * @return An optional error.
   */
  static std::optional<Error> read_view_with_default(
      const R& _r, const InputVarType& _var,
      NamedTuple<FieldTypes...>* _view) noexcept {
    if constexpr (_no_field_names) {
      auto arr = _r.to_array(_var);
      if (!arr) [[unlikely]] {
        return arr.error();
      }
      return read_object_or_array_with_default(_r, *arr, _view);
    } else {
      auto obj = _r.to_object(_var);
      if (!obj) [[unlikely]] {
        return obj.error();
      }
      return read_object_or_array_with_default(_r, *obj, _view);
    }
  }

  /**
   * @brief Writes a named tuple to the output.
   *
   * @tparam P The type of the parent.
   * @param _w The writer to use.
   * @param _tup The named tuple to write.
   * @param _parent The parent object.
   */
  template <class P>
  static void write(const W& _w, const NamedTuple<FieldTypes...>& _tup,
                    const P& _parent) {
    if constexpr (_no_field_names) {
      auto arr = ParentType::add_array(_w, _tup.num_fields(), _parent);
      build_object(_w, _tup, &arr, std::make_integer_sequence<int, size_>());
      _w.end_array(&arr);
    } else {
      auto obj = ParentType::add_object(_w, _tup.num_fields(), _parent);
      build_object(_w, _tup, &obj, std::make_integer_sequence<int, size_>());
      _w.end_object(&obj);
    }
  }

  /**
   * @brief Generates the schema for the named tuple.
   *
   * @param _definitions The map of definitions to add to.
   * @param _view Optional view into a default-constructed instance.
   * @param _field_meta Optional out-of-band field metadata (`rfl::Metadata<U>`).
   * @param _raw_names Optional pre-processor field names for metadata lookup.
   * @return The schema type.
   */
  template <typename View = void>
  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions,
      View* _view = nullptr,
      const std::span<const FieldMeta> _field_meta = {},
      const std::span<const std::string_view> _raw_names = {}) noexcept {
    SchemaType schema;
    build_schema(_definitions, &schema, _view, _field_meta, _raw_names,
                 std::make_integer_sequence<int, size_>());
    return schema::Type{schema};
  }

 private:
  template <int... _is>
  static void build_object(const W& _w, const NamedTuple<FieldTypes...>& _tup,
                           OutputObjectOrArrayType* _ptr,
                           std::integer_sequence<int, _is...>) {
    (detail::write_named_tuple_field<R, W, _ignore_empty_containers,
                                     _all_required, _no_field_names,
                                     ProcessorsType>(
         _w, internal::nth_element_t<_is, FieldTypes...>::name_.string_view(),
         rfl::get<_is>(_tup), _ptr),
     ...);
  }

  template <typename View, int... _is>
  static void build_schema(std::map<std::string, schema::Type>* _definitions,
                           SchemaType* _schema, View* _view,
                           const std::span<const FieldMeta> _field_meta,
                           const std::span<const std::string_view> _raw_names,
                           std::integer_sequence<int, _is...>) noexcept {
    if constexpr (!std::is_same_v<View, void>) {
      (detail::build_named_tuple_field_schema<
           R, W, _no_field_names, ProcessorsType,
           std::remove_cvref_t<
               typename internal::nth_element_t<_is, FieldTypes...>::Type>>(
           _definitions, _schema,
           internal::nth_element_t<_is, FieldTypes...>::name(),
           static_cast<size_t>(_is) < _raw_names.size()
               ? _raw_names[static_cast<size_t>(_is)]
               : std::string_view{},
           _view ? detail::extract_schema_field_ptr(rfl::get<_is>(*_view))
                 : nullptr,
           _field_meta),
       ...);
    } else {
      (detail::build_named_tuple_field_schema<
           R, W, _no_field_names, ProcessorsType,
           std::remove_cvref_t<
               typename internal::nth_element_t<_is, FieldTypes...>::Type>>(
           _definitions, _schema,
           internal::nth_element_t<_is, FieldTypes...>::name(),
           static_cast<size_t>(_is) < _raw_names.size()
               ? _raw_names[static_cast<size_t>(_is)]
               : std::string_view{},
           nullptr, _field_meta),
       ...);
    }

    if constexpr (NamedTupleType::pos_extra_fields() != -1) {
      using F = internal::nth_element_t<NamedTupleType::pos_extra_fields(),
                                        FieldTypes...>;
      using ExtraFieldsType = std::remove_cvref_t<typename F::Type>;
      using U = std::remove_cvref_t<typename ExtraFieldsType::Type>;
      _schema->additional_properties_ = std::make_shared<schema::Type>(
          Parser<R, W, U, ProcessorsType>::to_schema(_definitions));
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType> &&
                         !_no_field_names) {
      _schema->additional_properties_ = false;
    }
  }

  /// Generates error messages for when fields are missing.
  template <bool _defaults_constructed, int... _is>
  static RFL_ALWAYS_INLINE void handle_missing_fields(
      const std::array<bool, size_>& _found, const NamedTupleType& _view,
      std::array<bool, size_>* _set, FieldErrors<ProcessorsType>* _errors,
      const bool _value_initialize_default_vals,
      std::integer_sequence<int, _is...>) noexcept {
    (detail::handle_missing_named_tuple_field<
         _ignore_empty_containers, _all_required, _defaults_constructed,
         ProcessorsType>(
         internal::nth_element_t<_is, FieldTypes...>::name(),
         std::get<_is>(_found), rfl::get<_is>(_view),
         _set ? &std::get<_is>(*_set) : nullptr, _errors,
         _value_initialize_default_vals),
     ...);
  }

  static RFL_ALWAYS_INLINE
      std::pair<std::array<bool, NamedTupleType::size()>, std::optional<Error>>
      read_object_or_array(const R& _r,
                           const InputObjectOrArrayType& _obj_or_arr,
                           NamedTupleType* _view,
                           const bool _value_initialize_default_vals) noexcept {
    auto found = std::array<bool, NamedTupleType::size()>();
    found.fill(false);
    auto set = std::array<bool, NamedTupleType::size()>();
    set.fill(false);
    FieldErrors<ProcessorsType> errors;
    const auto reader = ViewReaderType(&_r, _view, &found, &set, &errors);
    if constexpr (_no_field_names) {
      auto err = _r.read_array(reader, _obj_or_arr);
      if (err) [[unlikely]] {
        return std::make_pair(set, std::move(err));
      }
    } else {
      auto err = _r.read_object(reader, _obj_or_arr);
      if (err) [[unlikely]] {
        return std::make_pair(set, std::move(err));
      }
    }
    if (!should_stop_on_field_error<ProcessorsType>(errors)) {
      handle_missing_fields</*_defaults_constructed=*/false>(
          found, *_view, &set, &errors, _value_initialize_default_vals,
          std::make_integer_sequence<int, size_>());
    }
    if constexpr (internal::fail_fast_v<ProcessorsType>) {
      return std::make_pair(set, std::move(errors).error());
    } else {
      if (errors.size() != 0) [[unlikely]] {
        return std::make_pair(set, to_single_error(std::move(errors)));
      }
      return std::make_pair(set, std::optional<Error>());
    }
  }

  static std::optional<Error> read_object_or_array_with_default(
      const R& _r, const InputObjectOrArrayType& _obj_or_arr,
      NamedTupleType* _view) noexcept {
    FieldErrors<ProcessorsType> errors;
    const auto reader = ViewReaderWithDefaultType(&_r, _view, &errors);
    if constexpr (_no_field_names) {
      auto err = _r.read_array(reader, _obj_or_arr);
      if (err) [[unlikely]] {
        return err;
      }
    } else {
      auto err = _r.read_object(reader, _obj_or_arr);
      if (err) [[unlikely]] {
        return err;
      }
    }
    if constexpr (internal::has_default_val_v<NamedTupleType> &&
                  !internal::default_if_missing_v<ProcessorsType>) {
      if (!should_stop_on_field_error<ProcessorsType>(errors)) {
        handle_missing_fields</*_defaults_constructed=*/true>(
            reader.found(), *_view, nullptr, &errors,
            /*_value_initialize_default_vals=*/false,
            std::make_integer_sequence<int, size_>());
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
};

}  // namespace rfl::parsing

#endif
