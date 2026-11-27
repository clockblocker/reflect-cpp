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
#include "options.hpp"
#include "parse_argv.hpp"
#include "resolve_args.hpp"

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
  std::string negation_prefix = "no";

  DynamicOptions() {
    opts_.negative_numbers_are_options = false;
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
    sync_flags();
    return internal::check_short_names(opts_)
        .and_then([&](bool) {
          return internal::parse_argv(_argc, _argv, opts_);
        })
        .and_then([&](ParsedArgs _parsed) {
          return finish(std::move(_parsed));
        });
  }

  /// Parses `argc`/`argv` (skipping `argv[0]`).
  rfl::Result<DynamicParseResult> parse(int _argc,
                                        const char* const _argv[]) const {
    sync_flags();
    return internal::check_short_names(opts_)
        .and_then([&](bool) {
          return internal::parse_argv(_argc, _argv, opts_);
        })
        .and_then([&](ParsedArgs _parsed) {
          return finish(std::move(_parsed));
        });
  }

  /// Parses a range of argument strings (does NOT skip the first element).
  template <class R = std::initializer_list<std::string_view>>
    requires internal::StringRange<R> || internal::WideStringRange<R>
  rfl::Result<DynamicParseResult> parse(R&& _args) const {
    sync_flags();
    return internal::check_short_names(opts_)
        .and_then([&](bool) {
          return internal::parse_argv(std::forward<R>(_args), opts_);
        })
        .and_then([&](ParsedArgs _parsed) {
          return finish(std::move(_parsed));
        });
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
    for (const auto& m : meta_) {
      oss << "  ";
      if (!m.short_name.empty()) {
        oss << "-" << m.short_name << ", ";
      }
      oss << "--" << m.long_name;
      if (m.kind == internal::OptionKind::value) {
        oss << " arg";
        if (!m.default_hint.empty()) {
          oss << " (=" << m.default_hint << ")";
        }
      } else if (m.kind == internal::OptionKind::implicit_value) {
        oss << " [=arg]";
      }
      if (!m.description.empty()) {
        oss << "  " << m.description;
      }
      oss << "\n";
    }
    return oss.str();
  }

 private:
  struct Meta {
    std::string long_name;
    std::string short_name;
    internal::OptionKind kind = internal::OptionKind::unknown;
    bool required = false;
    std::string description;
    std::string default_hint;
    std::string group;
  };

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
    meta_.push_back(Meta{_long_name, _short_name, _opt.kind, _flags.required,
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
  std::vector<Meta> meta_;
  mutable bool finalized_ = false;
};

}  // namespace rfl::cli

#endif
