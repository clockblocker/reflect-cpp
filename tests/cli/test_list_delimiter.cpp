#include <gtest/gtest.h>

#include <array>
#include <map>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <rfl/json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace test_list_delimiter {

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
  std::optional<Strings> hosts;
};

struct Opts {
  rfl::Short<"t", std::optional<Strings>> tags;
  std::optional<std::vector<int>> ports;
  std::optional<std::array<int, 2>> size;
  std::optional<std::string> name;
  std::optional<Database> database;
  std::optional<std::map<std::string, Strings>> groups;
  rfl::ImplicitValue<std::optional<Strings>, "cpu,gpu"> reports;
  rfl::Positional<std::optional<Strings>> files;
};

using Semicolon = rfl::cli::ListDelimiter<";">;
using NoSplit = rfl::cli::ListDelimiter<"">;

TEST(cli, test_list_delimiter_semicolon) {
  const auto result = read<Opts, Semicolon>(
      {"--tags=a;b,c", "-t", "d", "--ports", "80;81", "--size=3;4",
       "--name=x;y", "f;g", "h,i"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& o = result.value();
  EXPECT_EQ(o.tags(), (Strings{"a", "b,c", "d"}));
  EXPECT_EQ(o.ports, (std::vector<int>{80, 81}));
  EXPECT_EQ(o.size, (std::array<int, 2>{3, 4}));
  // Only lists are split.
  EXPECT_EQ(o.name, "x;y");
  EXPECT_EQ(o.files(), (Strings{"f", "g", "h,i"}));
}

TEST(cli, test_list_delimiter_applies_to_every_list) {
  const auto result = read<Opts, Semicolon>(
      {"--database.hosts=a;b", "--groups.x", "c;d", "--reports"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& o = result.value();
  ASSERT_TRUE(o.database);
  EXPECT_EQ(o.database->hosts, (Strings{"a", "b"}));
  EXPECT_EQ(o.groups, (std::map<std::string, Strings>{{"x", {"c", "d"}}}));
  // The implicit value is split like any other value.
  EXPECT_EQ(o.reports(), (Strings{"cpu,gpu"}));
}

TEST(cli, test_list_delimiter_colon_and_space) {
  const auto colon =
      read<Opts, rfl::cli::ListDelimiter<":">>({"--tags", "/usr/bin:/bin"});
  ASSERT_TRUE(colon) << colon.error().what();
  EXPECT_EQ(colon.value().tags(), (Strings{"/usr/bin", "/bin"}));

  // As with ',', empty items are skipped.
  const auto space = read<Opts, rfl::cli::ListDelimiter<" ">>(
      {"--tags", "a b  c", "--tags= d", "x,y z"});
  ASSERT_TRUE(space) << space.error().what();
  EXPECT_EQ(space.value().tags(), (Strings{"a", "b", "c", "d"}));
  EXPECT_EQ(space.value().files(), (Strings{"x,y", "z"}));
}

TEST(cli, test_list_delimiter_comma_is_the_default) {
  const auto explicit_comma =
      read<Opts, rfl::cli::ListDelimiter<",">>({"--tags=a,b;c", "d,e"});
  const auto by_default = read<Opts>({"--tags=a,b;c", "d,e"});
  ASSERT_TRUE(explicit_comma) << explicit_comma.error().what();
  ASSERT_TRUE(by_default) << by_default.error().what();
  EXPECT_EQ(by_default.value().tags(), (Strings{"a", "b;c"}));
  EXPECT_EQ(by_default.value().files(), (Strings{"d", "e"}));
  EXPECT_EQ(rfl::json::write(explicit_comma.value()),
            rfl::json::write(by_default.value()));
}

TEST(cli, test_list_delimiter_empty_disables_splitting) {
  const auto result = read<Opts, NoSplit>(
      {"--tags=a,b", "-t", "c;d", "--tags=", "--reports", "x,y", "",
       "z w"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& o = result.value();
  // Each occurrence and each argument is exactly one item, even if empty.
  EXPECT_EQ(o.tags(), (Strings{"a,b", "c;d", ""}));
  EXPECT_EQ(o.reports(), (Strings{"cpu,gpu"}));
  EXPECT_EQ(o.files(), (Strings{"x,y", "", "z w"}));

  const auto ints = read<Opts, NoSplit>({"--ports", "80", "--ports", "81"});
  ASSERT_TRUE(ints) << ints.error().what();
  EXPECT_EQ(ints.value().ports, (std::vector<int>{80, 81}));
  EXPECT_NE(error_of(read<Opts, NoSplit>({"--ports=80,81"})), "(no error)");
}

TEST(cli, test_list_delimiter_with_other_settings) {
  for (const auto& result :
       {read<Opts, rfl::NoExtraFields, Semicolon>({"--tag=a;b", "c;d"}),
        read<Opts, Semicolon, rfl::NoExtraFields>({"--tag=a;b", "c;d"})}) {
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().tags(), (Strings{"a", "b"}));
    EXPECT_EQ(result.value().files(), (Strings{"c", "d"}));
  }
  EXPECT_EQ(error_of(read<Opts, Semicolon, rfl::NoExtraFields>({"--x=1"})),
            "Unknown option '--x'.");
}

// The trait that rfl::cli::read uses.
static_assert(rfl::cli::internal::list_delimiter_v<rfl::Processors<>> == ',');
static_assert(rfl::cli::internal::list_delimiter_v<
                  rfl::Processors<rfl::NoExtraFields, Semicolon>> == ';');
static_assert(!rfl::cli::internal::list_delimiter_v<
                  rfl::Processors<NoSplit, rfl::NoExtraFields>>
                   .has_value());

struct Record {
  std::string name;
  std::vector<std::string> tags;
  std::optional<std::vector<int>> ports;
};

TEST(cli, test_list_delimiter_ignored_by_json) {
  const auto record = Record{.name = "a;b", .tags = {"x,y", "z"},
                             .ports = std::vector<int>{1, 2}};

  const auto json = rfl::json::write(record);
  EXPECT_EQ(rfl::json::write<Semicolon>(record), json);
  EXPECT_EQ(rfl::json::write<NoSplit>(record), json);

  const auto back = rfl::json::read<Record, Semicolon>(json);
  ASSERT_TRUE(back) << back.error().what();
  EXPECT_EQ(rfl::json::write(back.value()), json);

  EXPECT_EQ((rfl::json::to_schema<Record, Semicolon>()),
            rfl::json::to_schema<Record>());
  EXPECT_EQ((rfl::json::to_schema<Record, NoSplit>()),
            rfl::json::to_schema<Record>());
}

}  // namespace test_list_delimiter
