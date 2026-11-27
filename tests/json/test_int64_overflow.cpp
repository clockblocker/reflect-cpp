#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <rfl/json.hpp>
#include <string>
#include <variant>

namespace test_int64_overflow {

struct SignedHolder {
  int64_t val;
};

struct Int32Holder {
  int32_t val;
};

struct VariantHolder {
  std::variant<int64_t, uint64_t> val;
};

TEST(json, int64_rejects_unsigned_overflow) {
  // INT64_MAX succeeds
  {
    const auto res =
        rfl::json::read<SignedHolder>(R"({"val":9223372036854775807})");
    ASSERT_TRUE(res.has_value()) << res.error().what();
    EXPECT_EQ(res->val, std::numeric_limits<int64_t>::max());
  }

  // INT64_MIN succeeds
  {
    const auto res =
        rfl::json::read<SignedHolder>(R"({"val":-9223372036854775808})");
    ASSERT_TRUE(res.has_value()) << res.error().what();
    EXPECT_EQ(res->val, std::numeric_limits<int64_t>::min());
  }

  // INT64_MAX + 1 must fail with ErrorKind::invalid_value (not wrap to INT64_MIN)
  {
    const auto res =
        rfl::json::read<SignedHolder>(R"({"val":9223372036854775808})");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::invalid_value);
  }

  // UINT64_MAX must fail with ErrorKind::invalid_value (not wrap to -1)
  {
    const auto res =
        rfl::json::read<SignedHolder>(R"({"val":18446744073709551615})");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::invalid_value);
  }

  // UINT64_MAX must also fail for int32_t (previously wrapped to -1 and passed!)
  {
    const auto res =
        rfl::json::read<Int32Holder>(R"({"val":18446744073709551615})");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::invalid_value);
  }

  // Non-integer still reports ErrorKind::invalid_type
  {
    const auto res = rfl::json::read<SignedHolder>(R"({"val":"abc"})");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::invalid_type);
  }
}

TEST(json, variant_int64_uint64_disambiguation) {
  constexpr uint64_t kAboveI64 =
      static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1ULL;
  constexpr uint64_t kMaxU64 = std::numeric_limits<uint64_t>::max();

  // Negative value -> int64_t
  {
    const VariantHolder orig{.val = int64_t{-42}};
    const auto parsed = rfl::json::read<VariantHolder>(rfl::json::write(orig));
    ASSERT_TRUE(parsed.has_value()) << parsed.error().what();
    ASSERT_TRUE(std::holds_alternative<int64_t>(parsed->val));
    EXPECT_EQ(std::get<int64_t>(parsed->val), -42);
  }

  // INT64_MAX -> int64_t
  {
    const VariantHolder orig{.val = std::numeric_limits<int64_t>::max()};
    const auto parsed = rfl::json::read<VariantHolder>(rfl::json::write(orig));
    ASSERT_TRUE(parsed.has_value()) << parsed.error().what();
    ASSERT_TRUE(std::holds_alternative<int64_t>(parsed->val));
    EXPECT_EQ(std::get<int64_t>(parsed->val),
              std::numeric_limits<int64_t>::max());
  }

  // INT64_MAX + 1 -> falls through int64_t to uint64_t
  {
    const VariantHolder orig{.val = kAboveI64};
    const auto parsed = rfl::json::read<VariantHolder>(rfl::json::write(orig));
    ASSERT_TRUE(parsed.has_value()) << parsed.error().what();
    ASSERT_TRUE(std::holds_alternative<uint64_t>(parsed->val));
    EXPECT_EQ(std::get<uint64_t>(parsed->val), kAboveI64);
  }

  // UINT64_MAX -> falls through int64_t to uint64_t
  {
    const VariantHolder orig{.val = kMaxU64};
    const auto parsed = rfl::json::read<VariantHolder>(rfl::json::write(orig));
    ASSERT_TRUE(parsed.has_value()) << parsed.error().what();
    ASSERT_TRUE(std::holds_alternative<uint64_t>(parsed->val));
    EXPECT_EQ(std::get<uint64_t>(parsed->val), kMaxU64);
  }
}

}  // namespace test_int64_overflow
