#ifndef RFL_CLI_USAGE_HPP_
#define RFL_CLI_USAGE_HPP_

#include <algorithm>
#include <cstddef>
#include <exception>
#include <locale>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "../Generic.hpp"
#include "../Processors.hpp"
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
#include "../parsing/schema/ValidationType.hpp"
#include "../parsing/schema/make.hpp"
#include "../to_generic.hpp"
#include "Reader.hpp"
#include "options.hpp"

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
  /// The placeholder for one value: "<value>", "<bool>" or "<a|b|c>". Lists
  /// are rendered from it with the list delimiter.
  std::string value = "<value>";
  /// The allowed values of enums and literals.
  std::vector<std::string> choices;
  std::string description;
  std::optional<std::string> deprecation;
  /// Rendered constraints, such as "Range: 1..65535.".
  std::vector<std::string> constraints;
  std::optional<std::string> default_value;
};

template <class NamedTupleType, class ProcessorsType, size_t _i>
void add_usage_field_meta(const bool _default_vals_are_optional,
                          std::map<std::string, UsageFieldMeta>* _meta) {
  using Info = field_info<NamedTupleType, _i>;
  using ValueType = std::remove_cvref_t<typename Info::InnerType>;
  auto& meta = (*_meta)[std::string(Info::name())];
  // The same condition as in NamedTupleParser::handle_one_missing_field.
  // is_default_val_v sees a DefaultVal through Short, Positional, Rename,
  // Description and Deprecated, in any order, as the parser does. The schema
  // cannot tell, because it marks a DefaultVal field as optional even where
  // the parser requires it (see usage_default_vals_are_optional).
  meta.required =
      !(rfl::internal::is_default_val_v<ValueType>
        && _default_vals_are_optional)
      && !rfl::internal::is_extra_fields_v<ValueType>
      && (rfl::internal::no_optionals_v<ProcessorsType>
          || parsing::is_required<ValueType, false>());
}

/// Collects whether each top-level field is required from the same
/// compile-time traits that the parser uses, so the help matches what the
/// parser accepts. Positional, Short and flags come from the parser's option
/// table (make_options).
template <class NamedTupleType, class ProcessorsType, size_t... _is>
std::map<std::string, UsageFieldMeta> collect_usage_field_meta(
    const bool _default_vals_are_optional, std::index_sequence<_is...>) {
  std::map<std::string, UsageFieldMeta> meta;
  (add_usage_field_meta<NamedTupleType, ProcessorsType, _is>(
       _default_vals_are_optional, &meta),
   ...);
  return meta;
}

/// Whether the parser fills in the rfl::DefaultVal fields of T that are
/// missing. It does so by default-constructing T before reading it. If T is
/// not default-constructible, or T{} throws (for instance because a
/// validator rejects another field's default value), they are required, or
/// the read fails if one of them is not wrapped in another type. An
/// rfl::NamedTuple has no in-class initializers, so it is never
/// default-constructed for its defaults.
template <class T>
bool usage_default_vals_are_optional() {
  if constexpr (rfl::internal::is_named_tuple_v<T>) {
    return true;
  } else if constexpr (std::is_default_constructible_v<T>) {
    try {
      [[maybe_unused]] const auto t = T{};
      return true;
    } catch (const std::exception&) {
      return false;
    }
  } else {
    return false;
  }
}

/// Renders a number without locale-dependent separators.
inline std::string usage_number_to_string(const double _value) {
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream.precision(15);
  stream << _value;
  return stream.str();
}

inline std::string usage_number_to_string(
    const rfl::Variant<double, int>& _value) {
  return _value.visit([](const auto _v) -> std::string {
    if constexpr (std::is_same_v<std::remove_cvref_t<decltype(_v)>, int>) {
      return std::to_string(_v);
    } else {
      return usage_number_to_string(_v);
    }
  });
}

/// Renders a default value the way it would be passed on the command line.
/// Returns nullopt for values that cannot be rendered sensibly (null,
/// objects, empty arrays).
/// Lists are joined with the list delimiter.
inline std::optional<std::string> usage_default_to_string(
    const Generic& _value) {
  return std::visit(
      [](const auto& _v) -> std::optional<std::string> {
        using V = std::remove_cvref_t<decltype(_v)>;
        if constexpr (std::is_same_v<V, bool>) {
          return _v ? "true" : "false";
        } else if constexpr (std::is_same_v<V, int64_t>) {
          return std::to_string(_v);
        } else if constexpr (std::is_same_v<V, double>) {
          return usage_number_to_string(_v);
        } else if constexpr (std::is_same_v<V, std::string>) {
          return _v.empty() ? std::string("\"\"") : _v;
        } else if constexpr (std::is_same_v<V, Generic::Array>) {
          if (_v.empty()) {
            return std::nullopt;
          }
          std::string joined;
          for (const auto& element : _v) {
            const auto str = usage_default_to_string(element);
            if (!str) {
              return std::nullopt;
            }
            if (!joined.empty()) {
              joined += array_delimiter;
            }
            joined += *str;
          }
          return joined;
        } else {
          return std::nullopt;
        }
      },
      _value.variant());
}

/// Renders a numeric bound such as ">= 1", "1..10" or "< 0 or > 10".
/// Returns nullopt if the validation is not purely numeric.
inline std::optional<std::string> usage_bounds_to_string(
    const parsing::schema::ValidationType& _validation) {
  using ValidationType = parsing::schema::ValidationType;
  const auto join = [](const std::vector<ValidationType>& _types,
                       const std::string& _sep) -> std::optional<std::string> {
    std::string joined;
    for (const auto& t : _types) {
      const auto str = usage_bounds_to_string(t);
      if (!str) {
        return std::nullopt;
      }
      joined += (joined.empty() ? "" : _sep) + *str;
    }
    return joined;
  };
  return _validation.variant_.visit(
      [&](const auto& _v) -> std::optional<std::string> {
        using V = std::remove_cvref_t<decltype(_v)>;
        if constexpr (std::is_same_v<V, ValidationType::AllOf>) {
          if (_v.types_.size() == 2) {
            using Min = ValidationType::Minimum;
            using Max = ValidationType::Maximum;
            const auto& first = _v.types_[0].variant_;
            const auto& second = _v.types_[1].variant_;
            const auto* min = rfl::get_if<Min>(&first);
            const auto* max = rfl::get_if<Max>(&second);
            if (!min && !max) {
              min = rfl::get_if<Min>(&second);
              max = rfl::get_if<Max>(&first);
            }
            if (min && max) {
              return usage_number_to_string(min->value_) + ".."
                     + usage_number_to_string(max->value_);
            }
          }
          return join(_v.types_, " and ");
        } else if constexpr (std::is_same_v<V, ValidationType::AnyOf>
                             || std::is_same_v<V, ValidationType::OneOf>) {
          return join(_v.types_, " or ");
        } else if constexpr (std::is_same_v<V, ValidationType::EqualTo>) {
          return usage_number_to_string(_v.value_);
        } else if constexpr (std::is_same_v<V, ValidationType::NotEqualTo>) {
          return "!= " + usage_number_to_string(_v.value_);
        } else if constexpr (std::is_same_v<V, ValidationType::Minimum>) {
          return ">= " + usage_number_to_string(_v.value_);
        } else if constexpr (std::is_same_v<V, ValidationType::Maximum>) {
          return "<= " + usage_number_to_string(_v.value_);
        } else if constexpr (std::is_same_v<V,
                                            ValidationType::ExclusiveMinimum>) {
          return "> " + usage_number_to_string(_v.value_);
        } else if constexpr (std::is_same_v<V,
                                            ValidationType::ExclusiveMaximum>) {
          return "< " + usage_number_to_string(_v.value_);
        } else {
          return std::nullopt;
        }
      });
}

/// Renders a validation as one or more phrases, such as "Range: 1..10",
/// "Size: >= 1" or "Pattern: ^[a-z]+$".
inline std::vector<std::string> usage_constraints(
    const parsing::schema::ValidationType& _validation) {
  using ValidationType = parsing::schema::ValidationType;
  if (const auto bounds = usage_bounds_to_string(_validation)) {
    return {"Range: " + *bounds};
  }
  return _validation.variant_.visit(
      [](const auto& _v) -> std::vector<std::string> {
        using V = std::remove_cvref_t<decltype(_v)>;
        if constexpr (std::is_same_v<V, ValidationType::AllOf>) {
          std::vector<std::string> all;
          for (const auto& t : _v.types_) {
            const auto phrases = usage_constraints(t);
            all.insert(all.end(), phrases.begin(), phrases.end());
          }
          return all;
        } else if constexpr (std::is_same_v<V, ValidationType::AnyOf>
                             || std::is_same_v<V, ValidationType::OneOf>) {
          std::string joined;
          for (const auto& t : _v.types_) {
            std::string alternative;
            for (const auto& phrase : usage_constraints(t)) {
              alternative += (alternative.empty() ? "" : ", ") + phrase;
            }
            joined += (joined.empty() ? "" : " or ") + alternative;
          }
          return {joined};
        } else if constexpr (std::is_same_v<V, ValidationType::Size>) {
          const auto bounds = usage_bounds_to_string(*_v.size_limit_);
          if (!bounds) {
            return {};
          }
          return {"Size: " + *bounds};
        } else if constexpr (std::is_same_v<V, ValidationType::Regex>) {
          return {"Pattern: " + _v.pattern_};
        } else {
          return {};
        }
      });
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
  /// @param _defaults The default values of all fields, if known.
  std::vector<UsageArg> collect(const bool _all_optional,
                                const std::optional<Generic>& _defaults) {
    auto root = UsageArg{};
    root.required = !_all_optional;
    add(definition_->root_, std::move(root), _defaults, false);
    order_positionals();
    return std::move(args_);
  }

 private:
  /// @param _outermost Whether _type is the outermost type of a field.
  void add(const Type& _type, UsageArg _arg,
           const std::optional<Generic>& _default, const bool _outermost) {
    _type.variant_.visit([&](const auto& _t) {
      using T = std::remove_cvref_t<decltype(_t)>;
      // Description and Deprecated do not change how a field is read, so a
      // DefaultVal inside them still makes the field optional
      // (is_default_val_v). Short, Positional and Rename leave no node.
      if constexpr (std::is_same_v<T, Type::Description>) {
        if (_arg.description.empty()) {
          _arg.description = _t.description_;
        }
        add(*_t.type_, std::move(_arg), _default, _outermost);

      } else if constexpr (std::is_same_v<T, Type::Deprecated>) {
        if (_arg.description.empty()) {
          _arg.description = _t.description_;
        }
        _arg.deprecation = _t.deprecation_message_;
        add(*_t.type_, std::move(_arg), _default, _outermost);

      } else if constexpr (std::is_same_v<T, Type::Optional>) {
        if (!_arg.required_is_known && !optionals_are_required_) {
          _arg.required = false;
        }
        add(*_t.type_, std::move(_arg), _default, false);

      } else if constexpr (std::is_same_v<T, Type::DefaultVal>) {
        // Like the parser (is_default_val_v), only count a DefaultVal that
        // wraps the field, possibly inside Description or Deprecated. The
        // schema has its default value only if the struct around it could be
        // default-constructed, which is when the parser fills it in.
        if (!_arg.required_is_known && _outermost && _t.default_value_) {
          _arg.required = false;
        }
        add(*_t.type_, std::move(_arg),
            _t.default_value_ ? _t.default_value_ : _default, false);

      } else if constexpr (std::is_same_v<T, Type::Validated>) {
        const auto constraints = usage_constraints(_t.validation_);
        _arg.constraints.insert(_arg.constraints.end(), constraints.begin(),
                                constraints.end());
        add(*_t.type_, std::move(_arg), _default, false);

      } else if constexpr (std::is_same_v<T, Type::Reference>) {
        const auto it = definition_->definitions_.find(_t.name_);
        if (it == definition_->definitions_.end()
            || !visiting_.insert(_t.name_).second) {
          // Unknown or recursive: there is no finite set of paths to list.
          add_leaf(std::move(_arg), _default);
          return;
        }
        add(it->second, std::move(_arg), _default, _outermost);
        visiting_.erase(_t.name_);

      } else if constexpr (std::is_same_v<T, Type::Object>) {
        add_object(_t, _arg, _default);

      } else if constexpr (std::is_same_v<T, Type::StringMap>) {
        _arg.name += path_separator;
        _arg.name += "<key>";
        add_leaf(std::move(_arg), std::nullopt);

      } else if constexpr (std::is_same_v<T, Type::Boolean>) {
        _arg.value = "<bool>";
        add_leaf(std::move(_arg), _default);

      } else if constexpr (std::is_same_v<T, Type::Literal>) {
        add_choices(_t.values_, std::move(_arg), _default);

      } else if constexpr (std::is_same_v<T, Type::DescribedLiteral>) {
        auto values = std::vector<std::string>();
        for (const auto& v : _t.values_) {
          values.push_back(v.value_);
        }
        add_choices(values, std::move(_arg), _default);

      } else if constexpr (std::is_same_v<T, Type::TypedArray>
                           || std::is_same_v<T, Type::FixedSizeTypedArray>
                           || std::is_same_v<T, Type::Tuple>) {
        _arg.is_list = true;
        add_leaf(std::move(_arg), _default);

      } else {
        add_leaf(std::move(_arg), _default);
      }
    });
  }

  void add_object(const Type::Object& _object, const UsageArg& _parent,
                  const std::optional<Generic>& _defaults) {
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
      auto child_default = std::optional<Generic>();
      const auto* obj = _defaults
                            ? std::get_if<Generic::Object>(&_defaults->variant())
                            : nullptr;
      if (obj) {
        if (auto res = obj->get(name)) {
          child_default = std::move(*res);
        }
      }
      add(type, std::move(arg), child_default, true);
    }
  }

  void add_choices(const std::vector<std::string>& _values, UsageArg _arg,
                   const std::optional<Generic>& _default) {
    _arg.choices = _values;
    _arg.value = "<";
    for (size_t i = 0; i < _values.size(); ++i) {
      _arg.value += (i == 0 ? "" : "|") + _values[i];
    }
    _arg.value += ">";
    add_leaf(std::move(_arg), _default);
  }

  void add_leaf(UsageArg _arg, const std::optional<Generic>& _default) {
    if (_arg.name.empty()) {
      return;
    }
    // How the parser treats the option, from the table that it uses. A map
    // key ("labels.<key>") is found through its open prefix.
    const auto option = options_->find_long(_arg.name);
    _arg.kind = option.kind;
    if (_default) {
      _arg.default_value = usage_default_to_string(*_default);
    }
    args_.emplace_back(std::move(_arg));
  }

  /// Puts the positional arguments in the order in which the parser fills
  /// them (Options::positionals: outermost rfl::Positional fields first),
  /// in the places where positional arguments are.
  void order_positionals() {
    const auto& order = options_->positionals;
    auto slots = std::vector<size_t>();
    auto positionals = std::vector<UsageArg>();
    for (size_t i = 0; i < args_.size(); ++i) {
      if (args_[i].positional) {
        slots.push_back(i);
        positionals.push_back(std::move(args_[i]));
      }
    }
    const auto rank = [&](const UsageArg& _arg) {
      return std::find(order.begin(), order.end(), _arg.name) - order.begin();
    };
    std::stable_sort(positionals.begin(), positionals.end(),
                     [&](const UsageArg& _a, const UsageArg& _b) {
                       return rank(_a) < rank(_b);
                     });
    for (size_t i = 0; i < slots.size(); ++i) {
      args_[slots[i]] = std::move(positionals[i]);
    }
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

/// "Port to connect on. Range: 1..65535. Default: 8080. Required."
inline std::string usage_help_text(const UsageArg& _arg) {
  auto sentences = std::vector<std::string>();
  if (_arg.deprecation) {
    sentences.push_back(_arg.deprecation->empty()
                            ? std::string("Deprecated")
                            : "Deprecated: " + *_arg.deprecation);
  }
  if (_arg.positional && !_arg.choices.empty()) {
    auto choices = std::string("Choices: ");
    for (size_t i = 0; i < _arg.choices.size(); ++i) {
      choices += (i == 0 ? "" : "|") + _arg.choices[i];
    }
    sentences.push_back(std::move(choices));
  }
  sentences.insert(sentences.end(), _arg.constraints.begin(),
                   _arg.constraints.end());
  if (_arg.default_value) {
    sentences.push_back("Default: " + *_arg.default_value);
  }
  if (_arg.required) {
    sentences.push_back("Required");
  }
  auto text = _arg.description;
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

inline std::string format_usage(const std::string& _program_name,
                                const std::vector<UsageArg>& _args,
                                const bool _detailed) {
  auto out = "Usage: " + _program_name;
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
      for (const char c : right) {
        if (c == '\n') {
          out += continuation;
        } else {
          out += c;
        }
      }
    }
    out += '\n';
  }
  return out;
}

/// The default values of all fields, if they are known: only with
/// rfl::DefaultIfMissing (otherwise, plain fields are required and their
/// default member initializers are never used) and only if T is
/// default-constructible, which read<T, rfl::DefaultIfMissing> requires, too.
template <class T, class... Ps>
std::optional<Generic> usage_defaults() {
  using ProcessorsType = Processors<SnakeCaseToKebabCase, Ps...>;
  if constexpr (rfl::internal::default_if_missing_v<ProcessorsType>
                && std::is_default_constructible_v<T>) {
    try {
      return rfl::to_generic<SnakeCaseToKebabCase, Ps...>(T{});
    } catch (const std::exception&) {
      // For instance, a default value that does not satisfy its validator.
      return std::nullopt;
    }
  } else {
    return std::nullopt;
  }
}

}  // namespace internal

/// Generates the usage line, and optionally a help text with one row per
/// argument, for the struct that `rfl::cli::read<T, Ps...>()` parses into.
/// It applies the same processors as `read`, so the names always match what
/// the parser accepts (kebab-case, rfl::Rename, rfl::Flatten, nested paths).
///
/// The help text lists, where known, the description (rfl::Description), the
/// allowed values of enums and literals, the constraints of validators, the
/// default value (rfl::DefaultVal, or all fields with rfl::DefaultIfMissing)
/// and whether the argument is required.
/// @tparam T The struct type that the arguments are parsed into
/// @tparam Ps The processors passed to `rfl::cli::read`
/// @param _program_name The name shown after "Usage:", usually argv[0]
/// @param _detailed Whether to add a line for every argument
/// @return The usage text, ending with a newline
template <class T, class... Ps>
std::string usage(const std::string& _program_name,
                  const bool _detailed = false) {
  using ProcessorsType = Processors<SnakeCaseToKebabCase, Ps...>;
  using NamedTupleType = rfl::internal::processed_t<T, ProcessorsType>;
  const auto meta =
      internal::collect_usage_field_meta<NamedTupleType, ProcessorsType>(
          internal::usage_default_vals_are_optional<T>(),
          std::make_index_sequence<NamedTupleType::size()>{});
  // The table that read() parses with, so the help shows what the parser
  // accepts.
  const auto options = internal::make_options<T, ProcessorsType>();
  const auto definition =
      parsing::schema::make<Reader, generic::Writer, T, ProcessorsType>();
  auto collector = internal::UsageCollector(
      &definition, &meta, &options,
      rfl::internal::no_optionals_v<ProcessorsType>);
  const auto args = collector.collect(
      rfl::internal::default_if_missing_v<ProcessorsType>,
      internal::usage_defaults<T, Ps...>());
  return internal::format_usage(_program_name, args, _detailed);
}

}  // namespace rfl::cli

#endif
