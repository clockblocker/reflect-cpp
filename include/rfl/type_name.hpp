#ifndef RFL_TYPE_NAME_HPP_
#define RFL_TYPE_NAME_HPP_

#include <string_view>

#include "internal/get_type_name.hpp"

namespace rfl {

/// Compile-time null-terminated std::string_view containing the name of type T
/// (without instantiating an rfl::Literal type).
/// @tparam T The type whose name to retrieve
template <class T>
inline constexpr std::string_view type_name_v =
    internal::type_name_literal_v<T>.string_view();

/// Returns the compile-time name of type T as a null-terminated
/// std::string_view (usable in both constexpr and runtime contexts).
/// @tparam T The type whose name to retrieve
template <class T>
constexpr std::string_view type_name() noexcept {
  return type_name_v<T>;
}

}  // namespace rfl

#endif
