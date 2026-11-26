#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace test_usage_custom {

struct CatOptions {
  rfl::Group<"COMMAND OPTIONS",
             rfl::Description<"Buffer data in memory", rfl::DefaultVal<bool>>>
      buffer_data = false;
  rfl::Group<"COMMON OPTIONS",
             rfl::Short<"v", rfl::Description<"Set verbose mode",
                                              rfl::DefaultVal<bool>>>>
      verbose = false;
  rfl::Group<"INPUT OPTIONS",
             rfl::Short<"F", rfl::Description<"Format of input files",
                                              std::optional<std::string>>>>
      input_format;
  rfl::Hidden<rfl::Positional<std::optional<std::vector<std::string>>>>
      input_filenames;
};

using SubApp =
    rfl::cli::App<"", "", "", rfl::cli::BuiltinHelp::short_and_long>;

TEST(cli, test_usage_custom_synopsis_description_footer_and_help_group) {
  const rfl::cli::UsageOptions opts{
      .detailed = true,
      .synopsis = "osmium cat [OPTIONS] [OSM-FILE...]",
      .description = "Concatenate OSM files and convert to different formats.",
      .footer = "Use 'osmium help cat' to display the manual page.",
      .help_group = "COMMON OPTIONS",
  };
  const std::vector<std::string> args = {"-h"};
  const auto res =
      rfl::cli::read_app<CatOptions, SubApp>("osmium cat", args, opts);
  ASSERT_TRUE(res) << res.error().what();
  ASSERT_TRUE(std::holds_alternative<rfl::cli::HelpOrVersion>(*res));
  const auto& text = std::get<rfl::cli::HelpOrVersion>(*res).text;

  EXPECT_TRUE(text.starts_with(
      "Usage: osmium cat [OPTIONS] [OSM-FILE...]\n\n"
      "Concatenate OSM files and convert to different formats.\n"));
  const auto cmd_pos = text.find("\nCOMMAND OPTIONS:\n");
  const auto common_pos = text.find("\nCOMMON OPTIONS:\n");
  const auto help_pos = text.find("  -h, --help");
  const auto verbose_pos = text.find("  -v, --verbose");
  const auto input_pos = text.find("\nINPUT OPTIONS:\n");
  const auto footer_pos =
      text.find("\nUse 'osmium help cat' to display the manual page.\n");

  ASSERT_NE(cmd_pos, std::string::npos) << text;
  ASSERT_NE(common_pos, std::string::npos) << text;
  ASSERT_NE(help_pos, std::string::npos) << text;
  ASSERT_NE(verbose_pos, std::string::npos) << text;
  ASSERT_NE(input_pos, std::string::npos) << text;
  ASSERT_NE(footer_pos, std::string::npos) << text;

  // Built-in -h/--help is placed at the start of COMMON OPTIONS without
  // reordering COMMAND OPTIONS ahead of it.
  EXPECT_LT(cmd_pos, common_pos);
  EXPECT_LT(common_pos, help_pos);
  EXPECT_LT(help_pos, verbose_pos);
  EXPECT_LT(verbose_pos, input_pos);
  EXPECT_LT(input_pos, footer_pos);
  EXPECT_EQ(text.find("input-filenames"), std::string::npos);
}

}  // namespace test_usage_custom
