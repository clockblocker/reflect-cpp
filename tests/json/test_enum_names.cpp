#include <gtest/gtest.h>

#include <array>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <rfl/json.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace test_enum_names {

enum class OptimizationLevel {
  no_opt,
  size_opt,
  speed_opt,
  cxx_standard,
};

enum class OutputMode {
  plain_text,
  json_pretty,
  xml_compact,
};

enum class CustomPrefixEnum {
  fast_path,
  slow_path,
};

struct CompilerConfig {
  OptimizationLevel opt = OptimizationLevel::speed_opt;
  OutputMode output = OutputMode::plain_text;
  CustomPrefixEnum mode = CustomPrefixEnum::fast_path;
};

struct CliEnumConfig {
  rfl::DefaultVal<OptimizationLevel> opt = OptimizationLevel::speed_opt;
  rfl::DefaultVal<OutputMode> output = OutputMode::json_pretty;
};

enum class HttpStatus {
  ok = 200,
  not_found = 404,
};

struct StatusPayload {
  HttpStatus status = HttpStatus::ok;
};

inline constexpr std::array<std::string_view, 3> kCompressionModes = {
    "none", "fast", "max-compression"};
using CompressionLiteral = rfl::literal_from_table_t<kCompressionModes>;

struct ArchiveSettings {
  CompressionLiteral mode = CompressionLiteral::make<"max-compression">();
};

}  // namespace test_enum_names

namespace rfl {

// Combine snake_to_kebab transform with an explicit per-enumerator override
// (for identifiers like C++ keywords or non-identifier wire names like "c++20").
template <>
struct EnumNames<test_enum_names::OptimizationLevel> {
  static constexpr EnumNameTransform transform =
      EnumNameTransform::snake_to_kebab;
  static constexpr std::array<
      EnumOverride<test_enum_names::OptimizationLevel>, 1>
      overrides = {{
          {test_enum_names::OptimizationLevel::cxx_standard, "c++20"},
      }};
};

// Custom transform function.
template <>
struct EnumNames<test_enum_names::CustomPrefixEnum> {
  static std::string transform(const std::string_view _raw) {
    return "x-" + apply_enum_name_transform(_raw, EnumNameTransform::snake_to_kebab);
  }
};

// Out-of-range enum overrides (outside default enchantum range).
template <>
struct EnumNames<test_enum_names::HttpStatus> {
  static constexpr std::array<EnumOverride<test_enum_names::HttpStatus>, 2>
      overrides = {{
          {test_enum_names::HttpStatus::ok, "ok"},
          {test_enum_names::HttpStatus::not_found, "not-found"},
      }};
};

}  // namespace rfl

namespace test_enum_names {

TEST(json, test_enum_names_trait_overrides_and_transform) {
  EXPECT_EQ(rfl::enum_to_string(OptimizationLevel::no_opt), "no-opt");
  EXPECT_EQ(rfl::enum_to_string(OptimizationLevel::size_opt), "size-opt");
  EXPECT_EQ(rfl::enum_to_string(OptimizationLevel::speed_opt), "speed-opt");
  EXPECT_EQ(rfl::enum_to_string(OptimizationLevel::cxx_standard), "c++20");

  ASSERT_EQ(*rfl::string_to_enum<OptimizationLevel>("no-opt"),
            OptimizationLevel::no_opt);
  ASSERT_EQ(*rfl::string_to_enum<OptimizationLevel>("c++20"),
            OptimizationLevel::cxx_standard);
  // Raw C++ identifier should no longer match when transformed.
  EXPECT_FALSE(rfl::string_to_enum<OptimizationLevel>("no_opt"));
  EXPECT_FALSE(rfl::string_to_enum<OptimizationLevel>("cxx_standard"));

  // Custom transform function.
  EXPECT_EQ(rfl::enum_to_string(CustomPrefixEnum::fast_path), "x-fast-path");
  ASSERT_EQ(*rfl::string_to_enum<CustomPrefixEnum>("x-slow-path"),
            CustomPrefixEnum::slow_path);
}

TEST(json, test_enum_names_processor_transforms) {
  // OutputMode has no EnumNames<OutputMode> specialization; processor applies.
  const CompilerConfig cfg{
      .opt = OptimizationLevel::cxx_standard,
      .output = OutputMode::json_pretty,
      .mode = CustomPrefixEnum::slow_path,
  };

  const auto kebab_json = rfl::json::write<rfl::EnumSnakeCaseToKebabCase>(cfg);
  EXPECT_EQ(
      kebab_json,
      R"({"opt":"c++20","output":"json-pretty","mode":"x-slow-path"})");

  const auto parsed =
      rfl::json::read<CompilerConfig, rfl::EnumSnakeCaseToKebabCase>(kebab_json);
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed->opt, OptimizationLevel::cxx_standard);
  EXPECT_EQ(parsed->output, OutputMode::json_pretty);
  EXPECT_EQ(parsed->mode, CustomPrefixEnum::slow_path);

  // CamelCase and UpperCase processors.
  const auto camel_json = rfl::json::write<rfl::EnumSnakeCaseToCamelCase>(cfg);
  EXPECT_NE(camel_json.find(R"("output":"jsonPretty")"), std::string::npos)
      << camel_json;

  const auto pascal_json = rfl::json::write<rfl::EnumSnakeCaseToPascalCase>(cfg);
  EXPECT_NE(pascal_json.find(R"("output":"JsonPretty")"), std::string::npos)
      << pascal_json;

  const auto upper_json = rfl::json::write<rfl::EnumToUpperCase>(cfg);
  EXPECT_NE(upper_json.find(R"("output":"JSON_PRETTY")"), std::string::npos)
      << upper_json;
}

TEST(json, test_enum_names_json_schema) {
  const auto schema =
      rfl::json::to_schema<CompilerConfig, rfl::EnumSnakeCaseToKebabCase>();
  EXPECT_NE(schema.find(R"("enum":["no-opt","size-opt","speed-opt","c++20"])"),
            std::string::npos)
      << schema;
  EXPECT_NE(schema.find(R"("enum":["plain-text","json-pretty","xml-compact"])"),
            std::string::npos)
      << schema;
  EXPECT_NE(schema.find(R"("enum":["x-fast-path","x-slow-path"])"),
            std::string::npos)
      << schema;
}

TEST(json, test_enum_names_cli_read) {
  const std::vector<std::string> args = {
      "--opt=c++20", "--output=xml-compact"};
  const auto parsed =
      rfl::cli::read<CliEnumConfig, rfl::EnumSnakeCaseToKebabCase>(args);
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed->opt.value(), OptimizationLevel::cxx_standard);
  EXPECT_EQ(parsed->output.value(), OutputMode::xml_compact);
}

TEST(json, test_enum_names_out_of_range_overrides_in_schema_and_errors) {
  const auto names = rfl::get_enum_wire_names<HttpStatus>();
  ASSERT_EQ(names.size(), 2u);
  EXPECT_EQ(names[0].first, "ok");
  EXPECT_EQ(names[1].first, "not-found");

  const auto schema = rfl::json::to_schema<StatusPayload>();
  EXPECT_NE(schema.find(R"("enum":["ok","not-found"])"), std::string::npos)
      << schema;

  const auto parsed = rfl::json::read<StatusPayload>(R"({"status":"not-found"})");
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed->status, HttpStatus::not_found);

  const auto bad = rfl::string_to_enum<HttpStatus, true>("bogus");
  ASSERT_FALSE(bad);
  EXPECT_NE(bad.error().what().find("ok, not-found"), std::string::npos)
      << bad.error().what();
}

TEST(json, test_literal_from_table_t) {
  ArchiveSettings s;
  EXPECT_EQ(rfl::json::write(s), R"({"mode":"max-compression"})");

  const auto parsed =
      rfl::json::read<ArchiveSettings>(R"({"mode":"fast"})");
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed->mode.str(), "fast");

  const auto bad = rfl::json::read<ArchiveSettings>(R"({"mode":"bogus"})");
  EXPECT_FALSE(bad);

  const auto schema = rfl::json::to_schema<ArchiveSettings>();
  EXPECT_NE(schema.find(R"("enum":["none","fast","max-compression"])"),
            std::string::npos)
      << schema;
}

}  // namespace test_enum_names
