#ifndef RFL_INTERNAL_ISALIAS_HPP_
#define RFL_INTERNAL_ISALIAS_HPP_

#include <type_traits>

#include "../Alias.hpp"
#include "StringLiteral.hpp"

namespace rfl::internal {

template <class T>
struct remove_alias {
  using Type = T;
};

template <StringLiteral _name, class T>
struct remove_alias<Alias<_name, T>> {
  using Type = typename remove_alias<std::remove_cvref_t<T>>::Type;
};

template <class T>
using remove_alias_t =
    typename remove_alias<std::remove_cvref_t<std::remove_pointer_t<T>>>::Type;

}  // namespace rfl::internal

#endif
