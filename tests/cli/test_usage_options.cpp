#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

// rfl::cli::usage for the parser's option features: rfl::ImplicitValue,
// wrappers in any order, repeated list options, a list as the last
// positional field, positional order, rfl::cli::ListDelimiter and
// rfl::cli::OptionsFirst. Each test also reads the arguments as the help
// spells them.
namespace test_usage_options {

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "app");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

using Strings = std::vector<std::string>;

enum class Color { never, always, automatic };

struct WithImplicitValues {
  rfl::Description<"When to color the output",
                   rfl::ImplicitValue<std::optional<Color>, "always">>
      color;
  rfl::Short<"j", rfl::ImplicitValue<rfl::DefaultVal<int>, "4">> jobs = 1;
  rfl::ImplicitValue<std::optional<std::string>> tag;
  rfl::Short<"q", bool> quiet;
};

TEST(cli, test_usage_implicit_value) {
  EXPECT_EQ(rfl::cli::usage<WithImplicitValues>("app", true),
            "Usage: app [--color[=<never|always|automatic>]] "
            "[-j|--jobs[=<value>]] [--tag[=<value>]] -q|--quiet[=<bool>]\n"
            "\n"
            "Arguments:\n"
            "  --color[=<never|always|automatic>]  When to color the output. "
            "Without a value: always.\n"
            "  -j, --jobs[=<value>]                Without a value: 4. "
            "Default: 1.\n"
            "  --tag[=<value>]\n"
            "  -q, --quiet[=<bool>]                Required.\n");

  // Bare, the options stand for their implicit values.
  const auto bare = read<WithImplicitValues>({"--color", "-j", "--tag", "-q"});
  ASSERT_TRUE(bare) << bare.error().what();
  EXPECT_EQ(bare.value().color.value().value(), Color::always);
  EXPECT_EQ(bare.value().jobs.value().value().get(), 4);
  EXPECT_EQ(bare.value().tag.value(), std::string());
  EXPECT_TRUE(bare.value().quiet.value());

  // With a value after '=', and left out.
  const auto given = read<WithImplicitValues>({"--color=never", "-q"});
  ASSERT_TRUE(given) << given.error().what();
  EXPECT_EQ(given.value().color.value().value(), Color::never);
  EXPECT_EQ(given.value().jobs.value().value().get(), 1);
  EXPECT_FALSE(given.value().tag.value());
}

struct WrapperOrder {
  rfl::Description<"Where to write", rfl::Short<"o", std::string>> output;
  rfl::DefaultVal<rfl::Positional<std::string>> input = "in.txt";
  rfl::Description<"Be verbose", rfl::Short<"v", std::optional<bool>>>
      verbose;
};

TEST(cli, test_usage_wrapper_order) {
  EXPECT_EQ(rfl::cli::usage<WrapperOrder>("app", true),
            "Usage: app -o|--output=<value> [<input>] [-v|--verbose[=<bool>]]\n"
            "\n"
            "Arguments:\n"
            "  -o, --output=<value>    Where to write. Required.\n"
            "  <input>                 Default: in.txt.\n"
            "  -v, --verbose[=<bool>]  Be verbose\n");

  const auto result = read<WrapperOrder>({"-v", "-o", "out", "data.csv"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().output.value().value(), "out");
  EXPECT_EQ(result.value().input.value().value(), "data.csv");
  EXPECT_EQ(result.value().verbose.value().value(), true);

  const auto defaulted = read<WrapperOrder>({"-o", "out"});
  ASSERT_TRUE(defaulted) << defaulted.error().what();
  EXPECT_EQ(defaulted.value().input.value().value(), "in.txt");
  EXPECT_FALSE(read<WrapperOrder>({"data.csv"}));
}

struct Lists {
  rfl::Short<"r", rfl::Description<"Reports to write", Strings>> report;
  rfl::DefaultVal<Strings> tags = Strings{"a", "b"};
  rfl::Positional<std::string> command;
  rfl::Description<"Files to process", rfl::Positional<Strings>> files;
};

TEST(cli, test_usage_lists) {
  EXPECT_EQ(rfl::cli::usage<Lists>("app", true),
            "Usage: app -r|--report=<v1,v2,...> [--tags=<v1,v2,...>] "
            "<command> <files>...\n"
            "\n"
            "Arguments:\n"
            "  -r, --report=<v1,v2,...>  Reports to write. Can be repeated. "
            "Required.\n"
            "  --tags=<v1,v2,...>        Default: a,b. Can be repeated.\n"
            "  <command>                 Required.\n"
            "  <files>...                Files to process. Required.\n");

  const auto result = read<Lists>(
      {"-r", "a", "run", "--report=b,c", "x,y", "z", "--tags", "t"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().report.value().value(), (Strings{"a", "b", "c"}));
  EXPECT_EQ(result.value().tags.value(), (Strings{"t"}));
  EXPECT_EQ(result.value().command.value(), "run");
  EXPECT_EQ(result.value().files.value().value(), (Strings{"x", "y", "z"}));
}

/// The parser fills the outermost rfl::Positional fields first, then those
/// found through other wrappers, and a list takes the remaining arguments
/// only as the last of them.
struct PositionalOrder {
  rfl::Description<"Files to process", rfl::Positional<Strings>> files;
  rfl::Positional<Strings> pair;
  rfl::Positional<std::string> command;
};

TEST(cli, test_usage_positional_order) {
  EXPECT_EQ(rfl::cli::usage<PositionalOrder>("app", true),
            "Usage: app <pair> <command> <files>...\n"
            "\n"
            "Arguments:\n"
            "  <pair>      Required.\n"
            "  <command>   Required.\n"
            "  <files>...  Files to process. Required.\n");

  const auto result = read<PositionalOrder>({"a,b", "run", "x", "y"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().pair.value(), (Strings{"a", "b"}));
  EXPECT_EQ(result.value().command.value(), "run");
  EXPECT_EQ(result.value().files.value().value(), (Strings{"x", "y"}));
}

/// An rfl::Short found through other wrappers counts only if no other field
/// has its name.
struct ShortNameTaken {
  rfl::Short<"o", std::string> output;
  rfl::Description<"Where to log", rfl::Short<"o", std::string>> log;
};

TEST(cli, test_usage_short_name_taken) {
  EXPECT_EQ(rfl::cli::usage<ShortNameTaken>("app", true),
            "Usage: app -o|--output=<value> --log=<value>\n"
            "\n"
            "Arguments:\n"
            "  -o, --output=<value>  Required.\n"
            "  --log=<value>         Where to log. Required.\n");

  const auto result = read<ShortNameTaken>({"-o", "out", "--log=l"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().output.value(), "out");
  EXPECT_EQ(result.value().log.value().value(), "l");
}

TEST(cli, test_usage_list_delimiter) {
  using Semicolon = rfl::cli::ListDelimiter<";">;
  EXPECT_EQ((rfl::cli::usage<Lists, Semicolon>("app", true)),
            "Usage: app -r|--report=<v1;v2;...> [--tags=<v1;v2;...>] "
            "<command> <files>...\n"
            "\n"
            "Arguments:\n"
            "  -r, --report=<v1;v2;...>  Reports to write. Can be repeated. "
            "Required.\n"
            "  --tags=<v1;v2;...>        Default: a;b. Can be repeated.\n"
            "  <command>                 Required.\n"
            "  <files>...                Files to process. Required.\n");
  const auto semicolon =
      read<Lists, Semicolon>({"-r", "a;b,c", "run", "x;y", "--tags=t;u"});
  ASSERT_TRUE(semicolon) << semicolon.error().what();
  EXPECT_EQ(semicolon.value().report.value().value(),
            (Strings{"a", "b,c"}));
  EXPECT_EQ(semicolon.value().tags.value(), (Strings{"t", "u"}));
  EXPECT_EQ(semicolon.value().files.value().value(), (Strings{"x", "y"}));

  // Not split: every occurrence is one item, so a list takes one value per
  // occurrence.
  using NoSplit = rfl::cli::ListDelimiter<"">;
  EXPECT_EQ((rfl::cli::usage<Lists, NoSplit>("app", true)),
            "Usage: app -r|--report=<value> [--tags=<value>] "
            "<command> <files>...\n"
            "\n"
            "Arguments:\n"
            "  -r, --report=<value>  Reports to write. Can be repeated. "
            "Required.\n"
            "  --tags=<value>        Default: a, b. Can be repeated.\n"
            "  <command>             Required.\n"
            "  <files>...            Files to process. Required.\n");
  const auto no_split =
      read<Lists, NoSplit>({"-r", "a,b", "-r", "c", "run", "x,y", "z"});
  ASSERT_TRUE(no_split) << no_split.error().what();
  EXPECT_EQ(no_split.value().report.value().value(), (Strings{"a,b", "c"}));
  EXPECT_EQ(no_split.value().files.value().value(), (Strings{"x,y", "z"}));
}

// recipe [options] <name> <args...>, as in test_options_first.cpp.
struct Recipe {
  rfl::Short<"v", std::optional<bool>> verbose;
  rfl::Positional<std::string> name;
  rfl::Positional<rfl::DefaultVal<Strings>> args;
  rfl::Short<"j", std::optional<int>> jobs;
};

TEST(cli, test_usage_options_first) {
  // Without OptionsFirst, options and positional arguments can be mixed, so
  // the synopsis keeps the declaration order.
  EXPECT_EQ(rfl::cli::usage<Recipe>("recipe"),
            "Usage: recipe [-v|--verbose[=<bool>]] <name> [<args>...] "
            "[-j|--jobs=<value>]\n");

  // With OptionsFirst, the options end at the first positional argument, so
  // the synopsis lists them first. The rows keep the declaration order.
  EXPECT_EQ(
      (rfl::cli::usage<Recipe, rfl::cli::OptionsFirst,
                       rfl::cli::ListDelimiter<"">>("recipe", true)),
      "Usage: recipe [-v|--verbose[=<bool>]] [-j|--jobs=<value>] <name> "
      "[<args>...]\n"
      "\n"
      "Arguments:\n"
      "  -v, --verbose[=<bool>]\n"
      "  <name>                  Required.\n"
      "  <args>...\n"
      "  -j, --jobs=<value>\n");

  const auto result =
      read<Recipe, rfl::cli::OptionsFirst, rfl::cli::ListDelimiter<"">>(
          {"-v", "-j", "4", "build", "--jobs", "8"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().jobs.value(), 4);
  EXPECT_EQ(result.value().name.value(), "build");
  EXPECT_EQ(result.value().args.value().value(), (Strings{"--jobs", "8"}));
}

}  // namespace test_usage_options
