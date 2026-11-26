#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace test_read_app_ranges {

using SubApp = rfl::cli::App<"osmium cat", "1.19.1", "Concatenate OSM files.">;
using UnnamedApp = rfl::cli::App<"", "2.0.0", "Unnamed subcommand.">;

struct CatConfig {
  rfl::Short<"o", std::optional<std::string>> output;
  rfl::Positional<std::string> input;
};

TEST(cli, test_read_app_vector_struct) {
  const std::vector<std::string> args = {"-o", "out.osm", "in.osm"};
  const auto res = rfl::cli::read_app<CatConfig, SubApp>(args);
  ASSERT_TRUE(res) << res.error().what();
  ASSERT_TRUE(std::holds_alternative<CatConfig>(*res));
  const auto& cfg = std::get<CatConfig>(*res);
  EXPECT_EQ(cfg.output.value(), "out.osm");
  EXPECT_EQ(cfg.input.value(), "in.osm");
}

TEST(cli, test_read_app_vector_help_and_version) {
  const std::vector<std::string> help_args = {"-h"};
  const auto help = rfl::cli::read_app<CatConfig, SubApp>(help_args);
  ASSERT_TRUE(help) << help.error().what();
  ASSERT_TRUE(std::holds_alternative<rfl::cli::HelpOrVersion>(*help));
  const auto& info = std::get<rfl::cli::HelpOrVersion>(*help);
  EXPECT_EQ(info.kind, rfl::cli::HelpOrVersion::Kind::help);
  EXPECT_NE(info.text.find("Usage: osmium cat "), std::string::npos);

  const std::vector<std::string> ver_args = {"--version"};
  const auto ver = rfl::cli::read_app<CatConfig, SubApp>(ver_args);
  ASSERT_TRUE(ver) << ver.error().what();
  ASSERT_TRUE(std::holds_alternative<rfl::cli::HelpOrVersion>(*ver));
  EXPECT_EQ(std::get<rfl::cli::HelpOrVersion>(*ver).text,
            "osmium cat 1.19.1\n");
}

TEST(cli, test_read_app_explicit_program_name_and_initializer_list) {
  const auto help =
      rfl::cli::read_app<CatConfig, UnnamedApp>("osmium diff", {"--help"});
  ASSERT_TRUE(help) << help.error().what();
  ASSERT_TRUE(std::holds_alternative<rfl::cli::HelpOrVersion>(*help));
  const auto& info = std::get<rfl::cli::HelpOrVersion>(*help);
  EXPECT_EQ(info.kind, rfl::cli::HelpOrVersion::Kind::help);
  EXPECT_NE(info.text.find("Usage: osmium diff "), std::string::npos);

  const auto ver =
      rfl::cli::read_app<CatConfig, UnnamedApp>("osmium diff", {"--version"});
  ASSERT_TRUE(ver) << ver.error().what();
  EXPECT_EQ(std::get<rfl::cli::HelpOrVersion>(*ver).text,
            "osmium diff 2.0.0\n");
}

TEST(cli, test_read_app_wide_vector) {
  const std::vector<std::wstring> args = {L"-o", L"out.osm", L"in.osm"};
  const auto res = rfl::cli::read_app<CatConfig, SubApp>(args);
  ASSERT_TRUE(res) << res.error().what();
  ASSERT_TRUE(std::holds_alternative<CatConfig>(*res));
  EXPECT_EQ(std::get<CatConfig>(*res).input.value(), "in.osm");
}

TEST(cli, test_read_app_span) {
  const std::vector<std::string_view> args = {"in.osm"};
  const auto res = rfl::cli::read_app<CatConfig, SubApp>(
      "osmium cat", std::span<const std::string_view>(args));
  ASSERT_TRUE(res) << res.error().what();
  ASSERT_TRUE(std::holds_alternative<CatConfig>(*res));
  EXPECT_EQ(std::get<CatConfig>(*res).input.value(), "in.osm");
}

}  // namespace test_read_app_ranges
