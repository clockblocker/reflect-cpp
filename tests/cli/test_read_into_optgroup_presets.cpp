#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_read_into_optgroup_presets {

enum class NormMode { l1_root, l2 };

struct ServerConfig {
  std::string host = "127.0.0.1";
  int port = 8080;
  bool tls = false;
};

// Plain unmodified domain struct (zero rfl::cli::Opt wrappers).
struct DomainConfig {
  int max_features = 8192;
  double peak_threshold = 0.02;
  bool upright = false;
  std::filesystem::path output_dir = "/default/out";
  NormMode norm = NormMode::l1_root;
  ServerConfig server;
  std::optional<ServerConfig> backup;
};

TEST(cli, read_into_plain_domain_struct_preserves_untouched_fields) {
  DomainConfig cfg;
  cfg.max_features = 4096;               // e.g. from ModifyForLowQuality()
  cfg.server.host = "prod.internal";     // e.g. loaded from YAML/INI
  cfg.server.port = 9000;
  cfg.server.tls = true;
  cfg.output_dir = "/var/data/preloaded";

  const auto res =
      rfl::cli::read_into<rfl::cli::GnuStyle, rfl::cli::CaseInsensitiveEnums>(
          cfg, {"--server.port", "8443", "--upright", "--norm", "L2",
                "--backup.port", "9443"});
  ASSERT_TRUE(res) << res.error().what();

  // Untouched fields keep their pre-existing values.
  EXPECT_EQ(cfg.max_features, 4096);
  EXPECT_DOUBLE_EQ(cfg.peak_threshold, 0.02);
  EXPECT_EQ(cfg.output_dir, std::filesystem::path("/var/data/preloaded"));
  EXPECT_EQ(cfg.server.host, "prod.internal");
  EXPECT_TRUE(cfg.server.tls);

  // Provided fields are overlaid in place.
  EXPECT_EQ(cfg.server.port, 8443);
  EXPECT_TRUE(cfg.upright);
  EXPECT_EQ(cfg.norm, NormMode::l2);
  ASSERT_TRUE(cfg.backup.has_value());
  EXPECT_EQ(cfg.backup->host, "127.0.0.1");
  EXPECT_EQ(cfg.backup->port, 9443);

  EXPECT_TRUE(res->was_provided("server.port"));
  EXPECT_TRUE(res->was_provided("upright"));
  EXPECT_TRUE(res->was_provided("norm"));
  EXPECT_FALSE(res->was_provided("server.host"));
  EXPECT_FALSE(res->was_provided("max-features"));

  // Overlay via argc/argv and filesystem::path.
  const char* argv[] = {"prog", "--output-dir", "/tmp/cli_override"};
  const auto res2 = rfl::cli::read_into<rfl::cli::GnuStyle>(cfg, 3, argv);
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_EQ(cfg.output_dir, std::filesystem::path("/tmp/cli_override"));
  EXPECT_EQ(cfg.server.port, 8443);  // Still preserved from first pass!

  // Unknown option rejected by GnuStyle (NoExtraFields).
  const auto bad =
      rfl::cli::read_into<rfl::cli::GnuStyle>(cfg, {"--does-not-exist"});
  EXPECT_FALSE(bad);
}

struct OutputOpts {
  rfl::cli::Arg<std::optional<std::string>, rfl::cli::attr::Short<"o">,
                rfl::cli::attr::ValueName<"path">,
                rfl::cli::attr::Desc<"Output path">>
      output_dir;
  rfl::cli::Flag<rfl::cli::attr::Short<"O">,
                 rfl::cli::attr::Desc<"Overwrite existing files">>
      overwrite = false;
  rfl::cli::Flag<rfl::cli::attr::Group<"Experimental">,
                 rfl::cli::attr::Desc<"Override group on single field">>
      experimental_format = false;
};

struct InputOpts {
  rfl::cli::Arg<std::optional<std::string>, rfl::cli::attr::Short<"F">,
                rfl::cli::attr::Desc<"Input format">>
      input_format;
};

struct GroupedCli {
  rfl::cli::OptGroup<"Input Options", InputOpts> input;
  rfl::cli::OptGroup<"Output Options", OutputOpts> output;
  rfl::cli::Flag<rfl::cli::attr::Short<"v">, rfl::cli::attr::Desc<"Verbose">>
      verbose = false;
};

TEST(cli, opt_group_flattens_and_propagates_usage_groups) {
  const auto parsed = rfl::cli::read<GroupedCli, rfl::cli::GnuLastWins>(
      {"-F", "pbf", "-o", "/tmp/a", "-o", "/tmp/b", "-O", "-v"});
  ASSERT_TRUE(parsed) << parsed.error().what();

  // OptGroup inherits Flatten's operator-> / operator* and Opt's operator bool()
  EXPECT_TRUE(static_cast<bool>(parsed->input->input_format));
  EXPECT_EQ(*parsed->input->input_format, "pbf");
  EXPECT_EQ(parsed->output->output_dir->value(), "/tmp/b");  // GnuLastWins
  EXPECT_TRUE(static_cast<bool>(parsed->output->overwrite));
  EXPECT_TRUE(static_cast<bool>(parsed->verbose));

  // read_into also works seamlessly through OptGroup!
  GroupedCli target = *parsed;
  const auto ov =
      rfl::cli::read_into<rfl::cli::GnuLastWins>(target, {"-F", "xml"});
  ASSERT_TRUE(ov) << ov.error().what();
  EXPECT_EQ(*target.input->input_format, "xml");
  EXPECT_EQ(target.output->output_dir->value(), "/tmp/b");  // Preserved!

  // Usage renders inherited groups and respects explicit per-field override.
  const auto help_res = rfl::cli::usage<GroupedCli, rfl::cli::GnuLastWins>(
      "tool", rfl::cli::UsageOptions{.detailed = true});
  ASSERT_TRUE(help_res) << help_res.error().what();
  const auto& help = *help_res;
  const auto input_hdr = help.find("Input Options:");
  const auto output_hdr = help.find("Output Options:");
  const auto exp_hdr = help.find("Experimental:");
  ASSERT_NE(input_hdr, std::string::npos) << help;
  ASSERT_NE(output_hdr, std::string::npos) << help;
  ASSERT_NE(exp_hdr, std::string::npos) << help;

  EXPECT_LT(input_hdr, output_hdr);
  EXPECT_LT(output_hdr, exp_hdr);
  const auto input_opt_pos = help.find("--input-format", input_hdr);
  const auto output_dir_pos = help.find("--output-dir", output_hdr);
  const auto overwrite_pos = help.find("--overwrite", output_hdr);
  const auto exp_opt_pos = help.find("--experimental-format", exp_hdr);
  ASSERT_NE(input_opt_pos, std::string::npos) << help;
  EXPECT_LT(input_opt_pos, output_hdr);
  ASSERT_NE(output_dir_pos, std::string::npos) << help;
  EXPECT_LT(output_dir_pos, exp_hdr);
  ASSERT_NE(overwrite_pos, std::string::npos) << help;
  EXPECT_LT(overwrite_pos, exp_hdr);
  ASSERT_NE(exp_opt_pos, std::string::npos) << help;
}

struct RunnerOpts {
  rfl::cli::Flag<rfl::cli::attr::Short<"q">> quiet = false;
  rfl::cli::Arg<std::vector<std::string>, rfl::cli::attr::Positional> command;
};

TEST(cli, posix_strict_preset) {
  const auto r = rfl::cli::read<RunnerOpts, rfl::cli::PosixStrict>(
      {"-q", "python", "--version", "-c", "1"});
  ASSERT_TRUE(r) << r.error().what();
  EXPECT_TRUE(static_cast<bool>(r->quiet));
  EXPECT_EQ(*r->command,
            (std::vector<std::string>{"python", "--version", "-c", "1"}));
}

}  // namespace test_read_into_optgroup_presets
