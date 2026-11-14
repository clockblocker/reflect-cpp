#include <gtest/gtest.h>

#include <array>
#include <deque>
#include <map>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace test_repeated_options {

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

struct Database {
  std::optional<std::string> host;
  std::optional<Strings> replicas;
};

struct Opts {
  rfl::Short<"r", std::optional<Strings>> report;
  std::optional<std::vector<int>> ports;
  std::optional<std::set<std::string>> tags;
  std::optional<std::deque<std::string>> queue;
  std::optional<std::array<int, 3>> rgb;
  std::optional<std::pair<int, std::string>> range;
  rfl::Short<"o", std::optional<std::string>> output;
  std::optional<int> level;
  rfl::Short<"q", std::optional<bool>> quiet;
  std::optional<Database> database;
  std::optional<std::map<std::string, std::string>> labels;
  std::optional<std::map<std::string, Strings>> groups;
  rfl::ImplicitValue<std::optional<Strings>, "all"> help_reports;
  rfl::Positional<std::optional<std::string>> input;
};

TEST(cli, test_repeated_long_option_appends) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"--report", "a", "--report", "b", "--report", "c"},
           {"--report=a", "--report=b", "--report=c"},
           {"--report", "a,b", "--report", "c"},
           {"--report=a", "--report", "b,c"}}) {
    const auto result = read<Opts>(args);
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().report(), (Strings{"a", "b", "c"}));
  }
}

TEST(cli, test_repeated_short_and_long_merge_in_argv_order) {
  const auto result =
      read<Opts>({"-r", "a", "--report", "b", "-r=c", "-rd", "--report=e"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().report(), (Strings{"a", "b", "c", "d", "e"}));
}

TEST(cli, test_repeated_short_list_in_a_cluster) {
  // -q is a flag, so -qra is -q -r a.
  const auto result = read<Opts>({"-qra", "-r", "b", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().quiet(), true);
  EXPECT_EQ(result.value().report(), (Strings{"a", "b"}));
  EXPECT_EQ(result.value().input(), "in.txt");

  // The flag is still given only once.
  EXPECT_FALSE(read<Opts>({"-qra", "-qrb"}));
}

TEST(cli, test_repeated_other_list_types) {
  const auto result = read<Opts>(
      {"--ports", "80", "--ports=81,82", "--tags", "b", "--tags", "a,b",
       "--queue", "x", "--queue", "y", "--rgb", "1", "--rgb", "2,3",
       "--range", "5", "--range", "five"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& o = result.value();
  EXPECT_EQ(o.ports, (std::vector<int>{80, 81, 82}));
  EXPECT_EQ(o.tags, (std::set<std::string>{"a", "b"}));
  EXPECT_EQ(o.queue, (std::deque<std::string>{"x", "y"}));
  EXPECT_EQ(o.rgb, (std::array<int, 3>{1, 2, 3}));
  EXPECT_EQ(o.range, (std::pair<int, std::string>{5, "five"}));
}

TEST(cli, test_repeated_fixed_size_checked_on_all_items) {
  // Repeating is the same as joining with ',': the size is checked once.
  EXPECT_EQ(error_of(read<Opts>({"--rgb", "1", "--rgb", "2"})),
            error_of(read<Opts>({"--rgb=1,2"})));
  EXPECT_NE(error_of(read<Opts>({"--rgb=1,2"})), "(no error)");
}

TEST(cli, test_repeated_empty_occurrence_adds_nothing) {
  const auto some = read<Opts>({"--report=a", "--report=", "--report=b"});
  ASSERT_TRUE(some) << some.error().what();
  EXPECT_EQ(some.value().report(), (Strings{"a", "b"}));

  const auto none = read<Opts>({"--report=", "--report="});
  ASSERT_TRUE(none) << none.error().what();
  EXPECT_EQ(none.value().report(), Strings{});
}

TEST(cli, test_repeated_nested_list) {
  const auto result = read<Opts>({"--database.replicas", "a",
                                  "--database.replicas=b,c",
                                  "--database.host=db"});
  ASSERT_TRUE(result) << result.error().what();
  ASSERT_TRUE(result.value().database);
  EXPECT_EQ(result.value().database->replicas, (Strings{"a", "b", "c"}));
  EXPECT_EQ(result.value().database->host, "db");
}

TEST(cli, test_repeated_map_entry_list) {
  const auto result = read<Opts>(
      {"--groups.x", "a", "--groups.y=c", "--groups.x=b"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().groups,
            (std::map<std::string, Strings>{{"x", {"a", "b"}}, {"y", {"c"}}}));
}

TEST(cli, test_repeated_scalar_errors) {
  EXPECT_FALSE(read<Opts>({"--output", "a", "--output", "b"}));
  EXPECT_FALSE(read<Opts>({"--level=1", "--level=2"}));
  EXPECT_FALSE(read<Opts>({"-o", "a", "-o", "b"}));
  EXPECT_FALSE(read<Opts>({"-o", "a", "--output=b"}));
  EXPECT_FALSE(read<Opts>({"--quiet", "--quiet"}));
  EXPECT_FALSE(read<Opts>({"x", "--input=y"}));
}

TEST(cli, test_repeated_nested_scalar_and_map_key_error) {
  EXPECT_FALSE(read<Opts>({"--database.host=a", "--database.host", "b"}));
  EXPECT_FALSE(read<Opts>({"--labels.env=a", "--labels.env=b"}));

  const auto two_keys = read<Opts>({"--labels.env=a", "--labels.os=b"});
  ASSERT_TRUE(two_keys) << two_keys.error().what();
  EXPECT_EQ(two_keys.value().labels,
            (std::map<std::string, std::string>{{"env", "a"}, {"os", "b"}}));
}

TEST(cli, test_repeated_implicit_value_list) {
  // A bare occurrence adds the implicit value, split like any other value.
  const auto bare = read<Opts>({"--help-reports"});
  ASSERT_TRUE(bare) << bare.error().what();
  EXPECT_EQ(bare.value().help_reports(), (Strings{"all"}));

  const auto mixed = read<Opts>(
      {"--help-reports=cpu", "--help-reports", "--help-rep=gpu,mem"});
  ASSERT_TRUE(mixed) << mixed.error().what();
  EXPECT_EQ(mixed.value().help_reports(), (Strings{"cpu", "all", "gpu", "mem"}));

  // It still never takes the next token.
  const auto next = read<Opts>({"--help-reports", "in.txt"});
  ASSERT_TRUE(next) << next.error().what();
  EXPECT_EQ(next.value().help_reports(), (Strings{"all"}));
  EXPECT_EQ(next.value().input(), "in.txt");

  const auto absent = read<Opts>({});
  ASSERT_TRUE(absent) << absent.error().what();
  EXPECT_FALSE(absent.value().help_reports().has_value());
}

struct Wrapped {
  rfl::Description<"Files to include",
                   rfl::DefaultVal<std::vector<std::string>>>
      include;
  rfl::Rename<"L", std::optional<std::vector<std::string>>> lib_dirs;
  rfl::Validator<std::vector<int>, rfl::Size<rfl::Maximum<2>>> pair;
};

TEST(cli, test_repeated_through_wrappers) {
  const auto result = read<Wrapped>(
      {"--include", "a.h", "--include=b.h", "--L", "/x", "--L", "/y",
       "--pair", "1", "--pair", "2"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().include().value(), (Strings{"a.h", "b.h"}));
  EXPECT_EQ(result.value().lib_dirs(), (Strings{"/x", "/y"}));
  EXPECT_EQ(result.value().pair.value(), (std::vector<int>{1, 2}));

  // The validator sees every item.
  EXPECT_NE(error_of(read<Wrapped>({"--pair", "1", "--pair", "2,3"})),
            "(no error)");
}

}  // namespace test_repeated_options
