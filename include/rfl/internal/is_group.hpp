#ifndef RFL_INTERNAL_ISGROUP_HPP_
#define RFL_INTERNAL_ISGROUP_HPP_

#include <type_traits>

#include "../Group.hpp"
#include "StringLiteral.hpp"

namespace rfl::internal {

template <class T>
class is_group;

template <class T>
class is_group : public std::false_type {};

template <internal::StringLiteral _group, class Type>
class is_group<Group<_group, Type>> : public std::true_type {};

template <class T>
constexpr bool is_group_v =
    is_group<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace rfl::internal

#endif
