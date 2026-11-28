#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/yaml.hpp>
#include <string>

namespace test_yaml_default_tag {

struct TargetConfigV1 {
  using Tag = rfl::Literal<"1">;
  std::string name;
  rfl::DefaultVal<int> maxRetries = 3;
};

struct TargetConfigV2 {
  using Tag = rfl::Literal<"2">;
  std::string name;
  bool strictMode;
};

using TargetConfigSchema =
    rfl::TaggedUnion<"version", TargetConfigV1, TargetConfigV2>;

TEST(yaml, test_default_tag_unversioned_and_versioned_yaml) {
  using YamlProcessors =
      rfl::Processors<rfl::CamelCaseToSnakeCase, rfl::SnakeCaseToKebabCase,
                      rfl::NoExtraFields, rfl::DefaultTag<"version", "1">>;

  // Unversioned YAML falls back to schema v1 without manual YAML::Node edits.
  const std::string v1_yaml = "name: legacy-qpu\n";
  const auto res_v1 =
      rfl::yaml::read<TargetConfigSchema, YamlProcessors>(v1_yaml);
  ASSERT_TRUE(res_v1) << res_v1.error().what();
  const auto* v1 = rfl::get_if<TargetConfigV1>(&res_v1->variant());
  ASSERT_NE(v1, nullptr);
  EXPECT_EQ(v1->name, "legacy-qpu");
  EXPECT_EQ(v1->maxRetries.value(), 3);

  // Explicit version "2" dispatches to TargetConfigV2.
  const std::string v2_yaml =
      "version: \"2\"\n"
      "name: modern-qpu\n"
      "strict-mode: true\n";
  const auto res_v2 =
      rfl::yaml::read<TargetConfigSchema, YamlProcessors>(v2_yaml);
  ASSERT_TRUE(res_v2) << res_v2.error().what();
  const auto* v2 = rfl::get_if<TargetConfigV2>(&res_v2->variant());
  ASSERT_NE(v2, nullptr);
  EXPECT_EQ(v2->name, "modern-qpu");
  EXPECT_TRUE(v2->strictMode);

  // Present unknown version still fails.
  const auto res_bad = rfl::yaml::read<TargetConfigSchema, YamlProcessors>(
      std::string("version: \"3\"\nname: bad\n"));
  ASSERT_FALSE(res_bad);
}

}  // namespace test_yaml_default_tag
