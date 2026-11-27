#ifndef RFL_INTERNAL_ADD_NULL_FIELDS_V_HPP_
#define RFL_INTERNAL_ADD_NULL_FIELDS_V_HPP_

#include <type_traits>

#include "../AddNullFields.hpp"
#include "../Processors.hpp"

namespace rfl::internal {

template <class ProcessorsType>
struct add_null_fields;

template <>
struct add_null_fields<Processors<>> : std::false_type {};

template <class Head, class... Tail>
struct add_null_fields<Processors<Head, Tail...>>
    : std::bool_constant<std::is_same_v<Head, AddNullFields> ||
                         add_null_fields<Processors<Tail...>>::value> {};

template <class ProcessorsType>
constexpr bool add_null_fields_v =
    add_null_fields<std::remove_cvref_t<ProcessorsType>>::value;

}  // namespace rfl::internal

#endif
