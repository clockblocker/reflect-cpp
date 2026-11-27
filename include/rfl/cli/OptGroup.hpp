#ifndef RFL_CLI_OPTGROUP_HPP_
#define RFL_CLI_OPTGROUP_HPP_

#include <string_view>
#include <type_traits>

#include "../Flatten.hpp"
#include "../internal/StringLiteral.hpp"
#include "../internal/is_flatten_field.hpp"

namespace rfl::cli {

/// Embeds a sub-struct's CLI options flat into the parent struct (just like
/// `rfl::Flatten<T>`) while automatically assigning every ungrouped option in
/// `T` to help section `_group`.
///
/// Individual fields inside `T` may still specify their own `attr::Group<...>`
/// or `rfl::Group<...>` to override the inherited group.
///
/// Example:
/// ```cpp
/// struct OutputArgs {
///   rfl::cli::Opt<std::optional<std::string>, rfl::cli::attr::Short<"o">> out;
///   rfl::cli::Flag<rfl::cli::attr::Short<"f">> force = false;
/// };
/// struct MyCli {
///   rfl::cli::OptGroup<"Output Options", OutputArgs> output;
/// };
/// ```
template <rfl::internal::StringLiteral _group, class T>
struct OptGroup : rfl::Flatten<T> {
  using Base = rfl::Flatten<T>;
  using Type = std::remove_cvref_t<T>;
  static constexpr auto group_name_ = _group;
  static constexpr bool is_opt_group_ = true;

  using Base::Base;
  using Base::operator=;

  OptGroup() = default;
  OptGroup(const OptGroup&) = default;
  OptGroup(OptGroup&&) noexcept = default;
  OptGroup& operator=(const OptGroup&) = default;
  OptGroup& operator=(OptGroup&&) noexcept = default;
};

namespace internal {

template <class T>
struct is_opt_group : std::false_type {};

template <rfl::internal::StringLiteral _group, class T>
struct is_opt_group<OptGroup<_group, T>> : std::true_type {};

template <class T>
constexpr bool is_opt_group_v =
    is_opt_group<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace internal

}  // namespace rfl::cli

namespace rfl::internal {

template <StringLiteral _group, class T>
class is_flatten_field<rfl::cli::OptGroup<_group, T>> : public std::true_type {};

}  // namespace rfl::internal

#endif
