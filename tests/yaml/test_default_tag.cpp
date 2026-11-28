#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/yaml.hpp>
#include <string>
#include <vector>

namespace test_yaml_default_tag {

enum class CostModel { t_count, two_qubit_gate_count, gate_count };
enum class LoopUnrolling { all, index_use };

// Plain public struct with in-class defaults (no rfl:: wrappers in header).
struct QuakeUnmappedProfile {
  std::vector<std::string> basisGates = {"rx", "rz", "cx"};
  double approximationTolerance = 1e-6;
  std::optional<CostModel> costModel;
  LoopUnrolling unrollLoops = LoopUnrolling::index_use;
  unsigned maxUnrollIterations = 1000;
};

struct TargetConfigV1 {
  using Tag = rfl::Literal<"1">;
  std::string name;
  CostModel costModel = CostModel::gate_count;
};

struct TargetConfigV2 {
  using Tag = rfl::Literal<"2">;
  std::string name;
  QuakeUnmappedProfile profile;
};

using TargetConfigSchema =
    rfl::TaggedUnion<"version", TargetConfigV1, TargetConfigV2>;

}  // namespace test_yaml_default_tag

namespace rfl {
template <>
struct DefaultIfMissingFor<test_yaml_default_tag::QuakeUnmappedProfile>
    : std::true_type {};
}  // namespace rfl

namespace test_yaml_default_tag {

TEST(yaml, test_default_tag_with_cudaq_style_yaml_pipeline) {
  using YamlProcessors =
      rfl::Processors<rfl::CamelCaseToSnakeCase, rfl::SnakeCaseToKebabCase,
                      rfl::EnumSnakeCaseToKebabCase, rfl::EnumNamesOnly,
                      rfl::NoExtraFields, rfl::NoDuplicateKeys,
                      rfl::DefaultTag<"version", "1">>;

  // 1. Unversioned YAML defaults to schema v1 without manual YAML::Node edits.
  const std::string v1_yaml =
      "name: legacy-qpu\n"
      "cost-model: two-qubit-gate-count\n";
  const auto res_v1 =
      rfl::yaml::read<TargetConfigSchema, YamlProcessors>(v1_yaml);
  ASSERT_TRUE(res_v1) << res_v1.error().what();
  const auto* v1 = rfl::get_if<TargetConfigV1>(&res_v1->variant());
  ASSERT_NE(v1, nullptr);
  EXPECT_EQ(v1->name, "legacy-qpu");
  EXPECT_EQ(v1->costModel, CostModel::two_qubit_gate_count);

  // 2. Numeric enum spellings are rejected by EnumNamesOnly.
  const std::string bad_enum_yaml =
      "name: legacy-qpu\n"
      "cost-model: \"0\"\n";
  const auto res_bad_enum =
      rfl::yaml::read<TargetConfigSchema, YamlProcessors>(bad_enum_yaml);
  ASSERT_FALSE(res_bad_enum);

  // 3. Version 2 preserves plain-struct in-class defaults via
  //    DefaultIfMissingFor<QuakeUnmappedProfile> while keeping TargetConfigV2's
  //    own `name` field required.
  const std::string v2_yaml =
      "version: \"2\"\n"
      "name: modern-qpu\n"
      "profile:\n"
      "  cost-model: t-count\n";
  const auto res_v2 =
      rfl::yaml::read<TargetConfigSchema, YamlProcessors>(v2_yaml);
  ASSERT_TRUE(res_v2) << res_v2.error().what();
  const auto* v2 = rfl::get_if<TargetConfigV2>(&res_v2->variant());
  ASSERT_NE(v2, nullptr);
  EXPECT_EQ(v2->name, "modern-qpu");
  EXPECT_EQ(v2->profile.basisGates,
            (std::vector<std::string>{"rx", "rz", "cx"}));
  EXPECT_DOUBLE_EQ(v2->profile.approximationTolerance, 1e-6);
  ASSERT_TRUE(v2->profile.costModel.has_value());
  EXPECT_EQ(*v2->profile.costModel, CostModel::t_count);
  EXPECT_EQ(v2->profile.unrollLoops, LoopUnrolling::index_use);
  EXPECT_EQ(v2->profile.maxUnrollIterations, 1000u);

  // 4. Required field on TargetConfigV2 (`name`) is still rejected if omitted.
  const std::string v2_missing_name =
      "version: \"2\"\n"
      "profile: {}\n";
  const auto res_missing =
      rfl::yaml::read<TargetConfigSchema, YamlProcessors>(v2_missing_name);
  ASSERT_FALSE(res_missing);
}

}  // namespace test_yaml_default_tag
