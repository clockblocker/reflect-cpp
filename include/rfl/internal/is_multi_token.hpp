#ifndef RFL_INTERNAL_IS_MULTI_TOKEN_HPP_
#define RFL_INTERNAL_IS_MULTI_TOKEN_HPP_

#include <type_traits>

#include "../MultiToken.hpp"

namespace rfl::internal {

template <class T>
class is_multi_token : public std::false_type {};

template <class T>
class is_multi_token<MultiToken<T>> : public std::true_type {};

template <class T>
constexpr bool is_multi_token_v =
    is_multi_token<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace rfl::internal

#endif
