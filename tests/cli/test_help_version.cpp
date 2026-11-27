#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <variant>
#include <vector>

// `--help` and `--version` in rfl::cli::read_app, turned on by
// rfl::cli::App, and their rows in rfl::cli::usage.
namespace test_help_version {

template <class T, class... Ps>
rfl::Result<std::variant<T, rfl::cli::HelpOrVersion>> read_app(
    std::vector<const char*> _args) {
  _args.insert(_args.begin(), "app");
  return rfl::cli::read_app<T, Ps...>(static_cast<int>(_args.size()),
                                      const_cast<char**>(_args.data()));
}

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "app");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

/// The help or version text, or "(struct)" or the error.
template <class T>
std::string text_of(
    const rfl::Result<std::variant<T, rfl::cli::HelpOrVersion>>& _result) {
  if (!_result) {
    return std::string("error: ") + _result.error().what();
  }
  const auto* info = std::get_if<rfl::cli::HelpOrVersion>(&*_result);
  return info ? info->text : std::string("(struct)");
}

template <class T>
std::optional<rfl::cli::HelpOrVersion::Kind> kind_of(
    const rfl::Result<std::variant<T, rfl::cli::HelpOrVersion>>& _result) {
  if (!_result) {
    return std::nullopt;
  }
  const auto* info = std::get_if<rfl::cli::HelpOrVersion>(&*_result);
  return info ? std::optional(info->kind) : std::nullopt;
}

using Kind = rfl::cli::HelpOrVersion::Kind;

struct Copy {
  rfl::Positional<rfl::Description<"File to copy", std::string>> source;
  rfl::Short<"o", rfl::Description<"Where to copy it", std::string>> output;
  rfl::Short<"v", std::optional<bool>> verbose;
  rfl::Short<"j", std::optional<int>> jobs;
};

using CopyApp = rfl::cli::App<"copy", "1.2.0", "Copies a file.">;

constexpr const char* copy_help =
    "copy 1.2.0\n"
    "Copies a file.\n"
    "\n"
    "Usage: app [-h|--help] [--version] <source> -o|--output=<value> "
    "[-v|--verbose[=<bool>]] [-j|--jobs=<value>]\n"
    "\n"
    "Arguments:\n"
    "  -h, --help              Show this help\n"
    "  --version               Show the version\n"
    "  <source>                File to copy. Required.\n"
    "  -o, --output=<value>    Where to copy it. Required.\n"
    "  -v, --verbose[=<bool>]\n"
    "  -j, --jobs=<value>\n";

TEST(cli, test_help_version_usage) {
  EXPECT_EQ((rfl::cli::usage<Copy, CopyApp>("app", true).value()), copy_help);

  // The usage line has no header.
  EXPECT_EQ((rfl::cli::usage<Copy, CopyApp>("app").value()),
            "Usage: app [-h|--help] [--version] <source> -o|--output=<value> "
            "[-v|--verbose[=<bool>]] [-j|--jobs=<value>]\n");

  // Without rfl::cli::App, there are no such rows.
  EXPECT_EQ(rfl::cli::usage<Copy>("app").value(),
            "Usage: app <source> -o|--output=<value> "
            "[-v|--verbose[=<bool>]] [-j|--jobs=<value>]\n");
}

TEST(cli, test_help_version_help) {
  // The required fields are missing, but the help is shown.
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"--help"}, {"-h"}, {"-vh"}, {"in.txt", "-h"},
           {"--version", "--help"}, {"--help=true"}, {"-h", "--jobs=x"}}) {
    const auto result = read_app<Copy, CopyApp>(args);
    EXPECT_EQ(kind_of(result), Kind::help) << args.front();
    EXPECT_EQ(text_of(result), copy_help) << args.front();
  }
}

TEST(cli, test_help_version_version) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"--version"}, {"in.txt", "--version", "-v"}}) {
    const auto result = read_app<Copy, CopyApp>(args);
    EXPECT_EQ(kind_of(result), Kind::version) << args.front();
    EXPECT_EQ(text_of(result), "copy 1.2.0\n") << args.front();
  }
}

TEST(cli, test_help_version_reads_the_struct) {
  const auto result =
      read_app<Copy, CopyApp>({"in.txt", "-o", "out.txt", "-j", "4"});
  ASSERT_TRUE(result) << result.error().what();
  const auto* copy = std::get_if<Copy>(&*result);
  ASSERT_TRUE(copy);
  EXPECT_EQ(copy->source.value().value(), "in.txt");
  EXPECT_EQ(copy->output.value().value(), "out.txt");
  EXPECT_EQ(copy->jobs.value(), 4);

  // `--help=false` does not ask for the help.
  const auto not_help =
      read_app<Copy, CopyApp>({"in.txt", "-o", "out.txt", "--help=false"});
  ASSERT_TRUE(not_help) << not_help.error().what();
  EXPECT_TRUE(std::holds_alternative<Copy>(*not_help));
}

TEST(cli, test_help_version_errors) {
  // Errors are errors, not help.
  EXPECT_EQ(text_of(read_app<Copy, CopyApp>({"in.txt"})),
            "error: Field named 'output' not found.");
  EXPECT_EQ(text_of(read_app<Copy, CopyApp>({"--help=yes"})),
            "error: Could not cast 'yes' to boolean for key 'help'.");
  // An earlier token that fails to parse is an error.
  EXPECT_EQ(text_of(read_app<Copy, CopyApp>({"-o", "--help"})),
            "error: Missing value for option '-o'.");
  // After `--`, everything is positional.
  EXPECT_EQ(text_of(read_app<Copy, CopyApp>({"-o", "x", "--", "--help"})),
            "(struct)");
}

TEST(cli, test_help_version_off_by_default) {
  // Without rfl::cli::App, `--help` is an option like any other: ignored
  // unless rfl::NoExtraFields is passed.
  const auto ignored = read<Copy>({"in.txt", "-o", "out.txt", "--help"});
  ASSERT_TRUE(ignored) << ignored.error().what();
  const auto rejected =
      read<Copy, rfl::NoExtraFields>({"in.txt", "-o", "out.txt", "--help"});
  ASSERT_FALSE(rejected);
  EXPECT_NE(rejected.error().what().find("help"), std::string::npos)
      << rejected.error().what();
  EXPECT_FALSE(read<Copy>({"-h"}));

  // read_app without rfl::cli::App is read.
  EXPECT_EQ(text_of(read_app<Copy>({"in.txt", "-o", "out.txt", "--help"})),
            "(struct)");
}

// A field that takes the name of a built-in option does not compile: a
// field with rfl::Short<"h", ...> (wherever it sits among the field's
// wrappers), a field named `help`, or, with a version, a field named
// `version`. rfl::cli::App opts out of them instead.

// rfl::cli::BuiltinHelp::long_only leaves `-h` to a field: the help is
// `--help` only.
struct WithHostShort {
  rfl::Short<"h", rfl::Description<"Host to connect to", std::string>> host;
};

using LongHelpApp =
    rfl::cli::App<"client", "", "", rfl::cli::BuiltinHelp::long_only>;

TEST(cli, test_help_version_long_only) {
  EXPECT_EQ((rfl::cli::usage<WithHostShort, LongHelpApp>("client", true)
                 .value()),
            "client\n"
            "\n"
            "Usage: client [--help] -h|--host=<value>\n"
            "\n"
            "Arguments:\n"
            "  --help              Show this help\n"
            "  -h, --host=<value>  Host to connect to. Required.\n");

  const auto host = read_app<WithHostShort, LongHelpApp>({"-h", "example.com"});
  ASSERT_TRUE(host) << host.error().what();
  EXPECT_EQ(std::get<WithHostShort>(*host).host.value().value(),
            "example.com");
  EXPECT_EQ(kind_of(read_app<WithHostShort, LongHelpApp>({"--help"})),
            Kind::help);

  // No version: `--version` is an unknown option.
  const auto ver_rejected =
      read_app<WithHostShort, LongHelpApp, rfl::NoExtraFields>({"--version"});
  ASSERT_FALSE(ver_rejected);
  EXPECT_NE(ver_rejected.error().what().find("version"), std::string::npos)
      << ver_rejected.error().what();
}

// The same with rfl::Short inside other wrappers.
struct WithWrappedHostShort {
  rfl::Description<"Host to connect to", rfl::Short<"h", std::string>> host;
};

TEST(cli, test_help_version_long_only_wrapped_short_h) {
  EXPECT_EQ((rfl::cli::usage<WithWrappedHostShort, LongHelpApp>("client")
                 .value()),
            "Usage: client [--help] -h|--host=<value>\n");
  const auto host =
      read_app<WithWrappedHostShort, LongHelpApp>({"-h", "example.com"});
  ASSERT_TRUE(host) << host.error().what();
  EXPECT_EQ(std::get<WithWrappedHostShort>(*host).host.value().value(),
            "example.com");
}

// rfl::cli::BuiltinHelp::none leaves out the help, like argparse's
// add_help=False, so a field can be called `help`. Here it is
// `--help=<topic>`.
struct WithHelpField {
  rfl::Description<"Show help on a topic", std::optional<std::string>> help;
  rfl::Short<"n", std::optional<int>> count;
};

TEST(cli, test_help_version_without_help) {
  using MyApp = rfl::cli::App<"tool", "0.1", "", rfl::cli::BuiltinHelp::none>;
  EXPECT_EQ((rfl::cli::usage<WithHelpField, MyApp>("tool", true).value()),
            "tool 0.1\n"
            "\n"
            "Usage: tool [--version] [--help=<value>] [-n|--count=<value>]\n"
            "\n"
            "Arguments:\n"
            "  --version            Show the version\n"
            "  --help=<value>       Show help on a topic\n"
            "  -n, --count=<value>\n");

  const auto topic = read_app<WithHelpField, MyApp>({"--help=lists"});
  ASSERT_TRUE(topic) << topic.error().what();
  EXPECT_EQ(std::get<WithHelpField>(*topic).help.value().value(), "lists");
  // `-h` is not the help either.
  EXPECT_FALSE((read_app<WithHelpField, MyApp>({"-h"})));
  EXPECT_EQ(text_of(read_app<WithHelpField, MyApp>({"--version"})),
            "tool 0.1\n");
}

// Without a version, there is no `--version`, so a field can be called
// `version`.
struct WithVersionField {
  std::optional<int> version;
};

TEST(cli, test_help_version_without_version) {
  using MyApp = rfl::cli::App<"tool">;
  const auto result = read_app<WithVersionField, MyApp>({"--version=3"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(std::get<WithVersionField>(*result).version, 3);
  EXPECT_EQ((rfl::cli::usage<WithVersionField, MyApp>("tool").value()),
            "Usage: tool [-h|--help] [--version=<value>]\n");
}

TEST(cli, test_help_version_without_name) {
  // Without a name, the program name is used.
  using MyApp = rfl::cli::App<"", "2.0", "", rfl::cli::BuiltinHelp::long_only>;
  EXPECT_EQ(text_of(read_app<WithHostShort, MyApp>({"--version"})),
            "app 2.0\n");
  EXPECT_EQ((rfl::cli::usage<WithHostShort, MyApp>("app", true).value()),
            "app 2.0\n"
            "\n"
            "Usage: app [--help] [--version] -h|--host=<value>\n"
            "\n"
            "Arguments:\n"
            "  --help              Show this help\n"
            "  --version           Show the version\n"
            "  -h, --host=<value>  Host to connect to. Required.\n");

  // Without anything, there is no header and no `--version`.
  EXPECT_EQ((rfl::cli::usage<WithVersionField, rfl::cli::App<>>("app", true).value()),
            "Usage: app [-h|--help] [--version=<value>]\n"
            "\n"
            "Arguments:\n"
            "  -h, --help         Show this help\n"
            "  --version=<value>\n");
}

TEST(cli, test_help_version_no_extra_fields) {
  // The built-in options are known options, so rfl::NoExtraFields accepts
  // them.
  EXPECT_EQ(
      (kind_of(read_app<Copy, CopyApp, rfl::NoExtraFields>({"--help"}))),
      Kind::help);
  const auto result = read_app<Copy, CopyApp, rfl::NoExtraFields>(
      {"in.txt", "-o", "out.txt", "--help=0"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(std::holds_alternative<Copy>(*result));
}

TEST(cli, test_help_version_spellings) {
  // `-h` and `--help` together are the help.
  EXPECT_EQ(kind_of(read_app<Copy, CopyApp>({"-h", "--help"})), Kind::help);
  // A negative number is still a positional argument.
  const auto negative = read_app<Copy, CopyApp>({"-5", "-o", "out.txt"});
  ASSERT_TRUE(negative) << negative.error().what();
  EXPECT_EQ(std::get<Copy>(*negative).source.value().value(), "-5");
  // const argv, as for rfl::cli::read.
  const char* const args[] = {"app", "-vh"};
  EXPECT_EQ(kind_of(rfl::cli::read_app<Copy, CopyApp>(2, args)), Kind::help);
}

TEST(cli, test_help_version_wide) {
  const wchar_t* const args[] = {L"app", L"--version"};
  const auto result = rfl::cli::read_app<Copy, CopyApp>(2, args);
  EXPECT_EQ(text_of(result), "copy 1.2.0\n");
}

TEST(cli, test_help_version_edge_cases) {
  // Nested Processors<CopyApp>.
  EXPECT_EQ(
      (text_of(read_app<Copy, rfl::Processors<CopyApp>>({"--version"}))),
      "copy 1.2.0\n");

  // Description-only header (empty name and version).
  using AboutOnlyApp = rfl::cli::App<"", "", "Standalone description.">;
  const auto about_usage =
      rfl::cli::usage<Copy, AboutOnlyApp>("./copy", true).value();
  EXPECT_TRUE(about_usage.starts_with("Standalone description.\n\nUsage: ./copy"))
      << about_usage;

  // Empty App::name and empty program_name must not emit a leading space.
  using VerOnlyApp = rfl::cli::App<"", "2.0">;
  const char* const empty_prog[] = {"", "--version"};
  EXPECT_EQ(text_of(rfl::cli::read_app<Copy, VerOnlyApp>(2, empty_prog)),
            "2.0\n");
  const auto ver_header = rfl::cli::usage<Copy, VerOnlyApp>("", true).value();
  EXPECT_TRUE(ver_header.starts_with("2.0\n\nUsage: ")) << ver_header;

  // Both --help and --version: help wins in either order.
  EXPECT_EQ(kind_of(read_app<Copy, CopyApp>({"--help", "--version"})),
            Kind::help);
  EXPECT_EQ(kind_of(read_app<Copy, CopyApp>({"--version", "--help"})),
            Kind::help);

  // Explicit false flags do not trigger help/version.
  EXPECT_EQ(kind_of(read_app<Copy, CopyApp>({"--help=false", "--version"})),
            Kind::version);
  EXPECT_EQ(text_of(read_app<Copy, CopyApp>(
                {"in.txt", "-o", "out.txt", "-h=false", "--version=false"})),
            "(struct)");
}

}  // namespace test_help_version
