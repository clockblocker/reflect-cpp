#ifndef RFL_CLI_OPTIONS_HPP_
#define RFL_CLI_OPTIONS_HPP_

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../DefaultVal.hpp"
#include "../Deprecated.hpp"
#include "../Description.hpp"
#include "../Generic.hpp"
#include "../ImplicitValue.hpp"
#include "../Positional.hpp"
#include "../Rename.hpp"
#include "../Short.hpp"
#include "../Tuple.hpp"
#include "../internal/StringLiteral.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/has_reflector.hpp"
#include "../internal/is_extra_fields.hpp"
#include "../internal/is_named_tuple.hpp"
#include "../internal/is_positional.hpp"
#include "../internal/is_short.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "../internal/processed_t.hpp"
#include "../parsing/is_string_map.hpp"
#include "NoAbbreviations.hpp"
#include "OptionsFirst.hpp"
#include "Parser.hpp"
#include "Reader.hpp"

namespace rfl::cli::internal {

/// What an option does with the token that follows it.
enum class OptionKind {
  /// A boolean field: `--flag` or `-f`. `--flag` never takes the next
  /// token; `-f` takes it only if it is `true`, `false`, `1` or `0`.
  flag,
  /// A field that takes a value: `--key value`, `--key=value`, `-k value` or
  /// `-k=value`.
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

  /// For OptionKind::implicit_value: the value that the option stands for
  /// when it is given without one.
  std::string implicit_value = {};

  /// Whether the field is a list (see is_option_list_v). Every
  /// occurrence of a list option adds to it, however the option is spelled,
  /// so it may be repeated. Any other option may be given only once.
  bool is_list = false;
};

/// The options that a target struct accepts, built from its fields by
/// make_options on every read. parse_argv uses it to decide whether an
/// option takes the next token as its value, and resolve_args to map short
/// aliases and positional arguments to fields. Paths are dotted
/// (`database.host`) and use the field names after processors (kebab-case,
/// rfl::Rename) are applied.
struct Options {
  /// The long options, by path.
  std::map<std::string, Option> long_options;

  /// The short aliases given by rfl::Short (`p` for `-p`), mapped to the path
  /// of their field.
  std::map<std::string, std::string> short_options;

  /// The paths of the rfl::Positional fields: first those whose outermost
  /// wrapper is rfl::Positional, then those where it is found through other
  /// wrappers, each in declaration order. Each takes one positional
  /// argument, except that a list as the last one takes the remaining
  /// positional arguments (see last_positional_is_list).
  std::vector<std::string> positionals;

  /// Prefixes (ending in path_separator, or empty for the root) under which
  /// the keys are not known in advance, such as maps and rfl::ExtraFields.
  /// Maps to the option of a direct sub-key.
  std::map<std::string, Option> open_prefixes;

  /// For the open prefixes whose sub-keys hold structs or maps: the options
  /// inside one such value, with paths relative to it, so that
  /// `--servers.eu.port` is looked up as `port`.
  std::map<std::string, std::shared_ptr<const Options>> open_prefix_options;

  /// Whether options that match no field are an error (rfl::NoExtraFields).
  bool reject_unknown = false;

  /// Whether a long option that matches no field may be an abbreviation of
  /// one (turned off by rfl::cli::NoAbbreviations).
  bool allow_abbreviations = true;

  /// Whether short options can be grouped (`-qv`, `-fcsv`). make_options
  /// sets it. Without a table (the table-less parse_argv), `-abc` is the
  /// one short option `abc`.
  bool group_short_options = false;

  /// Whether tokens such as `-5` and `-.5` are short options rather than
  /// negative numbers. They are when parse_argv is called without the
  /// struct. make_options clears it unless, as in argparse, a short option's
  /// name looks like a number (`-1`).
  bool negative_numbers_are_options = true;

  /// A short name that two fields give with an outermost rfl::Short, such as
  /// `rfl::Short<"o", A> a; rfl::Short<"o", B> b;`, or empty. Reading such a
  /// struct fails.
  std::string duplicate_short_name;

  /// Whether the options end at the first positional argument, so that every
  /// later token is a positional argument (rfl::cli::OptionsFirst).
  bool options_first = false;

  /// Returns the long options that `--_prefix` could abbreviate: every
  /// long option that starts with `_prefix`, in alphabetical order. Only the
  /// last segment of a dotted name is abbreviated, so `_prefix` must spell
  /// out the others and end inside the last one: `database.h` abbreviates
  /// `database.host`, but `data` and `database.` do not. Keys of maps and of
  /// rfl::ExtraFields are not known in advance, so they are never among
  /// them. Only meaningful if find_long(_prefix) is unknown: an exact match
  /// always wins.
  std::vector<std::string> find_abbreviations(
      const std::string& _prefix
  ) const {
    std::vector<std::string> names;
    if (_prefix.empty() || _prefix.back() == path_separator) {
      return names;
    }
    const auto last_separator = _prefix.rfind(path_separator);
    for (auto it = long_options.lower_bound(_prefix);
         it != long_options.end() && it->first.starts_with(_prefix); ++it) {
      if (it->first.rfind(path_separator) == last_separator) {
        names.push_back(it->first);
      }
    }
    return names;
  }

  /// Whether the last rfl::Positional field is a list. It then takes every
  /// positional argument after those of the other positional fields.
  bool last_positional_is_list() const {
    return !positionals.empty() && find_long(positionals.back()).is_list;
  }

  /// Returns the long option `--_name`.
  Option find_long(const std::string& _name) const {
    const auto it = long_options.find(_name);
    if (it != long_options.end()) {
      return it->second;
    }

    // A key below a leaf field (for example a variant of structs): the
    // reader decides what it means.
    for (auto pos = _name.find(path_separator); pos != std::string::npos;
         pos = _name.find(path_separator, pos + 1)) {
      if (long_options.count(_name.substr(0, pos))) {
        return Option{OptionKind::other};
      }
    }

    // A nested struct or map itself, such as `--database`.
    const auto prefix = _name + path_separator;
    if (has_prefix(long_options, prefix) || has_prefix(open_prefixes, prefix)) {
      return Option{OptionKind::other};
    }

    // A key inside a map or a struct with rfl::ExtraFields. The longest
    // matching prefix wins.
    const std::pair<const std::string, Option>* open = nullptr;
    for (const auto& p : open_prefixes) {
      if (_name.size() > p.first.size() && _name.starts_with(p.first)
          && (!open || p.first.size() > open->first.size())) {
        open = &p;
      }
    }
    if (open) {
      const auto rest = _name.substr(open->first.size());
      const auto sep = rest.find(path_separator);
      if (sep == std::string::npos) {
        return open->second;
      }
      const auto nested = open_prefix_options.find(open->first);
      if (nested != open_prefix_options.end()) {
        return nested->second->find_long(rest.substr(sep + 1));
      }
      return Option{OptionKind::other};
    }

    return Option{};
  }

  /// Returns the option that the short alias `-_name` stands for.
  Option find_short(const std::string& _name) const {
    const auto it = short_options.find(_name);
    return it == short_options.end() ? Option{} : find_long(it->second);
  }

 private:
  static bool has_prefix(
      const std::map<std::string, Option>& _map, const std::string& _prefix
  ) {
    const auto it = _map.lower_bound(_prefix);
    return it != _map.end() && it->first.starts_with(_prefix);
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

template <class... Ts>
struct type_list {
  template <class T>
  static constexpr bool contains = (std::is_same_v<T, Ts> || ...);

  template <class T>
  using add = type_list<Ts..., T>;
};

/// What rfl::Short, rfl::Positional and rfl::ImplicitValue say about a field
/// of type T. They are found through each other and through rfl::DefaultVal,
/// std::optional, rfl::Description, rfl::Deprecated and rfl::Rename, in any
/// order. Of several rfl::Short and rfl::Positional, the outermost counts.
template <class T>
struct cli_annotations {
  static constexpr bool is_short = false;
  static constexpr bool is_positional = false;
  static constexpr size_t num_implicit_value = 0;
  static constexpr rfl::internal::StringLiteral<1> short_name = "";
  static constexpr rfl::internal::StringLiteral<1> implicit_value = "";
};

template <class T>
using cli_annotations_t = cli_annotations<std::remove_cvref_t<T>>;

template <class T>
struct cli_annotations<DefaultVal<T>> : cli_annotations_t<T> {};

template <class T>
struct cli_annotations<std::optional<T>> : cli_annotations_t<T> {};

template <rfl::internal::StringLiteral _description, class T>
struct cli_annotations<Description<_description, T>> : cli_annotations_t<T> {};

template <rfl::internal::StringLiteral _deprecation_message,
          rfl::internal::StringLiteral _description, class T>
struct cli_annotations<Deprecated<_deprecation_message, _description, T>>
    : cli_annotations_t<T> {};

template <rfl::internal::StringLiteral _name, class T>
struct cli_annotations<Rename<_name, T>> : cli_annotations_t<T> {};

template <class T>
struct cli_annotations<Positional<T>> : cli_annotations_t<T> {
  static constexpr bool is_short = false;
  static constexpr bool is_positional = true;
};

template <rfl::internal::StringLiteral _name, class T>
struct cli_annotations<Short<_name, T>> : cli_annotations_t<T> {
  static constexpr bool is_short = true;
  static constexpr bool is_positional = false;
  static constexpr auto short_name = _name;
};

template <class T, rfl::internal::StringLiteral _implicit>
struct cli_annotations<ImplicitValue<T, _implicit>> : cli_annotations_t<T> {
  static constexpr size_t num_implicit_value =
      cli_annotations_t<T>::num_implicit_value + 1;
  static constexpr auto implicit_value = _implicit;
};

/// Whether T is rfl::Short directly around rfl::Positional, or the other way
/// round, which is a compile-time error.
template <class T>
consteval bool is_short_around_positional() {
  if constexpr (rfl::internal::is_short_v<T>) {
    return rfl::internal::is_positional_v<typename T::Type>;
  }
  else if constexpr (rfl::internal::is_positional_v<T>) {
    return rfl::internal::is_short_v<typename T::Type>;
  }
  else {
    return false;
  }
}

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
  /// Whether rfl::Short is the outermost wrapper of the field, not found
  /// through others.
  static constexpr bool is_direct_short = rfl::internal::is_short_v<OuterType>;
  /// The same for rfl::Positional.
  static constexpr bool is_direct_positional =
      rfl::internal::is_positional_v<OuterType>;
  static constexpr bool has_implicit_value =
      Annotations::num_implicit_value > 0;

  static_assert(
      !is_short_around_positional<OuterType>(),
      "Nested wrappers (Positional<Short<...>> or Short<..., Positional<...>>) "
      "are not allowed.");
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
  else if constexpr (parsing::is_optional_v<U>) {
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
                     || rfl::internal::is_implicit_value_v<U>
                     || parsing::is_rename_v<U>
                     || parsing::is_default_val_v<U>
                     || parsing::is_skip_v<U>) {
    return unwrap_option_type<typename U::Type, ProcessorsType>();
  }
  else if constexpr (rfl::internal::has_reflection_type_v<U>) {
    return unwrap_option_type<typename U::ReflectionType, ProcessorsType>();
  }
  else if constexpr (rfl::internal::has_read_reflector<U>) {
    return unwrap_option_type<
        typename Reflector<U>::ReflType, ProcessorsType>();
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
    || parsing::is_tagged_union_v<T> || std::is_same_v<T, rfl::Generic>;

/// The NamedTuple whose fields are the options of the struct T: T itself if
/// it is an rfl::NamedTuple (whose names the processors leave alone), or T
/// with the processors applied.
template <class T, class ProcessorsType>
using option_fields_t = typename std::conditional_t<
    rfl::internal::is_named_tuple_v<T>, std::type_identity<T>,
    rfl::internal::Processed<T, ProcessorsType>>::type;

/// Whether the reader reads T as a list, through Reader::to_array: vectors,
/// sets, deques and the like, std::array, rfl::Array, C arrays, tuples and
/// pairs. Maps are not lists, even those whose keys are not strings. The
/// value of a list option is split into items at the list delimiter, and
/// repeating the option adds the items of every occurrence, so `--x a --x b`
/// is `--x a,b`.
template <class T>
constexpr bool is_option_list_v =
    (parsing::is_vector_like_v<T> && !parsing::is_map_like_v<T>
     && !parsing::is_bytestring_v<T> && !parsing::is_vectorstring_v<T>)
    || parsing::is_array_v<T> || parsing::is_rfl_array_v<T>
    || parsing::is_c_array_v<T> || parsing::is_tuple_v<T>
    || parsing::is_rfl_tuple_v<T> || parsing::is_pair_v<T>;

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
  static_assert(!std::is_arithmetic_v<U> || std::is_same_v<U, bool>
                    || Annotations::implicit_value.length > 0,
                "rfl::ImplicitValue on a number needs an implicit value, such "
                "as rfl::ImplicitValue<int, \"1\">.");
  return Option{OptionKind::implicit_value,
                std::string(Annotations::implicit_value.string_view()),
                is_option_list_v<U>};
}

/// The option for a key of a map or of rfl::ExtraFields with values of
/// type T.
template <class T, class ProcessorsType>
Option sub_key_option() {
  using U = unwrapped_option_t<T, ProcessorsType>;
  if constexpr (cli_annotations_t<T>::num_implicit_value > 0) {
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
    return Option{OptionKind::value, "", is_option_list_v<U>};
  }
}

template <class T, class ProcessorsType, class VisitedType>
void collect_options(const std::string& _prefix, Options* _options);

template <class T, class ProcessorsType, class VisitedType>
void collect_value_options(Options* _options);

/// Adds the open prefix `_prefix` of a map or of rfl::ExtraFields with
/// values of type T. If the values are structs or maps, also adds the
/// options inside one of them.
template <class T, class ProcessorsType, class VisitedType>
void add_open_prefix(const std::string& _prefix, Options* _options) {
  using U = unwrapped_option_t<T, ProcessorsType>;
  _options->open_prefixes[_prefix] = sub_key_option<T, ProcessorsType>();
  if constexpr (cli_annotations_t<T>::num_implicit_value == 0
                && (parsing::is_string_map_v<U>
                    || (is_option_struct_v<U>
                        && !VisitedType::template contains<U>))) {
    auto nested = std::make_shared<Options>();
    collect_value_options<T, ProcessorsType, VisitedType>(nested.get());
    _options->open_prefix_options[_prefix] = std::move(nested);
  }
}

template <class T, class ProcessorsType, class VisitedType>
void add_option(const std::string& _path, Options* _options) {
  using U = unwrapped_option_t<T, ProcessorsType>;
  if constexpr (cli_annotations_t<T>::num_implicit_value > 0) {
    _options->long_options[_path] =
        implicit_value_option<T, ProcessorsType>();
  }
  else if constexpr (std::is_same_v<U, bool>) {
    _options->long_options[_path] = Option{OptionKind::flag};
  }
  else if constexpr (is_open_option_v<U>) {
    _options->long_options[_path] = Option{OptionKind::other};
  }
  else if constexpr (parsing::is_string_map_v<U>) {
    add_open_prefix<typename U::mapped_type, ProcessorsType, VisitedType>(
        _path + path_separator, _options);
  }
  else if constexpr (is_option_struct_v<U>) {
    if constexpr (VisitedType::template contains<U>) {
      // A recursive type: stop here and let the reader handle deeper keys.
      _options->open_prefixes[_path + path_separator] =
          Option{OptionKind::other};
    }
    else {
      collect_options<U, ProcessorsType, VisitedType>(
          _path + path_separator, _options);
    }
  }
  else {
    _options->long_options[_path] =
        Option{OptionKind::value, "", is_option_list_v<U>};
  }
}

template <class FieldType, class ProcessorsType, class VisitedType>
void add_option_for_field(const std::string& _prefix, Options* _options) {
  using T = std::remove_cvref_t<typename FieldType::Type>;
  if constexpr (rfl::internal::is_extra_fields_v<T>) {
    add_open_prefix<typename T::mapped_type, ProcessorsType, VisitedType>(
        _prefix, _options);
  }
  else {
    add_option<T, ProcessorsType, VisitedType>(
        _prefix + std::string(FieldType::name()), _options);
  }
}

template <class T, class ProcessorsType, class VisitedType>
void collect_options(const std::string& _prefix, Options* _options) {
  using NT = option_fields_t<T, ProcessorsType>;
  using NextVisitedType = typename VisitedType::template add<T>;
  [&]<size_t... _is>(std::index_sequence<_is...>) {
    (add_option_for_field<
         rfl::tuple_element_t<_is, typename NT::Fields>, ProcessorsType,
         NextVisitedType>(_prefix, _options),
     ...);
  }(std::make_index_sequence<NT::size()>());
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
  }
  else {
    _options->open_prefixes[""] = Option{OptionKind::other};
  }
}

/// The rfl::Short of a top-level field.
struct ShortAlias {
  std::string name;
  std::string path;
  /// Whether rfl::Short is the outermost wrapper of the field.
  bool is_direct = false;
};

/// Adds the short alias and the positional slot of the _i-th top-level
/// field, if it has them.
template <class NamedTupleType, size_t _i>
void add_short_and_positional(std::vector<ShortAlias>* _aliases,
                              std::vector<std::string>* _wrapped_positionals,
                              Options* _options) {
  using Info = field_info<NamedTupleType, _i>;
  if constexpr (Info::is_short) {
    _aliases->push_back(ShortAlias{
        std::string(get_short_name<NamedTupleType, _i>().string_view()),
        std::string(Info::name()), Info::is_direct_short});
  }
  if constexpr (Info::is_direct_positional) {
    _options->positionals.emplace_back(Info::name());
  }
  else if constexpr (Info::is_positional) {
    _wrapped_positionals->emplace_back(Info::name());
  }
}

/// An outermost rfl::Short always counts; two with the same name are kept
/// as an error (Options::duplicate_short_name). An
/// rfl::Short found through other wrappers counts only if no other field
/// has its name.
inline void add_short_aliases(const std::vector<ShortAlias>& _aliases,
                              Options* _options) {
  for (const auto& alias : _aliases) {
    if (alias.is_direct
        && !_options->short_options.emplace(alias.name, alias.path).second
        && _options->duplicate_short_name.empty()) {
      _options->duplicate_short_name = alias.name;
    }
  }
  for (const auto& alias : _aliases) {
    const auto has_name = [&](const ShortAlias& _other) {
      return _other.name == alias.name;
    };
    if (!alias.is_direct
        && std::count_if(_aliases.begin(), _aliases.end(), has_name) == 1) {
      _options->short_options.emplace(alias.name, alias.path);
    }
  }
}

template <class T, class ProcessorsType>
void collect_short_and_positional(Options* _options) {
  using NT = option_fields_t<T, ProcessorsType>;
  std::vector<ShortAlias> aliases;
  std::vector<std::string> wrapped_positionals;
  [&]<size_t... _is>(std::index_sequence<_is...>) {
    (add_short_and_positional<NT, _is>(&aliases, &wrapped_positionals,
                                       _options),
     ...);
  }(std::make_index_sequence<NT::size()>());
  add_short_aliases(aliases, _options);
  // Outermost rfl::Positional fields take the first positional arguments,
  // and those found through other wrappers the ones after them.
  _options->positionals.insert(_options->positionals.end(),
                               wrapped_positionals.begin(),
                               wrapped_positionals.end());
}

/// Derives the options that T accepts from its fields: every long option,
/// with whether it is a flag, takes a value or has an implicit value, and
/// the rfl::Short aliases and rfl::Positional fields of the top-level
/// struct.
/// @tparam T The target struct type
/// @tparam ProcessorsType The processors applied to the struct
/// @return The options, with reject_unknown set if ProcessorsType contains
/// rfl::NoExtraFields, allow_abbreviations cleared if it contains
/// rfl::cli::NoAbbreviations, and options_first set if it contains
/// rfl::cli::OptionsFirst
template <class T, class ProcessorsType>
Options make_options() {
  Options options;
  options.group_short_options = true;
  options.reject_unknown = rfl::internal::no_extra_fields_v<ProcessorsType>;
  options.allow_abbreviations = !no_abbreviations_v<ProcessorsType>;
  options.options_first = options_first_v<ProcessorsType>;
  using U = unwrapped_option_t<T, ProcessorsType>;
  collect_value_options<T, ProcessorsType, type_list<>>(&options);
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
  return options;
}

}  // namespace rfl::cli::internal

#endif
