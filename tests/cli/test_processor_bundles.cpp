#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace test_processor_bundles {

using BaseCli = rfl::Processors<rfl::NoExtraFields,
                                rfl::cli::SeparateOptionValues,
                                rfl::cli::ListDelimiter<"">>;
using FullCli =
    rfl::Processors<BaseCli, rfl::cli::PureFlags, rfl::Processors<>>;
using AppBundle =
    rfl::Processors<rfl::cli::App<"demo", "2.0", "Demo app">, FullCli>;

static_assert(
    std::is_same_v<
        rfl::internal::flatten_processors_t<rfl::SnakeCaseToKebabCase, FullCli>,
        rfl::Processors<rfl::SnakeCaseToKebabCase, rfl::NoExtraFields,
                        rfl::cli::SeparateOptionValues,
                        rfl::cli::ListDelimiter<"">, rfl::cli::PureFlags>>);

struct Config {
  rfl::Short<"v", rfl::DefaultVal<bool>> verbose = false;
  std::optional<std::string> output;
  std::optional<std::vector<std::string>> items;
  rfl::Positional<rfl::DefaultVal<std::vector<std::string>>> ids;
};

TEST(cli, test_processor_bundle_read) {
  const std::vector<std::string> args = {
      "--output", "out.txt", "--items", "a,b", "-v", "1", "2"};
  const auto res = rfl::cli::read<Config, FullCli>(args);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_TRUE(rfl::unwrap(res->verbose));
  EXPECT_EQ(res->output, "out.txt");
  EXPECT_EQ(*res->items, (std::vector<std::string>{"a,b"}));
  EXPECT_EQ(rfl::unwrap(res->ids), (std::vector<std::string>{"1", "2"}));

  const std::vector<std::string> bad_args = {"--unknown"};
  const auto err = rfl::cli::read<Config, FullCli>(bad_args);
  EXPECT_FALSE(err);
}

TEST(cli, test_processor_bundle_read_app_and_usage) {
  const std::vector<std::string> help_args = {"--help"};
  const auto res = rfl::cli::read_app<Config, AppBundle>(help_args);
  ASSERT_TRUE(res) << res.error().what();
  const auto* info = std::get_if<rfl::cli::HelpOrVersion>(&*res);
  ASSERT_NE(info, nullptr);
  EXPECT_EQ(info->kind, rfl::cli::HelpOrVersion::Kind::help);
  EXPECT_NE(info->text.find("demo 2.0"), std::string::npos);
  // PureFlags inside the nested bundle takes effect in usage().
  EXPECT_EQ(info->text.find("[=<bool>]"), std::string::npos) << info->text;
  EXPECT_EQ(info->text.find("Default: false."), std::string::npos)
      << info->text;

  const auto direct = rfl::cli::usage<Config, AppBundle>("demo", true);
  ASSERT_TRUE(direct) << direct.error().what();
  EXPECT_EQ(*direct, info->text);
}

TEST(cli, test_processor_bundle_default_if_missing) {
  struct Simple {
    int count = 7;
    bool flag = false;
  };
  using Bundle = rfl::Processors<rfl::DefaultIfMissing, rfl::cli::PureFlags>;
  const auto text = rfl::cli::usage<Simple, Bundle>("simple", true);
  ASSERT_TRUE(text) << text.error().what();
  EXPECT_NE(text->find("Default: 7."), std::string::npos) << *text;
  EXPECT_EQ(text->find("Default: false."), std::string::npos) << *text;
}

}  // namespace test_processor_bundles
