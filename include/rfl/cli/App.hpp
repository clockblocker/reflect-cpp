#ifndef RFL_CLI_APP_HPP_
#define RFL_CLI_APP_HPP_

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

#include "../Processors.hpp"
#include "../Result.hpp"
#include "../internal/StringLiteral.hpp"
#include "options.hpp"

namespace rfl::cli {

/// A setting that turns on `--help` and `--version` in rfl::cli::read_app,
/// and adds a header and their rows to rfl::cli::usage. Without it, `--help`
/// and `--version` are ordinary options, as they always were.
///   using MyApp = rfl::cli::App<"copy", "1.2.0", "Copies files.">;
///   rfl::cli::read_app<Config, MyApp>(argc, argv);
///   rfl::cli::usage<Config, MyApp>(argv[0], true);
/// - `-h` and `--help` ask for the help, `--version` for the version. They
///   are flags in the option table, so they can be abbreviated (`--hel`) and
///   grouped (`-vh`) like any other, and `--` ends them.
/// - A field takes precedence over them: if a field has rfl::Short<"h", ...>,
///   `-h` is that field's and the help is `--help` only. A field named
///   `help` (after processors) turns the built-in help off, and a field named
///   `version` turns `--version` off. Such a field is read like any other.
/// - An empty version leaves out `--version`.
/// This is a marker type (doesn't modify data) that only rfl::cli::read_app
/// and rfl::cli::usage look at. Other formats ignore it. rfl::cli::read
/// rejects it at compile time, because its result has no room for a help
/// request.
/// @tparam _name The name shown in the header and before the version. If it
/// is empty, the program name (argv[0]) is used.
/// @tparam _version The version shown by `--version` and in the header
/// @tparam _about A one-line description, shown in the header
template <rfl::internal::StringLiteral _name = "",
          rfl::internal::StringLiteral _version = "",
          rfl::internal::StringLiteral _about = "">
struct App {
  static constexpr std::string_view name = _name.string_view();
  static constexpr std::string_view version = _version.string_view();
  static constexpr std::string_view about = _about.string_view();

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
};

template <rfl::internal::StringLiteral _name,
          rfl::internal::StringLiteral _version,
          rfl::internal::StringLiteral _about>
struct app_settings<App<_name, _version, _about>> {
  static constexpr size_t count = 1;
  static constexpr std::string_view name = App<_name, _version, _about>::name;
  static constexpr std::string_view version =
      App<_name, _version, _about>::version;
  static constexpr std::string_view about =
      App<_name, _version, _about>::about;
};

/// The first of Ps that is an rfl::cli::App, or void.
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

/// The built-in options that rfl::cli::App adds: those that no field takes.
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

/// Whether a field of the struct is called `--_name`, or has it as the path
/// of a nested struct or map. Keys of rfl::ExtraFields do not count: they are
/// not known in advance.
inline bool is_field_name(const Options& _options, const std::string& _name) {
  if (_options.long_options.count(_name)) {
    return true;
  }
  const auto prefix = _name + path_separator;
  const auto starts_with_prefix = [&](const auto& _map) {
    const auto it = _map.lower_bound(prefix);
    return it != _map.end() && it->first.starts_with(prefix);
  };
  return starts_with_prefix(_options.long_options)
         || starts_with_prefix(_options.open_prefixes);
}

/// Adds the built-in options of the rfl::cli::App in ProcessorsType to
/// _options, unless a field takes their names. They are flags, so the parser
/// handles them as it does any flag.
/// @return Which of them were added
template <class ProcessorsType>
BuiltinOptions add_builtin_options(Options* _options) {
  using Settings = app_settings_t<ProcessorsType>;
  auto builtin = BuiltinOptions{};
  if constexpr (Settings::count > 0) {
    const auto help = std::string(help_option);
    const auto help_short = std::string(help_short_option);
    const auto version = std::string(version_option);
    builtin.help = !is_field_name(*_options, help);
    builtin.help_short =
        builtin.help && !_options->short_options.count(help_short);
    builtin.version =
        !Settings::version.empty() && !is_field_name(*_options, version);
    if (builtin.help) {
      _options->long_options[help] = Option{OptionKind::flag};
    }
    if (builtin.help_short) {
      _options->short_options[help_short] = help;
    }
    if (builtin.version) {
      _options->long_options[version] = Option{OptionKind::flag};
    }
  }
  return builtin;
}

/// Reads the value of a built-in flag the way the reader reads a bool:
/// empty (given without a value), "true" or "1" turn it on, "false" or "0"
/// leave it off.
inline rfl::Result<bool> builtin_flag_is_set(const std::string& _value,
                                             const std::string& _option) {
  if (_value.empty() || _value == "true" || _value == "1") {
    return true;
  }
  if (_value == "false" || _value == "0") {
    return false;
  }
  return error("Could not cast '" + _value + "' to boolean for '" + _option
               + "'.");
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
      header += Settings::name.empty() ? _program_name
                                       : std::string(Settings::name);
      if (!Settings::version.empty()) {
        header += " " + std::string(Settings::version);
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
/// name is empty.
template <class ProcessorsType>
std::string app_version_text(const std::string& _program_name) {
  using Settings = app_settings_t<ProcessorsType>;
  return (Settings::name.empty() ? _program_name
                                 : std::string(Settings::name))
         + " " + std::string(Settings::version) + '\n';
}

}  // namespace internal

}  // namespace rfl::cli

#endif
