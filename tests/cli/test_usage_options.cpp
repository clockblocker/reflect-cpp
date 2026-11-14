#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

// rfl::cli::usage for the parser's option features: wrappers in any order,
// repeated list options and positional order. Each test also reads the
// arguments as the help spells them.
namespace test_usage_options {

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "app");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

using Strings = std::vector<std::string>;

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
};

TEST(cli, test_usage_lists) {
  EXPECT_EQ(rfl::cli::usage<Lists>("app", true),
            "Usage: app -r|--report=<v1,v2,...> [--tags=<v1,v2,...>] "
            "<command>\n"
            "\n"
            "Arguments:\n"
            "  -r, --report=<v1,v2,...>  Reports to write. Can be repeated. "
            "Required.\n"
            "  --tags=<v1,v2,...>        Default: a,b. Can be repeated.\n"
            "  <command>                 Required.\n");

  const auto result =
      read<Lists>({"-r", "a", "run", "--report=b,c", "--tags", "t"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().report.value().value(), (Strings{"a", "b", "c"}));
  EXPECT_EQ(result.value().tags.value(), (Strings{"t"}));
  EXPECT_EQ(result.value().command.value(), "run");
}

/// The parser fills the outermost rfl::Positional fields first, then those
/// found through other wrappers.
struct PositionalOrder {
  rfl::Description<"Files to process", rfl::Positional<std::string>> files;
  rfl::Positional<std::string> pair;
  rfl::Positional<std::string> command;
};

TEST(cli, test_usage_positional_order) {
  EXPECT_EQ(rfl::cli::usage<PositionalOrder>("app", true),
            "Usage: app <pair> <command> <files>\n"
            "\n"
            "Arguments:\n"
            "  <pair>     Required.\n"
            "  <command>  Required.\n"
            "  <files>    Files to process. Required.\n");

  const auto result = read<PositionalOrder>({"a", "run", "x"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().pair.value(), "a");
  EXPECT_EQ(result.value().command.value(), "run");
  EXPECT_EQ(result.value().files.value().value(), "x");
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

}  // namespace test_usage_options
