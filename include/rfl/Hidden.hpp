#ifndef RFL_HIDDEN_HPP_
#define RFL_HIDDEN_HPP_

#include "internal/TransparentWrapper.hpp"

namespace rfl {

/// Marks a CLI option or positional argument as hidden from generated usage
/// and help text (`rfl::cli::usage` and `rfl::cli::read_app`), while still
/// parsing it normally.
/// For non-CLI formats (JSON, YAML, etc.), this is transparent.
template <class T>
struct Hidden : internal::TransparentWrapper<Hidden<T>, T> {
  using Base = internal::TransparentWrapper<Hidden<T>, T>;
  using Base::Base;
  using Base::operator=;

  Hidden()
    requires std::is_default_constructible_v<T>
  = default;
  Hidden(const Hidden&) = default;
  Hidden(Hidden&&) = default;
  Hidden& operator=(const Hidden&) = default;
  Hidden& operator=(Hidden&&) = default;

  template <class U>
  Hidden(const Hidden<U>& _field) : Base(_field.get()) {}

  template <class U>
  Hidden(Hidden<U>&& _field) noexcept(std::is_nothrow_constructible_v<T, U&&>)
      : Base(std::move(_field.value_)) {}

  template <class U>
  auto& operator=(const Hidden<U>& _field) {
    this->value_ = _field.get();
    return *this;
  }

  template <class U>
  auto& operator=(Hidden<U>&& _field) {
    this->value_ = std::move(_field.value_);
    return *this;
  }
};

}  // namespace rfl

#endif
