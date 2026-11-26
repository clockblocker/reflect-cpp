#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_pure_flags {

struct GetIdConfig {
  rfl::Short<"H", rfl::DefaultVal<bool>> with_history = false;
  rfl::Short<"r", rfl::DefaultVal<bool>> add_referenced = false;
  rfl::Positional<std::string> input;
  rfl::Positional<rfl::DefaultVal<std::vector<std::string>>> ids;
};

TEST(cli, test_pure_flags_numeric_positionals) {
  const std::vector<std::string> args = {"in.osm", "-H", "1", "0", "42"};
  const auto res =
      rfl::cli::read<GetIdConfig, rfl::cli::PureFlags,
                     rfl::cli::ListDelimiter<"">>(args);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_TRUE(res->with_history.value().get());
  EXPECT_FALSE(res->add_referenced.value().get());
  EXPECT_EQ(res->input.value(), "in.osm");
  EXPECT_EQ(res->ids.value().get(),
            (std::vector<std::string>{"1", "0", "42"}));
}

TEST(cli, test_pure_flags_bool_word_positionals) {
  const std::vector<std::string> args = {"in.osm", "-r", "false", "true"};
  const auto res =
      rfl::cli::read<GetIdConfig, rfl::cli::PureFlags,
                     rfl::cli::ListDelimiter<"">>(args);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_TRUE(res->add_referenced.value().get());
  EXPECT_EQ(res->ids.value().get(),
            (std::vector<std::string>{"false", "true"}));
}

TEST(cli, test_pure_flags_grouped_and_explicit_equals) {
  const std::vector<std::string> args = {"-Hr", "in.osm", "0"};
  const auto res = rfl::cli::read<GetIdConfig, rfl::cli::PureFlags>(args);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_TRUE(res->with_history.value().get());
  EXPECT_TRUE(res->add_referenced.value().get());
  EXPECT_EQ(res->input.value(), "in.osm");
  EXPECT_EQ(res->ids.value().get(), (std::vector<std::string>{"0"}));

  const std::vector<std::string> explicit_false = {"-H=false", "in.osm", "1"};
  const auto res2 =
      rfl::cli::read<GetIdConfig, rfl::cli::PureFlags>(explicit_false);
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_FALSE(res2->with_history.value().get());
  EXPECT_EQ(res2->ids.value().get(), (std::vector<std::string>{"1"}));
}

}  // namespace test_pure_flags
