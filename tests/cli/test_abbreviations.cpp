#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

namespace test_abbreviations {

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "program");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
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

TEST(cli, test_abbreviation_with_equals) {
  const auto result = read<Export>({"--rep=a,b"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().report,
            (std::vector<std::string>{"a", "b"}));
}

TEST(cli, test_abbreviation_with_space) {
  const auto result = read<Export>({"--rep", "a", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().report, (std::vector<std::string>{"a"}));
  EXPECT_EQ(result.value().input_file(), "in.txt");
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
  const auto result = read<Export>({"--format", "csv", "--format-v", "2"});
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
  // Past the end of `--format`, only `--format-version` is left.
  const auto unique = read<Export>({"--format-", "1"});
  ASSERT_TRUE(unique) << unique.error().what();
  EXPECT_EQ(unique.value().format_version, "1");
}

TEST(cli, test_abbreviation_ambiguous_lists_all_candidates) {
  const auto result = read<Export>({"--for", "x"});
  EXPECT_EQ(error_of(result),
            "Option '--for' is ambiguous; possibilities: '--force-export', "
            "'--format', '--format-version'.");
}

TEST(cli, test_abbreviation_ambiguous_with_no_extra_fields) {
  const auto result = read<Export, rfl::NoExtraFields>({"--for=x"});
  EXPECT_EQ(error_of(result),
            "Option '--for' is ambiguous; possibilities: '--force-export', "
            "'--format', '--format-version'.");
}

TEST(cli, test_abbreviation_nested) {
  const auto result =
      read<Export>({"--database.h", "db", "--database.p=5432"});
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
}

struct Cache {
  std::optional<std::string> host;
};

struct WithCache {
  std::optional<Cache> cache;
};

TEST(cli, test_abbreviation_does_not_end_in_parent) {
  // `cache.host` is the only option starting with `cac` or `cache.`, but
  // neither spells out `cache` and a part of `host`.
  for (const auto arg : {"--cac=db", "--cache.=db"}) {
    const auto lenient = read<WithCache>({arg});
    ASSERT_TRUE(lenient) << lenient.error().what();
    EXPECT_FALSE(lenient.value().cache && lenient.value().cache->host) << arg;
  }
  EXPECT_EQ(error_of(read<WithCache, rfl::NoExtraFields>({"--cac=db"})),
            "Unknown option '--cac'.");
  EXPECT_EQ(error_of(read<WithCache, rfl::NoExtraFields>({"--cache.=db"})),
            "Unknown option '--cache.'.");
  const auto last_part = read<WithCache>({"--cache.h=db"});
  ASSERT_TRUE(last_part) << last_part.error().what();
  ASSERT_TRUE(last_part.value().cache);
  EXPECT_EQ(last_part.value().cache->host, "db");
}

TEST(cli, test_abbreviation_positional_long_form) {
  const auto result = read<Export>({"--inp", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input_file(), "in.txt");

  const auto conflict = read<Export>({"--inp", "a", "b"});
  EXPECT_EQ(error_of(conflict),
            "Conflict: positional argument and '--input-file' both provided.");
}

TEST(cli, test_abbreviation_short_alias_long_form) {
  const auto result = read<Export>({"--out", "o.csv"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().output(), "o.csv");

  const auto conflict = read<Export>({"-o", "a", "--outp=b"});
  EXPECT_EQ(error_of(conflict),
            "Conflict: '-o' and '--output' both provided.");
}

TEST(cli, test_abbreviation_duplicate) {
  const auto result = read<Export>({"--format-v=a", "--format-version=b"});
  EXPECT_EQ(error_of(result), "Duplicate argument: --format-version=b");
  const auto both_short =
      read<Export>({"--format-v=a", "--format-ver", "b"});
  EXPECT_EQ(error_of(both_short), "Duplicate argument: --format-ver");

  // A list option adds the values of every spelling.
  const auto list = read<Export>({"--rep=a", "--report=b", "--re", "c"});
  ASSERT_TRUE(list) << list.error().what();
  EXPECT_EQ(list.value().report, (std::vector<std::string>{"a", "b", "c"}));
}

TEST(cli, test_abbreviation_missing_value_names_the_option) {
  const auto result = read<Export>({"in.txt", "--rep"});
  EXPECT_EQ(error_of(result), "Missing value for option '--report'.");
}

TEST(cli, test_abbreviation_never_into_map_keys) {
  // Map keys are not known in advance: `--labels.e` is the key "e".
  const auto key = read<Export, rfl::NoExtraFields>({"--labels.e", "prod"});
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
      read<Export, rfl::NoExtraFields>({"--rep", "a", "--forc", "--help-r"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().report, (std::vector<std::string>{"a"}));
  EXPECT_EQ(result.value().force_export, true);
  EXPECT_EQ(result.value().help_reports().value(), "all");
}

TEST(cli, test_abbreviation_not_after_separator) {
  const auto result = read<Export>({"--", "--rep"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_FALSE(result.value().report);
  EXPECT_EQ(result.value().input_file(), "--rep");
}

TEST(cli, test_abbreviation_not_for_short_options) {
  // A single dash starts short options, never an abbreviated long option.
  const auto result = read<Export>({"-out.csv"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().output(), "ut.csv");
  const auto unknown = read<Export>({"-rep"});
  EXPECT_EQ(error_of(unknown), "Unknown short argument: -r (in '-rep').");
}

TEST(cli, test_no_abbreviations) {
  const auto ignored = read<Export, rfl::cli::NoAbbreviations>(
      {"--rep=a", "--for", "in.txt"});
  ASSERT_TRUE(ignored) << ignored.error().what();
  EXPECT_FALSE(ignored.value().report);
  EXPECT_FALSE(ignored.value().force_export);
  EXPECT_EQ(ignored.value().input_file(), "in.txt");

  const auto exact =
      read<Export, rfl::cli::NoAbbreviations>({"--report=a", "--format=x"});
  ASSERT_TRUE(exact) << exact.error().what();
  EXPECT_EQ(exact.value().report, (std::vector<std::string>{"a"}));
  EXPECT_EQ(exact.value().format, "x");
}

TEST(cli, test_no_abbreviations_with_no_extra_fields) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"--rep=a"}, {"--rep", "a"}}) {
    const auto result =
        read<Export, rfl::NoExtraFields, rfl::cli::NoAbbreviations>(args);
    EXPECT_EQ(error_of(result), "Unknown option '--rep'.");
  }
  const auto other_order =
      read<Export, rfl::cli::NoAbbreviations, rfl::NoExtraFields>({"--for"});
  EXPECT_EQ(error_of(other_order), "Unknown option '--for'.");
}

TEST(cli, test_no_abbreviations_in_options) {
  EXPECT_TRUE((rfl::cli::internal::make_options<Export, rfl::Processors<>>()
                   .allow_abbreviations));
  EXPECT_FALSE((rfl::cli::internal::make_options<
                    Export, rfl::Processors<rfl::NoExtraFields,
                                            rfl::cli::NoAbbreviations>>()
                    .allow_abbreviations));
}

TEST(cli, test_no_abbreviations_ignored_by_other_formats) {
  Export e;
  e.format = "csv";
  e.report = std::vector<std::string>{"a"};
  e.database = Database{std::nullopt, 1};
  const auto json = rfl::json::write<rfl::cli::NoAbbreviations>(e);
  EXPECT_EQ(json, rfl::json::write(e));
  const auto back =
      rfl::json::read<Export, rfl::cli::NoAbbreviations>(json);
  ASSERT_TRUE(back) << back.error().what();
  EXPECT_EQ(back.value().format, "csv");
  ASSERT_TRUE(back.value().database);
  EXPECT_EQ(back.value().database->port, 1);
  EXPECT_EQ(rfl::json::write(back.value()), json);
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

TEST(cli, test_abbreviation_parse_argv) {
  const auto options = rfl::cli::internal::make_options<
      Export, rfl::Processors<rfl::SnakeCaseToKebabCase>>();
  EXPECT_EQ(options.find_abbreviations("rep"),
            (std::vector<std::string>{"report"}));
  EXPECT_EQ(options.find_abbreviations("form"),
            (std::vector<std::string>{"format", "format-version"}));
  EXPECT_TRUE(options.find_abbreviations("labels.e").empty());

  const char* args[] = {"program", "--rep", "a", "--database.h=x"};
  const auto parsed =
      rfl::cli::internal::parse_argv(4, const_cast<char**>(args), options);
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed.value().named,
            (std::map<std::string, std::string>{{"report", "a"},
                                                {"database.host", "x"}}));
}

TEST(cli, test_abbreviation_wide_char) {
  const wchar_t* args[] = {L"program", L"--rep", L"a", L"--forc"};
  const auto result = rfl::cli::read<Export>(4, const_cast<wchar_t**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().report, (std::vector<std::string>{"a"}));
  EXPECT_EQ(result.value().force_export, true);
}

}  // namespace test_abbreviations
