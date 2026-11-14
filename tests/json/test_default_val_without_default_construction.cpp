#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

// rfl::DefaultVal in places where the struct around it is not
// default-constructed before it is read: rfl::NamedTuple, structs without a
// default constructor, and the schema of an rfl::TaggedUnion alternative.
namespace test_default_val_without_default_construction {

// Counts every use of a Tracker whose constructor never ran.
struct Tracker {
  static constexpr int alive_ = 0x7ac4e1;
  static inline int unconstructed_uses_ = 0;

  using ReflectionType = int;

  Tracker() = default;
  Tracker(int _v) : v_(_v) {}
  Tracker(const Tracker& _other) : v_(_other.checked()) {}
  Tracker(Tracker&& _other) noexcept : v_(_other.checked()) {}
  ~Tracker() { state_ = 0; }
  Tracker& operator=(const Tracker& _other) {
    v_ = _other.checked();
    return *this;
  }
  Tracker& operator=(Tracker&& _other) noexcept {
    v_ = _other.checked();
    return *this;
  }

  int reflection() const { return checked(); }

  int checked() const {
    if (state_ != alive_) {
      ++unconstructed_uses_;
    }
    return v_;
  }

  int v_ = 0;
  int state_ = alive_;
};

struct NoDefaultConstructor {
  using ReflectionType = int;
  NoDefaultConstructor(int _v) : v_(_v) {}
  int reflection() const { return v_; }
  int v_;
};

// A NamedTuple has no in-class initializers, so a missing rfl::DefaultVal
// field is value-initialized. It is constructed before it is used.
TEST(json, test_default_val_named_tuple_missing_field) {
  Tracker::unconstructed_uses_ = 0;

  using WithTracker = rfl::NamedTuple<rfl::Field<"o", rfl::DefaultVal<Tracker>>,
                                      rfl::Field<"z", int>>;
  const auto tracker = rfl::json::read<WithTracker>(R"({"z":1})");
  ASSERT_TRUE(tracker) << tracker.error().what();
  EXPECT_EQ(rfl::get<"o">(tracker.value()).value().v_, 0);
  EXPECT_EQ(rfl::get<"z">(tracker.value()), 1);
  EXPECT_EQ(Tracker::unconstructed_uses_, 0);

  using WithString =
      rfl::NamedTuple<rfl::Field<"n", int>,
                      rfl::Field<"s", rfl::DefaultVal<std::string>>>;
  const auto str = rfl::json::read<WithString>(R"({"n":3})");
  ASSERT_TRUE(str) << str.error().what();
  EXPECT_EQ(rfl::get<"s">(str.value()).value(), "");
}

// If the value cannot be value-initialized, or its constructor throws, the
// missing field is an error.
TEST(json, test_default_val_named_tuple_missing_field_without_zero_value) {
  using NoZero =
      rfl::NamedTuple<rfl::Field<"o", rfl::DefaultVal<NoDefaultConstructor>>,
                      rfl::Field<"z", int>>;
  const auto no_zero = rfl::json::read<NoZero>(R"({"z":1})");
  ASSERT_FALSE(no_zero);
  EXPECT_EQ(no_zero.error().what(), std::string("Field named 'o' not found."));

  using Invalid = rfl::NamedTuple<
      rfl::Field<"v", rfl::DefaultVal<rfl::Validator<int, rfl::Minimum<1>>>>,
      rfl::Field<"z", int>>;
  const auto invalid = rfl::json::read<Invalid>(R"({"z":1})");
  ASSERT_FALSE(invalid);
  EXPECT_EQ(invalid.error().what(), std::string("Field named 'v' not found."));

  const auto present = rfl::json::read<Invalid>(R"({"v":3,"z":1})");
  ASSERT_TRUE(present) << present.error().what();
  EXPECT_EQ(rfl::get<"v">(present.value()).value().value(), 3);
}

// A struct that cannot be default-constructed is read into raw memory, so the
// in-class default of an rfl::DefaultVal field is not available and the field
// is required.
struct DefaultAndNoDefaultConstructor {
  rfl::DefaultVal<int> jobs = 4;
  NoDefaultConstructor n;
};

TEST(json, test_default_val_not_default_constructible) {
  const auto res =
      rfl::json::read<DefaultAndNoDefaultConstructor>(R"({"jobs":8,"n":3})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res.value().jobs.value(), 8);
  EXPECT_EQ(res.value().n.v_, 3);

  const auto missing =
      rfl::json::read<DefaultAndNoDefaultConstructor>(R"({"n":3})");
  ASSERT_FALSE(missing);
  EXPECT_EQ(missing.error().what(),
            std::string("Field named 'jobs' not found."));

  const auto schema = rfl::json::to_schema<DefaultAndNoDefaultConstructor>();
  EXPECT_NE(schema.find(R"("jobs":{"type":"integer"})"), std::string::npos)
      << schema;
}

// The schema leaves out the default values if default construction throws,
// for instance because a validator rejects a field's zero value.
struct DefaultAndInvalidDefault {
  rfl::DefaultVal<int> jobs = 4;
  rfl::Validator<int, rfl::Minimum<1>> n;
};

TEST(json, test_default_val_schema_with_invalid_default) {
  const auto schema = rfl::json::to_schema<DefaultAndInvalidDefault>();
  EXPECT_NE(schema.find(R"("jobs":{"type":"integer"})"), std::string::npos)
      << schema;
}

// The schema reads the defaults of an rfl::TaggedUnion alternative from a
// default-constructed instance of that alternative.
struct DefaultAlternative {
  rfl::DefaultVal<std::string> output = std::string("out.csv");
  int k = 0;
};

struct OtherAlternative {
  std::string q;
};

TEST(json, test_default_val_tagged_union_schema) {
  EXPECT_EQ(
      (rfl::json::to_schema<
          rfl::TaggedUnion<"type", DefaultAlternative, OtherAlternative>>()),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[{"$ref":"#/$defs/test_default_val_without_default_construction__DefaultAlternative__tagged"},{"$ref":"#/$defs/test_default_val_without_default_construction__OtherAlternative__tagged"}],"$defs":{"test_default_val_without_default_construction__DefaultAlternative__tagged":{"type":"object","properties":{"type":{"type":"string","enum":["DefaultAlternative"]},"output":{"type":"string","default":"out.csv"},"k":{"type":"integer"}},"required":["type","k"]},"test_default_val_without_default_construction__OtherAlternative__tagged":{"type":"object","properties":{"type":{"type":"string","enum":["OtherAlternative"]},"q":{"type":"string"}},"required":["type","q"]}}})");
}

}  // namespace test_default_val_without_default_construction
