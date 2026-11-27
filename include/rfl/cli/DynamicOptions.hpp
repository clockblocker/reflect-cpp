#ifndef RFL_CLI_DYNAMICOPTIONS_HPP_
#define RFL_CLI_DYNAMICOPTIONS_HPP_

#include <cctype>
#include <initializer_list>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../Result.hpp"
#include "options.hpp"
#include "parse_argv.hpp"

namespace rfl::cli {

/// Flags controlling how a dynamically registered option behaves.
struct DynamicOptionFlags {
  /// Mask option argument bytes in place with '*' when parsing mutable argv.
  bool sensitive = false;
  /// Option may be repeated and every occurrence is retained.
  bool is_list = false;
  /// Option consumes multiple consecutive non-option tokens.
  bool multi_token = false;
};

/// Result returned by `DynamicOptions::parse(...)`.
struct DynamicParseResult {
  /// Every recognized option occurrence `(canonical_long_name, value)` in exact
  /// argv order.
  std::vector<std::pair<std::string, std::string>> occurrences;
  /// All positional (non-option) arguments in argv order.
  std::vector<std::string> positional;
  /// Effective (last) value for each recognized canonical long option name.
  std::map<std::string, std::string> values;
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

  DynamicOptions() {
    opts_.negative_numbers_are_options = false;
  }

  /// Registers a boolean flag option (`--name`, optional `-s`).
  DynamicOptions& add_flag(std::string _long_name,
                           std::string _short_name = {}) {
    internal::Option opt;
    opt.kind = internal::OptionKind::flag;
    add_entry(std::move(_long_name), std::move(_short_name), opt);
    return *this;
  }

  /// Registers an option taking a required value (`--name=val` / `--name val`).
  DynamicOptions& add_value(std::string _long_name,
                            std::string _short_name = {},
                            const DynamicOptionFlags _flags = {}) {
    internal::Option opt;
    opt.kind = internal::OptionKind::value;
    opt.is_list = _flags.is_list;
    opt.multi_token = _flags.multi_token;
    opt.sensitive = _flags.sensitive;
    add_entry(std::move(_long_name), std::move(_short_name), opt);
    return *this;
  }

  /// Registers an option taking an optional value (`--name` uses
  /// `_implicit_value`; `--name=val` uses `val`).
  DynamicOptions& add_optional_value(std::string _long_name,
                                     std::string _implicit_value = {},
                                     std::string _short_name = {},
                                     const DynamicOptionFlags _flags = {}) {
    internal::Option opt;
    opt.kind = internal::OptionKind::implicit_value;
    opt.implicit_value = std::move(_implicit_value);
    opt.is_list = _flags.is_list;
    opt.multi_token = _flags.multi_token;
    opt.sensitive = _flags.sensitive;
    add_entry(std::move(_long_name), std::move(_short_name), opt);
    return *this;
  }

  /// Finalizes internal sorted lookup tables so repeated `parse()` calls do
  /// not re-sort. Called automatically on first `parse()` if not called first.
  void finalize() const {
    if (finalized_) {
      return;
    }
    opts_.separate_values = separate_values;
    opts_.allow_abbreviations = allow_abbreviations;
    opts_.pure_flags = pure_flags;
    opts_.last_value_wins = last_value_wins;
    opts_.require_equals = require_equals;
    opts_.options_first = options_first;
    opts_.reject_unknown = reject_unknown;
    opts_.group_short_options = group_short_options;
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

 private:
  void add_entry(std::string _long_name, std::string _short_name,
                 const internal::Option& _opt) {
    finalized_ = false;
    if (!_short_name.empty()) {
      const auto first = static_cast<unsigned char>(_short_name[0]);
      if (std::isdigit(first) || _short_name[0] == '.') {
        opts_.negative_numbers_are_options = true;
      }
      opts_.add_short_alias(_short_name, _long_name);
    }
    opts_.long_options.emplace_back(std::move(_long_name), _opt);
  }

  void sync_flags() const {
    if (!finalized_) {
      finalize();
    } else {
      opts_.separate_values = separate_values;
      opts_.allow_abbreviations = allow_abbreviations;
      opts_.pure_flags = pure_flags;
      opts_.last_value_wins = last_value_wins;
      opts_.require_equals = require_equals;
      opts_.options_first = options_first;
      opts_.reject_unknown = reject_unknown;
      opts_.group_short_options = group_short_options;
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
    out.positional = std::move(_parsed.positional);
    out.values = std::move(_parsed.named);
    for (auto& [short_key, val] : _parsed.short_args) {
      if (const auto* target = opts_.find_short_target(short_key)) {
        out.values[*target] = std::move(val);
      }
    }
    return out;
  }

  mutable internal::Options opts_;
  mutable bool finalized_ = false;
};

}  // namespace rfl::cli

#endif
