#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <variant>

namespace test_json_schema_one_of {

struct Circle {
  double radius;
};

struct Square {
  double width;
};

struct Rectangle {
  double width;
  double height;
};

TEST(json, test_json_schema_one_of_tagged_union) {
  // The tags make the alternatives mutually exclusive, so oneOf is safe.
  const auto json_schema =
      rfl::json::to_schema<rfl::TaggedUnion<"shape", Circle, Square>>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","oneOf":[{"$ref":"#/$defs/test_json_schema_one_of__Circle__tagged"},{"$ref":"#/$defs/test_json_schema_one_of__Square__tagged"}],"$defs":{"test_json_schema_one_of__Circle__tagged":{"type":"object","properties":{"shape":{"type":"string","enum":["Circle"]},"radius":{"type":"number"}},"required":["shape","radius"]},"test_json_schema_one_of__Square__tagged":{"type":"object","properties":{"shape":{"type":"string","enum":["Square"]},"width":{"type":"number"}},"required":["shape","width"]}}})";

  EXPECT_EQ(json_schema, expected);
}

TEST(json, test_json_schema_one_of_untagged_variant) {
  // 1 is both an integer and a number, so oneOf would reject it.
  const auto json_schema = rfl::json::to_schema<rfl::Variant<int, double>>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[{"type":"integer"},{"type":"number"}],"$defs":{}})";

  EXPECT_EQ(json_schema, expected);
}

TEST(json, test_json_schema_one_of_overlapping_structs) {
  // {"width":1,"height":2} matches both, because additional properties are
  // allowed, so oneOf would reject it.
  const auto json_schema =
      rfl::json::to_schema<rfl::Variant<Square, Rectangle>>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[{"$ref":"#/$defs/test_json_schema_one_of__Square"},{"$ref":"#/$defs/test_json_schema_one_of__Rectangle"}],"$defs":{"test_json_schema_one_of__Rectangle":{"type":"object","properties":{"width":{"type":"number"},"height":{"type":"number"}},"required":["width","height"]},"test_json_schema_one_of__Square":{"type":"object","properties":{"width":{"type":"number"}},"required":["width"]}}})";

  EXPECT_EQ(json_schema, expected);
}

TEST(json, test_json_schema_one_of_optional) {
  const auto json_schema = rfl::json::to_schema<std::optional<int>>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[{"type":"integer"},{"type":"null"}],"$defs":{}})";

  EXPECT_EQ(json_schema, expected);
}

TEST(json, test_json_schema_one_of_processor_rfl_variant) {
  const auto json_schema = rfl::json::to_schema<rfl::Variant<int, std::string>,
                                                rfl::VariantsAsOneOf>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","oneOf":[{"type":"integer"},{"type":"string"}],"$defs":{}})";

  EXPECT_EQ(json_schema, expected);
}

TEST(json, test_json_schema_one_of_processor_std_variant) {
  const auto json_schema = rfl::json::to_schema<std::variant<Circle, Square>,
                                                rfl::VariantsAsOneOf>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","oneOf":[{"$ref":"#/$defs/test_json_schema_one_of__Circle"},{"$ref":"#/$defs/test_json_schema_one_of__Square"}],"$defs":{"test_json_schema_one_of__Circle":{"type":"object","properties":{"radius":{"type":"number"}},"required":["radius"]},"test_json_schema_one_of__Square":{"type":"object","properties":{"width":{"type":"number"}},"required":["width"]}}})";

  EXPECT_EQ(json_schema, expected);
}

TEST(json, test_json_schema_one_of_processor_field_variant) {
  using FieldVariant =
      rfl::Variant<rfl::Field<"circle", Circle>, rfl::Field<"square", Square>>;

  const auto json_schema =
      rfl::json::to_schema<FieldVariant, rfl::VariantsAsOneOf>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","oneOf":[{"type":"object","properties":{"circle":{"$ref":"#/$defs/test_json_schema_one_of__Circle"}},"required":["circle"]},{"type":"object","properties":{"square":{"$ref":"#/$defs/test_json_schema_one_of__Square"}},"required":["square"]}],"$defs":{"test_json_schema_one_of__Circle":{"type":"object","properties":{"radius":{"type":"number"}},"required":["radius"]},"test_json_schema_one_of__Square":{"type":"object","properties":{"width":{"type":"number"}},"required":["width"]}}})";

  EXPECT_EQ(json_schema, expected);
}

TEST(json, test_json_schema_one_of_processor_leaves_tagged_union) {
  const auto json_schema =
      rfl::json::to_schema<rfl::TaggedUnion<"shape", Circle, Square>,
                           rfl::VariantsAsOneOf>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","oneOf":[{"$ref":"#/$defs/test_json_schema_one_of__Circle__tagged"},{"$ref":"#/$defs/test_json_schema_one_of__Square__tagged"}],"$defs":{"test_json_schema_one_of__Circle__tagged":{"type":"object","properties":{"shape":{"type":"string","enum":["Circle"]},"radius":{"type":"number"}},"required":["shape","radius"]},"test_json_schema_one_of__Square__tagged":{"type":"object","properties":{"shape":{"type":"string","enum":["Square"]},"width":{"type":"number"}},"required":["shape","width"]}}})";

  EXPECT_EQ(json_schema, expected);
}

TEST(json, test_json_schema_one_of_processor_leaves_optional) {
  const auto json_schema =
      rfl::json::to_schema<std::optional<int>, rfl::VariantsAsOneOf>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[{"type":"integer"},{"type":"null"}],"$defs":{}})";

  EXPECT_EQ(json_schema, expected);
}

TEST(json, test_json_schema_one_of_processor_leaves_generic) {
  // 1 is both an integer and a number, so rfl::Generic must stay anyOf.
  const auto json_schema =
      rfl::json::to_schema<rfl::Generic, rfl::VariantsAsOneOf>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/rfl__Generic","$defs":{"rfl__Generic":{"anyOf":[{"anyOf":[{"type":"boolean"},{"type":"integer"},{"type":"number"},{"type":"string"},{"type":"object","additionalProperties":{"$ref":"#/$defs/rfl__Generic"}},{"type":"array","items":{"$ref":"#/$defs/rfl__Generic"}}]},{"type":"null"}]}}})";

  EXPECT_EQ(json_schema, expected);
}

}  // namespace test_json_schema_one_of
