#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/num_fields.hpp>
#include <rfl/type_name.hpp>
#include <string>
#include <string_view>

namespace test_type_name_and_num_fields {

struct Empty {};

struct Point {
  int x;
  int y;
};

namespace nested {
struct Config {
  std::string name;
  int port = 8080;
  bool tls = true;
};
}  // namespace nested

// Compile-time checks: rfl::type_name<T>() and rfl::type_name_v<T>
static_assert(rfl::type_name<Point>() == rfl::type_name_v<Point>);
static_assert(rfl::type_name<Point>() ==
              rfl::internal::get_type_name_str_view<Point>());
static_assert(rfl::type_name<Point>().find("Point") != std::string_view::npos);
static_assert(rfl::type_name_v<nested::Config>.find("nested::Config") !=
              std::string_view::npos);
static_assert(rfl::type_name_v<Point>.data()[rfl::type_name_v<Point>.size()] ==
              '\0');

// Compile-time checks: rfl::num_fields<T> and rfl::num_fields_v<T>
static_assert(rfl::num_fields<Empty> == 0);
static_assert(rfl::num_fields_v<Empty> == 0);
static_assert(rfl::num_fields<Point> == 2);
static_assert(rfl::num_fields_v<Point> == 2);
static_assert(rfl::num_fields<nested::Config> == 3);
static_assert(rfl::num_fields_v<nested::Config> == 3);

TEST(json, test_type_name_and_num_fields) {
  constexpr std::string_view sv = rfl::type_name<nested::Config>();
  EXPECT_EQ(sv, rfl::type_name_v<nested::Config>);
  EXPECT_EQ(sv, rfl::type_name_t<nested::Config>().str());
  EXPECT_NE(sv.find("nested::Config"), std::string_view::npos);

  EXPECT_EQ(rfl::num_fields_v<Empty>, 0u);
  EXPECT_EQ(rfl::num_fields_v<Point>, 2u);
  EXPECT_EQ(rfl::num_fields_v<nested::Config>, 3u);
}

}  // namespace test_type_name_and_num_fields
