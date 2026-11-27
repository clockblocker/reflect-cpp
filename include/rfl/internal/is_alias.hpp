#ifndef RFL_INTERNAL_ISALIAS_HPP_
#define RFL_INTERNAL_ISALIAS_HPP_

#include <type_traits>

#include "StringLiteral.hpp"

namespace rfl {

template <internal::StringLiteral _name, class T>
struct Alias;

namespace internal {

template <class T>
class is_alias : public std::false_type {};

template <StringLiteral _name, class Type>
class is_alias<Alias<_name, Type>> : public std::true_type {};

template <class T>
constexpr bool is_alias_v =
    is_alias<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

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

}  // namespace internal
}  // namespace rfl

#endif
