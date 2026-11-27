#ifndef RFL_INTERNAL_ADD_NULL_FIELDS_V_HPP_
#define RFL_INTERNAL_ADD_NULL_FIELDS_V_HPP_

#include <type_traits>

#include "../AddNullFields.hpp"
#include "../Processors.hpp"

namespace rfl::internal {

template <class T>
struct add_null_fields : std::false_type {};

template <>
struct add_null_fields<AddNullFields> : std::true_type {};

template <class... Ps>
struct add_null_fields<Processors<Ps...>>
    : std::bool_constant<(add_null_fields<Ps>::value || ...)> {};

template <class T>
inline constexpr bool add_null_fields_v =
    add_null_fields<std::remove_cvref_t<T>>::value;

}  // namespace rfl::internal

#endif
