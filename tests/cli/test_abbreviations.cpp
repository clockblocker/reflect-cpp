#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <utility>
#include <vector>

namespace test_abbreviations {

template <class T, class... Ps>
rfl::Result<T> read_exact(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "program");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

/// Reads with rfl::cli::AllowAbbreviations.
template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  return read_exact<T, rfl::cli::AllowAbbreviations, Ps...>(std::move(_args));
}

template <class T>
std::string error_of(const rfl::Result<T>& _result) {
  return _result ? std::string("(no error)")
                 : std::string(_result.error().what());
}

struct Database {
  std::optional<std::string> host;
  std::optional<int> port;
};

struct Export {
  std::optional<std::string> format;
  std::optional<std::string> format_version;
  std::optional<bool> force_export;
  std::optional<std::vector<std::string>> report;
  rfl::ImplicitValue<std::optional<std::string>, "all"> help_reports;
  rfl::Short<"o", std::optional<std::string>> output;
  rfl::Positional<std::optional<std::string>> input_file;
  std::optional<Database> database;
  std::optional<std::map<std::string, std::string>> labels;
};

TEST(cli, test_abbreviation_value) {
  const auto result = read<Export>({"--rep=a,b"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().report,
            (std::vector<std::string>{"a", "b"}));

  // With rfl::cli::SeparateOptionValues, also from the next argument.
  const auto separate = read<Export, rfl::cli::SeparateOptionValues>(
      {"--rep", "a", "in.txt"});
  ASSERT_TRUE(separate) << separate.error().what();
  EXPECT_EQ(separate.value().report, (std::vector<std::string>{"a"}));
  EXPECT_EQ(separate.value().input_file(), "in.txt");
}

TEST(cli, test_abbreviation_flag) {
  const auto result = read<Export>({"--forc", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().force_export, true);
  EXPECT_EQ(result.value().input_file(), "in.txt");

  // Like `--force-export`, an abbreviated long flag never takes the next
  // token, even if it is a bool literal.
  const auto with_value = read<Export>({"--force-e", "false"});
  ASSERT_TRUE(with_value) << with_value.error().what();
  EXPECT_EQ(with_value.value().force_export, true);
  EXPECT_EQ(with_value.value().input_file(), "false");

  const auto with_equals = read<Export>({"--forc=false"});
  ASSERT_TRUE(with_equals) << with_equals.error().what();
  EXPECT_EQ(with_equals.value().force_export, false);
}

TEST(cli, test_abbreviation_implicit_value) {
  const auto bare = read<Export>({"--help", "in.txt"});
  ASSERT_TRUE(bare) << bare.error().what();
  EXPECT_EQ(bare.value().help_reports().value(), "all");
  EXPECT_EQ(bare.value().input_file(), "in.txt");

  const auto with_value = read<Export>({"--help-r=memory"});
  ASSERT_TRUE(with_value) << with_value.error().what();
  EXPECT_EQ(with_value.value().help_reports().value(), "memory");
}

TEST(cli, test_abbreviation_exact_match_wins) {
  const auto result = read<Export>({"--format=csv", "--format-v=2"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().format, "csv");
  EXPECT_EQ(result.value().format_version, "2");
}

TEST(cli, test_abbreviation_ambiguous) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"--form", "csv"}, {"--form=csv"}, {"in.txt", "--form"}}) {
    const auto result = read<Export>(args);
    EXPECT_EQ(error_of(result),
              "Option '--form' is ambiguous; possibilities: '--format', "
              "'--format-version'.");
  }
  // Every candidate is listed.
  EXPECT_EQ(error_of(read<Export>({"--for=x"})),
            "Option '--for' is ambiguous; possibilities: '--force-export', "
            "'--format', '--format-version'.");
  // Past the end of `--format`, only `--format-version` is left.
  const auto unique = read<Export>({"--format-=1"});
  ASSERT_TRUE(unique) << unique.error().what();
  EXPECT_EQ(unique.value().format_version, "1");
}

TEST(cli, test_abbreviation_nested) {
  const auto result =
      read<Export>({"--database.h=db", "--database.p=5432"});
  ASSERT_TRUE(result) << result.error().what();
  ASSERT_TRUE(result.value().database);
  EXPECT_EQ(result.value().database->host, "db");
  EXPECT_EQ(result.value().database->port, 5432);

  // Only the last part of a dotted name is shortened, so the struct's name
  // cannot be, and an abbreviation cannot end at or inside it.
  const auto inner = read<Export, rfl::NoExtraFields>({"--datab.h=db"});
  EXPECT_EQ(error_of(inner), "Unknown option '--datab.h'.");
  const auto parent = read<Export, rfl::NoExtraFields>({"--data=x"});
  EXPECT_EQ(error_of(parent), "Unknown option '--data'.");
  const auto dot = read<Export, rfl::NoExtraFields>({"--database.=x"});
  EXPECT_EQ(error_of(dot), "Unknown option '--database.'.");
}

TEST(cli, test_abbreviation_positional_and_short_fields) {
  const auto result = read<Export>({"--inp=in.txt", "--out=o.csv"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input_file(), "in.txt");
  EXPECT_EQ(result.value().output(), "o.csv");

  EXPECT_EQ(error_of(read<Export>({"--inp=a", "b"})),
            "Conflict: positional argument and '--input-file' both provided.");
  EXPECT_EQ(error_of(read<Export>({"-o", "a", "--outp=b"})),
            "Conflict: '-o' and '--output' both provided.");
}

TEST(cli, test_abbreviation_duplicate) {
  const auto result = read<Export>({"--format-v=a", "--format-version=b"});
  EXPECT_EQ(error_of(result), "Duplicate argument: --format-version=b");
  const auto both_short =
      read<Export>({"--format-v=a", "--format-ver", "b"});
  EXPECT_EQ(error_of(both_short), "Duplicate argument: --format-ver");
}

TEST(cli, test_abbreviation_missing_value_names_the_option) {
  const auto result =
      read<Export, rfl::cli::SeparateOptionValues>({"in.txt", "--rep"});
  EXPECT_EQ(error_of(result), "Missing value for option '--report'.");
}

TEST(cli, test_abbreviation_never_into_map_keys) {
  // Map keys are not known in advance: `--labels.e` is the key "e".
  const auto key = read<Export, rfl::NoExtraFields>({"--labels.e=prod"});
  ASSERT_TRUE(key) << key.error().what();
  EXPECT_EQ(key.value().labels,
            (std::map<std::string, std::string>{{"e", "prod"}}));

  // The map's own name is not abbreviated either.
  const auto map_name = read<Export, rfl::NoExtraFields>({"--lab.env=prod"});
  EXPECT_EQ(error_of(map_name), "Unknown option '--lab.env'.");
}

TEST(cli, test_abbreviation_no_match) {
  // Without rfl::NoExtraFields an unknown option is ignored.
  const auto lenient = read<Export>({"--nope", "in.txt"});
  ASSERT_TRUE(lenient) << lenient.error().what();
  EXPECT_EQ(lenient.value().input_file(), "in.txt");
  EXPECT_FALSE(lenient.value().report);

  const auto strict = read<Export, rfl::NoExtraFields>({"--reps=a"});
  EXPECT_EQ(error_of(strict), "Unknown option '--reps'.");
}

TEST(cli, test_abbreviation_no_extra_fields) {
  const auto result =
      read<Export, rfl::NoExtraFields>({"--rep=a", "--forc"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().report, (std::vector<std::string>{"a"}));
  EXPECT_EQ(result.value().force_export, true);
}

TEST(cli, test_abbreviation_only_for_long_options) {
  const auto separated = read<Export>({"--", "--rep"});
  ASSERT_TRUE(separated) << separated.error().what();
  EXPECT_FALSE(separated.value().report);
  EXPECT_EQ(separated.value().input_file(), "--rep");

  // A single dash starts short options, never an abbreviated long option.
  const auto result = read<Export>({"-out.csv"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().output(), "ut.csv");
  const auto unknown = read<Export>({"-rep"});
  EXPECT_EQ(error_of(unknown), "Unknown short argument: -r (in '-rep').");
}

TEST(cli, test_no_abbreviations_by_default) {
  const auto ignored = read_exact<Export>({"--rep=a", "--for", "in.txt"});
  ASSERT_TRUE(ignored) << ignored.error().what();
  EXPECT_FALSE(ignored.value().report);
  EXPECT_FALSE(ignored.value().force_export);
  EXPECT_EQ(ignored.value().input_file(), "in.txt");

  const auto exact = read_exact<Export>({"--report=a", "--format=x"});
  ASSERT_TRUE(exact) << exact.error().what();
  EXPECT_EQ(exact.value().report, (std::vector<std::string>{"a"}));
  EXPECT_EQ(exact.value().format, "x");

  for (const auto& args : std::vector<std::vector<const char*>>{
           {"--rep=a"}, {"--rep", "a"}}) {
    const auto result = read_exact<Export, rfl::NoExtraFields>(args);
    EXPECT_EQ(error_of(result), "Unknown option '--rep'.");
  }
  EXPECT_EQ(error_of(read_exact<Export, rfl::NoExtraFields>({"--for"})),
            "Unknown option '--for'.");
  // The processors can come in any order.
  const auto other_order =
      read_exact<Export, rfl::NoExtraFields, rfl::cli::AllowAbbreviations>(
          {"--rep=a"});
  ASSERT_TRUE(other_order) << other_order.error().what();
  EXPECT_EQ(other_order.value().report, (std::vector<std::string>{"a"}));
}

TEST(cli, test_abbreviation_resolve_args) {
  // The two-step API expands abbreviations, too, but parse_argv does not
  // know the struct, so the value must follow '='.
  const char* args[] = {"program", "--rep=a", "--forc", "--database.h=db"};
  const auto result =
      rfl::cli::parse_argv(4, const_cast<char**>(args))
          .and_then(rfl::cli::resolve_args<
                    Export, rfl::Processors<rfl::SnakeCaseToKebabCase,
                                            rfl::cli::AllowAbbreviations>>);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().at("report"), "a");
  EXPECT_EQ(result.value().at("force-export"), "");
  EXPECT_EQ(result.value().at("database.host"), "db");

  const char* ambiguous[] = {"program", "--for=x"};
  const auto error =
      rfl::cli::parse_argv(2, const_cast<char**>(ambiguous))
          .and_then(rfl::cli::resolve_args<
                    Export, rfl::Processors<rfl::SnakeCaseToKebabCase,
                                            rfl::cli::AllowAbbreviations>>);
  EXPECT_EQ(error_of(error),
            "Option '--for' is ambiguous; possibilities: '--force-export', "
            "'--format', '--format-version'.");
}

struct Renamed {
  rfl::Rename<"outDir", std::optional<std::string>> out_dir;
  std::optional<bool> dry_run;
};

TEST(cli, test_abbreviation_renamed_and_kebab_case) {
  // A field has one long name: rfl::Rename replaces it.
  const auto result = read<Renamed, rfl::NoExtraFields>({"--out=x", "--dry"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().out_dir(), "x");
  EXPECT_EQ(result.value().dry_run, true);

  const auto old_name = read<Renamed, rfl::NoExtraFields>({"--out-d=x"});
  EXPECT_EQ(error_of(old_name), "Unknown option '--out-d'.");
}

struct WithExtra {
  std::optional<std::string> report;
  rfl::ExtraFields<std::string> extra;
};

TEST(cli, test_abbreviation_not_with_extra_fields) {
  // Under rfl::ExtraFields every name is a key of its own, so nothing is
  // abbreviated.
  const auto result = read<WithExtra>({"--rep=a", "--report=b"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().report, "b");
  EXPECT_EQ(result.value().extra.at("rep"), "a");
}

}  // namespace test_abbreviations
