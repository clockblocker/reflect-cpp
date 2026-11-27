#ifndef RFL_MULTITOKEN_HPP_
#define RFL_MULTITOKEN_HPP_

#include "internal/TransparentWrapper.hpp"

namespace rfl {

/// Marks a CLI list option (such as `std::vector<std::string>`) as accepting
/// multiple space-separated tokens after a single option occurrence (like
/// Boost.Program_options's `multitoken()` or argparse's `nargs='+'`), e.g.:
///   `--population a b c --rounds 5`
/// Consumes subsequent tokens until the next option or `--`.
/// For non-CLI formats (JSON, YAML, etc.), this is transparent.
template <class T>
struct MultiToken : internal::TransparentWrapper<MultiToken<T>, T> {
  using Base = internal::TransparentWrapper<MultiToken<T>, T>;
  using Base::Base;
  using Base::operator=;

  MultiToken()
    requires std::is_default_constructible_v<T>
  = default;
  MultiToken(const MultiToken&) = default;
  MultiToken(MultiToken&&) = default;
  MultiToken& operator=(const MultiToken&) = default;
  MultiToken& operator=(MultiToken&&) = default;

  template <class U>
  MultiToken(const MultiToken<U>& _field) : Base(_field.get()) {}

  template <class U>
  MultiToken(MultiToken<U>&& _field) noexcept(
      std::is_nothrow_constructible_v<T, U&&>)
      : Base(std::move(_field.value_)) {}

  template <class U>
  auto& operator=(const MultiToken<U>& _field) {
    this->value_ = _field.get();
    return *this;
  }

  template <class U>
  auto& operator=(MultiToken<U>&& _field) {
    this->value_ = std::move(_field.value_);
    return *this;
  }
};

}  // namespace rfl

#endif
