#ifndef RFL_CLI_DYNAMICOPTIONS_HPP_
#define RFL_CLI_DYNAMICOPTIONS_HPP_

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <istream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../Result.hpp"
#include "Settings.hpp"
#include "options.hpp"
#include "parse_argv.hpp"
#include "resolve_args.hpp"
#include "suggest.hpp"

namespace rfl::cli {

/// Flags controlling how a dynamically registered option behaves and renders.
struct DynamicOptionFlags {
  /// Mask option argument bytes in place with '*' when parsing mutable argv.
  bool sensitive = false;
  /// Option may be repeated and every occurrence is retained.
  bool is_list = false;
  /// Option consumes multiple consecutive non-option tokens.
  bool multi_token = false;
  /// Option must be provided (via CLI or merged config file).
  bool required = false;
  /// Option is hidden from `usage()` and typo `suggest()` candidates.
  bool hidden = false;
  /// Help description shown by `DynamicOptions::usage()`.
  std::string description = {};
  /// Display-only default value hint shown by `DynamicOptions::usage()`.
  std::string default_hint = {};
  /// Optional section/group name shown by `DynamicOptions::usage()`.
  std::string group = {};
};

/// Result returned by `DynamicOptions::parse(...)`.
struct DynamicParseResult {
  /// Every recognized option occurrence `(canonical_long_name, value)` in exact
  /// argv order.
  std::vector<std::pair<std::string, std::string>> occurrences;
  /// Rich per-occurrence metadata (`name`, `value`, `has_value`, `negated`,
  /// `raw`) in exact argv order.
  std::vector<OptionOccurrence> detailed_occurrences;
  /// All positional (non-option) arguments in argv order.
  std::vector<std::string> positional;
  /// Effective (last) value for each recognized canonical long option name.
  std::map<std::string, std::string> values;
};

/// Result returned by `DynamicOptions::parse_ini(...)`.
struct DynamicIniResult {
  /// Recognized INI entries `(canonical_long_name, value)` in file order.
  std::vector<std::pair<std::string, std::string>> occurrences;
  /// Effective (last) value per recognized canonical long option name.
  std::map<std::string, std::string> values;
  /// Keys present in the INI stream that were not registered (populated when
  /// `allow_unregistered` is true).
  std::vector<std::string> unregistered;
};

/// Runtime option table for CLIs whose option schema is constructed dynamically
/// (e.g. plugin-registered or handler-factory CLIs) rather than from a single
/// static C++ struct. Uses the same argv parser engine as `rfl::cli::read`.
class DynamicOptions {
 public:
  bool separate_values = true;
  bool allow_abbreviations = false;
  bool pure_flags = true;
  bool last_value_wins = true;
  bool require_equals = false;
  bool options_first = false;
  bool reject_unknown = true;
  bool group_short_options = true;
  bool single_dash_long = false;
  bool bool_value_tokens = false;
  bool allow_universal_negation = false;
  bool negative_numbers_are_options = false;
  bool allow_option_like_values = false;
  std::string negation_prefix = "no";

  DynamicOptions() {
    opts_.negative_numbers_are_options = false;
  }

  /// Applies a consolidated `rfl::cli::Settings` configuration to this dynamic
  /// option table.
  DynamicOptions& apply_settings(const Settings& _settings) {
    separate_values = _settings.separate_values;
    allow_abbreviations = _settings.allow_abbreviations;
    pure_flags = _settings.pure_flags;
    last_value_wins = _settings.last_value_wins;
    require_equals = _settings.require_equals;
    options_first = _settings.options_first;
    reject_unknown = _settings.reject_unknown;
    single_dash_long = _settings.single_dash_long;
    bool_value_tokens = _settings.bool_value_tokens;
    allow_universal_negation = _settings.allow_universal_negation;
    negation_prefix = _settings.negation_prefix;
    allow_option_like_values = _settings.allow_option_like_values;
    return *this;
  }

  /// Read-only view of registered dynamic option metadata (used by
  /// `CommandNode::describe()`).
  struct OptionMeta {
    std::string long_name;
    std::string short_name;
    OptionKind kind = OptionKind::unknown;
    bool is_list = false;
    bool is_counter = false;
    bool multi_token = false;
    bool sensitive = false;
    std::string implicit_value;
    bool required = false;
    bool hidden = false;
    std::string description;
    std::string default_hint;
    std::string group;
  };

  const std::vector<OptionMeta>& registered_options() const noexcept {
    return meta_;
  }

  const internal::Options& compiled_options() const {
    sync_flags();
    return opts_;
  }

  /// Registers a boolean flag option (`--name`, optional `-s`).
  DynamicOptions& add_flag(std::string _long_name,
                           std::string _short_name = {},
                           const DynamicOptionFlags& _flags = {}) {
    internal::Option opt;
    opt.kind = internal::OptionKind::flag;
    add_entry(std::move(_long_name), std::move(_short_name), opt, _flags);
    return *this;
  }

  /// Registers a repeatable counter flag (`-v`, `-vvv`, `--verbose` → 1, 3, 1).
  DynamicOptions& add_counter(std::string _long_name,
                              std::string _short_name = {},
                              const DynamicOptionFlags& _flags = {}) {
    internal::Option opt;
    opt.kind = internal::OptionKind::flag;
    opt.is_counter = true;
    add_entry(std::move(_long_name), std::move(_short_name), opt, _flags);
    return *this;
  }

  /// Registers an option taking a required value (`--name=val` / `--name val`).
  DynamicOptions& add_value(std::string _long_name,
                            std::string _short_name = {},
                            const DynamicOptionFlags& _flags = {}) {
    internal::Option opt;
    opt.kind = internal::OptionKind::value;
    opt.is_list = _flags.is_list;
    opt.multi_token = _flags.multi_token;
    opt.sensitive = _flags.sensitive;
    add_entry(std::move(_long_name), std::move(_short_name), opt, _flags);
    return *this;
  }

  /// Registers an option taking an optional value (`--name` uses
  /// `_implicit_value`; `--name=val` uses `val`).
  DynamicOptions& add_optional_value(std::string _long_name,
                                     std::string _implicit_value = {},
                                     std::string _short_name = {},
                                     const DynamicOptionFlags& _flags = {}) {
    internal::Option opt;
    opt.kind = internal::OptionKind::implicit_value;
    opt.implicit_value = std::move(_implicit_value);
    opt.is_list = _flags.is_list;
    opt.multi_token = _flags.multi_token;
    opt.sensitive = _flags.sensitive;
    add_entry(std::move(_long_name), std::move(_short_name), opt, _flags);
    return *this;
  }

  /// Registers an alternate long option name mapping to `_canonical_long_name`.
  DynamicOptions& add_alias(std::string _alias,
                            std::string _canonical_long_name) {
    finalized_ = false;
    opts_.add_long_alias(_alias, _canonical_long_name);
    return *this;
  }

  /// Registers a negated option name (e.g. `no-pyc` → `pyc`, `fno-inline` →
  /// `finline`).
  DynamicOptions& add_negated_alias(std::string _negated_name,
                                    std::string _canonical_long_name) {
    finalized_ = false;
    opts_.add_negated_alias(_negated_name, _canonical_long_name);
    return *this;
  }

  /// Declares that `--_option` cannot be used with `--_excluded`.
  DynamicOptions& add_exclusion(std::string _option, std::string _excluded) {
    opts_.add_exclusion(_option, _excluded);
    return *this;
  }

  /// Declares that `--_option` requires `--_needed`.
  DynamicOptions& add_requirement(std::string _option, std::string _needed) {
    opts_.add_requirement(_option, _needed);
    return *this;
  }

  /// Registers a single-token prefix-matched option (e.g. `D` matching `-DFOO=1`,
  /// `Wno-` matching `-Wno-fatal`, `O` matching `-O3`). The matched suffix is
  /// captured in `OptionOccurrence::prefix_suffix` and `OptionOccurrence::value`.
  DynamicOptions& add_prefix_flag(std::string _prefix,
                                  const DynamicOptionFlags& _flags = {}) {
    return add_prefix_entry(std::move(_prefix), internal::OptionKind::flag,
                            _flags);
  }

  /// Registers a prefix-matched option that also consumes a value argument
  /// (e.g. `debugi-` matching `-debugi-V3Options 3`). The suffix is captured in
  /// `OptionOccurrence::prefix_suffix` and the argument in `OptionOccurrence::value`.
  DynamicOptions& add_prefix_value(std::string _prefix,
                                   const DynamicOptionFlags& _flags = {}) {
    return add_prefix_entry(std::move(_prefix), internal::OptionKind::value,
                            _flags);
  }

  /// Adds an extra suggestion candidate string (e.g. `-fno-inline` or custom
  /// non-dash arguments) considered by `suggest()`.
  DynamicOptions& add_suggestion_candidate(std::string _candidate) {
    extra_candidates_.push_back(std::move(_candidate));
    return *this;
  }

  /// Suggests the closest registered (non-hidden) option for `_unknown`, or
  /// `std::nullopt` if no candidate meets `_min_similarity`.
  std::optional<std::string> suggest(const std::string_view _unknown,
                                     const double _min_similarity = 0.6) const {
    const std::string prefix = single_dash_long ? "-" : "--";
    std::vector<std::string> hidden_names;
    for (const auto& m : meta_) {
      if (m.hidden) {
        hidden_names.push_back(m.long_name);
      }
    }
    std::sort(hidden_names.begin(), hidden_names.end());
    const auto is_hidden = [&](const std::string& _name) {
      return std::binary_search(hidden_names.begin(), hidden_names.end(), _name);
    };

    std::vector<std::string> candidates;
    candidates.reserve(meta_.size() + opts_.long_aliases.size()
                       + opts_.negated_options.size() + extra_candidates_.size());
    for (const auto& m : meta_) {
      if (!m.hidden) {
        candidates.push_back(prefix + m.long_name);
      }
    }
    for (const auto& [alias, target] : opts_.long_aliases) {
      if (!is_hidden(target)) {
        candidates.push_back(prefix + alias);
      }
    }
    for (const auto& [neg, target] : opts_.negated_options) {
      if (!is_hidden(target)) {
        candidates.push_back(prefix + neg);
      }
    }
    for (const auto& extra : extra_candidates_) {
      candidates.push_back(extra);
    }
    const auto matches = suggest_closest(_unknown, candidates, _min_similarity, 1);
    if (matches.empty()) {
      return std::nullopt;
    }
    return matches.front();
  }

  /// Returns true if `_long_name` (or alias) is registered as a value option.
  bool is_value_option(const std::string_view _long_name) const {
    sync_flags();
    std::string_view target = _long_name;
    if (const auto* canonical = opts_.find_long_alias(target)) {
      target = *canonical;
    }
    return opts_.find_long(target).kind == internal::OptionKind::value;
  }

  /// Returns true if `_short_name` is registered as a value option.
  bool is_short_value_option(const std::string_view _short_name) const {
    sync_flags();
    return opts_.find_short(_short_name).kind == internal::OptionKind::value;
  }

  /// Finalizes internal sorted lookup tables so repeated `parse()` calls do
  /// not re-sort. Called automatically on first `parse()` if not called first.
  void finalize() const {
    if (finalized_) {
      return;
    }
    sync_flags_impl();
    opts_.finalize();
    finalized_ = true;
  }

  /// Parses `argc`/`argv` (skipping `argv[0]`). Masks sensitive option values
  /// in place with `'*'` in `argv`.
  rfl::Result<DynamicParseResult> parse(int _argc, char* _argv[]) const {
    return parse_impl([&] { return internal::parse_argv(_argc, _argv, opts_); });
  }

  /// Parses `argc`/`argv` (skipping `argv[0]`).
  rfl::Result<DynamicParseResult> parse(int _argc,
                                        const char* const _argv[]) const {
    return parse_impl([&] { return internal::parse_argv(_argc, _argv, opts_); });
  }

  /// Parses a range of argument strings (does NOT skip the first element).
  template <class R = std::initializer_list<std::string_view>>
    requires internal::StringRange<R> || internal::WideStringRange<R>
  rfl::Result<DynamicParseResult> parse(R&& _args) const {
    return parse_impl(
        [&] { return internal::parse_argv(std::forward<R>(_args), opts_); });
  }

  /// Parses a standard INI configuration stream (`[Section]` + `key = value`)
  /// against registered options (dotted keys `Section.key`).
  rfl::Result<DynamicIniResult> parse_ini(
      std::istream& _in, const bool _allow_unregistered = false) const {
    sync_flags();
    DynamicIniResult out;
    std::string section;
    std::string line;
    size_t line_no = 0;
    const auto trim = [](std::string_view _sv) -> std::string_view {
      while (!_sv.empty()
             && std::isspace(static_cast<unsigned char>(_sv.front()))) {
        _sv.remove_prefix(1);
      }
      while (!_sv.empty()
             && std::isspace(static_cast<unsigned char>(_sv.back()))) {
        _sv.remove_suffix(1);
      }
      return _sv;
    };
    while (std::getline(_in, line)) {
      ++line_no;
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      const auto sv = trim(line);
      if (sv.empty() || sv.front() == '#' || sv.front() == ';') {
        continue;
      }
      if (sv.front() == '[') {
        const auto close = sv.find(']');
        if (close == std::string_view::npos) {
          return error("INI syntax error at line " + std::to_string(line_no)
                       + ": missing ']'.");
        }
        section = std::string(trim(sv.substr(1, close - 1)));
        continue;
      }
      const auto eq = sv.find('=');
      if (eq == std::string_view::npos) {
        return error("INI syntax error at line " + std::to_string(line_no)
                     + ": expected 'key = value'.");
      }
      const auto raw_key = trim(sv.substr(0, eq));
      const auto raw_val = trim(sv.substr(eq + 1));
      std::string full_key =
          section.empty() ? std::string(raw_key)
                          : section + "." + std::string(raw_key);
      if (const auto* canonical = opts_.find_long_alias(full_key)) {
        full_key = *canonical;
      }
      const auto opt = opts_.find_long(full_key);
      if (opt.kind == internal::OptionKind::unknown) {
        if (!_allow_unregistered) {
          return error("Unrecognized option '" + full_key + "'.");
        }
        out.unregistered.push_back(std::move(full_key));
        continue;
      }
      out.occurrences.emplace_back(full_key, std::string(raw_val));
      out.values[full_key] = std::string(raw_val);
    }
    return out;
  }

  /// Checks that every option registered with `required = true` is present in
  /// `_values` (e.g. after merging CLI values over INI config values).
  rfl::Result<bool> check_required(
      const std::map<std::string, std::string>& _values) const {
    for (const auto& entry : meta_) {
      if (entry.required && !_values.count(entry.long_name)) {
        return error("Missing required option '--" + entry.long_name + "'.");
      }
    }
    return true;
  }

  /// Formats a human-readable help string for all registered dynamic options.
  std::string usage(const std::string_view _header = {}) const {
    std::ostringstream oss;
    if (!_header.empty()) {
      oss << _header << "\n";
    }
    const auto format_entry = [&](const OptionMeta& _m) {
      oss << "  ";
      if (!_m.short_name.empty()) {
        oss << "-" << _m.short_name << ", ";
      }
      oss << (single_dash_long ? "-" : "--") << _m.long_name;
      if (_m.kind == internal::OptionKind::value) {
        oss << " arg";
        if (!_m.default_hint.empty()) {
          oss << " (=" << _m.default_hint << ")";
        }
      } else if (_m.kind == internal::OptionKind::implicit_value) {
        oss << " [=arg]";
      }
      if (!_m.description.empty()) {
        oss << "  " << _m.description;
      }
      oss << "\n";
    };

    const bool has_groups = std::any_of(
        meta_.begin(), meta_.end(),
        [](const OptionMeta& _m) { return !_m.hidden && !_m.group.empty(); });
    if (!has_groups) {
      for (const auto& m : meta_) {
        if (!m.hidden) {
          format_entry(m);
        }
      }
      return oss.str();
    }

    std::vector<std::string> group_order;
    for (const auto& m : meta_) {
      if (m.hidden) {
        continue;
      }
      if (std::find(group_order.begin(), group_order.end(), m.group)
          == group_order.end()) {
        group_order.push_back(m.group);
      }
    }
    bool first_group = true;
    for (const auto& g : group_order) {
      if (!first_group) {
        oss << "\n";
      }
      first_group = false;
      if (!g.empty()) {
        oss << g << ":\n";
      }
      for (const auto& m : meta_) {
        if (!m.hidden && m.group == g) {
          format_entry(m);
        }
      }
    }
    return oss.str();
  }

 private:
  DynamicOptions& add_prefix_entry(std::string _prefix,
                                   const internal::OptionKind _kind,
                                   const DynamicOptionFlags& _flags) {
    finalized_ = false;
    internal::Option opt;
    opt.kind = _kind;
    opt.is_list = true;
    opt.sensitive = _flags.sensitive;
    meta_.push_back(OptionMeta{_prefix, {}, opt.kind, true, false, false,
                               _flags.sensitive, {}, _flags.required,
                               _flags.hidden, _flags.description,
                               _flags.default_hint, _flags.group});
    opts_.add_prefix_option(std::move(_prefix), opt);
    return *this;
  }

  template <class ParseFn>
  rfl::Result<DynamicParseResult> parse_impl(ParseFn&& _parse_fn) const {
    sync_flags();
    return internal::check_short_names(opts_)
        .and_then([&](bool) { return _parse_fn(); })
        .and_then([&](ParsedArgs _parsed) { return finish(std::move(_parsed)); });
  }

  void add_entry(std::string _long_name, std::string _short_name,
                 const internal::Option& _opt,
                 const DynamicOptionFlags& _flags) {
    finalized_ = false;
    if (!_short_name.empty()) {
      const auto first = static_cast<unsigned char>(_short_name[0]);
      if (std::isdigit(first) || _short_name[0] == '.') {
        opts_.negative_numbers_are_options = true;
      }
      opts_.add_short_alias(_short_name, _long_name);
    }
    meta_.push_back(OptionMeta{_long_name, _short_name, _opt.kind, _opt.is_list,
                               _opt.is_counter, _opt.multi_token,
                               _opt.sensitive, _opt.implicit_value,
                               _flags.required, _flags.hidden,
                               _flags.description, _flags.default_hint,
                               _flags.group});
    opts_.long_options.emplace_back(std::move(_long_name), _opt);
  }

  void sync_flags_impl() const {
    opts_.separate_values = separate_values;
    opts_.allow_abbreviations = allow_abbreviations;
    opts_.pure_flags = pure_flags;
    opts_.last_value_wins = last_value_wins;
    opts_.require_equals = require_equals;
    opts_.options_first = options_first;
    opts_.reject_unknown = reject_unknown;
    opts_.group_short_options = group_short_options;
    opts_.single_dash_long = single_dash_long;
    opts_.bool_value_tokens = bool_value_tokens;
    opts_.allow_universal_negation = allow_universal_negation;
    opts_.negation_prefix = negation_prefix;
    opts_.allow_option_like_values = allow_option_like_values;
    if (negative_numbers_are_options) {
      opts_.negative_numbers_are_options = true;
    }
  }

  void sync_flags() const {
    if (!finalized_) {
      finalize();
    } else {
      sync_flags_impl();
    }
  }

  rfl::Result<DynamicParseResult> finish(ParsedArgs _parsed) const {
    if (opts_.reject_unknown) {
      for (const auto& [short_key, _] : _parsed.short_args) {
        if (opts_.find_short(short_key).kind == internal::OptionKind::unknown) {
          return error("Unknown short argument: -" + short_key + ".");
        }
      }
    }
    DynamicParseResult out;
    out.occurrences = std::move(_parsed.occurrences);
    out.detailed_occurrences = std::move(_parsed.detailed_occurrences);
    out.positional = std::move(_parsed.positional);
    out.values = std::move(_parsed.named);
    for (auto& [short_key, val] : _parsed.short_args) {
      if (const auto* target = opts_.find_short_target(short_key)) {
        out.values[*target] = std::move(val);
      }
    }
    if (const auto ok = internal::check_option_constraints(out.values, opts_);
        !ok) {
      return error(ok.error().what());
    }
    return out;
  }

  mutable internal::Options opts_;
  std::vector<OptionMeta> meta_;
  std::vector<std::string> extra_candidates_;
  mutable bool finalized_ = false;
};

}  // namespace rfl::cli

#endif
