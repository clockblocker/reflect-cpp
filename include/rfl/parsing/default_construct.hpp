#ifndef RFL_PARSING_DEFAULT_CONSTRUCT_HPP_
#define RFL_PARSING_DEFAULT_CONSTRUCT_HPP_

#include <exception>
#include <type_traits>

#include "../Result.hpp"
#include "../internal/is_safe_default_constructible.hpp"

namespace rfl::parsing {

/// Default-constructs T, catching any exception thrown by T's default
/// constructor and returning it as an rfl::Error so noexcept parsers never
/// terminate.
template <class T>
  requires internal::is_safe_default_constructible_v<T>
Result<T> default_construct() noexcept {
  if constexpr (std::is_nothrow_default_constructible_v<T> &&
                std::is_nothrow_move_constructible_v<T>) {
    return T{};
  } else {
    try {
      return T{};
    } catch (std::exception& e) {
      return error(e.what());
    }
  }
}

}  // namespace rfl::parsing

#endif
