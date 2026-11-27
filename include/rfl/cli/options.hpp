#ifndef RFL_CLI_OPTIONS_HPP_
#define RFL_CLI_OPTIONS_HPP_

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../Alias.hpp"
#include "../DefaultHint.hpp"
#include "../DefaultVal.hpp"
#include "../Deprecated.hpp"
#include "../Description.hpp"
#include "../Group.hpp"
#include "../Hidden.hpp"
#include "../ImplicitValue.hpp"
#include "../MultiToken.hpp"
#include "../Nullable.hpp"
#include "../Positional.hpp"
#include "../Rename.hpp"
#include "../Short.hpp"
#include "../TriState.hpp"
#include "../Tuple.hpp"
#include "../ValueName.hpp"
#include "../internal/StringLiteral.hpp"
#include "../internal/TransparentWrapper.hpp"
#include "../internal/field_aliases.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/has_reflector.hpp"
#include "../internal/is_alias.hpp"
#include "../internal/is_extra_fields.hpp"
#include "../internal/is_named_tuple.hpp"
#include "../internal/is_positional.hpp"
#include "../internal/is_short.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "../internal/processed_t.hpp"
#include "../parsing/is_string_map.hpp"
#include "AllowAbbreviations.hpp"
#include "BoolValueTokens.hpp"
#include "LastValueWins.hpp"
#include "Opt.hpp"
#include "OptGroup.hpp"
#include "OptionsFirst.hpp"
#include "Parser.hpp"
#include "PreserveNames.hpp"
#include "PureFlags.hpp"
#include "Reader.hpp"
#include "RequireEquals.hpp"
#include "SeparateOptionValues.hpp"
#include "SingleDashLong.hpp"

namespace rfl::cli::internal {

/// What an option does with the token that follows it.
enum class OptionKind {
  /// A boolean field: `--flag` or `-f`. `--flag` never takes the next
  /// token; `-f` takes it only if it is `true`, `false`, `1` or `0`.
  flag,
  /// A field that takes a value: `--key=value`, `-k value`, `-k=value`, and
  /// with rfl::cli::SeparateOptionValues `--key value`.
  value,
  /// A field wrapped in rfl::ImplicitValue: `--key` and `-k` stand for its
  /// implicit value. A value can only be given after '=', never as the next
  /// token.
  implicit_value,
  /// A known path whose kind cannot be decided from the type (a nested
  /// struct, a variant, rfl::Generic, or a sub-key of a variant). `--key`
  /// never takes the next token, so alone it passes an empty value. A short
  /// alias `-k` takes the next token unless it looks like an option.
  other,
  /// No field matches this option.
  unknown
};

/// How an option is given on the command line.
struct Option {
  OptionKind kind = OptionKind::unknown;

  /// Whether the field is a list (see is_option_list_v). Every
  /// occurrence of a list option adds to it, however the option is spelled,
  /// so it may be repeated. Any other option may be given only once.
  bool is_list = false;

  /// For OptionKind::implicit_value: the value that the option stands for
  /// when it is given without one.
  std::string implicit_value = {};

  /// Whether a list option consumes multiple space-separated tokens after one
  /// occurrence (rfl::MultiToken).
  bool multi_token = false;

  /// Whether the option carries a sensitive value (rfl::cli::attr::Sensitive)
  /// that should be overwritten with `'*'` in mutable `char* argv[]`.
  bool sensitive = false;

  /// Whether the option is an additive counter flag (`-v`, `-vvv` → 1, 3).
  bool is_counter = false;
};

/// The options that a target struct accepts, built once per <T, ProcessorsType>
/// by make_options and cached via cached_options. parse_argv uses it to decide
/// whether an option takes the next token as its value and whether a long
/// option is known, and resolve_args to map short aliases and positional
/// arguments to fields. Paths are dotted (`database.host`) and use the field
/// names after processors (kebab-case, rfl::Rename) are applied.
/// All keyed tables are flat sorted vectors searched with std::string_view via
/// std::lower_bound after finalize().
struct Options {
  /// The long options, sorted by path.
  std::vector<std::pair<std::string, Option>> long_options;

  /// The short aliases given by rfl::Short (`p` for `-p`), sorted by short
  /// name, mapped to the path of their field.
  std::vector<std::pair<std::string, std::string>> short_options;

  /// Alternate long names (`attr::Alias`) mapped to their canonical long path.
  /// Sorted by alias name.
  std::vector<std::pair<std::string, std::string>> long_aliases;

  /// Negated long option names (`--no-foo`, `-nofoo`, `-fno-inline`) mapped to
  /// their canonical long path. Sorted by negated name.
  std::vector<std::pair<std::string, std::string>> negated_options;

  /// Prefix/partial-match options (`-D<def>`, `-I<dir>`, `-Wno-<msg>`,
  /// `-debugi-<src> <lvl>`), sorted longest-prefix-first after `finalize()`.
  std::vector<std::pair<std::string, Option>> prefix_options;

  /// Mutual exclusions `(option, excluded_option)` in canonical long names.
  std::vector<std::pair<std::string, std::string>> exclusions;

  /// Co-dependency requirements `(option, required_option)` in canonical long
  /// names.
  std::vector<std::pair<std::string, std::string>> requirements;

  /// The paths of the rfl::Positional fields, in declaration order, wherever
  /// rfl::Positional sits among a field's wrappers. Each takes one positional
  /// argument, except that a list as the last one takes the remaining
  /// positional arguments (see last_positional_is_list).
  std::vector<std::string> positionals;

  /// Prefixes (ending in path_separator, or empty for the root) under which
  /// the keys are not known in advance, such as maps and rfl::ExtraFields.
  /// Sorted by prefix; maps to the option of a direct sub-key.
  std::vector<std::pair<std::string, Option>> open_prefixes;

  /// For the open prefixes whose sub-keys hold structs or maps: the options
  /// inside one such value, with paths relative to it, so that
  /// `--servers.eu.port` is looked up as `port`. Sorted by prefix.
  std::vector<std::pair<std::string, std::unique_ptr<Options>>>
      open_prefix_options;

  /// Paths of fields wrapped in rfl::Hidden, which are parsed normally but
  /// omitted from usage() and --help. Sorted.
  std::vector<std::string> hidden_options;

  /// Section header name for fields wrapped in rfl::Group, sorted by path.
  std::vector<std::pair<std::string, std::string>> option_groups;

  /// Custom value placeholder name for fields wrapped in rfl::ValueName,
  /// sorted by path.
  std::vector<std::pair<std::string, std::string>> option_value_names;

  /// Whether options that match no field are an error (rfl::NoExtraFields).
  bool reject_unknown = false;

  /// Whether a long option that matches no field may be an abbreviation of
  /// one (rfl::cli::AllowAbbreviations).
  bool allow_abbreviations = false;

  /// Whether short options can be grouped (`-qv`, `-fcsv`). make_options
  /// sets it. Without a table (the table-less parse_argv), `-abc` is the
  /// one short option `abc`.
  bool group_short_options = false;

  /// Whether a long option that takes a value takes it from the next token
  /// when it is given without '=' (rfl::cli::SeparateOptionValues).
  bool separate_values = false;

  /// Returns the cached table with every long option through nested structs
  /// and maps, if this one lists only the top-level fields (see make_options).
  /// parse_argv looks up nested options in it, for instance to tell whether
  /// a repeated nested option is a list.
  const Options& (*all_paths_options)() = nullptr;

  /// Whether tokens such as `-5` and `-.5` are short options rather than
  /// negative numbers. They are when parse_argv is called without the
  /// struct. make_options clears it unless, as in argparse, a short option's
  /// name looks like a number (`-1`).
  bool negative_numbers_are_options = true;

  /// A short name that two fields give with rfl::Short, such as
  /// `rfl::Short<"o", A> a; rfl::Short<"o", B> b;`, or empty. Reading such a
  /// struct fails.
  std::string duplicate_short_name;

  /// Whether the options end at the first positional argument, so that every
  /// later token is a positional argument (rfl::cli::OptionsFirst).
  bool options_first = false;

  /// Whether short boolean flags never consume a following `true`/`false`/`1`/`0`
  /// token (rfl::cli::PureFlags).
  bool pure_flags = false;

  /// Whether repeated non-list options overwrite their previous value instead
  /// of failing (rfl::cli::LastValueWins).
  bool last_value_wins = false;

  /// Whether long value options require `=` when `separate_values` is false
  /// (rfl::cli::RequireEquals).
  bool require_equals = false;

  /// Whether single-dash tokens (`-foo`, `-foo=bar`) are parsed as long
  /// options rather than short option clusters (rfl::cli::SingleDashLong).
  bool single_dash_long = false;

  /// Whether long boolean flags accept a separate following boolean literal
  /// token (`--flag 0`, `--flag 1`, `--flag true`, `--flag false`).
  bool bool_value_tokens = false;

  /// Whether any option accepts `<negation_prefix><name>` negation
  /// automatically (e.g. Bitcoin Core's `-nofoo` with `negation_prefix="no"`).
  bool allow_universal_negation = false;

  /// Prefix used when `allow_universal_negation` is true (default `"no"`).
  std::string negation_prefix = "no";

  /// Whether `OptionKind::value` options consume the next token even when it
  /// begins with `'-'` (e.g. `-CFLAGS -O2`, matching POSIX `getopt_long`).
  bool allow_option_like_values = false;

  /// Records one leaf long option along with its optional hidden/group/value_name
  /// metadata. Non-template to avoid per-field template instantiations.
  void add_leaf_option(std::string _path, const Option& _opt,
                       const bool _hidden, const std::string_view _group,
                       const std::string_view _value_name) {
    if (_hidden) {
      hidden_options.push_back(_path);
    }
    if (!_group.empty()) {
      option_groups.emplace_back(_path, std::string(_group));
    }
    if (!_value_name.empty()) {
      option_value_names.emplace_back(_path, std::string(_value_name));
    }
    long_options.emplace_back(std::move(_path), _opt);
  }

  /// Records a short alias (and tracks the first duplicate, if any).
  void add_short_alias(const std::string_view _short_name,
                       const std::string_view _long_name) {
    const bool already_exists = std::any_of(
        short_options.begin(), short_options.end(),
        [&](const auto& _entry) { return _entry.first == _short_name; });
    if (already_exists) {
      if (duplicate_short_name.empty()) {
        duplicate_short_name = std::string(_short_name);
      }
    } else {
      short_options.emplace_back(std::string(_short_name),
                                 std::string(_long_name));
    }
  }

  /// Records a long option alias (e.g. `directory` → `dir`).
  void add_long_alias(const std::string_view _alias,
                      const std::string_view _long_name) {
    if (!_alias.empty() && _alias != _long_name) {
      long_aliases.emplace_back(std::string(_alias), std::string(_long_name));
    }
  }

  void add_exclusion(const std::string_view _option,
                     const std::string_view _excluded) {
    if (!_option.empty() && !_excluded.empty()) {
      exclusions.emplace_back(std::string(_option), std::string(_excluded));
    }
  }

  void add_requirement(const std::string_view _option,
                       const std::string_view _needed) {
    if (!_option.empty() && !_needed.empty()) {
      requirements.emplace_back(std::string(_option), std::string(_needed));
    }
  }

  /// Records a negated long option name (e.g. `no-pyc` → `pyc`).
  void add_negated_alias(const std::string_view _negated_name,
                         const std::string_view _long_name) {
    if (!_negated_name.empty() && _negated_name != _long_name) {
      negated_options.emplace_back(std::string(_negated_name),
                                   std::string(_long_name));
    }
  }

  /// Records a prefix/partial-match option (e.g. `D`, `Wno-`, `debugi-`).
  void add_prefix_option(std::string _prefix, const Option& _opt) {
    if (!_prefix.empty()) {
      prefix_options.emplace_back(std::move(_prefix), _opt);
    }
  }

  /// Sorts all lookup tables so binary search lookups work. Must be called
  /// after populating or mutating any keyed table.
  void finalize() {
    sort_pairs(long_options);
    sort_pairs(short_options);
    sort_pairs(long_aliases);
    sort_pairs(negated_options);
    std::stable_sort(prefix_options.begin(), prefix_options.end(),
                     [](const auto& _a, const auto& _b) {
                       if (_a.first.size() != _b.first.size()) {
                         return _a.first.size() > _b.first.size();
                       }
                       return _a.first < _b.first;
                     });
    sort_pairs(open_prefixes);
    sort_pairs(open_prefix_options);
    for (auto& entry : open_prefix_options) {
      entry.second->finalize();
    }
    std::sort(hidden_options.begin(), hidden_options.end());
    hidden_options.erase(
        std::unique(hidden_options.begin(), hidden_options.end()),
        hidden_options.end());
    sort_pairs(option_groups);
    sort_pairs(option_value_names);
  }

  /// Returns the long options that `--_prefix` could abbreviate: every
  /// long option that starts with `_prefix`, in alphabetical order. Only the
  /// last segment of a dotted name is abbreviated, so `_prefix` must spell
  /// out the others and end inside the last one: `database.h` abbreviates
  /// `database.host`, but `data` and `database.` do not. Keys of maps and of
  /// rfl::ExtraFields are not known in advance, so they are never among
  /// them. Only meaningful if find_long(_prefix) is unknown: an exact match
  /// always wins.
  std::vector<std::string> find_abbreviations(
      const std::string_view _prefix
  ) const {
    std::vector<std::string> names;
    if (_prefix.empty() || _prefix.back() == path_separator) {
      return names;
    }
    const auto last_separator = _prefix.rfind(path_separator);
    for (auto it = lower_bound_pair(long_options, _prefix);
         it != long_options.end() && it->first.starts_with(_prefix); ++it) {
      if (it->first.rfind(path_separator) == last_separator) {
        names.push_back(it->first);
      }
    }
    for (auto it = lower_bound_pair(long_aliases, _prefix);
         it != long_aliases.end() && it->first.starts_with(_prefix); ++it) {
      if (it->first.rfind(path_separator) == last_separator
          && std::find(names.begin(), names.end(), it->second) == names.end()) {
        names.push_back(it->second);
      }
    }
    std::sort(names.begin(), names.end());
    return names;
  }

  /// Resolves a long option alias to its canonical long option name, or nullptr.
  const std::string* find_long_alias(const std::string_view _name) const {
    return find_pair(long_aliases, _name);
  }

  /// Resolves a negated option name (e.g. `no-pyc`) to its canonical long
  /// option name, or nullptr.
  const std::string* find_negated_option(const std::string_view _name) const {
    return find_pair(negated_options, _name);
  }

  /// Finds the longest registered prefix option matching `_name`, or nullptr.
  const std::pair<std::string, Option>* find_prefix_option(
      const std::string_view _name) const {
    for (const auto& entry : prefix_options) {
      if (_name.starts_with(entry.first)) {
        return &entry;
      }
    }
    return nullptr;
  }

  /// Returns the long option `--_name`.
  Option find_long(const std::string_view _name) const {
    if (const auto* opt = find_pair(long_options, _name)) {
      return *opt;
    }

    // A key below an open leaf field (for example a variant of structs): the
    // reader decides what it means.
    for (auto pos = _name.find(path_separator); pos != std::string_view::npos;
         pos = _name.find(path_separator, pos + 1)) {
      if (const auto* parent = find_pair(long_options, _name.substr(0, pos));
          parent && parent->kind == OptionKind::other) {
        return Option{OptionKind::other};
      }
    }

    // A key inside a map or a struct with rfl::ExtraFields. The longest
    // matching prefix wins.
    const std::pair<std::string, Option>* open = nullptr;
    for (const auto& p : open_prefixes) {
      if (_name.size() > p.first.size() && _name.starts_with(p.first)
          && (!open || p.first.size() > open->first.size())) {
        open = &p;
      }
    }
    if (open) {
      const auto rest = _name.substr(open->first.size());
      const auto sep = rest.find(path_separator);
      if (sep != std::string_view::npos) {
        // A dotted key whose leading segment is a known field (nested struct,
        // map, or scalar, e.g. `--database.nope` or `--port.foo` beside
        // rfl::ExtraFields) belongs to that field, not to ExtraFields.
        const auto head = open->first + std::string(rest.substr(0, sep));
        const auto head_prefix = head + path_separator;
        if (find_pair(long_options, head) != nullptr
            || has_prefix(long_options, head_prefix)
            || has_prefix(open_prefixes, head_prefix)) {
          return Option{};
        }
      }
      if (sep == std::string_view::npos) {
        return open->second;
      }
      if (const auto* nested = find_pair(open_prefix_options, open->first)) {
        return (*nested)->find_long(rest.substr(sep + 1));
      }
      return open->second.kind == OptionKind::other ? Option{OptionKind::other}
                                                    : Option{};
    }

    return Option{};
  }

  /// Returns the long option path that `-_name` stands for, or nullptr.
  const std::string* find_short_target(const std::string_view _name) const {
    return find_pair(short_options, _name);
  }

  /// Returns the option that the short alias `-_name` stands for.
  Option find_short(const std::string_view _name) const {
    const auto* target = find_short_target(_name);
    return target ? find_long(*target) : Option{};
  }

  /// Whether `_path` is hidden from usage() and --help.
  bool is_hidden(const std::string_view _path) const {
    const auto it =
        std::lower_bound(hidden_options.begin(), hidden_options.end(), _path);
    return it != hidden_options.end() && *it == _path;
  }

  /// Returns the rfl::Group name for `_path`, or empty string_view if none.
  std::string_view find_group(const std::string_view _path) const {
    const auto* g = find_pair(option_groups, _path);
    return g ? std::string_view(*g) : std::string_view{};
  }

  /// Returns the rfl::ValueName for `_path`, or empty string_view if none.
  std::string_view find_value_name(const std::string_view _path) const {
    const auto* vn = find_pair(option_value_names, _path);
    return vn ? std::string_view(*vn) : std::string_view{};
  }

  /// Whether the last rfl::Positional field is a list. It then takes every
  /// positional argument after those of the other positional fields.
  bool last_positional_is_list() const {
    return !positionals.empty() && find_long(positionals.back()).is_list;
  }

 private:
  template <class V>
  static void sort_pairs(std::vector<std::pair<std::string, V>>& _vec) {
    std::stable_sort(_vec.begin(), _vec.end(),
                     [](const auto& _a, const auto& _b) {
                       return _a.first < _b.first;
                     });
  }

  template <class V>
  static typename std::vector<std::pair<std::string, V>>::const_iterator
  lower_bound_pair(const std::vector<std::pair<std::string, V>>& _vec,
                   const std::string_view _key) {
    return std::lower_bound(
        _vec.begin(), _vec.end(), _key,
        [](const auto& _entry, const std::string_view _k) {
          return std::string_view(_entry.first) < _k;
        });
  }

  template <class V>
  static const V* find_pair(
      const std::vector<std::pair<std::string, V>>& _vec,
      const std::string_view _key) {
    const auto it = lower_bound_pair(_vec, _key);
    return (it != _vec.end() && it->first == _key) ? &it->second : nullptr;
  }

  static bool has_prefix(
      const std::vector<std::pair<std::string, Option>>& _vec,
      const std::string_view _prefix) {
    const auto it = lower_bound_pair(_vec, _prefix);
    return it != _vec.end() && it->first.starts_with(_prefix);
  }
};

/// Returns true if the token is one of the values that a bool flag given
/// without '=' takes from the next token: "true", "false", "1" or "0".
/// @param _token The command-line token to check
/// @return true if it is a boolean literal, false otherwise
inline bool is_bool_literal(std::string_view _token) noexcept {
  return _token == "true" || _token == "false" || _token == "1"
         || _token == "0";
}

/// What rfl::Short, rfl::Positional and rfl::ImplicitValue say about a field
/// of type T. They are found through each other and through rfl::DefaultVal,
/// std::optional, rfl::Description, rfl::Deprecated and rfl::Rename, in any
/// order. `count` is the number of rfl::Short and rfl::Positional wrappers; a
/// field may have at most one.
template <class T>
struct cli_annotations {
  static constexpr bool is_short = false;
  static constexpr bool is_positional = false;
  static constexpr bool is_hidden = false;
  static constexpr bool has_group = false;
  static constexpr bool has_value_name = false;
  static constexpr bool is_multi_token = false;
  static constexpr bool is_sensitive = false;
  static constexpr bool is_counter = false;
  static constexpr size_t num_implicit_value = 0;
  static constexpr rfl::internal::StringLiteral<1> short_name = "";
  static constexpr int count = 0;
  static constexpr rfl::internal::StringLiteral<1> implicit_value = "";
  static constexpr rfl::internal::StringLiteral<1> group_name = "";
  static constexpr rfl::internal::StringLiteral<1> value_name = "";

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

template <class T>
using cli_annotations_t = cli_annotations<std::remove_cvref_t<T>>;

template <class T>
struct cli_annotations<DefaultVal<T>> : cli_annotations_t<T> {};

template <class T>
struct cli_annotations<std::optional<T>> : cli_annotations_t<T> {};

template <class T>
struct cli_annotations<Nullable<T>> : cli_annotations_t<T> {};

template <class T>
struct cli_annotations<TriState<T>> : cli_annotations_t<T> {};

template <rfl::internal::StringLiteral _description, class T>
struct cli_annotations<Description<_description, T>> : cli_annotations_t<T> {};

template <rfl::internal::StringLiteral _deprecation_message,
          rfl::internal::StringLiteral _description, class T>
struct cli_annotations<Deprecated<_deprecation_message, _description, T>>
    : cli_annotations_t<T> {};

template <rfl::internal::StringLiteral _name, class T>
struct cli_annotations<Rename<_name, T>> : cli_annotations_t<T> {};

template <rfl::internal::StringLiteral _name, class T>
struct cli_annotations<Alias<_name, T>> : cli_annotations_t<T> {
  template <class Fn>
  static void for_each_alias(Fn&& _fn) {
    _fn(_name.string_view());
    cli_annotations_t<T>::for_each_alias(_fn);
  }
};

template <class T>
struct cli_annotations<Positional<T>> : cli_annotations_t<T> {
  static constexpr bool is_short = false;
  static constexpr bool is_positional = true;
  static constexpr int count = cli_annotations_t<T>::count + 1;
};

template <rfl::internal::StringLiteral _name, class T>
struct cli_annotations<Short<_name, T>> : cli_annotations_t<T> {
  static constexpr bool is_short = true;
  static constexpr bool is_positional = false;
  static constexpr auto short_name = _name;
  static constexpr int count = cli_annotations_t<T>::count + 1;
};

template <class T, rfl::internal::StringLiteral _implicit>
struct cli_annotations<ImplicitValue<T, _implicit>> : cli_annotations_t<T> {
  static constexpr size_t num_implicit_value =
      cli_annotations_t<T>::num_implicit_value + 1;
  static constexpr auto implicit_value = _implicit;
};

template <class T>
struct cli_annotations<Hidden<T>> : cli_annotations_t<T> {
  static constexpr bool is_hidden = true;
};

template <rfl::internal::StringLiteral _group, class T>
struct cli_annotations<Group<_group, T>> : cli_annotations_t<T> {
  static constexpr bool has_group = true;
  static constexpr auto group_name = _group;
};

template <rfl::internal::StringLiteral _value_name, class T>
struct cli_annotations<ValueName<_value_name, T>> : cli_annotations_t<T> {
  static constexpr bool has_value_name = true;
  static constexpr auto value_name = _value_name;
};

template <class T>
struct cli_annotations<MultiToken<T>> : cli_annotations_t<T> {
  static constexpr bool is_multi_token = true;
};

template <rfl::internal::StringLiteral _hint, class T>
struct cli_annotations<DefaultHint<_hint, T>> : cli_annotations_t<T> {};

template <class T, class... Attrs>
struct cli_annotations<Opt<T, Attrs...>> {
  using Inner = cli_annotations_t<T>;
  using Info = typename Opt<T, Attrs...>::AttrsInfo;

  static constexpr bool is_short = Info::is_short || Inner::is_short;
  static constexpr bool is_positional =
      Info::is_positional || Inner::is_positional;
  static constexpr bool is_hidden = Info::is_hidden || Inner::is_hidden;
  static constexpr bool has_group = Info::has_group || Inner::has_group;
  static constexpr bool has_value_name =
      Info::has_value_name || Inner::has_value_name;
  static constexpr bool is_multi_token =
      Info::is_multi_token || Inner::is_multi_token;
  static constexpr bool is_sensitive =
      Info::is_sensitive || Inner::is_sensitive;
  static constexpr bool is_counter =
      Info::is_counter || Inner::is_counter;
  static constexpr size_t num_implicit_value =
      Info::num_implicit_value + Inner::num_implicit_value;
  static constexpr int count = Info::count + Inner::count;

  template <class Fn>
  static void for_each_alias(Fn&& _fn) {
    Info::for_each_alias(_fn);
    Inner::for_each_alias(_fn);
  }
  template <class Fn>
  static void for_each_excludes(Fn&& _fn) {
    Info::for_each_excludes(_fn);
    Inner::for_each_excludes(_fn);
  }
  template <class Fn>
  static void for_each_needs(Fn&& _fn) {
    Info::for_each_needs(_fn);
    Inner::for_each_needs(_fn);
  }
  template <class Fn>
  static void for_each_negated_prefix(Fn&& _fn) {
    Info::for_each_negated_prefix(_fn);
    Inner::for_each_negated_prefix(_fn);
  }
  template <class Fn>
  static void for_each_negated_name(Fn&& _fn) {
    Info::for_each_negated_name(_fn);
    Inner::for_each_negated_name(_fn);
  }

  static constexpr auto short_name = []() {
    if constexpr (Info::is_short) {
      return Info::short_name;
    } else {
      return Inner::short_name;
    }
  }();
  static constexpr auto implicit_value = []() {
    if constexpr (Info::num_implicit_value > 0) {
      return Info::implicit_value;
    } else {
      return Inner::implicit_value;
    }
  }();
  static constexpr auto group_name = []() {
    if constexpr (Info::has_group) {
      return Info::group_name;
    } else {
      return Inner::group_name;
    }
  }();
  static constexpr auto value_name = []() {
    if constexpr (Info::has_value_name) {
      return Info::value_name;
    } else {
      return Inner::value_name;
    }
  }();
};

/// The CLI metadata of the _i-th field of a NamedTuple. InnerType is the
/// declared type of the field, whichever wrappers it consists of.
template <class NamedTupleType, size_t _i>
struct field_info {
  using FieldType = rfl::tuple_element_t<_i, typename NamedTupleType::Fields>;
  using InnerType = typename FieldType::Type;
  using OuterType = std::remove_pointer_t<std::remove_cvref_t<InnerType>>;
  using Annotations = cli_annotations_t<OuterType>;

  static constexpr bool is_positional = Annotations::is_positional;
  static constexpr bool is_short = Annotations::is_short;
  static constexpr bool is_hidden = Annotations::is_hidden;
  static constexpr bool has_implicit_value =
      Annotations::num_implicit_value > 0;

  static_assert(Annotations::count <= 1,
                "A field can have only one rfl::Short or rfl::Positional, "
                "wherever it sits among the field's other wrappers.");
  static_assert(Annotations::num_implicit_value <= 1,
                "A field can have at most one rfl::ImplicitValue.");
  static_assert(!is_positional || !has_implicit_value,
                "A field cannot be both rfl::Positional and "
                "rfl::ImplicitValue.");

  static constexpr std::string_view name() { return FieldType::name(); }
};

/// The short name of the _i-th field, or an empty string if it has none.
template <class NamedTupleType, size_t _i>
constexpr auto get_short_name() {
  using Info = field_info<NamedTupleType, _i>;
  if constexpr (Info::is_short) {
    return Info::Annotations::short_name;
  }
  else {
    return rfl::internal::StringLiteral<1>("");
  }
}

/// Strips everything that does not change how a field is written on the
/// command line: optionals, pointers, rfl::Short, rfl::Positional,
/// rfl::ImplicitValue, rfl::Rename, rfl::DefaultVal, rfl::Skip, types with a
/// ReflectionType (rfl::Validator, rfl::Description, rfl::Literal, ...),
/// reflectors and custom parsers.
template <class T, class ProcessorsType>
auto unwrap_option_type() {
  using U = std::remove_cvref_t<T>;
  if constexpr (std::is_pointer_v<U>) {
    return unwrap_option_type<std::remove_pointer_t<U>, ProcessorsType>();
  }
  else if constexpr (parsing::is_optional_v<U>
                     || rfl::internal::is_nullable_wrapper_v<U>
                     || rfl::internal::is_tri_state_v<U>) {
    return unwrap_option_type<typename U::value_type, ProcessorsType>();
  }
  else if constexpr (parsing::is_unique_ptr_v<U>
                     || parsing::is_shared_ptr_v<U>) {
    return unwrap_option_type<typename U::element_type, ProcessorsType>();
  }
  else if constexpr (parsing::is_box_v<U>) {
    return unwrap_option_type<
        typename parsing::is_box<U>::element_type, ProcessorsType>();
  }
  else if constexpr (parsing::is_ref_v<U>) {
    return unwrap_option_type<
        typename parsing::is_ref<U>::element_type, ProcessorsType>();
  }
  else if constexpr (rfl::internal::is_short_v<U>
                     || rfl::internal::is_positional_v<U>
                     || rfl::internal::is_alias_v<U>
                     || rfl::internal::is_transparent_wrapper_v<U>
                     || parsing::is_rename_v<U>
                     || parsing::is_default_val_v<U>
                     || parsing::is_skip_v<U>) {
    return unwrap_option_type<typename U::Type, ProcessorsType>();
  }
  else if constexpr (rfl::internal::has_read_reflector<U>) {
    return unwrap_option_type<
        typename Reflector<U>::ReflType, ProcessorsType>();
  }
  else if constexpr (rfl::internal::has_reflection_type_v<U>) {
    return unwrap_option_type<typename U::ReflectionType, ProcessorsType>();
  }
  else if constexpr (requires {
                       typename Parser<U, ProcessorsType>::
                           CustomParserHelperStruct;
                     }) {
    return unwrap_option_type<
        typename Parser<U, ProcessorsType>::CustomParserHelperStruct,
        ProcessorsType>();
  }
  else {
    return std::type_identity<U>{};
  }
}

template <class T, class ProcessorsType>
using unwrapped_option_t =
    typename decltype(unwrap_option_type<T, ProcessorsType>())::type;

/// Whether the reader treats T as a struct whose fields are addressed as
/// `--field.sub-field`.
template <class T>
constexpr bool is_option_struct_v =
    (std::is_class_v<T> && std::is_aggregate_v<T> && !parsing::is_array_v<T>
     && !parsing::is_rfl_array_v<T>)
    || rfl::internal::is_named_tuple_v<T>;

/// Whether an option for T may or may not need a value, depending on what
/// it holds: a variant or rfl::Generic can be a bool or a struct. Such an
/// option never takes the next token.
template <class T>
constexpr bool is_open_option_v =
    parsing::is_variant_v<T> || parsing::is_rfl_variant_v<T>
    || parsing::is_tagged_union_v<T>;

/// The NamedTuple whose fields are the options of the struct T: T itself if
/// it is an rfl::NamedTuple (whose names the processors leave alone), or T
/// with the processors applied.
template <class T, class ProcessorsType>
using option_fields_t = typename std::conditional_t<
    rfl::internal::is_named_tuple_v<T>, std::type_identity<T>,
    rfl::internal::Processed<T, ProcessorsType>>::type;

template <class... Ts>
struct type_list {
  template <class T>
  static constexpr bool contains = (std::is_same_v<T, Ts> || ...);

  template <class T>
  using add = type_list<Ts..., T>;
};

/// Whether the reader reads T as a list, through Reader::to_array: vectors,
/// sets, deques and the like, std::array, rfl::Array, C arrays, tuples and
/// pairs. Maps are not lists, even those whose keys are not strings. The
/// value of a list option is split into items at the list delimiter, and
/// repeating the option adds the items of every occurrence, so
/// `--x=a --x=b` is `--x=a,b`.
template <class T>
constexpr bool is_option_list_v =
    (parsing::is_vector_like_v<T> && !parsing::is_map_like_v<T>
     && !parsing::is_bytestring_v<T> && !parsing::is_vectorstring_v<T>)
    || parsing::is_array_v<T> || parsing::is_rfl_array_v<T>
    || parsing::is_c_array_v<T> || parsing::is_tuple_v<T>
    || parsing::is_rfl_tuple_v<T> || parsing::is_pair_v<T>;

/// Whether an empty implicit value means something for T: a bool flag is
/// true, a string is empty, a list has no items.
template <class T>
constexpr bool empty_implicit_value_is_valid_v =
    std::is_same_v<T, bool> || std::is_same_v<T, std::string>
    || is_option_list_v<T>;

/// The option for a field of type T wrapped in rfl::ImplicitValue.
template <class T, class ProcessorsType>
Option implicit_value_option() {
  using U = unwrapped_option_t<T, ProcessorsType>;
  using Annotations = cli_annotations_t<T>;
  static_assert(Annotations::num_implicit_value <= 1,
                "A field can have at most one rfl::ImplicitValue.");
  static_assert(!parsing::is_string_map_v<U> && !is_option_struct_v<U>,
                "rfl::ImplicitValue must wrap a field that takes a value, not "
                "a struct or a map.");
  static_assert(empty_implicit_value_is_valid_v<U>
                    || Annotations::implicit_value.length > 0,
                "rfl::ImplicitValue needs an implicit value unless it wraps a "
                "bool, a std::string or a list, such as "
                "rfl::ImplicitValue<int, \"1\">.");
  return Option{OptionKind::implicit_value, is_option_list_v<U>,
                std::string(Annotations::implicit_value.string_view()), false,
                Annotations::is_sensitive};
}

/// The option for a field, or a key of a map or of rfl::ExtraFields, with
/// values of type T.
template <class T, class ProcessorsType>
Option option_for() {
  using U = unwrapped_option_t<T, ProcessorsType>;
  static_assert(!cli_annotations_t<T>::is_multi_token || is_option_list_v<U>,
                "rfl::MultiToken can only wrap a list type (such as std::vector).");
  if constexpr (cli_annotations_t<T>::is_counter) {
    static_assert(std::is_integral_v<U> && !std::is_same_v<U, bool>,
                  "rfl::cli::attr::Counter must wrap an integral type (not bool).");
    Option opt;
    opt.kind = OptionKind::flag;
    opt.is_counter = true;
    return opt;
  }
  else if constexpr (cli_annotations_t<T>::num_implicit_value > 0) {
    return implicit_value_option<T, ProcessorsType>();
  }
  else if constexpr (std::is_same_v<U, bool>) {
    return Option{OptionKind::flag};
  }
  else if constexpr (parsing::is_string_map_v<U> || is_option_struct_v<U>
                     || is_open_option_v<U>) {
    return Option{OptionKind::other};
  }
  else {
    return Option{OptionKind::value, is_option_list_v<U>, {},
                  cli_annotations_t<T>::is_multi_token,
                  cli_annotations_t<T>::is_sensitive};
  }
}

template <class T, class ProcessorsType, class VisitedType>
void collect_options(const std::string& _prefix, Options* _options);

template <class T, class ProcessorsType, class VisitedType>
void collect_value_options(Options* _options);

template <class T, class ProcessorsType, class VisitedType>
void collect_opt_groups(const std::string& _prefix, Options* _options);

/// Adds the open prefix `_prefix` of a map or of rfl::ExtraFields with
/// values of type T. If the values are structs or maps, also adds the
/// options inside one of them.
template <class T, class ProcessorsType, class VisitedType>
void add_open_prefix(const std::string& _prefix, Options* _options) {
  using U = unwrapped_option_t<T, ProcessorsType>;
  _options->open_prefixes.emplace_back(_prefix, option_for<T, ProcessorsType>());
  if constexpr (cli_annotations_t<T>::num_implicit_value == 0
                && (parsing::is_string_map_v<U>
                    || (is_option_struct_v<U>
                        && !VisitedType::template contains<U>))) {
    auto nested = std::make_unique<Options>();
    collect_value_options<T, ProcessorsType, VisitedType>(nested.get());
    _options->open_prefix_options.emplace_back(_prefix, std::move(nested));
  }
}

/// Adds the options inside one value of type T to `_options`, with paths
/// relative to it: the fields of a struct, or the keys of a map.
template <class T, class ProcessorsType, class VisitedType>
void collect_value_options(Options* _options) {
  using U = unwrapped_option_t<T, ProcessorsType>;
  if constexpr (parsing::is_string_map_v<U>) {
    add_open_prefix<typename U::mapped_type, ProcessorsType, VisitedType>(
        "", _options);
  }
  else if constexpr (is_option_struct_v<U>) {
    collect_options<U, ProcessorsType, VisitedType>("", _options);
    collect_opt_groups<U, ProcessorsType, VisitedType>("", _options);
  }
  else {
    _options->open_prefixes.emplace_back("", Option{OptionKind::other});
  }
}

template <class T, class ProcessorsType, class VisitedType>
void collect_alias_paths(const std::string& _alias_prefix,
                         const std::string& _canonical_prefix,
                         Options* _options) {
  using NT = option_fields_t<T, ProcessorsType>;
  using NextVisitedType = typename VisitedType::template add<T>;
  [&]<size_t... _is>(std::index_sequence<_is...>) {
    auto add_one = [&]<size_t _i>() {
      using FieldType = rfl::tuple_element_t<_i, typename NT::Fields>;
      using FT = std::remove_cvref_t<typename FieldType::Type>;
      if constexpr (!rfl::internal::is_extra_fields_v<FT>) {
        using U = unwrapped_option_t<FT, ProcessorsType>;
        using Ann = cli_annotations_t<FT>;
        const std::string field_name(FieldType::name());
        const std::string alias_path = _alias_prefix + field_name;
        const std::string canonical_path = _canonical_prefix + field_name;
        if constexpr (is_option_struct_v<U> && !is_open_option_v<U>
                      && !parsing::is_string_map_v<U>
                      && !NextVisitedType::template contains<U>) {
          collect_alias_paths<U, ProcessorsType, NextVisitedType>(
              alias_path + path_separator, canonical_path + path_separator,
              _options);
          Ann::for_each_alias([&](const std::string_view _alias) {
            collect_alias_paths<U, ProcessorsType, NextVisitedType>(
                _alias_prefix + std::string(_alias) + path_separator,
                canonical_path + path_separator, _options);
          });
        } else {
          _options->add_long_alias(alias_path, canonical_path);
          Ann::for_each_alias([&](const std::string_view _alias) {
            _options->add_long_alias(_alias_prefix + std::string(_alias),
                                     canonical_path);
          });
        }
      }
    };
    (add_one.template operator()<_is>(), ...);
  }(std::make_index_sequence<NT::size()>());
}

template <class T, class ProcessorsType, class VisitedType>
void collect_options(const std::string& _prefix, Options* _options) {
  using NT = option_fields_t<T, ProcessorsType>;
  static_assert(
      rfl::internal::no_alias_collisions<typename NT::Fields>(),
      "Field alias collides with another field's name or alias.");
  using NextVisitedType = typename VisitedType::template add<T>;
  [&]<size_t... _is>(std::index_sequence<_is...>) {
    auto add_one = [&]<size_t _i>() {
      using FieldType = rfl::tuple_element_t<_i, typename NT::Fields>;
      using FT = std::remove_cvref_t<typename FieldType::Type>;
      if constexpr (rfl::internal::is_extra_fields_v<FT>) {
        add_open_prefix<typename FT::mapped_type, ProcessorsType,
                        NextVisitedType>(_prefix, _options);
      } else {
        using U = unwrapped_option_t<FT, ProcessorsType>;
        using Ann = cli_annotations_t<FT>;
        std::string path = _prefix + std::string(FieldType::name());
        Ann::for_each_alias([&](const std::string_view _alias) {
          _options->add_long_alias(
              _prefix.empty() ? std::string(_alias)
                              : _prefix + std::string(_alias),
              path);
        });
        Ann::for_each_excludes([&](const std::string_view _other) {
          _options->add_exclusion(path, _other);
        });
        Ann::for_each_needs([&](const std::string_view _other) {
          _options->add_requirement(path, _other);
        });
        Ann::for_each_negated_prefix([&](const std::string_view _neg_prefix) {
          _options->add_negated_alias(
              _prefix + std::string(_neg_prefix) + std::string(FieldType::name()),
              path);
        });
        Ann::for_each_negated_name([&](const std::string_view _neg_name) {
          _options->add_negated_alias(
              _prefix.empty() ? std::string(_neg_name)
                              : _prefix + std::string(_neg_name),
              path);
        });
        constexpr bool is_hidden = Ann::is_hidden;
        constexpr auto group_name =
            Ann::has_group ? Ann::group_name.string_view() : std::string_view{};
        constexpr auto value_name =
            Ann::has_value_name ? Ann::value_name.string_view()
                                : std::string_view{};
        if constexpr (Ann::is_counter) {
          static_assert(
              std::is_integral_v<U> && !std::is_same_v<U, bool>,
              "rfl::cli::attr::Counter must wrap an integral type (not bool).");
          Option opt;
          opt.kind = OptionKind::flag;
          opt.is_counter = true;
          _options->add_leaf_option(std::move(path), opt, is_hidden, group_name,
                                    value_name);
        } else if constexpr (Ann::num_implicit_value > 0) {
          _options->add_leaf_option(
              std::move(path), implicit_value_option<FT, ProcessorsType>(),
              is_hidden, group_name, value_name);
        } else if constexpr (std::is_same_v<U, bool>) {
          _options->add_leaf_option(std::move(path), Option{OptionKind::flag},
                                    is_hidden, group_name, value_name);
        } else if constexpr (is_open_option_v<U>) {
          _options->add_leaf_option(std::move(path), Option{OptionKind::other},
                                    is_hidden, group_name, value_name);
        } else if constexpr (parsing::is_string_map_v<U>) {
          if constexpr (is_hidden) {
            _options->hidden_options.push_back(path);
          }
          if constexpr (!group_name.empty()) {
            _options->option_groups.emplace_back(path, std::string(group_name));
          }
          if constexpr (!value_name.empty()) {
            _options->option_value_names.emplace_back(path,
                                                      std::string(value_name));
          }
          add_open_prefix<typename U::mapped_type, ProcessorsType,
                          NextVisitedType>(path + path_separator, _options);
        } else if constexpr (is_option_struct_v<U>) {
          if constexpr (is_hidden) {
            _options->hidden_options.push_back(path);
          }
          if constexpr (NextVisitedType::template contains<U>) {
            _options->open_prefixes.emplace_back(path + path_separator,
                                                 Option{OptionKind::other});
          } else {
            collect_options<U, ProcessorsType, NextVisitedType>(
                path + path_separator, _options);
            collect_opt_groups<U, ProcessorsType, NextVisitedType>(
                path + path_separator, _options);
            Ann::for_each_alias([&](const std::string_view _alias) {
              collect_alias_paths<U, ProcessorsType, NextVisitedType>(
                  _prefix + std::string(_alias) + path_separator,
                  path + path_separator, _options);
            });
          }
        } else {
          static_assert(
              !Ann::is_multi_token || is_option_list_v<U>,
              "rfl::MultiToken can only wrap a list type (such as std::vector).");
          _options->add_leaf_option(
              std::move(path),
              Option{OptionKind::value, is_option_list_v<U>, {},
                     Ann::is_multi_token, Ann::is_sensitive},
              is_hidden, group_name, value_name);
        }
      }
    };
    (add_one.template operator()<_is>(), ...);
  }(std::make_index_sequence<NT::size()>());
}

/// Assigns `_group` to every leaf option inside `Sub` that does not already
/// have an explicit group in `_options->option_groups`.
template <class Sub, class ProcessorsType, class VisitedType>
void apply_inherited_group(const std::string& _prefix,
                           const std::string_view _group, Options* _options) {
  using SubNT = option_fields_t<Sub, ProcessorsType>;
  using NextVisited = typename VisitedType::template add<Sub>;
  const auto set_group_if_unset = [&](const std::string& _path) {
    const bool already_set = std::any_of(
        _options->option_groups.begin(), _options->option_groups.end(),
        [&](const auto& _entry) { return _entry.first == _path; });
    if (!already_set) {
      _options->option_groups.emplace_back(_path, std::string(_group));
    }
  };
  [&]<size_t... _is>(std::index_sequence<_is...>) {
    auto apply_one = [&]<size_t _i>() {
      using FieldType = rfl::tuple_element_t<_i, typename SubNT::Fields>;
      using FT = std::remove_cvref_t<typename FieldType::Type>;
      if constexpr (!rfl::internal::is_extra_fields_v<FT>) {
        using U = unwrapped_option_t<FT, ProcessorsType>;
        const std::string path = _prefix + std::string(FieldType::name());
        if constexpr (is_option_struct_v<U> && !is_open_option_v<U>
                      && !parsing::is_string_map_v<U>
                      && !NextVisited::template contains<U>) {
          apply_inherited_group<U, ProcessorsType, NextVisited>(
              path + path_separator, _group, _options);
        } else {
          set_group_if_unset(path);
        }
      }
    };
    (apply_one.template operator()<_is>(), ...);
  }(std::make_index_sequence<SubNT::size()>());
}

/// Walks the unflattened members of `T` to propagate `rfl::cli::OptGroup`
/// section names across flattened sub-structs (inner groups take precedence
/// over outer groups; explicit per-field groups take precedence over both).
template <class T, class ProcessorsType, class VisitedType>
void collect_opt_groups(const std::string& _prefix, Options* _options) {
  using RawT = std::remove_cvref_t<T>;
  if constexpr (!is_option_struct_v<RawT>
                || rfl::internal::is_named_tuple_v<RawT>
                || VisitedType::template contains<RawT>) {
    return;
  } else {
    using NextVisited = typename VisitedType::template add<RawT>;
    using PtrTuple = rfl::internal::ptr_tuple_t<RawT>;
    [&]<size_t... _is>(std::index_sequence<_is...>) {
      auto visit = [&]<size_t _i>() {
        using MemberType = std::remove_cvref_t<
            std::remove_pointer_t<rfl::tuple_element_t<_i, PtrTuple>>>;
        if constexpr (is_opt_group_v<MemberType>) {
          using Sub = typename MemberType::Type;
          // Recurse first so inner OptGroups take precedence over outer ones.
          collect_opt_groups<Sub, ProcessorsType, NextVisited>(_prefix,
                                                               _options);
          apply_inherited_group<Sub, ProcessorsType, NextVisited>(
              _prefix, MemberType::group_name_.string_view(), _options);
        } else if constexpr (rfl::internal::is_flatten_field_v<MemberType>) {
          using Sub = typename MemberType::Type;
          collect_opt_groups<Sub, ProcessorsType, NextVisited>(_prefix,
                                                               _options);
        }
      };
      (visit.template operator()<_is>(), ...);
    }(std::make_index_sequence<rfl::tuple_size_v<PtrTuple>>());
  }
}

template <class T, class ProcessorsType>
void collect_short_and_positional(Options* _options) {
  using NT = option_fields_t<T, ProcessorsType>;
  [&]<size_t... _is>(std::index_sequence<_is...>) {
    auto add_one = [&]<size_t _i>() {
      using Info = field_info<NT, _i>;
      if constexpr (Info::is_short) {
        constexpr auto short_lit = get_short_name<NT, _i>();
        _options->add_short_alias(short_lit.string_view(), Info::name());
      }
      if constexpr (Info::is_positional) {
        _options->positionals.emplace_back(Info::name());
      }
    };
    (add_one.template operator()<_is>(), ...);
  }(std::make_index_sequence<NT::size()>());
}

/// Adds the options of the top-level fields of T, which is all that short
/// options need.
template <class T, class ProcessorsType>
void collect_top_level_options(Options* _options) {
  using NT = option_fields_t<T, ProcessorsType>;
  static_assert(
      rfl::internal::no_alias_collisions<typename NT::Fields>(),
      "Field alias collides with another field's name or alias.");
  [&]<size_t... _is>(std::index_sequence<_is...>) {
    auto add_one = [&]<size_t _i>() {
      using FieldType = rfl::tuple_element_t<_i, typename NT::Fields>;
      using FT = std::remove_cvref_t<typename FieldType::Type>;
      if constexpr (!rfl::internal::is_extra_fields_v<FT>) {
        using Ann = cli_annotations_t<FT>;
        const auto name = FieldType::name();
        Ann::for_each_alias([&](const std::string_view _alias) {
          _options->add_long_alias(_alias, name);
        });
        Ann::for_each_excludes([&](const std::string_view _other) {
          _options->add_exclusion(name, _other);
        });
        Ann::for_each_needs([&](const std::string_view _other) {
          _options->add_requirement(name, _other);
        });
        Ann::for_each_negated_prefix([&](const std::string_view _neg_prefix) {
          _options->add_negated_alias(
              std::string(_neg_prefix) + std::string(name), name);
        });
        Ann::for_each_negated_name([&](const std::string_view _neg_name) {
          _options->add_negated_alias(_neg_name, name);
        });
        _options->long_options.emplace_back(std::string(name),
                                            option_for<FT, ProcessorsType>());
      }
    };
    (add_one.template operator()<_is>(), ...);
  }(std::make_index_sequence<NT::size()>());
}

template <class T, class ProcessorsType,
          bool _all_paths = rfl::internal::no_extra_fields_v<ProcessorsType>
                            || separate_option_values_v<ProcessorsType>
                            || allow_abbreviations_v<ProcessorsType>>
const Options& cached_options();

/// Derives the options that T accepts from its fields: the long options,
/// with whether each is a flag, takes a value or has an implicit value, and
/// the rfl::Short aliases and rfl::Positional fields of the top-level struct.
/// @tparam T The target struct type
/// @tparam ProcessorsType The processors applied to the struct
/// @tparam _all_paths Whether to list every long option through nested
/// structs, maps and rfl::ExtraFields, rather than only the top-level fields.
/// Only rfl::NoExtraFields, to reject unknown options,
/// rfl::cli::SeparateOptionValues, to tell whether a nested option takes the
/// next token, and rfl::cli::AllowAbbreviations, to abbreviate nested
/// options, need them all.
/// @return The finalized options table
template <class T, class ProcessorsType,
          bool _all_paths = rfl::internal::no_extra_fields_v<ProcessorsType>
                            || separate_option_values_v<ProcessorsType>
                            || allow_abbreviations_v<ProcessorsType>>
Options make_options() {
  Options options;
  options.group_short_options = true;
  options.reject_unknown = rfl::internal::no_extra_fields_v<ProcessorsType>;
  options.separate_values = separate_option_values_v<ProcessorsType>;
  if constexpr (!_all_paths) {
    options.all_paths_options = &cached_options<T, ProcessorsType, true>;
  }
  options.allow_abbreviations = allow_abbreviations_v<ProcessorsType>;
  options.options_first = options_first_v<ProcessorsType>;
  options.pure_flags = pure_flags_v<ProcessorsType>;
  options.last_value_wins = last_value_wins_v<ProcessorsType>;
  options.require_equals = require_equals_v<ProcessorsType>;
  options.single_dash_long = single_dash_long_v<ProcessorsType>;
  options.bool_value_tokens = bool_value_tokens_v<ProcessorsType>;
  using U = unwrapped_option_t<T, ProcessorsType>;
  if constexpr (_all_paths) {
    collect_value_options<T, ProcessorsType, type_list<>>(&options);
  }
  else if constexpr (is_option_struct_v<U>) {
    collect_top_level_options<U, ProcessorsType>(&options);
  }
  if constexpr (is_option_struct_v<U>) {
    collect_short_and_positional<U, ProcessorsType>(&options);
  }
  // As in argparse, -5 is a negative number unless a short option's name
  // looks like a number (-1).
  options.negative_numbers_are_options = false;
  for (const auto& [name, path] : options.short_options) {
    if (!name.empty()
        && ((name[0] >= '0' && name[0] <= '9') || name[0] == '.')) {
      options.negative_numbers_are_options = true;
    }
  }
  options.finalize();
  return options;
}

/// Returns a process-lifetime cached Options table for
/// <T, ProcessorsType, _all_paths>, avoiding table construction and sorting
/// on every read() call.
template <class T, class ProcessorsType, bool _all_paths>
const Options& cached_options() {
  static const Options options = make_options<T, ProcessorsType, _all_paths>();
  return options;
}

}  // namespace rfl::cli::internal

#endif
