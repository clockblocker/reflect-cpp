#ifndef RFL_INTERNAL_IS_VALUE_NAME_HPP_
#define RFL_INTERNAL_IS_VALUE_NAME_HPP_

#include <type_traits>

#include "../ValueName.hpp"
#include "StringLiteral.hpp"

namespace rfl::internal {

template <class T>
class is_value_name : public std::false_type {};

template <internal::StringLiteral _value_name, class T>
class is_value_name<ValueName<_value_name, T>> : public std::true_type {};

template <class T>
constexpr bool is_value_name_v =
    is_value_name<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace rfl::internal

#endif
