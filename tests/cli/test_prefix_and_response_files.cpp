#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_prefix_and_response_files {

TEST(cli, dynamic_options_prefix_match_and_suggest) {
  rfl::cli::DynamicOptions opts;
  opts.single_dash_long = true;
  opts.separate_values = true;

  opts.add_flag("lint-only")
      .add_negated_alias("no-lint-only", "lint-only")
      .add_negated_alias("nolint-only", "lint-only")
      .add_flag("finline")
      .add_negated_alias("fno-inline", "finline")
      .add_value("top-module")
      .add_flag("secret-internal", {}, {.hidden = true})
      .add_prefix_flag("D")
      .add_prefix_flag("I")
      .add_prefix_flag("O")
      .add_prefix_flag("Wno-")
      .add_prefix_flag("Wwarn-")
      .add_prefix_value("debugi-");

  const auto r = opts.parse({"--lint-only", "-fno-inline", "-DFOO=1",
                             "-I/usr/include", "-O3", "-Wno-fatal",
                             "-debugi-V3Options", "5", "-top-module", "top",
                             "design.sv"});
  ASSERT_TRUE(r) << r.error().what();
  EXPECT_EQ(r->positional, (std::vector<std::string>{"design.sv"}));
  EXPECT_EQ(r->values.at("top-module"), "top");
  EXPECT_EQ(r->values.at("finline"), "false");

  ASSERT_EQ(r->detailed_occurrences.size(), 8u);
  EXPECT_EQ(r->detailed_occurrences[0].name, "lint-only");
  EXPECT_FALSE(r->detailed_occurrences[0].negated);

  EXPECT_EQ(r->detailed_occurrences[1].name, "finline");
  EXPECT_TRUE(r->detailed_occurrences[1].negated);

  EXPECT_EQ(r->detailed_occurrences[2].name, "D");
  EXPECT_EQ(r->detailed_occurrences[2].prefix_suffix, "FOO=1");
  EXPECT_EQ(r->detailed_occurrences[2].value, "FOO=1");

  EXPECT_EQ(r->detailed_occurrences[3].name, "I");
  EXPECT_EQ(r->detailed_occurrences[3].prefix_suffix, "/usr/include");

  EXPECT_EQ(r->detailed_occurrences[4].name, "O");
  EXPECT_EQ(r->detailed_occurrences[4].prefix_suffix, "3");

  EXPECT_EQ(r->detailed_occurrences[5].name, "Wno-");
  EXPECT_EQ(r->detailed_occurrences[5].prefix_suffix, "fatal");

  EXPECT_EQ(r->detailed_occurrences[6].name, "debugi-");
  EXPECT_EQ(r->detailed_occurrences[6].prefix_suffix, "V3Options");
  EXPECT_EQ(r->detailed_occurrences[6].value, "5");

  // Suggestions exclude hidden options and include negated aliases.
  const auto s1 = opts.suggest("-lint-onli");
  ASSERT_TRUE(s1.has_value());
  EXPECT_EQ(*s1, "-lint-only");

  const auto s2 = opts.suggest("-fno-inlin");
  ASSERT_TRUE(s2.has_value());
  EXPECT_EQ(*s2, "-fno-inline");

  const auto s_hidden = opts.suggest("-secret-interna");
  EXPECT_FALSE(s_hidden.has_value());
  EXPECT_EQ(opts.usage().find("secret-internal"), std::string::npos);
}

TEST(cli, expand_response_files) {
  const auto dir = std::filesystem::temp_directory_path() / "rfl_cli_rsp_test";
  std::filesystem::create_directories(dir);
  const auto inner = dir / "inner.rsp";
  const auto outer = dir / "outer.rsp";
  const auto cycle = dir / "cycle.rsp";

  {
    std::ofstream f(inner);
    f << "// slash comment\n"
      << "-DFOO=\"hello world\" 'single arg'\n";
  }
  {
    std::ofstream f(outer);
    f << "# hash comment\n"
      << "--lint-only\n"
      << "-f " << inner.string() << "\n"
      << "top.sv\n";
  }
  {
    std::ofstream f(cycle);
    f << "@" << cycle.string() << "\n";
  }

  rfl::cli::ResponseFileOptions ropts;
  ropts.flag_triggers = {"-f"};
  ropts.strip_slash_comments = true;

  const std::vector<std::string> input = {
      "@" + outer.string(), "--", "@" + inner.string()};
  const auto expanded = rfl::cli::expand_response_files(input, ropts);
  ASSERT_TRUE(expanded) << expanded.error().what();
  EXPECT_EQ(*expanded,
            (std::vector<std::string>{
                "--lint-only", "-DFOO=hello world", "single arg", "top.sv",
                "--", "@" + inner.string()}));

  // Cycle hits max_depth cleanly.
  const std::vector<std::string> cyc_input = {"@" + cycle.string()};
  const auto cyc_res = rfl::cli::expand_response_files(cyc_input, ropts);
  EXPECT_FALSE(cyc_res);

  // Missing file fails cleanly.
  const std::vector<std::string> miss_input = {"@/nonexistent_rfl_rsp_12345.rsp"};
  EXPECT_FALSE(rfl::cli::expand_response_files(miss_input, ropts));

  std::filesystem::remove_all(dir);
}

}  // namespace test_prefix_and_response_files
