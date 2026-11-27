#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

namespace test_direct_struct_c_arrays {

struct DirectCArrays {
  int nums[3] = {1, 2, 3};
  std::string words[2] = {"a", "b"};
  int tail = 9;
};

static_assert(rfl::parsing::can_use_direct_struct_parser_v<
              DirectCArrays, rfl::Processors<>>);

TEST(json_c_arrays_and_inheritance, test_direct_struct_c_arrays) {
  const std::string json =
      R"({"nums":[10,20,30],"words":["hi","yo"],"tail":42})";
  const auto res = rfl::json::read<DirectCArrays>(json);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->nums[0], 10);
  EXPECT_EQ(res->nums[2], 30);
  EXPECT_EQ(res->words[0], "hi");
  EXPECT_EQ(res->words[1], "yo");
  EXPECT_EQ(res->tail, 42);
  EXPECT_EQ(rfl::json::write(*res), json);

  const auto def_res =
      rfl::json::read<DirectCArrays, rfl::DefaultIfMissing>(R"({"tail":77})");
  ASSERT_TRUE(def_res.has_value()) << def_res.error().what();
  EXPECT_EQ(def_res->nums[0], 1);
  EXPECT_EQ(def_res->nums[2], 3);
  EXPECT_EQ(def_res->words[0], "a");
  EXPECT_EQ(def_res->words[1], "b");
  EXPECT_EQ(def_res->tail, 77);
}

}  // namespace test_direct_struct_c_arrays
