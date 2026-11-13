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
  EXPECT_EQ(std::string(result.error().what()),
            "Too many positional arguments: expected at most 1, got 2.");
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
  ASSERT_FALSE(absent);
  EXPECT_EQ(std::string(absent.error().what()),
            "Field named 'tag' not found.");

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

TEST(cli, test_implicit_value_options) {
  const auto options =
      rfl::cli::internal::make_options<Opts, rfl::Processors<rfl::SnakeCaseToKebabCase>>();
  const auto help_reports = options.find_long("help-reports");
  EXPECT_EQ(help_reports.kind, rfl::cli::internal::OptionKind::implicit_value);
  EXPECT_EQ(help_reports.implicit_value, "all");
  const auto help = options.find_short("h");
  EXPECT_EQ(help.kind, rfl::cli::internal::OptionKind::implicit_value);
  EXPECT_EQ(help.implicit_value, "");
  EXPECT_EQ(options.find_long("force-export").kind,
            rfl::cli::internal::OptionKind::flag);
}

TEST(cli, test_implicit_value_without_options_table) {
  // parse_argv without the options cannot know that --help-reports and -h
  // have implicit values. resolve_args<T> applies them, and gives back the
  // token that -h took.
  using ProcessorsType = rfl::Processors<rfl::SnakeCaseToKebabCase>;
  const auto resolve =
      [](std::vector<const char*> _args) {
        _args.insert(_args.begin(), "program");
        return rfl::cli::parse_argv(static_cast<int>(_args.size()),
                                    _args.data())
            .and_then([](rfl::cli::ParsedArgs _parsed) {
              return rfl::cli::resolve_args<Opts, ProcessorsType>(
                  std::move(_parsed));
            });
      };

  const auto bare = resolve({"--help-reports", "-h", "in.rep", "--verbosity"});
  ASSERT_TRUE(bare) << bare.error().what();
  EXPECT_EQ(bare.value().at("help-reports"), "all");
  EXPECT_EQ(bare.value().at("help"), "");
  EXPECT_EQ(bare.value().at("verbosity"), "2");
  EXPECT_EQ(bare.value().at("input"), "in.rep");

  const auto with_values =
      resolve({"--help-reports=", "-h=x", "--verbosity=3"});
  ASSERT_TRUE(with_values) << with_values.error().what();
  EXPECT_EQ(with_values.value().at("help-reports"), "");
  EXPECT_EQ(with_values.value().at("help"), "x");
  EXPECT_EQ(with_values.value().at("verbosity"), "3");
  EXPECT_FALSE(with_values.value().contains("input"));

  // With the options, parse_argv has already applied them.
  const auto options = rfl::cli::internal::make_options<Opts, ProcessorsType>();
  const char* args[] = {"program", "--help-reports", "-h", "in.rep"};
  const auto parsed = rfl::cli::internal::parse_argv(4, args, options);
  ASSERT_TRUE(parsed) << parsed.error().what();
  const auto resolved =
      rfl::cli::resolve_args<Opts, ProcessorsType>(parsed.value());
  ASSERT_TRUE(resolved) << resolved.error().what();
  EXPECT_EQ(resolved.value().at("help-reports"), "all");
  EXPECT_EQ(resolved.value().at("help"), "");
  EXPECT_EQ(resolved.value().at("input"), "in.rep");
}

// An rfl::Rename renames the option if it is the outermost wrapper, as for
// every other wrapper.
struct RenamedOutermost {
  rfl::Rename<"report",
              rfl::ImplicitValue<std::optional<std::string>, "all">>
      help_reports;
  rfl::Rename<"out", rfl::Short<"o", rfl::ImplicitValue<
                                         std::optional<std::string>, "O">>>
      output;
  rfl::Rename<"in", rfl::Positional<std::optional<std::string>>> input;
};

TEST(cli, test_implicit_value_rename_outermost) {
  const auto bare =
      read<RenamedOutermost, rfl::NoExtraFields>({"--report", "x.rep"});
  ASSERT_TRUE(bare) << bare.error().what();
  EXPECT_EQ(bare.value().help_reports().value(), "all");
  EXPECT_EQ(bare.value().input().value(), "x.rep");

  const auto named = read<RenamedOutermost, rfl::NoExtraFields>(
      {"--report=r", "--out=o", "--in=i"});
  ASSERT_TRUE(named) << named.error().what();
  EXPECT_EQ(named.value().help_reports().value(), "r");
  EXPECT_EQ(named.value().output().value().value(), "o");
  EXPECT_EQ(named.value().input().value(), "i");

  const auto short_bare = read<RenamedOutermost>({"-o", "x.rep"});
  ASSERT_TRUE(short_bare) << short_bare.error().what();
  EXPECT_EQ(short_bare.value().output().value().value(), "O");
  EXPECT_EQ(short_bare.value().input().value(), "x.rep");

  const auto field_name =
      read<RenamedOutermost, rfl::NoExtraFields>({"--help-reports"});
  ASSERT_FALSE(field_name);
  EXPECT_EQ(std::string(field_name.error().what()),
            "Unknown option '--help-reports'.");
}

// Inside another wrapper, an rfl::Rename is ignored, as everywhere in
// reflect-cpp: the option keeps the field's name.
struct RenamedInside {
  rfl::ImplicitValue<rfl::Rename<"report", std::optional<std::string>>, "all">
      help_reports;
  rfl::Short<"o", rfl::Rename<"out", std::optional<std::string>>> output;
  rfl::Positional<rfl::Rename<"in", std::optional<std::string>>> input;
};

TEST(cli, test_rename_inside_other_wrappers_is_ignored) {
  const auto named = read<RenamedInside, rfl::NoExtraFields>(
      {"--help-reports", "--output", "o", "--input=i"});
  ASSERT_TRUE(named) << named.error().what();
  EXPECT_EQ(named.value().help_reports().value(), "all");
  EXPECT_EQ(named.value().output().value(), "o");
  EXPECT_EQ(named.value().input().value(), "i");

  const auto short_name = read<RenamedInside>({"-o", "o", "in.txt"});
  ASSERT_TRUE(short_name) << short_name.error().what();
  EXPECT_EQ(short_name.value().output().value(), "o");
  EXPECT_EQ(short_name.value().input().value(), "in.txt");

  const auto renamed = read<RenamedInside, rfl::NoExtraFields>({"--report"});
  ASSERT_FALSE(renamed);
  EXPECT_EQ(std::string(renamed.error().what()),
            "Unknown option '--report'.");
}

}  // namespace test_implicit_value
