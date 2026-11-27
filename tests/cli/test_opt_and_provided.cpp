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

TEST(cli, test_standalone_default_hint) {
  struct Config {
    rfl::DefaultHint<"42", rfl::Description<"Answer", std::optional<int>>>
        answer;
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
