#include <gtest/gtest.h>

#include <array>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <variant>

namespace test_error_causes {

// Mirrors osmium-tool#3 BBox union: array of 4 coords OR struct with validated
// bounds where failing Minimum<-180> previously lost path/kind/code.
struct BBoxObject {
  rfl::Validator<double, rfl::Minimum<-180.0>, rfl::Maximum<180.0>> left;
  rfl::Validator<double, rfl::Minimum<-90.0>, rfl::Maximum<90.0>> bottom;
  rfl::Validator<double, rfl::Minimum<-180.0>, rfl::Maximum<180.0>> right;
  rfl::Validator<double, rfl::Minimum<-90.0>, rfl::Maximum<90.0>> top;
};

using BBoxArray = std::array<double, 4>;
using StdBBox = std::variant<BBoxArray, BBoxObject>;
using RflBBox = rfl::Variant<BBoxArray, BBoxObject>;

struct ExportConfig {
  StdBBox bbox;
};

struct RflExportConfig {
  RflBBox bbox;
};

struct NestedUnion {
  std::variant<int, std::variant<bool, BBoxObject>> item;
};

TEST(json, test_std_variant_preserves_branch_causes_with_path_kind_code) {
  const auto res = rfl::json::read<ExportConfig>(
      R"({"bbox":{"left":-999.0,"bottom":0.0,"right":10.0,"top":10.0}})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().kind(), rfl::ErrorKind::invalid_value);

  const auto details = res.error().details();
  ASSERT_EQ(details.size(), 1u);
  ASSERT_EQ(details[0].path.size(), 1u);
  EXPECT_EQ(std::get<std::string>(details[0].path[0]), "bbox");
  EXPECT_EQ(details[0].kind, rfl::ErrorKind::invalid_value);

  // Branch 0 (BBoxArray) failed with type_mismatch; branch 1 (BBoxObject)
  // failed at relative path ["left"] with constraint_violation / "minimum".
  ASSERT_EQ(details[0].causes.size(), 2u);
  EXPECT_EQ(details[0].causes[0].kind, rfl::ErrorKind::invalid_type);

  const auto& bbox_cause = details[0].causes[1];
  ASSERT_EQ(bbox_cause.path.size(), 1u);
  EXPECT_EQ(std::get<std::string>(bbox_cause.path[0]), "left");
  EXPECT_EQ(bbox_cause.kind, rfl::ErrorKind::constraint_violation);
  EXPECT_EQ(bbox_cause.code, "minimum");
  EXPECT_TRUE(bbox_cause.causes.empty());
}

TEST(json, test_rfl_variant_preserves_branch_causes) {
  const auto res = rfl::json::read<RflExportConfig>(
      R"({"bbox":{"left":0.0,"bottom":0.0,"right":10.0}})");
  ASSERT_FALSE(res);
  const auto details = res.error().details();
  ASSERT_EQ(details.size(), 1u);
  ASSERT_EQ(details[0].causes.size(), 2u);
  EXPECT_EQ(details[0].causes[0].kind, rfl::ErrorKind::invalid_type);

  const auto& obj_cause = details[0].causes[1];
  ASSERT_EQ(obj_cause.path.size(), 1u);
  EXPECT_EQ(std::get<std::string>(obj_cause.path[0]), "top");
  EXPECT_EQ(obj_cause.kind, rfl::ErrorKind::missing_field);
}

TEST(json, test_nested_variant_builds_recursive_causes_tree) {
  const auto res = rfl::json::read<NestedUnion>(
      R"({"item":{"left":0.0,"bottom":999.0,"right":10.0,"top":10.0}})");
  ASSERT_FALSE(res);
  const auto details = res.error().details();
  ASSERT_EQ(details.size(), 1u);
  ASSERT_EQ(details[0].causes.size(), 2u);
  // Outer branch 0 = int (type_mismatch), outer branch 1 = inner variant.
  EXPECT_EQ(details[0].causes[0].kind, rfl::ErrorKind::invalid_type);
  const auto& inner_var = details[0].causes[1];
  EXPECT_EQ(inner_var.kind, rfl::ErrorKind::invalid_value);
  ASSERT_EQ(inner_var.causes.size(), 2u);
  EXPECT_EQ(inner_var.causes[0].kind, rfl::ErrorKind::invalid_type);
  ASSERT_EQ(inner_var.causes[1].path.size(), 1u);
  EXPECT_EQ(std::get<std::string>(inner_var.causes[1].path[0]), "bottom");
  EXPECT_EQ(inner_var.causes[1].kind, rfl::ErrorKind::constraint_violation);
  EXPECT_EQ(inner_var.causes[1].code, "maximum");
}

TEST(json, test_any_of_validator_preserves_branch_causes) {
  using PositiveOrEvenRange = rfl::Validator<
      int, rfl::AnyOf<rfl::Minimum<10>, rfl::Maximum<-10>>>;
  struct Holder {
    PositiveOrEvenRange score;
  };
  const auto res = rfl::json::read<Holder>(R"({"score":0})");
  ASSERT_FALSE(res);
  const auto details = res.error().details();
  ASSERT_EQ(details.size(), 1u);
  ASSERT_EQ(details[0].path.size(), 1u);
  EXPECT_EQ(std::get<std::string>(details[0].path[0]), "score");
  EXPECT_EQ(details[0].kind, rfl::ErrorKind::constraint_violation);
  EXPECT_EQ(details[0].code, "any_of");
  ASSERT_EQ(details[0].causes.size(), 2u);
  EXPECT_EQ(details[0].causes[0].kind, rfl::ErrorKind::constraint_violation);
  EXPECT_EQ(details[0].causes[0].code, "minimum");
  EXPECT_EQ(details[0].causes[1].kind, rfl::ErrorKind::constraint_violation);
  EXPECT_EQ(details[0].causes[1].code, "maximum");
}

TEST(json, test_error_detail_aggregate_compat_and_equality) {
  // Trailing defaulted `causes` keeps existing aggregate initializers valid.
  rfl::ErrorDetail a{.path = {std::string("x")},
                     .message = "bad",
                     .kind = rfl::ErrorKind::invalid_value,
                     .code = "minimum"};
  rfl::ErrorDetail b = a;
  EXPECT_EQ(a, b);
  b.causes.push_back(rfl::ErrorDetail{.path = {}, .message = "inner"});
  EXPECT_NE(a, b);

  // Leaf errors stay zero-allocation (no causes allocated).
  const auto leaf = rfl::json::read<int>("\"abc\"");
  ASSERT_FALSE(leaf);
  EXPECT_FALSE(rfl::internal::has_allocated_details(leaf.error()));
  EXPECT_TRUE(leaf.error().details()[0].causes.empty());
}

}  // namespace test_error_causes
