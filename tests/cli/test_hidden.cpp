#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <optional>
#include <string>
#include <vector>

namespace test_hidden {

struct SortConfig {
  rfl::Short<"v", rfl::Description<"Verbose output", rfl::DefaultVal<bool>>>
      verbose = false;
  rfl::Hidden<rfl::Short<"s", std::optional<std::string>>> strategy;
  rfl::Positional<rfl::Hidden<std::optional<std::vector<std::string>>>>
      input_filenames;
};

struct HiddenFlattened {
  rfl::Short<"d", rfl::Hidden<rfl::DefaultVal<int>>> debug_level = 0;
};

struct WrapperOrderConfig {
  rfl::Description<"Visible", std::optional<std::string>> visible;
  rfl::Flatten<HiddenFlattened> extra;
};

TEST(cli, test_hidden_parsed_but_omitted_from_usage) {
  const std::vector<std::string> args = {"-v", "-s", "multipass", "a.osm",
                                         "b.osm"};
  const auto res =
      rfl::cli::read<SortConfig, rfl::NoExtraFields,
                     rfl::cli::SeparateOptionValues,
                     rfl::cli::ListDelimiter<"">>(args);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_TRUE(res->verbose.value().value().get());
  EXPECT_EQ(res->strategy.value().value(), "multipass");
  ASSERT_TRUE(res->input_filenames.value().value().has_value());
  EXPECT_EQ(*res->input_filenames.value().value(),
            (std::vector<std::string>{"a.osm", "b.osm"}));

  const auto short_usage = rfl::cli::usage<SortConfig>("osmium sort", false);
  ASSERT_TRUE(short_usage) << short_usage.error().what();
  EXPECT_EQ(*short_usage, "Usage: osmium sort [-v|--verbose[=<bool>]]\n");

  const auto detailed = rfl::cli::usage<SortConfig>("osmium sort", true);
  ASSERT_TRUE(detailed) << detailed.error().what();
  EXPECT_EQ(detailed->find("strategy"), std::string::npos);
  EXPECT_EQ(detailed->find("input-filenames"), std::string::npos);
  EXPECT_NE(detailed->find("--verbose"), std::string::npos);
}

TEST(cli, test_hidden_inside_short_and_flatten) {
  const std::vector<std::string> args = {"--visible=hi", "-d", "3"};
  const auto res = rfl::cli::read<WrapperOrderConfig, rfl::NoExtraFields>(args);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->visible.value(), "hi");
  EXPECT_EQ(res->extra().debug_level.value().value().get(), 3);

  const auto detailed = rfl::cli::usage<WrapperOrderConfig>("app", true);
  ASSERT_TRUE(detailed) << detailed.error().what();
  EXPECT_NE(detailed->find("--visible"), std::string::npos);
  EXPECT_EQ(detailed->find("debug-level"), std::string::npos);
}

}  // namespace test_hidden
