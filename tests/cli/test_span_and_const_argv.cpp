#include <gtest/gtest.h>

#include <array>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <span>
#include <string>
#include <vector>

namespace test_span_and_const_argv {

struct Config {
  std::string host;
  int port;
};

struct PositionalConfig {
  rfl::Positional<std::string> input_file;
  bool verbose;
};

struct OptionalConfig {
  std::optional<std::string> host;
};

TEST(cli, test_const_argv) {
  const char* argv[] = {"program", "--host=localhost", "--port=8080"};
  const auto result = rfl::cli::read<Config>(3, argv);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().host, "localhost");
  EXPECT_EQ(result.value().port, 8080);
}

TEST(cli, test_const_pointer_to_const_argv) {
  const char* const argv[] = {"program", "--host=localhost", "--port=8080"};
  const auto result = rfl::cli::read<Config>(3, argv);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().port, 8080);
}

TEST(cli, test_const_argv_still_skips_program_name) {
  // argv[0] is the program name, even if it looks like a positional.
  const char* argv[] = {"in.txt", "out.txt", "--verbose"};
  const auto result = rfl::cli::read<PositionalConfig>(3, argv);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input_file(), "out.txt");
}

TEST(cli, test_const_argv_invalid_argc) {
  const char* argv[] = {"program"};
  EXPECT_FALSE(rfl::cli::read<OptionalConfig>(-1, argv));
}

TEST(cli, test_const_wide_argv) {
  const wchar_t* argv[] = {L"program", L"--host=localhost", L"--port=8080"};
  const auto result = rfl::cli::read<Config>(3, argv);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().host, "localhost");
  EXPECT_EQ(result.value().port, 8080);
}

TEST(cli, test_span_from_vector) {
  const std::vector<const char*> args = {"--host=localhost", "--port=8080"};
  const auto result = rfl::cli::read<Config>(args);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().host, "localhost");
  EXPECT_EQ(result.value().port, 8080);
}

TEST(cli, test_span_from_array) {
  const std::array<const char*, 2> args = {"--host=localhost", "--port=8080"};
  const auto result = rfl::cli::read<Config>(args);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().port, 8080);
}

TEST(cli, test_span_does_not_skip_first_element) {
  const char* args[] = {"in.txt", "--verbose"};
  const auto result = rfl::cli::read<PositionalConfig>(args);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input_file(), "in.txt");
  EXPECT_TRUE(result.value().verbose);
}

TEST(cli, test_span_first_element_is_an_option) {
  // With argc/argv, "--host=localhost" would be dropped as the program name.
  const char* args[] = {"--host=localhost", "--port=8080"};
  const auto result = rfl::cli::read<Config>(std::span(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().host, "localhost");
}

TEST(cli, test_span_over_rest_of_mutable_argv) {
  // The program handles "program serve" itself and passes on the rest.
  char program[] = "program";
  char subcommand[] = "serve";
  char host[] = "--host=localhost";
  char port[] = "--port=8080";
  char* argv[] = {program, subcommand, host, port};
  const int argc = 4;
  const auto result = rfl::cli::read<Config>(std::span(argv + 2, argc - 2));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().host, "localhost");
  EXPECT_EQ(result.value().port, 8080);
}

TEST(cli, test_span_empty) {
  const auto result =
      rfl::cli::read<OptionalConfig>(std::span<const char* const>());
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_FALSE(result.value().host);
}

TEST(cli, test_span_missing_field) {
  const std::vector<const char*> args = {"--host=localhost"};
  EXPECT_FALSE(rfl::cli::read<Config>(args));
}

TEST(cli, test_parse_argv_span) {
  const char* args[] = {"first", "--host=localhost", "-p", "8080"};
  const auto result = rfl::cli::parse_argv(std::span(args));
  ASSERT_TRUE(result) << result.error().what();
  ASSERT_EQ(result.value().positional.size(), 1);
  EXPECT_EQ(result.value().positional[0], "first");
  EXPECT_EQ(result.value().named.at("host"), "localhost");
  EXPECT_EQ(result.value().short_args.at("p"), "8080");
}

TEST(cli, test_parse_argv_const_argv_skips_program_name) {
  const char* argv[] = {"first", "second"};
  const auto result = rfl::cli::parse_argv(2, argv);
  ASSERT_TRUE(result) << result.error().what();
  ASSERT_EQ(result.value().positional.size(), 1);
  EXPECT_EQ(result.value().positional[0], "second");
}

}  // namespace test_span_and_const_argv
