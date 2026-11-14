#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <variant>
#include <vector>

// `--key value` with rfl::cli::SeparateOptionValues.
namespace test_long_option_values {

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "program");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

template <class T, class... Ps>
rfl::Result<T> read_separate(std::vector<const char*> _args) {
  return read<T, rfl::cli::SeparateOptionValues, Ps...>(std::move(_args));
}

struct Database {
  std::string host;
  int port;
  std::optional<bool> ssl;
};

struct Config {
  std::string sql;
  int offset;
  std::vector<std::string> tags;
  rfl::Description<"A label", std::optional<std::string>> label;
  Database database;
};

TEST(cli, test_long_space_value) {
  const auto result = read_separate<Config>(
      {"--sql", "SELECT 1", "--offset", "-5", "--tags", "a,b", "--label", "x",
       "--database.host", "localhost", "--database.port", "5432",
       "--database.ssl"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& c = result.value();
  EXPECT_EQ(c.sql, "SELECT 1");
  EXPECT_EQ(c.offset, -5);
  EXPECT_EQ(c.tags, (std::vector<std::string>{"a", "b"}));
  EXPECT_EQ(c.label(), "x");
  EXPECT_EQ(c.database.host, "localhost");
  EXPECT_EQ(c.database.port, 5432);
  EXPECT_EQ(c.database.ssl, true);
}

struct Query {
  std::optional<std::string> sql;
  std::optional<bool> verbose;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_long_space_value_is_opt_in) {
  // Without the processor, a long option takes its value only after '='.
  const auto next = read<Query>({"--sql", "in.txt"});
  ASSERT_TRUE(next) << next.error().what();
  EXPECT_EQ(next.value().sql, "");
  EXPECT_EQ(next.value().input_file(), "in.txt");

  const auto before_option = read<Query>({"--sql", "--verbose", "in.txt"});
  ASSERT_TRUE(before_option) << before_option.error().what();
  EXPECT_EQ(before_option.value().sql, "");
  EXPECT_EQ(before_option.value().verbose, true);

  const auto last = read<Query>({"in.txt", "--sql"});
  ASSERT_TRUE(last) << last.error().what();
  EXPECT_EQ(last.value().sql, "");
}

TEST(cli, test_long_missing_value) {
  // At the end, before another option and before `--`. `--sql=` passes an
  // empty value.
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"in.txt", "--sql"},
           {"--sql", "--verbose", "in.txt"},
           {"--sql", "--", "in.txt"}}) {
    const auto result = read_separate<Query>(args);
    ASSERT_FALSE(result);
    EXPECT_EQ(std::string(result.error().what()),
              "Missing value for option '--sql'.");
  }
  const auto empty = read_separate<Query>({"--sql=", "in.txt"});
  ASSERT_TRUE(empty) << empty.error().what();
  EXPECT_EQ(empty.value().sql, "");
}

struct Force {
  bool force;
  rfl::Short<"q", std::optional<bool>> quiet;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_long_bool_does_not_take_next_token) {
  // Unlike `-q 0`, a long flag never takes the next token, even if it is a
  // boolean literal.
  for (const char* next : {"in.txt", "0"}) {
    const auto result = read_separate<Force>({"--force", next, "--quiet"});
    ASSERT_TRUE(result) << next << ": " << result.error().what();
    EXPECT_TRUE(result.value().force) << next;
    EXPECT_EQ(result.value().quiet(), true) << next;
    EXPECT_EQ(result.value().input_file(), next);
  }
}

TEST(cli, test_long_unknown_option_does_not_take_next_token) {
  // Without rfl::NoExtraFields, unknown options are ignored.
  const auto result =
      read_separate<Query>({"--nope", "in.txt", "--sql", "x"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().sql, "x");
  EXPECT_EQ(result.value().input_file(), "in.txt");
}

struct WithMaps {
  std::map<std::string, std::string> labels;
  std::map<std::string, bool> features;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_long_space_map) {
  const auto result = read_separate<WithMaps, rfl::NoExtraFields>(
      {"--labels.env", "prod", "--labels.team=data", "--features.fast",
       "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& m = result.value();
  EXPECT_EQ(m.labels, (std::map<std::string, std::string>{
                          {"env", "prod"}, {"team", "data"}}));
  EXPECT_EQ(m.features, (std::map<std::string, bool>{{"fast", true}}));
  EXPECT_EQ(m.input_file(), "in.txt");
}

struct WithVariants {
  std::optional<std::variant<bool, std::string>> mode;
  rfl::Positional<std::optional<std::string>> input_file;
};

TEST(cli, test_long_variant_without_value) {
  // A variant may hold a bool, so the next token is never its value.
  const auto result = read_separate<WithVariants>({"--mode", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().mode, (std::variant<bool, std::string>(true)));
  EXPECT_EQ(result.value().input_file(), "in.txt");
}

}  // namespace test_long_option_values
