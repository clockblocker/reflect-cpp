#ifndef RFL_INTERNAL_ISIMPLICITVALUE_HPP_
#define RFL_INTERNAL_ISIMPLICITVALUE_HPP_

#include <type_traits>

#include "../ImplicitValue.hpp"
#include "StringLiteral.hpp"

namespace rfl::internal {

template <class T>
class is_implicit_value;

template <class T>
class is_implicit_value : public std::false_type {};

template <class Type, StringLiteral _implicit>
class is_implicit_value<ImplicitValue<Type, _implicit>>
    : public std::true_type {};

template <class T>
constexpr bool is_implicit_value_v =
    is_implicit_value<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace rfl::internal

#endif
