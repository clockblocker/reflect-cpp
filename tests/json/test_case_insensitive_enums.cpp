#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

namespace test_case_insensitive_enums {

enum class LogLevel { critical, err, warn, info, debug, trace, off };
enum class ChannelPriority { Disabled, Flexible, Strict };

struct Config {
  LogLevel log_level = LogLevel::info;
  ChannelPriority priority = ChannelPriority::Flexible;
};

TEST(json, test_case_insensitive_enums) {
  const auto r1 = rfl::json::read<Config, rfl::CaseInsensitiveEnums>(
      R"({"log_level":"DEBUG","priority":"strict"})");
  ASSERT_TRUE(r1) << r1.error().what();
  EXPECT_EQ(r1->log_level, LogLevel::debug);
  EXPECT_EQ(r1->priority, ChannelPriority::Strict);

  const auto r2 = rfl::json::read<Config, rfl::CaseInsensitiveEnums>(
      R"({"log_level":"WaRn","priority":"DiSaBlEd"})");
  ASSERT_TRUE(r2) << r2.error().what();
  EXPECT_EQ(r2->log_level, LogLevel::warn);
  EXPECT_EQ(r2->priority, ChannelPriority::Disabled);

  // Without CaseInsensitiveEnums, wrong case is rejected.
  const auto strict_fail =
      rfl::json::read<Config>(R"({"log_level":"DEBUG","priority":"Strict"})");
  EXPECT_FALSE(strict_fail);

  // Combined with EnumNamesOnly, unknown strings and numeric strings fail.
  const auto bad_name =
      rfl::json::read<Config, rfl::CaseInsensitiveEnums, rfl::EnumNamesOnly>(
          R"({"log_level":"nope","priority":"Strict"})");
  EXPECT_FALSE(bad_name);

  const auto bad_num =
      rfl::json::read<Config, rfl::CaseInsensitiveEnums, rfl::EnumNamesOnly>(
          R"({"log_level":"0","priority":"Strict"})");
  EXPECT_FALSE(bad_num);

  // Direct helper.
  const auto direct = rfl::parse_enum_icase<LogLevel, true>("TRACE");
  ASSERT_TRUE(direct);
  EXPECT_EQ(*direct, LogLevel::trace);
}

}  // namespace test_case_insensitive_enums
