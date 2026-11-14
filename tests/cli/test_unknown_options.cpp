#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_unknown_options {

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "program");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

struct Database {
  std::string host;
  int port;
  std::optional<bool> ssl;
};

struct Query {
  std::string sql;
  std::optional<bool> verbose;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_long_unknown_option_no_extra_fields) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"--sql=x", "--nope", "1", "in.txt"},
           {"--sql=x", "--nope=1", "in.txt"},
           {"--sql=x", "in.txt", "--nope"},
           {"--nope", "--sql=x", "in.txt"}}) {
    const auto result = read<Query, rfl::NoExtraFields>(args);
    ASSERT_FALSE(result);
    EXPECT_EQ(std::string(result.error().what()), "Unknown option '--nope'.");
  }
}

struct WithDatabase {
  std::string sql;
  Database database;
};

TEST(cli, test_long_unknown_nested_option_no_extra_fields) {
  const auto result = read<WithDatabase, rfl::NoExtraFields>(
      {"--sql=x", "--database.host=h", "--database.port=1", "--database.nope",
       "1"});
  ASSERT_FALSE(result);
  EXPECT_EQ(std::string(result.error().what()),
            "Unknown option '--database.nope'.");
}

struct Flags {
  bool verbose;
  std::optional<bool> dry_run;
  rfl::Short<"q", std::optional<bool>> quiet;
  rfl::Description<"Colorize output", std::optional<bool>> color;
  Database database;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_long_known_options_no_extra_fields) {
  const auto result = read<Flags, rfl::NoExtraFields>(
      {"--verbose", "in.txt", "--database.host=h", "--database.port=1", "-q"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().verbose);
  EXPECT_EQ(result.value().quiet(), true);
  EXPECT_EQ(result.value().database.host, "h");
  EXPECT_EQ(result.value().input_file(), "in.txt");
}

struct Server {
  std::string host;
  int port;
};

struct WithMaps {
  std::map<std::string, std::string> labels;
  std::map<std::string, Server> servers;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_unknown_option_map_keys) {
  // Any key of a map is an option, but the fields of a struct in a map are
  // known.
  const auto result = read<WithMaps, rfl::NoExtraFields>(
      {"--labels.env=prod", "--servers.eu.host=h", "--servers.eu.port=1",
       "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().labels.at("env"), "prod");
  EXPECT_EQ(result.value().servers.at("eu").port, 1);

  const auto unknown = read<WithMaps, rfl::NoExtraFields>(
      {"--servers.eu.nope=1", "in.txt"});
  ASSERT_FALSE(unknown);
  EXPECT_EQ(std::string(unknown.error().what()),
            "Unknown option '--servers.eu.nope'.");
}

}  // namespace test_unknown_options
