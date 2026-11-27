#include <gtest/gtest.h>

#include <rfl/cli.hpp>
#include <string>

namespace test_preserve_names_and_bool_tokens {

using rfl::cli::Opt;

struct SiftExtraction {
  Opt<bool> use_gpu = true;
  Opt<int> max_image_size = 3200;
};

struct PreserveNamesCli {
  Opt<std::string> database_path{};
  SiftExtraction SiftExtraction{};
};

using PreserveProcs = rfl::Processors<rfl::DefaultIfMissing, rfl::NoExtraFields,
                                      rfl::cli::PreserveNames,
                                      rfl::cli::SeparateOptionValues,
                                      rfl::cli::BoolValueTokens,
                                      rfl::cli::LastValueWins>;

TEST(cli, preserve_names_and_bool_value_tokens) {
  const auto res = rfl::cli::read<PreserveNamesCli, PreserveProcs>(
      {"--database_path", "/tmp/db.sqlite", "--SiftExtraction.use_gpu", "0",
       "--SiftExtraction.max_image_size", "1600"});
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->database_path.value(), "/tmp/db.sqlite");
  EXPECT_FALSE(res->SiftExtraction.use_gpu.value());
  EXPECT_EQ(res->SiftExtraction.max_image_size.value(), 1600);

  // Explicit `=value` and separate boolean token `1` both work.
  const auto res2 = rfl::cli::read<PreserveNamesCli, PreserveProcs>(
      {"--database_path=/tmp/db2", "--SiftExtraction.use_gpu", "1"});
  ASSERT_TRUE(res2.has_value()) << res2.error().what();
  EXPECT_TRUE(res2->SiftExtraction.use_gpu.value());

  // Usage preserves snake_case and PascalCase.snake_case verbatim.
  const auto help = rfl::cli::usage<PreserveNamesCli, PreserveProcs>("app", true);
  ASSERT_TRUE(help.has_value()) << help.error().what();
  EXPECT_NE(help->find("--database_path"), std::string::npos);
  EXPECT_NE(help->find("--SiftExtraction.use_gpu"), std::string::npos);
}

}  // namespace test_preserve_names_and_bool_tokens
