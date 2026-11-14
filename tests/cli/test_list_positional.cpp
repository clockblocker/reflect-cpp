#include <gtest/gtest.h>

#include <array>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace test_list_positional {

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "program");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

using Strings = std::vector<std::string>;

struct Cmd {
  rfl::Short<"v", std::optional<bool>> verbose;
  rfl::Short<"l", std::optional<int>> level;
  rfl::Positional<std::string> command;
  rfl::Positional<Strings> files;
};

TEST(cli, test_list_positional_takes_remaining_arguments) {
  // Each argument is split at the commas.
  for (const auto& [args, expected] :
       std::vector<std::pair<std::vector<const char*>, Strings>>{
           {{"run", "a,b"}, {"a", "b"}},
           {{"run", "a", "b"}, {"a", "b"}},
           {{"run", "a,b", "c"}, {"a", "b", "c"}},
           {{"run", "a", "b,c", "d"}, {"a", "b", "c", "d"}}}) {
    const auto result = read<Cmd>(args);
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().command(), "run");
    EXPECT_EQ(result.value().files(), expected);
  }
}

TEST(cli, test_list_positional_mixed_with_options) {
  // Options can come anywhere, also between the list's arguments.
  const auto result = read<Cmd>({"run", "a", "-v", "b", "--level", "3", "c"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().verbose(), true);
  EXPECT_EQ(result.value().level(), 3);
  EXPECT_EQ(result.value().files(), (Strings{"a", "b", "c"}));

  const auto cluster = read<Cmd>({"run", "a", "-vl3", "b"});
  ASSERT_TRUE(cluster) << cluster.error().what();
  EXPECT_EQ(cluster.value().verbose(), true);
  EXPECT_EQ(cluster.value().level(), 3);
  EXPECT_EQ(cluster.value().files(), (Strings{"a", "b"}));
}

TEST(cli, test_list_positional_after_double_dash) {
  const auto result = read<Cmd>({"-v", "--", "run", "-x", "--y=1", "--"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().command(), "run");
  EXPECT_EQ(result.value().files(), (Strings{"-x", "--y=1", "--"}));
}

struct Numbers {
  rfl::Positional<std::vector<int>> n;
};

TEST(cli, test_list_positional_negative_numbers) {
  const auto result = read<Numbers>({"1", "-2", "3,-4"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().n(), (std::vector<int>{1, -2, 3, -4}));
}

TEST(cli, test_list_positional_required_absent) {
  // A plain list needs at least one argument, like argparse's nargs='+'.
  EXPECT_FALSE(read<Cmd>({"run"}));

  // An empty argument is an empty list, as `--files=` is.
  const auto empty = read<Cmd>({"run", ""});
  ASSERT_TRUE(empty) << empty.error().what();
  EXPECT_EQ(empty.value().files(), Strings{});
}

struct Defaults {
  rfl::Positional<std::optional<std::string>> first;
  rfl::DefaultVal<rfl::Positional<std::vector<int>>> rest =
      std::vector<int>{7, 8};
};

struct Optional {
  rfl::Positional<std::optional<std::set<int>>> ids;
};

TEST(cli, test_list_positional_default_and_optional) {
  const auto none = read<Defaults>({});
  ASSERT_TRUE(none) << none.error().what();
  EXPECT_FALSE(none.value().first().has_value());
  EXPECT_EQ(none.value().rest.value()(), (std::vector<int>{7, 8}));

  const auto one = read<Defaults>({"x"});
  ASSERT_TRUE(one) << one.error().what();
  EXPECT_EQ(one.value().first(), "x");
  EXPECT_EQ(one.value().rest.value()(), (std::vector<int>{7, 8}));

  const auto three = read<Defaults>({"x", "1", "2,3"});
  ASSERT_TRUE(three) << three.error().what();
  EXPECT_EQ(three.value().rest.value()(), (std::vector<int>{1, 2, 3}));

  const auto absent = read<Optional>({});
  ASSERT_TRUE(absent) << absent.error().what();
  EXPECT_FALSE(absent.value().ids().has_value());

  const auto ids = read<Optional>({"3", "1,3", "2"});
  ASSERT_TRUE(ids) << ids.error().what();
  EXPECT_EQ(ids.value().ids(), (std::set<int>{1, 2, 3}));
}

TEST(cli, test_list_positional_long_form) {
  const auto equals = read<Cmd>({"run", "--files=a,b"});
  ASSERT_TRUE(equals) << equals.error().what();
  EXPECT_EQ(equals.value().files(), (Strings{"a", "b"}));

  const auto repeated =
      read<Cmd>({"--files", "a", "run", "--files=b,c", "--files", "d"});
  ASSERT_TRUE(repeated) << repeated.error().what();
  EXPECT_EQ(repeated.value().command(), "run");
  EXPECT_EQ(repeated.value().files(), (Strings{"a", "b", "c", "d"}));

  // Bare arguments and the long form are not mixed.
  EXPECT_FALSE(read<Cmd>({"run", "--files=a", "b"}));
}

struct Two {
  rfl::Positional<std::string> input;
  rfl::Positional<std::string> output;
};

TEST(cli, test_list_positional_scalar_last_overflows) {
  EXPECT_FALSE(read<Two>({"a", "b", "c"}));
}

struct PairFirst {
  rfl::Positional<std::pair<int, int>> range;
  rfl::Positional<std::array<std::string, 2>> names;
  rfl::Positional<std::optional<std::tuple<int, bool>>> extra;
  rfl::Positional<std::string> last;
};

TEST(cli, test_list_positional_fixed_size_not_last) {
  // Pairs, tuples and std::array have a fixed size, so they can come
  // before other positional fields. Each takes one argument, split at the
  // commas.
  const auto result = read<PairFirst>({"1,2", "a,b", "3,true", "x"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().range(), (std::pair<int, int>{1, 2}));
  EXPECT_EQ(result.value().names(), (std::array<std::string, 2>{"a", "b"}));
  EXPECT_EQ(result.value().extra(), (std::tuple<int, bool>{3, true}));
  EXPECT_EQ(result.value().last(), "x");

  EXPECT_FALSE(read<PairFirst>({"1", "2", "a,b", "3,true", "x"}));
}

struct ListFirst {
  rfl::Positional<Strings> files;
  rfl::Positional<std::string> dest;
};

struct ListInTheMiddle {
  rfl::Positional<std::string> a;
  rfl::Positional<std::optional<Strings>> b;
  std::optional<bool> flag;
  rfl::Positional<std::optional<std::string>> c;
};

struct SetFirst {
  rfl::Positional<std::set<int>> ids;
  rfl::Positional<std::string> dest;
};

TEST(cli, test_list_positional_not_last_takes_one_argument) {
  // A list that is not the last positional field takes one argument, split
  // at the commas.
  const auto first = read<ListFirst>({"a,b", "d"});
  ASSERT_TRUE(first) << first.error().what();
  EXPECT_EQ(first.value().files(), (Strings{"a", "b"}));
  EXPECT_EQ(first.value().dest(), "d");
  EXPECT_FALSE(read<ListFirst>({"a", "b", "d"}));
  EXPECT_FALSE(read<ListFirst>({"a"}));

  const auto middle = read<ListInTheMiddle>({"x", "1,2", "--flag", "y"});
  ASSERT_TRUE(middle) << middle.error().what();
  EXPECT_EQ(middle.value().a(), "x");
  EXPECT_EQ(middle.value().b(), (Strings{"1", "2"}));
  EXPECT_EQ(middle.value().c(), "y");
  EXPECT_FALSE(read<ListInTheMiddle>({"x", "1", "2", "y"}));

  const auto set = read<SetFirst>({"3,1", "d"});
  ASSERT_TRUE(set) << set.error().what();
  EXPECT_EQ(set.value().ids(), (std::set<int>{1, 3}));
  EXPECT_EQ(set.value().dest(), "d");
}

struct ListLastAmongOptions {
  rfl::Positional<std::string> a;
  rfl::Positional<Strings> b;
  std::optional<Strings> not_positional;
};

TEST(cli, test_list_positional_last_among_options) {
  // Only the last positional field takes the remaining arguments; a list
  // option that is not positional does not.
  const auto result = read<ListLastAmongOptions>(
      {"x", "p", "--not-positional=n,m", "q,r"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().a(), "x");
  EXPECT_EQ(result.value().b(), (Strings{"p", "q", "r"}));
  EXPECT_EQ(result.value().not_positional, (Strings{"n", "m"}));
}

// A Positional found through other wrappers comes after the outermost ones,
// so a wrapped list declared first is the last positional field.
struct WrappedListFirst {
  std::optional<rfl::Positional<Strings>> files;
  rfl::Positional<std::string> command;
};

TEST(cli, test_list_positional_wrapped_list_comes_last) {
  const auto result = read<WrappedListFirst>({"run", "a", "b,c"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().command(), "run");
  EXPECT_EQ(result.value().files.value().value(), (Strings{"a", "b", "c"}));

  const auto alone = read<WrappedListFirst>({"run"});
  ASSERT_TRUE(alone) << alone.error().what();
  EXPECT_EQ(alone.value().command(), "run");
  EXPECT_FALSE(alone.value().files.has_value());
}

}  // namespace test_list_positional
