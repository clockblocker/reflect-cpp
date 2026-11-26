#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_multi_token {

struct PhaserLike {
  rfl::MultiToken<rfl::ValueName<"<CHROMOSOMES>", std::optional<std::vector<std::string>>>>
      population;
  rfl::Short<"p", rfl::MultiToken<std::optional<std::vector<std::string>>>>
      files;
  std::optional<int> rounds;
  rfl::Positional<std::optional<std::vector<std::string>>> input_files;
};

using Cli = rfl::Processors<rfl::NoExtraFields, rfl::cli::SeparateOptionValues,
                            rfl::cli::ListDelimiter<"">, rfl::cli::PureFlags>;

TEST(cli, test_multi_token_long_and_stop_at_next_option_or_double_dash) {
  const std::vector<std::string> args = {
      "--population", "a", "afc", "xadd", "--rounds", "10", "--", "prog.yul"};
  const auto res = rfl::cli::read<PhaserLike, Cli>(args);
  ASSERT_TRUE(res) << res.error().what();
  ASSERT_TRUE(rfl::unwrap(res->population).has_value());
  EXPECT_EQ(*rfl::unwrap(res->population),
            (std::vector<std::string>{"a", "afc", "xadd"}));
  EXPECT_EQ(res->rounds, 10);
  ASSERT_TRUE(rfl::unwrap(res->input_files).has_value());
  EXPECT_EQ(*rfl::unwrap(res->input_files),
            (std::vector<std::string>{"prog.yul"}));
}

TEST(cli, test_multi_token_repeated_and_short) {
  const std::vector<std::string> args = {
      "--population", "a", "b", "--population=c", "-p", "f1", "f2"};
  const auto res = rfl::cli::read<PhaserLike, Cli>(args);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(*rfl::unwrap(res->population),
            (std::vector<std::string>{"a", "b", "c"}));
  EXPECT_EQ(*rfl::unwrap(res->files), (std::vector<std::string>{"f1", "f2"}));
}

TEST(cli, test_multi_token_usage) {
  const auto help = rfl::cli::usage<PhaserLike, Cli>("yul-phaser", true);
  ASSERT_TRUE(help) << help.error().what();
  EXPECT_NE(help->find("--population=<CHROMOSOMES>..."), std::string::npos)
      << *help;
  EXPECT_NE(help->find("-p, --files=<value>..."), std::string::npos) << *help;
}

}  // namespace test_multi_token
