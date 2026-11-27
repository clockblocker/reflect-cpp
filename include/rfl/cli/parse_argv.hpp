#ifndef RFL_CLI_PARSE_ARGV_HPP_
#define RFL_CLI_PARSE_ARGV_HPP_

#include <algorithm>
#include <cctype>
#include <concepts>
#include <initializer_list>
#include <map>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../Result.hpp"
#include "../internal/strings/utf8_conversions.hpp"
#include "options.hpp"
#include "resolve_args.hpp"

namespace rfl::cli {

/// Returns true if the token looks like a CLI option ('-' followed by
/// anything except a digit or '.'), as opposed to a negative number like
/// "-42".
/// This helps distinguish option flags from negative numeric values.
/// @param _token The command-line token to check
/// @return true if it looks like an option, false otherwise
inline bool looks_like_option(std::string_view _token) noexcept {
  return _token.size() >= 2
         && _token[0] == '-'
         && !std::isdigit(static_cast<unsigned char>(_token[1]))
         && _token[1] != '.';
}

namespace internal {

/// Parses command-line arguments into categorized buckets.
/// Handles three types of arguments:
/// - Long options: --key=value, --key value (with
///   `_options.separate_values`) or --flag (stored in `named`)
/// - Short options: -x value, -x=value, or -x (stored in `short_args`)
/// - Positional arguments: bare arguments not prefixed with dashes (stored in `positional`)
/// The special argument "--" forces all subsequent arguments to be treated as positional.
/// If `_options.options_first` is set (rfl::cli::OptionsFirst), so does the
/// first positional argument, which is kept: every later token is a
/// positional argument as it is, including "--", as in getopt's '+' mode.
/// `_options` says what each option does with the token that follows it
/// when it is given without '='. A short option that takes a value consumes
/// it, and fails if there is none; so does a long one with
/// `_options.separate_values` (rfl::cli::SeparateOptionValues). A long
/// boolean flag never consumes it; a short one consumes it only if it is
/// "true", "false", "1" or "0". An option wrapped in rfl::ImplicitValue
/// never consumes it and gets its implicit value. Other long options never
/// consume it. Short options missing from `_options` consume it unless it
/// looks like an option, as they do without `_options`. With
/// `_options.reject_unknown`, a long option that matches no field is an
/// error.
/// A token that looks like a negative number (-5, -.5) is a positional
/// argument unless `_options.negative_numbers_are_options` is set, as it is
/// without `_options` and when the struct has a short option whose name
/// looks like a number (rfl::Short<"1", ...>), as in argparse.
/// Short options known to `_options` can be grouped as in getopt: in `-qvf`
/// every flag is set until the first option that takes a value, which
/// takes the rest of the token (`-qfcsv`) or, if nothing is left, the next
/// token (`-qf csv`). An rfl::ImplicitValue option takes the rest of the
/// token if there is one, and never the next token. Without `_options` from
/// make_options, short options are not grouped: `-abc` is the short option
/// `abc`.
/// If `_options.allow_abbreviations` is set (rfl::cli::AllowAbbreviations),
/// a long option that matches no field stands for the one long option it is
/// a prefix of (`--rep` for `--report`), and a prefix of several is an
/// error.
/// Every element of `_args` is parsed; nothing is skipped as the program name.
/// Every occurrence of a list option (Option::is_list) adds to it, however
/// it is spelled. Its first occurrence is kept in `named` under its long
/// name, and the later ones in `later_occurrences`, in argv order. Any other
/// option given twice is an error.
/// @param _args The command-line arguments to parse
/// @param _options The options of the target struct (see make_options)
/// @return A Result containing ParsedArgs with categorized arguments or an error
/// --key=value / --key value (with separate_values) / --flag → named
/// -x value / -x=value / -x → short_args
/// bare arguments → positional
/// -- → everything after goes to positional
inline rfl::Result<ParsedArgs> parse_string_views(
    std::span<const std::string_view> _args, const Options& _options,
    std::vector<std::string_view>* _sensitive_spans = nullptr) {
  ParsedArgs result;
  const bool negative_numbers_are_options =
      _options.negative_numbers_are_options;

  // Without rfl::NoExtraFields, `_options` lists only the top-level fields
  // (see make_options), so a nested option such as `--database.replicas` is
  // looked up in the cached table of all paths on first use.
  const Options* all_paths = nullptr;
  const auto find_long = [&](const std::string_view _key) {
    if (!_options.all_paths_options
        || _key.find(path_separator) == std::string_view::npos) {
      return _options.find_long(_key);
    }
    if (!all_paths) {
      all_paths = &_options.all_paths_options();
    }
    return all_paths->find_long(_key);
  };

  // Every occurrence of a list option adds to it. Counter options increment on
  // each occurrence. Any other option can be given only once unless
  // LastValueWins is set.
  const auto add_long = [&](std::string _key, std::string _val,
                            const Option& _option, std::string_view _raw,
                            const bool _has_value = false,
                            const bool _negated = false,
                            std::string_view _raw_val = {},
                            std::string _prefix_suffix = {}) -> rfl::Result<bool> {
    if (_option.is_counter) {
      if (!_val.empty()) {
        return error("Counter option '" + std::string(_raw)
                     + "' does not take a value.");
      }
      const auto it = result.named.find(_key);
      unsigned long long count = 1;
      if (it == result.named.end()) {
        result.named.emplace(_key, "1");
      } else {
        count = std::stoull(it->second) + 1;
        it->second = std::to_string(count);
      }
      auto count_str = std::to_string(count);
      result.detailed_occurrences.push_back(
          OptionOccurrence{_key, count_str, false, false, std::string(_raw), {}});
      result.occurrences.emplace_back(std::move(_key), std::move(count_str));
      return true;
    }
    result.detailed_occurrences.push_back(OptionOccurrence{
        _key,
        _negated ? std::string(_raw_val) : _val,
        _has_value,
        _negated,
        std::string(_raw),
        std::move(_prefix_suffix),
    });
    result.occurrences.emplace_back(_key, _val);
    const auto it = result.named.find(_key);
    if (it == result.named.end()) {
      result.named.emplace(std::move(_key), std::move(_val));
    }
    else if (_option.is_list) {
      result.later_occurrences[it->first].emplace_back(std::move(_val));
    }
    else if (_options.last_value_wins) {
      it->second = std::move(_val);
    }
    else {
      return error("Duplicate argument: " + std::string(_raw));
    }
    return true;
  };

  const auto add_short = [&](std::string _key, std::string _val,
                             const Option& _option, std::string_view _raw,
                             const bool _has_value = false) -> rfl::Result<bool> {
    if (_option.is_list || _option.is_counter
        || (_options.last_value_wins && _option.kind != OptionKind::unknown)) {
      // Under its long name, so that `-r a --report b` keeps argv order and
      // LastValueWins / Counter resolve short/long occurrences cleanly.
      return add_long(*_options.find_short_target(_key),
                      std::move(_val), _option, _raw, _has_value, false);
    }
    if (const auto* target = _options.find_short_target(_key)) {
      result.detailed_occurrences.push_back(OptionOccurrence{
          *target, _val, _has_value, false, std::string(_raw), {}});
      result.occurrences.emplace_back(*target, _val);
    }
    result.short_order.emplace_back(_key);
    if (!result.short_args.emplace(std::move(_key), std::move(_val)).second) {
      return error("Duplicate short argument: " + std::string(_raw));
    }
    return true;
  };

  const auto record_sensitive = [&](const Option& _option,
                                    const std::string_view _span) {
    if (_sensitive_spans && _option.sensitive && !_span.empty()) {
      _sensitive_spans->push_back(_span);
    }
  };

  // The value of the option _args[*_i], given without '=', taking the next
  // token if the option says so.
  const auto take_value = [&](const Option& _option, std::string_view _raw,
                              size_t* _i) -> rfl::Result<std::string> {
    const bool has_next = *_i + 1 < _args.size();
    switch (_option.kind) {
      case OptionKind::value:
        // -k value
        if (!has_next
            || (!_options.allow_option_like_values
                && looks_like_option(_args[*_i + 1]))) {
          return error(
              "Missing value for option '" + std::string(_raw) + "'.");
        }
        record_sensitive(_option, _args[*_i + 1]);
        return std::string(_args[++*_i]);

      case OptionKind::flag:
        // -f false: a flag takes a bool literal unless PureFlags or Counter is
        // set. Any other token stays where it is, so `-v input.txt` leaves
        // "input.txt" positional.
        if (!_options.pure_flags && !_option.is_counter && has_next
            && is_bool_literal(_args[*_i + 1])) {
          return std::string(_args[++*_i]);
        }
        return std::string();

      case OptionKind::implicit_value:
        // Never takes the next token: `-h input.txt` leaves "input.txt"
        // positional.
        return std::string(_option.implicit_value);

      case OptionKind::other:
        // A variant, rfl::Generic or nested struct: `-m fast` takes the next
        // token unless it looks like an option.
        if (has_next && !looks_like_option(_args[*_i + 1])) {
          record_sensitive(_option, _args[*_i + 1]);
          return std::string(_args[++*_i]);
        }
        return std::string();

      default:
        return std::string();
    }
  };

  bool force_positional = false;
  for (size_t i = 0; i < _args.size(); ++i) {
    const std::string_view arg_raw(_args[i]);

    if (force_positional) {
      result.positional.emplace_back(arg_raw);
      continue;
    }

    // "--" separator: everything after is positional.
    if (arg_raw == "--") {
      force_positional = true;
      continue;
    }

    // Long argument: --key=value, --key value or --flag (and single-dash long
    // options -key=value / -key when SingleDashLong is enabled).
    const bool is_double_dash = arg_raw.starts_with("--");
    const bool is_single_dash_long =
        !is_double_dash && _options.single_dash_long && arg_raw.size() >= 2
        && arg_raw[0] == '-'
        && (looks_like_option(arg_raw) || negative_numbers_are_options);
    if (is_double_dash || is_single_dash_long) {
      const auto arg = arg_raw.substr(is_double_dash ? 2 : 1);
      const auto eq = arg.find('=');
      auto key = std::string(
          eq == std::string_view::npos ? arg : arg.substr(0, eq));
      const std::string_view eq_val =
          eq == std::string_view::npos ? std::string_view{} : arg.substr(eq + 1);
      auto val = std::string(eq_val);
      if (key.empty()) {
        return error("Empty key in argument: " + std::string(arg_raw));
      }
      bool negated = false;
      bool matched_prefix = false;
      std::string prefix_suffix;
      if (const auto* canonical = _options.find_long_alias(key)) {
        key = *canonical;
      }
      auto option = find_long(key);
      if (option.kind == OptionKind::unknown) {
        if (const auto* neg_target = _options.find_negated_option(key)) {
          key = *neg_target;
          option = find_long(key);
          negated = true;
        } else if (_options.allow_universal_negation
                   && !_options.negation_prefix.empty()
                   && key.size() > _options.negation_prefix.size()
                   && key.starts_with(_options.negation_prefix)) {
          const auto stripped =
              std::string_view(key).substr(_options.negation_prefix.size());
          const auto stripped_opt = find_long(stripped);
          if (stripped_opt.kind != OptionKind::unknown
              || !_options.reject_unknown) {
            key = std::string(stripped);
            option = stripped_opt;
            negated = true;
          }
        }
      }
      if (option.kind == OptionKind::unknown) {
        if (const auto* pfx = _options.find_prefix_option(arg)) {
          matched_prefix = true;
          option = pfx->second;
          if (option.kind == OptionKind::flag) {
            // Single-token prefix match (e.g. -DFOO=1, -Wno-fatal, -O3):
            // full remainder after prefix is both prefix_suffix and value.
            prefix_suffix = std::string(arg.substr(pfx->first.size()));
            val = prefix_suffix;
          } else {
            // Prefix match taking a separate (or =value) argument
            // (e.g. -debugi-V3Options 3).
            prefix_suffix = std::string(
                std::string_view(key).substr(pfx->first.size()));
          }
          key = pfx->first;
        }
      }
      if (option.kind == OptionKind::unknown && _options.allow_abbreviations) {
        // --rep for --report: a prefix of exactly one long option.
        const auto names = _options.find_abbreviations(key);
        if (names.size() > 1) {
          return error(internal::ambiguous_option_message(key, names));
        }
        if (names.size() == 1) {
          key = names.front();
          option = _options.find_long(key);
        }
      }
      if (option.kind == OptionKind::unknown && _options.reject_unknown) {
        return error("Unknown option '"
                     + std::string(is_double_dash ? "--" : "-") + key + "'.");
      }
      if (eq != std::string_view::npos && !matched_prefix) {
        record_sensitive(option, eq_val);
      }
      bool consume_multi = false;
      bool has_value =
          (eq != std::string_view::npos)
          || (matched_prefix && option.kind == OptionKind::flag);
      if (negated) {
        if (!has_value) {
          val = "false";
        } else {
          val = (eq_val == "0" || eq_val == "false") ? "true" : "false";
        }
      }
      else if (matched_prefix && option.kind == OptionKind::flag) {
        // Value already captured as the suffix after the prefix.
      }
      else if (eq == std::string_view::npos && option.kind == OptionKind::flag
               && !option.is_counter && _options.bool_value_tokens
               && i + 1 < _args.size() && is_bool_literal(_args[i + 1])) {
        val = std::string(_args[++i]);
        has_value = true;
      }
      else if (eq == std::string_view::npos && option.kind == OptionKind::value
               && _options.separate_values) {
        // --key value (rfl::cli::SeparateOptionValues). A flag never takes
        // the next token: `--verbose 0` leaves "0" positional.
        // The message names the option that `key` abbreviates.
        auto v = take_value(
            option, std::string(is_double_dash ? "--" : "-") + key, &i);
        if (!v) { return error(v.error().what()); }
        val = std::move(*v);
        has_value = true;
        consume_multi = option.multi_token;
      }
      else if (eq == std::string_view::npos && option.kind == OptionKind::value
               && _options.require_equals) {
        return error("Missing value for option '"
                     + std::string(is_double_dash ? "--" : "-") + key + "'.");
      }
      else if (eq == std::string_view::npos
               && option.kind == OptionKind::implicit_value) {
        // --help: no value. An rfl::ImplicitValue option stands for its
        // implicit value.
        val = std::string(option.implicit_value);
      }
      const auto r = add_long(key, std::move(val), option, arg_raw, has_value,
                              negated, eq_val, std::move(prefix_suffix));
      if (!r) { return error(r.error().what()); }
      while (consume_multi && i + 1 < _args.size() && _args[i + 1] != "--"
             && !looks_like_option(_args[i + 1])) {
        record_sensitive(option, _args[i + 1]);
        const auto r2 =
            add_long(key, std::string(_args[++i]), option, arg_raw, true, false);
        if (!r2) { return error(r2.error().what()); }
      }
      continue;
    }

    // Short options: -x, -x value, -x=value, multi-character single-dash
    // options (-CC, -rp=paths), and single-char clusters (-qv, -fvalue).
    if (arg_raw.size() >= 2 && arg_raw[0] == '-') {
      const auto group = arg_raw.substr(1);

      // A negative number such as -42 or -.5 is a positional argument,
      // unless, as in argparse, a short option's name looks like a number
      // (-1). Then -42 is a group of short options. Without a table, it is
      // the short option "42".
      if (!looks_like_option(arg_raw) && !negative_numbers_are_options) {
        result.positional.emplace_back(arg_raw);
        force_positional = _options.options_first;
        continue;
      }

      // Check for an exact multi-character single-dash short option (e.g. -CC,
      // -rp, -rp=paths) before splitting character-by-character into clusters.
      const auto full_eq = group.find('=');
      const auto full_key =
          full_eq == std::string_view::npos ? group : group.substr(0, full_eq);
      const bool exact_multi_short =
          _options.group_short_options && full_key.size() > 1
          && _options.find_short(full_key).kind != OptionKind::unknown;

      // Without a table, the short options are not known, so -abc is the one
      // short option "abc": -abc value, -abc=value or -abc.
      for (size_t j = 0; j < group.size(); ++j) {
        // Short names are single ASCII characters (or exact multi-char short
        // names matched above). An unknown character may take several bytes in
        // UTF-8, so take them all for the message.
        const auto c = static_cast<unsigned char>(group[j]);
        auto key = std::string(
            exact_multi_short
                ? full_key
                : (_options.group_short_options
                       ? group.substr(
                             j, c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1)
                       : full_key));
        const auto option = _options.find_short(key);
        const auto rest = group.substr(j + key.size());
        const auto name = "-" + key;
        const bool alone = j == 0 && rest.empty();
        auto val = std::string();

        bool consume_multi = false;
        if (j == 0 && rest.starts_with('=')) {
          // -x=value, whatever -x is. Unlike getopt, where -f=csv gives
          // "=csv", the '=' is not part of the value.
          const auto v_span = rest.substr(1);
          record_sensitive(option, v_span);
          val = std::string(v_span);
        }
        else if (option.kind == OptionKind::unknown) {
          if (!alone) {
            return error("Unknown short argument: " + name + " (in '"
                         + std::string(arg_raw) + "').");
          }
          // Not in `_options`: consume the next token as the value if it
          // doesn't look like an option. resolve_args reports unknown short
          // options.
          if (i + 1 < _args.size() && !looks_like_option(_args[i + 1])) {
            val = std::string(_args[++i]);
          }
        }
        else if (option.kind == OptionKind::flag && !rest.empty()) {
          // -qv: a flag followed by more options. Like getopt, a flag in a
          // group takes no value: -vq=false is an error, -q=false is not.
          if (rest.starts_with('=')) {
            return error("Option '" + name + "' does not take a value in '"
                         + std::string(arg_raw) + "'. Write '" + name
                         + std::string(rest) + "' on its own.");
          }
          const auto r = add_short(std::move(key), "", option, arg_raw);
          if (!r) { return error(r.error().what()); }
          continue;
        }
        else if (!rest.empty()) {
          // -fvalue, -qfvalue, -hvalue: the rest of the token is the value.
          // -qf=value is the same as -qfvalue, as -f=value is.
          const auto v_span = rest.starts_with('=') ? rest.substr(1) : rest;
          record_sensitive(option, v_span);
          val = std::string(v_span);
        }
        else {
          // The last option of the token handles the next one as it would
          // alone: -qf csv takes "csv", -vq false takes "false".
          auto v = take_value(option, name, &i);
          if (!v) { return error(v.error().what()); }
          val = std::move(*v);
          consume_multi = option.multi_token;
        }

        const auto r = add_short(key, std::move(val), option, arg_raw);
        if (!r) { return error(r.error().what()); }
        while (consume_multi && i + 1 < _args.size() && _args[i + 1] != "--"
               && !looks_like_option(_args[i + 1])) {
          record_sensitive(option, _args[i + 1]);
          const auto r2 =
              add_short(key, std::string(_args[++i]), option, arg_raw);
          if (!r2) { return error(r2.error().what()); }
        }
        break;
      }
      continue;
    }

    // Bare argument → positional. With OptionsFirst, every later token is
    // one too, as in getopt's '+' mode.
    result.positional.emplace_back(arg_raw);
    force_positional = _options.options_first;
  }
  return result;
}

/// An element type that a `std::string_view` can point into after the
/// iterator has moved on: an lvalue reference to a string, a pointer, or a
/// `std::string_view`. A temporary `std::string` is not one, because the
/// view would dangle.
template <class Ref>
concept StableStringReference =
    std::convertible_to<Ref, std::string_view> &&
    (std::is_lvalue_reference_v<Ref> || std::is_pointer_v<Ref> ||
     std::same_as<std::remove_cv_t<Ref>, std::string_view>);

/// A range of narrow strings, such as `std::vector<std::string>`,
/// `std::span<const char* const>` or `const char*[]`. The parser keeps a
/// `std::string_view` to every element, so the range must be multi-pass
/// (`std::views::istream` is rejected) and must not yield temporary
/// `std::string`s.
template <class R>
concept StringRange =
    std::ranges::forward_range<R> &&
    StableStringReference<std::ranges::range_reference_t<R>>;

/// A range of wide strings, such as `std::vector<std::wstring>` or
/// `const wchar_t*[]`. Every element is converted to UTF-8 before the next
/// one is read, so single-pass ranges and temporaries are fine here.
template <class R>
concept WideStringRange =
    std::ranges::input_range<R> &&
    std::convertible_to<std::ranges::range_reference_t<R>, std::wstring_view>;

template <class S>
bool is_null(const S& _str) noexcept {
  if constexpr (std::is_pointer_v<S>) {
    return _str == nullptr;
  } else {
    return false;
  }
}

/// Parses every element of `_args`, rejecting null pointers.
/// `_first_index` is the index of the first element in error messages.
template <StringRange R>
rfl::Result<ParsedArgs> parse_strings(
    R&& _args, size_t _first_index, const Options& _options,
    std::vector<std::string_view>* _sensitive_spans = nullptr) {
  std::vector<std::string_view> views;
  auto i = _first_index;
  for (auto&& arg : _args) {
    if (is_null(arg)) {
      return error("Argument " + std::to_string(i) + " is a null pointer.");
    }
    const std::string_view view = arg;
    views.push_back(view);
    ++i;
  }
  return parse_string_views(views, _options, _sensitive_spans);
}

/// Converts every element of `_args` to UTF-8 and parses it.
/// `_first_index` is the index of the first element in error messages.
template <WideStringRange R>
rfl::Result<ParsedArgs> parse_wide_strings(R&& _args, size_t _first_index,
                                           const Options& _options) {
  std::vector<std::string> narrow;
  auto i = _first_index;
  for (auto&& arg : _args) {
    if (is_null(arg)) {
      return error("Argument " + std::to_string(i) + " is a null pointer.");
    }
    const std::wstring_view view = arg;
    auto str = rfl::internal::strings::wstring_to_utf8(std::wstring(view));
    if (!str) {
      return error("Could not convert argument " + std::to_string(i) +
                   " from wide to UTF-8.");
    }
    narrow.emplace_back(std::move(*str));
    ++i;
  }
  return parse_strings(narrow, _first_index, _options);
}

/// Validates argc/argv and returns the arguments after argv[0], the program
/// name.
template <class CharT>
rfl::Result<std::span<const CharT* const>> skip_program_name(
    int argc, const CharT* const argv[]) {
  if (argc < 0 || (argc > 0 && !argv)) {
    return error("Invalid argc/argv.");
  }
  if (argc <= 1) {
    return std::span<const CharT* const>();
  }
  return std::span<const CharT* const>(argv + 1, static_cast<size_t>(argc - 1));
}

/// Parses a sequence of command-line arguments with the options of the
/// target struct, as rfl::cli::read does. Every element is parsed.
/// A negative number such as -5 or -.5 is a positional argument, unless
/// `_options.negative_numbers_are_options` is set: by default, and by
/// make_options if the struct has a short option like rfl::Short<"1", ...>.
/// @param _args The command-line arguments to parse
/// @param _options The options of the target struct (see make_options)
/// @return A Result containing ParsedArgs with categorized arguments or an error
template <class R = std::initializer_list<std::string_view>>
  requires StringRange<R> || WideStringRange<R>
rfl::Result<ParsedArgs> parse_argv(R&& _args, const Options& _options) {
  if constexpr (StringRange<R>) {
    return parse_strings(std::forward<R>(_args), 0, _options);
  } else {
    return parse_wide_strings(std::forward<R>(_args), 0, _options);
  }
}

/// Parses argc/argv with the options of the target struct, skipping argv[0]
/// (the program name).
inline rfl::Result<ParsedArgs> parse_argv(int argc, const char* const argv[],
                                          const Options& _options) {
  return skip_program_name(argc, argv).and_then(
      [&](auto _args) { return parse_strings(_args, 1, _options); });
}

/// Parses mutable argc/argv with the options of the target struct, skipping
/// argv[0] and overwriting any `Option::sensitive` value spans in place with
/// `'*'`.
inline rfl::Result<ParsedArgs> parse_argv(int argc, char* argv[],
                                          const Options& _options) {
  return skip_program_name(argc, static_cast<const char* const*>(argv))
      .and_then([&](auto _args) {
        std::vector<std::string_view> sensitive_spans;
        auto res = parse_strings(_args, 1, _options, &sensitive_spans);
        for (const auto& span : sensitive_spans) {
          auto* p = const_cast<char*>(span.data());
          std::fill(p, p + span.size(), '*');
        }
        return res;
      });
}

/// Parses wide argc/argv with the options of the target struct, skipping
/// argv[0] (the program name). Converts each wide string to UTF-8.
inline rfl::Result<ParsedArgs> parse_argv(int argc, const wchar_t* const argv[],
                                          const Options& _options) {
  return skip_program_name(argc, argv).and_then(
      [&](auto _args) { return parse_wide_strings(_args, 1, _options); });
}

}  // namespace internal

/// Parses a sequence of command-line arguments into categorized buckets,
/// without knowing the target struct.
/// Accepts a range of narrow or wide strings (wide strings are converted
/// to UTF-8) or a braced list of narrow strings. A range of narrow strings
/// must be multi-pass and must not yield temporary `std::string`s, as for
/// rfl::cli::read. Every element is parsed: unlike the argc/argv overloads,
/// the first one is NOT skipped as the program name.
/// Long options never consume the next token, so values must be passed as
/// --key=value. Short options consume it unless it looks like an option. A
/// token such as -5 or -.5 is a short option. resolve_args<T, ProcessorsType>
/// then fixes up what depends on the types of T: bool flags.
/// @param _args The command-line arguments to parse
/// @return A Result containing ParsedArgs with categorized arguments or an error
template <class R = std::initializer_list<std::string_view>>
  requires internal::StringRange<R> || internal::WideStringRange<R>
rfl::Result<ParsedArgs> parse_argv(R&& _args) {
  return internal::parse_argv(std::forward<R>(_args), internal::Options{});
}

/// Parses command-line arguments without knowing the target struct,
/// skipping argv[0] (the program name). Long options never consume the next
/// token, so values must be passed as --key=value. Short options consume it
/// unless it looks like an option. A token such as -5 or -.5 is a short
/// option.
/// @param argc Number of command-line arguments
/// @param argv Array of command-line argument strings
/// @return A Result containing ParsedArgs with categorized arguments or an error
inline rfl::Result<ParsedArgs> parse_argv(int argc, const char* const argv[]) {
  return internal::parse_argv(argc, argv, internal::Options{});
}

inline rfl::Result<ParsedArgs> parse_argv(int argc, char* argv[]) {
  return parse_argv(argc, static_cast<const char* const*>(argv));
}

/// Parses wide-character command-line arguments without knowing the target
/// struct. See the narrow overload.
/// @param argc Number of wide-character command-line arguments
/// @param argv Array of wide-character command-line argument strings
/// @return A Result containing ParsedArgs with UTF-8 encoded arguments
inline rfl::Result<ParsedArgs> parse_argv(int argc,
                                          const wchar_t* const argv[]) {
  return internal::parse_argv(argc, argv, internal::Options{});
}

inline rfl::Result<ParsedArgs> parse_argv(int argc, wchar_t* argv[]) {
  return parse_argv(argc, static_cast<const wchar_t* const*>(argv));
}

}  // namespace rfl::cli

#endif
