#include <gtest/gtest.h>

#include <optional>
#include <rfl/cli.hpp>
#include <string>

namespace test_negatable_and_single_dash {

using rfl::cli::Opt;
using namespace rfl::cli::attr;

struct MambaStyleFlags {
  Opt<bool, Negatable<>> pyc = true;
  Opt<bool, NegatedName<"run-link-scripts">> skip_run_link_scripts = false;
  Opt<bool, Alias<"no-builds">, NegatedName<"build">> no_build = false;
};

using MambaProcs =
    rfl::Processors<rfl::DefaultIfMissing, rfl::NoExtraFields,
                    rfl::cli::PureFlags, rfl::cli::LastValueWins>;

TEST(cli, negatable_and_explicit_negated_name) {
  const auto r1 = rfl::cli::read<MambaStyleFlags, MambaProcs>(
      {"--no-pyc", "--skip-run-link-scripts", "--no-builds"});
  ASSERT_TRUE(r1.has_value()) << r1.error().what();
  EXPECT_FALSE(r1->pyc.value());
  EXPECT_TRUE(r1->skip_run_link_scripts.value());
  EXPECT_TRUE(r1->no_build.value());

  // LastValueWins reconciles positive <-> negated in order.
  const auto r2 = rfl::cli::read<MambaStyleFlags, MambaProcs>(
      {"--no-pyc", "--pyc", "--skip-run-link-scripts", "--run-link-scripts",
       "--no-build", "--build"});
  ASSERT_TRUE(r2.has_value()) << r2.error().what();
  EXPECT_TRUE(r2->pyc.value());
  EXPECT_FALSE(r2->skip_run_link_scripts.value());
  EXPECT_FALSE(r2->no_build.value());
}

struct CompilerSingleDashCli {
  Opt<std::optional<std::string>> datadir{};
  Opt<bool, Negatable<"no">> regtest = false;
  Opt<bool, NegatedName<"fno-inline">> finline = true;
};

using SingleDashProcs =
    rfl::Processors<rfl::DefaultIfMissing, rfl::NoExtraFields,
                    rfl::cli::PureFlags, rfl::cli::LastValueWins,
                    rfl::cli::SingleDashLong>;

TEST(cli, single_dash_long_and_fno_negation) {
  const auto r = rfl::cli::read<CompilerSingleDashCli, SingleDashProcs>(
      {"-datadir=/tmp/btc", "-regtest", "-noregtest", "-fno-inline"});
  ASSERT_TRUE(r.has_value()) << r.error().what();
  EXPECT_EQ(r->datadir.value(), "/tmp/btc");
  EXPECT_FALSE(r->regtest.value());
  EXPECT_FALSE(r->finline.value());
}

TEST(cli, dynamic_options_bitcoin_universal_negation_and_detailed_occurrences) {
  rfl::cli::DynamicOptions opts;
  opts.single_dash_long = true;
  opts.separate_values = false;
  opts.options_first = true;
  opts.allow_universal_negation = true;
  opts.negation_prefix = "no";
  opts.add_optional_value("foo")
      .add_optional_value("bar")
      .add_optional_value("datadir");

  const auto r = opts.parse(
      {"-nofoo", "-foo", "-nobar=0", "--datadir=/data", "getinfo", "-extra"});
  ASSERT_TRUE(r.has_value()) << r.error().what();
  ASSERT_EQ(r->detailed_occurrences.size(), 4u);

  EXPECT_EQ(r->detailed_occurrences[0].name, "foo");
  EXPECT_TRUE(r->detailed_occurrences[0].negated);
  EXPECT_FALSE(r->detailed_occurrences[0].has_value);
  EXPECT_EQ(r->detailed_occurrences[0].raw, "-nofoo");

  EXPECT_EQ(r->detailed_occurrences[1].name, "foo");
  EXPECT_FALSE(r->detailed_occurrences[1].negated);
  EXPECT_FALSE(r->detailed_occurrences[1].has_value);

  EXPECT_EQ(r->detailed_occurrences[2].name, "bar");
  EXPECT_TRUE(r->detailed_occurrences[2].negated);
  EXPECT_TRUE(r->detailed_occurrences[2].has_value);
  EXPECT_EQ(r->detailed_occurrences[2].value, "0");

  EXPECT_EQ(r->detailed_occurrences[3].name, "datadir");
  EXPECT_FALSE(r->detailed_occurrences[3].negated);
  EXPECT_TRUE(r->detailed_occurrences[3].has_value);
  EXPECT_EQ(r->detailed_occurrences[3].value, "/data");

  ASSERT_EQ(r->positional.size(), 2u);
  EXPECT_EQ(r->positional[0], "getinfo");
  EXPECT_EQ(r->positional[1], "-extra");
}

}  // namespace test_negatable_and_single_dash
