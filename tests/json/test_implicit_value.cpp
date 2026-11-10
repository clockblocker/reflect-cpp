#include <gtest/gtest.h>

#include <exception>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <type_traits>

#include "write_and_read.hpp"

// rfl::ImplicitValue only changes how a CLI option is given, so for JSON a
// field wrapped in it must behave exactly like the field without it.
namespace test_implicit_value {

struct Plain {
  std::optional<std::string> help;
  rfl::Short<"l", rfl::DefaultVal<int>> level = 3;
  rfl::Description<"Report to print", rfl::DefaultVal<std::string>> report =
      std::string("summary");
  std::string tag;
};

struct Implicit {
  rfl::ImplicitValue<std::optional<std::string>> help;
  rfl::Short<"l", rfl::ImplicitValue<rfl::DefaultVal<int>, "1">> level = 3;
  rfl::Description<"Report to print",
                   rfl::DefaultVal<rfl::ImplicitValue<std::string, "all">>>
      report = std::string("summary");
  rfl::ImplicitValue<std::string, "x"> tag;
};

TEST(json, test_implicit_value) {
  write_and_read(Implicit{.tag = std::string("t")},
                 R"({"level":3,"report":"summary","tag":"t"})");
  write_and_read(
      Implicit{.help = std::string("h"),
               .level = 5,
               .report = std::string("all"),
               .tag = std::string("t")},
      R"({"help":"h","level":5,"report":"all","tag":"t"})");

  const auto defaults = rfl::json::read<Implicit>(R"({"tag":"t"})");
  ASSERT_TRUE(defaults) << defaults.error().what();
  EXPECT_FALSE(defaults.value().help().has_value());
  EXPECT_EQ(defaults.value().level().value().value(), 3);
  EXPECT_EQ(defaults.value().report().value().value(), "summary");

  const auto missing = rfl::json::read<Implicit>(R"({})");
  ASSERT_FALSE(missing);
  EXPECT_EQ(std::string(missing.error().what()),
            "Field named 'tag' not found.");

  auto schema = rfl::json::to_schema<Implicit>();
  for (auto pos = schema.find("Implicit"); pos != std::string::npos;
       pos = schema.find("Implicit", pos)) {
    schema.replace(pos, 8, "Plain");
  }
  EXPECT_EQ(schema, rfl::json::to_schema<Plain>());
  EXPECT_NE(schema.find(R"("default":"summary")"), std::string::npos)
      << schema;
}

using Positive = rfl::Validator<int, rfl::Minimum<1>>;

static_assert(!std::is_nothrow_constructible_v<
                  rfl::ImplicitValue<rfl::DefaultVal<Positive>, "5">, int>,
              "Converting to a Validator can throw.");
static_assert(!std::is_nothrow_constructible_v<
                  rfl::ImplicitValue<Positive, "5">, int>);
static_assert(
    std::is_nothrow_constructible_v<rfl::ImplicitValue<int, "1">, short>);
static_assert(
    std::is_nothrow_move_constructible_v<rfl::ImplicitValue<std::string>>);

struct InvalidInitializer {
  rfl::ImplicitValue<rfl::DefaultVal<Positive>, "5"> retries = 0;
};

TEST(json, test_implicit_value_throwing_initializer) {
  EXPECT_THROW((void)InvalidInitializer{}, std::exception);
  const auto res = rfl::json::read<InvalidInitializer>(R"({"retries":3})");
  ASSERT_FALSE(res);
  EXPECT_NE(std::string(res.error().what())
                .find("Value expected to be greater than or equal to 1, but "
                      "got 0."),
            std::string::npos)
      << res.error().what();
}

// As for every other wrapper, an rfl::Rename renames the field if it is the
// outermost wrapper.
struct RenamedOutermost {
  rfl::Rename<"outer", rfl::ImplicitValue<int, "1">> field_one;
  rfl::Rename<"shortRenamed",
              rfl::Short<"s", rfl::ImplicitValue<std::optional<std::string>,
                                                 "x">>>
      field_two;
  rfl::Rename<"withDefault",
              rfl::ImplicitValue<rfl::DefaultVal<int>, "2">>
      field_three = 5;
};

TEST(json, test_implicit_value_rename_outermost) {
  write_and_read(
      RenamedOutermost{.field_one = 2,
                       .field_two = std::optional<std::string>("v"),
                       .field_three = 7},
      R"({"outer":2,"shortRenamed":"v","withDefault":7})");
  EXPECT_EQ(rfl::json::write<rfl::SnakeCaseToCamelCase>(
                RenamedOutermost{.field_one = 2, .field_three = 7}),
            R"({"outer":2,"withDefault":7})");
  const auto missing = rfl::json::read<RenamedOutermost>(R"({"outer":1})");
  ASSERT_TRUE(missing) << missing.error().what();
  EXPECT_EQ(missing.value().field_three().value().value(), 5);
  const auto required = rfl::json::read<RenamedOutermost>(R"({})");
  ASSERT_FALSE(required);
  EXPECT_EQ(std::string(required.error().what()),
            "Field named 'outer' not found.");
  EXPECT_EQ(
      rfl::json::to_schema<RenamedOutermost>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_implicit_value__RenamedOutermost","$defs":{"test_implicit_value__RenamedOutermost":{"type":"object","properties":{"outer":{"type":"integer"},"shortRenamed":{"anyOf":[{"type":"string"},{"type":"null"}]},"withDefault":{"type":"integer","default":5}},"required":["outer"]}}})");
}

// Inside another wrapper, an rfl::Rename is ignored, as everywhere in
// reflect-cpp: the field keeps its own name.
struct RenamedInside {
  rfl::ImplicitValue<rfl::Rename<"renamed", std::optional<std::string>>, "x">
      field_one;
  rfl::Short<"s", rfl::Rename<"short_renamed", std::string>> field_two;
  rfl::Positional<rfl::Rename<"positional_renamed", int>> field_three;
};

TEST(json, test_rename_inside_other_wrappers_is_ignored) {
  write_and_read(
      RenamedInside{
          .field_one = rfl::Rename<"renamed", std::optional<std::string>>(
              std::string("v")),
          .field_two = rfl::Rename<"short_renamed", std::string>("w"),
          .field_three = rfl::Rename<"positional_renamed", int>(2)},
      R"({"field_one":"v","field_two":"w","field_three":2})");
  EXPECT_EQ(rfl::json::write<rfl::SnakeCaseToCamelCase>(RenamedInside{
                .field_one =
                    rfl::Rename<"renamed", std::optional<std::string>>(
                        std::string("v")),
                .field_two = rfl::Rename<"short_renamed", std::string>("w"),
                .field_three = rfl::Rename<"positional_renamed", int>(2)}),
            R"({"fieldOne":"v","fieldTwo":"w","fieldThree":2})");
}

}  // namespace test_implicit_value
