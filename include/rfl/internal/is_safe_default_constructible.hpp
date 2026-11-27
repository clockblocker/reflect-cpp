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

namespace rfl::internal {

/// Visited type list used to detect recursive default-construction cycles
/// (which would otherwise cause either infinite template recursion or runtime
/// stack overflow on cyclic Box/Ref graphs).
template <class... Ts>
struct SafeDefaultVisited {};

template <class T, class Visited>
struct is_in_safe_default_visited : std::false_type {};

template <class T, class... Ts>
struct is_in_safe_default_visited<T, SafeDefaultVisited<Ts...>>
    : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

template <class T, class Visited>
struct add_safe_default_visited;

template <class T, class... Ts>
struct add_safe_default_visited<T, SafeDefaultVisited<Ts...>> {
  using type = SafeDefaultVisited<T, Ts...>;
};

template <class T, class Visited = SafeDefaultVisited<>>
struct is_safe_default_constructible_impl;

template <class T, class Visited = SafeDefaultVisited<>>
struct is_safe_default_constructible_checked {
  using U = std::remove_cvref_t<T>;
  static constexpr bool value =
      std::conditional_t<is_in_safe_default_visited<U, Visited>::value,
                         std::false_type,
                         is_safe_default_constructible_impl<U, Visited>>::value;
};

template <class T, class Visited = SafeDefaultVisited<>>
inline constexpr bool is_safe_default_constructible_v =
    is_safe_default_constructible_checked<T, Visited>::value;

template <class FieldsTuple, class Visited>
struct are_fields_safe_default_constructible : std::true_type {};

template <class... FieldTypes, class Visited>
struct are_fields_safe_default_constructible<Tuple<FieldTypes...>, Visited>
    : std::conjunction<is_safe_default_constructible_checked<
          typename FieldTypes::Type, Visited>...> {};

/// Trait for whether `T{}` is both well-formed AND guaranteed not to trigger
/// an unconstrained `Box()`/`Ref()` instantiation error or infinite recursive
/// default construction (e.g. `struct DirectNode { rfl::Box<DirectNode> next; }`).
template <class T, class Visited>
struct is_safe_default_constructible_impl {
  using U = std::remove_cvref_t<T>;
  using NextVisited = typename add_safe_default_visited<U, Visited>::type;

  static consteval bool eval() {
    if constexpr (std::is_pointer_v<U>) {
      return true;
    } else if constexpr (std::is_default_constructible_v<U> &&
                         std::is_class_v<U> && std::is_aggregate_v<U> &&
                         !has_reflection_type_v<U> && !has_read_reflector<U> &&
                         !has_write_reflector<U>) {
      if constexpr (requires { typename named_tuple_t<U>; }) {
        return are_fields_safe_default_constructible<
            typename named_tuple_t<U>::Fields, NextVisited>::value;
      } else {
        return true;
      }
    } else {
      return std::is_default_constructible_v<U>;
    }
  }

  static constexpr bool value = eval();
};

// Empty / null-constructing types: default construction never constructs V.
template <class V, class Visited>
struct is_safe_default_constructible_impl<std::optional<V>, Visited>
    : std::true_type {};

template <class V, class D, class Visited>
struct is_safe_default_constructible_impl<std::unique_ptr<V, D>, Visited>
    : std::is_default_constructible<std::unique_ptr<V, D>> {};

template <class V, class Visited>
struct is_safe_default_constructible_impl<std::shared_ptr<V>, Visited>
    : std::true_type {};

template <class Visited>
struct is_safe_default_constructible_impl<Generic, Visited> : std::true_type {};

template <class V, class Visited>
struct is_safe_default_constructible_impl<Object<V>, Visited>
    : std::true_type {};

template <class V, class Visited>
struct is_safe_default_constructible_impl<ExtraFields<V>, Visited>
    : std::true_type {};

template <class Visited>
struct is_safe_default_constructible_impl<std::monostate, Visited>
    : std::true_type {};

template <class Visited>
struct is_safe_default_constructible_impl<std::atomic_flag, Visited>
    : std::true_type {};

// Always-populated smart pointers: default construction constructs V{}.
template <class V, Copyability C, class Visited>
struct is_safe_default_constructible_impl<Box<V, C>, Visited>
    : is_safe_default_constructible_checked<
          V, typename add_safe_default_visited<Box<V, C>, Visited>::type> {};

template <class V, class Visited>
struct is_safe_default_constructible_impl<Ref<V>, Visited>
    : is_safe_default_constructible_checked<
          V, typename add_safe_default_visited<Ref<V>, Visited>::type> {};

// Variants / tagged unions: default construction constructs the 1st alternative.
template <class Visited>
struct is_safe_default_constructible_impl<std::variant<>, Visited>
    : std::false_type {};

template <class First, class... Rest, class Visited>
struct is_safe_default_constructible_impl<std::variant<First, Rest...>, Visited>
    : std::conjunction<
          std::is_default_constructible<std::variant<First, Rest...>>,
          is_safe_default_constructible_checked<
              First,
              typename add_safe_default_visited<std::variant<First, Rest...>,
                                                Visited>::type>> {};

template <class Visited>
struct is_safe_default_constructible_impl<Variant<>, Visited>
    : std::false_type {};

template <class First, class... Rest, class Visited>
struct is_safe_default_constructible_impl<Variant<First, Rest...>, Visited>
    : std::conjunction<
          std::is_default_constructible<Variant<First, Rest...>>,
          is_safe_default_constructible_checked<
              First,
              typename add_safe_default_visited<Variant<First, Rest...>,
                                                Visited>::type>> {};

template <StringLiteral _disc, class Visited>
struct is_safe_default_constructible_impl<TaggedUnion<_disc>, Visited>
    : std::false_type {};

template <StringLiteral _disc, class First, class... Rest, class Visited>
struct is_safe_default_constructible_impl<TaggedUnion<_disc, First, Rest...>,
                                          Visited>
    : std::conjunction<
          std::is_default_constructible<TaggedUnion<_disc, First, Rest...>>,
          is_safe_default_constructible_checked<
              First, typename add_safe_default_visited<
                         TaggedUnion<_disc, First, Rest...>, Visited>::type>> {
};

// Tuples / pairs: default construction constructs every element.
template <class... Ts, class Visited>
struct is_safe_default_constructible_impl<std::tuple<Ts...>, Visited>
    : std::conjunction<
          std::is_default_constructible<std::tuple<Ts...>>,
          is_safe_default_constructible_checked<
              Ts, typename add_safe_default_visited<std::tuple<Ts...>,
                                                    Visited>::type>...> {};

template <class... Ts, class Visited>
struct is_safe_default_constructible_impl<Tuple<Ts...>, Visited>
    : std::conjunction<
          std::is_default_constructible<Tuple<Ts...>>,
          is_safe_default_constructible_checked<
              Ts, typename add_safe_default_visited<Tuple<Ts...>,
                                                    Visited>::type>...> {};

template <class A, class B, class Visited>
struct is_safe_default_constructible_impl<std::pair<A, B>, Visited>
    : std::conjunction<
          std::is_default_constructible<std::pair<A, B>>,
          is_safe_default_constructible_checked<
              A,
              typename add_safe_default_visited<std::pair<A, B>, Visited>::type>,
          is_safe_default_constructible_checked<
              B, typename add_safe_default_visited<std::pair<A, B>,
                                                   Visited>::type>> {};

// Fixed-size arrays: zero-length arrays do not construct any element.
template <class V, size_t N, class Visited>
struct is_safe_default_constructible_impl<std::array<V, N>, Visited>
    : std::conditional_t<
          N == 0, std::true_type,
          is_safe_default_constructible_checked<
              V, typename add_safe_default_visited<std::array<V, N>,
                                                   Visited>::type>> {};

template <class V, size_t N, class Visited>
struct is_safe_default_constructible_impl<V[N], Visited>
    : std::conditional_t<
          N == 0, std::true_type,
          is_safe_default_constructible_checked<
              V, typename add_safe_default_visited<V[N], Visited>::type>> {};

template <class V, class Visited>
struct is_safe_default_constructible_impl<Array<V>, Visited>
    : is_safe_default_constructible_checked<
          typename Array<V>::StdArrayType,
          typename add_safe_default_visited<Array<V>, Visited>::type> {};

// Value-owning wrappers: check wrapper default constructibility, then recurse.
template <StringLiteral _name, class V, class Visited>
struct is_safe_default_constructible_impl<Rename<_name, V>, Visited>
    : std::conjunction<
          std::is_default_constructible<Rename<_name, V>>,
          is_safe_default_constructible_checked<
              V, typename add_safe_default_visited<Rename<_name, V>,
                                                   Visited>::type>> {};

template <StringLiteral _desc, class V, class Visited>
struct is_safe_default_constructible_impl<Description<_desc, V>, Visited>
    : std::conjunction<
          std::is_default_constructible<Description<_desc, V>>,
          is_safe_default_constructible_checked<
              V, typename add_safe_default_visited<Description<_desc, V>,
                                                   Visited>::type>> {};

template <StringLiteral _msg, StringLiteral _desc, class V, class Visited>
struct is_safe_default_constructible_impl<Deprecated<_msg, _desc, V>, Visited>
    : std::conjunction<
          std::is_default_constructible<Deprecated<_msg, _desc, V>>,
          is_safe_default_constructible_checked<
              V, typename add_safe_default_visited<Deprecated<_msg, _desc, V>,
                                                   Visited>::type>> {};

template <class V, class Visited>
struct is_safe_default_constructible_impl<DefaultVal<V>, Visited>
    : std::conjunction<
          std::is_default_constructible<DefaultVal<V>>,
          is_safe_default_constructible_checked<
              V, typename add_safe_default_visited<DefaultVal<V>,
                                                   Visited>::type>> {};

template <class V, class Visited>
struct is_safe_default_constructible_impl<Commented<V>, Visited>
    : std::conjunction<
          std::is_default_constructible<Commented<V>>,
          is_safe_default_constructible_checked<
              V, typename add_safe_default_visited<Commented<V>,
                                                   Visited>::type>> {};

template <class V, class Visited>
struct is_safe_default_constructible_impl<Attribute<V>, Visited>
    : std::conjunction<
          std::is_default_constructible<Attribute<V>>,
          is_safe_default_constructible_checked<
              V, typename add_safe_default_visited<Attribute<V>,
                                                   Visited>::type>> {};

template <class V, bool _skip_s, bool _skip_d, class Visited>
struct is_safe_default_constructible_impl<Skip<V, _skip_s, _skip_d>, Visited>
    : std::conjunction<
          std::is_default_constructible<Skip<V, _skip_s, _skip_d>>,
          is_safe_default_constructible_checked<
              V, typename add_safe_default_visited<Skip<V, _skip_s, _skip_d>,
                                                   Visited>::type>> {};

template <StringLiteral _name, class V, class Visited>
struct is_safe_default_constructible_impl<Short<_name, V>, Visited>
    : std::conjunction<
          std::is_default_constructible<Short<_name, V>>,
          is_safe_default_constructible_checked<
              V, typename add_safe_default_visited<Short<_name, V>,
                                                   Visited>::type>> {};

template <class V, class Visited>
struct is_safe_default_constructible_impl<Positional<V>, Visited>
    : std::conjunction<
          std::is_default_constructible<Positional<V>>,
          is_safe_default_constructible_checked<
              V, typename add_safe_default_visited<Positional<V>,
                                                   Visited>::type>> {};

template <class V, class Visited>
struct is_safe_default_constructible_impl<Flatten<V>, Visited>
    : std::conjunction<
          std::is_default_constructible<Flatten<V>>,
          is_safe_default_constructible_checked<
              V, typename add_safe_default_visited<Flatten<V>,
                                                   Visited>::type>> {};

template <StringLiteral _name, class V, class Visited>
struct is_safe_default_constructible_impl<Field<_name, V>, Visited>
    : std::conjunction<
          std::is_default_constructible<Field<_name, V>>,
          is_safe_default_constructible_checked<
              V, typename add_safe_default_visited<Field<_name, V>,
                                                   Visited>::type>> {};

template <class V, class Pred, class... Preds, class Visited>
struct is_safe_default_constructible_impl<Validator<V, Pred, Preds...>, Visited>
    : std::conjunction<
          std::is_default_constructible<Validator<V, Pred, Preds...>>,
          is_safe_default_constructible_checked<
              V, typename add_safe_default_visited<Validator<V, Pred, Preds...>,
                                                   Visited>::type>> {};

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
template <class V, class E, class Visited>
struct is_safe_default_constructible_impl<std::expected<V, E>, Visited>
    : std::conjunction<
          std::is_default_constructible<std::expected<V, E>>,
          std::conditional_t<
              std::is_void_v<V>, std::true_type,
              is_safe_default_constructible_checked<
                  V, typename add_safe_default_visited<std::expected<V, E>,
                                                       Visited>::type>>> {};
#endif

template <class T>
struct is_safe_default_constructible
    : std::bool_constant<is_safe_default_constructible_v<T>> {};

}  // namespace rfl::internal

#endif  // RFL_INTERNAL_IS_SAFE_DEFAULT_CONSTRUCTIBLE_HPP_
