#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

namespace test_implicit_value {

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "program");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

enum class Color { red, green, blue };

struct Opts {
  rfl::Positional<std::optional<std::string>> input;
  rfl::Short<"h", rfl::ImplicitValue<std::optional<std::string>>> help;
  rfl::ImplicitValue<std::optional<std::string>, "all"> help_reports;
  rfl::ImplicitValue<rfl::DefaultVal<int>, "2"> verbosity = 0;
  rfl::ImplicitValue<std::optional<Color>, "green"> color;
  std::optional<bool> force_export;
};

TEST(cli, test_implicit_value_bare) {
  const auto result =
      read<Opts>({"--help", "--help-reports", "--verbosity", "--color"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& o = result.value();
  EXPECT_EQ(o.help().value().value(), "");
  EXPECT_EQ(o.help_reports().value(), "all");
  EXPECT_EQ(o.verbosity().value(), 2);
  EXPECT_EQ(o.color().value(), Color::green);
  EXPECT_FALSE(o.input().has_value());
}

TEST(cli, test_implicit_value_with_equals) {
  const auto result = read<Opts>({"--help=tag", "--help-reports=memory",
                                  "--verbosity=5", "--color=blue"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& o = result.value();
  EXPECT_EQ(o.help().value().value(), "tag");
  EXPECT_EQ(o.help_reports().value(), "memory");
  EXPECT_EQ(o.verbosity().value(), 5);
  EXPECT_EQ(o.color().value(), Color::blue);
}

TEST(cli, test_implicit_value_empty_with_equals) {
  const auto result = read<Opts>({"--help-reports="});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().help_reports().value(), "");
}

TEST(cli, test_implicit_value_does_not_consume_next_token) {
  for (const char* option : {"--help", "--help-reports", "--verbosity", "-h"}) {
    const auto result = read<Opts>({option, "r.rep"});
    ASSERT_TRUE(result) << option << ": " << result.error().what();
    EXPECT_EQ(result.value().input().value(), "r.rep") << option;
  }
  const auto result = read<Opts>({"--help-reports", "--force-export"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().help_reports().value(), "all");
  EXPECT_EQ(result.value().force_export, true);
}

TEST(cli, test_implicit_value_next_token_is_too_many_positionals) {
  const auto result = read<Opts>({"in.rep", "--help-reports", "memory"});
  ASSERT_FALSE(result);
  EXPECT_NE(std::string(result.error().what()).find("Too many positional"),
            std::string::npos)
      << result.error().what();
}

TEST(cli, test_implicit_value_absent) {
  const auto result = read<Opts>({"r.rep"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& o = result.value();
  EXPECT_FALSE(o.help().value().has_value());
  EXPECT_FALSE(o.help_reports().has_value());
  EXPECT_EQ(o.verbosity().value(), 0);
  EXPECT_FALSE(o.color().has_value());
  EXPECT_EQ(rfl::json::write(o), R"({"input":"r.rep","verbosity":0})");
}

TEST(cli, test_implicit_value_short) {
  const auto bare = read<Opts>({"-h"});
  ASSERT_TRUE(bare) << bare.error().what();
  EXPECT_EQ(bare.value().help().value().value(), "");

  const auto with_value = read<Opts>({"-h=tag", "r.rep"});
  ASSERT_TRUE(with_value) << with_value.error().what();
  EXPECT_EQ(with_value.value().help().value().value(), "tag");
  EXPECT_EQ(with_value.value().input().value(), "r.rep");
}

struct Required {
  rfl::ImplicitValue<std::string, "x"> tag;
};

TEST(cli, test_implicit_value_required) {
  const auto absent = read<Required>({});
  EXPECT_FALSE(absent);

  const auto bare = read<Required>({"--tag"});
  ASSERT_TRUE(bare) << bare.error().what();
  EXPECT_EQ(bare.value().tag(), "x");
}

struct Database {
  rfl::ImplicitValue<std::optional<std::string>, "require"> ssl_mode;
};

// The same option, wrapped in other orders and in a nested struct.
struct Orders {
  rfl::ImplicitValue<rfl::Short<"a", std::optional<std::string>>, "A"> a;
  rfl::Description<"B", rfl::Short<"b", rfl::ImplicitValue<
                                            std::optional<std::string>, "B">>>
      b;
  rfl::Short<"c", rfl::Description<"C", rfl::ImplicitValue<
                                            std::optional<std::string>, "C">>>
      c;
  rfl::Rename<"dee", rfl::ImplicitValue<std::optional<std::string>, "D">> d;
  std::optional<rfl::ImplicitValue<std::string, "E">> e;
  rfl::DefaultVal<rfl::ImplicitValue<std::string, "F">> f = std::string("f");
  rfl::ImplicitValue<rfl::DefaultVal<std::string>, "G"> g = std::string("g");
  std::optional<Database> database;
  rfl::Positional<std::optional<std::string>> input;
};

TEST(cli, test_implicit_value_wrapper_orders) {
  const auto bare =
      read<Orders, rfl::NoExtraFields>({"-a", "-b", "-c", "--dee", "--e", "--f",
                                        "--g", "--database.ssl-mode", "r.rep"});
  ASSERT_TRUE(bare) << bare.error().what();
  const auto& o = bare.value();
  EXPECT_EQ(o.a().value().value(), "A");
  EXPECT_EQ(o.b().value().value().value(), "B");
  EXPECT_EQ(o.c().value().value().value(), "C");
  EXPECT_EQ(o.d().value().value(), "D");
  EXPECT_EQ(o.e.value().value(), "E");
  EXPECT_EQ(o.f().value(), "F");
  EXPECT_EQ(o.g().value(), "G");
  EXPECT_EQ(o.database->ssl_mode().value(), "require");
  EXPECT_EQ(o.input().value(), "r.rep");

  const auto with_values = read<Orders>(
      {"-a=1", "--b=2", "-c=3", "--dee=4", "--e=5", "--f=6", "--g=7",
       "--database.ssl-mode=off"});
  ASSERT_TRUE(with_values) << with_values.error().what();
  const auto& v = with_values.value();
  EXPECT_EQ(v.a().value().value(), "1");
  EXPECT_EQ(v.b().value().value().value(), "2");
  EXPECT_EQ(v.c().value().value().value(), "3");
  EXPECT_EQ(v.d().value().value(), "4");
  EXPECT_EQ(v.e.value().value(), "5");
  EXPECT_EQ(v.f().value(), "6");
  EXPECT_EQ(v.g().value(), "7");
  EXPECT_EQ(v.database->ssl_mode().value(), "off");

  const auto absent = read<Orders>({});
  ASSERT_TRUE(absent) << absent.error().what();
  EXPECT_FALSE(absent.value().a().value().has_value());
  EXPECT_FALSE(absent.value().e.has_value());
  EXPECT_EQ(absent.value().f().value(), "f");
  EXPECT_EQ(absent.value().g().value(), "g");
}

}  // namespace test_implicit_value
