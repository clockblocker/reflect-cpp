#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_options_first {

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

using Strings = std::vector<std::string>;

// recipe [options] <name> <args...>: everything after the name goes to a
// backend, unchanged.
struct Recipe {
  rfl::Short<"v", std::optional<bool>> verbose;
  rfl::Short<"j", std::optional<int>> jobs;
  std::optional<Strings> env;
  rfl::ImplicitValue<std::optional<std::string>, "all"> help;
  rfl::Positional<std::string> name;
  rfl::Positional<rfl::DefaultVal<Strings>> args;
};

template <class... Ps>
rfl::Result<Recipe> recipe(std::vector<const char*> _args) {
  return read<Recipe, rfl::cli::OptionsFirst, rfl::cli::ListDelimiter<"">,
              Ps...>(std::move(_args));
}

TEST(cli, test_options_first_recipe) {
  const auto result =
      recipe({"-v", "--jobs", "4", "build", "--jobs", "8", "-x",
              "--unknown=1", "-v", "--", "y,z", ""});
  ASSERT_TRUE(result) << result.error().what();
  const auto& r = result.value();
  EXPECT_EQ(r.verbose(), true);
  EXPECT_EQ(r.jobs(), 4);
  EXPECT_EQ(r.name(), "build");
  EXPECT_EQ(r.args().value(), (Strings{"--jobs", "8", "-x", "--unknown=1",
                                       "-v", "--", "y,z", ""}));
}

TEST(cli, test_options_first_no_remaining_arguments) {
  const auto result = recipe({"-v", "build"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().name(), "build");
  EXPECT_EQ(result.value().args().value(), Strings{});

  EXPECT_EQ(error_of(recipe({"-v"})), "Field named 'name' not found.");
}

TEST(cli, test_options_first_double_dash) {
  // Before the first positional argument, '--' ends the options and is
  // dropped. After it, '--' is an argument, as in getopt's '+' mode.
  const auto before = recipe({"-v", "--", "-j", "build"});
  ASSERT_TRUE(before) << before.error().what();
  EXPECT_EQ(before.value().name(), "-j");
  EXPECT_EQ(before.value().args().value(), (Strings{"build"}));

  const auto twice = recipe({"--", "--", "build"});
  ASSERT_TRUE(twice) << twice.error().what();
  EXPECT_EQ(twice.value().name(), "--");
  EXPECT_EQ(twice.value().args().value(), (Strings{"build"}));

  const auto after = recipe({"build", "--"});
  ASSERT_TRUE(after) << after.error().what();
  EXPECT_EQ(after.value().args().value(), (Strings{"--"}));
}

TEST(cli, test_options_first_options_before_the_name) {
  // Short clusters, abbreviations, implicit values, bool literals and
  // repeated list options work as usual before the first positional
  // argument.
  const auto ok = recipe({"-vj4", "--env", "A=1", "--en=B=2", "--help",
                          "build", "-vj8", "--env", "C", "--help"});
  ASSERT_TRUE(ok) << ok.error().what();
  const auto& r = ok.value();
  EXPECT_EQ(r.verbose(), true);
  EXPECT_EQ(r.jobs(), 4);
  EXPECT_EQ(r.env, (Strings{"A=1", "B=2"}));
  EXPECT_EQ(r.help(), "all");
  EXPECT_EQ(r.name(), "build");
  EXPECT_EQ(r.args().value(), (Strings{"-vj8", "--env", "C", "--help"}));

  // A short bool flag takes a bool literal; a long one never does.
  const auto literal = recipe({"-v", "false", "build", "x"});
  ASSERT_TRUE(literal) << literal.error().what();
  EXPECT_EQ(literal.value().verbose(), false);
  EXPECT_EQ(literal.value().args().value(), (Strings{"x"}));
}

TEST(cli, test_options_first_negative_number_is_positional) {
  const auto result = recipe({"-j", "-1", "-2", "-v"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().jobs(), -1);
  EXPECT_EQ(result.value().name(), "-2");
  EXPECT_EQ(result.value().args().value(), (Strings{"-v"}));
}

TEST(cli, test_options_first_with_no_extra_fields) {
  EXPECT_EQ(error_of(recipe<rfl::NoExtraFields>({"--nope", "build"})),
            "Unknown option '--nope'.");
  EXPECT_EQ(error_of(recipe<rfl::NoExtraFields>({"-x", "build"})),
            "Unknown short argument: -x");

  const auto after = recipe<rfl::NoExtraFields>({"build", "--nope", "-x"});
  ASSERT_TRUE(after) << after.error().what();
  EXPECT_EQ(after.value().args().value(), (Strings{"--nope", "-x"}));
}

TEST(cli, test_options_first_long_form_of_positionals) {
  const auto result = recipe({"--args=x,y", "build"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().name(), "build");
  EXPECT_EQ(result.value().args().value(), (Strings{"x,y"}));
}

TEST(cli, test_options_first_splits_with_the_default_delimiter) {
  // Without ListDelimiter<"">, the remaining arguments are taken as they
  // are, but still split at the commas.
  const auto result = read<Recipe, rfl::cli::OptionsFirst>(
      {"build", "a,b", "--jobs=2,3"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_FALSE(result.value().jobs().has_value());
  EXPECT_EQ(result.value().args().value(),
            (Strings{"a", "b", "--jobs=2", "3"}));
}

TEST(cli, test_options_first_off_by_default) {
  const auto result = read<Recipe>({"build", "--jobs", "8", "x", "--", "-v"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().jobs(), 8);
  EXPECT_FALSE(result.value().verbose().has_value());
  EXPECT_EQ(result.value().args().value(), (Strings{"x", "-v"}));
}

struct One {
  rfl::Positional<std::string> file;
  std::optional<bool> verbose;
};

TEST(cli, test_options_first_without_a_list_positional) {
  // Like getopt's operands, the later tokens are positional arguments, so
  // there are too many of them.
  EXPECT_EQ(error_of(read<One, rfl::cli::OptionsFirst>({"f", "--verbose"})),
            "Too many positional arguments: expected at most 1, got 2.");

  const auto ok = read<One, rfl::cli::OptionsFirst>({"--verbose", "f"});
  ASSERT_TRUE(ok) << ok.error().what();
  EXPECT_EQ(ok.value().file(), "f");
  EXPECT_EQ(ok.value().verbose, true);
}

}  // namespace test_options_first
