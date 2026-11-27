#include <gtest/gtest.h>

#include <cstring>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_parsing_modes_and_occurrences {

using namespace rfl::cli::attr;

struct AnalyzerOpts {
  rfl::cli::Opt<rfl::DefaultVal<bool>, Short<"CC">, Desc<"Keep comments">>
      keep_comments = false;
  rfl::cli::Opt<std::optional<std::string>, Short<"rp">, Implicit<"">,
                Desc<"Relative paths">>
      relative_paths;
  rfl::cli::Opt<rfl::DefaultVal<bool>, Short<"f">, Desc<"Force">> force = false;
  rfl::cli::Opt<std::optional<int>, Desc<"Max configs">> max_configs;
  rfl::cli::Opt<std::optional<std::string>, Short<"j">, Desc<"Jobs">> jobs;
  rfl::cli::Opt<std::optional<std::vector<std::string>>, Desc<"Enable checks">>
      enable;
  rfl::cli::Opt<std::optional<std::vector<std::string>>, Desc<"Disable checks">>
      disable;
  rfl::cli::Opt<std::optional<std::string>, Short<"A">, Sensitive,
                Desc<"Secret token">>
      secret;
  rfl::cli::Opt<std::optional<std::vector<std::string>>, Positional> files;
};

using StrictEqCli =
    rfl::Processors<rfl::NoExtraFields, rfl::cli::RequireEquals,
                    rfl::cli::LastValueWins, rfl::cli::PureFlags,
                    rfl::cli::ListDelimiter<"">>;

TEST(cli, test_multi_char_short_options) {
  const auto res = rfl::cli::read<AnalyzerOpts, StrictEqCli>(
      {"-CC", "-rp=src;inc", "main.cpp"});
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_TRUE(rfl::unwrap(res->keep_comments));
  EXPECT_EQ(rfl::unwrap(res->relative_paths),
            std::optional<std::string>("src;inc"));

  const auto bare_rp =
      rfl::cli::read<AnalyzerOpts, StrictEqCli>({"-rp", "main.cpp"});
  ASSERT_TRUE(bare_rp) << bare_rp.error().what();
  EXPECT_EQ(rfl::unwrap(bare_rp->relative_paths),
            std::optional<std::string>(""));
}

TEST(cli, test_require_equals_rejects_bare_value_option) {
  const auto missing =
      rfl::cli::read<AnalyzerOpts, StrictEqCli>({"--max-configs", "main.cpp"});
  ASSERT_FALSE(missing);
  EXPECT_NE(std::string(missing.error().what()).find("Missing value"),
            std::string::npos);

  // ImplicitValue and flags still work without '='.
  const auto ok = rfl::cli::read<AnalyzerOpts, StrictEqCli>(
      {"--relative-paths", "--force", "--max-configs=5", "main.cpp"});
  ASSERT_TRUE(ok) << ok.error().what();
  EXPECT_TRUE(rfl::unwrap(ok->force));
  EXPECT_EQ(rfl::unwrap(ok->max_configs), std::optional<int>(5));
  // RequireEquals takes precedence even when combined with SeparateOptionValues.
  using CombinedProcs =
      rfl::Processors<rfl::NoExtraFields, rfl::cli::SeparateOptionValues,
                      rfl::cli::RequireEquals, rfl::cli::LastValueWins,
                      rfl::cli::PureFlags>;
  const auto combined_missing =
      rfl::cli::read<AnalyzerOpts, CombinedProcs>({"--max-configs", "5"});
  ASSERT_FALSE(combined_missing);
  const auto combined_ok =
      rfl::cli::read<AnalyzerOpts, CombinedProcs>({"--max-configs=5"});
  ASSERT_TRUE(combined_ok) << combined_ok.error().what();
  EXPECT_EQ(rfl::unwrap(combined_ok->max_configs), std::optional<int>(5));
}

TEST(cli, test_last_value_wins_and_occurrences_order) {
  const auto res = rfl::cli::read_detailed<AnalyzerOpts, StrictEqCli>(
      {"-f", "--max-configs=3", "--max-configs=7", "--force", "-j", "2", "-j4",
       "--enable=all", "--disable=style", "--enable=warning", "a.cpp"});
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_TRUE(rfl::unwrap(res->value.force));
  EXPECT_EQ(rfl::unwrap(res->value.max_configs), std::optional<int>(7));
  EXPECT_EQ(rfl::unwrap(res->value.jobs), std::optional<std::string>("4"));

  // Verify exact argv occurrence order with canonical long names.
  using Pair = std::pair<std::string, std::string>;
  const std::vector<Pair> expected = {
      {"force", ""},           {"max-configs", "3"},
      {"max-configs", "7"},    {"force", ""},
      {"jobs", "2"},           {"jobs", "4"},
      {"enable", "all"},       {"disable", "style"},
      {"enable", "warning"},
  };
  EXPECT_EQ(res->occurrences, expected);
}

TEST(cli, test_sensitive_masks_mutable_argv_in_place) {
  char prog[] = "tool";
  char opt1[] = "-A";
  char val1[] = "ALPHA";
  char opt2[] = "--secret=TOPSECRET";
  char file[] = "a.cpp";
  char* argv[] = {prog, opt1, val1, opt2, file};

  const auto res = rfl::cli::read_detailed<AnalyzerOpts, StrictEqCli>(5, argv);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(rfl::unwrap(res->value.secret),
            std::optional<std::string>("TOPSECRET"));
  // Both sensitive values were overwritten in place with '*'.
  EXPECT_STREQ(val1, "*****");
  EXPECT_STREQ(opt2, "--secret=*********");
  // Non-sensitive args remain untouched.
  EXPECT_STREQ(file, "a.cpp");
}

}  // namespace test_parsing_modes_and_occurrences
