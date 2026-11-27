#include <gtest/gtest.h>

#include <cstring>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_dynamic_options {

TEST(cli, dynamic_options_basic_and_sensitive) {
  rfl::cli::DynamicOptions opts;
  opts.allow_abbreviations = true;
  opts.add_value("timeout", "A", {.sensitive = true})
      .add_value("dir", "d")
      .add_optional_value("help", "#basic", "h")
      .add_flag("quiet", "q");

  std::string prog = "aria2c";
  std::string f1 = "-A";
  std::string v1 = "ALPHA";
  std::string f2 = "--di";
  std::string v2 = "BRAVO";
  std::string f3 = "-q";
  std::string p1 = "nonopt1";
  std::string p2 = "nonopt2";
  char* argv[] = {prog.data(), f1.data(), v1.data(), f2.data(),
                  v2.data(),   f3.data(), p1.data(), p2.data()};

  const auto res = opts.parse(8, argv);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  ASSERT_EQ(res->occurrences.size(), 3u);
  EXPECT_EQ(res->occurrences[0], std::make_pair(std::string("timeout"),
                                                std::string("ALPHA")));
  EXPECT_EQ(res->occurrences[1], std::make_pair(std::string("dir"),
                                                std::string("BRAVO")));
  EXPECT_EQ(res->occurrences[2], std::make_pair(std::string("quiet"),
                                                std::string("")));
  ASSERT_EQ(res->positional.size(), 2u);
  EXPECT_EQ(res->positional[0], "nonopt1");
  EXPECT_EQ(res->positional[1], "nonopt2");
  EXPECT_EQ(v1, "*****");
  EXPECT_EQ(v2, "BRAVO");
}

TEST(cli, dynamic_options_errors_and_optional_arg) {
  rfl::cli::DynamicOptions opts;
  opts.allow_abbreviations = true;
  opts.add_value("dir", "d")
      .add_value("daemon")
      .add_optional_value("help", "", "h");

  // Optional arg without value leaves next token as positional.
  const auto ok = opts.parse({"-h", "http://example.com"});
  ASSERT_TRUE(ok.has_value()) << ok.error().what();
  ASSERT_EQ(ok->occurrences.size(), 1u);
  EXPECT_EQ(ok->occurrences[0].first, "help");
  EXPECT_EQ(ok->occurrences[0].second, "");
  ASSERT_EQ(ok->positional.size(), 1u);
  EXPECT_EQ(ok->positional[0], "http://example.com");

  // Unknown long option rejected.
  const auto unk_long = opts.parse({"--unknown"});
  ASSERT_FALSE(unk_long.has_value());
  EXPECT_NE(unk_long.error().what().find("Unknown option '--unknown'"),
            std::string::npos);

  // Ambiguous prefix rejected.
  const auto ambig = opts.parse({"--d"});
  ASSERT_FALSE(ambig.has_value());
  EXPECT_NE(ambig.error().what().find("Option '--d' is ambiguous"),
            std::string::npos);

  // Unknown short option rejected.
  const auto unk_short = opts.parse({"-Z"});
  ASSERT_FALSE(unk_short.has_value());
  EXPECT_NE(unk_short.error().what().find("Unknown short argument: -Z"),
            std::string::npos);
}

struct GroupedCli {
  rfl::cli::Opt<bool, rfl::cli::attr::Group<"Core">,
                rfl::cli::attr::Desc<"Enable verbose output.">>
      verbose = false;
  rfl::cli::Opt<bool, rfl::cli::attr::Group<"Premium">,
                rfl::cli::attr::Desc<"Premium-only option.">>
      premium = false;
};

TEST(cli, usage_hidden_groups) {
  rfl::cli::UsageOptions opts;
  opts.detailed = true;
  opts.hidden_groups = {"Premium"};
  const auto hidden =
      rfl::cli::usage<GroupedCli, rfl::DefaultIfMissing, rfl::cli::PureFlags>(
          "tool", opts);
  ASSERT_TRUE(hidden.has_value()) << hidden.error().what();
  EXPECT_NE(hidden->find("--verbose"), std::string::npos);
  EXPECT_EQ(hidden->find("--premium"), std::string::npos);
  EXPECT_EQ(hidden->find("Premium:"), std::string::npos);

  opts.hidden_groups.clear();
  const auto shown =
      rfl::cli::usage<GroupedCli, rfl::DefaultIfMissing, rfl::cli::PureFlags>(
          "tool", opts);
  ASSERT_TRUE(shown.has_value()) << shown.error().what();
  EXPECT_NE(shown->find("--premium"), std::string::npos);
  EXPECT_NE(shown->find("Premium:"), std::string::npos);
}

}  // namespace test_dynamic_options
