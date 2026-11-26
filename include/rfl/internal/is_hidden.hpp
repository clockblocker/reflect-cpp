#ifndef RFL_INTERNAL_ISHIDDEN_HPP_
#define RFL_INTERNAL_ISHIDDEN_HPP_

#include <type_traits>

#include "../Hidden.hpp"

namespace rfl::internal {

template <class T>
class is_hidden;

template <class T>
class is_hidden : public std::false_type {};

template <class Type>
class is_hidden<Hidden<Type>> : public std::true_type {};

template <class T>
constexpr bool is_hidden_v =
    is_hidden<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace rfl::internal

#endif
