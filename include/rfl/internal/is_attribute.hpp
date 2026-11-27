#ifndef RFL_INTERNAL_ISATTRIBUTE_HPP_
#define RFL_INTERNAL_ISATTRIBUTE_HPP_

#include <type_traits>

#include "../Attribute.hpp"
#include "is_alias.hpp"
#include "remove_rename.hpp"

namespace rfl::internal {

template <class T>
class is_attribute;

template <class T>
class is_attribute : public std::false_type {};

template <class Type>
class is_attribute<Attribute<Type>> : public std::true_type {};

template <class T>
constexpr bool is_attribute_v =
    is_attribute<remove_alias_t<remove_rename_t<remove_alias_t<T>>>>::value;

}  // namespace rfl::internal

#endif
