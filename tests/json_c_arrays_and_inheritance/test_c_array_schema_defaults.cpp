#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

namespace test_c_array_schema_defaults {

struct CArrayWithDefaults {
  int coords[3] = {1, 2, 3};
  rfl::DefaultVal<std::string> label = "origin";
};

struct WithCArray {
  int a[3];
  int b;
};

TEST(json, test_c_array_schema_with_defaults_and_num_fields) {
  static_assert(rfl::num_fields<WithCArray> == 2);

  const auto schema_dv = rfl::json::to_schema<CArrayWithDefaults>();
  EXPECT_NE(schema_dv.find("\"default\":\"origin\""), std::string::npos);

  const auto schema_dim =
      rfl::json::to_schema<CArrayWithDefaults, rfl::DefaultIfMissing>();
  EXPECT_NE(schema_dim.find("\"default\":[1,2,3]"), std::string::npos);
  EXPECT_NE(schema_dim.find("\"default\":\"origin\""), std::string::npos);
}

}  // namespace test_c_array_schema_defaults
