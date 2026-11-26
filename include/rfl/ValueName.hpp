#ifndef RFL_VALUENAME_HPP_
#define RFL_VALUENAME_HPP_

#include "internal/StringLiteral.hpp"
#include "internal/TransparentWrapper.hpp"

namespace rfl {

/// Sets a custom value placeholder name (metavar, e.g. `"path"` or `"<NUM>"`)
/// for a CLI option in `rfl::cli::usage` and `rfl::cli::read_app`.
/// For non-CLI formats (JSON, YAML, etc.), this is transparent.
template <internal::StringLiteral _value_name, class T>
struct ValueName : internal::TransparentWrapper<ValueName<_value_name, T>, T> {
  using Base = internal::TransparentWrapper<ValueName<_value_name, T>, T>;
  using Base::Base;
  using Base::operator=;

  static constexpr auto value_name_ = _value_name;
  static_assert(_value_name.length > 0, "ValueName must not be empty.");

  ValueName()
    requires std::is_default_constructible_v<T>
  = default;
  ValueName(const ValueName&) = default;
  ValueName(ValueName&&) = default;
  ValueName& operator=(const ValueName&) = default;
  ValueName& operator=(ValueName&&) = default;

  template <class U>
  ValueName(const ValueName<_value_name, U>& _field) : Base(_field.get()) {}

  template <class U>
  ValueName(ValueName<_value_name, U>&& _field) noexcept(
      std::is_nothrow_constructible_v<T, U&&>)
      : Base(std::move(_field.value_)) {}

  template <class U>
  auto& operator=(const ValueName<_value_name, U>& _field) {
    this->value_ = _field.get();
    return *this;
  }

  template <class U>
  auto& operator=(ValueName<_value_name, U>&& _field) {
    this->value_ = std::move(_field.value_);
    return *this;
  }
};

}  // namespace rfl

#endif
