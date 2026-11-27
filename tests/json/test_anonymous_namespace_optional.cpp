#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <rfl/json.hpp>
#include <string>

namespace {

struct AnonInner {
  int x;
  std::string label;
};

struct AnonMoveOnly {
  std::unique_ptr<int> ptr;
};

struct AnonOuter {
  std::optional<AnonInner> inner;
  std::optional<AnonMoveOnly> move_only;
};

}  // namespace

TEST(json, anonymous_namespace_struct_in_optional) {
  AnonOuter src{
      .inner = AnonInner{.x = 42, .label = "hello"},
      .move_only = AnonMoveOnly{.ptr = std::make_unique<int>(7)},
  };
  const auto json_str = rfl::json::write(src);
  const auto parsed = rfl::json::read<AnonOuter>(json_str);
  ASSERT_TRUE(parsed.has_value()) << parsed.error().what();
  ASSERT_TRUE(parsed->inner.has_value());
  EXPECT_EQ(parsed->inner->x, 42);
  EXPECT_EQ(parsed->inner->label, "hello");
  ASSERT_TRUE(parsed->move_only.has_value());
  ASSERT_NE(parsed->move_only->ptr, nullptr);
  EXPECT_EQ(*parsed->move_only->ptr, 7);

  const auto empty_parsed = rfl::json::read<AnonOuter>("{}");
  ASSERT_TRUE(empty_parsed.has_value()) << empty_parsed.error().what();
  EXPECT_FALSE(empty_parsed->inner.has_value());
  EXPECT_FALSE(empty_parsed->move_only.has_value());
}
