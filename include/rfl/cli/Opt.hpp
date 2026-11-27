#ifndef RFL_CLI_OPT_HPP_
#define RFL_CLI_OPT_HPP_

#include <cstddef>
#include <string_view>
#include <type_traits>

#include "../internal/StringLiteral.hpp"
#include "../internal/TransparentWrapper.hpp"

namespace rfl::cli {

/// Attribute tags accepted by `rfl::cli::Opt<T, Attrs...>` in any order.
namespace attr {

template <rfl::internal::StringLiteral _name>
struct Short {
  static constexpr auto value = _name;
};

struct Positional {};

struct Hidden {};

template <rfl::internal::StringLiteral _group>
struct Group {
  static constexpr auto value = _group;
};

template <rfl::internal::StringLiteral _name>
struct ValueName {
  static constexpr auto value = _name;
};

template <rfl::internal::StringLiteral _val = "">
struct Implicit {
  static constexpr auto value = _val;
};

struct MultiToken {};

/// Marks an option's value as sensitive so mutable `char* argv[]` overloads
/// overwrite the value in place with `'*'` after parsing (hiding secrets from
/// process listings).
struct Sensitive {};

/// Adds an alternate long option name (without leading `--`) that maps to the
/// same field as its canonical long name (may be repeated on one `Opt`).
template <rfl::internal::StringLiteral _name>
struct Alias {
  static constexpr auto value = _name;
};

/// Marks an integral field as a repeatable counter flag (`-v`, `-vvv`,
/// `--verbose --verbose` → 1, 3, 2).
struct Counter {};

/// Declares that this option cannot be combined with `--_other` (canonical long
/// name). May be repeated on one `Opt`.
template <rfl::internal::StringLiteral _other>
struct Excludes {
  static constexpr auto value = _other;
};

/// Declares that this option requires `--_other` (canonical long name) to also
/// be supplied. May be repeated on one `Opt`.
template <rfl::internal::StringLiteral _other>
struct Needs {
  static constexpr auto value = _other;
};

/// Allows negating a boolean/option flag via `--<prefix><name>` (default
/// `"no-"`, e.g. `--pyc` / `--no-pyc`, or `"no"` for `-nofoo`).
template <rfl::internal::StringLiteral _prefix = "no-">
struct Negatable {
  static constexpr auto prefix = _prefix;
};

/// Registers an explicit negated long option name (without leading `--`) that
/// sets this option to `false` (e.g. `--skip-run-link-scripts` with
/// `NegatedName<"run-link-scripts">`, or `-finline` with
/// `NegatedName<"fno-inline">`). May be repeated on one `Opt`.
template <rfl::internal::StringLiteral _name>
struct NegatedName {
  static constexpr auto value = _name;
};

template <rfl::internal::StringLiteral _desc>
struct Desc {
  static constexpr auto value = _desc;
};

template <rfl::internal::StringLiteral _msg,
          rfl::internal::StringLiteral _desc = "">
struct Deprecated {
  static constexpr auto message = _msg;
  static constexpr auto description = _desc;
};

template <rfl::internal::StringLiteral _hint>
struct DefaultHint {
  static constexpr auto value = _hint;
};

}  // namespace attr

namespace internal {

template <class... Attrs>
struct opt_attrs;

template <>
struct opt_attrs<> {
  static constexpr bool is_short = false;
  static constexpr bool is_positional = false;
  static constexpr bool is_hidden = false;
  static constexpr bool has_group = false;
  static constexpr bool has_value_name = false;
  static constexpr bool is_multi_token = false;
  static constexpr bool is_sensitive = false;
  static constexpr bool is_counter = false;
  static constexpr size_t num_implicit_value = 0;
  static constexpr int count = 0;
  static constexpr rfl::internal::StringLiteral<1> short_name = "";
  static constexpr rfl::internal::StringLiteral<1> implicit_value = "";
  static constexpr rfl::internal::StringLiteral<1> group_name = "";
  static constexpr rfl::internal::StringLiteral<1> value_name = "";
  static constexpr std::string_view description = {};
  static constexpr bool is_deprecated = false;
  static constexpr std::string_view deprecation_message = {};
  static constexpr std::string_view default_hint = {};

  template <class Fn>
  static void for_each_alias(Fn&&) {}
  template <class Fn>
  static void for_each_excludes(Fn&&) {}
  template <class Fn>
  static void for_each_needs(Fn&&) {}
  template <class Fn>
  static void for_each_negated_prefix(Fn&&) {}
  template <class Fn>
  static void for_each_negated_name(Fn&&) {}
};

template <class First, class... Rest>
struct opt_attrs<First, Rest...> {
  static_assert(
      !std::is_same_v<First, First>,
      "Unknown attribute passed to rfl::cli::Opt. Use tags from "
      "rfl::cli::attr (Short, Alias, Positional, Hidden, Group, ValueName, "
      "Implicit, MultiToken, Sensitive, Counter, Excludes, Needs, Negatable, "
      "NegatedName, Desc, Deprecated, DefaultHint).");
};

template <rfl::internal::StringLiteral _name, class... Rest>
struct opt_attrs<attr::Short<_name>, Rest...> : opt_attrs<Rest...> {
  using Next = opt_attrs<Rest...>;
  static constexpr bool is_short = true;
  static constexpr bool is_positional = false;
  static constexpr auto short_name = _name;
  static constexpr int count = Next::count + 1;
};

template <class... Rest>
struct opt_attrs<attr::Positional, Rest...> : opt_attrs<Rest...> {
  using Next = opt_attrs<Rest...>;
  static constexpr bool is_short = false;
  static constexpr bool is_positional = true;
  static constexpr int count = Next::count + 1;
};

template <class... Rest>
struct opt_attrs<attr::Hidden, Rest...> : opt_attrs<Rest...> {
  static constexpr bool is_hidden = true;
};

template <rfl::internal::StringLiteral _group, class... Rest>
struct opt_attrs<attr::Group<_group>, Rest...> : opt_attrs<Rest...> {
  static constexpr bool has_group = true;
  static constexpr auto group_name = _group;
};

template <rfl::internal::StringLiteral _name, class... Rest>
struct opt_attrs<attr::ValueName<_name>, Rest...> : opt_attrs<Rest...> {
  static constexpr bool has_value_name = true;
  static constexpr auto value_name = _name;
};

template <rfl::internal::StringLiteral _val, class... Rest>
struct opt_attrs<attr::Implicit<_val>, Rest...> : opt_attrs<Rest...> {
  using Next = opt_attrs<Rest...>;
  static constexpr size_t num_implicit_value = Next::num_implicit_value + 1;
  static constexpr auto implicit_value = _val;
};

template <class... Rest>
struct opt_attrs<attr::MultiToken, Rest...> : opt_attrs<Rest...> {
  static constexpr bool is_multi_token = true;
};

template <class... Rest>
struct opt_attrs<attr::Sensitive, Rest...> : opt_attrs<Rest...> {
  static constexpr bool is_sensitive = true;
};

template <rfl::internal::StringLiteral _name, class... Rest>
struct opt_attrs<attr::Alias<_name>, Rest...> : opt_attrs<Rest...> {
  template <class Fn>
  static void for_each_alias(Fn&& _fn) {
    _fn(_name.string_view());
    opt_attrs<Rest...>::for_each_alias(_fn);
  }
};

template <class... Rest>
struct opt_attrs<attr::Counter, Rest...> : opt_attrs<Rest...> {
  static constexpr bool is_counter = true;
};

template <rfl::internal::StringLiteral _other, class... Rest>
struct opt_attrs<attr::Excludes<_other>, Rest...> : opt_attrs<Rest...> {
  template <class Fn>
  static void for_each_excludes(Fn&& _fn) {
    _fn(_other.string_view());
    opt_attrs<Rest...>::for_each_excludes(_fn);
  }
};

template <rfl::internal::StringLiteral _other, class... Rest>
struct opt_attrs<attr::Needs<_other>, Rest...> : opt_attrs<Rest...> {
  template <class Fn>
  static void for_each_needs(Fn&& _fn) {
    _fn(_other.string_view());
    opt_attrs<Rest...>::for_each_needs(_fn);
  }
};

template <rfl::internal::StringLiteral _prefix, class... Rest>
struct opt_attrs<attr::Negatable<_prefix>, Rest...> : opt_attrs<Rest...> {
  template <class Fn>
  static void for_each_negated_prefix(Fn&& _fn) {
    _fn(_prefix.string_view());
    opt_attrs<Rest...>::for_each_negated_prefix(_fn);
  }
};

template <rfl::internal::StringLiteral _name, class... Rest>
struct opt_attrs<attr::NegatedName<_name>, Rest...> : opt_attrs<Rest...> {
  template <class Fn>
  static void for_each_negated_name(Fn&& _fn) {
    _fn(_name.string_view());
    opt_attrs<Rest...>::for_each_negated_name(_fn);
  }
};

template <rfl::internal::StringLiteral _desc, class... Rest>
struct opt_attrs<attr::Desc<_desc>, Rest...> : opt_attrs<Rest...> {
  using Next = opt_attrs<Rest...>;
  static constexpr std::string_view description =
      _desc.string_view().empty() ? Next::description : _desc.string_view();
};

template <rfl::internal::StringLiteral _msg,
          rfl::internal::StringLiteral _desc, class... Rest>
struct opt_attrs<attr::Deprecated<_msg, _desc>, Rest...> : opt_attrs<Rest...> {
  using Next = opt_attrs<Rest...>;
  static constexpr std::string_view description =
      _desc.string_view().empty() ? Next::description : _desc.string_view();
  static constexpr bool is_deprecated = true;
  static constexpr std::string_view deprecation_message =
      Next::is_deprecated ? Next::deprecation_message : _msg.string_view();
};

template <rfl::internal::StringLiteral _hint, class... Rest>
struct opt_attrs<attr::DefaultHint<_hint>, Rest...> : opt_attrs<Rest...> {
  using Next = opt_attrs<Rest...>;
  static constexpr std::string_view default_hint =
      _hint.string_view().empty() ? Next::default_hint : _hint.string_view();
};

}  // namespace internal

/// Single-layer variadic CLI option wrapper carrying any combination of
/// `rfl::cli::attr::*` metadata tags in any order, replacing deeply nested
/// `Group<Short<ValueName<Description<T>>>>` stacks with one wrapper.
///
/// Example:
/// ```cpp
/// using namespace rfl::cli::attr;
/// rfl::cli::Opt<std::optional<std::string>,
///               Short<"o">, Group<"Output">, ValueName<"path">,
///               Desc<"Output directory">> output_dir;
/// ```
template <class T, class... Attrs>
struct Opt : rfl::internal::TransparentWrapper<Opt<T, Attrs...>, T> {
  using Base = rfl::internal::TransparentWrapper<Opt<T, Attrs...>, T>;
  using Base::Base;
  using Base::operator=;

  using AttrsInfo = internal::opt_attrs<Attrs...>;

  /// Contextual boolean conversion when the underlying type supports it
  /// (e.g. `Opt<bool>` or `Opt<std::optional<T>>`).
  explicit operator bool() const noexcept
    requires requires(const T& _v) { static_cast<bool>(_v); }
  {
    return static_cast<bool>(this->value_);
  }
};

/// Shorthand for a boolean CLI flag (`Opt<bool, Attrs...>`).
template <class... Attrs>
using Flag = Opt<bool, Attrs...>;

/// Shorthand alias for a typed CLI argument (`Opt<T, Attrs...>`).
template <class T, class... Attrs>
using Arg = Opt<T, Attrs...>;

namespace internal {

template <class T>
struct is_cli_opt : std::false_type {};

template <class T, class... Attrs>
struct is_cli_opt<Opt<T, Attrs...>> : std::true_type {};

template <class T>
constexpr bool is_cli_opt_v =
    is_cli_opt<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace internal

}  // namespace rfl::cli

#endif
