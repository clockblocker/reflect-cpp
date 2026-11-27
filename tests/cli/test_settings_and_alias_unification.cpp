#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

namespace test_settings_and_alias_unification {

struct CompilerCli {
  rfl::cli::Opt<std::string, rfl::cli::attr::Short<"o">,
                rfl::cli::attr::Alias<"output-directory">,
                rfl::cli::attr::Desc<"Output dir">>
      output_dir = "build";
  rfl::cli::Alias<"opt-level", rfl::DefaultVal<int>> optimize = 0;
  rfl::cli::Flag<rfl::cli::attr::Short<"v">> verbose = false;
  std::vector<std::string> include_paths;
  rfl::Positional<std::optional<std::vector<std::string>>> sources;
};

using CompilerApp = rfl::cli::App<"solc-demo", "0.8.28", "Solidity demo">;

TEST(cli, test_issue_99_settings_presets_and_runtime_modes) {
  // 1. Settings::gnu() supports separate values, pure flags, rejects unknown.
  const auto gnu_res = rfl::cli::read<CompilerCli, rfl::DefaultIfMissing>(
      {"-v", "--output-dir", "dist", "--optimize", "2", "a.sol", "b.sol"},
      rfl::cli::Settings::gnu());
  ASSERT_TRUE(gnu_res) << gnu_res.error().what();
  EXPECT_TRUE(gnu_res->verbose.value());
  EXPECT_EQ(gnu_res->output_dir.value(), "dist");
  EXPECT_EQ(gnu_res->optimize.value().value(), 2);
  ASSERT_TRUE(gnu_res->sources.value().has_value());
  EXPECT_EQ(*gnu_res->sources.value(),
            (std::vector<std::string>{"a.sol", "b.sol"}));

  // Unknown option rejected by Settings::gnu().
  const auto gnu_unknown = rfl::cli::read<CompilerCli, rfl::DefaultIfMissing>(
      {"--bogus=1"}, rfl::cli::Settings::gnu());
  EXPECT_FALSE(gnu_unknown);

  // Repeated scalar rejected by Settings::gnu(), accepted by Settings::gnu_last_wins().
  const auto gnu_dup = rfl::cli::read<CompilerCli, rfl::DefaultIfMissing>(
      {"--output-dir", "a", "--output-dir", "b"}, rfl::cli::Settings::gnu());
  EXPECT_FALSE(gnu_dup);

  const auto last_wins = rfl::cli::read<CompilerCli, rfl::DefaultIfMissing>(
      {"--output-dir", "a", "--output-dir", "b"},
      rfl::cli::Settings::gnu_last_wins());
  ASSERT_TRUE(last_wins) << last_wins.error().what();
  EXPECT_EQ(last_wins->output_dir.value(), "b");

  // 2. Settings::posix_strict() stops option parsing at first positional.
  const auto posix = rfl::cli::read<CompilerCli, rfl::DefaultIfMissing>(
      {"-v", "main.sol", "--not-an-option"},
      rfl::cli::Settings::posix_strict());
  ASSERT_TRUE(posix) << posix.error().what();
  EXPECT_TRUE(posix->verbose.value());
  ASSERT_TRUE(posix->sources.value().has_value());
  EXPECT_EQ(*posix->sources.value(),
            (std::vector<std::string>{"main.sol", "--not-an-option"}));

  // 3. Settings::single_dash() accepts single-dash long options.
  const auto sd = rfl::cli::read<CompilerCli, rfl::DefaultIfMissing>(
      {"-output-dir", "out", "-optimize=3"}, rfl::cli::Settings::single_dash());
  ASSERT_TRUE(sd) << sd.error().what();
  EXPECT_EQ(sd->output_dir.value(), "out");
  EXPECT_EQ(sd->optimize.value().value(), 3);

  // 4. Custom list_delimiter & allow_abbreviations at runtime without extra template instantiations.
  auto custom = rfl::cli::Settings::gnu();
  custom.list_delimiter = ':';
  custom.allow_abbreviations = true;
  const auto custom_res = rfl::cli::read<CompilerCli, rfl::DefaultIfMissing>(
      {"--inc", "a:b:c"}, custom);
  ASSERT_TRUE(custom_res) << custom_res.error().what();
  EXPECT_EQ(custom_res->include_paths,
            (std::vector<std::string>{"a", "b", "c"}));

  // Invalid Settings combination (keep_empty_items without list_delimiter) fails cleanly.
  auto bad_settings = rfl::cli::Settings::gnu();
  bad_settings.list_delimiter = std::nullopt;
  bad_settings.keep_empty_items = true;
  const auto bad_res =
      rfl::cli::read<CompilerCli, rfl::DefaultIfMissing>({}, bad_settings);
  EXPECT_FALSE(bad_res);

  // 5. Settings::from_processors<GnuLastWins>() matches Settings::gnu_last_wins().
  const auto from_proc =
      rfl::cli::Settings::from_processors<rfl::cli::GnuLastWins>();
  EXPECT_TRUE(from_proc.separate_values);
  EXPECT_TRUE(from_proc.pure_flags);
  EXPECT_TRUE(from_proc.reject_unknown);
  EXPECT_TRUE(from_proc.last_value_wins);
}

TEST(cli, test_issue_99_settings_with_read_into_and_read_app_and_usage) {
  // read_into with runtime Settings preserves untouched fields and overlays alias.
  CompilerCli cfg;
  cfg.output_dir = "initial";
  cfg.optimize = 1;
  const auto overlay = rfl::cli::read_into(
      cfg, {"--opt-level", "3", "--output-directory", "next"},
      rfl::cli::Settings::gnu());
  ASSERT_TRUE(overlay) << overlay.error().what();
  EXPECT_EQ(cfg.output_dir.value(), "next");
  EXPECT_EQ(cfg.optimize.value().value(), 3);
  EXPECT_TRUE(overlay->was_provided("output-dir"));
  EXPECT_TRUE(overlay->was_provided("optimize"));

  // read_app with Settings handles --help using runtime Settings (bare pure flags, custom delimiter).
  auto settings = rfl::cli::Settings::gnu();
  settings.list_delimiter = ';';
  const auto app_help =
      rfl::cli::read_app<CompilerCli, CompilerApp, rfl::DefaultIfMissing>(
          {"--help"}, settings);
  ASSERT_TRUE(app_help) << app_help.error().what();
  const auto* info = std::get_if<rfl::cli::HelpOrVersion>(&*app_help);
  ASSERT_NE(info, nullptr);
  EXPECT_EQ(info->kind, rfl::cli::HelpOrVersion::Kind::help);
  EXPECT_NE(info->text.find("<v1;v2;...>"), std::string::npos) << info->text;
  EXPECT_NE(info->text.find("-v, --verbose\n"), std::string::npos) << info->text;
  EXPECT_NE(info->text.find("Default: 0."), std::string::npos) << info->text;
}

TEST(cli, test_issue_99_alias_reconciled_across_cli_json_and_usage) {
  // rfl::cli::Alias IS core rfl::Alias.
  static_assert(std::is_same_v<rfl::cli::Alias<"x", int>, rfl::Alias<"x", int>>);

  // rfl::cli::attr::Alias on Opt<...> AND rfl::cli::Alias BOTH work in JSON AND CLI!
  const auto from_json = rfl::json::read<CompilerCli, rfl::DefaultIfMissing>(
      R"({"output-directory":"json-out","opt-level":2,"verbose":true})");
  ASSERT_TRUE(from_json) << from_json.error().what();
  EXPECT_EQ(from_json->output_dir.value(), "json-out");
  EXPECT_EQ(from_json->optimize.value().value(), 2);
  EXPECT_TRUE(from_json->verbose.value());

  // Both aliases also appear cleanly in public rfl::cli::describe().
  const auto desc = rfl::cli::describe<CompilerCli, rfl::DefaultIfMissing>(
      "solc-demo", rfl::cli::Settings::gnu());
  ASSERT_TRUE(desc) << desc.error().what();
  bool saw_output_alias = false;
  bool saw_opt_alias = false;
  for (const auto& opt : desc->options) {
    if (opt.name == "output-dir") {
      EXPECT_EQ(opt.aliases, (std::vector<std::string>{"output-directory"}));
      saw_output_alias = true;
    }
    if (opt.name == "optimize") {
      EXPECT_EQ(opt.aliases, (std::vector<std::string>{"opt-level"}));
      EXPECT_EQ(opt.default_value, std::optional<std::string>("0"));
      saw_opt_alias = true;
    }
  }
  EXPECT_TRUE(saw_output_alias);
  EXPECT_TRUE(saw_opt_alias);

  // DynamicOptions::apply_settings() accepts the same Settings struct.
  rfl::cli::DynamicOptions dyn;
  dyn.apply_settings(rfl::cli::Settings::single_dash());
  dyn.add_value("optimize");
  const auto dyn_res = dyn.parse({"-optimize", "2"});
  ASSERT_TRUE(dyn_res) << dyn_res.error().what();
  EXPECT_EQ(dyn_res->values.at("optimize"), "2");
}

TEST(cli, test_issue_99_settings_overrides_no_extra_fields_and_braced_usage_opts) {
  // Settings{.reject_unknown = false} cleanly overrides compile-time GnuStyle (NoExtraFields).
  auto relaxed = rfl::cli::Settings::gnu();
  relaxed.reject_unknown = false;
  const auto ok = rfl::cli::read<CompilerCli, rfl::cli::GnuStyle>(
      {"--output-dir", "relaxed-out", "--unknown-flag=ignored"}, relaxed);
  ASSERT_TRUE(ok) << ok.error().what();
  EXPECT_EQ(ok->output_dir.value(), "relaxed-out");

  // Positional braced-init UsageOptions must remain completely unambiguous against Settings!
  const auto u1 = rfl::cli::usage<CompilerCli, rfl::DefaultIfMissing>(
      "solc-demo", {true, "Custom Header"});
  ASSERT_TRUE(u1) << u1.error().what();
  EXPECT_NE(u1->find("Custom Header"), std::string::npos);

  const auto d1 = rfl::cli::describe<CompilerCli, rfl::DefaultIfMissing>(
      "solc-demo", {});
  ASSERT_TRUE(d1) << d1.error().what();
  EXPECT_EQ(d1->name, "solc-demo");

  const auto a1 =
      rfl::cli::read_app<CompilerCli, CompilerApp, rfl::DefaultIfMissing>(
          "solc-demo", {"--help"}, {});
  ASSERT_TRUE(a1) << a1.error().what();
  EXPECT_NE(std::get_if<rfl::cli::HelpOrVersion>(&*a1), nullptr);

  const char* help_argv[] = {"solc-demo", "--help"};
  const auto a2 =
      rfl::cli::read_app<CompilerCli, CompilerApp, rfl::DefaultIfMissing>(
          2, help_argv, {false});
  ASSERT_TRUE(a2) << a2.error().what();
  EXPECT_NE(std::get_if<rfl::cli::HelpOrVersion>(&*a2), nullptr);

  // usage() with invalid Settings fails consistently with read()/describe().
  auto bad = rfl::cli::Settings::gnu();
  bad.list_delimiter = std::nullopt;
  bad.keep_empty_items = true;
  EXPECT_FALSE((rfl::cli::usage<CompilerCli, rfl::DefaultIfMissing>("solc", bad)));
}

}  // namespace test_settings_and_alias_unification
