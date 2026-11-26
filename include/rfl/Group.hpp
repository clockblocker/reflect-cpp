#ifndef RFL_GROUP_HPP_
#define RFL_GROUP_HPP_

#include <type_traits>
#include <utility>

#include "default.hpp"
#include "internal/StringLiteral.hpp"

namespace rfl {

/// Assigns a CLI option to a named help section (e.g. "OUTPUT OPTIONS") in
/// `rfl::cli::usage` and `rfl::cli::read_app`.
/// For non-CLI formats (JSON, YAML, etc.), this is transparent.
template <internal::StringLiteral _group, class T>
struct Group {
  /// The underlying type.
  using Type = T;

  /// Every format reads and writes the underlying value directly.
  using ReflectionType = Type;

  /// The section header name.
  static constexpr auto group_name_ = _group;

  static_assert(_group.length > 0, "Group name must not be empty.");

  Group()
    requires std::is_default_constructible_v<Type>
      : value_(Type()) {}

  Group(const Type& _value) : value_(_value) {}

  Group(Type&& _value) noexcept(std::is_nothrow_move_constructible_v<Type>)
      : value_(std::move(_value)) {}

  Group(Group<_group, T>&& _field) = default;

  Group(const Group<_group, T>& _field) = default;

  template <class U>
  Group(const Group<_group, U>& _field) : value_(_field.get()) {}

  template <class U>
  Group(Group<_group, U>&& _field) noexcept(
      std::is_nothrow_constructible_v<Type, U&&>)
      : value_(std::move(_field.value_)) {}

  template <class U>
    requires std::is_convertible_v<U, Type>
  Group(const U& _value) : value_(_value) {}

  template <class U>
    requires std::is_convertible_v<U, Type>
  Group(U&& _value) noexcept(std::is_nothrow_constructible_v<Type, U&&>)
      : value_(std::forward<U>(_value)) {}

  template <class U = Type>
    requires std::is_default_constructible_v<U>
  Group(const Default&) : value_(Type()) {}

  ~Group() = default;

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

  Group<_group, T>& operator=(const Group<_group, T>& _field) = default;

  Group<_group, T>& operator=(Group<_group, T>&& _field) = default;

  template <class U>
  auto& operator=(const Group<_group, U>& _field) {
    value_ = _field.get();
    return *this;
  }

  template <class U>
  auto& operator=(Group<_group, U>&& _field) {
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
