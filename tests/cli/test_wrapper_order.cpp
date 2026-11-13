#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

// rfl::Short and rfl::Positional work through rfl::DefaultVal, std::optional,
// rfl::Description, rfl::Rename and rfl::Deprecated, in either order.
namespace test_wrapper_order {

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "program");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

struct Shorts {
  rfl::DefaultVal<rfl::Short<"o", std::string>> output = std::string("o");
  rfl::Short<"O", rfl::DefaultVal<std::string>> other_output =
      std::string("O");
  std::optional<rfl::Short<"f", std::string>> format;
  rfl::Short<"F", std::optional<std::string>> other_format;
  rfl::Description<"Log file", rfl::Short<"l", std::optional<std::string>>>
      log;
  rfl::Short<"L", rfl::Description<"Level", rfl::DefaultVal<int>>> level = 1;
  rfl::Rename<"outDir", rfl::Short<"d", std::optional<std::string>>> out_dir;
  rfl::Deprecated<"Use -j.", "Threads", rfl::Short<"t", rfl::DefaultVal<int>>>
      threads = 2;
  rfl::Short<"j", rfl::Deprecated<"", "Jobs", rfl::DefaultVal<int>>> jobs = 3;
  rfl::Description<"Title", rfl::DefaultVal<rfl::Short<"T", std::string>>>
      title = std::string("T");
};

TEST(cli, test_short_through_wrappers_given) {
  const auto result = read<Shorts, rfl::NoExtraFields>(
      {"-o", "a", "-O", "b", "-f", "c", "-F", "d", "-l", "e", "-L", "4", "-d",
       "f", "-t", "5", "-j", "6", "-T", "g"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& s = result.value();
  EXPECT_EQ(s.output().value(), "a");
  EXPECT_EQ(s.other_output().value(), "b");
  EXPECT_EQ(s.format.value().value(), "c");
  EXPECT_EQ(s.other_format().value(), "d");
  EXPECT_EQ(s.log().value().value(), "e");
  EXPECT_EQ(s.level().value().value(), 4);
  EXPECT_EQ(s.out_dir().value().value(), "f");
  EXPECT_EQ(s.threads().value().value(), 5);
  EXPECT_EQ(s.jobs().value().value(), 6);
  EXPECT_EQ(s.title().value().value(), "g");
}

TEST(cli, test_short_through_wrappers_absent) {
  const auto result = read<Shorts>({});
  ASSERT_TRUE(result) << result.error().what();
  const auto& s = result.value();
  EXPECT_EQ(s.output().value(), "o");
  EXPECT_EQ(s.other_output().value(), "O");
  EXPECT_FALSE(s.format.has_value());
  EXPECT_FALSE(s.other_format().has_value());
  EXPECT_FALSE(s.log().value().has_value());
  EXPECT_EQ(s.level().value().value(), 1);
  EXPECT_FALSE(s.out_dir().value().has_value());
  EXPECT_EQ(s.threads().value().value(), 2);
  EXPECT_EQ(s.jobs().value().value(), 3);
  EXPECT_EQ(s.title().value().value(), "T");
}

TEST(cli, test_short_through_wrappers_conflict_with_long) {
  const auto result = read<Shorts>({"-d", "a", "--outDir=b"});
  ASSERT_FALSE(result);
  EXPECT_EQ(std::string(result.error().what()),
            "Conflict: '-d' and '--outDir' both provided.");
}

// `second` and `fourth` are rfl::Positional outermost, so they take the first
// two arguments, as when only they were positional. The others follow in
// declaration order.
struct Positionals {
  rfl::DefaultVal<rfl::Positional<std::string>> first = std::string("1");
  rfl::Positional<rfl::DefaultVal<std::string>> second = std::string("2");
  rfl::Description<"Third", rfl::Positional<std::optional<std::string>>> third;
  rfl::Positional<rfl::Description<"Fourth", std::optional<std::string>>>
      fourth;
  std::optional<rfl::Positional<std::string>> fifth;
  rfl::Rename<"sixth", rfl::Positional<std::optional<std::string>>> sixth_;
  rfl::Deprecated<"", "Seventh", rfl::Positional<std::optional<std::string>>>
      seventh;
  std::optional<bool> verbose;
};

TEST(cli, test_positional_through_wrappers_given) {
  const auto result = read<Positionals>(
      {"a", "b", "c", "--verbose", "d", "e", "f", "g"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& p = result.value();
  EXPECT_EQ(p.second().value(), "a");
  EXPECT_EQ(p.fourth().value().value(), "b");
  EXPECT_EQ(p.first().value(), "c");
  EXPECT_EQ(p.third().value().value(), "d");
  EXPECT_EQ(p.fifth.value().value(), "e");
  EXPECT_EQ(p.sixth_().value().value(), "f");
  EXPECT_EQ(p.seventh().value().value(), "g");
  EXPECT_EQ(p.verbose, true);
}

TEST(cli, test_positional_through_wrappers_absent) {
  const auto result = read<Positionals>({"a"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& p = result.value();
  EXPECT_EQ(p.second().value(), "a");
  EXPECT_EQ(p.first().value(), "1");
  EXPECT_FALSE(p.third().value().has_value());
  EXPECT_FALSE(p.fifth.has_value());
  EXPECT_FALSE(p.seventh().value().has_value());

  const auto none = read<Positionals>({});
  ASSERT_TRUE(none) << none.error().what();
  EXPECT_EQ(none.value().first().value(), "1");
  EXPECT_EQ(none.value().second().value(), "2");
}

TEST(cli, test_positional_through_wrappers_by_name) {
  const auto result = read<Positionals>({"--sixth=f", "a"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().second().value(), "a");
  EXPECT_EQ(result.value().sixth_().value().value(), "f");
}

// A Positional found through other wrappers comes after the outermost ones,
// so a struct that worked when only those were positional reads the same.
struct WrappedPositionalFirst {
  std::optional<rfl::Positional<std::string>> out;
  rfl::Positional<std::string> in;
};

TEST(cli, test_outermost_positionals_come_first) {
  const auto one = read<WrappedPositionalFirst>({"x"});
  ASSERT_TRUE(one) << one.error().what();
  EXPECT_EQ(one.value().in(), "x");
  EXPECT_FALSE(one.value().out.has_value());

  const auto two = read<WrappedPositionalFirst>({"x", "y"});
  ASSERT_TRUE(two) << two.error().what();
  EXPECT_EQ(two.value().in(), "x");
  EXPECT_EQ(two.value().out.value().value(), "y");
}

struct ShortFlags {
  rfl::Short<"q", rfl::DefaultVal<bool>> quiet = false;
  rfl::Short<"v", std::optional<bool>> verbose;
  rfl::Description<"Dry run", rfl::Short<"n", bool>> dry_run;
  rfl::DefaultVal<rfl::Short<"f", bool>> force = false;
  rfl::Positional<std::optional<std::string>> input;
  rfl::Positional<std::optional<std::string>> output;
};

TEST(cli, test_short_flags_through_wrappers_do_not_consume_token) {
  for (const char* flag : {"-q", "-v", "-n", "-f"}) {
    std::vector<const char*> args = {flag, "in.rep"};
    if (std::string(flag) != "-n") {
      args.push_back("--dry-run=false");
    }
    const auto result = read<ShortFlags>(args);
    ASSERT_TRUE(result) << flag << ": " << result.error().what();
    EXPECT_EQ(result.value().input().value(), "in.rep") << flag;
  }

  const auto result =
      read<ShortFlags>({"-q", "-v", "in.rep", "-n", "out.csv", "-f"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& f = result.value();
  EXPECT_TRUE(f.quiet().value());
  EXPECT_EQ(f.verbose(), true);
  EXPECT_TRUE(f.dry_run().value());
  EXPECT_TRUE(f.force().value());
  EXPECT_EQ(f.input().value(), "in.rep");
  EXPECT_EQ(f.output().value(), "out.csv");
}

TEST(cli, test_short_flags_through_wrappers_keep_argv_order) {
  const auto result = read<ShortFlags>({"in.rep", "-q", "out.csv", "-n"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input().value(), "in.rep");
  EXPECT_EQ(result.value().output().value(), "out.csv");
}

TEST(cli, test_short_flags_through_wrappers_take_bool_literal) {
  const auto result =
      read<ShortFlags>({"-q", "true", "-v", "0", "-n", "false", "-f=1", "x"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& f = result.value();
  EXPECT_TRUE(f.quiet().value());
  EXPECT_EQ(f.verbose(), false);
  EXPECT_FALSE(f.dry_run().value());
  EXPECT_TRUE(f.force().value());
  EXPECT_EQ(f.input().value(), "x");
}

TEST(cli, test_short_flags_through_wrappers_explicit_value_is_not_reclaimed) {
  const auto result = read<ShortFlags>({"-q=in.rep", "-n"});
  ASSERT_FALSE(result);
  EXPECT_NE(std::string(result.error().what())
                .find("Could not cast 'in.rep' to boolean for key 'quiet'."),
            std::string::npos)
      << result.error().what();
}

TEST(cli, test_short_flags_through_wrappers_without_options_table) {
  // parse_argv without the options lets -q take "in.rep". resolve_args<T>
  // gives it back to the positional arguments, as for Short<"v", bool>.
  const char* args[] = {"program", "-q", "in.rep", "-n"};
  const auto parsed = rfl::cli::parse_argv(4, const_cast<char**>(args));
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed.value().short_args.at("q"), "in.rep");
  const auto resolved = rfl::cli::resolve_args<
      ShortFlags, rfl::Processors<rfl::SnakeCaseToKebabCase>>(parsed.value());
  ASSERT_TRUE(resolved) << resolved.error().what();
  EXPECT_EQ(resolved.value().at("quiet"), "");
  EXPECT_EQ(resolved.value().at("input"), "in.rep");
}

struct ShortValues {
  rfl::Short<"o", std::string> output;
  rfl::Short<"p", rfl::DefaultVal<int>> port = 80;
};

TEST(cli, test_short_missing_value) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"-o"}, {"-o", "-p", "1"}, {"-o", "--", "x"}}) {
    const auto result = read<ShortValues>(args);
    ASSERT_FALSE(result);
    EXPECT_EQ(std::string(result.error().what()),
              "Missing value for option '-o'.");
  }
  const auto empty = read<ShortValues>({"-o=", "-p", "-1"});
  ASSERT_TRUE(empty) << empty.error().what();
  EXPECT_EQ(empty.value().output(), "");
  EXPECT_EQ(empty.value().port().value(), -1);
}

// Of several rfl::Short and rfl::Positional on one field, the outermost
// counts, as when only the outermost wrapper was looked at.
struct ShortInShort {
  rfl::Short<"a", rfl::Short<"b", std::string>> value;
};

struct PositionalInShort {
  rfl::Short<"o", rfl::Description<"Out", rfl::Positional<std::string>>> out;
};

struct PositionalInPositional {
  rfl::Positional<rfl::Positional<std::string>> input;
};

TEST(cli, test_outermost_short_or_positional_counts) {
  const auto a = read<ShortInShort>({"-a", "x"});
  ASSERT_TRUE(a) << a.error().what();
  EXPECT_EQ(a.value().value().value(), "x");
  EXPECT_EQ(std::string(read<ShortInShort>({"-b", "x"}).error().what()),
            "Unknown short argument: -b");

  const auto b = read<PositionalInShort>({"-o", "x"});
  ASSERT_TRUE(b) << b.error().what();
  EXPECT_EQ(b.value().out().value().value(), "x");
  EXPECT_EQ(std::string(read<PositionalInShort>({"x"}).error().what()),
            "Too many positional arguments: expected at most 0, got 1.");

  const auto c = read<PositionalInPositional>({"x"});
  ASSERT_TRUE(c) << c.error().what();
  EXPECT_EQ(c.value().input().value(), "x");
}

// A Short found through other wrappers counts only if no other field has its
// name. An outermost one always counts.
struct WrappedAndDirectShort {
  rfl::Description<"Name", rfl::Short<"o", std::string>> name;
  rfl::Short<"o", int> count;
};

struct TwoWrappedShorts {
  rfl::Description<"A", rfl::Short<"o", std::string>> a;
  rfl::Description<"B", rfl::Short<"o", std::string>> b;
};

TEST(cli, test_wrapped_short_name_taken_by_another_field) {
  const auto direct = read<WrappedAndDirectShort>({"-o", "5", "--name=x"});
  ASSERT_TRUE(direct) << direct.error().what();
  EXPECT_EQ(direct.value().count(), 5);
  EXPECT_EQ(direct.value().name().value(), "x");

  const auto both = read<TwoWrappedShorts>({"--a=x", "--b=y"});
  ASSERT_TRUE(both) << both.error().what();
  EXPECT_EQ(both.value().a().value(), "x");
  EXPECT_EQ(both.value().b().value(), "y");
  EXPECT_EQ(std::string(read<TwoWrappedShorts>({"-o", "x"}).error().what()),
            "Unknown short argument: -o");
}

// Two fields with the same outermost Short compile, but every read fails.
struct DuplicateShort {
  rfl::Short<"o", std::optional<std::string>> a;
  rfl::Short<"o", std::optional<std::string>> b;
};

TEST(cli, test_duplicate_short_name_fails_every_read) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {}, {"-o", "x"}, {"--a=x", "--b=y"}}) {
    const auto result = read<DuplicateShort>(args);
    ASSERT_FALSE(result);
    EXPECT_EQ(std::string(result.error().what()),
              "Duplicate short name '-o' in struct definition.");
  }

  const char* argv[] = {"program", "--a=x"};
  const auto resolved =
      rfl::cli::parse_argv(2, argv)
          .and_then(rfl::cli::resolve_args<
                    DuplicateShort, rfl::Processors<rfl::SnakeCaseToKebabCase>>);
  ASSERT_FALSE(resolved);
  EXPECT_EQ(std::string(resolved.error().what()),
            "Duplicate short name '-o' in struct definition.");
}

}  // namespace test_wrapper_order
