#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <variant>

namespace test_optional_variant_schema {

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

TEST(json, optional_preserves_inner_annotated_variant) {
  const auto schema = rfl::json::to_schema<
      std::optional<rfl::Description<"inner", std::variant<int, std::string>>>>();
  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[{"description":"inner","anyOf":[{"type":"integer"},{"type":"string"}]},{"type":"null"}],"$defs":{}})";
  EXPECT_EQ(schema, expected);
}

}  // namespace test_optional_variant_schema
