#ifndef RFL_HIDDEN_HPP_
#define RFL_HIDDEN_HPP_

#include <type_traits>
#include <utility>

#include "default.hpp"

namespace rfl {

/// Marks a CLI option or positional argument as hidden from generated usage
/// and help text (`rfl::cli::usage` and `rfl::cli::read_app`), while still
/// parsing it normally.
/// For non-CLI formats (JSON, YAML, etc.), this is transparent.
template <class T>
struct Hidden {
  /// The underlying type.
  using Type = T;

  /// Every format reads and writes the underlying value directly.
  using ReflectionType = Type;

  Hidden()
    requires std::is_default_constructible_v<Type>
      : value_(Type()) {}

  Hidden(const Type& _value) : value_(_value) {}

  Hidden(Type&& _value) noexcept(std::is_nothrow_move_constructible_v<Type>)
      : value_(std::move(_value)) {}

  Hidden(Hidden<T>&& _field) = default;

  Hidden(const Hidden<T>& _field) = default;

  template <class U>
  Hidden(const Hidden<U>& _field) : value_(_field.get()) {}

  template <class U>
  Hidden(Hidden<U>&& _field) noexcept(std::is_nothrow_constructible_v<Type, U&&>)
      : value_(std::move(_field.value_)) {}

  template <class U>
    requires std::is_convertible_v<U, Type>
  Hidden(const U& _value) : value_(_value) {}

  template <class U>
    requires std::is_convertible_v<U, Type>
  Hidden(U&& _value) noexcept(std::is_nothrow_constructible_v<Type, U&&>)
      : value_(std::forward<U>(_value)) {}

  template <class U = Type>
    requires std::is_default_constructible_v<U>
  Hidden(const Default&) : value_(Type()) {}

  ~Hidden() = default;

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

  Hidden<T>& operator=(const Hidden<T>& _field) = default;

  Hidden<T>& operator=(Hidden<T>&& _field) = default;

  template <class U>
  auto& operator=(const Hidden<U>& _field) {
    value_ = _field.get();
    return *this;
  }

  template <class U>
  auto& operator=(Hidden<U>&& _field) {
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
