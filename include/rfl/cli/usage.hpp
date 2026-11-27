#ifndef RFL_CLI_USAGE_HPP_
#define RFL_CLI_USAGE_HPP_

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <locale>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "../DefaultHint.hpp"
#include "../Processors.hpp"
#include "../Result.hpp"
#include "../SnakeCaseToKebabCase.hpp"
#include "../Tuple.hpp"
#include "../apply.hpp"
#include "../enums.hpp"
#include "../internal/DeferredDefaults.hpp"
#include "../internal/TransparentWrapper.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/default_if_missing_v.hpp"
#include "../internal/has_default_val_v.hpp"
#include "../internal/has_reflection_method_v.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/has_reflector.hpp"
#include "../internal/is_bitset.hpp"
#include "../internal/is_default_val_v.hpp"
#include "../internal/is_deprecated.hpp"
#include "../internal/is_description.hpp"
#include "../internal/is_extra_fields.hpp"
#include "../internal/is_literal.hpp"
#include "../internal/is_named_tuple.hpp"
#include "../internal/is_positional.hpp"
#include "../internal/is_short.hpp"
#include "../internal/is_validator.hpp"
#include "../internal/no_optionals_v.hpp"
#include "../internal/underlying_enums_v.hpp"
#include "../parsing/ParserCommented.hpp"
#include "../parsing/ParserFilepath.hpp"
#include "../parsing/check_default_value.hpp"
#include "../parsing/schema/ValidationType.hpp"
#include "../to_view.hpp"
#include "App.hpp"
#include "ListDelimiter.hpp"
#include "Opt.hpp"
#include "OptionsFirst.hpp"
#include "Reader.hpp"
#include "options.hpp"
#include "resolve_args.hpp"

namespace rfl::cli {

/// Runtime customization for `rfl::cli::usage` and `rfl::cli::read_app`.
struct UsageOptions {
  bool detailed = true;
  /// If non-empty, shown before the Usage line in detailed output (overrides
  /// any header from rfl::cli::App).
  std::string header = {};
  /// If non-empty, replaces the auto-generated synopsis after "Usage: ".
  std::string synopsis = {};
  /// If non-empty, shown after the Usage line in detailed output.
  std::string description = {};
  /// If non-empty, appended after a blank line at the end of detailed output.
  std::string footer = {};
  /// If non-empty, places built-in `--help` and `--version` in this group
  /// (at the start of the group if fields already use it).
  std::string help_group = {};
  /// Maximum left-column width used to align option descriptions; options
  /// wider than this place their description on the next line instead of
  /// widening every other row. Set to 0 for no cap.
  size_t max_option_width = 40;
  /// Group names to omit from generated detailed help output (e.g. conditional
  /// feature groups hidden at runtime).
  std::vector<std::string> hidden_groups = {};
};

namespace internal {

/// Everything usage() renders about one argument.
struct UsageArg {
  /// The long name, including the path of nested objects ("database.host").
  std::string name;
  std::string short_name;
  bool positional = false;
  /// What the option does with a value, from the parser's option table:
  /// flags (`--verbose`) and rfl::ImplicitValue (`--help`) can be given
  /// without one.
  OptionKind kind = OptionKind::value;
  /// For OptionKind::implicit_value: what the option stands for without a
  /// value.
  std::string implicit_value;
  /// Whether the field is a list, which can be repeated.
  bool is_list = false;
  /// Whether the field accepts multiple space-separated tokens (rfl::MultiToken).
  bool multi_token = false;
  /// Whether the field is the last positional one and a list, so that it
  /// takes the remaining positional arguments.
  bool takes_rest = false;
  bool required = true;
  /// Whether the option is shown without a value, as the built-in `--help`
  /// and `--version` are.
  bool bare = false;
  /// The placeholder for one value: "<value>", "<bool>" or "<a|b|c>". Lists
  /// are rendered from it with the list delimiter.
  std::string value = "<value>";
  /// Whether `value` was explicitly set via rfl::ValueName.
  bool has_custom_value_name = false;
  /// The allowed values of enums and literals.
  std::vector<std::string> choices;
  std::string description;
  std::optional<std::string> deprecation;
  /// Rendered constraints, such as "Range: 1..65535.".
  std::vector<std::string> constraints;
  std::optional<std::string> default_value;
  /// Display-only default value hint (from rfl::DefaultHint or attr::DefaultHint).
  std::string default_hint;
  /// Optional help section header (from rfl::Group).
  std::string group;
};

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

/// Renders a C++ default value directly the way it would be passed on the
/// command line, without converting through rfl::Generic.
/// Returns nullopt for values that cannot be rendered sensibly (nullopt,
/// structs, maps, empty lists, or multi-element lists without a delimiter).
template <class ProcessorsType, class T>
std::optional<std::string> format_default_value(
    const T& _val, const std::optional<char> _delimiter) {
  using U = std::remove_cvref_t<T>;
  if constexpr (std::is_same_v<U, bool>) {
    return _val ? std::string("true") : std::string("false");
  } else if constexpr (std::is_integral_v<U>) {
    return std::to_string(_val);
  } else if constexpr (std::is_floating_point_v<U>) {
    return usage_number_to_string(static_cast<double>(_val));
  } else if constexpr (std::is_same_v<U, std::string> ||
                       std::is_same_v<U, std::string_view>) {
    return _val.empty() ? std::string("\"\"") : std::string(_val);
  } else if constexpr (parsing::is_filepath_v<U>) {
    const auto s = _val.string();
    return s.empty() ? std::string("\"\"") : s;
  } else if constexpr (rfl::internal::is_bitset_v<U>) {
    return _val.to_string();
  } else if constexpr (rfl::internal::is_literal_v<U>) {
    const auto s = _val.str();
    return s.empty() ? std::string("\"\"") : s;
  } else if constexpr (std::is_enum_v<U>) {
    if constexpr (rfl::internal::has_write_reflector<U> &&
                  requires(const U& _u) { Reflector<U>::from(_u); }) {
      return format_default_value<ProcessorsType>(Reflector<U>::from(_val),
                                                  _delimiter);
    } else if constexpr (rfl::internal::underlying_enums_v<ProcessorsType>) {
      return std::to_string(static_cast<std::underlying_type_t<U>>(_val));
    } else {
      const auto s = rfl::enum_to_string<U, ProcessorsType>(_val);
      return s.empty() ? std::string("\"\"") : s;
    }
  } else if constexpr (parsing::is_optional_v<U> ||
                       rfl::internal::is_nullable_wrapper_v<U> ||
                       rfl::internal::is_tri_state_v<U> ||
                       parsing::is_unique_ptr_v<U> ||
                       parsing::is_shared_ptr_v<U>) {
    if (!_val) {
      return std::nullopt;
    }
    return format_default_value<ProcessorsType>(*_val, _delimiter);
  } else if constexpr (parsing::is_box_v<U> || parsing::is_ref_v<U>) {
    if (!_val.get()) {
      return std::nullopt;
    }
    return format_default_value<ProcessorsType>(*_val, _delimiter);
  } else if constexpr (std::is_pointer_v<U>) {
    if constexpr (!std::is_void_v<std::remove_pointer_t<U>>) {
      return _val ? format_default_value<ProcessorsType>(*_val, _delimiter)
                  : std::nullopt;
    } else {
      return std::nullopt;
    }
  } else if constexpr (rfl::internal::is_short_v<U> ||
                       rfl::internal::is_positional_v<U> ||
                       rfl::internal::is_transparent_wrapper_v<U> ||
                       parsing::is_rename_v<U> ||
                       parsing::is_default_val_v<U> ||
                       parsing::is_skip_v<U> ||
                       rfl::internal::is_description_v<U> ||
                       rfl::internal::is_deprecated_v<U> ||
                       rfl::internal::is_validator_v<U> ||
                       parsing::is_commented_v<U>) {
    return format_default_value<ProcessorsType>(_val.value(), _delimiter);
  } else if constexpr (parsing::is_variant_v<U>) {
    if (_val.valueless_by_exception()) {
      return std::nullopt;
    }
    return std::visit(
        [&](const auto& _v) {
          return format_default_value<ProcessorsType>(_v, _delimiter);
        },
        _val);
  } else if constexpr (parsing::is_rfl_variant_v<U>) {
    return _val.visit([&](const auto& _v) {
      return format_default_value<ProcessorsType>(_v, _delimiter);
    });
  } else if constexpr (parsing::is_rfl_array_v<U>) {
    return format_default_value<ProcessorsType>(_val.arr_, _delimiter);
  } else if constexpr (parsing::is_tuple_v<U> || parsing::is_pair_v<U> ||
                       parsing::is_rfl_tuple_v<U>) {
    std::string joined;
    size_t count = 0;
    bool ok = true;
    const auto append = [&](const auto& _elem) {
      if (!ok) {
        return;
      }
      if (count > 0 && !_delimiter) {
        ok = false;
        return;
      }
      const auto str = format_default_value<ProcessorsType>(_elem, _delimiter);
      if (!str) {
        ok = false;
        return;
      }
      if (count > 0) {
        joined += *_delimiter;
      }
      joined += *str;
      ++count;
    };
    if constexpr (parsing::is_rfl_tuple_v<U>) {
      rfl::apply([&](const auto&... _es) { (append(_es), ...); }, _val);
    } else {
      std::apply([&](const auto&... _es) { (append(_es), ...); }, _val);
    }
    if (!ok || count == 0) {
      return std::nullopt;
    }
    return joined;
  } else if constexpr (is_option_list_v<U> && std::ranges::range<const U>) {
    std::string joined;
    size_t count = 0;
    for (const auto& elem : _val) {
      if (count > 0 && !_delimiter) {
        return std::nullopt;
      }
      const auto str = format_default_value<ProcessorsType>(elem, _delimiter);
      if (!str) {
        return std::nullopt;
      }
      if (count > 0) {
        joined += *_delimiter;
      }
      joined += *str;
      ++count;
    }
    if (count == 0) {
      return std::nullopt;
    }
    return joined;
  } else if constexpr (rfl::internal::has_write_reflector<U> &&
                       requires(const U& _u) { Reflector<U>::from(_u); }) {
    return format_default_value<ProcessorsType>(Reflector<U>::from(_val),
                                                _delimiter);
  } else if constexpr (rfl::internal::has_reflection_type_v<U> &&
                       rfl::internal::has_reflection_method_v<U> &&
                       requires(const U& _u) { _u.reflection(); }) {
    return format_default_value<ProcessorsType>(_val.reflection(), _delimiter);
  } else {
    return std::nullopt;
  }
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

/// Compile-time metadata peeling for all transparent, documentation, optional,
/// pointer, and DefaultVal wrappers around a field's core type. Peeling at
/// compile time lets UsageWalker funnel all fields with the same unwrapped
/// core type (e.g. bool, int, std::string, std::vector<std::string>) through a
/// single walk_core<CoreType> instantiation, keeping symbol tables and compile
/// times small on large CLI structs.
template <class T, bool _outermost = true>
struct usage_peel;

template <class T, bool _outermost>
struct usage_peel_base {
  using CoreType = T;
  static constexpr std::string_view description = {};
  static constexpr bool is_deprecated = false;
  static constexpr std::string_view deprecation_message = {};
  static constexpr std::string_view default_hint = {};
  static constexpr bool has_outermost_default_val = false;
  static constexpr bool has_any_default_val = false;
  static constexpr bool has_nullable = false;

  static inline const CoreType* unwrap_ptr(const T* _p) noexcept { return _p; }
};

template <class T, bool _outermost>
struct usage_peel : usage_peel_base<std::remove_cvref_t<T>, _outermost> {};

template <rfl::internal::StringLiteral _desc, class T, bool _outermost>
struct usage_peel<Description<_desc, T>, _outermost> {
  using Next = usage_peel<std::remove_cvref_t<T>, _outermost>;
  using CoreType = typename Next::CoreType;
  static constexpr std::string_view description =
      _desc.string_view().empty() ? Next::description : _desc.string_view();
  static constexpr bool is_deprecated = Next::is_deprecated;
  static constexpr std::string_view deprecation_message =
      Next::deprecation_message;
  static constexpr std::string_view default_hint = Next::default_hint;
  static constexpr bool has_outermost_default_val =
      Next::has_outermost_default_val;
  static constexpr bool has_any_default_val = Next::has_any_default_val;
  static constexpr bool has_nullable = Next::has_nullable;

  static inline const CoreType* unwrap_ptr(
      const Description<_desc, T>* _p) noexcept {
    return Next::unwrap_ptr(_p ? &_p->value() : nullptr);
  }
};

template <rfl::internal::StringLiteral _msg, rfl::internal::StringLiteral _desc,
          class T, bool _outermost>
struct usage_peel<Deprecated<_msg, _desc, T>, _outermost> {
  using Next = usage_peel<std::remove_cvref_t<T>, _outermost>;
  using CoreType = typename Next::CoreType;
  static constexpr std::string_view description =
      _desc.string_view().empty() ? Next::description : _desc.string_view();
  static constexpr bool is_deprecated = true;
  static constexpr std::string_view deprecation_message =
      Next::is_deprecated ? Next::deprecation_message : _msg.string_view();
  static constexpr std::string_view default_hint = Next::default_hint;
  static constexpr bool has_outermost_default_val =
      Next::has_outermost_default_val;
  static constexpr bool has_any_default_val = Next::has_any_default_val;
  static constexpr bool has_nullable = Next::has_nullable;

  static inline const CoreType* unwrap_ptr(
      const Deprecated<_msg, _desc, T>* _p) noexcept {
    return Next::unwrap_ptr(_p ? &_p->value() : nullptr);
  }
};

template <rfl::internal::StringLiteral _hint, class T, bool _outermost>
struct usage_peel<DefaultHint<_hint, T>, _outermost> {
  using Next = usage_peel<std::remove_cvref_t<T>, _outermost>;
  using CoreType = typename Next::CoreType;
  static constexpr std::string_view description = Next::description;
  static constexpr bool is_deprecated = Next::is_deprecated;
  static constexpr std::string_view deprecation_message =
      Next::deprecation_message;
  static constexpr std::string_view default_hint =
      _hint.string_view().empty() ? Next::default_hint : _hint.string_view();
  static constexpr bool has_outermost_default_val =
      Next::has_outermost_default_val;
  static constexpr bool has_any_default_val = Next::has_any_default_val;
  static constexpr bool has_nullable = Next::has_nullable;

  static inline const CoreType* unwrap_ptr(
      const DefaultHint<_hint, T>* _p) noexcept {
    return Next::unwrap_ptr(_p ? &_p->value() : nullptr);
  }
};

template <class T, class... Attrs, bool _outermost>
struct usage_peel<Opt<T, Attrs...>, _outermost> {
  using Info = typename Opt<T, Attrs...>::AttrsInfo;
  using Next = usage_peel<std::remove_cvref_t<T>, _outermost>;
  using CoreType = typename Next::CoreType;
  static constexpr std::string_view description =
      Info::description.empty() ? Next::description : Info::description;
  static constexpr bool is_deprecated =
      Info::is_deprecated || Next::is_deprecated;
  static constexpr std::string_view deprecation_message =
      Next::is_deprecated ? Next::deprecation_message
                          : Info::deprecation_message;
  static constexpr std::string_view default_hint =
      Info::default_hint.empty() ? Next::default_hint : Info::default_hint;
  static constexpr bool has_outermost_default_val =
      Next::has_outermost_default_val;
  static constexpr bool has_any_default_val = Next::has_any_default_val;
  static constexpr bool has_nullable = Next::has_nullable;

  static inline const CoreType* unwrap_ptr(
      const Opt<T, Attrs...>* _p) noexcept {
    return Next::unwrap_ptr(_p ? &_p->value() : nullptr);
  }
};

template <class T, bool _outermost>
  requires((rfl::internal::is_short_v<T> || rfl::internal::is_positional_v<T> ||
            rfl::internal::is_transparent_wrapper_v<T> ||
            parsing::is_rename_v<T> || parsing::is_skip_v<T> ||
            parsing::is_commented_v<T>) &&
           !rfl::internal::is_default_hint_v<T> && !is_cli_opt_v<T>)
struct usage_peel<T, _outermost> {
  using Next = usage_peel<std::remove_cvref_t<typename T::Type>, _outermost>;
  using CoreType = typename Next::CoreType;
  static constexpr std::string_view description = Next::description;
  static constexpr bool is_deprecated = Next::is_deprecated;
  static constexpr std::string_view deprecation_message =
      Next::deprecation_message;
  static constexpr std::string_view default_hint = Next::default_hint;
  static constexpr bool has_outermost_default_val =
      Next::has_outermost_default_val;
  static constexpr bool has_any_default_val = Next::has_any_default_val;
  static constexpr bool has_nullable = Next::has_nullable;

  static inline const CoreType* unwrap_ptr(const T* _p) noexcept {
    return Next::unwrap_ptr(_p ? &_p->value() : nullptr);
  }
};

template <class T, bool _outermost>
struct usage_peel<DefaultVal<T>, _outermost> {
  using Next = usage_peel<std::remove_cvref_t<T>, false>;
  using CoreType = typename Next::CoreType;
  static constexpr std::string_view description = Next::description;
  static constexpr bool is_deprecated = Next::is_deprecated;
  static constexpr std::string_view deprecation_message =
      Next::deprecation_message;
  static constexpr std::string_view default_hint = Next::default_hint;
  static constexpr bool has_outermost_default_val = _outermost;
  static constexpr bool has_any_default_val = true;
  static constexpr bool has_nullable = Next::has_nullable;

  static inline const CoreType* unwrap_ptr(const DefaultVal<T>* _p) noexcept {
    return Next::unwrap_ptr(_p ? &_p->value() : nullptr);
  }
};

template <class T, bool _outermost>
struct usage_peel<std::optional<T>, _outermost> {
  using Next = usage_peel<std::remove_cvref_t<T>, false>;
  using CoreType = typename Next::CoreType;
  static constexpr std::string_view description = Next::description;
  static constexpr bool is_deprecated = Next::is_deprecated;
  static constexpr std::string_view deprecation_message =
      Next::deprecation_message;
  static constexpr std::string_view default_hint = Next::default_hint;
  static constexpr bool has_outermost_default_val =
      Next::has_outermost_default_val;
  static constexpr bool has_any_default_val = Next::has_any_default_val;
  static constexpr bool has_nullable = true;

  static inline const CoreType* unwrap_ptr(
      const std::optional<T>* _p) noexcept {
    return Next::unwrap_ptr((_p && *_p) ? &(**_p) : nullptr);
  }
};

template <class T, bool _outermost>
  requires(rfl::internal::is_nullable_wrapper_v<T> ||
           rfl::internal::is_tri_state_v<T>)
struct usage_peel<T, _outermost> {
  using Next = usage_peel<std::remove_cvref_t<typename T::value_type>, false>;
  using CoreType = typename Next::CoreType;
  static constexpr std::string_view description = Next::description;
  static constexpr bool is_deprecated = Next::is_deprecated;
  static constexpr std::string_view deprecation_message =
      Next::deprecation_message;
  static constexpr std::string_view default_hint = Next::default_hint;
  static constexpr bool has_outermost_default_val =
      Next::has_outermost_default_val;
  static constexpr bool has_any_default_val = Next::has_any_default_val;
  static constexpr bool has_nullable = true;

  static inline const CoreType* unwrap_ptr(const T* _p) noexcept {
    return Next::unwrap_ptr((_p && _p->has_value()) ? &(**_p) : nullptr);
  }
};

template <class T, bool _outermost>
  requires(parsing::is_unique_ptr_v<T> || parsing::is_shared_ptr_v<T>)
struct usage_peel<T, _outermost> {
  using Next =
      usage_peel<std::remove_cvref_t<typename T::element_type>, false>;
  using CoreType = typename Next::CoreType;
  static constexpr std::string_view description = Next::description;
  static constexpr bool is_deprecated = Next::is_deprecated;
  static constexpr std::string_view deprecation_message =
      Next::deprecation_message;
  static constexpr std::string_view default_hint = Next::default_hint;
  static constexpr bool has_outermost_default_val =
      Next::has_outermost_default_val;
  static constexpr bool has_any_default_val = Next::has_any_default_val;
  static constexpr bool has_nullable = true;

  static inline const CoreType* unwrap_ptr(const T* _p) noexcept {
    return Next::unwrap_ptr((_p && *_p) ? _p->get() : nullptr);
  }
};

template <class T, bool _outermost>
  requires(parsing::is_box_v<T> || parsing::is_ref_v<T>)
struct usage_peel<T, _outermost> {
  using Elem = std::conditional_t<
      parsing::is_box_v<T>, typename parsing::is_box<T>::element_type,
      typename parsing::is_ref<T>::element_type>;
  using Next = usage_peel<std::remove_cvref_t<Elem>, false>;
  using CoreType = typename Next::CoreType;
  static constexpr std::string_view description = Next::description;
  static constexpr bool is_deprecated = Next::is_deprecated;
  static constexpr std::string_view deprecation_message =
      Next::deprecation_message;
  static constexpr std::string_view default_hint = Next::default_hint;
  static constexpr bool has_outermost_default_val =
      Next::has_outermost_default_val;
  static constexpr bool has_any_default_val = Next::has_any_default_val;
  static constexpr bool has_nullable = Next::has_nullable;

  static inline const CoreType* unwrap_ptr(const T* _p) noexcept {
    return Next::unwrap_ptr((_p && _p->get()) ? _p->get() : nullptr);
  }
};

/// Walks the target struct's fields directly at compile time and collects one
/// UsageArg per command-line argument in declaration order, without building a
/// JSON schema or converting to rfl::Generic.
template <class ProcessorsType>
class UsageWalker {
 public:
  UsageWalker(const Options* _options, const std::optional<char> _delimiter)
      : options_(_options),
        optionals_are_required_(rfl::internal::no_optionals_v<ProcessorsType>),
        delimiter_(_delimiter) {
    short_names_.reserve(_options->short_options.size());
    for (const auto& [short_name, path] : _options->short_options) {
      short_names_.emplace_back(path, short_name);
    }
    std::sort(short_names_.begin(), short_names_.end(),
              [](const auto& _a, const auto& _b) {
                return _a.first < _b.first;
              });
  }

  template <class T>
  std::vector<UsageArg> collect() {
    auto root = UsageArg{};
    constexpr bool all_optional =
        rfl::internal::default_if_missing_v<ProcessorsType>;
    root.required = !all_optional;
    walk_peeled<std::remove_cvref_t<T>, type_list<>, /*_outermost=*/false>(
        std::move(root), static_cast<const std::remove_cvref_t<T>*>(nullptr),
        /*_emit_default=*/all_optional);
    mark_rest_positional();
    return std::move(args_);
  }

 private:
  template <class U>
  static std::optional<U> try_default_construct(
      rfl::internal::DeferredDefaults* _deferred) {
    try {
      const auto not_deferred = rfl::internal::DeferredDefaults::Scope(nullptr);
      return U{};
    } catch (const std::exception&) {
    }
    const auto scope = rfl::internal::DeferredDefaults::Scope(_deferred);
    return U{};
  }

  template <class T, class VisitedType, bool _outermost>
  RFL_ALWAYS_INLINE void walk_peeled(UsageArg _arg, const T* _default_ptr,
                                     const bool _emit_default) {
    using Peel = usage_peel<std::remove_cvref_t<T>, _outermost>;
    using Core = typename Peel::CoreType;
    if constexpr (!Peel::description.empty()) {
      if (_arg.description.empty()) {
        _arg.description = std::string(Peel::description);
      }
    }
    if constexpr (Peel::is_deprecated) {
      _arg.deprecation = std::string(Peel::deprecation_message);
    }
    if constexpr (!Peel::default_hint.empty()) {
      if (_arg.default_hint.empty()) {
        _arg.default_hint = std::string(Peel::default_hint);
      }
    }
    if constexpr (Peel::has_outermost_default_val) {
      if (_default_ptr != nullptr) {
        _arg.required = false;
      }
    }
    if constexpr (Peel::has_nullable) {
      if (!optionals_are_required_) {
        _arg.required = false;
      }
    }
    const bool emit = Peel::has_any_default_val ? (_default_ptr != nullptr)
                                                : _emit_default;
    const Core* core_ptr = Peel::unwrap_ptr(_default_ptr);
    walk_core<Core, VisitedType>(std::move(_arg), core_ptr, emit);
  }

  template <class U, class VisitedType>
  void walk_core(UsageArg _arg, const U* _default_ptr,
                 const bool _emit_default) {
    if constexpr (rfl::internal::is_validator_v<U>) {
      using Inner = std::remove_cvref_t<typename U::ReflectionType>;
      using ValidationType = std::remove_cvref_t<typename U::ValidationType>;
      const auto constraints =
          usage_constraints(ValidationType::template to_schema<Inner>());
      _arg.constraints.insert(_arg.constraints.end(), constraints.begin(),
                              constraints.end());
      walk_peeled<Inner, VisitedType, /*_outermost=*/false>(
          std::move(_arg), _default_ptr ? &_default_ptr->value() : nullptr,
          _emit_default);

    } else if constexpr (rfl::internal::is_literal_v<U>) {
      add_choices(U::names(), std::move(_arg),
                  (_emit_default && _default_ptr)
                      ? format_default_value<ProcessorsType>(*_default_ptr,
                                                             delimiter_)
                      : std::nullopt);

    } else if constexpr (std::is_enum_v<U>) {
      if constexpr (rfl::internal::has_read_reflector<U>) {
        walk_reflector<U, VisitedType>(std::move(_arg), _default_ptr,
                                       _emit_default);
      } else if constexpr (rfl::internal::underlying_enums_v<ProcessorsType> ||
                           rfl::internal::enums::is_flag_enum<U>) {
        add_leaf(std::move(_arg),
                 (_emit_default && _default_ptr)
                     ? format_default_value<ProcessorsType>(*_default_ptr,
                                                            delimiter_)
                     : std::nullopt);
      } else {
        const auto wire_names = rfl::get_enum_wire_names<U, ProcessorsType>();
        std::vector<std::string> values;
        values.reserve(wire_names.size());
        for (const auto& [name, val] : wire_names) {
          values.push_back(name);
        }
        add_choices(values, std::move(_arg),
                    (_emit_default && _default_ptr)
                        ? format_default_value<ProcessorsType>(*_default_ptr,
                                                               delimiter_)
                        : std::nullopt);
      }

    } else if constexpr (std::is_same_v<U, bool>) {
      _arg.value = "<bool>";
      add_leaf(std::move(_arg),
               (_emit_default && _default_ptr)
                   ? format_default_value<ProcessorsType>(*_default_ptr,
                                                          delimiter_)
                   : std::nullopt);

    } else if constexpr (parsing::is_string_map_v<U>) {
      _arg.name += path_separator;
      _arg.name += "<key>";
      add_leaf(std::move(_arg), std::nullopt);

    } else if constexpr (rfl::internal::has_read_reflector<U>) {
      walk_reflector<U, VisitedType>(std::move(_arg), _default_ptr,
                                     _emit_default);

    } else if constexpr (rfl::internal::has_reflection_type_v<U>) {
      if constexpr (VisitedType::template contains<U>) {
        add_leaf(std::move(_arg), std::nullopt);
      } else {
        using Refl = std::remove_cvref_t<typename U::ReflectionType>;
        using NextVisited = typename VisitedType::template add<U>;
        if constexpr (rfl::internal::has_reflection_method_v<U> &&
                      requires(const U& _u) { _u.reflection(); }) {
          if (_default_ptr) {
            const auto refl = _default_ptr->reflection();
            walk_peeled<Refl, NextVisited, /*_outermost=*/false>(
                std::move(_arg), &refl, _emit_default);
            return;
          }
        }
        walk_peeled<Refl, NextVisited, /*_outermost=*/false>(
            std::move(_arg), static_cast<const Refl*>(nullptr), _emit_default);
      }

    } else if constexpr (is_option_struct_v<U>) {
      if constexpr (VisitedType::template contains<U>) {
        add_leaf(std::move(_arg), std::nullopt);
      } else {
        walk_struct<U, typename VisitedType::template add<U>>(
            _arg, _default_ptr, _emit_default);
      }

    } else {
      add_leaf(std::move(_arg),
               (_emit_default && _default_ptr)
                   ? format_default_value<ProcessorsType>(*_default_ptr,
                                                          delimiter_)
                   : std::nullopt);
    }
  }

  template <class U, class VisitedType>
  void walk_reflector(UsageArg _arg, const U* _default_ptr,
                      const bool _emit_default) {
    if constexpr (VisitedType::template contains<U>) {
      add_leaf(std::move(_arg), std::nullopt);
    } else {
      using Refl = std::remove_cvref_t<typename Reflector<U>::ReflType>;
      using NextVisited = typename VisitedType::template add<U>;
      if constexpr (rfl::internal::has_write_reflector<U> &&
                    requires(const U& _u) { Reflector<U>::from(_u); }) {
        if (_default_ptr) {
          const auto refl = Reflector<U>::from(*_default_ptr);
          walk_peeled<Refl, NextVisited, /*_outermost=*/false>(
              std::move(_arg), &refl, _emit_default);
          return;
        }
      }
      walk_peeled<Refl, NextVisited, /*_outermost=*/false>(
          std::move(_arg), static_cast<const Refl*>(nullptr), _emit_default);
    }
  }

  template <class S, class VisitedType>
  void walk_struct(const UsageArg& _parent, const S* _parent_default,
                   const bool _emit_all_defaults) {
    using NT = option_fields_t<S, ProcessorsType>;
    constexpr size_t N = NT::size();

    if constexpr (rfl::internal::is_named_tuple_v<S>) {
      std::optional<S> local_defaults;
      if (!_parent_default && _emit_all_defaults) {
        if constexpr (std::is_default_constructible_v<S>) {
          try {
            local_defaults.emplace();
          } catch (const std::exception&) {
          }
        }
      }
      const S* def_obj =
          _parent_default ? _parent_default
                          : (local_defaults ? &(*local_defaults) : nullptr);
      [&]<size_t... _is>(std::index_sequence<_is...>) {
        auto visit = [&]<size_t _i>() {
          using FieldType = rfl::tuple_element_t<_i, typename S::Fields>;
          using FieldVal = std::remove_cvref_t<typename FieldType::Type>;
          if constexpr (!rfl::internal::is_extra_fields_v<FieldVal>) {
            auto arg = make_child_arg(_parent, FieldType::name());
            if (options_->is_hidden(arg.name)) {
              return;
            }
            if constexpr (rfl::internal::is_default_val_v<FieldVal>) {
              arg.required = false;
            }
            const FieldVal* ptr = def_obj ? &rfl::get<_i>(*def_obj) : nullptr;
            walk_peeled<FieldVal, VisitedType, /*_outermost=*/true>(
                std::move(arg), ptr, _emit_all_defaults);
          }
        };
        (visit.template operator()<_is>(), ...);
      }(std::make_index_sequence<N>());
      return;
    } else {
      if constexpr (rfl::internal::has_unwrapped_default_val_v<S>) {
        static_assert(requires { S{}; },
                      "A struct with rfl::DefaultVal fields must be "
                      "default-constructible, because the default values come "
                      "from a default-constructed instance.");
      }

      std::optional<S> local_defaults;
      auto no_default = std::array<bool, N>{};
      rfl::internal::DeferredDefaults deferred;
      const S* def_obj = _parent_default;
      if (!def_obj) {
        const bool want_defaults =
            _emit_all_defaults || rfl::internal::has_default_val_v<S>;
        if constexpr (std::is_default_constructible_v<S>) {
          if (want_defaults) {
            try {
              local_defaults = try_default_construct<S>(&deferred);
            } catch (const std::exception&) {
              if constexpr (rfl::internal::has_unwrapped_default_val_v<S>) {
                throw;
              }
            }
            if (local_defaults) {
              def_obj = &(*local_defaults);
            }
          }
        }
      }

      if (def_obj) {
        // Reuse the non-const to_view<S> instantiation already created by read()
        // instead of duplicating the entire reflection tree for `const S`.
        // The view is only read through const pointers below.
        auto view = ProcessorsType::template process<S>(
            rfl::to_view(const_cast<S&>(*def_obj)));
        using ViewType = decltype(view);
        if (deferred.num() != 0) {
          auto all_check = parsing::DefaultValueCheck{.stop_early = false};
          [&]<size_t... _is>(std::index_sequence<_is...>) {
            auto chk = [&]<size_t _i>() {
              using VFT = typename rfl::tuple_element_t<
                  _i, typename ViewType::Fields>::Type;
              if constexpr (std::is_pointer_v<VFT>) {
                parsing::check_default_value(*rfl::get<_i>(view), &all_check);
              }
            };
            (chk.template operator()<_is>(), ...);
          }(std::make_index_sequence<N>());
          if (all_check.num_invalid < deferred.num()) {
            no_default.fill(true);
          } else {
            [&]<size_t... _is>(std::index_sequence<_is...>) {
              auto chk = [&]<size_t _i>() {
                using VFT = typename rfl::tuple_element_t<
                    _i, typename ViewType::Fields>::Type;
                if constexpr (std::is_pointer_v<VFT>) {
                  auto check = parsing::DefaultValueCheck{};
                  parsing::check_default_value(*rfl::get<_i>(view), &check);
                  no_default[_i] = check.invalid || check.unchecked;
                }
              };
              (chk.template operator()<_is>(), ...);
            }(std::make_index_sequence<N>());
          }
        }
        [&]<size_t... _is>(std::index_sequence<_is...>) {
          auto visit = [&]<size_t _i>() {
            using FieldType = rfl::tuple_element_t<_i, typename NT::Fields>;
            using FieldVal = std::remove_cvref_t<typename FieldType::Type>;
            if constexpr (!rfl::internal::is_extra_fields_v<FieldVal>) {
              auto arg = make_child_arg(_parent, FieldType::name());
              if (options_->is_hidden(arg.name)) {
                return;
              }
              const FieldVal* ptr = nullptr;
              using VFT = typename rfl::tuple_element_t<
                  _i, typename ViewType::Fields>::Type;
              if constexpr (std::is_pointer_v<VFT>) {
                if (!no_default[_i]) {
                  ptr = rfl::get<_i>(view);
                }
              }
              walk_peeled<FieldVal, VisitedType, /*_outermost=*/true>(
                  std::move(arg), ptr, _emit_all_defaults);
            }
          };
          (visit.template operator()<_is>(), ...);
        }(std::make_index_sequence<N>());
        return;
      }

      [&]<size_t... _is>(std::index_sequence<_is...>) {
        auto visit = [&]<size_t _i>() {
          using FieldType = rfl::tuple_element_t<_i, typename NT::Fields>;
          using FieldVal = std::remove_cvref_t<typename FieldType::Type>;
          if constexpr (!rfl::internal::is_extra_fields_v<FieldVal>) {
            auto arg = make_child_arg(_parent, FieldType::name());
            if (options_->is_hidden(arg.name)) {
              return;
            }
            walk_peeled<FieldVal, VisitedType, /*_outermost=*/true>(
                std::move(arg), static_cast<const FieldVal*>(nullptr),
                _emit_all_defaults);
          }
        };
        (visit.template operator()<_is>(), ...);
      }(std::make_index_sequence<N>());
    }
  }

  UsageArg make_child_arg(const UsageArg& _parent,
                          const std::string_view _field_name) const {
    const bool is_root = _parent.name.empty();
    auto arg = UsageArg{};
    arg.name = is_root
                   ? std::string(_field_name)
                   : _parent.name + path_separator + std::string(_field_name);
    arg.required = _parent.required;
    if (is_root) {
      const auto& positionals = options_->positionals;
      arg.positional =
          std::find(positionals.begin(), positionals.end(), arg.name)
          != positionals.end();
      const auto short_it = std::lower_bound(
          short_names_.begin(), short_names_.end(), std::string_view(arg.name),
          [](const auto& _entry, const std::string_view _key) {
            return _entry.first < _key;
          });
      if (short_it != short_names_.end() && short_it->first == arg.name) {
        arg.short_name = std::string(short_it->second);
      }
    }
    return arg;
  }

  void add_choices(const std::vector<std::string>& _values, UsageArg _arg,
                   std::optional<std::string> _default) {
    _arg.choices = _values;
    _arg.value = "<";
    for (size_t i = 0; i < _values.size(); ++i) {
      _arg.value += (i == 0 ? "" : "|") + _values[i];
    }
    _arg.value += ">";
    add_leaf(std::move(_arg), std::move(_default));
  }

  void add_leaf(UsageArg _arg, std::optional<std::string> _default) {
    if (_arg.name.empty()) {
      return;
    }
    const auto option = options_->find_long(_arg.name);
    _arg.kind = option.kind;
    _arg.implicit_value = std::string(option.implicit_value);
    _arg.is_list = option.is_list || option.is_counter;
    _arg.multi_token = option.multi_token;
    const bool pure_flag =
        (options_->pure_flags || option.is_counter)
        && _arg.kind == OptionKind::flag && !_arg.positional;
    if (pure_flag) {
      _arg.bare = true;
    }
    if (_default && !(pure_flag && (*_default == "false" || *_default == "0"))) {
      _arg.default_value = std::move(_default);
    } else if (!_arg.default_hint.empty() &&
               !(pure_flag && _arg.default_hint == "false")) {
      _arg.default_value = _arg.default_hint;
    }
    if (const auto grp = options_->find_group(_arg.name); !grp.empty()) {
      _arg.group = std::string(grp);
    }
    if (const auto vn = options_->find_value_name(_arg.name); !vn.empty()) {
      _arg.value = (vn.front() == '<' && vn.back() == '>')
                       ? std::string(vn)
                       : "<" + std::string(vn) + ">";
      _arg.has_custom_value_name = true;
    }
    args_.emplace_back(std::move(_arg));
  }

  /// Marks the positional argument that takes the remaining arguments: a
  /// list as the last one (Options::last_positional_is_list).
  void mark_rest_positional() {
    if (!options_->last_positional_is_list()) {
      return;
    }
    for (auto& arg : args_) {
      arg.takes_rest =
          arg.positional && arg.name == options_->positionals.back();
    }
  }

 private:
  const Options* options_;
  const bool optionals_are_required_;
  const std::optional<char> delimiter_;
  std::vector<std::pair<std::string_view, std::string_view>> short_names_;
  std::vector<UsageArg> args_;
};

/// The placeholder for the value of _arg: "<value>", "<a|b>", or for a list
/// "<v1,v2,...>" with the list delimiter. If lists are not split
/// (ListDelimiter<"">), every occurrence is one item: "<value>".
inline std::string usage_value_to_string(const UsageArg& _arg,
                                         const std::optional<char> _delimiter) {
  if (_arg.has_custom_value_name || !_arg.is_list || !_delimiter) {
    return _arg.value;
  }
  const auto d = std::string(1, *_delimiter);
  return "<v1" + d + "v2" + d + "...>";
}

/// "<input-file>", "<files>...", "-p|--port=<value>", "--verbose[=<bool>]",
/// "--help[=<value>]"
inline std::string usage_arg_to_string(const UsageArg& _arg,
                                       const std::string& _short_sep,
                                       const std::optional<char> _delimiter) {
  if (_arg.positional) {
    // A list as the last positional field takes the remaining arguments.
    // Any other positional field takes one.
    return "<" + _arg.name + ">" + (_arg.takes_rest ? "..." : "");
  }
  auto str = _arg.short_name.empty() ? std::string()
                                     : "-" + _arg.short_name + _short_sep;
  str += "--" + _arg.name;
  if (_arg.bare) {
    return str;
  }
  const auto value = usage_value_to_string(_arg, _delimiter);
  switch (_arg.kind) {
    case OptionKind::flag:
    case OptionKind::implicit_value:
      // `--verbose`, `--help`: the value can be left out.
      str += "[=" + value + "]";
      break;
    default:
      str += "=" + value + (_arg.multi_token ? "..." : "");
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
  if (_arg.kind == OptionKind::implicit_value && !_arg.positional
      && !_arg.implicit_value.empty()) {
    sentences.push_back("Without a value: " + _arg.implicit_value);
  }
  if (_arg.default_value) {
    sentences.push_back("Default: " + *_arg.default_value);
  }
  if (_arg.is_list && !_arg.positional) {
    sentences.push_back("Can be repeated");
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

/// The rows of the built-in options of rfl::cli::App that no field takes.
inline std::vector<UsageArg> builtin_usage_args(
    const BuiltinOptions& _builtin,
    const std::string& _help_group = std::string()) {
  auto args = std::vector<UsageArg>();
  const auto make = [&](std::string_view _name, std::string_view _short_name,
                        std::string _description) {
    auto arg = UsageArg{};
    arg.name = std::string(_name);
    arg.short_name = std::string(_short_name);
    arg.kind = OptionKind::flag;
    arg.bare = true;
    arg.required = false;
    arg.description = std::move(_description);
    arg.group = _help_group;
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
/// @param _options_first Whether the options end at the first positional
/// argument (rfl::cli::OptionsFirst), so the synopsis lists them first.
inline std::string format_usage(const std::string& _program_name,
                                const std::vector<UsageArg>& _args,
                                const UsageOptions& _opts,
                                const std::optional<char> _delimiter,
                                const bool _options_first,
                                const std::string& _header) {
  auto out = std::string();
  if (_opts.detailed) {
    if (!_opts.header.empty()) {
      out += _opts.header;
      if (!out.ends_with('\n')) {
        out += '\n';
      }
      out += '\n';
    } else {
      out += _header;
    }
  }
  if (!_opts.synopsis.empty()) {
    out += "Usage: " + _opts.synopsis + "\n";
  } else {
    auto synopsis = _args;
    if (_options_first) {
      std::stable_partition(synopsis.begin(), synopsis.end(),
                            [](const UsageArg& _arg) { return !_arg.positional; });
    }
    out += "Usage: " + _program_name;
    for (const auto& arg : synopsis) {
      const auto str = usage_arg_to_string(arg, "|", _delimiter);
      out += arg.required ? " " + str : " [" + str + "]";
    }
    out += '\n';
  }
  if (!_opts.detailed) {
    return out;
  }
  if (!_opts.description.empty()) {
    out += "\n" + _opts.description + "\n";
  }
  if (_args.empty()) {
    if (!_opts.footer.empty()) {
      out += "\n" + _opts.footer + "\n";
    }
    return out;
  }

  auto rows = std::vector<std::pair<std::string, std::string>>();
  rows.reserve(_args.size());
  size_t width = 0;
  for (const auto& arg : _args) {
    rows.emplace_back(usage_arg_to_string(arg, ", ", _delimiter),
                      usage_help_text(arg));
    const size_t row_w = rows.back().first.size();
    if (_opts.max_option_width == 0 || row_w <= _opts.max_option_width) {
      width = std::max(width, row_w);
    } else {
      width = std::max(width, _opts.max_option_width);
    }
  }

  constexpr size_t indent = 2;
  constexpr size_t gap = 2;
  const auto continuation = '\n' + std::string(indent + width + gap, ' ');
  const auto emit_row = [&](const std::string& _left,
                            const std::string& _right) {
    out += std::string(indent, ' ') + _left;
    if (!_right.empty()) {
      if (_left.size() <= width) {
        out += std::string(width - _left.size() + gap, ' ');
      } else {
        out += continuation;
      }
      for (const char c : _right) {
        if (c == '\n') {
          out += continuation;
        } else {
          out += c;
        }
      }
    }
    out += '\n';
  };

  const bool has_groups = std::any_of(
      _args.begin(), _args.end(),
      [](const UsageArg& _arg) { return !_arg.group.empty(); });

  if (!has_groups) {
    out += "\nArguments:\n";
    for (const auto& [left, right] : rows) {
      emit_row(left, right);
    }
  } else {
    // Emit sections in order of first appearance; ungrouped args (if any) go
    // under "Arguments".
    std::vector<std::string> section_order;
    for (const auto& arg : _args) {
      const std::string section =
          arg.group.empty() ? std::string("Arguments") : arg.group;
      if (std::find(section_order.begin(), section_order.end(), section)
          == section_order.end()) {
        section_order.push_back(section);
      }
    }
    for (const auto& section : section_order) {
      out += "\n" + section + ":\n";
      for (size_t i = 0; i < _args.size(); ++i) {
        const std::string arg_section =
            _args[i].group.empty() ? std::string("Arguments") : _args[i].group;
        if (arg_section == section) {
          emit_row(rows[i].first, rows[i].second);
        }
      }
    }
  }
  if (!_opts.footer.empty()) {
    out += "\n" + _opts.footer + "\n";
  }
  return out;
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
///
/// With an rfl::cli::App in Ps, the detailed form starts with its header
/// (name, version and description), and both forms list the built-in
/// `-h, --help` and `--version` that rfl::cli::read_app handles, unless a
/// field takes their names.
/// @tparam T The struct type that the arguments are parsed into
/// @tparam Ps The processors passed to `rfl::cli::read`
/// @param _program_name The name shown after "Usage:", usually argv[0]
/// @param _opts Customization options for the generated help text
/// @return The usage text, ending with a newline, or an error if no read of
/// T can succeed: two fields have the same short name, or T has a bare
/// rfl::DefaultVal and default-constructing it throws.
template <class T, class... Ps>
rfl::Result<std::string> usage(const std::string& _program_name,
                               const UsageOptions& _opts) {
  using ProcessorsType = internal::cli_processors_t<Ps...>;
  constexpr auto delimiter = internal::list_delimiter_v<ProcessorsType>;
  // The table that read() parses with, so the help shows what the parser
  // accepts, including the options inside nested structs and maps, and the
  // built-in options of rfl::cli::App that read_app handles.
  const auto& [options, builtin] =
      internal::cached_app_options<T, ProcessorsType, /*_all_paths=*/true>();
  return internal::check_short_names(options).and_then(
      [&](auto) -> rfl::Result<std::string> {
        try {
          auto walker =
              internal::UsageWalker<ProcessorsType>(&options, delimiter);
          auto builtin_args =
              internal::builtin_usage_args(builtin, _opts.help_group);
          auto fields = walker.template collect<T>();
          auto args = std::vector<internal::UsageArg>();
          args.reserve(builtin_args.size() + fields.size());
          if (!_opts.help_group.empty() && !builtin_args.empty()) {
            const auto pos = std::find_if(
                fields.begin(), fields.end(),
                [&](const internal::UsageArg& _a) {
                  return _a.group == _opts.help_group;
                });
            if (pos != fields.end()) {
              args.insert(args.end(), fields.begin(), pos);
              args.insert(args.end(), builtin_args.begin(), builtin_args.end());
              args.insert(args.end(), pos, fields.end());
            } else {
              args = std::move(builtin_args);
              args.insert(args.end(), fields.begin(), fields.end());
            }
          } else {
            args = std::move(builtin_args);
            args.insert(args.end(), fields.begin(), fields.end());
          }
          if (!_opts.hidden_groups.empty()) {
            args.erase(
                std::remove_if(
                    args.begin(), args.end(),
                    [&](const internal::UsageArg& _a) {
                      return !_a.group.empty() &&
                             std::find(_opts.hidden_groups.begin(),
                                       _opts.hidden_groups.end(),
                                       _a.group) != _opts.hidden_groups.end();
                    }),
                args.end());
          }
          return internal::format_usage(
              _program_name, args, _opts, delimiter, options.options_first,
              internal::app_header<ProcessorsType>(_program_name));
        } catch (std::exception& e) {
          return error(e.what());
        }
      });
}

template <class T, class... Ps>
rfl::Result<std::string> usage(const std::string& _program_name,
                               const bool _detailed = false) {
  return usage<T, Ps...>(_program_name, UsageOptions{.detailed = _detailed});
}

}  // namespace rfl::cli

#endif
