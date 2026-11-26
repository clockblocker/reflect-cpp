#ifndef RFL_MULTITOKEN_HPP_
#define RFL_MULTITOKEN_HPP_

#include <type_traits>
#include <utility>

#include "default.hpp"

namespace rfl {

/// Marks a CLI list option (such as `std::vector<std::string>`) as accepting
/// multiple space-separated tokens after a single option occurrence (like
/// Boost.Program_options's `multitoken()` or argparse's `nargs='+'`), e.g.:
///   `--population a b c --rounds 5`
/// Consumes subsequent tokens until the next option or `--`.
/// For non-CLI formats (JSON, YAML, etc.), this is transparent.
template <class T>
struct MultiToken {
  /// The underlying type.
  using Type = T;

  /// Every format reads and writes the underlying value directly.
  using ReflectionType = Type;

  MultiToken()
    requires std::is_default_constructible_v<Type>
      : value_(Type()) {}

  MultiToken(const Type& _value) : value_(_value) {}

  MultiToken(Type&& _value) noexcept(
      std::is_nothrow_move_constructible_v<Type>)
      : value_(std::move(_value)) {}

  MultiToken(MultiToken<T>&& _field) = default;

  MultiToken(const MultiToken<T>& _field) = default;

  template <class U>
  MultiToken(const MultiToken<U>& _field) : value_(_field.get()) {}

  template <class U>
  MultiToken(MultiToken<U>&& _field) noexcept(
      std::is_nothrow_constructible_v<Type, U&&>)
      : value_(std::move(_field.value_)) {}

  template <class U>
    requires std::is_convertible_v<U, Type>
  MultiToken(const U& _value) : value_(_value) {}

  template <class U>
    requires std::is_convertible_v<U, Type>
  MultiToken(U&& _value) noexcept(std::is_nothrow_constructible_v<Type, U&&>)
      : value_(std::forward<U>(_value)) {}

  template <class U = Type>
    requires std::is_default_constructible_v<U>
  MultiToken(const Default&) : value_(Type()) {}

  ~MultiToken() = default;

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

  MultiToken<T>& operator=(const MultiToken<T>& _field) = default;

  MultiToken<T>& operator=(MultiToken<T>&& _field) = default;

  template <class U>
  auto& operator=(const MultiToken<U>& _field) {
    value_ = _field.get();
    return *this;
  }

  template <class U>
  auto& operator=(MultiToken<U>&& _field) {
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
