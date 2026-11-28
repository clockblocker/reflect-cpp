#include <cstdint>
#include <limits>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <vector>

#include "write_and_read.hpp"

namespace test_stringified {

struct Record {
  rfl::Stringified<int64_t> id;
  rfl::Stringified<uint64_t> sequence;
};

TEST(json, test_stringified_64bit_roundtrip) {
  const auto record = Record{
      .id = 9007199254740993LL,
      .sequence = std::numeric_limits<uint64_t>::max(),
  };
  write_and_read(
      record,
      R"({"id":"9007199254740993","sequence":"18446744073709551615"})");
}

TEST(json, test_stringified_signed_and_narrow_bounds) {
  struct Bounds {
    rfl::Stringified<int64_t> min_i64;
    rfl::Stringified<int64_t> max_i64;
    rfl::Stringified<int8_t> min_i8;
    rfl::Stringified<uint8_t> max_u8;
  };

  const auto bounds = Bounds{
      .min_i64 = std::numeric_limits<int64_t>::min(),
      .max_i64 = std::numeric_limits<int64_t>::max(),
      .min_i8 = std::numeric_limits<int8_t>::min(),
      .max_u8 = std::numeric_limits<uint8_t>::max(),
  };
  write_and_read(
      bounds,
      R"({"min_i64":"-9223372036854775808","max_i64":"9223372036854775807","min_i8":"-128","max_u8":"255"})");
}

TEST(json, test_stringified_invalid_inputs) {
  struct SingleSigned {
    rfl::Stringified<int64_t> v;
  };
  struct SingleUnsigned {
    rfl::Stringified<uint64_t> v;
  };
  struct SingleSmall {
    rfl::Stringified<int8_t> v;
  };

  // Bare JSON number is rejected (wire type is string).
  EXPECT_FALSE(rfl::json::read<SingleSigned>(R"({"v":42})"));
  // Empty string, whitespace, plus sign, float, trailing garbage.
  EXPECT_FALSE(rfl::json::read<SingleSigned>(R"({"v":""})"));
  EXPECT_FALSE(rfl::json::read<SingleSigned>(R"({"v":" 42"})"));
  EXPECT_FALSE(rfl::json::read<SingleSigned>(R"({"v":"42 "})"));
  EXPECT_FALSE(rfl::json::read<SingleSigned>(R"({"v":"+42"})"));
  EXPECT_FALSE(rfl::json::read<SingleSigned>(R"({"v":"42.0"})"));
  EXPECT_FALSE(rfl::json::read<SingleSigned>(R"({"v":"42abc"})"));
  // Overflow / underflow.
  EXPECT_FALSE(
      rfl::json::read<SingleSigned>(R"({"v":"9223372036854775808"})"));
  EXPECT_FALSE(
      rfl::json::read<SingleSigned>(R"({"v":"-9223372036854775809"})"));
  EXPECT_FALSE(
      rfl::json::read<SingleUnsigned>(R"({"v":"18446744073709551616"})"));
  EXPECT_FALSE(rfl::json::read<SingleUnsigned>(R"({"v":"-1"})"));
  EXPECT_FALSE(rfl::json::read<SingleSmall>(R"({"v":"128"})"));
  EXPECT_FALSE(rfl::json::read<SingleSmall>(R"({"v":"-129"})"));
}

TEST(json, test_stringified_accessors_and_composition) {
  struct Envelope {
    std::optional<rfl::Stringified<int64_t>> maybe_id;
    std::vector<rfl::Stringified<uint64_t>> ids;
  };

  const auto env = Envelope{
      .maybe_id = -42LL,
      .ids = {1ULL, 9007199254740993ULL},
  };
  write_and_read(env, R"({"maybe_id":"-42","ids":["1","9007199254740993"]})");

  rfl::Stringified<int64_t> x = 123LL;
  EXPECT_EQ(x.value(), 123LL);
  EXPECT_EQ(x.get(), 123LL);
  EXPECT_EQ(*x, 123LL);
  EXPECT_EQ(x(), 123LL);
  EXPECT_EQ(x.str(), "123");
  EXPECT_EQ(x, 123LL);
  x.set(456LL);
  EXPECT_EQ(*x.operator->(), 456LL);

  const auto parsed = rfl::Stringified<int64_t>::from_string("9007199254740993");
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(parsed->value(), 9007199254740993LL);
  EXPECT_FALSE(rfl::Stringified<uint64_t>::from_string("-5").has_value());

  const auto schema = rfl::json::to_schema<Record>();
  EXPECT_NE(schema.find(R"("type":"string")"), std::string::npos);
}

}  // namespace test_stringified
