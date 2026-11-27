#ifndef RFL_INTERNAL_FIELD_ALIASES_HPP_
#define RFL_INTERNAL_FIELD_ALIASES_HPP_

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Alias.hpp"
#include "../Attribute.hpp"
#include "../Commented.hpp"
#include "../DefaultVal.hpp"
#include "../Deprecated.hpp"
#include "../Description.hpp"
#include "../Positional.hpp"
#include "../Rename.hpp"
#include "../Short.hpp"
#include "../Skip.hpp"
#include "../Tuple.hpp"
#include "StringLiteral.hpp"

namespace rfl::internal {

/// Compile-time list of read-only alias names declared on a field.
template <StringLiteral... _names>
struct AliasList {
  static constexpr size_t size = sizeof...(_names);

  static constexpr std::array<std::string_view, sizeof...(_names)>
  to_array() noexcept {
    return {_names.string_view()...};
  }

  static constexpr bool contains(std::string_view _name) noexcept {
    return ((_name == _names.string_view()) || ...);
  }
};

template <StringLiteral _head, class List>
struct prepend_alias;

template <StringLiteral _head, StringLiteral... _tail>
struct prepend_alias<_head, AliasList<_tail...>> {
  using type = AliasList<_head, _tail...>;
};

/// Collects all rfl::Alias names declared on a field type, unwrapping through
/// transparent field wrappers (Alias, Rename, Description, Deprecated,
/// DefaultVal, Short, Positional, std::optional, Skip, Attribute, Commented).
template <class T>
struct field_aliases_impl {
  using type = AliasList<>;
};

template <class T>
using field_aliases_t = typename field_aliases_impl<
    std::remove_cvref_t<std::remove_pointer_t<T>>>::type;

template <StringLiteral _name, class T>
struct field_aliases_impl<Alias<_name, T>> {
  using type = typename prepend_alias<_name, field_aliases_t<T>>::type;
};

template <StringLiteral _name, class T>
struct field_aliases_impl<Rename<_name, T>> {
  using type = field_aliases_t<T>;
};

template <StringLiteral _desc, class T>
struct field_aliases_impl<Description<_desc, T>> {
  using type = field_aliases_t<T>;
};

template <StringLiteral _msg, StringLiteral _desc, class T>
struct field_aliases_impl<Deprecated<_msg, _desc, T>> {
  using type = field_aliases_t<T>;
};

template <class T>
struct field_aliases_impl<DefaultVal<T>> {
  using type = field_aliases_t<T>;
};

template <StringLiteral _name, class T>
struct field_aliases_impl<Short<_name, T>> {
  using type = field_aliases_t<T>;
};

template <class T>
struct field_aliases_impl<Positional<T>> {
  using type = field_aliases_t<T>;
};

template <class T>
struct field_aliases_impl<std::optional<T>> {
  using type = field_aliases_t<T>;
};

template <class T, bool _skip_serialization, bool _skip_deserialization>
struct field_aliases_impl<Skip<T, _skip_serialization, _skip_deserialization>> {
  using type = field_aliases_t<T>;
};

template <class T>
struct field_aliases_impl<Attribute<T>> {
  using type = field_aliases_t<T>;
};

template <class T>
struct field_aliases_impl<Commented<T>> {
  using type = field_aliases_t<T>;
};

/// Returns true if any field in Fields declares at least one rfl::Alias.
template <class Fields>
consteval bool has_any_aliases() {
  constexpr size_t N = rfl::tuple_size_v<Fields>;
  return []<size_t... Is>(std::index_sequence<Is...>) {
    return (false || ... ||
            (field_aliases_t<typename std::remove_cvref_t<
                 rfl::tuple_element_t<Is, Fields>>::Type>::size > 0));
  }(std::make_index_sequence<N>());
}

/// Checks at compile time that no alias on field A equals the primary wire name
/// or any alias on a different field B.
template <class Fields>
consteval bool no_alias_collisions() {
  constexpr size_t N = rfl::tuple_size_v<Fields>;
  if constexpr (N <= 1 || !has_any_aliases<Fields>()) {
    return true;
  } else {
    constexpr auto primary_names =
        []<size_t... Is>(std::index_sequence<Is...>) {
          return std::array<std::string_view, N>{
              std::remove_cvref_t<rfl::tuple_element_t<Is, Fields>>::name()...};
        }(std::make_index_sequence<N>());

    constexpr size_t total_aliases =
        []<size_t... Is>(std::index_sequence<Is...>) {
          return (size_t{0} + ... +
                  field_aliases_t<typename std::remove_cvref_t<
                      rfl::tuple_element_t<Is, Fields>>::Type>::size);
        }(std::make_index_sequence<N>());

    struct AliasEntry {
      size_t field_idx;
      std::string_view name;
    };

    std::array<AliasEntry, total_aliases> all_aliases{};
    size_t pos = 0;
    [&]<size_t... Is>(std::index_sequence<Is...>) {
      (
          [&]<size_t I>() {
            constexpr auto arr =
                field_aliases_t<typename std::remove_cvref_t<
                    rfl::tuple_element_t<I, Fields>>::Type>::to_array();
            for (const auto sv : arr) {
              all_aliases[pos++] = AliasEntry{I, sv};
            }
          }.template operator()<Is>(),
          ...);
    }(std::make_index_sequence<N>());

    for (size_t a = 0; a < total_aliases; ++a) {
      for (size_t f = 0; f < N; ++f) {
        if (f != all_aliases[a].field_idx &&
            all_aliases[a].name == primary_names[f]) {
          return false;
        }
      }
      for (size_t b = a + 1; b < total_aliases; ++b) {
        if (all_aliases[a].field_idx != all_aliases[b].field_idx &&
            all_aliases[a].name == all_aliases[b].name) {
          return false;
        }
      }
    }
    return true;
  }
}

}  // namespace rfl::internal

#endif
