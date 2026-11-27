#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_cli_alias {

struct ServerConfig {
  rfl::Alias<"download_dir", rfl::Alias<"downloadDir", std::string>>
      download_dir;
  rfl::Short<"v", rfl::Alias<"debug", rfl::DefaultVal<bool>>> verbose = false;
  rfl::Alias<"listen-port", rfl::DefaultVal<int>> port = 8080;
  rfl::Positional<rfl::Alias<"src", std::optional<std::string>>> input;
};

TEST(cli, test_alias_basic_and_no_extra_fields) {
  const auto res = rfl::cli::read<ServerConfig, rfl::NoExtraFields>(
      {"--download_dir=/var/dl", "--debug", "--listen-port=9090",
       "--src=file.txt"});
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->download_dir, "/var/dl");
  EXPECT_TRUE(res->verbose.value().value().value());
  EXPECT_EQ(res->port.value().value(), 9090);
  ASSERT_TRUE(res->input.value().value().has_value());
  EXPECT_EQ(*res->input.value().value(), "file.txt");

  // Second chained alias also works.
  const auto res2 = rfl::cli::read<ServerConfig, rfl::NoExtraFields>(
      {"--downloadDir=/camel"});
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_EQ(res2->download_dir, "/camel");
  EXPECT_FALSE(res2->verbose.value().value().value());
  EXPECT_EQ(res2->port.value().value(), 8080);

  // Canonical kebab-case name still works.
  const auto res3 = rfl::cli::read<ServerConfig, rfl::NoExtraFields>(
      {"--download-dir=/kebab", "-v", "pos.txt"});
  ASSERT_TRUE(res3) << res3.error().what();
  EXPECT_EQ(res3->download_dir, "/kebab");
  EXPECT_TRUE(res3->verbose.value().value().value());
  EXPECT_EQ(*res3->input.value().value(), "pos.txt");
}

TEST(cli, test_alias_conflicts_rejected) {
  // Primary + alias conflict.
  const auto c1 = rfl::cli::read<ServerConfig>(
      {"--download-dir=/a", "--download_dir=/b"});
  EXPECT_FALSE(c1);

  // Two aliases of the same field conflict.
  const auto c2 = rfl::cli::read<ServerConfig>(
      {"--download_dir=/a", "--downloadDir=/b"});
  EXPECT_FALSE(c2);

  // Short flag + alias conflict.
  const auto c3 = rfl::cli::read<ServerConfig>(
      {"--download-dir=/a", "-v", "--debug"});
  EXPECT_FALSE(c3);

  // Positional + alias conflict.
  const auto c4 = rfl::cli::read<ServerConfig>(
      {"--download-dir=/a", "pos.txt", "--src=other.txt"});
  EXPECT_FALSE(c4);
}

struct AliasWrappingShortAndPositional {
  rfl::Alias<"old-flag", rfl::Short<"f", rfl::DefaultVal<bool>>> flag = false;
  rfl::Alias<"old-file", rfl::Positional<std::string>> file;
};

TEST(cli, test_alias_outer_around_short_and_positional) {
  // -f must still be recognized as a bool flag through outer rfl::Alias,
  // leaving "data.txt" for the positional field.
  const auto res =
      rfl::cli::read<AliasWrappingShortAndPositional, rfl::NoExtraFields>(
          {"-f", "data.txt"});
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_TRUE(res->flag.value().value().value());
  EXPECT_EQ(res->file.value().value(), "data.txt");

  // Passing both via their aliases also works.
  const auto via_alias =
      rfl::cli::read<AliasWrappingShortAndPositional, rfl::NoExtraFields>(
          {"--old-flag", "--old-file=other.txt"});
  ASSERT_TRUE(via_alias) << via_alias.error().what();
  EXPECT_TRUE(via_alias->flag.value().value().value());
  EXPECT_EQ(via_alias->file.value().value(), "other.txt");
}

struct NestedDb {
  rfl::Alias<"hostname", std::string> host;
};

struct AppConfig {
  rfl::Alias<"database", NestedDb> db;
};

TEST(cli, test_alias_nested_struct) {
  const auto res = rfl::cli::read<AppConfig, rfl::NoExtraFields>(
      {"--database.hostname=db.local"});
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->db.value().host, "db.local");
}

}  // namespace test_cli_alias
