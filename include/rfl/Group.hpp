#ifndef RFL_GROUP_HPP_
#define RFL_GROUP_HPP_

#include "internal/StringLiteral.hpp"
#include "internal/TransparentWrapper.hpp"

namespace rfl {

/// Assigns a CLI option to a named help section (e.g. "OUTPUT OPTIONS") in
/// `rfl::cli::usage` and `rfl::cli::read_app`.
/// For non-CLI formats (JSON, YAML, etc.), this is transparent.
template <internal::StringLiteral _group, class T>
struct Group : internal::TransparentWrapper<Group<_group, T>, T> {
  using Base = internal::TransparentWrapper<Group<_group, T>, T>;
  using Base::Base;
  using Base::operator=;

  static constexpr auto group_name_ = _group;
  static_assert(_group.length > 0, "Group name must not be empty.");

  Group()
    requires std::is_default_constructible_v<T>
  = default;
  Group(const Group&) = default;
  Group(Group&&) = default;
  Group& operator=(const Group&) = default;
  Group& operator=(Group&&) = default;

  template <class U>
  Group(const Group<_group, U>& _field) : Base(_field.get()) {}

  template <class U>
  Group(Group<_group, U>&& _field) noexcept(
      std::is_nothrow_constructible_v<T, U&&>)
      : Base(std::move(_field.value_)) {}

  template <class U>
  auto& operator=(const Group<_group, U>& _field) {
    this->value_ = _field.get();
    return *this;
  }

  template <class U>
  auto& operator=(Group<_group, U>&& _field) {
    this->value_ = std::move(_field.value_);
    return *this;
  }
};

}  // namespace rfl

#endif
