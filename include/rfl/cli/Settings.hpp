#ifndef RFL_CLI_SETTINGS_HPP_
#define RFL_CLI_SETTINGS_HPP_

#include <optional>
#include <string>

#include "../NoExtraFields.hpp"
#include "../Processors.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "AllowAbbreviations.hpp"
#include "BoolValueTokens.hpp"
#include "KeepEmptyItems.hpp"
#include "LastValueWins.hpp"
#include "ListDelimiter.hpp"
#include "OptionsFirst.hpp"
#include "PureFlags.hpp"
#include "RequireEquals.hpp"
#include "SeparateOptionValues.hpp"
#include "SingleDashLong.hpp"
#include "options.hpp"

namespace rfl::cli {

/// Consolidated runtime configuration for CLI parsing modes.
///
/// Replaces combinatorial Wave 5 processor packs (`SeparateOptionValues`,
/// `PureFlags`, `LastValueWins`, `OptionsFirst`, `AllowAbbreviations`,
/// `RequireEquals`, `SingleDashLong`, `BoolValueTokens`, `KeepEmptyItems`,
/// `ListDelimiter`, `NoExtraFields`) with a single value object so callers can
/// switch presets or modes without duplicating parser template instantiations.
///
/// Compile-time Wave 5 processors remain supported and can be converted into a
/// `Settings` value via `Settings::from_processors<Ps...>()`.
struct Settings {
  /// Accept `--key value` in addition to `--key=value`.
  bool separate_values = false;
  /// Short boolean flags (`-v`) never consume a following `true`/`false`/`1`/`0`.
  bool pure_flags = false;
  /// Repeated scalar options overwrite earlier values instead of failing.
  bool last_value_wins = false;
  /// Stop parsing options at the first positional argument (POSIX `+` mode).
  bool options_first = false;
  /// Allow unambiguous prefixes of long options (`--rep` -> `--report`).
  bool allow_abbreviations = false;
  /// Require `=` for long value options when `separate_values` is false.
  bool require_equals = false;
  /// Parse single-dash tokens (`-foo`, `-foo=bar`) as long options.
  bool single_dash_long = false;
  /// Allow long boolean flags to consume a following boolean literal token.
  bool bool_value_tokens = false;
  /// Reject unrecognized long and short options.
  bool reject_unknown = false;
  /// Keep empty items when splitting list values on `list_delimiter`.
  bool keep_empty_items = false;
  /// Character separating items inside a single list option value (`std::nullopt`
  /// disables splitting so each occurrence is one item).
  std::optional<char> list_delimiter = array_delimiter;
  /// Allow automatic `<negation_prefix><name>` negation for any boolean option.
  bool allow_universal_negation = false;
  /// Prefix used when `allow_universal_negation` is true (default `"no"`).
  std::string negation_prefix = "no";
  /// Allow value options to consume a following token that starts with `'-'`.
  bool allow_option_like_values = false;

  /// Default reflect-cpp CLI mode (`--key=value`, comma-delimited lists).
  static Settings default_settings() { return Settings{}; }

  /// Standard GNU-style CLI mode (`--key value`, pure flags, reject unknown).
  static Settings gnu() {
    Settings s;
    s.separate_values = true;
    s.pure_flags = true;
    s.reject_unknown = true;
    return s;
  }

  /// GNU-style CLI mode where repeated scalar options overwrite earlier values.
  static Settings gnu_last_wins() {
    Settings s = gnu();
    s.last_value_wins = true;
    return s;
  }

  /// Strict POSIX-style CLI mode stopping option parsing at the first positional.
  static Settings posix_strict() {
    Settings s = gnu();
    s.options_first = true;
    return s;
  }

  /// Single-dash long option mode (e.g. `-foo value`, `-bar=1`).
  static Settings single_dash() {
    Settings s = gnu();
    s.single_dash_long = true;
    return s;
  }

  /// Builds a `Settings` value reflecting the Wave 5 compile-time processors
  /// in `Ps...` (including bundles such as `rfl::cli::GnuStyle`).
  template <class... Ps>
  static Settings from_processors() {
    using P = internal::cli_processors_t<Ps...>;
    Settings s;
    s.separate_values = internal::separate_option_values_v<P>;
    s.pure_flags = internal::pure_flags_v<P>;
    s.last_value_wins = internal::last_value_wins_v<P>;
    s.options_first = internal::options_first_v<P>;
    s.allow_abbreviations = internal::allow_abbreviations_v<P>;
    s.require_equals = internal::require_equals_v<P>;
    s.single_dash_long = internal::single_dash_long_v<P>;
    s.bool_value_tokens = internal::bool_value_tokens_v<P>;
    s.reject_unknown = rfl::internal::no_extra_fields_v<P>;
    s.keep_empty_items = internal::keep_empty_items_v<P>;
    s.list_delimiter = internal::list_delimiter_v<P>;
    return s;
  }
};

namespace internal {

/// Applies runtime `Settings` onto a copy of a finalized `Options` table.
inline Options apply_settings(const Options& _base, const Settings& _settings) {
  Options opts = _base;
  opts.separate_values = _settings.separate_values;
  opts.pure_flags = _settings.pure_flags;
  opts.last_value_wins = _settings.last_value_wins;
  opts.options_first = _settings.options_first;
  opts.allow_abbreviations = _settings.allow_abbreviations;
  opts.require_equals = _settings.require_equals;
  opts.single_dash_long = _settings.single_dash_long;
  opts.bool_value_tokens = _settings.bool_value_tokens;
  opts.reject_unknown = _settings.reject_unknown;
  opts.allow_universal_negation = _settings.allow_universal_negation;
  opts.negation_prefix = _settings.negation_prefix;
  opts.allow_option_like_values = _settings.allow_option_like_values;
  return opts;
}

inline rfl::Result<bool> validate_settings(const Settings& _settings) {
  if (_settings.keep_empty_items && !_settings.list_delimiter.has_value()) {
    return error(
        "rfl::cli::Settings::keep_empty_items has no effect when "
        "list_delimiter is empty (std::nullopt), which does not split lists.");
  }
  return true;
}

// Strips `rfl::NoExtraFields` from a flat `Processors<...>` pack so runtime
// `Settings{.reject_unknown = false}` cleanly overrides compile-time bundles
// like `rfl::cli::GnuStyle`.
template <class Acc, class Rest>
struct strip_no_extra_fields;

template <class... Acc>
struct strip_no_extra_fields<Processors<Acc...>, Processors<>> {
  using type = Processors<Acc...>;
};

template <class... Acc, class... Tail>
struct strip_no_extra_fields<Processors<Acc...>,
                             Processors<NoExtraFields, Tail...>>
    : strip_no_extra_fields<Processors<Acc...>, Processors<Tail...>> {};

template <class... Acc, class Head, class... Tail>
struct strip_no_extra_fields<Processors<Acc...>, Processors<Head, Tail...>>
    : strip_no_extra_fields<Processors<Acc..., Head>, Processors<Tail...>> {};

template <class ProcessorsType>
using strip_no_extra_fields_t =
    typename strip_no_extra_fields<Processors<>, ProcessorsType>::type;

template <class ProcessorsType>
struct add_no_extra_fields;

template <class... Ps>
struct add_no_extra_fields<Processors<Ps...>> {
  using type = std::conditional_t<
      rfl::internal::no_extra_fields_v<Processors<Ps...>>, Processors<Ps...>,
      Processors<Ps..., NoExtraFields>>;
};

template <class ProcessorsType>
using add_no_extra_fields_t =
    typename add_no_extra_fields<ProcessorsType>::type;

}  // namespace internal

}  // namespace rfl::cli

#endif
