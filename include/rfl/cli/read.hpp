#ifndef RFL_CLI_READ_HPP_
#define RFL_CLI_READ_HPP_

#include <algorithm>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../Processors.hpp"
#include "../SnakeCaseToKebabCase.hpp"
#include "../internal/to_ptr_named_tuple.hpp"
#include "App.hpp"
#include "KeepEmptyItems.hpp"
#include "ListDelimiter.hpp"
#include "Parser.hpp"
#include "Reader.hpp"
#include "options.hpp"
#include "parse_argv.hpp"
#include "resolve_args.hpp"

namespace rfl::cli {

/// Result returned by `rfl::cli::read_detailed`: holds the parsed struct along
/// with the sorted canonical long names of every option and positional argument
/// that was explicitly passed on the command line (including options whose
/// given value equals their default).
template <class T>
struct ReadResult {
  T value;
  std::vector<std::string> provided;
  /// Every recognized option occurrence `(canonical_long_name, raw_value)` in
  /// exact argv order.
  std::vector<std::pair<std::string, std::string>> occurrences;

  /// Returns true if the canonical option or positional name `_name` was
  /// explicitly supplied on the command line.
  bool was_provided(const std::string_view _name) const {
    const auto it = std::lower_bound(provided.begin(), provided.end(), _name);
    return it != provided.end() && *it == _name;
  }
};

/// Result returned by `rfl::cli::read_into`: holds the sorted canonical names
/// of every option and positional argument overlaid onto the target struct,
/// plus the full ordered occurrence logs.
struct ReadIntoResult {
  std::vector<std::string> provided;
  std::vector<std::pair<std::string, std::string>> occurrences;
  std::vector<OptionOccurrence> detailed_occurrences;

  /// Returns true if the canonical option or positional name `_name` was
  /// explicitly supplied on the command line.
  bool was_provided(const std::string_view _name) const {
    const auto it = std::lower_bound(provided.begin(), provided.end(), _name);
    return it != provided.end() && *it == _name;
  }
};

namespace internal {

inline bool has_prefix_key(const std::map<std::string, std::string>& _values,
                           const std::string& _prefix_dot) {
  const auto it = _values.lower_bound(_prefix_dot);
  return it != _values.end() && it->first.starts_with(_prefix_dot);
}

template <class T, class ProcessorsType>
rfl::Result<bool> overlay_into_struct(T& _target, const Reader& _r,
                                      const ResolvedArgs& _args,
                                      const std::string& _prefix);

template <class FT, class ProcessorsType>
rfl::Result<bool> overlay_field_nested(FT& _field, const Reader& _r,
                                       const ResolvedArgs& _args,
                                       const std::string& _path) {
  using RawFT = std::remove_cvref_t<FT>;
  if constexpr (rfl::internal::is_transparent_wrapper_v<RawFT>
                || rfl::internal::is_short_v<RawFT>
                || rfl::internal::is_positional_v<RawFT>
                || parsing::is_rename_v<RawFT>
                || parsing::is_default_val_v<RawFT>) {
    return overlay_field_nested<typename RawFT::Type, ProcessorsType>(
        _field.value_, _r, _args, _path);
  } else if constexpr (parsing::is_optional_v<RawFT>) {
    using Inner = typename RawFT::value_type;
    if (_field.has_value()) {
      return overlay_field_nested<Inner, ProcessorsType>(*_field, _r, _args,
                                                         _path);
    }
    if constexpr (std::is_default_constructible_v<Inner>) {
      Inner fresh{};
      const auto ok = overlay_field_nested<Inner, ProcessorsType>(
          fresh, _r, _args, _path);
      if (!ok) {
        return error(ok.error().what());
      }
      _field = std::move(fresh);
      return true;
    } else {
      const auto var = CliVarType{&_args.values, _path, std::nullopt,
                                  &_args.later_occurrences};
      auto parsed = Parser<Inner, ProcessorsType>::read(_r, var);
      if (!parsed) {
        return error(parsed.error().what());
      }
      _field = std::move(*parsed);
      return true;
    }
  } else if constexpr (parsing::is_unique_ptr_v<RawFT>
                       || parsing::is_shared_ptr_v<RawFT>) {
    using Inner = typename RawFT::element_type;
    if (_field) {
      return overlay_field_nested<Inner, ProcessorsType>(*_field, _r, _args,
                                                         _path);
    }
    const auto var = CliVarType{&_args.values, _path, std::nullopt,
                                &_args.later_occurrences};
    auto parsed = Parser<RawFT, ProcessorsType>::read(_r, var);
    if (!parsed) {
      return error(parsed.error().what());
    }
    _field = std::move(*parsed);
    return true;
  } else if constexpr (is_option_struct_v<RawFT>
                       && !rfl::internal::is_named_tuple_v<RawFT>) {
    return overlay_into_struct<RawFT, ProcessorsType>(_field, _r, _args, _path);
  } else {
    const auto var = CliVarType{&_args.values, _path, std::nullopt,
                                &_args.later_occurrences};
    auto parsed = Parser<RawFT, ProcessorsType>::read(_r, var);
    if (!parsed) {
      return error(parsed.error().what());
    }
    _field = std::move(*parsed);
    return true;
  }
}

template <class T, class ProcessorsType>
rfl::Result<bool> overlay_into_struct(T& _target, const Reader& _r,
                                      const ResolvedArgs& _args,
                                      const std::string& _prefix) {
  auto view = rfl::internal::to_ptr_named_tuple(_target);
  auto processed = ProcessorsType::template process<T>(std::move(view));
  using ProcessedNT = std::remove_cvref_t<decltype(processed)>;

  std::vector<std::string> known_paths;
  known_paths.reserve(ProcessedNT::size());

  rfl::Result<bool> status = true;
  [&]<size_t... _is>(std::index_sequence<_is...>) {
    auto visit = [&]<size_t _i>() {
      if (!status) {
        return;
      }
      using FieldType = rfl::tuple_element_t<_i, typename ProcessedNT::Fields>;
      using FT = std::remove_cvref_t<std::remove_pointer_t<typename FieldType::Type>>;
      auto* field_ptr = rfl::get<_i>(processed);
      if constexpr (rfl::internal::is_extra_fields_v<FT>) {
        return;
      } else {
        const std::string path =
            _prefix.empty()
                ? std::string(FieldType::name())
                : _prefix + path_separator + std::string(FieldType::name());
        known_paths.push_back(path);

        using U = unwrapped_option_t<FT, ProcessorsType>;
        const std::string path_dot = path + path_separator;
        if (_args.values.count(path) > 0) {
          const auto var = CliVarType{&_args.values, path, std::nullopt,
                                      &_args.later_occurrences};
          auto parsed = Parser<FT, ProcessorsType>::read(_r, var);
          if (!parsed) {
            status = error(parsed.error().what());
            return;
          }
          *field_ptr = std::move(*parsed);
        } else if constexpr (is_option_struct_v<U> && !is_open_option_v<U>
                             && !parsing::is_string_map_v<U>) {
          if (has_prefix_key(_args.values, path_dot)) {
            status =
                overlay_field_nested<FT, ProcessorsType>(*field_ptr, _r, _args,
                                                         path);
          }
        } else if constexpr (parsing::is_string_map_v<U>) {
          if (has_prefix_key(_args.values, path_dot)) {
            const auto var = CliVarType{&_args.values, path, std::nullopt,
                                        &_args.later_occurrences};
            auto parsed = Parser<FT, ProcessorsType>::read(_r, var);
            if (!parsed) {
              status = error(parsed.error().what());
              return;
            }
            *field_ptr = std::move(*parsed);
          }
        }
      }
    };
    (visit.template operator()<_is>(), ...);
  }(std::make_index_sequence<ProcessedNT::size()>());

  if (!status) {
    return status;
  }

  // Handle rfl::ExtraFields if present: overlay any remaining keys under _prefix.
  [&]<size_t... _is>(std::index_sequence<_is...>) {
    auto visit_extra = [&]<size_t _i>() {
      if (!status) {
        return;
      }
      using FieldType = rfl::tuple_element_t<_i, typename ProcessedNT::Fields>;
      using FT = std::remove_cvref_t<std::remove_pointer_t<typename FieldType::Type>>;
      if constexpr (rfl::internal::is_extra_fields_v<FT>) {
        using MappedT = typename FT::mapped_type;
        auto* extra_ptr = rfl::get<_i>(processed);
        const std::string pfx =
            _prefix.empty() ? std::string{} : _prefix + path_separator;
        for (const auto& [k, _] : _args.values) {
          if (!pfx.empty() && !k.starts_with(pfx)) {
            continue;
          }
          const std::string sub = k.substr(pfx.size());
          const auto dot = sub.find(path_separator);
          const std::string head_path =
              pfx + (dot == std::string::npos ? sub : sub.substr(0, dot));
          if (std::find(known_paths.begin(), known_paths.end(), head_path)
              != known_paths.end()) {
            continue;
          }
          if (dot != std::string::npos) {
            continue;
          }
          const auto var = CliVarType{&_args.values, k, std::nullopt,
                                      &_args.later_occurrences};
          auto parsed = Parser<MappedT, ProcessorsType>::read(_r, var);
          if (!parsed) {
            status = error(parsed.error().what());
            return;
          }
          (*extra_ptr)[sub] = std::move(*parsed);
        }
      }
    };
    (visit_extra.template operator()<_is>(), ...);
  }(std::make_index_sequence<ProcessedNT::size()>());

  return status;
}

template <class T, class... Ps, class ParseFn>
rfl::Result<ReadIntoResult> parse_and_read_into(T& _target,
                                                const ParseFn& _parse) {
  using ProcessorsType = internal::cli_processors_t<Ps...>;
  static_assert(!internal::has_app_v<ProcessorsType>,
                "rfl::cli::App turns on --help and --version, which "
                "rfl::cli::read_into cannot report. Call rfl::cli::read_app "
                "instead.");
  static_assert(!internal::keep_empty_items_v<ProcessorsType>
                    || internal::list_delimiter_v<ProcessorsType>.has_value(),
                "rfl::cli::KeepEmptyItems has no effect with "
                "rfl::cli::ListDelimiter<\"\">, which does not split lists, "
                "so an empty item is kept as \"\" anyway. Remove one of "
                "them.");
  const auto& options = cached_options<T, ProcessorsType>();
  return check_short_names(options)
      .and_then([&](bool) { return _parse(options); })
      .and_then([&](ParsedArgs _parsed) {
        return resolve_args(std::move(_parsed), options);
      })
      .and_then([&](ResolvedArgs _args) -> rfl::Result<ReadIntoResult> {
        std::vector<std::string> provided;
        provided.reserve(_args.values.size());
        for (const auto& [key, _] : _args.values) {
          provided.push_back(key);
        }
        const auto r =
            Reader(internal::list_delimiter_v<ProcessorsType>,
                   internal::keep_empty_items_v<ProcessorsType>,
                   std::move(_args.verbatim_list));
        return overlay_into_struct<T, ProcessorsType>(_target, r, _args, "")
            .transform([&](bool) {
              return ReadIntoResult{std::move(provided),
                                    std::move(_args.occurrences),
                                    std::move(_args.detailed_occurrences)};
            });
      });
}

static_assert(list_delimiter_v<Processors<>> == array_delimiter);

/// Resolves the parsed arguments and reads them into T, also capturing the
/// sorted list of explicitly provided canonical option names.
template <class T, class ProcessorsType>
rfl::Result<ReadResult<T>> read_parsed_detailed(ParsedArgs _parsed,
                                                const Options& _options) {
  static_assert(!internal::keep_empty_items_v<ProcessorsType>
                    || internal::list_delimiter_v<ProcessorsType>.has_value(),
                "rfl::cli::KeepEmptyItems has no effect with "
                "rfl::cli::ListDelimiter<\"\">, which does not split lists, "
                "so an empty item is kept as \"\" anyway. Remove one of "
                "them.");
  return resolve_args(std::move(_parsed), _options)
      .and_then([](auto _args) -> rfl::Result<ReadResult<T>> {
        std::vector<std::string> provided;
        provided.reserve(_args.values.size());
        for (const auto& [key, _] : _args.values) {
          provided.push_back(key);
        }
        const auto r =
            Reader(internal::list_delimiter_v<ProcessorsType>,
                   internal::keep_empty_items_v<ProcessorsType>,
                   std::move(_args.verbatim_list));
        const auto var = CliVarType{&_args.values, "", std::nullopt,
                                    &_args.later_occurrences};
        return Parser<T, ProcessorsType>::read(r, var).transform(
            [&](T&& _val) {
              return ReadResult<T>{std::move(_val), std::move(provided),
                                   std::move(_args.occurrences)};
            });
      });
}

/// Resolves the parsed arguments and reads them into T.
template <class T, class ProcessorsType>
rfl::Result<T> read_parsed(ParsedArgs _parsed, const Options& _options) {
  static_assert(!internal::keep_empty_items_v<ProcessorsType>
                    || internal::list_delimiter_v<ProcessorsType>.has_value(),
                "rfl::cli::KeepEmptyItems has no effect with "
                "rfl::cli::ListDelimiter<\"\">, which does not split lists, "
                "so an empty item is kept as \"\" anyway. Remove one of "
                "them.");
  return resolve_args(std::move(_parsed), _options)
      .and_then([](auto _args) -> rfl::Result<T> {
        const auto r =
            Reader(internal::list_delimiter_v<ProcessorsType>,
                   internal::keep_empty_items_v<ProcessorsType>,
                   std::move(_args.verbatim_list));
        const auto var = CliVarType{&_args.values, "", std::nullopt,
                                    &_args.later_occurrences};
        return Parser<T, ProcessorsType>::read(r, var);
      });
}

/// Categorizes the arguments with `_parse`, which takes the options of T
/// (see make_options) and returns ParsedArgs, and turns them into ReadResult<T>.
template <class T, class... Ps, class ParseFn>
rfl::Result<ReadResult<T>> parse_and_read_detailed(const ParseFn& _parse) {
  using ProcessorsType = internal::cli_processors_t<Ps...>;
  static_assert(!internal::has_app_v<ProcessorsType>,
                "rfl::cli::App turns on --help and --version, which "
                "rfl::cli::read_detailed cannot report. Call rfl::cli::read_app "
                "instead.");
  const auto& options = cached_options<T, ProcessorsType>();
  return check_short_names(options)
      .and_then([&](bool) { return _parse(options); })
      .and_then([&](ParsedArgs _parsed) {
        return read_parsed_detailed<T, ProcessorsType>(std::move(_parsed),
                                                       options);
      });
}

/// Categorizes the arguments with `_parse`, which takes the options of T
/// (see make_options) and returns ParsedArgs, and turns them into a struct.
template <class T, class... Ps, class ParseFn>
rfl::Result<T> parse_and_read(const ParseFn& _parse) {
  using ProcessorsType = internal::cli_processors_t<Ps...>;
  static_assert(!internal::has_app_v<ProcessorsType>,
                "rfl::cli::App turns on --help and --version, which "
                "rfl::cli::read cannot report. Call rfl::cli::read_app "
                "instead.");
  const auto& options = cached_options<T, ProcessorsType>();
  return check_short_names(options)
      .and_then([&](bool) { return _parse(options); })
      .and_then([&](ParsedArgs _parsed) {
        return read_parsed<T, ProcessorsType>(std::move(_parsed), options);
      });
}

}  // namespace internal

/// Parses a sequence of command-line arguments into a struct using reflection.
/// Field names are automatically converted from snake_case to kebab-case for CLI arguments.
/// For example, a struct field named `host_name` will match the CLI argument `--host-name`.
/// Supports nested objects (e.g., --database.host), arrays (e.g., --ports=8080,8081),
/// positional arguments, and short flags.
///
/// Accepts a range of narrow or wide strings, such as
/// `std::vector<std::string>`, `std::span<const char* const>` or
/// `const wchar_t*[]`, or a braced list of narrow strings such as
/// `{"--port=8080"}`. Wide strings are converted to UTF-8. The number of
/// arguments is taken from the range. The parser keeps a std::string_view to
/// each narrow argument, so a range of narrow strings must be multi-pass and
/// must yield lvalue strings, pointers or string_views, not temporary
/// `std::string`s.
///
/// Every element is parsed: unlike `read(argc, argv)`, the first one is NOT
/// skipped as the program name. To pass main()'s arguments, drop argv[0]
/// yourself: `std::span(argv, argc)` would parse the program name as an
/// argument.
/// @tparam T The struct type to parse into
/// @tparam Ps Optional processors to apply during parsing, including
/// settings such as rfl::NoExtraFields and rfl::cli::AllowAbbreviations
/// @param _args The command-line arguments to parse
/// @return A Result containing the parsed struct or an error
template <class T, class... Ps,
          class R = std::initializer_list<std::string_view>>
  requires internal::StringRange<R> || internal::WideStringRange<R>
rfl::Result<T> read(R&& _args) {
  return internal::parse_and_read<T, Ps...>([&](const auto& _options) {
    return internal::parse_argv(std::forward<R>(_args), _options);
  });
}

/// Parses command-line arguments into a struct using reflection.
/// argv[0] is skipped as the program name. See the sequence overload above
/// for the supported syntax.
/// @tparam T The struct type to parse into
/// @tparam Ps Optional processors to apply during parsing, including
/// settings such as rfl::NoExtraFields and rfl::cli::AllowAbbreviations
/// @param argc Number of command-line arguments
/// @param argv Array of command-line argument strings
/// @return A Result containing the parsed struct or an error
template <class T, class... Ps>
rfl::Result<T> read(int argc, const char* const argv[]) {
  return internal::parse_and_read<T, Ps...>(
      [&](const auto& _options) {
        return internal::parse_argv(argc, argv, _options);
      });
}

template <class T, class... Ps>
rfl::Result<T> read(int argc, char* argv[]) {
  return internal::parse_and_read<T, Ps...>(
      [&](const auto& _options) {
        return internal::parse_argv(argc, argv, _options);
      });
}

/// Parses wide-character command-line arguments into a struct using reflection.
/// argv[0] is skipped as the program name. Wide strings are converted to
/// UTF-8 before parsing.
/// @tparam T The struct type to parse into
/// @tparam Ps Optional processors to apply during parsing
/// @param argc Number of wide-character command-line arguments
/// @param argv Array of wide-character argument strings
/// @return A Result containing the parsed struct or an error
template <class T, class... Ps>
rfl::Result<T> read(int argc, const wchar_t* const argv[]) {
  return internal::parse_and_read<T, Ps...>(
      [&](const auto& _options) {
        return internal::parse_argv(argc, argv, _options);
      });
}

template <class T, class... Ps>
rfl::Result<T> read(int argc, wchar_t* argv[]) {
  return read<T, Ps...>(argc, static_cast<const wchar_t* const*>(argv));
}

/// Like `rfl::cli::read`, returning `ReadResult<T>` containing both the parsed
/// struct and the sorted canonical names of explicitly provided options.
template <class T, class... Ps,
          class R = std::initializer_list<std::string_view>>
  requires internal::StringRange<R> || internal::WideStringRange<R>
rfl::Result<ReadResult<T>> read_detailed(R&& _args) {
  return internal::parse_and_read_detailed<T, Ps...>(
      [&](const auto& _options) {
        return internal::parse_argv(std::forward<R>(_args), _options);
      });
}

template <class T, class... Ps>
rfl::Result<ReadResult<T>> read_detailed(int argc, const char* const argv[]) {
  return internal::parse_and_read_detailed<T, Ps...>(
      [&](const auto& _options) {
        return internal::parse_argv(argc, argv, _options);
      });
}

template <class T, class... Ps>
rfl::Result<ReadResult<T>> read_detailed(int argc, char* argv[]) {
  return internal::parse_and_read_detailed<T, Ps...>(
      [&](const auto& _options) {
        return internal::parse_argv(argc, argv, _options);
      });
}

template <class T, class... Ps>
rfl::Result<ReadResult<T>> read_detailed(int argc,
                                         const wchar_t* const argv[]) {
  return internal::parse_and_read_detailed<T, Ps...>(
      [&](const auto& _options) {
        return internal::parse_argv(argc, argv, _options);
      });
}

template <class T, class... Ps>
rfl::Result<ReadResult<T>> read_detailed(int argc, wchar_t* argv[]) {
  return read_detailed<T, Ps...>(argc,
                                 static_cast<const wchar_t* const*>(argv));
}

/// Overlays command-line arguments onto an existing struct `_target` in place,
/// mutating ONLY the fields explicitly supplied on the command line and
/// preserving all other fields (defaults, presets, or values previously loaded
/// from YAML/JSON/INI).
template <class... Ps, class T,
          class R = std::initializer_list<std::string_view>>
  requires internal::StringRange<R> || internal::WideStringRange<R>
rfl::Result<ReadIntoResult> read_into(T& _target, R&& _args) {
  return internal::parse_and_read_into<T, Ps...>(
      _target, [&](const auto& _options) {
        return internal::parse_argv(std::forward<R>(_args), _options);
      });
}

template <class... Ps, class T>
rfl::Result<ReadIntoResult> read_into(T& _target, int argc,
                                      const char* const argv[]) {
  return internal::parse_and_read_into<T, Ps...>(
      _target, [&](const auto& _options) {
        return internal::parse_argv(argc, argv, _options);
      });
}

template <class... Ps, class T>
rfl::Result<ReadIntoResult> read_into(T& _target, int argc, char* argv[]) {
  return internal::parse_and_read_into<T, Ps...>(
      _target, [&](const auto& _options) {
        return internal::parse_argv(argc, argv, _options);
      });
}

template <class... Ps, class T>
rfl::Result<ReadIntoResult> read_into(T& _target, int argc,
                                      const wchar_t* const argv[]) {
  return internal::parse_and_read_into<T, Ps...>(
      _target, [&](const auto& _options) {
        return internal::parse_argv(argc, argv, _options);
      });
}

template <class... Ps, class T>
rfl::Result<ReadIntoResult> read_into(T& _target, int argc, wchar_t* argv[]) {
  return read_into<Ps...>(_target, argc,
                          static_cast<const wchar_t* const*>(argv));
}

}  // namespace rfl::cli

#endif
