#include <cstdint>
#include <rfl.hpp>
#include <rfl/json.hpp>

#include "write_and_read.hpp"

namespace test_integers {

TEST(json, test_narrow_integer_bounds) {
  struct NarrowIntegers {
    int16_t signed_value;
    uint16_t unsigned_value;
  };

  write_and_read(NarrowIntegers{.signed_value = INT16_MIN,
                                .unsigned_value = UINT16_MAX},
                 R"({"signed_value":-32768,"unsigned_value":65535})");
  EXPECT_FALSE(rfl::json::read<NarrowIntegers>(
      R"({"signed_value":32768,"unsigned_value":1})"));
  EXPECT_FALSE(rfl::json::read<NarrowIntegers>(
      R"({"signed_value":-32769,"unsigned_value":1})"));
  EXPECT_FALSE(rfl::json::read<NarrowIntegers>(
      R"({"signed_value":0,"unsigned_value":100200})"));
}

TEST(json, test_signed_overflow_from_large_uint64) {
  struct Signed64 {
    int64_t value;
  };
  struct Signed32 {
    int32_t value;
  };
  struct Signed8 {
    int8_t value;
  };

  // INT64_MAX + 1 (9223372036854775808) and UINT64_MAX (18446744073709551615)
  // are parsed by yyjson as YYJSON_SUBTYPE_UINT and must not wrap around to
  // negative signed integers (e.g. INT64_MIN or -1).
  EXPECT_FALSE(
      rfl::json::read<Signed64>(R"({"value":9223372036854775808})"));
  EXPECT_FALSE(
      rfl::json::read<Signed64>(R"({"value":18446744073709551615})"));
  EXPECT_FALSE(
      rfl::json::read<Signed32>(R"({"value":18446744073709551615})"));
  EXPECT_FALSE(
      rfl::json::read<Signed8>(R"({"value":18446744073709551615})"));

  // Boundary values must still succeed.
  const auto max_i64 =
      rfl::json::read<Signed64>(R"({"value":9223372036854775807})");
  ASSERT_TRUE(max_i64.has_value());
  EXPECT_EQ(max_i64->value, INT64_MAX);

  const auto min_i64 =
      rfl::json::read<Signed64>(R"({"value":-9223372036854775808})");
  ASSERT_TRUE(min_i64.has_value());
  EXPECT_EQ(min_i64->value, INT64_MIN);

  // rfl::Generic must not wrap UINT64_MAX into int64_t(-1).
  const auto generic_val =
      rfl::json::read<rfl::Generic>("18446744073709551615");
  ASSERT_TRUE(generic_val.has_value());
  EXPECT_FALSE(generic_val->to_int64().has_value());
}

TEST(json, test_integers) {

  struct Integers {
      int i32;
      unsigned int u32;
      long long i64;
      unsigned long long u64;
  };

  write_and_read(Integers{.i32 = INT32_MAX, .u32 = UINT32_MAX, .i64 = INT64_MAX, .u64 = UINT64_MAX}, R"({"i32":2147483647,"u32":4294967295,"i64":9223372036854775807,"u64":18446744073709551615})");
  write_and_read(Integers{.i32 = INT32_MIN, .u32 = 0, .i64 = INT64_MIN, .u64 = 0}, R"({"i32":-2147483648,"u32":0,"i64":-9223372036854775808,"u64":0})");
}

}  // namespace test_integers
