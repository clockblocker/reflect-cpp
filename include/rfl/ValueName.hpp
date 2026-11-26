#ifndef RFL_VALUENAME_HPP_
#define RFL_VALUENAME_HPP_

#include <type_traits>
#include <utility>

#include "default.hpp"
#include "internal/StringLiteral.hpp"

namespace rfl {

/// Sets a custom value placeholder name (metavar, e.g. `"path"` or `"<NUM>"`)
/// for a CLI option in `rfl::cli::usage` and `rfl::cli::read_app`.
/// For non-CLI formats (JSON, YAML, etc.), this is transparent.
template <internal::StringLiteral _value_name, class T>
struct ValueName {
  /// The underlying type.
  using Type = T;

  /// Every format reads and writes the underlying value directly.
  using ReflectionType = Type;

  /// The value placeholder name.
  static constexpr auto value_name_ = _value_name;

  static_assert(_value_name.length > 0, "ValueName must not be empty.");

  ValueName()
    requires std::is_default_constructible_v<Type>
      : value_(Type()) {}

  ValueName(const Type& _value) : value_(_value) {}

  ValueName(Type&& _value) noexcept(std::is_nothrow_move_constructible_v<Type>)
      : value_(std::move(_value)) {}

  ValueName(ValueName<_value_name, T>&& _field) = default;

  ValueName(const ValueName<_value_name, T>& _field) = default;

  template <class U>
  ValueName(const ValueName<_value_name, U>& _field) : value_(_field.get()) {}

  template <class U>
  ValueName(ValueName<_value_name, U>&& _field) noexcept(
      std::is_nothrow_constructible_v<Type, U&&>)
      : value_(std::move(_field.value_)) {}

  template <class U>
    requires std::is_convertible_v<U, Type>
  ValueName(const U& _value) : value_(_value) {}

  template <class U>
    requires std::is_convertible_v<U, Type>
  ValueName(U&& _value) noexcept(std::is_nothrow_constructible_v<Type, U&&>)
      : value_(std::forward<U>(_value)) {}

  template <class U = Type>
    requires std::is_default_constructible_v<U>
  ValueName(const Default&) : value_(Type()) {}

  ~ValueName() = default;

  const Type& get() const noexcept { return value_; }

  Type& get() noexcept { return value_; }

  Type& operator*() noexcept { return value_; }

  const Type& operator*() const noexcept { return value_; }

  Type& operator()() noexcept { return value_; }

  const Type& operator()() const noexcept { return value_; }

  Type* operator->() noexcept { return &value_; }

  const Type* operator->() const noexcept { return &value_; }

  auto& operator=(const Type& _value) {
    value_ = _value;
    return *this;
  }

  auto& operator=(Type&& _value) noexcept(
      std::is_nothrow_move_assignable_v<Type>) {
    value_ = std::move(_value);
    return *this;
  }

  template <class U>
    requires std::is_convertible_v<U, Type>
  auto& operator=(const U& _value) {
    value_ = _value;
    return *this;
  }

  template <class U = Type>
    requires std::is_default_constructible_v<U>
  auto& operator=(const Default&) {
    value_ = Type();
    return *this;
  }

  ValueName<_value_name, T>& operator=(
      const ValueName<_value_name, T>& _field) = default;

  ValueName<_value_name, T>& operator=(ValueName<_value_name, T>&& _field) =
      default;

  template <class U>
  auto& operator=(const ValueName<_value_name, U>& _field) {
    value_ = _field.get();
    return *this;
  }

  template <class U>
  auto& operator=(ValueName<_value_name, U>&& _field) {
    value_ = std::move(_field.value_);
    return *this;
  }

  void set(const Type& _value) { value_ = _value; }

  void set(Type&& _value) { value_ = std::move(_value); }

  Type& value() noexcept { return value_; }

  const Type& value() const noexcept { return value_; }

  const Type& reflection() const { return value_; }

  Type value_;
};

}  // namespace rfl

#endif
