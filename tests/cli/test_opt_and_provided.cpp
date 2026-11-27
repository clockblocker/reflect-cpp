#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

namespace test_opt_and_provided {

using namespace rfl::cli::attr;

struct FlatOpts {
  rfl::cli::Opt<std::optional<std::string>, Short<"o">, Group<"Output">,
                ValueName<"path">, Desc<"Write output to path">>
      output_dir;

  rfl::cli::Opt<rfl::DefaultVal<bool>, Short<"v">, Group<"General">,
                Desc<"Verbose logging">>
      verbose = false;

  rfl::cli::Opt<std::optional<int>, Group<"Optimizer">, ValueName<"n">,
                DefaultHint<"200">, Desc<"Optimization runs">>
      optimize_runs;

  rfl::cli::Opt<std::optional<std::vector<std::string>>, Group<"General">,
                MultiToken, ValueName<"items">, Desc<"Multiple items">>
      items;

  rfl::cli::Opt<std::optional<std::string>, Group<"General">, Implicit<"all">,
                Desc<"Show report">>
      report;

  rfl::cli::Opt<std::optional<int>, Group<"General">,
                Deprecated<"Use --optimize-runs", "Legacy runs">>
      old_runs;

  rfl::cli::Opt<rfl::DefaultVal<bool>, Hidden> secret = false;

  rfl::cli::Opt<std::optional<std::string>, Positional, Desc<"Input file">>
      input;
};

using CliSettings =
    rfl::Processors<rfl::NoExtraFields, rfl::cli::SeparateOptionValues,
                    rfl::cli::PureFlags, rfl::cli::ListDelimiter<"">>;

TEST(cli, test_opt_parsing_and_unwrap) {
  const auto res = rfl::cli::read<FlatOpts, CliSettings>(
      {"-v", "-o", "/tmp/out", "--optimize-runs", "200", "--items", "a", "b",
       "--report", "--secret", "file.sol"});
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(rfl::unwrap(res->output_dir), std::optional<std::string>("/tmp/out"));
  EXPECT_TRUE(rfl::unwrap(res->verbose));
  EXPECT_EQ(rfl::unwrap(res->optimize_runs), std::optional<int>(200));
  ASSERT_TRUE(rfl::unwrap(res->items).has_value());
  EXPECT_EQ(*rfl::unwrap(res->items), (std::vector<std::string>{"a", "b"}));
  EXPECT_EQ(rfl::unwrap(res->report), std::optional<std::string>("all"));
  EXPECT_TRUE(rfl::unwrap(res->secret));
  EXPECT_EQ(rfl::unwrap(res->input), std::optional<std::string>("file.sol"));

  // Opt is transparent to JSON serialization too.
  const auto json = rfl::json::write(*res);
  EXPECT_NE(json.find("\"output_dir\":\"/tmp/out\""), std::string::npos) << json;
}

TEST(cli, test_opt_usage_and_default_hint) {
  const auto help = rfl::cli::usage<FlatOpts, CliSettings>("tool", true);
  ASSERT_TRUE(help) << help.error().what();

  EXPECT_NE(help->find("\nOutput:\n"), std::string::npos) << *help;
  EXPECT_NE(help->find("-o, --output-dir=<path>"), std::string::npos) << *help;
  EXPECT_NE(help->find("Write output to path"), std::string::npos) << *help;

  EXPECT_NE(help->find("\nOptimizer:\n"), std::string::npos) << *help;
  EXPECT_NE(help->find("--optimize-runs=<n>"), std::string::npos) << *help;
  EXPECT_NE(help->find("Optimization runs. Default: 200."), std::string::npos)
      << *help;

  EXPECT_NE(help->find("--items=<items>..."), std::string::npos) << *help;
  EXPECT_NE(help->find("--report[=<value>]"), std::string::npos) << *help;
  EXPECT_NE(help->find("Without a value: all."), std::string::npos) << *help;
  EXPECT_NE(help->find("Legacy runs. Deprecated: Use --optimize-runs."),
            std::string::npos)
      << *help;
  // Hidden option omitted from usage.
  EXPECT_EQ(help->find("secret"), std::string::npos) << *help;
}

TEST(cli, test_opt_value_name_brackets_and_default_hint_optional) {
  struct Config {
    rfl::cli::Opt<std::optional<int>, DefaultHint<"42">, Desc<"Answer">> answer;
    rfl::cli::Opt<std::optional<std::vector<std::string>>, ValueName<"<PATH>">>
        include_path;
  };
  const auto empty = rfl::cli::read<Config>({});
  ASSERT_TRUE(empty) << empty.error().what();
  EXPECT_FALSE(rfl::unwrap(empty->answer).has_value());

  const auto given = rfl::cli::read<Config>({"--answer=42"});
  ASSERT_TRUE(given) << given.error().what();
  EXPECT_EQ(rfl::unwrap(given->answer), std::optional<int>(42));

  const auto help = rfl::cli::usage<Config>("app", true);
  ASSERT_TRUE(help) << help.error().what();
  EXPECT_NE(help->find("Answer. Default: 42."), std::string::npos) << *help;
  // Already bracketed value names are not double-wrapped.
  EXPECT_NE(help->find("--include-path=<PATH>"), std::string::npos) << *help;
  EXPECT_EQ(help->find("<<PATH>>"), std::string::npos) << *help;
}

TEST(cli, test_opt_multi_token_long_short_and_double_dash) {
  struct PhaserLike {
    rfl::cli::Opt<std::optional<std::vector<std::string>>, MultiToken,
                  ValueName<"<CHROMOSOMES>">>
        population;
    rfl::cli::Opt<std::optional<std::vector<std::string>>, Short<"p">,
                  MultiToken>
        files;
    std::optional<int> rounds;
    rfl::cli::Opt<std::optional<std::vector<std::string>>, Positional>
        input_files;
  };

  const auto r1 = rfl::cli::read<PhaserLike, CliSettings>(
      {"--population", "a", "afc", "xadd", "--rounds", "10", "--", "prog.yul"});
  ASSERT_TRUE(r1) << r1.error().what();
  ASSERT_TRUE(rfl::unwrap(r1->population).has_value());
  EXPECT_EQ(*rfl::unwrap(r1->population),
            (std::vector<std::string>{"a", "afc", "xadd"}));
  EXPECT_EQ(r1->rounds, 10);
  ASSERT_TRUE(rfl::unwrap(r1->input_files).has_value());
  EXPECT_EQ(*rfl::unwrap(r1->input_files),
            (std::vector<std::string>{"prog.yul"}));

  const auto r2 = rfl::cli::read<PhaserLike, CliSettings>(
      {"--population", "a", "b", "--population=c", "-p", "f1", "f2"});
  ASSERT_TRUE(r2) << r2.error().what();
  EXPECT_EQ(*rfl::unwrap(r2->population),
            (std::vector<std::string>{"a", "b", "c"}));
  EXPECT_EQ(*rfl::unwrap(r2->files), (std::vector<std::string>{"f1", "f2"}));

  const auto help = rfl::cli::usage<PhaserLike, CliSettings>("yul-phaser", true);
  ASSERT_TRUE(help) << help.error().what();
  EXPECT_NE(help->find("--population=<CHROMOSOMES>..."), std::string::npos)
      << *help;
  EXPECT_NE(help->find("-p, --files=<value>..."), std::string::npos) << *help;
}

TEST(cli, test_read_detailed_tracks_explicitly_provided) {
  const auto res = rfl::cli::read_detailed<FlatOpts, CliSettings>(
      {"-v", "--optimize-runs", "200", "--verbose=false"});
  // Duplicate --verbose should still fail cleanly.
  EXPECT_FALSE(res);

  const auto ok = rfl::cli::read_detailed<FlatOpts, CliSettings>(
      {"-o", "dir", "--verbose=false", "--optimize-runs", "200", "in.sol"});
  ASSERT_TRUE(ok) << ok.error().what();
  // Even though --verbose was explicitly set to its default value `false`,
  // was_provided("verbose") is true!
  EXPECT_FALSE(rfl::unwrap(ok->value.verbose));
  EXPECT_TRUE(ok->was_provided("verbose"));
  EXPECT_TRUE(ok->was_provided("output-dir"));
  EXPECT_TRUE(ok->was_provided("optimize-runs"));
  EXPECT_TRUE(ok->was_provided("input"));
  EXPECT_FALSE(ok->was_provided("items"));
  EXPECT_FALSE(ok->was_provided("report"));
  EXPECT_FALSE(ok->was_provided("secret"));
  EXPECT_EQ(ok->provided, (std::vector<std::string>{"input", "optimize-runs",
                                                    "output-dir", "verbose"}));
}

}  // namespace test_opt_and_provided
