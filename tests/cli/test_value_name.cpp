#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_value_name {

struct SolcLike {
  rfl::Short<"o", rfl::ValueName<"path", rfl::Description<"Output directory",
                                                          std::optional<std::string>>>>
      output_dir;
  rfl::ValueName<"n", rfl::Description<"Optimizer runs", rfl::DefaultVal<int>>>
      optimize_runs = 200;
  rfl::ValueName<"<PATH>", std::optional<std::vector<std::string>>> include_path;
};

TEST(cli, test_value_name_parsing_and_usage) {
  const std::vector<std::string> args = {"-o", "/tmp/out", "--optimize-runs",
                                         "500", "--include-path", "/a"};
  const auto res =
      rfl::cli::read<SolcLike, rfl::cli::SeparateOptionValues>(args);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(rfl::unwrap(res->output_dir), "/tmp/out");
  EXPECT_EQ(rfl::unwrap(res->optimize_runs), 500);
  ASSERT_TRUE(rfl::unwrap(res->include_path).has_value());
  EXPECT_EQ(*rfl::unwrap(res->include_path), (std::vector<std::string>{"/a"}));

  const auto help = rfl::cli::usage<SolcLike>("solc", true);
  ASSERT_TRUE(help) << help.error().what();
  EXPECT_NE(help->find("-o, --output-dir=<path>"), std::string::npos) << *help;
  EXPECT_NE(help->find("--optimize-runs=<n>"), std::string::npos) << *help;
  EXPECT_NE(help->find("Default: 200."), std::string::npos) << *help;
  // Already bracketed value names are not double-wrapped.
  EXPECT_NE(help->find("--include-path=<PATH>"), std::string::npos) << *help;
  EXPECT_EQ(help->find("<<PATH>>"), std::string::npos) << *help;
}

}  // namespace test_value_name
