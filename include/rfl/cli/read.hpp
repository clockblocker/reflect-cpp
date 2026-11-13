#ifndef RFL_CLI_READ_HPP_
#define RFL_CLI_READ_HPP_

#include <span>

#include "../Processors.hpp"
#include "../SnakeCaseToKebabCase.hpp"
#include "../internal/strings/utf8_conversions.hpp"
#include "Parser.hpp"
#include "Reader.hpp"
#include "parse_argv.hpp"
#include "resolve_args.hpp"

namespace rfl::cli {

/// Parses command-line arguments into a struct using reflection.
/// Field names are automatically converted from snake_case to kebab-case for CLI arguments.
/// For example, a struct field named `host_name` will match the CLI argument `--host-name`.
/// Supports nested objects (e.g., --database.host), arrays (e.g., --ports=8080,8081),
/// positional arguments, and short flags.
/// Every element of `_args` is parsed: unlike the argc/argv overload, the
/// first one is not skipped as the program name. Use this to parse a subset
/// of the arguments, or arguments that do not come straight from main().
/// @tparam T The struct type to parse into
/// @tparam Ps Optional processors to apply during parsing
/// @param _args The command-line arguments to parse
/// @return A Result containing the parsed struct or an error
template <class T, class... Ps>
rfl::Result<T> read(std::span<const char* const> _args) {
  using ProcessorsType = Processors<SnakeCaseToKebabCase, Ps...>;
  using NT = rfl::internal::processed_t<T, ProcessorsType>;
  constexpr bool negative_numbers_are_options =
      internal::has_numeric_short_name<NT>(
          std::make_index_sequence<NT::size()>{});
  return internal::parse_args(_args, negative_numbers_are_options)
      .and_then(resolve_args<T, ProcessorsType>)
      .and_then([](auto _resolved) -> rfl::Result<T> {
        const auto r = Reader();
        const auto var = CliVarType{&_resolved, "", std::nullopt};
        return Parser<T, ProcessorsType>::read(r, var);
      });
}

/// Parses command-line arguments into a struct using reflection.
/// argv[0] is skipped as the program name. Takes both `char**` and
/// `const char**`.
/// @tparam T The struct type to parse into
/// @tparam Ps Optional processors to apply during parsing
/// @param argc Number of command-line arguments
/// @param argv Array of command-line argument strings
/// @return A Result containing the parsed struct or an error
template <class T, class... Ps>
rfl::Result<T> read(int argc, const char* const argv[]) {
  if (argc < 0 || (argc > 0 && !argv)) {
    return error("Invalid argc/argv.");
  }
  if (argc <= 1) {
    return read<T, Ps...>(std::span<const char* const>());
  }
  return read<T, Ps...>(std::span(argv + 1, static_cast<size_t>(argc - 1)));
}

/// Parses wide-character command-line arguments into a struct using reflection.
/// Field names are automatically converted from snake_case to kebab-case for CLI arguments.
/// Wide strings are converted to UTF-8 before parsing.
/// @tparam T The struct type to parse into
/// @tparam Ps Optional processors to apply during parsing
/// @param argc Number of wide-character command-line arguments
/// @param argv Array of wide-character argument strings
/// @return A Result containing the parsed struct or an error
template <class T, class... Ps>
rfl::Result<T> read(int argc, const wchar_t* const argv[]) {
  if (argc < 0 || (argc > 0 && !argv)) {
    return error("Invalid argc/argv.");
  }
  std::vector<std::string> narrow_argv;
  narrow_argv.reserve(argc);
  std::vector<char*> narrow_argv_ptrs;
  narrow_argv_ptrs.reserve(argc);
  for (int i = 0; i < argc; ++i) {
    const auto str = rfl::internal::strings::wstring_to_utf8(argv[i]);
    if (!str) {
      return error("Could not convert argument " + std::to_string(i)
                   + " from wide to UTF-8.");
    }
    narrow_argv.emplace_back(std::move(*str));
    narrow_argv_ptrs.push_back(narrow_argv.back().data());
  }
  return read<T, Ps...>(argc, narrow_argv_ptrs.data());
}

}  // namespace rfl::cli

#endif
