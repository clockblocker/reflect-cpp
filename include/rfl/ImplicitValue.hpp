#ifndef RFL_IMPLICITVALUE_HPP_
#define RFL_IMPLICITVALUE_HPP_

#include "internal/StringLiteral.hpp"
#include "internal/TransparentWrapper.hpp"

namespace rfl {

/// Lets a CLI option be given without a value, like Boost.Program_options'
/// implicit_value: `--help` stands for `--help=<_implicit>`, and `--help=x`
/// sets `x`. `--help x` never takes `x` as the value, so `x` stays a
/// positional argument. If the option is absent, the field is read as usual
/// (std::optional gives std::nullopt, rfl::DefaultVal gives its default).
/// For non-CLI formats (JSON, YAML, etc.), this is transparent.
template <class T, internal::StringLiteral _implicit = "">
struct ImplicitValue
    : internal::TransparentWrapper<ImplicitValue<T, _implicit>, T> {
  using Base = internal::TransparentWrapper<ImplicitValue<T, _implicit>, T>;
  using Base::Base;
  using Base::operator=;

  static constexpr auto implicit_value_ = _implicit;

  ImplicitValue()
    requires std::is_default_constructible_v<T>
  = default;
  ImplicitValue(const ImplicitValue&) = default;
  ImplicitValue(ImplicitValue&&) = default;
  ImplicitValue& operator=(const ImplicitValue&) = default;
  ImplicitValue& operator=(ImplicitValue&&) = default;

  template <class U>
  ImplicitValue(const ImplicitValue<U, _implicit>& _field)
      : Base(_field.get()) {}

  template <class U>
  ImplicitValue(ImplicitValue<U, _implicit>&& _field) noexcept(
      std::is_nothrow_constructible_v<T, U&&>)
      : Base(std::move(_field.value_)) {}

  template <class U>
  auto& operator=(const ImplicitValue<U, _implicit>& _field) {
    this->value_ = _field.get();
    return *this;
  }

  template <class U>
  auto& operator=(ImplicitValue<U, _implicit>&& _field) {
    this->value_ = std::move(_field.value_);
    return *this;
  }
};

}  // namespace rfl

#endif
