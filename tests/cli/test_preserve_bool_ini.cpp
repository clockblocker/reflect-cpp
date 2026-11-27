#include <gtest/gtest.h>

#include <rfl/cli.hpp>
#include <sstream>
#include <string>

namespace test_preserve_bool_ini {

using rfl::cli::Opt;
using namespace rfl::cli::attr;

struct SiftExtraction {
  Opt<bool> use_gpu = true;
  Opt<int> max_image_size = 3200;
};

struct ColmapStyleCli {
  Opt<std::string> database_path{};
  SiftExtraction SiftExtraction{};
};

using ColmapProcs = rfl::Processors<rfl::DefaultIfMissing, rfl::NoExtraFields,
                                    rfl::cli::PreserveNames,
                                    rfl::cli::SeparateOptionValues,
                                    rfl::cli::BoolValueTokens,
                                    rfl::cli::LastValueWins>;

TEST(cli, preserve_names_and_bool_value_tokens) {
  const auto res = rfl::cli::read<ColmapStyleCli, ColmapProcs>(
      {"--database_path", "/tmp/db.sqlite", "--SiftExtraction.use_gpu", "0",
       "--SiftExtraction.max_image_size", "1600"});
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->database_path.value(), "/tmp/db.sqlite");
  EXPECT_FALSE(res->SiftExtraction.use_gpu.value());
  EXPECT_EQ(res->SiftExtraction.max_image_size.value(), 1600);

  // Non-bool token after flag is not consumed; bool=1 / true works too.
  const auto res2 = rfl::cli::read<ColmapStyleCli, ColmapProcs>(
      {"--database_path=/tmp/db2", "--SiftExtraction.use_gpu", "1"});
  ASSERT_TRUE(res2.has_value()) << res2.error().what();
  EXPECT_TRUE(res2->SiftExtraction.use_gpu.value());

  // Usage preserves snake_case and PascalCase.snake_case verbatim.
  const auto help = rfl::cli::usage<ColmapStyleCli, ColmapProcs>("colmap", true);
  ASSERT_TRUE(help.has_value()) << help.error().what();
  EXPECT_NE(help->find("--database_path"), std::string::npos);
  EXPECT_NE(help->find("--SiftExtraction.use_gpu"), std::string::npos);
}

TEST(cli, dynamic_options_ini_and_required_and_usage) {
  rfl::cli::DynamicOptions opts;
  opts.bool_value_tokens = true;
  opts.add_value("database_path", "", {.required = true, .description = "Path to DB"})
      .add_flag("SiftExtraction.use_gpu", "", {.default_hint = "1", .description = "Use GPU"})
      .add_value("SiftExtraction.max_num_features", "", {.default_hint = "8192"});

  std::istringstream ini(
      "# comment\n"
      "database_path = /ini/db.sqlite\n"
      "unknown_top = 1\n"
      "[SiftExtraction]\n"
      "use_gpu = 0\n"
      "max_num_features = 4096\n"
      "unknown_nested = xyz\n");

  const auto ini_res = opts.parse_ini(ini, /*allow_unregistered=*/true);
  ASSERT_TRUE(ini_res.has_value()) << ini_res.error().what();
  EXPECT_EQ(ini_res->values.at("database_path"), "/ini/db.sqlite");
  EXPECT_EQ(ini_res->values.at("SiftExtraction.use_gpu"), "0");
  EXPECT_EQ(ini_res->values.at("SiftExtraction.max_num_features"), "4096");
  ASSERT_EQ(ini_res->unregistered.size(), 2u);
  EXPECT_EQ(ini_res->unregistered[0], "unknown_top");
  EXPECT_EQ(ini_res->unregistered[1], "SiftExtraction.unknown_nested");

  // CLI overrides INI.
  const auto cli_res = opts.parse({"--SiftExtraction.use_gpu", "1"});
  ASSERT_TRUE(cli_res.has_value()) << cli_res.error().what();
  auto merged = ini_res->values;
  for (const auto& [k, v] : cli_res->values) {
    merged[k] = v;
  }
  EXPECT_EQ(merged.at("database_path"), "/ini/db.sqlite");
  EXPECT_EQ(merged.at("SiftExtraction.use_gpu"), "1");
  EXPECT_TRUE(opts.check_required(merged).has_value());

  // Missing required option fails check_required.
  std::map<std::string, std::string> empty;
  EXPECT_FALSE(opts.check_required(empty).has_value());

  // Usage renders metadata.
  const auto help = opts.usage("Options:");
  EXPECT_NE(help.find("--database_path"), std::string::npos);
  EXPECT_NE(help.find("Path to DB"), std::string::npos);
  EXPECT_NE(help.find("(=8192)"), std::string::npos);
}

}  // namespace test_preserve_bool_ini
