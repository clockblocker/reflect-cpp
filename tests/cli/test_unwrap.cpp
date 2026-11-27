#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <optional>
#include <string>

namespace test_unwrap {

struct Config {
  rfl::Group<"COMMON OPTIONS",
             rfl::Short<"v", rfl::Description<"Verbose", rfl::DefaultVal<bool>>>>
      verbose = false;
  rfl::Group<"OUTPUT OPTIONS",
             rfl::Short<"o", rfl::Description<"Output",
                                              std::optional<std::string>>>>
      output;
  rfl::Hidden<rfl::Positional<rfl::DefaultVal<std::string>>> input = "in.osm";
};

TEST(cli, test_unwrap_nested_wrappers) {
  const std::vector<std::string> args = {"-v", "-o", "out.osm"};
  const auto res =
      rfl::cli::read<Config, rfl::cli::SeparateOptionValues>(args);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_TRUE(rfl::unwrap(res->verbose));
  ASSERT_TRUE(rfl::unwrap(res->output).has_value());
  EXPECT_EQ(*rfl::unwrap(res->output), "out.osm");
  EXPECT_EQ(rfl::unwrap(res->input), "in.osm");

  // Mutable reference works too.
  auto cfg = *res;
  rfl::unwrap(cfg.verbose) = false;
  EXPECT_FALSE(rfl::unwrap(cfg.verbose));
}

}  // namespace test_unwrap
