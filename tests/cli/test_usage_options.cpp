#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

// rfl::cli::usage for the parser's option features: wrappers in any order
// and positional order. Each test also reads the arguments as the help
// spells them.
namespace test_usage_options {

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "app");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

struct WrapperOrder {
  rfl::Description<"Where to write", rfl::Short<"o", std::string>> output;
  rfl::DefaultVal<rfl::Positional<std::string>> input = "in.txt";
  rfl::Description<"Be verbose", rfl::Short<"v", std::optional<bool>>>
      verbose;
};

TEST(cli, test_usage_wrapper_order) {
  EXPECT_EQ(rfl::cli::usage<WrapperOrder>("app", true).value(),
            "Usage: app -o|--output=<value> [<input>] [-v|--verbose[=<bool>]]\n"
            "\n"
            "Arguments:\n"
            "  -o, --output=<value>    Where to write. Required.\n"
            "  <input>                 Default: in.txt.\n"
            "  -v, --verbose[=<bool>]  Be verbose\n");

  const auto defaulted = read<WrapperOrder>({"-o", "out"});
  ASSERT_TRUE(defaulted) << defaulted.error().what();
  EXPECT_EQ(defaulted.value().input.value().value(), "in.txt");
}

/// The parser fills the rfl::Positional fields in declaration order, wherever
/// rfl::Positional sits among their wrappers.
struct PositionalOrder {
  rfl::Description<"Files to process", rfl::Positional<std::string>> files;
  rfl::Positional<std::string> pair;
  rfl::Positional<std::string> command;
};

TEST(cli, test_usage_positional_order) {
  EXPECT_EQ(rfl::cli::usage<PositionalOrder>("app", true).value(),
            "Usage: app <files> <pair> <command>\n"
            "\n"
            "Arguments:\n"
            "  <files>    Files to process. Required.\n"
            "  <pair>     Required.\n"
            "  <command>  Required.\n");

  const auto result = read<PositionalOrder>({"x", "a", "run"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().pair.value(), "a");
  EXPECT_EQ(result.value().command.value(), "run");
  EXPECT_EQ(result.value().files.value().value(), "x");
}

/// Two fields with the same short name fail every read, and usage too.
struct ShortNameTaken {
  rfl::Short<"o", std::string> output;
  rfl::Description<"Where to log", rfl::Short<"o", std::string>> log;
};

TEST(cli, test_usage_short_name_taken) {
  const auto usage = rfl::cli::usage<ShortNameTaken>("app", true);
  ASSERT_FALSE(usage);
  EXPECT_EQ(std::string(usage.error().what()),
            "Duplicate short name '-o' in struct definition.");
}

}  // namespace test_usage_options
