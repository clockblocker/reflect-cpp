#ifndef RFL_CLI_READ_HPP_
#define RFL_CLI_READ_HPP_

#include <initializer_list>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "../Processors.hpp"
#include "../SnakeCaseToKebabCase.hpp"
#include "../internal/strings/utf8_conversions.hpp"
#include "App.hpp"
#include "KeepEmptyItems.hpp"
#include "ListDelimiter.hpp"
#include "Parser.hpp"
#include "Reader.hpp"
#include "options.hpp"
#include "parse_argv.hpp"
#include "resolve_args.hpp"
#include "usage.hpp"

namespace rfl::cli {

namespace internal {

static_assert(list_delimiter_v<Processors<>> == array_delimiter);

/// Resolves the parsed arguments and reads them into T.
template <class T, class ProcessorsType>
rfl::Result<T> read_parsed(ParsedArgs _parsed, const Options& _options) {
  return resolve_args(std::move(_parsed), _options)
      .and_then([](auto _args) -> rfl::Result<T> {
        const auto r =
            Reader(internal::list_delimiter_v<ProcessorsType>,
                   internal::keep_empty_items_v<ProcessorsType>);
        const auto var = CliVarType{&_args.values, "", std::nullopt,
                                    &_args.later_occurrences};
        return Parser<T, ProcessorsType>::read(r, var);
      });
}

/// Categorizes the arguments with `_parse`, which takes the options of T
/// (see make_options) and returns ParsedArgs, and turns them into a struct.
template <class T, class... Ps, class ParseFn>
rfl::Result<T> parse_and_read(const ParseFn& _parse) {
  using ProcessorsType = Processors<SnakeCaseToKebabCase, Ps...>;
  static_assert(!internal::has_app_v<ProcessorsType>,
                "rfl::cli::App turns on --help and --version, which "
                "rfl::cli::read cannot report. Call rfl::cli::read_app "
                "instead.");
  const auto options = make_options<T, ProcessorsType>();
  return check_short_names(options)
      .and_then([&](bool) { return _parse(options); })
      .and_then([&](ParsedArgs _parsed) {
        return read_parsed<T, ProcessorsType>(std::move(_parsed), options);
      });
}

/// Takes the built-in option `_key` out of `_args`, if it is enabled and
/// was given.
/// @return Whether it was given and set
inline rfl::Result<bool> take_builtin_flag(
    const bool _enabled, std::map<std::string, std::string>* _args,
    const std::string_view _key, const std::string& _option) {
  if (!_enabled) {
    return false;
  }
  const auto it = _args->find(std::string(_key));
  if (it == _args->end()) {
    return false;
  }
  const auto value = std::move(it->second);
  _args->erase(it);
  return builtin_flag_is_set(value, _option);
}

/// Like parse_and_read, with the built-in options of the rfl::cli::App in
/// Ps. If the arguments ask for the help or the version, returns its text
/// instead of reading the struct.
template <class T, class... Ps, class ParseFn>
rfl::Result<std::variant<T, HelpOrVersion>> parse_and_read_app(
    const std::string& _program_name, const ParseFn& _parse) {
  using ProcessorsType = Processors<SnakeCaseToKebabCase, Ps...>;
  using ResultType = std::variant<T, HelpOrVersion>;
  auto options = make_options<T, ProcessorsType>();
  const auto builtin = add_builtin_options<ProcessorsType>(&options);
  const auto short_names = check_short_names(options);
  if (!short_names) {
    return error(short_names.error().what());
  }
  auto parsed = _parse(options);
  if (!parsed) {
    return error(parsed.error().what());
  }

  // Take the built-in options out, so that the struct never sees them.
  const auto help = take_builtin_flag(builtin.help, &parsed->named,
                                      help_option, "--help");
  const auto help_short =
      take_builtin_flag(builtin.help_short, &parsed->short_args,
                        help_short_option, "-h");
  const auto version = take_builtin_flag(builtin.version, &parsed->named,
                                         version_option, "--version");
  for (const auto* r : {&help, &help_short, &version}) {
    if (!*r) {
      return error(r->error().what());
    }
  }

  if (*help || *help_short) {
    return ResultType(HelpOrVersion{HelpOrVersion::Kind::help,
                                    usage<T, Ps...>(_program_name, true)});
  }
  if (*version) {
    return ResultType(
        HelpOrVersion{HelpOrVersion::Kind::version,
                      app_version_text<ProcessorsType>(_program_name)});
  }
  return read_parsed<T, ProcessorsType>(std::move(*parsed), options)
      .transform([](T&& _t) { return ResultType(std::move(_t)); });
}

/// argv[0], or an empty string if there is none.
inline std::string program_name(int argc, const char* const argv[]) {
  return argc > 0 && argv && argv[0] ? std::string(argv[0]) : std::string();
}

/// argv[0] converted to UTF-8, or an empty string if there is none or it
/// cannot be converted.
inline std::string program_name(int argc, const wchar_t* const argv[]) {
  if (argc <= 0 || !argv || !argv[0]) {
    return std::string();
  }
  return rfl::internal::strings::wstring_to_utf8(std::wstring(argv[0]))
      .value_or(std::string());
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
/// settings such as rfl::NoExtraFields and rfl::cli::NoAbbreviations
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
/// settings such as rfl::NoExtraFields and rfl::cli::NoAbbreviations
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
  return read<T, Ps...>(argc, static_cast<const char* const*>(argv));
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

/// Like rfl::cli::read, but with `--help` and `--version` turned on by an
/// rfl::cli::App in Ps (see App.hpp). If the command line asks for one of
/// them, the result holds an rfl::cli::HelpOrVersion with the text to print,
/// and the struct is not read, so missing or invalid fields don't matter.
/// The help wins if both are given. Otherwise, it holds the struct, read
/// exactly as rfl::cli::read would. Errors are errors either way.
///   const auto result = rfl::cli::read_app<Config, MyApp>(argc, argv);
///   if (!result) { ... result.error().what() ... return 2; }
///   if (const auto* info = std::get_if<rfl::cli::HelpOrVersion>(&*result)) {
///     std::cout << info->text;
///     return 0;
///   }
///   const auto& config = std::get<Config>(*result);
/// `--help` and `--version` are found once argv is split into options, so
/// an earlier token that fails to parse (such as an option that is missing
/// its value) is still an error.
/// @tparam T The struct type to parse into
/// @tparam Ps Optional processors and settings, as for rfl::cli::read, and
/// at most one rfl::cli::App
/// @param argc Number of command-line arguments
/// @param argv Array of command-line argument strings. argv[0] is the
/// program name, which the help and the version show if the rfl::cli::App
/// has no name.
/// @return The struct, or the help or version text, or an error
template <class T, class... Ps>
rfl::Result<std::variant<T, HelpOrVersion>> read_app(int argc,
                                                     const char* const argv[]) {
  return internal::parse_and_read_app<T, Ps...>(
      internal::program_name(argc, argv),
      [&](const auto& _options) {
        return internal::parse_argv(argc, argv, _options);
      });
}

template <class T, class... Ps>
rfl::Result<std::variant<T, HelpOrVersion>> read_app(int argc, char* argv[]) {
  return read_app<T, Ps...>(argc, static_cast<const char* const*>(argv));
}

/// Like rfl::cli::read_app, for wide-character arguments, which are
/// converted to UTF-8 first.
template <class T, class... Ps>
rfl::Result<std::variant<T, HelpOrVersion>> read_app(
    int argc, const wchar_t* const argv[]) {
  return internal::parse_and_read_app<T, Ps...>(
      internal::program_name(argc, argv),
      [&](const auto& _options) {
        return internal::parse_argv(argc, argv, _options);
      });
}

template <class T, class... Ps>
rfl::Result<std::variant<T, HelpOrVersion>> read_app(int argc,
                                                     wchar_t* argv[]) {
  return read_app<T, Ps...>(argc, static_cast<const wchar_t* const*>(argv));
}

}  // namespace rfl::cli

#endif
