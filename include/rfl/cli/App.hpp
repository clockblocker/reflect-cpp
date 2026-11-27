#ifndef RFL_CLI_APP_HPP_
#define RFL_CLI_APP_HPP_

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Processors.hpp"
#include "../internal/StringLiteral.hpp"
#include "options.hpp"

namespace rfl::cli {

/// Which built-in help options rfl::cli::App adds.
enum class BuiltinHelp {
  /// `-h` and `--help`
  short_and_long,
  /// `--help` only, so that a field can have rfl::Short<"h", ...>
  long_only,
  /// None, so that a field can be called `help`
  none
};

/// A setting that turns on `--help` and `--version` in rfl::cli::read_app,
/// and adds a header and their rows to rfl::cli::usage. Without it, `--help`
/// and `--version` are ordinary options, as they always were.
///   using MyApp = rfl::cli::App<"copy", "1.2.0", "Copies files.">;
///   rfl::cli::read_app<Config, MyApp>(argc, argv);
///   rfl::cli::usage<Config, MyApp>(argv[0], true);
/// - `-h` and `--help` ask for the help, `--version` for the version. They
///   are flags in the option table, so they can be grouped (`-vh`) like any
///   other, and `--` ends them.
/// - A field that takes one of their names is a compile-time error, as in
///   argparse ("conflicting option string: -h"): rfl::Short<"h", ...>
///   wherever it sits among the field's wrappers, a field named `help` or,
///   with a version, `version` (after processors). Opt out of them instead:
///   BuiltinHelp::long_only leaves `-h` to a field, BuiltinHelp::none leaves
///   out the help, like argparse's add_help=False, and an empty version
///   leaves out `--version`.
/// This is a marker type (doesn't modify data) that only rfl::cli::read_app
/// and rfl::cli::usage look at. Other formats ignore it. rfl::cli::read
/// rejects it at compile time, because its result has no room for a help
/// request.
/// @tparam _name The name shown in the header and before the version. If it
/// is empty, the program name (argv[0]) is used.
/// @tparam _version The version shown by `--version` and in the header
/// @tparam _about A one-line description, shown in the header
/// @tparam _help Which built-in help options to add
template <rfl::internal::StringLiteral _name = "",
          rfl::internal::StringLiteral _version = "",
          rfl::internal::StringLiteral _about = "",
          BuiltinHelp _help = BuiltinHelp::short_and_long>
struct App {
  static constexpr std::string_view name = _name.string_view();
  static constexpr std::string_view version = _version.string_view();
  static constexpr std::string_view about = _about.string_view();
  static constexpr BuiltinHelp help = _help;

  /// Identity process function - returns the named tuple unchanged.
  /// @tparam StructType The struct type being processed
  /// @param _named_tuple The named tuple representation of the struct
  /// @return The same named tuple (unchanged)
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

/// What rfl::cli::read_app returns instead of the struct when the command
/// line asks for the help or the version. The caller prints `text` (to
/// stdout, usually) and exits successfully.
struct HelpOrVersion {
  enum class Kind { help, version };

  Kind kind;

  /// The help (rfl::cli::usage with the details) or the version line, ending
  /// with a newline.
  std::string text;
};

namespace internal {

template <class T>
struct app_settings {
  static constexpr size_t count = 0;
  static constexpr std::string_view name = "";
  static constexpr std::string_view version = "";
  static constexpr std::string_view about = "";
  static constexpr BuiltinHelp help = BuiltinHelp::none;
};

template <rfl::internal::StringLiteral _name,
          rfl::internal::StringLiteral _version,
          rfl::internal::StringLiteral _about, BuiltinHelp _help>
struct app_settings<App<_name, _version, _about, _help>> {
  using AppType = App<_name, _version, _about, _help>;
  static constexpr size_t count = 1;
  static constexpr std::string_view name = AppType::name;
  static constexpr std::string_view version = AppType::version;
  static constexpr std::string_view about = AppType::about;
  static constexpr BuiltinHelp help = AppType::help;
};

/// The first of Ps that is or contains an rfl::cli::App, or void.
template <class... Ps>
struct find_app {
  using type = void;
};

template <class P, class... Ps>
struct find_app<P, Ps...> {
  using type = std::conditional_t<(app_settings<P>::count > 0), P,
                                  typename find_app<Ps...>::type>;
};

template <class... Ps>
struct app_settings<Processors<Ps...>>
    : app_settings<typename find_app<Ps...>::type> {
  static constexpr size_t count = (app_settings<Ps>::count + ... + 0);
  static_assert(count <= 1, "Pass at most one rfl::cli::App.");
};

/// The rfl::cli::App in the processors, if any.
template <class T>
using app_settings_t =
    app_settings<std::remove_cvref_t<std::remove_pointer_t<T>>>;

/// Whether the processors contain rfl::cli::App.
template <class T>
constexpr bool has_app_v = app_settings_t<T>::count > 0;

/// The built-in options that rfl::cli::App adds.
struct BuiltinOptions {
  /// `--help`
  bool help = false;
  /// `-h`, for `--help`
  bool help_short = false;
  /// `--version`
  bool version = false;
};

/// The long name of the built-in help.
inline constexpr std::string_view help_option = "help";

/// The short name of the built-in help.
inline constexpr std::string_view help_short_option = "h";

/// The long name of the built-in version.
inline constexpr std::string_view version_option = "version";

/// Whether a top-level field of the NamedTuple NT is called `_name`.
template <class NT, size_t... _is>
consteval bool has_field_named(const std::string_view _name,
                               std::index_sequence<_is...>) {
  return ((field_info<NT, _is>::name() == _name) || ...);
}

/// Whether a top-level field of the NamedTuple NT has the short name
/// `_name`, wherever rfl::Short sits among its wrappers.
template <class NT, size_t... _is>
consteval bool has_short_named(const std::string_view _name,
                               std::index_sequence<_is...>) {
  return ((field_info<NT, _is>::is_short
           && get_short_name<NT, _is>().string_view() == _name)
          || ...);
}

/// Adds the built-in options of the rfl::cli::App in ProcessorsType to
/// _options. They are flags, so the parser handles them as it does any
/// flag. A field of T that takes one of their names does not compile.
/// @return Which of them were added
template <class T, class ProcessorsType>
BuiltinOptions add_builtin_options(Options* _options) {
  using Settings = app_settings_t<ProcessorsType>;
  auto builtin = BuiltinOptions{};
  if constexpr (Settings::count > 0) {
    builtin.help = Settings::help != BuiltinHelp::none;
    builtin.help_short = Settings::help == BuiltinHelp::short_and_long;
    builtin.version = !Settings::version.empty();
    using U = unwrapped_option_t<T, ProcessorsType>;
    if constexpr (is_option_struct_v<U>) {
      using NT = option_fields_t<U, ProcessorsType>;
      constexpr auto fields = std::make_index_sequence<NT::size()>();
      static_assert(Settings::help == BuiltinHelp::none
                        || !has_field_named<NT>(help_option, fields),
                    "A field named 'help' clashes with the built-in --help "
                    "of rfl::cli::App. Rename the field, or pass "
                    "rfl::cli::BuiltinHelp::none to rfl::cli::App.");
      static_assert(Settings::help != BuiltinHelp::short_and_long
                        || !has_short_named<NT>(help_short_option, fields),
                    "A field with rfl::Short<\"h\", ...> clashes with the "
                    "built-in -h of rfl::cli::App. Use another short name, "
                    "or pass rfl::cli::BuiltinHelp::long_only to "
                    "rfl::cli::App to keep only --help.");
      static_assert(Settings::version.empty()
                        || !has_field_named<NT>(version_option, fields),
                    "A field named 'version' clashes with the built-in "
                    "--version of rfl::cli::App. Rename the field, or give "
                    "rfl::cli::App no version.");
    }
    if (builtin.help) {
      _options->long_options[std::string(help_option)] =
          Option{OptionKind::flag};
    }
    if (builtin.help_short) {
      _options->short_options[std::string(help_short_option)] =
          std::string(help_option);
    }
    if (builtin.version) {
      _options->long_options[std::string(version_option)] =
          Option{OptionKind::flag};
    }
  }
  return builtin;
}

/// The header of the help: "name version" and the description, each on a
/// line of its own if it is given, and a blank line. Empty if the
/// processors contain no rfl::cli::App or it has none of them.
template <class ProcessorsType>
std::string app_header(const std::string& _program_name) {
  using Settings = app_settings_t<ProcessorsType>;
  auto header = std::string();
  if constexpr (Settings::count > 0) {
    if (!Settings::name.empty() || !Settings::version.empty()) {
      const auto title = Settings::name.empty() ? _program_name
                                                : std::string(Settings::name);
      header += title;
      if (!Settings::version.empty()) {
        if (!title.empty()) {
          header += ' ';
        }
        header += std::string(Settings::version);
      }
      header += '\n';
    }
    if (!Settings::about.empty()) {
      header += std::string(Settings::about) + '\n';
    }
    if (!header.empty()) {
      header += '\n';
    }
  }
  return header;
}

/// What `--version` prints: "name version", or "program version" if the
/// name is empty (or just "version" if both are empty).
template <class ProcessorsType>
std::string app_version_text(const std::string& _program_name) {
  using Settings = app_settings_t<ProcessorsType>;
  const auto title = Settings::name.empty() ? _program_name
                                            : std::string(Settings::name);
  if (title.empty()) {
    return std::string(Settings::version) + '\n';
  }
  return title + " " + std::string(Settings::version) + '\n';
}

}  // namespace internal

}  // namespace rfl::cli

#endif
