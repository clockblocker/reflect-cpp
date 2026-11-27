#ifndef RFL_DEFAULTHINT_HPP_
#define RFL_DEFAULTHINT_HPP_

#include <type_traits>

#include "internal/StringLiteral.hpp"
#include "internal/TransparentWrapper.hpp"

namespace rfl {

/// Attaches a display-only default value string shown by `rfl::cli::usage()`
/// without changing parsing semantics. Useful on `std::optional<T>` fields
/// when the application needs to distinguish "omitted" from "explicitly passed
/// the default value" while still documenting the effective default in `--help`.
/// @tparam _hint The default value text shown as `Default: <_hint>.`
/// @tparam T The wrapped field type (typically `std::optional<U>`)
template <internal::StringLiteral _hint, class T>
struct DefaultHint
    : internal::TransparentWrapper<DefaultHint<_hint, T>, T> {
  using Base = internal::TransparentWrapper<DefaultHint<_hint, T>, T>;
  using Base::Base;
  using Base::operator=;

  static constexpr internal::StringLiteral default_hint_ = _hint;
};

namespace internal {

template <class T>
struct is_default_hint : std::false_type {};

template <StringLiteral _hint, class T>
struct is_default_hint<DefaultHint<_hint, T>> : std::true_type {};

template <class T>
constexpr bool is_default_hint_v =
    is_default_hint<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace internal

}  // namespace rfl

#endif
