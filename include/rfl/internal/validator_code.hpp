#ifndef RFL_INTERNAL_VALIDATOR_CODE_HPP_
#define RFL_INTERNAL_VALIDATOR_CODE_HPP_

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "Error.hpp"

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
    using CodeT = std::remove_cvref_t<decltype(V::code)>;
    if constexpr (std::is_null_pointer_v<CodeT>) {
      return {};
    } else if constexpr (std::is_pointer_v<CodeT>) {
      return V::code ? std::string_view(V::code) : std::string_view{};
    } else {
      return std::string_view(V::code);
    }
  } else {
    return {};
  }
}

/// Composes a wrapped validator error (e.g. `Size<V>` or `CodePoints<V>`)
/// preserving zero-allocation built-in codes whenever possible, and preserving
/// allocated nested details/causes (e.g. `Size<AnyOf<...>>`) when present.
template <class V, class ComposeFn>
Error compose_prefixed_validator_error(Error&& _inner_err,
                                       const std::string_view _msg_prefix,
                                       const std::string_view _code_prefix,
                                       const BuiltinErrorCode _fallback_builtin,
                                       ComposeFn&& _compose_builtin) {
  set_error_code_if_empty(_inner_err, validator_code<V>());
  auto msg = std::string(_msg_prefix) + _inner_err.what();
  if (has_allocated_details(_inner_err)) {
    auto details = std::move(_inner_err).details();
    const auto fallback_str = builtin_error_code_str(_fallback_builtin);
    for (auto& d : details) {
      d.message = std::string(_msg_prefix) + d.message;
      if (d.kind == ErrorKind::other) {
        d.kind = ErrorKind::constraint_violation;
      }
      d.code = d.code.empty() ? std::string(fallback_str)
                              : std::string(_code_prefix) + d.code;
    }
    return Error(std::move(msg), std::move(details));
  }
  const auto inner_builtin = builtin_error_code_of(_inner_err);
  if (inner_builtin != BuiltinErrorCode::none) {
    const auto composed = _compose_builtin(inner_builtin);
    if (composed != BuiltinErrorCode::none) {
      return Error(std::move(msg), ErrorKind::constraint_violation, composed);
    }
  }
  const auto inner_code = _inner_err.code();
  if (inner_code.empty()) {
    return Error(std::move(msg), ErrorKind::constraint_violation,
                 _fallback_builtin);
  }
  return Error(std::move(msg), ErrorKind::constraint_violation,
               std::string(_code_prefix) + std::string(inner_code));
}

}  // namespace rfl::internal

#endif
