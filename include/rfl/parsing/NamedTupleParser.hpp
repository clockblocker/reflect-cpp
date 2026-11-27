#ifndef RFL_PARSING_NAMEDTUPLEPARSER_HPP_
#define RFL_PARSING_NAMEDTUPLEPARSER_HPP_

#include <array>
#include <atomic>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#if __has_include(<expected>)
#include <expected>
#endif

#include "../Alias.hpp"
#include "../Attribute.hpp"
#include "../Box.hpp"
#include "../Commented.hpp"
#include "../DefaultVal.hpp"
#include "../Deprecated.hpp"
#include "../Description.hpp"
#include "../ExtraFields.hpp"
#include "../Field.hpp"
#include "../Flatten.hpp"
#include "../Generic.hpp"
#include "../Literal.hpp"
#include "../NamedTuple.hpp"
#include "../Object.hpp"
#include "../Positional.hpp"
#include "../Ref.hpp"
#include "../Rename.hpp"
#include "../Result.hpp"
#include "../Short.hpp"
#include "../TaggedUnion.hpp"
#include "../Tuple.hpp"
#include "../Validator.hpp"
#include "../Variant.hpp"
#include "../internal/Array.hpp"
#include "../internal/Skip.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/default_if_missing_v.hpp"
#include "../internal/default_if_null_v.hpp"
#include "../internal/field_aliases.hpp"
#include "../internal/has_default_val_v.hpp"
#include "../internal/has_reflection_method_v.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/has_reflector.hpp"
#include "../internal/is_attribute.hpp"
#include "../internal/is_basic_type.hpp"
#include "../internal/is_default_val_v.hpp"
#include "../internal/is_deprecated.hpp"
#include "../internal/is_description.hpp"
#include "../internal/is_extra_fields.hpp"
#include "../internal/is_safe_default_constructible.hpp"
#include "../internal/is_skip.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "../internal/nth_element_t.hpp"
#include "../internal/ptr_cast.hpp"
#include "../named_tuple_t.hpp"
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
#include "is_nullable.hpp"
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

/// Visited type list used to break cycles in recursive types when evaluating
/// `can_write_generic`.
template <class... Ts>
struct CanWriteVisited {};

template <class T, class Visited>
struct is_in_visited : std::false_type {};

template <class T, class... Ts>
struct is_in_visited<T, CanWriteVisited<Ts...>>
    : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

template <class T, class Visited>
struct add_visited;

template <class T, class... Ts>
struct add_visited<T, CanWriteVisited<Ts...>> {
  using type = CanWriteVisited<T, Ts...>;
};

template <class T>
concept HasValueTypeRange = std::is_class_v<T> && requires(const T& _t) {
  typename T::value_type;
  _t.begin();
  _t.end();
};

template <class T>
concept HasMappedType =
    std::is_class_v<T> && requires { typename T::mapped_type; };

template <class T, class Visited = CanWriteVisited<>>
struct can_write_generic_impl;

template <class T, class Visited = CanWriteVisited<>>
struct can_write_generic_checked {
  using U = std::remove_cvref_t<T>;
  static constexpr bool value =
      std::conditional_t<is_in_visited<U, Visited>::value, std::true_type,
                         can_write_generic_impl<U, Visited>>::value;
};

template <class T, class Visited = CanWriteVisited<>>
inline constexpr bool can_write_generic_v =
    can_write_generic_checked<T, Visited>::value;

/// Trait for whether the generic writer can serialize `T`, i.e. whether a
/// schema `default` can be produced for it. Wrappers, containers, tuples,
/// variants, and reflected aggregate structs are inspected recursively with
/// cycle protection. Types that can only be constructed from `ReflectionType`
/// (or only have a read `Reflector`) cannot be written back to `Generic`.
template <class T, class Visited>
struct can_write_generic_impl {
  using U = std::remove_cvref_t<T>;
  using NextVisited = typename add_visited<U, Visited>::type;

  static consteval bool eval() {
    if constexpr (std::is_void_v<U>) {
      return false;
    } else if constexpr (is_basic_type_v<U> || std::is_enum_v<U> ||
                         std::is_same_v<U, std::string_view> ||
                         std::is_same_v<U, std::wstring> ||
                         std::is_same_v<U, std::wstring_view>) {
      return true;
    } else if constexpr (has_read_reflector<U> && !has_write_reflector<U>) {
      return false;
    } else if constexpr (has_write_reflector<U>) {
      return can_write_generic_v<typename Reflector<U>::ReflType, NextVisited>;
    } else if constexpr (has_reflection_type_v<U>) {
      if constexpr (has_reflection_method_v<U> || IsReflectionTypeWrapper<U>) {
        return can_write_generic_v<typename U::ReflectionType, NextVisited>;
      } else {
        return false;
      }
    } else if constexpr (HasValueTypeRange<U>) {
      if constexpr (HasMappedType<U>) {
        return can_write_generic_v<typename U::value_type, NextVisited> &&
               can_write_generic_v<typename U::mapped_type, NextVisited>;
      } else {
        return can_write_generic_v<typename U::value_type, NextVisited>;
      }
    } else if constexpr (std::is_class_v<U> && std::is_aggregate_v<U>) {
      return can_write_generic_v<named_tuple_t<U>, NextVisited>;
    } else {
      return true;
    }
  }

  static constexpr bool value = eval();
};

template <class T, class Visited>
struct can_write_generic_impl<DefaultVal<T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<DefaultVal<T>, Visited>::type>> {};

template <StringLiteral _name, class T, class Visited>
struct can_write_generic_impl<Rename<_name, T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<Rename<_name, T>, Visited>::type>> {};

template <StringLiteral _name, class T, class Visited>
struct can_write_generic_impl<Alias<_name, T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<Alias<_name, T>, Visited>::type>> {};

template <StringLiteral _desc, class T, class Visited>
struct can_write_generic_impl<Description<_desc, T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<Description<_desc, T>, Visited>::type>> {};

template <StringLiteral _msg, StringLiteral _desc, class T, class Visited>
struct can_write_generic_impl<Deprecated<_msg, _desc, T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T,
          typename add_visited<Deprecated<_msg, _desc, T>, Visited>::type>> {};

template <StringLiteral _name, class T, class Visited>
struct can_write_generic_impl<Short<_name, T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<Short<_name, T>, Visited>::type>> {};

template <class T, class Visited>
struct can_write_generic_impl<Positional<T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<Positional<T>, Visited>::type>> {};

template <class T, class Visited>
struct can_write_generic_impl<Commented<T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<Commented<T>, Visited>::type>> {};

template <class T, class Visited>
struct can_write_generic_impl<Attribute<T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<Attribute<T>, Visited>::type>> {};

template <class T, Copyability C, class Visited>
struct can_write_generic_impl<Box<T, C>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<Box<T, C>, Visited>::type>> {};

template <class T, class Visited>
struct can_write_generic_impl<Ref<T>, Visited>
    : std::bool_constant<
          can_write_generic_v<T, typename add_visited<Ref<T>, Visited>::type>> {
};

template <class T, class V, class... Vs, class Visited>
struct can_write_generic_impl<Validator<T, V, Vs...>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<Validator<T, V, Vs...>, Visited>::type>> {};

template <StringLiteral... _s, class Visited>
struct can_write_generic_impl<Literal<_s...>, Visited> : std::true_type {};

template <class T, bool _skip_serialization, bool _skip_deserialization,
          class Visited>
struct can_write_generic_impl<
    Skip<T, _skip_serialization, _skip_deserialization>, Visited>
    : std::bool_constant<
          _skip_serialization ||
          can_write_generic_v<
              T, typename add_visited<
                     Skip<T, _skip_serialization, _skip_deserialization>,
                     Visited>::type>> {};

template <class T, class Visited>
struct can_write_generic_impl<Flatten<T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<Flatten<T>, Visited>::type>> {};

template <StringLiteral _name, class T, class Visited>
struct can_write_generic_impl<Field<_name, T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<Field<_name, T>, Visited>::type>> {};

template <class... FieldTypes, class Visited>
struct can_write_generic_impl<NamedTuple<FieldTypes...>, Visited>
    : std::bool_constant<
          (can_write_generic_v<
               typename FieldTypes::Type,
               typename add_visited<NamedTuple<FieldTypes...>, Visited>::type> &&
           ...)> {};

template <class T, class Visited>
struct can_write_generic_impl<Result<T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<Result<T>, Visited>::type>> {};

template <class T, class Visited>
struct can_write_generic_impl<Object<T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<Object<T>, Visited>::type>> {};

template <class T, class Visited>
struct can_write_generic_impl<ExtraFields<T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<ExtraFields<T>, Visited>::type>> {};

template <class Visited>
struct can_write_generic_impl<Generic, Visited> : std::true_type {};

template <class T, class Visited>
struct can_write_generic_impl<std::optional<T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<std::optional<T>, Visited>::type>> {};

template <class T, class Visited>
struct can_write_generic_impl<std::shared_ptr<T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<std::shared_ptr<T>, Visited>::type>> {};

template <class T, class D, class Visited>
struct can_write_generic_impl<std::unique_ptr<T, D>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<std::unique_ptr<T, D>, Visited>::type>> {};

template <class T, class Visited>
struct can_write_generic_impl<std::reference_wrapper<T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<std::reference_wrapper<T>, Visited>::type>> {
};

template <class T, size_t Extent, class Visited>
struct can_write_generic_impl<std::span<T, Extent>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<std::span<T, Extent>, Visited>::type>> {};

template <class T, class Visited>
struct can_write_generic_impl<std::atomic<T>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<std::atomic<T>, Visited>::type>> {};

template <class Visited>
struct can_write_generic_impl<std::atomic_flag, Visited> : std::true_type {};

template <class T, size_t N, class Visited>
struct can_write_generic_impl<std::array<T, N>, Visited>
    : std::bool_constant<can_write_generic_v<
          T, typename add_visited<std::array<T, N>, Visited>::type>> {};

template <class T, class Visited>
struct can_write_generic_impl<Array<T>, Visited>
    : std::bool_constant<can_write_generic_v<
          std::remove_all_extents_t<T>,
          typename add_visited<Array<T>, Visited>::type>> {};

template <class T, class Visited>
struct can_write_generic_impl<T*, Visited>
    : std::bool_constant<
          can_write_generic_v<T, typename add_visited<T*, Visited>::type>> {};

template <class T, size_t N, class Visited>
struct can_write_generic_impl<T[N], Visited>
    : std::bool_constant<
          can_write_generic_v<T, typename add_visited<T[N], Visited>::type>> {};

template <class T1, class T2, class Visited>
struct can_write_generic_impl<std::pair<T1, T2>, Visited>
    : std::bool_constant<
          can_write_generic_v<
              T1, typename add_visited<std::pair<T1, T2>, Visited>::type> &&
          can_write_generic_v<
              T2, typename add_visited<std::pair<T1, T2>, Visited>::type>> {};

template <class... Ts, class Visited>
struct can_write_generic_impl<std::tuple<Ts...>, Visited>
    : std::bool_constant<
          (can_write_generic_v<
               Ts, typename add_visited<std::tuple<Ts...>, Visited>::type> &&
           ...)> {};

template <class... Ts, class Visited>
struct can_write_generic_impl<Tuple<Ts...>, Visited>
    : std::bool_constant<
          (can_write_generic_v<
               Ts, typename add_visited<Tuple<Ts...>, Visited>::type> &&
           ...)> {};

template <class... Ts, class Visited>
struct can_write_generic_impl<std::variant<Ts...>, Visited>
    : std::bool_constant<
          (can_write_generic_v<
               Ts, typename add_visited<std::variant<Ts...>, Visited>::type> &&
           ...)> {};

template <class... Ts, class Visited>
struct can_write_generic_impl<Variant<Ts...>, Visited>
    : std::bool_constant<
          (can_write_generic_v<
               Ts, typename add_visited<Variant<Ts...>, Visited>::type> &&
           ...)> {};

template <StringLiteral _discriminator, class... Ts, class Visited>
struct can_write_generic_impl<TaggedUnion<_discriminator, Ts...>, Visited>
    : std::bool_constant<
          (can_write_generic_v<
               Ts, typename add_visited<TaggedUnion<_discriminator, Ts...>,
                                        Visited>::type> &&
           ...)> {};

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
template <class T, class E, class Visited>
struct can_write_generic_impl<std::expected<T, E>, Visited>
    : std::bool_constant<
          can_write_generic_v<
              T, typename add_visited<std::expected<T, E>, Visited>::type> &&
          can_write_generic_v<
              E, typename add_visited<std::expected<T, E>, Visited>::type>> {};
#endif

template <class T>
struct can_write_generic : std::bool_constant<can_write_generic_v<T>> {};

/// Satisfied when the generic writer can serialize `T`, i.e. when a schema
/// `default` can be produced for it. Every type is writable, except those that
/// can be constructed from `ReflectionType`, but not converted back to it.
template <class T>
concept CanWriteGeneric = can_write_generic<std::remove_cvref_t<T>>::value;

/// Satisfied when a schema `default` can be emitted for the field type `T`:
/// it must contain a `DefaultVal` (see `is_default_val_v`) whose value
/// satisfies `CanWriteGeneric`.
template <class T>
concept CanEmitDefault =
    is_default_val_v<T> && CanWriteGeneric<typename default_val_t<T>::Type>;

}  // namespace internal
}  // namespace rfl

namespace rfl::parsing {

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

  using ViewReaderOverlayType = std::conditional_t<
      _no_field_names,
      ViewReaderWithDefaultAndStrippedFieldNames<R, W, NamedTupleType,
                                                 ProcessorsType, true>,
      ViewReaderWithDefault<R, W, NamedTupleType, ProcessorsType, true>>;

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

  static_assert(
      internal::no_alias_collisions<typename NamedTupleType::Fields>(),
      "Field alias collides with another field's name or alias.");

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
    if constexpr (internal::default_if_null_v<ProcessorsType> &&
                  (internal::is_safe_default_constructible_v<
                       typename std::remove_cvref_t<FieldTypes>::Type> &&
                   ...)) {
      if (_r.is_empty(_var)) {
        try {
          return NamedTuple<FieldTypes...>(
              typename std::remove_cvref_t<FieldTypes>::Type{}...);
        } catch (std::exception& e) {
          return error(e.what());
        }
      }
    }
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
   *         field that is not inside rfl::Short, rfl::Positional,
   *         rfl::Description or rfl::Deprecated is value-initialized.
   *         Otherwise it is required, because the default value in the
   *         struct's in-class initializer is not available.
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
   * @brief Overlays input data onto an existing named tuple, preserving
   *        fields that are absent from the input.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @param _base The base named tuple to overlay onto.
   * @return A Result containing the overlaid named tuple or an error.
   */
  static Result<NamedTuple<FieldTypes...>> read_into(
      const R& _r, const InputVarType& _var,
      NamedTuple<FieldTypes...> _base) noexcept {
    try {
      auto view = rfl::to_view(_base);
      using ViewType = std::remove_cvref_t<decltype(view)>;
      auto err = Parser<R, W, ViewType, ProcessorsType>::read_view_into(
          _r, _var, &view);
      if (err) [[unlikely]] {
        return error(std::move(*err));
      }
      return make_result_safe<Result<NamedTuple<FieldTypes...>>>(
          std::move(_base));
    } catch (std::exception& e) {
      return error(e.what());
    }
  }

  static std::optional<Error> read_into_in_place(
      const R& _r, const InputVarType& _var,
      NamedTuple<FieldTypes...>* _base) noexcept {
    try {
      auto view = rfl::to_view(*_base);
      using ViewType = std::remove_cvref_t<decltype(view)>;
      return Parser<R, W, ViewType, ProcessorsType>::read_view_into(_r, _var,
                                                                    &view);
    } catch (std::exception& e) {
      return Error(e.what());
    }
  }

  /**
   * @brief Overlays input data into a view, preserving missing fields and
   *        recursively overlaying nested structs.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @param _view The view to overlay into.
   * @return An optional error.
   */
  static std::optional<Error> read_view_into(
      const R& _r, const InputVarType& _var,
      NamedTuple<FieldTypes...>* _view) noexcept {
    if constexpr (_no_field_names) {
      auto arr = _r.to_array(_var);
      if (!arr) [[unlikely]] {
        return arr.error();
      }
      return read_object_or_array_into(_r, *arr, _view);
    } else {
      auto obj = _r.to_object(_var);
      if (!obj) [[unlikely]] {
        return obj.error();
      }
      return read_object_or_array_into(_r, *obj, _view);
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
      auto obj =
          ParentType::add_object(_w, num_serialized_fields(_tup), _parent);
      build_object(_w, _tup, &obj, std::make_integer_sequence<int, size_>());
      _w.end_object(&obj);
    }
  }

  template <int _i>
  static void write_single_field(const W& _w,
                                 const NamedTuple<FieldTypes...>& _tup,
                                 OutputObjectOrArrayType* _ptr) {
    add_field_to_object<_i>(_w, _tup, _ptr);
  }

  static void write_fields(const W& _w, const NamedTuple<FieldTypes...>& _tup,
                           OutputObjectOrArrayType* _ptr) {
    build_object(_w, _tup, _ptr, std::make_integer_sequence<int, size_>());
  }

  template <typename View, size_t _i>
  static void add_single_field_to_schema(
      std::map<std::string, schema::Type>* _definitions, SchemaType* _schema,
      View* _view) noexcept {
    add_field_to_schema<View, _i>(_definitions, _schema, _view);
  }

  /**
   * @brief Generates the schema for the named tuple.
   *
   * @param _definitions The map of definitions to add to.
   * @return The schema type.
   */
  template <typename View = void>
  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions,
      View* _view = nullptr) noexcept {
    SchemaType schema;
    build_schema(_definitions, &schema, _view,
                 std::make_integer_sequence<int, size_>());
    return schema::Type{schema};
  }

 private:
  static constexpr size_t num_skipped_serialization_fields() noexcept {
    return (size_t{0} + ... +
            (internal::skip_serialization_v<
                 std::remove_cvref_t<typename FieldTypes::Type>>
                 ? size_t{1}
                 : size_t{0}));
  }

  static size_t num_serialized_fields(
      const NamedTuple<FieldTypes...>& _tup) noexcept {
    return _tup.num_fields() - num_skipped_serialization_fields();
  }

  template <int _i>
  static void add_field_to_object(const W& _w,
                                  const NamedTuple<FieldTypes...>& _tup,
                                  OutputObjectOrArrayType* _ptr) {
    using FieldType = internal::nth_element_t<_i, FieldTypes...>;
    using ValueType = std::remove_cvref_t<typename FieldType::Type>;
    if constexpr (!_no_field_names &&
                  internal::skip_serialization_v<ValueType>) {
      return;
    } else {
      const auto value = rfl::get<_i>(_tup);
      if constexpr (internal::is_extra_fields_v<ValueType>) {
        for (const auto& [k, v] : *value) {
          const auto new_parent = make_parent(k, _ptr);
          Parser<R, W, std::remove_cvref_t<decltype(v)>, ProcessorsType>::write(
              _w, v, new_parent);
        }
      } else if constexpr (!_all_required && !_no_field_names &&
                           !is_required<ValueType,
                                        _ignore_empty_containers>()) {
        constexpr auto name = FieldType::name_.string_view();
        const auto new_parent = make_parent(name, _ptr);
        if (!is_empty(value)) {
          if constexpr (internal::is_attribute_v<ValueType>) {
            Parser<R, W, ValueType, ProcessorsType>::write(
                _w, value, new_parent.as_attribute());
          } else {
            Parser<R, W, ValueType, ProcessorsType>::write(_w, value,
                                                           new_parent);
          }
        }
      } else {
        constexpr auto name = FieldType::name_.string_view();
        const auto new_parent = make_parent(name, _ptr);
        if constexpr (internal::is_attribute_v<ValueType>) {
          Parser<R, W, ValueType, ProcessorsType>::write(
              _w, value, new_parent.as_attribute());
        } else {
          Parser<R, W, ValueType, ProcessorsType>::write(_w, value, new_parent);
        }
      }
    }
  }

  template <typename View, size_t _i>
  static void add_field_to_schema(
      std::map<std::string, schema::Type>* _definitions,
      SchemaType* _schema,
      View* _view) noexcept {
    using F = internal::nth_element_t<_i, FieldTypes...>;
    using U = std::remove_cvref_t<typename F::Type>;
    if constexpr (!internal::skip_serialization_v<U> &&
                  !internal::is_extra_fields_v<U>) {
      // Add default value here
      auto s = Parser<R, W, U, ProcessorsType>::to_schema(_definitions);
      if constexpr (!std::is_same_v<View, void>) {
        if (_view != nullptr) {
          if constexpr (internal::is_default_val_v<U>) {
            if constexpr (internal::CanEmitDefault<U>) {
              try {
                set_default_value(
                    rfl::to_generic<ProcessorsType>(
                        internal::get_default_val(*rfl::get<_i>(*_view))
                            .value()),
                    &s);
              } catch (...) {
              }
            }
          } else if constexpr (internal::default_if_missing_v<ProcessorsType> &&
                               internal::CanWriteGeneric<U>) {
            const auto& field_val = *rfl::get<_i>(*_view);
            bool skip_default = false;
            if constexpr (std::is_pointer_v<U>) {
              skip_default = (field_val == nullptr);
            } else if constexpr (!is_required<U, false>()) {
              skip_default = is_empty(field_val);
            }
            if (!skip_default) {
              try {
                auto default_val = rfl::to_generic<ProcessorsType>(field_val);
                s = schema::Type{schema::Type::DefaultVal{
                    .type_ = Ref<schema::Type>::make(std::move(s)),
                    .default_value_ = std::move(default_val)}};
              } catch (...) {
              }
            }
          }
        }
      }
      if constexpr (_no_field_names) {
        _schema->types_.emplace_back(std::move(s));
      } else {
        _schema->types_[std::string(F::name())] = std::move(s);
      }
    }
  }

  /// Sets the default value of the DefaultVal node in `_schema`. Rename, Short
  /// and Positional leave no node in the schema, but Description and
  /// Deprecated wrap the DefaultVal node in one of their own.
  static void set_default_value(Generic _default, schema::Type* _schema) {
    _schema->variant_.visit([&](auto& _s) {
      using S = std::remove_cvref_t<decltype(_s)>;
      if constexpr (std::is_same_v<S, schema::Type::DefaultVal>) {
        _s.default_value_ = std::move(_default);
      } else if constexpr (std::is_same_v<S, schema::Type::Description> ||
                           std::is_same_v<S, schema::Type::Deprecated>) {
        set_default_value(std::move(_default), &*_s.type_);
      }
    });
  }

  template <int... _is>
  static void build_object(const W& _w, const NamedTuple<FieldTypes...>& _tup,
                           OutputObjectOrArrayType* _ptr,
                           std::integer_sequence<int, _is...>) {
    (add_field_to_object<_is>(_w, _tup, _ptr), ...);
  }

  template <typename View,  int... _is>
  static void build_schema(std::map<std::string, schema::Type>* _definitions,
                           SchemaType* _schema,
                           View* _view,
                           std::integer_sequence<int, _is...>) noexcept {
    (add_field_to_schema<View, _is>(_definitions, _schema, _view), ...);

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
  template <int _i>
  static RFL_ALWAYS_INLINE void handle_one_missing_field(
      const std::array<bool, size_>& _found, const NamedTupleType& _view,
      std::array<bool, size_>* _set, FieldErrors<ProcessorsType>* _errors,
      const bool _value_initialize_default_vals) noexcept {
    if (should_stop_on_field_error<ProcessorsType>(*_errors)) return;
    using FieldType = internal::nth_element_t<_i, FieldTypes...>;
    using ValueType = std::remove_reference_t<
        std::remove_pointer_t<typename FieldType::Type>>;

    if (!std::get<_i>(_found)) {
      constexpr bool is_default_val_field =
          internal::is_default_val_v<ValueType>;
      constexpr bool is_required_field =
          !is_default_val_field && !internal::is_extra_fields_v<ValueType> &&
          !internal::skip_deserialization_v<ValueType> &&
          (_all_required || is_required<ValueType, _ignore_empty_containers>());

      // Without _set, the struct was default-constructed before it was read,
      // so the field already holds its default value. With _set, the fields
      // are constructed in raw memory. An rfl::NamedTuple has no in-class
      // initializers, so a missing bare rfl::DefaultVal field is
      // value-initialized there. Otherwise the field's default value is not
      // available, so the field is required.
      const bool defaults_constructed = _set == nullptr;

      if constexpr (internal::is_unwrapped_default_val_v<ValueType>) {
        if constexpr (can_value_initialize_default_val<ValueType>()) {
          if (_value_initialize_default_vals && !defaults_constructed) {
            // The value's constructor can throw, for instance a Validator
            // whose zero value is invalid.
            try {
              ::new (rfl::get<_i>(_view)) ValueType();
            } catch (std::exception& e) {
              constexpr auto current_name =
                  internal::nth_element_t<_i, FieldTypes...>::name();
              _errors->emplace_back(make_nested_error(
                  std::string(current_name),
                  "Field named '" + std::string(current_name) +
                      "' not found, and its default value could not be "
                      "constructed: " +
                      e.what(),
                  ErrorKind::missing_field));
              return;
            }
            std::get<_i>(*_set) = true;
            return;
          }
        }
      }

      if (is_required_field ||
          (is_default_val_field && !defaults_constructed)) {
        constexpr auto current_name =
            internal::nth_element_t<_i, FieldTypes...>::name();
        std::stringstream stream;
        stream << "Field named '" << std::string(current_name)
               << "' not found.";
        _errors->emplace_back(make_nested_error(
            std::string(current_name), stream.str(), ErrorKind::missing_field));
        return;
      }

      if constexpr (!is_required_field && !is_default_val_field) {
        if (defaults_constructed) {
          if constexpr (internal::default_if_null_v<ProcessorsType> &&
                        !internal::default_if_missing_v<ProcessorsType> &&
                        !internal::has_default_val_v<NamedTupleType> &&
                        is_nullable_v<ValueType>) {
            using NonConstT = std::remove_const_t<ValueType>;
            if constexpr (internal::is_safe_default_constructible_v<NonConstT>) {
              auto* const ptr = const_cast<NonConstT*>(rfl::get<_i>(_view));
              const auto reset = [&]() {
                if constexpr (std::is_const_v<ValueType> ||
                              !std::is_move_assignable_v<NonConstT>) {
                  if constexpr (std::is_nothrow_default_constructible_v<
                                    NonConstT>) {
                    std::destroy_at(ptr);
                    ::new (ptr) NonConstT{};
                  } else if constexpr (std::is_move_constructible_v<
                                           NonConstT>) {
                    NonConstT tmp{};
                    std::destroy_at(ptr);
                    ::new (ptr) NonConstT(std::move(tmp));
                  }
                } else {
                  *ptr = NonConstT{};
                }
              };
              if constexpr (std::is_nothrow_default_constructible_v<NonConstT> &&
                            (std::is_const_v<ValueType> ||
                             !std::is_move_assignable_v<NonConstT> ||
                             std::is_nothrow_move_assignable_v<NonConstT>)) {
                reset();
              } else {
                try {
                  reset();
                } catch (std::exception& e) {
                  constexpr auto current_name =
                      internal::nth_element_t<_i, FieldTypes...>::name();
                  _errors->emplace_back(make_nested_error(
                      std::string(current_name),
                      "Field named '" + std::string(current_name) +
                          "' not found, and its default value could not be "
                          "constructed: " +
                          e.what()));
                }
              }
            }
          }
          return;
        }
        using NonConstT = std::remove_const_t<ValueType>;
        if constexpr (!internal::is_safe_default_constructible_v<NonConstT>) {
          constexpr auto current_name =
              internal::nth_element_t<_i, FieldTypes...>::name();
          std::stringstream stream;
          stream << "Field named '" << std::string(current_name)
                 << "' not found.";
          _errors->emplace_back(make_nested_error(
              std::string(current_name), stream.str(), ErrorKind::missing_field));
          return;
        } else {
          const auto ptr = const_cast<NonConstT*>(rfl::get<_i>(_view));
          if constexpr (std::is_nothrow_default_constructible_v<NonConstT>) {
            ::new (ptr) NonConstT();
          } else {
            // The default value can throw, for instance a Validator whose
            // default value is invalid.
            try {
              ::new (ptr) NonConstT();
            } catch (std::exception& e) {
              constexpr auto current_name =
                  internal::nth_element_t<_i, FieldTypes...>::name();
              _errors->emplace_back(make_nested_error(
                  std::string(current_name),
                  "Field named '" + std::string(current_name) +
                      "' not found, and its default value could not be "
                      "constructed: " +
                      e.what(),
                  ErrorKind::missing_field));
              return;
            }
          }
          std::get<_i>(*_set) = true;
        }
      }
    }
  }

  /// Whether a missing rfl::DefaultVal field can be value-initialized. Only
  /// an rfl::NamedTuple does this at runtime, and it cannot have const fields,
  /// so const fields of structs need no instantiation.
  template <class ValueType>
  static constexpr bool can_value_initialize_default_val() {
    using T = typename internal::default_val_t<ValueType>::Type;
    return !std::is_const_v<ValueType> && std::is_default_constructible_v<T> &&
           std::is_default_constructible_v<ValueType>;
  }

  /// Generates error messages for when fields are missing.
  template <int... _is>
  static RFL_ALWAYS_INLINE void handle_missing_fields(
      const std::array<bool, size_>& _found, const NamedTupleType& _view,
      std::array<bool, size_>* _set, FieldErrors<ProcessorsType>* _errors,
      const bool _value_initialize_default_vals,
      std::integer_sequence<int, _is...>) noexcept {
    (handle_one_missing_field<_is>(_found, _view, _set, _errors,
                                   _value_initialize_default_vals),
     ...);
  }

  static auto make_parent(const std::string_view& _name,
                          OutputObjectOrArrayType* _ptr) {
    if constexpr (_no_field_names) {
      return typename ParentType::Array{_ptr};
    } else {
      return typename ParentType::Object{_name, _ptr};
    }
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
      handle_missing_fields(found, *_view, &set, &errors,
                            _value_initialize_default_vals,
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
    if constexpr (!internal::default_if_missing_v<ProcessorsType>) {
      if (!should_stop_on_field_error<ProcessorsType>(errors)) {
        handle_missing_fields(reader.found(), *_view, nullptr, &errors,
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

  static std::optional<Error> read_object_or_array_into(
      const R& _r, const InputObjectOrArrayType& _obj_or_arr,
      NamedTupleType* _view) noexcept {
    FieldErrors<ProcessorsType> errors;
    const auto reader = ViewReaderOverlayType(&_r, _view, &errors);
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
