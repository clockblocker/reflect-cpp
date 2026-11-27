#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

namespace test_descriptor_table_c_arrays {

struct WithCArrays {
  int nums[3] = {1, 2, 3};
  std::string words[2] = {"a", "b"};
  int tail = 9;
};

TEST(descriptor_table_c_arrays, c_arrays_read_write_default_schema) {
  const std::string json =
      R"({"nums":[10,20,30],"words":["hi","yo"],"tail":42})";
  const auto res = rfl::json::read<WithCArrays>(json);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->nums[0], 10);
  EXPECT_EQ(res->nums[2], 30);
  EXPECT_EQ(res->words[1], "yo");
  EXPECT_EQ(res->tail, 42);
  EXPECT_EQ(rfl::json::write(*res), json);

  const auto def_res =
      rfl::json::read<WithCArrays, rfl::DefaultIfMissing>(R"({"tail":77})");
  ASSERT_TRUE(def_res.has_value()) << def_res.error().what();
  EXPECT_EQ(def_res->nums[1], 2);
  EXPECT_EQ(def_res->words[0], "a");
  EXPECT_EQ(def_res->tail, 77);

  const auto schema = rfl::json::to_schema<WithCArrays>();
  EXPECT_NE(schema.find("nums"), std::string::npos);
  EXPECT_NE(schema.find("words"), std::string::npos);
}

struct EmptyBase {};
struct BaseWithMembers {
  int base_x = 10;
  std::string base_name = "base";
};
struct DerivedWithBaseMembers : EmptyBase, BaseWithMembers {};

TEST(descriptor_table_c_arrays, inherited_base_members_roundtrip) {
  const std::string json = R"({"base_x":42,"base_name":"hello"})";
  const auto res = rfl::json::read<DerivedWithBaseMembers>(json);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->base_x, 42);
  EXPECT_EQ(res->base_name, "hello");
  EXPECT_EQ(rfl::json::write(*res), json);
}

}  // namespace test_descriptor_table_c_arrays
