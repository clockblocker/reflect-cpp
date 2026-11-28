#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/internal/has_default_val_v.hpp>
#include <rfl/json.hpp>

namespace test_c_array_has_default_val {

struct WithCArrayDefault {
  const int c = 1;
  int arr[3] = {1, 2, 3};
  rfl::DefaultVal<int> d = 5;
};

struct WithCArrayPlain {
  int arr[3] = {1, 2, 3};
  int b = 2;
};

TEST(json, test_c_array_has_default_val) {
  using rfl::internal::has_default_val_v;
  static_assert(has_default_val_v<WithCArrayDefault>);
  static_assert(!has_default_val_v<WithCArrayPlain>);

  const auto parsed =
      rfl::json::read<WithCArrayDefault>(R"({"c":9,"arr":[4,5,6]})").value();
  EXPECT_EQ(parsed.c, 9);
  EXPECT_EQ(parsed.arr[0], 4);
  EXPECT_EQ(parsed.arr[2], 6);
  EXPECT_EQ(parsed.d.value(), 5);
}

}  // namespace test_c_array_has_default_val
