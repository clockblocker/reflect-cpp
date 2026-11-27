#ifndef RFL_TRISTATE_HPP_
#define RFL_TRISTATE_HPP_

#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>

namespace rfl {

/// Tag value representing an omitted (absent) field in `rfl::TriState<T>`.
struct OmittedTag {
  explicit OmittedTag() = default;
};

inline constexpr OmittedTag omitted{};

/// Tri-state field presence wrapper distinguishing:
///   1. `State::omitted` — key was absent from input (default-constructed;
///      omitted on serialization unless `rfl::NoOptionals` is used).
///   2. `State::null`    — key was explicitly present as `null` (serializes as
///      `"field": null`).
///   3. `State::value`   — key was explicitly present with a value of type `T`
///      (serializes as `"field": <value>`).
///
/// Works across `read`, `read_into` (preserving prior layer state when omitted,
/// overriding with `State::null` on explicit `null`, and recursively overlaying
/// nested structs when both layers hold values), `write`, and Draft 2020-12
/// JSON Schema generation.
template <class T>
class TriState {
 public:
  using value_type = T;
  using Type = T;

  enum class State : uint8_t {
    omitted = 0,
    null = 1,
    value = 2,
  };

  constexpr TriState() noexcept = default;
  constexpr TriState(OmittedTag) noexcept
      : value_(std::nullopt), state_(State::omitted) {}
  constexpr TriState(std::nullopt_t) noexcept
      : value_(std::nullopt), state_(State::null) {}
  constexpr TriState(const T& _val) : value_(_val), state_(State::value) {}
  constexpr TriState(T&& _val) noexcept(std::is_nothrow_move_constructible_v<T>)
      : value_(std::move(_val)), state_(State::value) {}

  template <class U>
    requires(!std::is_same_v<std::remove_cvref_t<U>, TriState> &&
             !std::is_same_v<std::remove_cvref_t<U>, OmittedTag> &&
             !std::is_same_v<std::remove_cvref_t<U>, std::nullopt_t> &&
             std::is_constructible_v<T, U&&>)
  constexpr TriState(U&& _val)
      : value_(std::in_place, std::forward<U>(_val)), state_(State::value) {}

  constexpr TriState(const TriState&) = default;
  constexpr TriState(TriState&&) noexcept(
      std::is_nothrow_move_constructible_v<T>) = default;
  constexpr TriState& operator=(const TriState&) = default;
  constexpr TriState& operator=(TriState&&) noexcept(
      std::is_nothrow_move_assignable_v<T> &&
      std::is_nothrow_move_constructible_v<T>) = default;

  constexpr TriState& operator=(OmittedTag) noexcept {
    value_.reset();
    state_ = State::omitted;
    return *this;
  }

  constexpr TriState& operator=(std::nullopt_t) noexcept {
    value_.reset();
    state_ = State::null;
    return *this;
  }

  template <class U = T>
    requires(!std::is_same_v<std::remove_cvref_t<U>, TriState> &&
             !std::is_same_v<std::remove_cvref_t<U>, OmittedTag> &&
             !std::is_same_v<std::remove_cvref_t<U>, std::nullopt_t> &&
             std::is_constructible_v<T, U&&> && std::is_assignable_v<T&, U&&>)
  constexpr TriState& operator=(U&& _val) {
    value_ = std::forward<U>(_val);
    state_ = State::value;
    return *this;
  }

  static constexpr TriState omitted() noexcept { return TriState(rfl::omitted); }
  static constexpr TriState null() noexcept { return TriState(std::nullopt); }

  template <class... Args>
  static constexpr TriState of(Args&&... _args) {
    TriState ts;
    ts.emplace(std::forward<Args>(_args)...);
    return ts;
  }

  constexpr State state() const noexcept { return state_; }
  constexpr bool is_omitted() const noexcept {
    return state_ == State::omitted;
  }
  constexpr bool is_present() const noexcept {
    return state_ != State::omitted;
  }
  constexpr bool is_null() const noexcept { return state_ == State::null; }
  constexpr bool has_value() const noexcept { return state_ == State::value; }
  constexpr explicit operator bool() const noexcept { return has_value(); }

  constexpr T& value() & { return value_.value(); }
  constexpr const T& value() const& { return value_.value(); }
  constexpr T&& value() && { return std::move(value_).value(); }
  constexpr const T&& value() const&& { return std::move(value_).value(); }

  template <class U>
  constexpr T value_or(U&& _default) const& {
    return has_value() ? *value_ : static_cast<T>(std::forward<U>(_default));
  }

  template <class U>
  constexpr T value_or(U&& _default) && {
    return has_value() ? std::move(*value_)
                       : static_cast<T>(std::forward<U>(_default));
  }

  constexpr T& operator*() & noexcept { return *value_; }
  constexpr const T& operator*() const& noexcept { return *value_; }
  constexpr T&& operator*() && noexcept { return std::move(*value_); }
  constexpr const T&& operator*() const&& noexcept { return std::move(*value_); }

  constexpr T* operator->() noexcept { return value_.operator->(); }
  constexpr const T* operator->() const noexcept { return value_.operator->(); }

  constexpr std::optional<T> to_optional() const& { return value_; }
  constexpr std::optional<T> to_optional() && noexcept(
      std::is_nothrow_move_constructible_v<T>) {
    return std::move(value_);
  }

  constexpr void reset() noexcept {
    value_.reset();
    state_ = State::omitted;
  }

  constexpr void set_omitted() noexcept { reset(); }

  constexpr void set_null() noexcept {
    value_.reset();
    state_ = State::null;
  }

  template <class... Args>
  constexpr T& emplace(Args&&... _args) {
    T& ref = value_.emplace(std::forward<Args>(_args)...);
    state_ = State::value;
    return ref;
  }

  constexpr bool operator==(const TriState& _other) const {
    if (state_ != _other.state_) {
      return false;
    }
    return state_ != State::value || *value_ == *_other.value_;
  }

 private:
  std::optional<T> value_{};
  State state_ = State::omitted;
};

}  // namespace rfl

#endif
