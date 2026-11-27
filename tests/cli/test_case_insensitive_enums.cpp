#include <gtest/gtest.h>

#include <rfl/cli.hpp>

namespace test_cli_case_insensitive_enums {

enum class LogLevel { critical, err, warn, info, debug, trace, off };
enum class ChannelPriority { Disabled, Flexible, Strict };

struct AppOpts {
  LogLevel log_level = LogLevel::info;
  ChannelPriority channel_priority = ChannelPriority::Flexible;
};

TEST(cli, test_case_insensitive_enums) {
  const auto r1 = rfl::cli::read<AppOpts, rfl::CaseInsensitiveEnums>(
      {"--log-level=DEBUG", "--channel-priority=STRICT"});
  ASSERT_TRUE(r1) << r1.error().what();
  EXPECT_EQ(r1->log_level, LogLevel::debug);
  EXPECT_EQ(r1->channel_priority, ChannelPriority::Strict);

  const auto r2 = rfl::cli::read<AppOpts, rfl::CaseInsensitiveEnums>(
      {"--log-level=WaRn", "--channel-priority=disabled"});
  ASSERT_TRUE(r2) << r2.error().what();
  EXPECT_EQ(r2->log_level, LogLevel::warn);
  EXPECT_EQ(r2->channel_priority, ChannelPriority::Disabled);

  const auto bad =
      rfl::cli::read<AppOpts, rfl::CaseInsensitiveEnums, rfl::EnumNamesOnly>(
          {"--log-level=nope", "--channel-priority=Strict"});
  EXPECT_FALSE(bad);
}

}  // namespace test_cli_case_insensitive_enums
