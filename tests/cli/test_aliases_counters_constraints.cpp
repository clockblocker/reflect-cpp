#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <rfl/cli.hpp>
#include <string>

namespace test_aliases_counters_constraints {

using rfl::cli::Opt;
using namespace rfl::cli::attr;

struct CcacheStyleCli {
  Opt<std::optional<std::string>, Short<"d">, Alias<"directory">,
      Desc<"Cache directory">>
      dir{};
  Opt<std::optional<std::string>, Alias<"dump-manifest">, Alias<"dump-result">,
      Desc<"Inspect cache entry">>
      inspect{};
  Opt<std::optional<uint32_t>, Alias<"recompress-threads">,
      Alias<"trim-recompress-threads">, Desc<"Worker threads">>
      threads{};
  Opt<uint8_t, Short<"v">, Counter, Desc<"Increase verbosity">> verbose{
      uint8_t{0}};
  Opt<bool, Short<"s">, Desc<"Show stats">> show_stats = false;
  Opt<bool, Excludes<"clear">, Excludes<"zero-stats">> dry_run = false;
  Opt<bool, Short<"C">> clear = false;
  Opt<bool, Short<"z">> zero_stats = false;
  Opt<std::optional<std::string>, Needs<"trim-max-size">> trim_dir{};
  Opt<std::optional<std::string>> trim_max_size{};
};

using Procs = rfl::Processors<rfl::DefaultIfMissing, rfl::NoExtraFields,
                              rfl::cli::SeparateOptionValues,
                              rfl::cli::PureFlags, rfl::cli::LastValueWins,
                              rfl::cli::AllowAbbreviations>;

TEST(cli, multiple_long_aliases_and_abbreviations) {
  const auto res = rfl::cli::read_detailed<CcacheStyleCli, Procs>(
      {"--directory", "/tmp/cache", "--dump-result=foo.r",
       "--trim-recompress-threads=4"});
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->value.dir.value(), "/tmp/cache");
  EXPECT_EQ(res->value.inspect.value(), "foo.r");
  EXPECT_EQ(res->value.threads.value(), 4u);
  EXPECT_TRUE(res->was_provided("dir"));
  EXPECT_TRUE(res->was_provided("inspect"));
  EXPECT_TRUE(res->was_provided("threads"));
  ASSERT_EQ(res->occurrences.size(), 3u);
  EXPECT_EQ(res->occurrences[0].first, "dir");
  EXPECT_EQ(res->occurrences[1].first, "inspect");
  EXPECT_EQ(res->occurrences[2].first, "threads");

  // Abbreviation of alias resolves to canonical option name.
  const auto abbr =
      rfl::cli::read<CcacheStyleCli, Procs>({"--direc=/tmp/abbr"});
  ASSERT_TRUE(abbr.has_value()) << abbr.error().what();
  EXPECT_EQ(abbr->dir.value(), "/tmp/abbr");
}

TEST(cli, counter_flag_short_cluster_and_long) {
  const auto res = rfl::cli::read_detailed<CcacheStyleCli, Procs>(
      {"-vv", "--verbose", "-vs"});
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->value.verbose.value(), 4u);
  EXPECT_TRUE(res->value.show_stats.value());

  // Counter flag rejects explicit =value.
  const auto bad = rfl::cli::read<CcacheStyleCli, Procs>({"--verbose=2"});
  ASSERT_FALSE(bad.has_value());

  // Usage renders counter as bare repeatable flag without =<value>.
  const auto help = rfl::cli::usage<CcacheStyleCli, Procs>("ccache", true);
  ASSERT_TRUE(help.has_value()) << help.error().what();
  EXPECT_NE(help->find("-v, --verbose"), std::string::npos);
  EXPECT_EQ(help->find("--verbose="), std::string::npos);
  EXPECT_NE(help->find("Can be repeated"), std::string::npos);
}

TEST(cli, excludes_and_needs_constraints) {
  const auto conflict =
      rfl::cli::read<CcacheStyleCli, Procs>({"--dry-run", "-C"});
  ASSERT_FALSE(conflict.has_value());
  EXPECT_NE(conflict.error().what().find("'--dry-run' cannot be used with '--clear'"),
            std::string::npos);

  const auto missing_need =
      rfl::cli::read<CcacheStyleCli, Procs>({"--trim-dir=/tmp/cache"});
  ASSERT_FALSE(missing_need.has_value());
  EXPECT_NE(missing_need.error().what().find("'--trim-dir' requires '--trim-max-size'"),
            std::string::npos);

  const auto ok = rfl::cli::read<CcacheStyleCli, Procs>(
      {"--trim-dir=/tmp/cache", "--trim-max-size=1G"});
  ASSERT_TRUE(ok.has_value()) << ok.error().what();
}

TEST(cli, dynamic_options_counter_alias_constraints) {
  rfl::cli::DynamicOptions opts;
  opts.add_value("dir", "d")
      .add_alias("directory", "dir")
      .add_counter("verbose", "v")
      .add_flag("dry-run")
      .add_flag("clear", "C")
      .add_exclusion("dry-run", "clear");

  const auto res = opts.parse({"--directory", "/tmp/c", "-vvv"});
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->values.at("dir"), "/tmp/c");
  EXPECT_EQ(res->values.at("verbose"), "3");

  const auto conflict = opts.parse({"--dry-run", "-C"});
  ASSERT_FALSE(conflict.has_value());
}

}  // namespace test_aliases_counters_constraints
