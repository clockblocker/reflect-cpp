#ifndef RFL_CLI_USAGE_HPP_
#define RFL_CLI_USAGE_HPP_

#include <algorithm>
#include <cstddef>
#include <exception>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../Processors.hpp"
#include "../Result.hpp"
#include "../SnakeCaseToKebabCase.hpp"
#include "../generic/Writer.hpp"
#include "../internal/default_if_missing_v.hpp"
#include "../internal/is_default_val_v.hpp"
#include "../internal/is_extra_fields.hpp"
#include "../internal/is_named_tuple.hpp"
#include "../internal/no_optionals_v.hpp"
#include "../internal/processed_t.hpp"
#include "../parsing/is_required.hpp"
#include "../parsing/schema/Definition.hpp"
#include "../parsing/schema/Type.hpp"
#include "../parsing/schema/make.hpp"
#include "App.hpp"
#include "Reader.hpp"
#include "options.hpp"
#include "resolve_args.hpp"

namespace rfl::cli {

namespace internal {

/// Whether a top-level field is required, which the schema cannot tell the
/// way the parser sees it.
struct UsageFieldMeta {
  bool required = true;
};

/// Everything usage() renders about one argument.
struct UsageArg {
  /// The long name, including the path of nested objects ("database.host").
  std::string name;
  std::string short_name;
  bool positional = false;
  /// What the option does with a value, from the parser's option table:
  /// flags (`--verbose`) can be given without one.
  OptionKind kind = OptionKind::value;
  /// Whether the field is a list.
  bool is_list = false;
  bool required = true;
  /// Whether `required` was taken from the parser's compile-time traits, so
  /// that the schema must not override it.
  bool required_is_known = false;
  /// Whether the option is shown without a value, as the built-in `--help`
  /// and `--version` are.
  bool bare = false;
  /// The placeholder for one value: "<value>", "<bool>" or "<a|b|c>". Lists
  /// are rendered from it with the list delimiter.
  std::string value = "<value>";
  /// The allowed values of enums and literals.
  std::vector<std::string> choices;
  std::string description;
};

/// Whether a missing field of type `ValueType` inside root type `U` is filled
/// in by the parser's default-value path. Mirrors
/// `NamedTupleParser::handle_one_missing_field` without invoking `T{}` at
/// runtime (for structs, `schema::make` already constructs `T{}` once if and
/// only if `has_default_val_v<T>` is true).
template <class U, class ValueType>
constexpr bool usage_default_val_is_optional() {
  if constexpr (!rfl::internal::is_default_val_v<ValueType>) {
    return false;
  } else if constexpr (rfl::internal::is_named_tuple_v<U>) {
    using Inner = typename rfl::internal::default_val_t<ValueType>::Type;
    return !std::is_const_v<ValueType> &&
           std::is_default_constructible_v<Inner> &&
           std::is_default_constructible_v<ValueType>;
  } else {
    return std::is_default_constructible_v<U>;
  }
}

template <class U, class NamedTupleType, class ProcessorsType, size_t _i>
void add_usage_field_meta(std::map<std::string, UsageFieldMeta>* _meta) {
  using Info = field_info<NamedTupleType, _i>;
  using ValueType = std::remove_cvref_t<typename Info::InnerType>;
  auto& meta = (*_meta)[std::string(Info::name())];
  // The same condition as in NamedTupleParser::handle_one_missing_field.
  // The schema cannot tell on its own, because it marks a DefaultVal field as
  // optional even where the parser requires it.
  meta.required =
      !usage_default_val_is_optional<U, ValueType>()
      && !rfl::internal::is_extra_fields_v<ValueType>
      && (rfl::internal::no_optionals_v<ProcessorsType>
          || parsing::is_required<ValueType, false>());
}

/// Collects whether each top-level field is required from the same
/// compile-time traits that the parser uses, so the help matches what the
/// parser accepts. Positional, Short and flags come from the parser's option
/// table (make_options).
template <class U, class NamedTupleType, class ProcessorsType, size_t... _is>
std::map<std::string, UsageFieldMeta> collect_usage_field_meta(
    std::index_sequence<_is...>) {
  std::map<std::string, UsageFieldMeta> meta;
  (add_usage_field_meta<U, NamedTupleType, ProcessorsType, _is>(&meta), ...);
  return meta;
}

/// Walks the format-agnostic schema of the struct and collects one UsageArg
/// per command-line argument, in declaration order.
class UsageCollector {
 public:
  using Type = parsing::schema::Type;

  UsageCollector(const parsing::schema::Definition* _definition,
                 const std::map<std::string, UsageFieldMeta>* _meta,
                 const Options* _options,
                 const bool _optionals_are_required)
      : definition_(_definition),
        meta_(_meta),
        options_(_options),
        optionals_are_required_(_optionals_are_required) {
    for (const auto& [short_name, path] : _options->short_options) {
      short_names_[path] = short_name;
    }
  }

  /// Collects the arguments.
  /// @param _all_optional Whether no argument is required
  /// (rfl::DefaultIfMissing).
  std::vector<UsageArg> collect(const bool _all_optional) {
    auto root = UsageArg{};
    root.required = !_all_optional;
    add(definition_->root_, std::move(root), false);
    return std::move(args_);
  }

 private:
  /// @param _outermost Whether _type is the outermost type of a field.
  void add(const Type& _type, UsageArg _arg, const bool _outermost) {
    _type.variant_.visit([&](const auto& _t) {
      using T = std::remove_cvref_t<decltype(_t)>;
      // Description and Deprecated wrap the inner schema node without changing
      // how the field is read. Short, Positional and Rename leave no node.
      if constexpr (std::is_same_v<T, Type::Description>
                    || std::is_same_v<T, Type::Deprecated>) {
        if (_arg.description.empty()) {
          _arg.description = _t.description_;
        }
        add(*_t.type_, std::move(_arg), _outermost);

      } else if constexpr (std::is_same_v<T, Type::Optional>) {
        if (!_arg.required_is_known && !optionals_are_required_) {
          _arg.required = false;
        }
        add(*_t.type_, std::move(_arg), false);

      } else if constexpr (std::is_same_v<T, Type::DefaultVal>) {
        // The schema populates `default_value_` only when the parser recognizes
        // the DefaultVal field and default-constructed its enclosing struct.
        if (!_arg.required_is_known && _outermost && _t.default_value_) {
          _arg.required = false;
        }
        add(*_t.type_, std::move(_arg), false);

      } else if constexpr (std::is_same_v<T, Type::Validated>) {
        add(*_t.type_, std::move(_arg), false);

      } else if constexpr (std::is_same_v<T, Type::Reference>) {
        const auto it = definition_->definitions_.find(_t.name_);
        if (it == definition_->definitions_.end()
            || !visiting_.insert(_t.name_).second) {
          // Unknown or recursive: there is no finite set of paths to list.
          add_leaf(std::move(_arg));
          return;
        }
        add(it->second, std::move(_arg), _outermost);
        visiting_.erase(_t.name_);

      } else if constexpr (std::is_same_v<T, Type::Object>) {
        add_object(_t, _arg);

      } else if constexpr (std::is_same_v<T, Type::StringMap>) {
        _arg.name += path_separator;
        _arg.name += "<key>";
        _arg.short_name.clear();
        _arg.positional = false;
        add(*_t.value_type_, std::move(_arg), false);

      } else if constexpr (std::is_same_v<T, Type::Boolean>) {
        _arg.value = "<bool>";
        _arg.kind = OptionKind::flag;
        add_leaf(std::move(_arg));

      } else if constexpr (std::is_same_v<T, Type::Literal>) {
        add_choices(_t.values_, std::move(_arg));

      } else if constexpr (std::is_same_v<T, Type::DescribedLiteral>) {
        auto values = std::vector<std::string>();
        for (const auto& v : _t.values_) {
          values.push_back(v.value_);
        }
        add_choices(values, std::move(_arg));

      } else if constexpr (std::is_same_v<T, Type::TypedArray>
                           || std::is_same_v<T, Type::FixedSizeTypedArray>
                           || std::is_same_v<T, Type::Tuple>) {
        _arg.is_list = true;
        add_leaf(std::move(_arg));

      } else {
        add_leaf(std::move(_arg));
      }
    });
  }

  void add_object(const Type::Object& _object, const UsageArg& _parent) {
    const bool is_root = _parent.name.empty();
    for (const auto& [name, type] : _object.types_) {
      auto arg = UsageArg{};
      arg.name = is_root ? name : _parent.name + path_separator + name;
      // Children of an optional object are only needed if it is given.
      arg.required = _parent.required;
      if (is_root) {
        const auto& positionals = options_->positionals;
        arg.positional = std::find(positionals.begin(), positionals.end(),
                                   name) != positionals.end();
        const auto short_name = short_names_.find(name);
        if (short_name != short_names_.end()) {
          arg.short_name = short_name->second;
        }
        const auto it = meta_->find(name);
        if (it != meta_->end()) {
          arg.required = arg.required && it->second.required;
          arg.required_is_known = true;
        }
      }
      add(type, std::move(arg), true);
    }
  }

  void add_choices(const std::vector<std::string>& _values, UsageArg _arg) {
    _arg.choices = _values;
    _arg.value = "<";
    for (size_t i = 0; i < _values.size(); ++i) {
      _arg.value += (i == 0 ? "" : "|") + _values[i];
    }
    _arg.value += ">";
    add_leaf(std::move(_arg));
  }

  void add_leaf(UsageArg _arg) {
    if (_arg.name.empty()) {
      return;
    }
    // How the parser treats the option, from the table that it uses. Keep
    // the schema-derived kind (such as OptionKind::flag for nested booleans)
    // when the option table only tracks top-level paths.
    const auto option = options_->find_long(_arg.name);
    if (option.kind != OptionKind::unknown) {
      _arg.kind = option.kind;
    }
    args_.emplace_back(std::move(_arg));
  }

 private:
  const parsing::schema::Definition* definition_;
  const std::map<std::string, UsageFieldMeta>* meta_;
  const Options* options_;
  const bool optionals_are_required_;
  /// The rfl::Short aliases, by the path of their field.
  std::map<std::string, std::string> short_names_;
  std::set<std::string> visiting_;
  std::vector<UsageArg> args_;
};

/// The placeholder for the value of _arg: "<value>", "<a|b>", or for a list
/// "<v1,v2,...>" with the list delimiter.
inline std::string usage_value_to_string(const UsageArg& _arg) {
  if (!_arg.is_list) {
    return _arg.value;
  }
  const auto d = std::string(1, array_delimiter);
  return "<v1" + d + "v2" + d + "...>";
}

/// "<input-file>", "-p|--port=<value>", "--verbose[=<bool>]"
inline std::string usage_arg_to_string(const UsageArg& _arg,
                                       const std::string& _short_sep) {
  if (_arg.positional) {
    return "<" + _arg.name + ">";
  }
  auto str = _arg.short_name.empty() ? std::string()
                                     : "-" + _arg.short_name + _short_sep;
  str += "--" + _arg.name;
  if (_arg.bare) {
    return str;
  }
  const auto value = usage_value_to_string(_arg);
  switch (_arg.kind) {
    case OptionKind::flag:
      // `--verbose`: the value can be left out.
      str += "[=" + value + "]";
      break;
    default:
      str += "=" + value;
      break;
  }
  return str;
}

inline bool usage_ends_sentence(const std::string& _str) {
  return !_str.empty()
         && (_str.back() == '.' || _str.back() == '!' || _str.back() == '?');
}

/// "Port to connect on. Required."
inline std::string usage_help_text(const UsageArg& _arg) {
  auto sentences = std::vector<std::string>();
  if (_arg.positional && !_arg.choices.empty()) {
    auto choices = std::string("Choices: ");
    for (size_t i = 0; i < _arg.choices.size(); ++i) {
      choices += (i == 0 ? "" : "|") + _arg.choices[i];
    }
    sentences.push_back(std::move(choices));
  }
  if (_arg.required) {
    sentences.push_back("Required");
  }
  auto text = _arg.description;
  while (!text.empty()
         && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r'
             || text.back() == '\n')) {
    text.pop_back();
  }
  for (const auto& s : sentences) {
    if (!text.empty()) {
      text += usage_ends_sentence(text) ? " " : ". ";
    }
    text += s;
  }
  if (!sentences.empty() && !usage_ends_sentence(text)) {
    text += ".";
  }
  return text;
}

/// The rows of the built-in options enabled by rfl::cli::App.
inline std::vector<UsageArg> builtin_usage_args(
    const BuiltinOptions& _builtin) {
  auto args = std::vector<UsageArg>();
  const auto make = [](std::string_view _name, std::string_view _short_name,
                       std::string _description) {
    auto arg = UsageArg{};
    arg.name = std::string(_name);
    arg.short_name = std::string(_short_name);
    arg.kind = OptionKind::flag;
    arg.bare = true;
    arg.required = false;
    arg.description = std::move(_description);
    return arg;
  };
  if (_builtin.help) {
    args.push_back(make(help_option,
                        _builtin.help_short ? help_short_option : "",
                        "Show this help"));
  }
  if (_builtin.version) {
    args.push_back(make(version_option, "", "Show the version"));
  }
  return args;
}

/// @param _header Shown before the usage line in the detailed form
/// (rfl::cli::App).
inline std::string format_usage(const std::string& _program_name,
                                const std::vector<UsageArg>& _args,
                                const bool _detailed,
                                const std::string& _header) {
  auto out = (_detailed ? _header : std::string()) + "Usage: " + _program_name;
  for (const auto& arg : _args) {
    const auto str = usage_arg_to_string(arg, "|");
    out += arg.required ? " " + str : " [" + str + "]";
  }
  out += '\n';
  if (!_detailed || _args.empty()) {
    return out;
  }

  auto rows = std::vector<std::pair<std::string, std::string>>();
  size_t width = 0;
  for (const auto& arg : _args) {
    rows.emplace_back(usage_arg_to_string(arg, ", "), usage_help_text(arg));
    width = std::max(width, rows.back().first.size());
  }

  constexpr size_t indent = 2;
  constexpr size_t gap = 2;
  const auto continuation = '\n' + std::string(indent + width + gap, ' ');
  out += "\nArguments:\n";
  for (const auto& [left, right] : rows) {
    out += std::string(indent, ' ') + left;
    if (!right.empty()) {
      out += std::string(width - left.size() + gap, ' ');
      for (size_t i = 0; i < right.size(); ++i) {
        const char c = right[i];
        if (c == '\n') {
          if (i + 1 < right.size() && right[i + 1] != '\n') {
            out += continuation;
          } else {
            out += '\n';
          }
        } else {
          out += c;
        }
      }
    }
    out += '\n';
  }
  return out;
}

}  // namespace internal

/// Generates the usage line, and optionally a help text with one row per
/// argument, for the struct that `rfl::cli::read<T, Ps...>()` parses into.
/// It applies the same processors as `read`, so the names always match what
/// the parser accepts (kebab-case, rfl::Rename, rfl::Flatten, nested paths).
///
/// The help text lists the description (rfl::Description), the allowed
/// values of enums and literals and whether the argument is required.
///
/// With an rfl::cli::App in Ps, the detailed form starts with its header
/// (name, version and description), and both forms list the built-in
/// `-h, --help` and `--version` options that rfl::cli::App enables (unless
/// opted out via rfl::cli::BuiltinHelp or an empty version).
/// @tparam T The struct type that the arguments are parsed into
/// @tparam Ps The processors passed to `rfl::cli::read`
/// @param _program_name The name shown after "Usage:", usually argv[0]
/// @param _detailed Whether to add a line for every argument
/// @return The usage text, ending with a newline, or an error if no read of
/// T can succeed: two fields have the same short name, or T has a bare
/// rfl::DefaultVal and default-constructing it throws.
template <class T, class... Ps>
rfl::Result<std::string> usage(const std::string& _program_name,
                               const bool _detailed = false) {
  using ProcessorsType = Processors<SnakeCaseToKebabCase, Ps...>;
  using U = internal::unwrapped_option_t<T, ProcessorsType>;
  // The table that read() parses with, so the help shows what the parser
  // accepts, plus the built-in options of rfl::cli::App that read_app handles.
  auto options = internal::make_options<T, ProcessorsType>();
  const auto builtin = internal::add_builtin_options<T, ProcessorsType>(&options);
  return internal::check_short_names(options).and_then(
      [&](auto) -> rfl::Result<std::string> {
        try {
          std::map<std::string, internal::UsageFieldMeta> meta;
          if constexpr (internal::is_option_struct_v<U>) {
            using NamedTupleType =
                internal::option_fields_t<U, ProcessorsType>;
            meta = internal::collect_usage_field_meta<U, NamedTupleType,
                                                      ProcessorsType>(
                std::make_index_sequence<NamedTupleType::size()>{});
          }
          // The schema takes the default values from a default-constructed
          // T, which can throw.
          const auto definition =
              parsing::schema::make<Reader, generic::Writer, T,
                                    ProcessorsType>();
          auto collector = internal::UsageCollector(
              &definition, &meta, &options,
              rfl::internal::no_optionals_v<ProcessorsType>);
          auto args = internal::builtin_usage_args(builtin);
          const auto fields = collector.collect(
              rfl::internal::default_if_missing_v<ProcessorsType>);
          args.insert(args.end(), fields.begin(), fields.end());
          return internal::format_usage(
              _program_name, args, _detailed,
              internal::app_header<ProcessorsType>(_program_name));
        } catch (const std::exception& e) {
          return error(e.what());
        }
      });
}

}  // namespace rfl::cli

#endif
