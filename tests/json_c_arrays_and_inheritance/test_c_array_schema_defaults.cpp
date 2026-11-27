#include <gtest/gtest.h>

#include <array>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

namespace test_c_array_schema_defaults {

struct CArrayWithDefaults {
  int coords[3] = {1, 2, 3};
  rfl::DefaultVal<std::string> label = "origin";
};

}  // namespace test_c_array_schema_defaults

template <>
struct rfl::Metadata<test_c_array_schema_defaults::CArrayWithDefaults> {
  static constexpr std::array<rfl::FieldMeta, 2> fields = {{
      {"coords", "3D coordinates"},
      {"label", "Point label"},
  }};
};

namespace test_c_array_schema_defaults {

TEST(json, test_c_array_schema_with_defaults_and_metadata) {
  const auto schema = rfl::json::to_schema<CArrayWithDefaults>();
  EXPECT_NE(schema.find("\"description\":\"3D coordinates\""),
            std::string::npos);
  EXPECT_NE(schema.find("\"description\":\"Point label\""), std::string::npos);
  EXPECT_NE(schema.find("\"default\":\"origin\""), std::string::npos);
}

}  // namespace test_c_array_schema_defaults
