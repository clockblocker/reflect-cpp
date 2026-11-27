#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <variant>

namespace test_optional_variant_schema {

struct Circle {
  double radius;
};

struct Square {
  double side;
};

struct WithOptionalVariants {
  std::optional<std::variant<int, std::string>> std_var;
  std::optional<rfl::Variant<int, std::string>> rfl_var;
  rfl::Description<"A described optional variant",
                   std::optional<std::variant<bool, double>>>
      described;
  std::optional<std::optional<int>> nested_opt;
};

TEST(json, optional_std_variant_flattens_any_of) {
  const auto schema =
      rfl::json::to_schema<std::optional<std::variant<int, std::string>>>();
  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[{"type":"integer"},{"type":"string"},{"type":"null"}],"$defs":{}})";
  EXPECT_EQ(schema, expected);
}

TEST(json, optional_rfl_variant_flattens_any_of) {
  const auto schema =
      rfl::json::to_schema<std::optional<rfl::Variant<int, std::string>>>();
  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[{"type":"integer"},{"type":"string"},{"type":"null"}],"$defs":{}})";
  EXPECT_EQ(schema, expected);
}

TEST(json, struct_with_optional_variants_and_description) {
  const auto schema = rfl::json::to_schema<WithOptionalVariants>();
  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_optional_variant_schema__WithOptionalVariants","$defs":{"test_optional_variant_schema__WithOptionalVariants":{"type":"object","properties":{"std_var":{"anyOf":[{"type":"integer"},{"type":"string"},{"type":"null"}]},"rfl_var":{"anyOf":[{"type":"integer"},{"type":"string"},{"type":"null"}]},"described":{"description":"A described optional variant","anyOf":[{"type":"boolean"},{"type":"number"},{"type":"null"}]},"nested_opt":{"anyOf":[{"type":"integer"},{"type":"null"}]}},"required":[]}}})";
  EXPECT_EQ(schema, expected);
}

TEST(json, optional_one_of_preserves_one_of_branch) {
  const auto schema =
      rfl::json::to_schema<std::optional<std::variant<int, std::string>>,
                           rfl::VariantsAsOneOf>();
  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[{"oneOf":[{"type":"integer"},{"type":"string"}]},{"type":"null"}],"$defs":{}})";
  EXPECT_EQ(schema, expected);
}

TEST(json, optional_tagged_union_preserves_one_of_branch) {
  using Shape = rfl::TaggedUnion<"kind", Circle, Square>;
  const auto schema = rfl::json::to_schema<std::optional<Shape>>();
  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[{"oneOf":[{"$ref":"#/$defs/test_optional_variant_schema__Circle__tagged"},{"$ref":"#/$defs/test_optional_variant_schema__Square__tagged"}]},{"type":"null"}],"$defs":{"test_optional_variant_schema__Circle__tagged":{"type":"object","properties":{"kind":{"type":"string","enum":["Circle"]},"radius":{"type":"number"}},"required":["kind","radius"]},"test_optional_variant_schema__Square__tagged":{"type":"object","properties":{"kind":{"type":"string","enum":["Square"]},"side":{"type":"number"}},"required":["kind","side"]}}})";
  EXPECT_EQ(schema, expected);
}

}  // namespace test_optional_variant_schema
