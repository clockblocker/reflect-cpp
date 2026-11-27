#ifndef RFL_INTERNAL_IS_SAFE_DEFAULT_CONSTRUCTIBLE_HPP_
#define RFL_INTERNAL_IS_SAFE_DEFAULT_CONSTRUCTIBLE_HPP_

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#if __has_include(<expected>)
#include <expected>
#endif

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
#include "../Object.hpp"
#include "../Positional.hpp"
#include "../Ref.hpp"
#include "../Rename.hpp"
#include "../Short.hpp"
#include "../TaggedUnion.hpp"
#include "../Tuple.hpp"
#include "../Validator.hpp"
#include "../Variant.hpp"
#include "../named_tuple_t.hpp"
#include "Array.hpp"
#include "Skip.hpp"
#include "StringLiteral.hpp"
#include "has_reflection_type_v.hpp"
#include "has_reflector.hpp"
#include "nth_element_t.hpp"

namespace rfl::internal {

template <class T, class... Visited>
consteval bool is_safe_default_constructible_impl();

template <class T>
struct safe_default_is_optional : std::false_type {};

template <class V>
struct safe_default_is_optional<std::optional<V>> : std::true_type {};

template <class T>
struct safe_default_is_unique_ptr : std::false_type {};

template <class V, class D>
struct safe_default_is_unique_ptr<std::unique_ptr<V, D>> : std::true_type {};

template <class T>
struct safe_default_is_shared_ptr : std::false_type {};

template <class V>
struct safe_default_is_shared_ptr<std::shared_ptr<V>> : std::true_type {};

template <class T>
struct safe_default_is_extra_fields : std::false_type {};

template <class V>
struct safe_default_is_extra_fields<ExtraFields<V>> : std::true_type {};

template <class T>
struct safe_default_is_object : std::false_type {};

template <class V>
struct safe_default_is_object<Object<V>> : std::true_type {};

template <class T>
struct safe_default_is_box : std::false_type {};

template <class V, Copyability C>
struct safe_default_is_box<Box<V, C>> : std::true_type {
  using element_type = V;
};

template <class T>
struct safe_default_is_ref : std::false_type {};

template <class V>
struct safe_default_is_ref<Ref<V>> : std::true_type {
  using element_type = V;
};

template <class T>
struct safe_default_is_rfl_array : std::false_type {};

template <class V>
struct safe_default_is_rfl_array<Array<V>> : std::true_type {
  using std_array_type = typename Array<V>::StdArrayType;
};

template <class T>
struct safe_default_is_std_array : std::false_type {};

template <class V, size_t N>
struct safe_default_is_std_array<std::array<V, N>> : std::true_type {
  using element_type = V;
  static constexpr size_t size = N;
};

template <class T>
struct safe_default_is_std_variant : std::false_type {};

template <class... Alts>
struct safe_default_is_std_variant<std::variant<Alts...>> : std::true_type {};

template <class T>
struct safe_default_is_rfl_variant : std::false_type {};

template <class... Alts>
struct safe_default_is_rfl_variant<Variant<Alts...>> : std::true_type {};

template <class T>
struct safe_default_is_tagged_union : std::false_type {};

template <StringLiteral _disc, class... Alts>
struct safe_default_is_tagged_union<TaggedUnion<_disc, Alts...>>
    : std::true_type {};

template <class T>
struct safe_default_is_std_tuple : std::false_type {};

template <class... Ts>
struct safe_default_is_std_tuple<std::tuple<Ts...>> : std::true_type {};

template <class T>
struct safe_default_is_rfl_tuple : std::false_type {};

template <class... Ts>
struct safe_default_is_rfl_tuple<Tuple<Ts...>> : std::true_type {};

template <class T>
struct safe_default_is_std_pair : std::false_type {};

template <class A, class B>
struct safe_default_is_std_pair<std::pair<A, B>> : std::true_type {
  using first_type = A;
  using second_type = B;
};

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
template <class T>
struct safe_default_is_expected : std::false_type {};

template <class V, class E>
struct safe_default_is_expected<std::expected<V, E>> : std::true_type {
  using value_type = V;
};
#endif

template <class T>
struct safe_default_value_wrapper : std::false_type {};

template <StringLiteral _name, class V>
struct safe_default_value_wrapper<Rename<_name, V>> : std::true_type {
  using element_type = V;
};

template <StringLiteral _desc, class V>
struct safe_default_value_wrapper<Description<_desc, V>> : std::true_type {
  using element_type = V;
};

template <StringLiteral _msg, StringLiteral _desc, class V>
struct safe_default_value_wrapper<Deprecated<_msg, _desc, V>> : std::true_type {
  using element_type = V;
};

template <class V>
struct safe_default_value_wrapper<DefaultVal<V>> : std::true_type {
  using element_type = V;
};

template <class V>
struct safe_default_value_wrapper<Commented<V>> : std::true_type {
  using element_type = V;
};

template <class V>
struct safe_default_value_wrapper<Attribute<V>> : std::true_type {
  using element_type = V;
};

template <class V, bool _skip_ser, bool _skip_deser>
struct safe_default_value_wrapper<Skip<V, _skip_ser, _skip_deser>>
    : std::true_type {
  using element_type = V;
};

template <StringLiteral _name, class V>
struct safe_default_value_wrapper<Short<_name, V>> : std::true_type {
  using element_type = V;
};

template <class V>
struct safe_default_value_wrapper<Positional<V>> : std::true_type {
  using element_type = V;
};

template <class V>
struct safe_default_value_wrapper<Flatten<V>> : std::true_type {
  using element_type = V;
};

template <StringLiteral _name, class V>
struct safe_default_value_wrapper<Field<_name, V>> : std::true_type {
  using element_type = V;
};

template <class V, class Val, class... Vals>
struct safe_default_value_wrapper<Validator<V, Val, Vals...>> : std::true_type {
  using element_type = V;
};

template <class T, class... Visited>
struct safe_default_check_first_alt : std::false_type {};

template <class... Alts, class... Visited>
struct safe_default_check_first_alt<std::variant<Alts...>, Visited...> {
  static consteval bool eval() {
    if constexpr (sizeof...(Alts) == 0) {
      return false;
    } else {
      return is_safe_default_constructible_impl<nth_element_t<0, Alts...>,
                                                Visited...>();
    }
  }
  static constexpr bool value = eval();
};

template <class... Alts, class... Visited>
struct safe_default_check_first_alt<Variant<Alts...>, Visited...> {
  static consteval bool eval() {
    if constexpr (sizeof...(Alts) == 0) {
      return false;
    } else {
      return is_safe_default_constructible_impl<nth_element_t<0, Alts...>,
                                                Visited...>();
    }
  }
  static constexpr bool value = eval();
};

template <StringLiteral _disc, class... Alts, class... Visited>
struct safe_default_check_first_alt<TaggedUnion<_disc, Alts...>, Visited...> {
  static consteval bool eval() {
    if constexpr (sizeof...(Alts) == 0) {
      return false;
    } else {
      return is_safe_default_constructible_impl<nth_element_t<0, Alts...>,
                                                Visited...>();
    }
  }
  static constexpr bool value = eval();
};

template <class T, class... Visited>
struct safe_default_check_all_elements : std::false_type {};

template <class... Ts, class... Visited>
struct safe_default_check_all_elements<std::tuple<Ts...>, Visited...>
    : std::bool_constant<
          (is_safe_default_constructible_impl<Ts, Visited...>() && ...)> {};

template <class... Ts, class... Visited>
struct safe_default_check_all_elements<Tuple<Ts...>, Visited...>
    : std::bool_constant<
          (is_safe_default_constructible_impl<Ts, Visited...>() && ...)> {};

// Checks every field of a reflected aggregate struct U (where
// std::is_default_constructible_v<U> is already known to be true).
// If a field type F is itself not std::is_default_constructible_v<F>, U can
// only be default-constructible because F has a default member initializer in
// U, so U{} will not call F{}. Otherwise F may be default-constructed by U{}
// and must be recursively verified.
template <class FieldsTuple, class... Visited>
struct safe_default_check_aggregate_fields : std::false_type {};

template <class... FieldTypes, class... Visited>
struct safe_default_check_aggregate_fields<Tuple<FieldTypes...>, Visited...> {
  template <class FieldType>
  static consteval bool check_one() {
    using F = typename std::remove_cvref_t<FieldType>::Type;
    if constexpr (!std::is_default_constructible_v<F>) {
      return true;
    } else {
      return is_safe_default_constructible_impl<F, Visited...>();
    }
  }

  static constexpr bool value = (check_one<FieldTypes>() && ...);
};

/// Checks whether T can be safely default-constructed without triggering hard
/// errors inside unconstrained Box/Ref/Tuple default constructors or infinite
/// runtime recursion on cyclic Box/Ref graphs.
template <class T, class... Visited>
consteval bool is_safe_default_constructible_impl() {
  if constexpr (std::is_reference_v<T>) {
    return false;
  } else {
    using U = std::remove_cvref_t<T>;
    if constexpr ((std::is_same_v<U, Visited> || ...)) {
      return false;
    } else if constexpr (safe_default_is_optional<U>::value ||
                         safe_default_is_shared_ptr<U>::value ||
                         safe_default_is_extra_fields<U>::value ||
                         safe_default_is_object<U>::value ||
                         std::is_pointer_v<U> || std::is_same_v<U, Generic> ||
                         std::is_same_v<U, std::atomic_flag>) {
      return true;
    } else if constexpr (safe_default_is_unique_ptr<U>::value) {
      return std::is_default_constructible_v<U>;
    } else if constexpr (safe_default_is_box<U>::value) {
      return is_safe_default_constructible_impl<
          typename safe_default_is_box<U>::element_type, U, Visited...>();
    } else if constexpr (safe_default_is_ref<U>::value) {
      return is_safe_default_constructible_impl<
          typename safe_default_is_ref<U>::element_type, U, Visited...>();
#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
    } else if constexpr (safe_default_is_expected<U>::value) {
      using V = typename safe_default_is_expected<U>::value_type;
      if constexpr (std::is_void_v<V>) {
        return true;
      } else {
        return std::is_default_constructible_v<U> &&
               is_safe_default_constructible_impl<V, U, Visited...>();
      }
#endif
    } else if constexpr (safe_default_is_std_variant<U>::value ||
                         safe_default_is_rfl_variant<U>::value ||
                         safe_default_is_tagged_union<U>::value) {
      return safe_default_check_first_alt<U, U, Visited...>::value;
    } else if constexpr (safe_default_is_std_tuple<U>::value ||
                         safe_default_is_rfl_tuple<U>::value) {
      return safe_default_check_all_elements<U, U, Visited...>::value;
    } else if constexpr (safe_default_is_std_pair<U>::value) {
      return is_safe_default_constructible_impl<
                 typename safe_default_is_std_pair<U>::first_type, U,
                 Visited...>() &&
             is_safe_default_constructible_impl<
                 typename safe_default_is_std_pair<U>::second_type, U,
                 Visited...>();
    } else if constexpr (safe_default_is_std_array<U>::value) {
      if constexpr (safe_default_is_std_array<U>::size == 0) {
        return true;
      } else {
        return is_safe_default_constructible_impl<
            typename safe_default_is_std_array<U>::element_type, U,
            Visited...>();
      }
    } else if constexpr (safe_default_is_rfl_array<U>::value) {
      return is_safe_default_constructible_impl<
          typename safe_default_is_rfl_array<U>::std_array_type, U,
          Visited...>();
    } else if constexpr (std::is_bounded_array_v<U>) {
      return is_safe_default_constructible_impl<std::remove_extent_t<U>, U,
                                                Visited...>();
    } else if constexpr (std::is_unbounded_array_v<U>) {
      return false;
    } else if constexpr (safe_default_value_wrapper<U>::value) {
      return std::is_default_constructible_v<U> &&
             is_safe_default_constructible_impl<
                 typename safe_default_value_wrapper<U>::element_type, U,
                 Visited...>();
    } else if constexpr (std::is_default_constructible_v<U> &&
                         std::is_class_v<U> && std::is_aggregate_v<U> &&
                         !has_reflection_type_v<U> && !has_read_reflector<U> &&
                         !has_write_reflector<U>) {
      if constexpr (requires { typename named_tuple_t<U>; }) {
        using Fields = typename named_tuple_t<U>::Fields;
        return safe_default_check_aggregate_fields<Fields, U,
                                                   Visited...>::value;
      } else {
        return true;
      }
    } else {
      return std::is_default_constructible_v<U>;
    }
  }
}

template <class T>
constexpr bool is_safe_default_constructible_v =
    is_safe_default_constructible_impl<T>();

template <class T>
struct is_safe_default_constructible
    : std::bool_constant<is_safe_default_constructible_v<T>> {};

}  // namespace rfl::internal

#endif  // RFL_INTERNAL_IS_SAFE_DEFAULT_CONSTRUCTIBLE_HPP_
