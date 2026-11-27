#include <gtest/gtest.h>

#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_subcommands_suggestions_enums {

enum class LogLevel { critical, err, warn, info, debug, trace, off };
enum class ChannelPriority { Disabled, Flexible, Strict };

struct AppOpts {
  rfl::cli::Opt<LogLevel> log_level = LogLevel::info;
  rfl::cli::Opt<ChannelPriority> channel_priority = ChannelPriority::Flexible;
};

TEST(cli, case_insensitive_enums) {
  const auto r1 = rfl::cli::read<AppOpts, rfl::cli::CaseInsensitiveEnums,
                                 rfl::cli::SeparateOptionValues>(
      {"--log-level", "DEBUG", "--channel-priority=STRICT"});
  ASSERT_TRUE(r1) << r1.error().what();
  EXPECT_EQ(r1->log_level.value(), LogLevel::debug);
  EXPECT_EQ(r1->channel_priority.value(), ChannelPriority::Strict);

  const auto r2 = rfl::cli::read<AppOpts, rfl::cli::CaseInsensitiveEnums>(
      {"--log-level=WaRn", "--channel-priority=disabled"});
  ASSERT_TRUE(r2) << r2.error().what();
  EXPECT_EQ(r2->log_level.value(), LogLevel::warn);
  EXPECT_EQ(r2->channel_priority.value(), ChannelPriority::Disabled);

  const auto bad = rfl::cli::read<AppOpts, rfl::cli::CaseInsensitiveEnums,
                                  rfl::EnumNamesOnly>(
      {"--log-level=nope"});
  EXPECT_FALSE(bad);

  // Direct helper for DynamicOptions callers.
  const auto direct = rfl::cli::parse_enum_icase<LogLevel, true>("TRACE");
  ASSERT_TRUE(direct);
  EXPECT_EQ(*direct, LogLevel::trace);
}

TEST(cli, suggestions_and_command_tree) {
  rfl::cli::CommandNode root("micromamba", "Fast conda package manager");
  root.require_subcommand(true);

  std::string ran;
  std::vector<std::string> run_cmd;

  root.add_subcommand("install", "Install packages")
      .callback([&](const rfl::cli::DynamicParseResult&) { ran = "install"; });
  root.add_subcommand("create", "Create environment");
  root.add_subcommand("list", "List packages");
  root.add_subcommand("info", "Info");
  root.add_subcommand("config", "Config");
  root.add_subcommand("activate", "Activate");
  root.add_subcommand("remove", "Remove packages").alias("uninstall");

  auto& run_cmd_node = root.add_subcommand("run", "Run in env");
  run_cmd_node.options().options_first = true;
  run_cmd_node.options().add_value("name", "n");
  run_cmd_node.callback([&](const rfl::cli::DynamicParseResult& _r) {
    ran = "run";
    run_cmd = _r.positional;
  });

  auto& rq = root.add_subcommand("repoquery", "Query packages");
  rq.require_subcommand(true);
  rq.add_subcommand("search", "Search");
  rq.add_subcommand("depends", "Depends");
  rq.add_subcommand("whoneeds", "Reverse depends")
      .callback([&](const rfl::cli::DynamicParseResult&) {
        ran = "repoquery.whoneeds";
      });

  // Exact + alias dispatch
  const auto ok1 = root.dispatch({"remove"});
  ASSERT_TRUE(ok1) << ok1.error().what();
  EXPECT_EQ(ok1->command_path, (std::vector<std::string>{"remove"}));

  const auto ok2 = root.dispatch({"uninstall"});
  ASSERT_TRUE(ok2) << ok2.error().what();
  EXPECT_EQ(ok2->command_path, (std::vector<std::string>{"remove"}));

  // Nested subcommand dispatch
  const auto ok3 = root.dispatch({"repoquery", "whoneeds", "openmp"});
  ASSERT_TRUE(ok3) << ok3.error().what();
  EXPECT_EQ(ok3->command_path,
            (std::vector<std::string>{"repoquery", "whoneeds"}));
  EXPECT_EQ(ran, "repoquery.whoneeds");
  EXPECT_EQ(ok3->parsed.positional, (std::vector<std::string>{"openmp"}));

  // Prefix command (`micromamba run -n myenv python --version`)
  const auto ok4 =
      root.dispatch({"run", "-n", "myenv", "python", "--version", "-c", "1"});
  ASSERT_TRUE(ok4) << ok4.error().what();
  EXPECT_EQ(ok4->parsed.values.at("name"), "myenv");
  EXPECT_EQ(run_cmd,
            (std::vector<std::string>{"python", "--version", "-c", "1"}));

  // Micromamba suggestion cases (missing, transposed, truncated, alias, nested)
  const std::vector<std::pair<std::string, std::string>> cases = {
      {"instal", "install"},     {"intsall", "install"},
      {"lst", "list"},           {"uninstal", "uninstall"},
      {"creat", "create"},       {"remov", "remove"},
      {"confog", "config"},      {"lsit", "list"},
      {"inof", "info"},          {"ifno", "info"},
      {"rnu", "run"},            {"actiavte", "activate"},
      {"reqoquery", "repoquery"}, {"activ", "activate"},
  };
  for (const auto& [typo, expected] : cases) {
    const auto err = root.dispatch({typo});
    ASSERT_FALSE(err) << "Expected error for typo: " << typo;
    const std::string msg = err.error().what();
    EXPECT_NE(msg.find("Did you mean '" + expected + "'?"), std::string::npos)
        << "typo=" << typo << " msg=" << msg;
    EXPECT_LT(msg.find("Did you mean"), msg.find("Run with --help"));
  }

  // Nested subcommand typo
  const auto nested_err = root.dispatch({"repoquery", "whoneds", "openmp"});
  ASSERT_FALSE(nested_err);
  EXPECT_NE(nested_err.error().what().find("Did you mean 'whoneeds'?"),
            std::string::npos);

  // Nonsense produces no false suggestion
  for (const auto& junk : std::vector<std::string>{"zzzzzzzz", "xq", "qwerty"}) {
    const auto err = root.dispatch({junk});
    ASSERT_FALSE(err);
    EXPECT_EQ(err.error().what().find("Did you mean"), std::string::npos)
        << "junk=" << junk << " msg=" << err.error().what();
  }
}

}  // namespace test_subcommands_suggestions_enums
