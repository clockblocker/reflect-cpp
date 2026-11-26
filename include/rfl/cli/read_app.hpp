#ifndef RFL_CLI_READ_APP_HPP_
#define RFL_CLI_READ_APP_HPP_

#include <initializer_list>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "../Processors.hpp"
#include "../Result.hpp"
#include "../SnakeCaseToKebabCase.hpp"
#include "../internal/strings/utf8_conversions.hpp"
#include "App.hpp"
#include "Reader.hpp"
#include "options.hpp"
#include "parse_argv.hpp"
#include "read.hpp"
#include "resolve_args.hpp"
#include "usage.hpp"

namespace rfl::cli {

namespace internal {

/// Takes the built-in option `_key` out of `_args`, if it is enabled and
/// was given, and reads its value as the reader reads a bool.
/// @return Whether it was given and set
inline rfl::Result<bool> take_builtin_flag(
    const bool _enabled, std::map<std::string, std::string>* _args,
    const std::string_view _key) {
  if (!_enabled) {
    return false;
  }
  const auto it = _args->find(std::string(_key));
  if (it == _args->end()) {
    return false;
  }
  const auto value = std::move(it->second);
  _args->erase(it);
  // -h is held under "h", but named after the option it stands for.
  return parse_value<bool>(
      value, _key == help_short_option ? std::string(help_option)
                                       : std::string(_key));
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
  const auto builtin = add_builtin_options<T, ProcessorsType>(&options);
  const auto short_names = check_short_names(options);
  if (!short_names) {
    return error(short_names.error().what());
  }
  auto parsed = _parse(options);
  if (!parsed) {
    return error(parsed.error().what());
  }

  // Take the built-in options out, so that the struct never sees them.
  const auto help =
      take_builtin_flag(builtin.help, &parsed->named, help_option);
  const auto help_short = take_builtin_flag(
      builtin.help_short, &parsed->short_args, help_short_option);
  const auto version =
      take_builtin_flag(builtin.version, &parsed->named, version_option);
  for (const auto* r : {&help, &help_short, &version}) {
    if (!*r) {
      return error(r->error().what());
    }
  }

  if (*help || *help_short) {
    return usage<T, Ps...>(_program_name, true)
        .transform([](std::string _text) {
          return ResultType(
              HelpOrVersion{HelpOrVersion::Kind::help, std::move(_text)});
        });
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

/// Like rfl::cli::read_app, reading every element of a range of narrow or
/// wide strings (none is skipped as the program name), using `_program_name`
/// in the help and version output when the rfl::cli::App has no name.
template <class T, class... Ps,
          class R = std::initializer_list<std::string_view>>
  requires(internal::StringRange<R> || internal::WideStringRange<R>)
rfl::Result<std::variant<T, HelpOrVersion>> read_app(
    std::string_view _program_name, R&& _args) {
  return internal::parse_and_read_app<T, Ps...>(
      std::string(_program_name), [&](const auto& _options) {
        return internal::parse_argv(std::forward<R>(_args), _options);
      });
}

/// Like rfl::cli::read_app, reading every element of a range of narrow or
/// wide strings, using the rfl::cli::App's name as the program name.
template <class T, class... Ps,
          class R = std::initializer_list<std::string_view>>
  requires(internal::StringRange<R> || internal::WideStringRange<R>)
rfl::Result<std::variant<T, HelpOrVersion>> read_app(R&& _args) {
  using ProcessorsType = Processors<SnakeCaseToKebabCase, Ps...>;
  return read_app<T, Ps...>(internal::app_settings_t<ProcessorsType>::name,
                            std::forward<R>(_args));
}

}  // namespace rfl::cli

#endif
