#ifndef RFL_INTERNAL_TRANSPARENT_WRAPPER_HPP_
#define RFL_INTERNAL_TRANSPARENT_WRAPPER_HPP_

#include <type_traits>
#include <utility>

#include "../default.hpp"

namespace rfl::internal {

/// CRTP base for transparent metadata wrappers (ImplicitValue, Hidden, Group,
/// ValueName, MultiToken, etc.) that hold a single `Type value_` and are
/// transparent to non-CLI serialization formats via `ReflectionType = Type`.
template <class Derived, class T>
struct TransparentWrapper {
  using Type = T;
  using ReflectionType = Type;
  static constexpr bool is_transparent_wrapper_ = true;

  TransparentWrapper()
    requires std::is_default_constructible_v<Type>
      : value_(Type()) {}

  TransparentWrapper(const Type& _value) : value_(_value) {}

  TransparentWrapper(Type&& _value) noexcept(
      std::is_nothrow_move_constructible_v<Type>)
      : value_(std::move(_value)) {}

  TransparentWrapper(const TransparentWrapper&) = default;
  TransparentWrapper(TransparentWrapper&&) = default;

  template <class U>
    requires std::is_convertible_v<U, Type>
  TransparentWrapper(const U& _value) : value_(_value) {}

  template <class U>
    requires std::is_convertible_v<U, Type>
  TransparentWrapper(U&& _value) noexcept(
      std::is_nothrow_constructible_v<Type, U&&>)
      : value_(std::forward<U>(_value)) {}

  template <class U = Type>
    requires std::is_default_constructible_v<U>
  TransparentWrapper(const Default&) : value_(Type()) {}

  ~TransparentWrapper() = default;

  TransparentWrapper& operator=(const TransparentWrapper&) = default;
  TransparentWrapper& operator=(TransparentWrapper&&) = default;

  const Type& get() const noexcept { return value_; }
  Type& get() noexcept { return value_; }

  Type& operator*() noexcept { return value_; }
  const Type& operator*() const noexcept { return value_; }

  Type& operator()() noexcept { return value_; }
  const Type& operator()() const noexcept { return value_; }

  Type* operator->() noexcept { return &value_; }
  const Type* operator->() const noexcept { return &value_; }

  Derived& operator=(const Type& _value) {
    value_ = _value;
    return static_cast<Derived&>(*this);
  }

  Derived& operator=(Type&& _value) noexcept(
      std::is_nothrow_move_assignable_v<Type>) {
    value_ = std::move(_value);
    return static_cast<Derived&>(*this);
  }

  template <class U>
    requires std::is_convertible_v<U, Type>
  Derived& operator=(const U& _value) {
    value_ = _value;
    return static_cast<Derived&>(*this);
  }

  template <class U = Type>
    requires std::is_default_constructible_v<U>
  Derived& operator=(const Default&) {
    value_ = Type();
    return static_cast<Derived&>(*this);
  }

  void set(const Type& _value) { value_ = _value; }
  void set(Type&& _value) { value_ = std::move(_value); }

  Type& value() noexcept { return value_; }
  const Type& value() const noexcept { return value_; }

  const Type& reflection() const { return value_; }

  Type value_;
};

template <class T>
concept TransparentWrapperType =
    !std::is_pointer_v<std::remove_cvref_t<T>> &&
    requires {
      typename std::remove_cvref_t<T>::Type;
      requires std::remove_cvref_t<T>::is_transparent_wrapper_ == true;
    };

template <class T>
constexpr bool is_transparent_wrapper_v =
    TransparentWrapperType<std::remove_cvref_t<std::remove_pointer_t<T>>>;

}  // namespace rfl::internal

#endif
