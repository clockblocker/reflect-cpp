#ifndef RFL_NULLABLE_HPP_
#define RFL_NULLABLE_HPP_

#include <optional>
#include <type_traits>
#include <utility>

namespace rfl {

/// A nullable field wrapper that ALWAYS serializes explicitly (`"field": null`
/// when empty, `"field": <value>` when present), unlike `std::optional<T>`
/// which omits empty keys by default.
///
/// On deserialization (`read` and `read_into`), `rfl::Nullable<T>` accepts an
/// omitted key (becoming null), an explicit `null` (becoming null), or a value
/// of type `T`. In JSON Schema (Draft 2020-12), it emits a nullable schema
/// (`anyOf: [<T>, {"type": "null"}]`) and is not marked required unless
/// `rfl::NoOptionals` is used.
template <class T>
class Nullable {
 public:
  using value_type = T;
  using Type = T;

  constexpr Nullable() noexcept = default;
  constexpr Nullable(std::nullopt_t) noexcept : value_(std::nullopt) {}
  constexpr Nullable(const T& _val) : value_(_val) {}
  constexpr Nullable(T&& _val) noexcept(std::is_nothrow_move_constructible_v<T>)
      : value_(std::move(_val)) {}
  constexpr Nullable(const std::optional<T>& _opt) : value_(_opt) {}
  constexpr Nullable(std::optional<T>&& _opt) noexcept(
      std::is_nothrow_move_constructible_v<T>)
      : value_(std::move(_opt)) {}

  template <class U>
    requires(!std::is_same_v<std::remove_cvref_t<U>, Nullable> &&
             !std::is_same_v<std::remove_cvref_t<U>, std::optional<T>> &&
             !std::is_same_v<std::remove_cvref_t<U>, std::nullopt_t> &&
             std::is_constructible_v<T, U&&>)
  constexpr Nullable(U&& _val) : value_(std::in_place, std::forward<U>(_val)) {}

  constexpr Nullable(const Nullable&) = default;
  constexpr Nullable(Nullable&&) noexcept(
      std::is_nothrow_move_constructible_v<T>) = default;
  constexpr Nullable& operator=(const Nullable&) = default;
  constexpr Nullable& operator=(Nullable&&) noexcept(
      std::is_nothrow_move_assignable_v<T> &&
      std::is_nothrow_move_constructible_v<T>) = default;

  constexpr Nullable& operator=(std::nullopt_t) noexcept {
    value_.reset();
    return *this;
  }

  constexpr Nullable& operator=(const std::optional<T>& _opt) {
    value_ = _opt;
    return *this;
  }

  constexpr Nullable& operator=(std::optional<T>&& _opt) noexcept(
      std::is_nothrow_move_assignable_v<T> &&
      std::is_nothrow_move_constructible_v<T>) {
    value_ = std::move(_opt);
    return *this;
  }

  template <class U = T>
    requires(!std::is_same_v<std::remove_cvref_t<U>, Nullable> &&
             !std::is_same_v<std::remove_cvref_t<U>, std::optional<T>> &&
             !std::is_same_v<std::remove_cvref_t<U>, std::nullopt_t> &&
             std::is_constructible_v<T, U&&> && std::is_assignable_v<T&, U&&>)
  constexpr Nullable& operator=(U&& _val) {
    value_ = std::forward<U>(_val);
    return *this;
  }

  constexpr bool has_value() const noexcept { return value_.has_value(); }
  constexpr bool is_null() const noexcept { return !value_.has_value(); }
  constexpr explicit operator bool() const noexcept {
    return value_.has_value();
  }

  constexpr T& value() & { return value_.value(); }
  constexpr const T& value() const& { return value_.value(); }
  constexpr T&& value() && { return std::move(value_).value(); }
  constexpr const T&& value() const&& { return std::move(value_).value(); }

  template <class U>
  constexpr T value_or(U&& _default) const& {
    return value_.value_or(std::forward<U>(_default));
  }

  template <class U>
  constexpr T value_or(U&& _default) && {
    return std::move(value_).value_or(std::forward<U>(_default));
  }

  constexpr T& operator*() & noexcept { return *value_; }
  constexpr const T& operator*() const& noexcept { return *value_; }
  constexpr T&& operator*() && noexcept { return std::move(*value_); }
  constexpr const T&& operator*() const&& noexcept { return std::move(*value_); }

  constexpr T* operator->() noexcept { return value_.operator->(); }
  constexpr const T* operator->() const noexcept { return value_.operator->(); }

  constexpr std::optional<T>& optional() & noexcept { return value_; }
  constexpr const std::optional<T>& optional() const& noexcept {
    return value_;
  }
  constexpr std::optional<T>&& optional() && noexcept {
    return std::move(value_);
  }

  constexpr void reset() noexcept { value_.reset(); }

  template <class... Args>
  constexpr T& emplace(Args&&... _args) {
    return value_.emplace(std::forward<Args>(_args)...);
  }

  constexpr bool operator==(const Nullable& _other) const = default;

  constexpr bool operator==(std::nullopt_t) const noexcept {
    return !value_.has_value();
  }

  constexpr bool operator==(const std::optional<T>& _other) const {
    return value_ == _other;
  }

  constexpr bool operator==(const T& _other) const {
    return value_.has_value() && *value_ == _other;
  }

 private:
  std::optional<T> value_{};
};

}  // namespace rfl

#endif
