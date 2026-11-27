#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <rfl.hpp>
#include <string>

namespace test_generic_read_into {

struct Inner {
  int a = 1;
  int b = 2;
};

struct Outer {
  std::string name = "base";
  Inner inner;
  std::optional<Inner> opt_inner;
};

TEST(generic, from_generic_into_recursive_and_optionals) {
  Outer base{
      .name = "workspace",
      .inner = Inner{.a = 10, .b = 20},
      .opt_inner = Inner{.a = 100, .b = 200},
  };

  auto inner_patch = rfl::Generic::Object();
  inner_patch["b"] = 99;

  auto opt_patch = rfl::Generic::Object();
  opt_patch["a"] = 555;

  auto layer1 = rfl::Generic::Object();
  layer1["inner"] = inner_patch;
  layer1["opt_inner"] = opt_patch;

  auto layer2 = rfl::Generic::Object();
  layer2["name"] = "local";

  const auto merged =
      rfl::from_generic_into(base, layer1)
          .and_then([&](Outer o) {
            return rfl::from_generic_into(std::move(o), layer2);
          })
          .value();

  // Original base untouched
  EXPECT_EQ(base.name, "workspace");
  EXPECT_EQ(base.inner.b, 20);
  EXPECT_EQ(base.opt_inner->a, 100);

  // Merged result has all layers applied
  EXPECT_EQ(merged.name, "local");
  EXPECT_EQ(merged.inner.a, 10);
  EXPECT_EQ(merged.inner.b, 99);
  ASSERT_TRUE(merged.opt_inner.has_value());
  EXPECT_EQ(merged.opt_inner->a, 555);
  EXPECT_EQ(merged.opt_inner->b, 200);

  // Explicit null clears optional
  auto clear_opt = rfl::Generic::Object();
  clear_opt["opt_inner"] = std::nullopt;
  const auto cleared = rfl::from_generic_into(merged, clear_opt).value();
  EXPECT_FALSE(cleared.opt_inner.has_value());
  EXPECT_EQ(cleared.inner.b, 99);
}

struct ProcessorConfig {
  rfl::Validator<int, rfl::Minimum<1>> max_items = 10;
  rfl::Rename<"customKey", std::string> key = "init";
  rfl::Skip<int> skipped = 42;
};

TEST(generic, from_generic_into_validators_processors_and_atomicity) {
  ProcessorConfig base;
  base.skipped = 77;

  auto valid_obj = rfl::Generic::Object();
  valid_obj["maxItems"] = 25;
  valid_obj["customKey"] = "hello";
  valid_obj["skipped"] = 999;

  const auto merged =
      rfl::from_generic_into<rfl::SnakeCaseToCamelCase, rfl::NoExtraFields>(
          base, valid_obj)
          .value();

  EXPECT_EQ(merged.max_items.value(), 25);
  EXPECT_EQ(merged.key.value(), "hello");
  EXPECT_EQ(merged.skipped.value(), 77);

  // Validator error leaves base untouched
  auto invalid_val = rfl::Generic::Object();
  invalid_val["customKey"] = "mutated";
  invalid_val["maxItems"] = 0;
  const auto res_invalid =
      rfl::from_generic_into<rfl::SnakeCaseToCamelCase>(base, invalid_val);
  EXPECT_FALSE(res_invalid.has_value());
  EXPECT_EQ(base.key.value(), "init");
  EXPECT_EQ(base.max_items.value(), 10);

  // Extra field rejected under NoExtraFields
  auto extra_obj = rfl::Generic::Object();
  extra_obj["unknown"] = 1;
  const auto res_extra =
      rfl::from_generic_into<rfl::NoExtraFields>(base, extra_obj);
  EXPECT_FALSE(res_extra.has_value());
}

struct MoveOnly {
  std::unique_ptr<Inner> inner;
};

TEST(generic, from_generic_into_move_only) {
  MoveOnly base{.inner = std::make_unique<Inner>(Inner{.a = 3, .b = 4})};

  auto inner_obj = rfl::Generic::Object();
  inner_obj["b"] = 40;
  auto root = rfl::Generic::Object();
  root["inner"] = inner_obj;

  const auto merged =
      rfl::generic::read_into(std::move(base), root).value();
  ASSERT_NE(merged.inner, nullptr);
  EXPECT_EQ(merged.inner->a, 3);
  EXPECT_EQ(merged.inner->b, 40);
}

struct NonDefaultInner {
  int a;
  std::string tag;
  NonDefaultInner() = delete;
  NonDefaultInner(int _a, std::string _tag) : a(_a), tag(std::move(_tag)) {}
};

struct NonDefaultOuter {
  NonDefaultInner inner;
  int count;
  NonDefaultOuter() = delete;
  NonDefaultOuter(NonDefaultInner _inner, int _count)
      : inner(std::move(_inner)), count(_count) {}
};

TEST(generic, read_into_non_default_constructible_and_no_field_names) {
  const NonDefaultOuter base(NonDefaultInner(10, "base"), 1);

  auto inner_patch = rfl::Generic::Object();
  inner_patch["a"] = 25;
  auto patch = rfl::Generic::Object();
  patch["inner"] = inner_patch;

  const auto merged = rfl::from_generic_into(base, patch);
  ASSERT_TRUE(merged.has_value()) << merged.error().what();
  EXPECT_EQ(merged->inner.a, 25);
  EXPECT_EQ(merged->inner.tag, "base");
  EXPECT_EQ(merged->count, 1);

  // rfl::NoFieldNames positional array overlay: only first element provided.
  auto arr = rfl::Generic::Array();
  auto inner_arr = rfl::Generic::Array();
  inner_arr.push_back(99);
  arr.push_back(inner_arr);
  const auto positional =
      rfl::from_generic_into<rfl::NoFieldNames>(base, arr);
  ASSERT_TRUE(positional.has_value()) << positional.error().what();
  EXPECT_EQ(positional->inner.a, 99);
  EXPECT_EQ(positional->inner.tag, "base");
  EXPECT_EQ(positional->count, 1);
}

}  // namespace test_generic_read_into
