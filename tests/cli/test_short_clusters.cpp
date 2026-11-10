#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_short_clusters {

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

struct Opts {
  rfl::Short<"a", bool> all;
  rfl::Short<"b", rfl::DefaultVal<bool>> brief = false;
  rfl::Short<"c", std::optional<bool>> color;
  rfl::Short<"q", rfl::DefaultVal<bool>> quiet = false;
  rfl::Short<"f", std::optional<std::string>> format;
  rfl::Short<"p", std::optional<int>> port;
  rfl::Short<"h", rfl::ImplicitValue<std::optional<std::string>>> help;
  rfl::Short<"l", rfl::ImplicitValue<std::optional<int>, "3">> level;
  rfl::Positional<std::optional<std::string>> input;
  rfl::Positional<std::optional<std::string>> output;
};

TEST(cli, test_short_cluster_flags) {
  const auto result = read<Opts>({"-abc"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& o = result.value();
  EXPECT_TRUE(o.all());
  EXPECT_TRUE(o.brief().value());
  EXPECT_EQ(o.color(), true);
  EXPECT_FALSE(o.quiet().value());
  EXPECT_FALSE(o.format());
}

TEST(cli, test_short_cluster_flags_any_order) {
  const auto result = read<Opts>({"-cba", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().all());
  EXPECT_TRUE(result.value().brief().value());
  EXPECT_EQ(result.value().color(), true);
  EXPECT_EQ(result.value().input(), "in.txt");
}

TEST(cli, test_short_attached_value) {
  const auto result = read<Opts>({"-a", "-fcsv"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().format(), "csv");
}

TEST(cli, test_short_cluster_then_attached_value) {
  const auto result = read<Opts>({"-a", "-qfcsv", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().quiet().value());
  EXPECT_EQ(result.value().format(), "csv");
  EXPECT_EQ(result.value().input(), "in.txt");
}

TEST(cli, test_short_cluster_value_takes_next_token) {
  const auto result = read<Opts>({"-a", "-qf", "csv", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().quiet().value());
  EXPECT_EQ(result.value().format(), "csv");
  EXPECT_EQ(result.value().input(), "in.txt");
}

TEST(cli, test_short_cluster_value_takes_rest_verbatim) {
  // Everything after the first option that takes a value is its value, even
  // letters that are short options themselves.
  const auto result = read<Opts>({"-afabc"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().all());
  EXPECT_FALSE(result.value().brief().value());
  EXPECT_EQ(result.value().format(), "abc");
}

TEST(cli, test_short_cluster_value_missing) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"-a", "-qf"}, {"-aqf", "-b"}, {"-aqf", "--format=x"}}) {
    const auto result = read<Opts>(args);
    EXPECT_EQ(error_of(result), "Missing value for option '-f'.");
  }
}

TEST(cli, test_short_equals_value) {
  // rfl keeps `-f=csv` meaning "csv", also in a group. getopt would give
  // "=csv".
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"-a", "-f=csv"}, {"-aqf=csv"}}) {
    const auto result = read<Opts>(args);
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().format(), "csv");
  }
  const auto empty = read<Opts>({"-af="});
  ASSERT_TRUE(empty) << empty.error().what();
  EXPECT_EQ(empty.value().format(), "");
  const auto twice = read<Opts>({"-af==x"});
  ASSERT_TRUE(twice) << twice.error().what();
  EXPECT_EQ(twice.value().format(), "=x");
}

TEST(cli, test_short_flag_equals_value_alone) {
  const auto result = read<Opts>({"-a=false", "-q=true"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_FALSE(result.value().all());
  EXPECT_TRUE(result.value().quiet().value());
}

TEST(cli, test_short_cluster_flag_takes_no_attached_value) {
  const auto result = read<Opts>({"-aq=false"});
  EXPECT_EQ(error_of(result),
            "Option '-q' does not take a value in '-aq=false'. Write "
            "'-q=false' on its own.");
  const auto second = read<Opts>({"-a", "-bq=false"});
  EXPECT_EQ(error_of(second),
            "Option '-q' does not take a value in '-bq=false'. Write "
            "'-q=false' on its own.");
}

TEST(cli, test_short_cluster_last_flag_takes_bool_literal) {
  // The last option of a group treats the next token as it would alone.
  const auto result = read<Opts>({"-qa", "false", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().quiet().value());
  EXPECT_FALSE(result.value().all());
  EXPECT_EQ(result.value().input(), "in.txt");
  EXPECT_FALSE(result.value().output());
}

TEST(cli, test_short_cluster_flag_does_not_take_other_tokens) {
  const auto result = read<Opts>({"-ab", "in.txt", "out.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().all());
  EXPECT_TRUE(result.value().brief().value());
  EXPECT_EQ(result.value().input(), "in.txt");
  EXPECT_EQ(result.value().output(), "out.txt");
}

TEST(cli, test_short_cluster_unknown_letter) {
  const auto middle = read<Opts>({"-axf", "csv"});
  EXPECT_EQ(error_of(middle), "Unknown short argument: -x (in '-axf').");
  const auto first = read<Opts>({"-xa"});
  EXPECT_EQ(error_of(first), "Unknown short argument: -x (in '-xa').");
  const auto last = read<Opts>({"-abz"});
  EXPECT_EQ(error_of(last), "Unknown short argument: -z (in '-abz').");
  // A character that takes several bytes in UTF-8 is named whole.
  const auto utf8 = read<Opts>({"-a\xC3\xA9", "-\xE2\x82\xAC"});
  EXPECT_EQ(error_of(utf8),
            "Unknown short argument: -\xC3\xA9 (in '-a\xC3\xA9').");
  const auto utf8_alone = read<Opts>({"-a", "-\xE2\x82\xAC"});
  EXPECT_EQ(error_of(utf8_alone), "Unknown short argument: -\xE2\x82\xAC");
}

TEST(cli, test_short_unknown_alone) {
  const auto alone = read<Opts>({"-a", "-x"});
  EXPECT_EQ(error_of(alone), "Unknown short argument: -x");
  const auto with_value = read<Opts>({"-a", "-x=1"});
  EXPECT_EQ(error_of(with_value), "Unknown short argument: -x");
}

TEST(cli, test_short_cluster_duplicate) {
  const auto result = read<Opts>({"-aa"});
  EXPECT_EQ(error_of(result), "Duplicate short argument: -aa");
  const auto across = read<Opts>({"-ab", "-ba"});
  EXPECT_EQ(error_of(across), "Duplicate short argument: -ba");
}

TEST(cli, test_short_cluster_conflicts_with_long) {
  const auto result = read<Opts>({"-afcsv", "--format=json"});
  EXPECT_EQ(error_of(result),
            "Conflict: '-f' and '--format' both provided.");
}

TEST(cli, test_short_cluster_implicit_value_attached) {
  // getopt's optional argument: the rest of the token, if any.
  const auto result = read<Opts>({"-a", "-hfoo"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().help().value(), "foo");

  const auto grouped = read<Opts>({"-aqhfoo", "-l7"});
  ASSERT_TRUE(grouped) << grouped.error().what();
  EXPECT_TRUE(grouped.value().quiet().value());
  EXPECT_EQ(grouped.value().help().value(), "foo");
  EXPECT_EQ(grouped.value().level().value(), 7);

  const auto equals = read<Opts>({"-aqh=foo"});
  ASSERT_TRUE(equals) << equals.error().what();
  EXPECT_EQ(equals.value().help().value(), "foo");
}

TEST(cli, test_short_cluster_implicit_value_bare) {
  // Without an attached value, the implicit value. The next token is never
  // taken.
  const auto result = read<Opts>({"-aqh", "in.txt", "-l"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().quiet().value());
  EXPECT_EQ(result.value().help().value(), "");
  EXPECT_EQ(result.value().level().value(), 3);
  EXPECT_EQ(result.value().input(), "in.txt");
}

TEST(cli, test_short_cluster_implicit_value_takes_rest_verbatim) {
  const auto result = read<Opts>({"-ahab"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().all());
  EXPECT_FALSE(result.value().brief().value());
  EXPECT_EQ(result.value().help().value(), "ab");
}

TEST(cli, test_short_negative_number_value) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"-a", "-p", "-42"}, {"-a", "-p-42"}, {"-ap", "-42"},
           {"-ap-42"}, {"-a", "-p=-42"}}) {
    const auto result = read<Opts>(args);
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().port(), -42);
  }
}

struct Numbers {
  rfl::Short<"v", bool> verbose;
  rfl::Short<"n", std::optional<double>> number;
  rfl::Positional<double> x;
  rfl::Positional<std::optional<double>> y;
};

TEST(cli, test_short_negative_number_positional) {
  const auto result = read<Numbers>({"-v", "-42", "-.5"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().verbose());
  EXPECT_DOUBLE_EQ(result.value().x(), -42.0);
  EXPECT_DOUBLE_EQ(result.value().y().value(), -0.5);

  const auto value = read<Numbers>({"-vn", "-1.5", "-3"});
  ASSERT_TRUE(value) << value.error().what();
  EXPECT_DOUBLE_EQ(value.value().number().value(), -1.5);
  EXPECT_DOUBLE_EQ(value.value().x(), -3.0);
}

struct DigitShort {
  rfl::Short<"1", bool> one_per_line;
  rfl::Short<"n", std::optional<int>> count;
  rfl::Positional<std::optional<int>> x;
};

TEST(cli, test_short_digit_option) {
  // As in argparse: once a short option's name looks like a number, every
  // token that looks like a negative number is a short option, and negative
  // positionals need "--".
  const auto result = read<DigitShort>({"-1", "--", "-5"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().one_per_line());
  EXPECT_EQ(result.value().x(), -5);

  const auto grouped = read<DigitShort>({"-1n2"});
  ASSERT_TRUE(grouped) << grouped.error().what();
  EXPECT_TRUE(grouped.value().one_per_line());
  EXPECT_EQ(grouped.value().count(), 2);

  // An option still takes a following negative number as its value.
  const auto value = read<DigitShort>({"-1", "-n", "-5"});
  ASSERT_TRUE(value) << value.error().what();
  EXPECT_EQ(value.value().count(), -5);
}

struct DigitShortNumbers {
  rfl::Short<"1", bool> one_per_line;
  rfl::Positional<std::optional<double>> x;
  rfl::Positional<std::optional<double>> y;
  rfl::Positional<std::optional<double>> z;
};

TEST(cli, test_short_digit_option_negative_numbers_are_groups) {
  EXPECT_EQ(error_of(read<DigitShort>({"-1", "-5"})),
            "Unknown short argument: -5");
  EXPECT_EQ(error_of(read<DigitShort>({"-2"})), "Unknown short argument: -2");
  EXPECT_EQ(error_of(read<DigitShort>({"-.5"})),
            "Unknown short argument: -. (in '-.5').");
  EXPECT_EQ(error_of(read<DigitShortNumbers>({"-1.5"})),
            "Unknown short argument: -. (in '-1.5').");
  EXPECT_EQ(error_of(read<DigitShortNumbers>({"-10"})),
            "Unknown short argument: -0 (in '-10').");
  EXPECT_EQ(error_of(read<DigitShortNumbers>({"-15"})),
            "Unknown short argument: -5 (in '-15').");

  const auto separated =
      read<DigitShortNumbers>({"-1", "--", "-1.5", "-10", "-15"});
  ASSERT_TRUE(separated) << separated.error().what();
  EXPECT_TRUE(separated.value().one_per_line());
  EXPECT_DOUBLE_EQ(separated.value().x().value(), -1.5);
  EXPECT_DOUBLE_EQ(separated.value().y().value(), -10.0);
  EXPECT_DOUBLE_EQ(separated.value().z().value(), -15.0);
}

TEST(cli, test_short_parse_argv_does_not_group) {
  // Without the target struct, parse_argv does not know the short options,
  // so -abc is the one short option "abc", and -5 the short option "5".
  const char* args[] = {"prog", "-abc", "x", "-de=f", "-g"};
  const auto parsed = rfl::cli::parse_argv(5, const_cast<char**>(args));
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed.value().short_args,
            (std::map<std::string, std::string>{
                {"abc", "x"}, {"de", "f"}, {"g", ""}}));
  EXPECT_TRUE(parsed.value().positional.empty());

  const auto range =
      rfl::cli::parse_argv(std::vector<std::string>{"-abc", "x", "-5"});
  ASSERT_TRUE(range) << range.error().what();
  EXPECT_EQ(range.value().short_args,
            (std::map<std::string, std::string>{{"abc", "x"}, {"5", ""}}));
  EXPECT_TRUE(range.value().positional.empty());
}

TEST(cli, test_short_cluster_after_separator) {
  const auto result = read<Opts>({"-ab", "--", "-qfcsv", "-x"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().all());
  EXPECT_FALSE(result.value().quiet().value());
  EXPECT_FALSE(result.value().format());
  EXPECT_EQ(result.value().input(), "-qfcsv");
  EXPECT_EQ(result.value().output(), "-x");
}

TEST(cli, test_short_cluster_value_after_separator_token) {
  // `--` after an option that needs a value is not its value.
  const auto result = read<Opts>({"-af", "--", "x"});
  EXPECT_EQ(error_of(result), "Missing value for option '-f'.");
}

TEST(cli, test_short_cluster_no_extra_fields) {
  const auto result =
      read<Opts, rfl::NoExtraFields>({"-abqfcsv", "-hx", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().all());
  EXPECT_EQ(result.value().format(), "csv");
  EXPECT_EQ(result.value().help().value(), "x");
  EXPECT_EQ(result.value().input(), "in.txt");

  const auto unknown = read<Opts, rfl::NoExtraFields>({"-aby"});
  EXPECT_EQ(error_of(unknown), "Unknown short argument: -y (in '-aby').");
}

struct Nested {
  rfl::Description<"Be quiet", rfl::Short<"q", bool>> quiet;
  rfl::DefaultVal<rfl::Short<"o", std::string>> output;
  std::optional<rfl::Short<"v", bool>> verbose;
};

TEST(cli, test_short_cluster_nested_wrappers) {
  const auto result = read<Nested>({"-vqoout.csv"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().quiet().value());
  EXPECT_EQ(result.value().output().value(), "out.csv");
  ASSERT_TRUE(result.value().verbose.has_value());
  EXPECT_TRUE(result.value().verbose.value().value());
}

TEST(cli, test_short_cluster_wide_char) {
  const wchar_t* args[] = {L"program", L"-aqfcsv", L"-hx", L"in.txt"};
  const auto result = rfl::cli::read<Opts>(4, const_cast<wchar_t**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().quiet().value());
  EXPECT_EQ(result.value().format(), "csv");
  EXPECT_EQ(result.value().help().value(), "x");
  EXPECT_EQ(result.value().input(), "in.txt");
}

TEST(cli, test_short_cluster_parse_argv) {
  const auto options = rfl::cli::internal::make_options<Opts, rfl::Processors<>>();
  const char* args[] = {"program", "-abfcsv", "-p", "-1", "-7"};
  const auto parsed =
      rfl::cli::internal::parse_argv(5, const_cast<char**>(args), options);
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed.value().short_args,
            (std::map<std::string, std::string>{
                {"a", ""}, {"b", ""}, {"f", "csv"}, {"p", "-1"}}));
  EXPECT_EQ(parsed.value().short_order,
            (std::vector<std::string>{"a", "b", "f", "p"}));
  EXPECT_EQ(parsed.value().positional, (std::vector<std::string>{"-7"}));
}

}  // namespace test_short_clusters
