#ifndef RFL_INTERNAL_NODUPLICATEKEYS_HPP_
#define RFL_INTERNAL_NODUPLICATEKEYS_HPP_

#include <type_traits>

#include "../NoDuplicateKeys.hpp"
#include "../Processors.hpp"

namespace rfl::internal {

template <class T>
struct no_duplicate_keys : std::false_type {};

template <>
struct no_duplicate_keys<NoDuplicateKeys> : std::true_type {};

template <class... Ps>
struct no_duplicate_keys<Processors<Ps...>>
    : std::bool_constant<(no_duplicate_keys<Ps>::value || ...)> {};

template <class T>
inline constexpr bool no_duplicate_keys_v =
    no_duplicate_keys<std::remove_cvref_t<T>>::value;

}  // namespace rfl::internal

#endif
