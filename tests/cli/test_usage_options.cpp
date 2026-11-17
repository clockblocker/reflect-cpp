#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

// rfl::cli::usage for the parser's option features: wrappers in any order,
// repeated list options, a list as the last positional field and positional
// order. Each test also reads the arguments as the help spells them.
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

struct Lists {
  rfl::Short<"r", rfl::Description<"Reports to write", Strings>> report;
  rfl::DefaultVal<Strings> tags = Strings{"a", "b"};
  rfl::Positional<std::string> command;
  rfl::Description<"Files to process", rfl::Positional<Strings>> files;
};

TEST(cli, test_usage_lists) {
  EXPECT_EQ(rfl::cli::usage<Lists>("app", true).value(),
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
      {"-r", "a", "run", "--report=b,c", "x,y", "z", "--tags=t"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().report.value().value(), (Strings{"a", "b", "c"}));
  EXPECT_EQ(result.value().tags.value(), (Strings{"t"}));
  EXPECT_EQ(result.value().command.value(), "run");
  EXPECT_EQ(result.value().files.value().value(), (Strings{"x", "y", "z"}));
}

/// The parser fills the rfl::Positional fields in declaration order, wherever
/// rfl::Positional sits among their wrappers, and a list takes the remaining
/// arguments only as the last of them.
struct PositionalOrder {
  rfl::Positional<Strings> pair;
  rfl::Positional<std::string> command;
  rfl::Description<"Files to process", rfl::Positional<Strings>> files;
};

TEST(cli, test_usage_positional_order) {
  EXPECT_EQ(rfl::cli::usage<PositionalOrder>("app", true).value(),
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

TEST(cli, test_usage_list_delimiter) {
  using Semicolon = rfl::cli::ListDelimiter<";">;
  EXPECT_EQ((rfl::cli::usage<Lists, Semicolon>("app", true).value()),
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
  // occurrence, and a default of several items, which no single value can
  // spell, is left out.
  using NoSplit = rfl::cli::ListDelimiter<"">;
  EXPECT_EQ((rfl::cli::usage<Lists, NoSplit>("app", true).value()),
            "Usage: app -r|--report=<value> [--tags=<value>] "
            "<command> <files>...\n"
            "\n"
            "Arguments:\n"
            "  -r, --report=<value>  Reports to write. Can be repeated. "
            "Required.\n"
            "  --tags=<value>        Can be repeated.\n"
            "  <command>             Required.\n"
            "  <files>...            Files to process. Required.\n");
  const auto no_split =
      read<Lists, NoSplit>({"-r", "a,b", "-r", "c", "run", "x,y", "z"});
  ASSERT_TRUE(no_split) << no_split.error().what();
  EXPECT_EQ(no_split.value().report.value().value(), (Strings{"a,b", "c"}));
  EXPECT_EQ(no_split.value().files.value().value(), (Strings{"x,y", "z"}));
}

}  // namespace test_usage_options
