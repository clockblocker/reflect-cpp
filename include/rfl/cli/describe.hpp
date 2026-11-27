#ifndef RFL_CLI_DESCRIBE_HPP_
#define RFL_CLI_DESCRIBE_HPP_

#include <algorithm>
#include <concepts>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../Result.hpp"
#include "App.hpp"
#include "Settings.hpp"
#include "options.hpp"
#include "usage.hpp"

namespace rfl::cli {

/// Public metadata for one named CLI option (`--flag`, `--key=val`, `-k`).
/// Plain reflected aggregate so callers can emit JSON descriptors directly via
/// `rfl::json::write(descriptor)` or drive shell completion / manpage generators
/// without reaching into `rfl::cli::internal::*`.
struct OptionDescriptor {
  /// Canonical long option name (including dotted nested path, e.g. `db.host`).
  std::string name;
  /// Short option alias without leading `-` (empty if none).
  std::string short_name;
  /// Alternate long option names (`rfl::Alias` / `rfl::cli::attr::Alias`).
  std::vector<std::string> aliases;
  /// Negated long option names (`--no-foo`, etc.).
  std::vector<std::string> negated_names;
  /// Parsing behavior (`flag`, `value`, `implicit_value`, `other`).
  OptionKind kind = OptionKind::value;
  /// Value implied when an `implicit_value` option is given without `=...`.
  std::string implicit_value;
  /// True for list/repeated options (vectors, sets, counters, etc.).
  bool is_list = false;
  /// True for additive counter flags (`-v`, `-vvv`).
  bool is_counter = false;
  /// True for `rfl::MultiToken` list options.
  bool multi_token = false;
  /// True for `rfl::cli::attr::Sensitive` options.
  bool sensitive = false;
  /// True when omitting the option fails parsing.
  bool required = false;
  /// True when rendered without a value placeholder (pure flags, counters,
  /// built-in `--help`/`--version`).
  bool bare = false;
  /// True when wrapped in `rfl::Hidden` (preserved here instead of erased).
  bool hidden = false;
  /// True for built-in options injected by `rfl::cli::App` (`--help`, `--version`).
  bool builtin = false;
  /// Raw value placeholder (`"<value>"`, `"<bool>"`, `"<a|b>"`, or custom
  /// `rfl::ValueName`).
  std::string value_name;
  /// Formatted value placeholder taking `is_list` and `list_delimiter` into
  /// account (e.g. `"<v1,v2,...>"` or custom `rfl::ValueName`).
  std::string value_placeholder;
  /// True when `value_name` was explicitly set via `rfl::ValueName`.
  bool has_custom_value_name = false;
  /// Allowed values for enum and `rfl::Literal` options.
  std::vector<std::string> choices;
  /// Help description (`rfl::Description` / `attr::Desc`).
  std::string description;
  /// Deprecation message if wrapped in `rfl::Deprecated` / `attr::Deprecated`.
  std::optional<std::string> deprecation;
  /// Human-readable validator constraints (e.g. `"Range: 1..65535"`).
  std::vector<std::string> constraints;
  /// Formatted default value (or `rfl::DefaultHint`) if available.
  std::optional<std::string> default_value;
  /// Explicit `rfl::DefaultHint` text if provided.
  std::string default_hint;
  /// Section header name from `rfl::Group` / `rfl::cli::OptGroup`.
  std::string group;
  /// Canonical option names mutually excluded with this option (`attr::Excludes`).
  std::vector<std::string> excludes;
  /// Canonical option names required when this option is given (`attr::Needs`).
  std::vector<std::string> needs;
};

/// Public metadata for one positional CLI argument (`rfl::Positional`).
/// Preserves `rfl::Hidden` positionals with `hidden = true` rather than erasing
/// them.
struct PositionalDescriptor {
  /// Canonical field name (after processors such as `SnakeCaseToKebabCase`).
  std::string name;
  /// Alternate long option names (`rfl::Alias` / `rfl::cli::attr::Alias`).
  std::vector<std::string> aliases;
  /// True when the positional field is a list type.
  bool is_list = false;
  /// True when this is the final positional list that consumes all remaining args.
  bool takes_rest = false;
  /// True when omitting the positional fails parsing.
  bool required = true;
  /// True when wrapped in `rfl::Hidden` (preserved here instead of erased).
  bool hidden = false;
  /// Raw value placeholder (`"<value>"`, `"<a|b>"`, or custom `rfl::ValueName`).
  std::string value_name;
  /// Formatted value placeholder taking `is_list` and `list_delimiter` into account.
  std::string value_placeholder;
  /// True when `value_name` was explicitly set via `rfl::ValueName`.
  bool has_custom_value_name = false;
  /// Allowed values for enum and `rfl::Literal` positionals.
  std::vector<std::string> choices;
  /// Help description (`rfl::Description` / `attr::Desc`).
  std::string description;
  /// Deprecation message if wrapped in `rfl::Deprecated`.
  std::optional<std::string> deprecation;
  /// Human-readable validator constraints.
  std::vector<std::string> constraints;
  /// Formatted default value (or `rfl::DefaultHint`) if available.
  std::optional<std::string> default_value;
  /// Explicit `rfl::DefaultHint` text if provided.
  std::string default_hint;
  /// Section header name from `rfl::Group` / `rfl::cli::OptGroup`.
  std::string group;
};

/// Public, recursively composable metadata for a CLI command or top-level app.
struct CommandDescriptor {
  std::string name;
  std::string version;
  std::string description;
  std::vector<std::string> aliases;
  bool require_subcommand = false;
  bool options_first = false;
  /// Effective list delimiter as a 0- or 1-character string (`","`, `""` if disabled).
  std::string list_delimiter;
  std::vector<OptionDescriptor> options;
  std::vector<PositionalDescriptor> positionals;
  std::vector<CommandDescriptor> subcommands;
};

/// Alias for top-level app descriptions returned by `rfl::cli::describe<T>()`.
using AppDescriptor = CommandDescriptor;

/// Public replacement for `rfl::cli::internal::usage_value_to_string`: formats
/// the value placeholder for an option or positional taking `is_list` and
/// `_delimiter` into account.
inline std::string format_value_placeholder(
    const std::string& _value_name, const bool _has_custom_value_name,
    const bool _is_list, const std::optional<char> _delimiter) {
  if (_has_custom_value_name || !_is_list || !_delimiter) {
    return _value_name;
  }
  const auto d = std::string(1, *_delimiter);
  return "<v1" + d + "v2" + d + "...>";
}

namespace internal {

inline std::vector<std::string> collect_aliases_for(
    const Options& _options, const std::string_view _canonical) {
  std::vector<std::string> out;
  for (const auto& [alias, target] : _options.long_aliases) {
    if (target == _canonical) {
      out.push_back(alias);
    }
  }
  return out;
}

inline std::vector<std::string> collect_negated_for(
    const Options& _options, const std::string_view _canonical) {
  std::vector<std::string> out;
  for (const auto& [neg, target] : _options.negated_options) {
    if (target == _canonical) {
      out.push_back(neg);
    }
  }
  return out;
}

inline std::vector<std::string> collect_relations_for(
    const std::vector<std::pair<std::string, std::string>>& _pairs,
    const std::string_view _canonical) {
  std::vector<std::string> out;
  for (const auto& [lhs, rhs] : _pairs) {
    if (lhs == _canonical) {
      out.push_back(rhs);
    }
  }
  return out;
}

template <class T, class... Ps>
rfl::Result<AppDescriptor> describe_from_options(
    const std::string& _program_name, const Options& _options,
    const BuiltinOptions& _builtin, const std::optional<char> _delimiter,
    const UsageOptions& _usage_opts) {
  using ProcessorsType = cli_processors_t<Ps...>;
  using AppMeta = app_settings_t<ProcessorsType>;
  return check_short_names(_options).and_then(
      [&](auto) -> rfl::Result<AppDescriptor> {
        try {
          AppDescriptor desc;
          if constexpr (AppMeta::count > 0) {
            desc.name = AppMeta::name.empty() ? _program_name
                                              : std::string(AppMeta::name);
            desc.version = std::string(AppMeta::version);
            desc.description = _usage_opts.description.empty()
                                   ? std::string(AppMeta::about)
                                   : _usage_opts.description;
          } else {
            desc.name = _program_name;
            desc.description = _usage_opts.description;
          }
          desc.options_first = _options.options_first;
          desc.list_delimiter =
              _delimiter ? std::string(1, *_delimiter) : std::string{};

          auto walker = UsageWalker<ProcessorsType>(
              &_options, _delimiter, /*_include_hidden=*/true);
          auto fields = walker.template collect<T>();
          auto builtin_args =
              builtin_usage_args(_builtin, _usage_opts.help_group);

          const auto is_hidden_group = [&](const std::string_view _grp) {
            if (_grp.empty() || _usage_opts.hidden_groups.empty()) {
              return false;
            }
            return std::find(_usage_opts.hidden_groups.begin(),
                             _usage_opts.hidden_groups.end(),
                             _grp) != _usage_opts.hidden_groups.end();
          };

          const auto to_option_desc = [&](const UsageArg& _arg,
                                          const bool _is_builtin) {
            const auto opt = _options.find_long(_arg.name);
            OptionDescriptor d;
            d.name = _arg.name;
            d.short_name = _arg.short_name;
            d.aliases =
                _is_builtin ? std::vector<std::string>{}
                            : collect_aliases_for(_options, _arg.name);
            d.negated_names =
                _is_builtin ? std::vector<std::string>{}
                            : collect_negated_for(_options, _arg.name);
            d.kind = _arg.kind;
            d.implicit_value = _arg.implicit_value;
            d.is_list = _arg.is_list;
            d.is_counter = !_is_builtin && opt.is_counter;
            d.multi_token = _arg.multi_token;
            d.sensitive = !_is_builtin && opt.sensitive;
            d.required = _arg.required;
            d.bare = _arg.bare;
            d.hidden = _arg.hidden || is_hidden_group(_arg.group);
            d.builtin = _is_builtin;
            d.value_name = _is_builtin ? std::string("<bool>") : _arg.value;
            d.value_placeholder =
                d.is_counter
                    ? std::string{}
                    : format_value_placeholder(d.value_name,
                                               _arg.has_custom_value_name,
                                               _arg.is_list, _delimiter);
            d.has_custom_value_name = _arg.has_custom_value_name;
            d.choices = _arg.choices;
            d.description = _arg.description;
            d.deprecation = _arg.deprecation;
            d.constraints = _arg.constraints;
            d.default_value = _arg.default_value;
            d.default_hint = _arg.default_hint;
            d.group = _arg.group;
            if (!_is_builtin) {
              d.excludes = collect_relations_for(_options.exclusions, _arg.name);
              d.needs = collect_relations_for(_options.requirements, _arg.name);
            }
            return d;
          };

          const auto to_pos_desc = [&](const UsageArg& _arg) {
            PositionalDescriptor p;
            p.name = _arg.name;
            p.aliases = collect_aliases_for(_options, _arg.name);
            p.is_list = _arg.is_list;
            p.takes_rest = _arg.takes_rest;
            p.required = _arg.required;
            p.hidden = _arg.hidden || is_hidden_group(_arg.group);
            p.value_name = _arg.value;
            p.value_placeholder = format_value_placeholder(
                _arg.value, _arg.has_custom_value_name, _arg.is_list,
                _delimiter);
            p.has_custom_value_name = _arg.has_custom_value_name;
            p.choices = _arg.choices;
            p.description = _arg.description;
            p.deprecation = _arg.deprecation;
            p.constraints = _arg.constraints;
            p.default_value = _arg.default_value;
            p.default_hint = _arg.default_hint;
            p.group = _arg.group;
            return p;
          };

          // Place built-in options consistently with usage().
          std::vector<OptionDescriptor> builtin_descs;
          builtin_descs.reserve(builtin_args.size());
          for (const auto& b : builtin_args) {
            builtin_descs.push_back(to_option_desc(b, /*_is_builtin=*/true));
          }

          bool inserted_builtins = false;
          if (_usage_opts.help_group.empty()) {
            for (auto& b : builtin_descs) {
              desc.options.push_back(std::move(b));
            }
            inserted_builtins = true;
          }

          for (const auto& f : fields) {
            if (f.positional) {
              desc.positionals.push_back(to_pos_desc(f));
            } else {
              if (!inserted_builtins && !_usage_opts.help_group.empty() &&
                  f.group == _usage_opts.help_group) {
                for (auto& b : builtin_descs) {
                  desc.options.push_back(std::move(b));
                }
                inserted_builtins = true;
              }
              desc.options.push_back(to_option_desc(f, /*_is_builtin=*/false));
            }
          }

          if (!inserted_builtins) {
            desc.options.insert(desc.options.begin(),
                                std::make_move_iterator(builtin_descs.begin()),
                                std::make_move_iterator(builtin_descs.end()));
          }

          return desc;
        } catch (std::exception& e) {
          return error(e.what());
        }
      });
}

}  // namespace internal

/// Returns a complete public `AppDescriptor` describing every option,
/// positional argument (including `rfl::Hidden` options and `rfl::Hidden`
/// positionals with `.hidden = true`), built-in `rfl::cli::App` flag, default
/// value, constraint, alias, and group for reflected struct `T` with
/// processors `Ps...`.
template <class T, class... Ps>
rfl::Result<AppDescriptor> describe(
    const std::string& _program_name = {},
    const UsageOptions& _usage_opts = UsageOptions{.detailed = true}) {
  using ProcessorsType = internal::cli_processors_t<Ps...>;
  constexpr auto delimiter = internal::list_delimiter_v<ProcessorsType>;
  const auto& [options, builtin] =
      internal::cached_app_options<T, ProcessorsType, /*_all_paths=*/true>();
  return internal::describe_from_options<T, Ps...>(
      _program_name, options, builtin, delimiter, _usage_opts);
}

/// Like `rfl::cli::describe<T, Ps...>`, applying a runtime `rfl::cli::Settings`
/// configuration. Constrained with `std::same_as<Settings>` so positional
/// braced-init-lists `{...}` resolve unambiguously to `UsageOptions`.
template <class T, class... Ps, std::same_as<Settings> SettingsT>
rfl::Result<AppDescriptor> describe(
    const std::string& _program_name, const SettingsT& _settings,
    const UsageOptions& _usage_opts = UsageOptions{.detailed = true}) {
  using ProcessorsType = internal::cli_processors_t<Ps...>;
  const auto valid = internal::validate_settings(_settings);
  if (!valid) {
    return error(valid.error().what());
  }
  const auto& [base_options, builtin] =
      internal::cached_app_options<T, ProcessorsType, /*_all_paths=*/true>();
  const auto options = internal::apply_settings(base_options, _settings);
  return internal::describe_from_options<T, Ps...>(
      _program_name, options, builtin, _settings.list_delimiter, _usage_opts);
}

}  // namespace rfl::cli

#endif
