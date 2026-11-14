#ifndef RFL_IMPLICITVALUE_HPP_
#define RFL_IMPLICITVALUE_HPP_

#include <type_traits>
#include <utility>

#include "default.hpp"
#include "internal/StringLiteral.hpp"

namespace rfl {

/// Lets a CLI option be given without a value, like Boost.Program_options'
/// implicit_value: `--help` stands for `--help=<_implicit>`, and `--help=x`
/// sets `x`. `--help x` never takes `x` as the value, so `x` stays a
/// positional argument. If the option is absent, the field is read as usual
/// (std::optional gives std::nullopt, rfl::DefaultVal gives its default).
/// For non-CLI formats (JSON, YAML, etc.), this is transparent.
/// @tparam T The type of the field
/// @tparam _implicit The CLI text that a bare option stands for. It is parsed
/// like any other value, so `rfl::ImplicitValue<int, "1">` is 1.
template <class T, internal::StringLiteral _implicit = "">
struct ImplicitValue {
  /// The underlying type.
  using Type = T;

  /// The value that a bare option stands for.
  static constexpr auto implicit_value_ = _implicit;

  /// @brief Default constructor.
  /// @details Constructs the underlying value using its default constructor.
  /// @requires Type must be default constructible.
  ImplicitValue()
    requires std::is_default_constructible_v<Type>
      : value_(Type()) {}

  /// @brief Constructs the ImplicitValue wrapper from a const reference to
  /// the underlying value.
  /// @param _value The value to assign to the underlying field.
  ImplicitValue(const Type& _value) : value_(_value) {}

  /// @brief Constructs the ImplicitValue wrapper by moving the underlying
  /// value.
  /// @param _value The value to move into the underlying field.
  ImplicitValue(Type&& _value) noexcept(
      std::is_nothrow_move_constructible_v<Type>)
      : value_(std::move(_value)) {}

  /// @brief Move constructor.
  /// @param _field The ImplicitValue field to move from.
  ImplicitValue(ImplicitValue<T, _implicit>&& _field) = default;

  /// @brief Copy constructor.
  /// @param _field The ImplicitValue field to copy from.
  ImplicitValue(const ImplicitValue<T, _implicit>& _field) = default;

  /// @brief Constructs from another ImplicitValue with a convertible
  /// underlying type.
  /// @tparam U The type of the other ImplicitValue's underlying value.
  /// @param _field The other ImplicitValue field to copy from.
  template <class U>
  ImplicitValue(const ImplicitValue<U, _implicit>& _field)
      : value_(_field.get()) {}

  /// @brief Move constructs from another ImplicitValue with a convertible
  /// underlying type.
  /// @tparam U The type of the other ImplicitValue's underlying value.
  /// @param _field The other ImplicitValue field to move from.
  template <class U>
  ImplicitValue(ImplicitValue<U, _implicit>&& _field) noexcept(
      std::is_nothrow_constructible_v<Type, U&&>)
      : value_(std::move(_field.value_)) {}

  /// @brief Constructs from a value convertible to the underlying type.
  /// @tparam U The type of the value.
  /// @param _value The value to assign.
  template <class U>
    requires std::is_convertible_v<U, Type>
  ImplicitValue(const U& _value) : value_(_value) {}

  /// @brief Move constructs from a value convertible to the underlying type.
  /// @tparam U The type of the value.
  /// @param _value The value to move.
  template <class U>
    requires std::is_convertible_v<U, Type>
  ImplicitValue(U&& _value) noexcept(
      std::is_nothrow_constructible_v<Type, U&&>)
      : value_(std::forward<U>(_value)) {}

  /// @brief Constructs the underlying object to its default value using
  /// Default tag.
  /// @tparam U The type to default construct (defaults to Type).
  /// @param unused Default tag.
  template <class U = Type>
    requires std::is_default_constructible_v<U>
  ImplicitValue(const Default&) : value_(Type()) {}

  /// @brief Destructor.
  /// @details Defaulted destructor for ImplicitValue.
  ~ImplicitValue() = default;

  /// @brief Returns a const reference to the underlying object.
  /// @return Const reference to the underlying value.
  const Type& get() const noexcept { return value_; }

  /// @brief Returns a reference to the underlying object.
  /// @return Reference to the underlying value.
  Type& get() noexcept { return value_; }

  /// @brief Dereference operator.
  /// @return Reference to the underlying value.
  Type& operator*() noexcept { return value_; }

  /// @brief Dereference operator (const).
  /// @return Const reference to the underlying value.
  const Type& operator*() const noexcept { return value_; }

  /// @brief Function call operator.
  /// @return Reference to the underlying value.
  Type& operator()() noexcept { return value_; }

  /// @brief Function call operator (const).
  /// @return Const reference to the underlying value.
  const Type& operator()() const noexcept { return value_; }

  /// @brief Pointer to the underlying value.
  /// @return Pointer to the underlying value.
  Type* operator->() noexcept { return &value_; }

  /// @brief Pointer to the underlying value (const).
  /// @return Const pointer to the underlying value.
  const Type* operator->() const noexcept { return &value_; }

  /// @brief Assigns a new value to the underlying object.
  /// @param _value The value to assign.
  /// @return Reference to this ImplicitValue.
  auto& operator=(const Type& _value) {
    value_ = _value;
    return *this;
  }

  /// @brief Move-assigns a new value to the underlying object.
  /// @param _value The value to move.
  /// @return Reference to this ImplicitValue.
  auto& operator=(Type&& _value) noexcept(
      std::is_nothrow_move_assignable_v<Type>) {
    value_ = std::move(_value);
    return *this;
  }

  /// @brief Assigns a value convertible to the underlying type.
  /// @tparam U The type of the value.
  /// @param _value The value to assign.
  /// @return Reference to this ImplicitValue.
  template <class U>
    requires std::is_convertible_v<U, Type>
  auto& operator=(const U& _value) {
    value_ = _value;
    return *this;
  }

  /// @brief Assigns the underlying object to its default value using Default
  /// tag.
  /// @tparam U The type to default construct (defaults to Type).
  /// @param unused Default tag.
  /// @return Reference to this ImplicitValue.
  template <class U = Type>
    requires std::is_default_constructible_v<U>
  auto& operator=(const Default&) {
    value_ = Type();
    return *this;
  }

  /// @brief Copy assignment operator.
  /// @param _field The ImplicitValue field to copy from.
  /// @return Reference to this ImplicitValue.
  ImplicitValue<T, _implicit>& operator=(
      const ImplicitValue<T, _implicit>& _field) = default;

  /// @brief Move assignment operator.
  /// @param _field The ImplicitValue field to move from.
  /// @return Reference to this ImplicitValue.
  ImplicitValue<T, _implicit>& operator=(
      ImplicitValue<T, _implicit>&& _field) = default;

  /// @brief Assigns from another ImplicitValue with a convertible underlying
  /// type.
  /// @tparam U The type of the other ImplicitValue's underlying value.
  /// @param _field The other ImplicitValue field to copy from.
  /// @return Reference to this ImplicitValue.
  template <class U>
  auto& operator=(const ImplicitValue<U, _implicit>& _field) {
    value_ = _field.get();
    return *this;
  }

  /// @brief Move-assigns from another ImplicitValue with a convertible
  /// underlying type.
  /// @tparam U The type of the other ImplicitValue's underlying value.
  /// @param _field The other ImplicitValue field to move from.
  /// @return Reference to this ImplicitValue.
  template <class U>
  auto& operator=(ImplicitValue<U, _implicit>&& _field) {
    value_ = std::move(_field.value_);
    return *this;
  }

  /// @brief Sets the underlying object to a new value.
  /// @param _value The value to assign.
  void set(const Type& _value) { value_ = _value; }

  /// @brief Sets the underlying object to a new value by moving.
  /// @param _value The value to move.
  void set(Type&& _value) { value_ = std::move(_value); }

  /// @brief Returns a reference to the underlying value.
  /// @return Reference to the underlying value.
  Type& value() noexcept { return value_; }

  /// @brief Returns a const reference to the underlying value.
  /// @return Const reference to the underlying value.
  const Type& value() const noexcept { return value_; }

  /// The underlying value.
  Type value_;
};

}  // namespace rfl

#endif
