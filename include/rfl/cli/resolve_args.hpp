#ifndef RFL_CLI_RESOLVE_ARGS_HPP_
#define RFL_CLI_RESOLVE_ARGS_HPP_

#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../Result.hpp"
#include "options.hpp"

namespace rfl::cli {

/// Rich per-occurrence record captured during argv parsing.
struct OptionOccurrence {
  /// Canonical long option name (without leading dashes).
  std::string name;
  /// Effective string value (or raw value after '=' when `has_value` is true).
  std::string value;
  /// True when an explicit value was supplied (`--k=v`, `--k v`, `-k v`).
  bool has_value = false;
  /// True when invoked via a negated option name (`--no-foo`, `-nofoo`).
  bool negated = false;
  /// Raw argv token that introduced this option.
  std::string raw;
  /// For prefix-matched options (e.g. `-Wno-fatal` matching prefix `Wno-`, or
  /// `-debugi-V3Options 3` matching `debugi-`), the matched suffix after the
  /// prefix. Empty for exact options.
  std::string prefix_suffix;
};

/// Holds the parsed command-line arguments categorized by type.
/// Used as an intermediate structure before resolving to a flat key-value map.
struct ParsedArgs {
  /// The long options, by name. A list option is held here under its long
  /// name however it is spelled, also as a short option. If it is given
  /// more than once, this holds its first occurrence, and
  /// `later_occurrences` the others.
  std::map<std::string, std::string> named;
  std::map<std::string, std::string> short_args;
  std::vector<std::string> positional;
  /// Insertion order of short arg keys (for stable reclaim ordering).
  std::vector<std::string> short_order;
  /// The occurrences after the first of each list option given more than
  /// once, by long name, in argv order.
  internal::CliLaterOccurrencesType later_occurrences;
  /// Every recognized option occurrence `(canonical_long_name, raw_value)` in
  /// exact argv order.
  std::vector<std::pair<std::string, std::string>> occurrences;
  /// Rich per-occurrence metadata in exact argv order.
  std::vector<OptionOccurrence> detailed_occurrences;
};

namespace internal {

/// The arguments resolved to the paths of their fields, which Reader reads.
struct ResolvedArgs {
  /// The value of each field. For a list given more than once, its first
  /// occurrence.
  std::map<std::string, std::string> values;
  /// The occurrences after the first of each list given more than once.
  CliLaterOccurrencesType later_occurrences;
  /// The path of the list whose items are taken as they are, never split or
  /// dropped, or empty: with rfl::cli::OptionsFirst, the list as the last
  /// positional field when it gets the rest of the command line.
  std::string verbatim_list;
  /// Every option occurrence `(canonical_long_name, raw_value)` in argv order.
  std::vector<std::pair<std::string, std::string>> occurrences;
  /// Rich per-occurrence metadata in argv order.
  std::vector<OptionOccurrence> detailed_occurrences;
};

/// Applies what parse_argv could not know without the option table.
/// Without it, every short option takes the next token, so `-v somefile`
/// makes "somefile" the value of -v. If -v is a bool flag (by its type once
/// unwrapped, as in options.hpp), "somefile" is not a valid bool value and
/// is returned to the positional list. We iterate in insertion order
/// (short_order) to preserve argv ordering.
inline void apply_option_kinds(const Options& _options, ParsedArgs* _parsed) {
  std::vector<std::string> reclaimed;
  for (const auto& short_name : _parsed->short_order) {
    const auto option = _options.find_short(short_name);
    auto it = _parsed->short_args.find(short_name);
    if (it == _parsed->short_args.end()) {
      continue;
    }
    auto& value = it->second;
    if (option.kind == OptionKind::flag && !value.empty()
        && (_options.pure_flags || !is_bool_literal(value))) {
      reclaimed.push_back(value);
      value.clear();
    }
  }
  // Insert reclaimed values before existing positional args to preserve
  // the original argv order: `-v file1 file2` → positional ["file1", "file2"].
  if (!reclaimed.empty()) {
    _parsed->positional.insert(
        _parsed->positional.begin(),
        std::make_move_iterator(reclaimed.begin()),
        std::make_move_iterator(reclaimed.end()));
  }
}

/// Fails if two fields of the struct have the same rfl::Short name, so
/// every read of such a struct fails.
inline rfl::Result<bool> check_short_names(const Options& _options) {
  if (!_options.duplicate_short_name.empty()) {
    return rfl::error("Duplicate short name '-" + _options.duplicate_short_name
                      + "' in struct definition.");
  }
  return true;
}

/// The error for more positional arguments than positional fields.
inline rfl::Unexpected<Error> too_many_positionals(const size_t _expected,
                                                    const size_t _got) {
  return rfl::error("Too many positional arguments: expected at most "
                    + std::to_string(_expected) + ", got "
                    + std::to_string(_got) + ".");
}

/// Validates mutual exclusion (`Excludes`) and co-dependency (`Needs`) rules
/// against the resolved canonical option names.
inline rfl::Result<bool> check_option_constraints(
    const std::map<std::string, std::string>& _values,
    const Options& _options) {
  for (const auto& [opt, excluded] : _options.exclusions) {
    if (_values.count(opt) && _values.count(excluded)) {
      return rfl::error("Option '--" + opt + "' cannot be used with '--"
                        + excluded + "'.");
    }
  }
  for (const auto& [opt, needed] : _options.requirements) {
    if (_values.count(opt) && !_values.count(needed)) {
      return rfl::error("Option '--" + opt + "' requires '--" + needed + "'.");
    }
  }
  return true;
}

/// Resolves ParsedArgs into a flat key-value map using the options of the
/// target struct.
/// This function:
/// 1. Maps positional arguments to their corresponding field names based on declaration order
///    If the last positional field is a list, it takes all remaining
///    positional arguments, as later occurrences of the list.
/// 2. Expands short arguments (e.g., -p) to long names (e.g., --port) using Short<> annotations
/// 3. Detects conflicts between different ways of specifying the same field
/// The caller checks the short names first (check_short_names).
/// @param _parsed The arguments, parsed by parse_argv with the same options
/// @param _options The options of the target struct (see make_options)
/// @return A Result containing the values of the fields, by path, or an
/// error
inline rfl::Result<ResolvedArgs> resolve_args(
    ParsedArgs _parsed, const Options& _options
) {
  std::map<std::string, std::string> result;
  for (auto& [key, val] : _parsed.named) {
    const auto* canonical = _options.find_long_alias(key);
    const auto& target = canonical ? *canonical : key;
    if (!result.emplace(target, std::move(val)).second) {
      return rfl::error(
          "Conflict: '--" + key + "' and '--" + target
          + "' refer to the same field.");
    }
  }

  // Merge positional args.
  const auto& positional_names = _options.positionals;
  if (_parsed.positional.size() > positional_names.size()
      && !_options.last_positional_is_list()) {
    return internal::too_many_positionals(positional_names.size(),
                                          _parsed.positional.size());
  }
  for (size_t i = 0; i < _parsed.positional.size(); ++i) {
    if (i >= positional_names.size()) {
      // A list as the last positional field takes the rest, one occurrence
      // per argument.
      _parsed.later_occurrences[positional_names.back()].emplace_back(
          std::move(_parsed.positional[i]));
      continue;
    }
    const auto& long_name = positional_names[i];
    if (result.count(long_name)) {
      return rfl::error(
          "Conflict: positional argument and '--" + long_name
          + "' both provided.");
    }
    result.emplace(long_name, std::move(_parsed.positional[i]));
  }

  // Merge short args.
  for (auto& [short_name, value] : _parsed.short_args) {
    const auto* long_name_ptr = _options.find_short_target(short_name);
    if (!long_name_ptr) {
      return rfl::error("Unknown short argument: -" + short_name);
    }
    const auto& long_name = *long_name_ptr;
    if (result.count(long_name)) {
      return rfl::error(
          "Conflict: '-" + short_name + "' and '--" + long_name
          + "' both provided.");
    }
    result.emplace(long_name, std::move(value));
  }

  // With rfl::cli::OptionsFirst, the rest of the command line is passed
  // through to the list as it is.
  auto verbatim_list = std::string();
  if (_options.options_first && _options.last_positional_is_list()
      && _parsed.positional.size() >= positional_names.size()) {
    verbatim_list = positional_names.back();
  }

  if (const auto ok = check_option_constraints(result, _options); !ok) {
    return rfl::error(ok.error().what());
  }

  return ResolvedArgs{std::move(result),
                      std::move(_parsed.later_occurrences),
                      std::move(verbatim_list),
                      std::move(_parsed.occurrences),
                      std::move(_parsed.detailed_occurrences)};
}

/// The error for a long option that abbreviates several options, worded
/// like getopt_long's: "Option '--for' is ambiguous; possibilities:
/// '--force-export', '--format'."
inline std::string ambiguous_option_message(
    const std::string& _key, const std::vector<std::string>& _names
) {
  auto msg = "Option '--" + _key + "' is ambiguous; possibilities: ";
  for (size_t i = 0; i < _names.size(); ++i) {
    msg += (i == 0 ? "'--" : ", '--") + _names[i] + "'";
  }
  return msg + ".";
}

/// Replaces the long options in `_named` that match no field by the one long
/// option they abbreviate (rfl::cli::AllowAbbreviations), as
/// parse_string_views does while parsing.
inline rfl::Result<std::map<std::string, std::string>> expand_abbreviations(
    std::map<std::string, std::string> _named, const Options& _options) {
  std::map<std::string, std::string> expanded;
  for (auto& [key, value] : _named) {
    auto name = key;
    if (_options.find_long(key).kind == OptionKind::unknown) {
      const auto names = _options.find_abbreviations(key);
      if (names.size() > 1) {
        return rfl::error(ambiguous_option_message(key, names));
      }
      if (names.size() == 1) {
        name = names.front();
      }
    }
    if (!expanded.emplace(name, std::move(value)).second) {
      return rfl::error("Duplicate argument: --" + name);
    }
  }
  return expanded;
}

}  // namespace internal

/// Resolves ParsedArgs into a flat key-value map using compile-time
/// metadata from the target struct T. Like the overload taking the options
/// of T, but for arguments parsed by parse_argv without them: values that
/// boolean short options took from the next token are returned to the
/// positional arguments. A flat map cannot hold the occurrences of a
/// repeated list option, which parse_argv without the options rejects, so
/// a list as the last positional field does not take the remaining
/// positional arguments here either: each positional field takes one.
/// With rfl::cli::AllowAbbreviations, long options that match no field are
/// expanded here (`--rep=a` for `--report=a`), but parse_argv, which does
/// not know the struct, has already read `--rep value` as `--rep` and a
/// positional argument. rfl::ImplicitValue needs the options, too: without
/// them, `--help` gets an empty value and `-h x` gives `x` to `-h`.
/// @tparam T The target struct type
/// @tparam ProcessorsType The processors applied to the struct
/// @param _parsed The parsed command-line arguments
/// @return A Result containing a flat map of field names to values or an error
template <class T, class ProcessorsType>
rfl::Result<std::map<std::string, std::string>> resolve_args(
    ParsedArgs _parsed
) {
  if (!_parsed.later_occurrences.empty()) {
    return rfl::error("Duplicate argument: --"
                      + _parsed.later_occurrences.begin()->first);
  }
  using FlatProcessors = rfl::internal::flatten_processors_t<ProcessorsType>;
  const auto& options = internal::cached_options<T, FlatProcessors>();
  const auto short_names = internal::check_short_names(options);
  if (!short_names) {
    return rfl::error(short_names.error().what());
  }
  if (options.allow_abbreviations) {
    auto named =
        internal::expand_abbreviations(std::move(_parsed.named), options);
    if (!named) {
      return rfl::error(named.error().what());
    }
    _parsed.named = std::move(*named);
  }
  internal::apply_option_kinds(options, &_parsed);
  if (_parsed.positional.size() > options.positionals.size()) {
    return internal::too_many_positionals(options.positionals.size(),
                                          _parsed.positional.size());
  }
  return internal::resolve_args(std::move(_parsed), options)
      .transform([](internal::ResolvedArgs _args) {
        return std::move(_args.values);
      });
}

}  // namespace rfl::cli

#endif
