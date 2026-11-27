#ifndef RFL_ALIAS_HPP_
#define RFL_ALIAS_HPP_

#include <compare>
#include <cstddef>
#include <functional>
#include <string_view>
#include <type_traits>
#include <utility>

#include "default.hpp"
#include "internal/StringLiteral.hpp"
#include "internal/is_alias.hpp"

namespace rfl {

/// Declares a read-only field alias accepted during deserialization while
/// always serializing (and generating schemas) under the canonical primary
/// field name.
///
/// Multiple aliases can be declared by nesting `rfl::Alias`:
/// `rfl::Alias<"download-dir", rfl::Alias<"downloadDir", std::string>>`
///
/// @tparam _name The compile-time string literal alias name accepted on read
/// @tparam T The underlying value (or wrapped field) type
template <internal::StringLiteral _name, class T>
struct Alias {
  /// The underlying type.
  using Type = T;

  /// The alias name as a compile-time string literal.
  static constexpr internal::StringLiteral alias_name_ = _name;

  /// Returns the alias name as a string_view.
  static constexpr std::string_view alias_name() noexcept {
    return alias_name_.string_view();
  }

  /// Default constructor - enabled only when Type is default-constructible.
  template <class U = Type>
    requires std::is_default_constructible_v<U>
  constexpr Alias() noexcept(std::is_nothrow_default_constructible_v<U>)
      : value_() {}

  /// Constructs from a const reference to the value.
  constexpr Alias(const Type& _value) : value_(_value) {}

  /// Constructs from an rvalue reference to the value.
  constexpr Alias(Type&& _value) noexcept(
      std::is_nothrow_move_constructible_v<Type>)
      : value_(std::move(_value)) {}

  /// Move constructor.
  Alias(Alias&& _field) = default;

  /// Copy constructor.
  Alias(const Alias& _field) = default;

  /// Copy constructor from Alias with the same alias name and a compatible
  /// underlying type.
  template <class U>
    requires(!std::is_same_v<U, Type> && std::is_convertible_v<const U&, Type>)
  constexpr Alias(const Alias<_name, U>& _field) : value_(_field.get()) {}

  /// Move constructor from Alias with the same alias name and a compatible
  /// underlying type.
  template <class U>
    requires(!std::is_same_v<U, Type> && std::is_convertible_v<U&&, Type>)
  constexpr Alias(Alias<_name, U>&& _field) noexcept(
      std::is_nothrow_constructible_v<Type, U&&>)
      : value_(std::move(_field.value_)) {}

  /// Constructs from any type convertible to Type.
  template <class U>
    requires(!std::is_same_v<std::remove_cvref_t<U>, Alias> &&
             !std::is_same_v<std::remove_cvref_t<U>, Type> &&
             !std::is_same_v<std::remove_cvref_t<U>, Default> &&
             std::is_convertible_v<U, Type>)
  constexpr Alias(U&& _value) noexcept(
      std::is_nothrow_constructible_v<Type, U>)
      : value_(std::forward<U>(_value)) {}

  /// Initializes the underlying object to its default value using the Default
  /// sentinel.
  template <class U = Type>
    requires std::is_default_constructible_v<U>
  constexpr Alias(const Default&) noexcept(
      std::is_nothrow_default_constructible_v<U>)
      : value_() {}

  /// Destructor.
  ~Alias() = default;

  /// Returns a const reference to the underlying value.
  constexpr const Type& get() const noexcept { return value_; }

  /// Returns a reference to the underlying value.
  constexpr Type& get() noexcept { return value_; }

  /// Dereference operator - returns the underlying value.
  constexpr Type& operator*() noexcept { return value_; }

  /// Dereference operator (const) - returns the underlying value.
  constexpr const Type& operator*() const noexcept { return value_; }

  /// Function call operator - returns the underlying value.
  constexpr Type& operator()() noexcept { return value_; }

  /// Function call operator (const) - returns the underlying value.
  constexpr const Type& operator()() const noexcept { return value_; }

  /// Pointer to the underlying value.
  constexpr Type* operator->() noexcept { return &value_; }

  /// Pointer to the underlying value (const).
  constexpr const Type* operator->() const noexcept { return &value_; }

  /// Implicit conversion to a mutable reference to the underlying value.
  constexpr operator Type&() noexcept { return value_; }

  /// Implicit conversion to a const reference to the underlying value.
  constexpr operator const Type&() const noexcept { return value_; }

  /// Assigns a new value (copy).
  auto& operator=(const Type& _value) {
    value_ = _value;
    return *this;
  }

  /// Assigns a new value (move).
  auto& operator=(Type&& _value) noexcept(
      std::is_nothrow_move_assignable_v<Type>) {
    value_ = std::move(_value);
    return *this;
  }

  /// Assigns from any type convertible to the underlying type.
  template <class U>
    requires(!std::is_same_v<std::remove_cvref_t<U>, Alias> &&
             !std::is_same_v<std::remove_cvref_t<U>, Type> &&
             !std::is_same_v<std::remove_cvref_t<U>, Default> &&
             std::is_convertible_v<U, Type>)
  auto& operator=(U&& _value) {
    value_ = std::forward<U>(_value);
    return *this;
  }

  /// Assigns the value to its default using the Default sentinel.
  template <class U = Type>
    requires std::is_default_constructible_v<U>
  auto& operator=(const Default&) {
    value_ = Type();
    return *this;
  }

  /// Copy assignment operator.
  Alias& operator=(const Alias& _field) = default;

  /// Move assignment operator.
  Alias& operator=(Alias&& _field) = default;

  /// Assigns from another Alias with compatible underlying type (copy).
  template <class U>
    requires(!std::is_same_v<U, Type> && std::is_convertible_v<const U&, Type>)
  auto& operator=(const Alias<_name, U>& _field) {
    value_ = _field.get();
    return *this;
  }

  /// Assigns from another Alias with compatible underlying type (move).
  template <class U>
    requires(!std::is_same_v<U, Type> && std::is_convertible_v<U&&, Type>)
  auto& operator=(Alias<_name, U>&& _field) {
    value_ = std::move(_field.value_);
    return *this;
  }

  /// Sets the value (copy).
  void set(const Type& _value) { value_ = _value; }

  /// Sets the value (move).
  void set(Type&& _value) noexcept(std::is_nothrow_move_assignable_v<Type>) {
    value_ = std::move(_value);
  }

  /// Returns a reference to the underlying value.
  constexpr Type& value() noexcept { return value_; }

  /// Returns a const reference to the underlying value.
  constexpr const Type& value() const noexcept { return value_; }

  /// The underlying value.
  Type value_;
};

/// Equality comparison between two Alias wrappers.
template <internal::StringLiteral _n1, class T1, internal::StringLiteral _n2,
          class T2>
  requires requires(const T1& _a, const T2& _b) { _a == _b; }
constexpr bool operator==(const Alias<_n1, T1>& _lhs,
                          const Alias<_n2, T2>& _rhs) {
  return _lhs.value() == _rhs.value();
}

/// Three-way comparison between two Alias wrappers.
template <internal::StringLiteral _n1, class T1, internal::StringLiteral _n2,
          class T2>
  requires requires(const T1& _a, const T2& _b) { _a <=> _b; }
constexpr auto operator<=>(const Alias<_n1, T1>& _lhs,
                           const Alias<_n2, T2>& _rhs) {
  return _lhs.value() <=> _rhs.value();
}

/// Equality comparison between an Alias wrapper and a non-Alias value.
template <internal::StringLiteral _n, class T, class U>
  requires(!internal::is_alias_v<U> &&
           requires(const T& _a, const U& _b) { _a == _b; })
constexpr bool operator==(const Alias<_n, T>& _lhs, const U& _rhs) {
  return _lhs.value() == _rhs;
}

/// Three-way comparison between an Alias wrapper and a non-Alias value.
template <internal::StringLiteral _n, class T, class U>
  requires(!internal::is_alias_v<U> &&
           requires(const T& _a, const U& _b) { _a <=> _b; })
constexpr auto operator<=>(const Alias<_n, T>& _lhs, const U& _rhs) {
  return _lhs.value() <=> _rhs;
}

}  // namespace rfl

namespace std {

/// Specialization of std::hash for rfl::Alias.
template <rfl::internal::StringLiteral _name, class T>
  requires requires(const T& _v) { std::hash<T>{}(_v); }
struct hash<rfl::Alias<_name, T>> {
  size_t operator()(const rfl::Alias<_name, T>& _a) const {
    return hash<T>{}(_a.value());
  }
};

}  // namespace std

#endif
