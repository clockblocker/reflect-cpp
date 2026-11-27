#ifndef RFL_INTERNAL_VALIDATOR_CODE_HPP_
#define RFL_INTERNAL_VALIDATOR_CODE_HPP_

#include <concepts>
#include <string_view>
#include <type_traits>

namespace rfl::internal {

/// Extracts an optional declarative rule code from a validator type V.
/// Supports:
///   - `static constexpr std::string_view code = "...";`
///   - `static constexpr const char* code = "...";` / `char code[] = "...";`
///   - `static constexpr rfl::internal::StringLiteral code = "...";`
template <class V>
constexpr std::string_view validator_code() noexcept {
  if constexpr (requires {
                  { V::code.string_view() }
                      -> std::convertible_to<std::string_view>;
                }) {
    return V::code.string_view();
  } else if constexpr (requires {
                         { V::code } -> std::convertible_to<std::string_view>;
                       }) {
    if constexpr (std::is_pointer_v<std::remove_cvref_t<decltype(V::code)>>) {
      return V::code ? std::string_view(V::code) : std::string_view{};
    } else {
      return std::string_view(V::code);
    }
  } else {
    return {};
  }
}

}  // namespace rfl::internal

#endif
