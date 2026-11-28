#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <rfl/json.hpp>
#include <span>
#include <stdexcept>
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

enum class MixedCaseEnum {
  Fast_Path,
  Read,
};

enum class StringViewTransformEnum {
  first_item,
  second_item,
};

enum class SpanOverridesEnum {
  alpha_val,
  beta_val,
  gamma_val,
};

enum class DupOverrideEnum {
  first,
  second,
};

enum class ThrowingTransformEnum {
  ok,
};

enum class DescribedWireEnum {
  fast_mode,
  safe_mode,
};

enum UnscopedWireEnum : uint8_t {
  unscoped_one = 1,
  unscoped_two = 2,
};

enum class WideWireEnum : int64_t {
  small_val = 1,
  big_val = 1000000,
};

enum class WireFlags : uint32_t {
  none = 0,
  read_access = 1u << 0,
  write_access = 1u << 1,
  exec_access = 1u << 2,
};

inline WireFlags operator|(WireFlags _a, WireFlags _b) {
  return static_cast<WireFlags>(static_cast<uint32_t>(_a) |
                                static_cast<uint32_t>(_b));
}

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

struct DescribedPayload {
  DescribedWireEnum mode = DescribedWireEnum::fast_mode;
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

// Custom transform function returning std::string.
template <>
struct EnumNames<test_enum_names::CustomPrefixEnum> {
  static std::string transform(const std::string_view _raw) {
    return "x-" + apply_enum_name_transform(_raw, EnumNameTransform::snake_to_kebab);
  }
};

// Custom transform function returning std::string_view.
template <>
struct EnumNames<test_enum_names::StringViewTransformEnum> {
  static std::string_view transform(const std::string_view _raw) {
    return _raw == "first_item" ? std::string_view("one")
                                : std::string_view("two");
  }
};

// Borrowed-range function form of overrides() without transform (non-overridden
// enumerators still inherit processor transforms).
template <>
struct EnumNames<test_enum_names::SpanOverridesEnum> {
  static std::span<const EnumOverride<test_enum_names::SpanOverridesEnum>>
  overrides() {
    static constexpr std::array<
        EnumOverride<test_enum_names::SpanOverridesEnum>, 1>
        kOverrides = {{
            {test_enum_names::SpanOverridesEnum::alpha_val, "custom-alpha"},
        }};
    return kOverrides;
  }
};

// Duplicate override entries for the same enumerator: first entry wins everywhere.
template <>
struct EnumNames<test_enum_names::DupOverrideEnum> {
  static constexpr std::array<
      EnumOverride<test_enum_names::DupOverrideEnum>, 2>
      overrides = {{
          {test_enum_names::DupOverrideEnum::first, "primary"},
          {test_enum_names::DupOverrideEnum::first, "shadowed"},
      }};
};

// Throwing custom transform must return rfl::Error rather than terminating.
template <>
struct EnumNames<test_enum_names::ThrowingTransformEnum> {
  static std::string transform(const std::string_view) {
    throw std::runtime_error("transform boom");
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

template <>
struct EnumNames<test_enum_names::WideWireEnum> {
  static constexpr std::array<EnumOverride<test_enum_names::WideWireEnum>, 1>
      overrides = {{
          {test_enum_names::WideWireEnum::big_val, "one-million"},
      }};
};

template <>
struct config::enum_descriptions<test_enum_names::DescribedWireEnum> {
  static constexpr bool has_descriptions = true;
  static std::string_view get(const test_enum_names::DescribedWireEnum _e) {
    return _e == test_enum_names::DescribedWireEnum::fast_mode
               ? "Fast execution"
               : "Safe execution";
  }
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
  // Raw C++ identifier and wrong case should not match.
  EXPECT_FALSE(rfl::string_to_enum<OptimizationLevel>("no_opt"));
  EXPECT_FALSE(rfl::string_to_enum<OptimizationLevel>("NO-OPT"));
  EXPECT_FALSE(rfl::string_to_enum<OptimizationLevel>("cxx_standard"));

  // Custom transform function returning std::string and std::string_view.
  EXPECT_EQ(rfl::enum_to_string(CustomPrefixEnum::fast_path), "x-fast-path");
  ASSERT_EQ(*rfl::string_to_enum<CustomPrefixEnum>("x-slow-path"),
            CustomPrefixEnum::slow_path);

  EXPECT_EQ(rfl::enum_to_string(StringViewTransformEnum::first_item), "one");
  ASSERT_EQ(*rfl::string_to_enum<StringViewTransformEnum>("two"),
            StringViewTransformEnum::second_item);

  // Borrowed-range overrides() function form + processor fallback.
  EXPECT_EQ(rfl::enum_to_string(SpanOverridesEnum::alpha_val), "custom-alpha");
  EXPECT_EQ(rfl::enum_to_string(SpanOverridesEnum::beta_val), "beta_val");
  EXPECT_EQ((rfl::enum_to_string<SpanOverridesEnum,
                                 rfl::EnumSnakeCaseToKebabCase>(
                SpanOverridesEnum::alpha_val)),
            "custom-alpha");
  EXPECT_EQ((rfl::enum_to_string<SpanOverridesEnum,
                                 rfl::EnumSnakeCaseToKebabCase>(
                SpanOverridesEnum::beta_val)),
            "beta-val");
  ASSERT_EQ(*(rfl::string_to_enum<SpanOverridesEnum, true,
                                  rfl::EnumSnakeCaseToKebabCase>("beta-val")),
            SpanOverridesEnum::beta_val);

  // Duplicate override entries: first wins consistently across read/write/schema.
  EXPECT_EQ(rfl::enum_to_string(DupOverrideEnum::first), "primary");
  ASSERT_EQ((*(rfl::string_to_enum<DupOverrideEnum, true>("primary"))),
            DupOverrideEnum::first);
  EXPECT_FALSE((rfl::string_to_enum<DupOverrideEnum, true>("shadowed")));

  // Throwing custom transform returns rfl::Error instead of terminating.
  const auto thrown =
      rfl::json::read<ThrowingTransformEnum>(R"("ok")");
  EXPECT_FALSE(thrown);
  EXPECT_NE(thrown.error().what().find("transform boom"), std::string::npos);
}

TEST(json, test_enum_names_processor_transforms_and_nested_processors) {
  static_assert(rfl::internal::enum_name_transform_v<
                    rfl::EnumSnakeCaseToKebabCase> ==
                rfl::EnumNameTransform::snake_to_kebab);
  static_assert(
      rfl::internal::enum_name_transform_v<
          rfl::Processors<rfl::Processors<rfl::EnumSnakeCaseToKebabCase>>> ==
      rfl::EnumNameTransform::snake_to_kebab);

  // Direct calls with bare processor and nested Processors<...>.
  EXPECT_EQ((rfl::enum_to_string<OutputMode, rfl::EnumSnakeCaseToKebabCase>(
                OutputMode::json_pretty)),
            "json-pretty");
  const auto direct_wire =
      rfl::get_enum_wire_names<OutputMode, rfl::EnumSnakeCaseToKebabCase>();
  ASSERT_EQ(direct_wire.size(), 3u);
  EXPECT_EQ(direct_wire[1].first, "json-pretty");

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

  // CamelCase lowercases the leading character too.
  EXPECT_EQ((rfl::enum_to_string<MixedCaseEnum, rfl::EnumSnakeCaseToCamelCase>(
                MixedCaseEnum::Fast_Path)),
            "fastPath");
  EXPECT_EQ((rfl::enum_to_string<MixedCaseEnum, rfl::EnumSnakeCaseToCamelCase>(
                MixedCaseEnum::Read)),
            "read");
  EXPECT_EQ((rfl::enum_to_string<MixedCaseEnum, rfl::EnumToLowerCase>(
                MixedCaseEnum::Fast_Path)),
            "fast_path");

  const auto camel_json = rfl::json::write<rfl::EnumSnakeCaseToCamelCase>(cfg);
  EXPECT_NE(camel_json.find(R"("output":"jsonPretty")"), std::string::npos)
      << camel_json;

  const auto pascal_json = rfl::json::write<rfl::EnumSnakeCaseToPascalCase>(cfg);
  EXPECT_NE(pascal_json.find(R"("output":"JsonPretty")"), std::string::npos)
      << pascal_json;

  const auto upper_json = rfl::json::write<rfl::EnumToUpperCase>(cfg);
  EXPECT_NE(upper_json.find(R"("output":"JSON_PRETTY")"), std::string::npos)
      << upper_json;

  // EnumNamesOnly rejects numeric strings, while UnderlyingEnums uses integers.
  EXPECT_FALSE(
      (rfl::json::read<OutputMode, rfl::EnumSnakeCaseToKebabCase,
                       rfl::EnumNamesOnly>(R"("1")")));
  EXPECT_EQ(
      (rfl::json::write<rfl::EnumSnakeCaseToKebabCase, rfl::UnderlyingEnums>(
          OutputMode::json_pretty)),
      "1");
}

TEST(json, test_enum_names_json_schema_and_default_val) {
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

  // DefaultVal<Enum> must emit the transformed wire name matching its enum schema.
  const auto default_schema =
      rfl::json::to_schema<CliEnumConfig, rfl::EnumSnakeCaseToKebabCase>();
  EXPECT_NE(default_schema.find(
                R"("enum":["plain-text","json-pretty","xml-compact"])"),
            std::string::npos)
      << default_schema;
  EXPECT_NE(default_schema.find(R"("default":"json-pretty")"),
            std::string::npos)
      << default_schema;
  EXPECT_NE(default_schema.find(R"("default":"speed-opt")"),
            std::string::npos)
      << default_schema;

  // DescribedLiteral respects enum wire transforms too.
  const auto desc_schema =
      rfl::json::to_schema<DescribedPayload, rfl::EnumSnakeCaseToKebabCase>();
  EXPECT_NE(desc_schema.find(R"("fast-mode")"), std::string::npos)
      << desc_schema;
  EXPECT_NE(desc_schema.find(R"("Fast execution")"), std::string::npos)
      << desc_schema;
}

TEST(json, test_enum_names_flags_unscoped_and_wide_underlying) {
  // Flag enum with TransformEnumNames: zero flag and combined flags.
  EXPECT_EQ((rfl::enum_to_string<WireFlags, rfl::EnumSnakeCaseToKebabCase>(
                WireFlags::none)),
            "none");
  const auto combined = WireFlags::read_access | WireFlags::exec_access;
  EXPECT_EQ((rfl::enum_to_string<WireFlags, rfl::EnumSnakeCaseToKebabCase>(
                combined)),
            "read-access|exec-access");
  const auto parsed_flags =
      rfl::string_to_enum<WireFlags, true, rfl::EnumSnakeCaseToKebabCase>(
          "read-access|write-access");
  ASSERT_TRUE(parsed_flags) << parsed_flags.error().what();
  EXPECT_EQ(*parsed_flags, WireFlags::read_access | WireFlags::write_access);

  // Unscoped uint8_t enum.
  EXPECT_EQ((rfl::enum_to_string<UnscopedWireEnum,
                                 rfl::EnumSnakeCaseToKebabCase>(
                unscoped_two)),
            "unscoped-two");
  ASSERT_EQ(*(rfl::string_to_enum<UnscopedWireEnum, true,
                                  rfl::EnumSnakeCaseToKebabCase>(
                "unscoped-one")),
            unscoped_one);

  // Non-int int64_t enum with out-of-range override + processor transform.
  EXPECT_EQ((rfl::enum_to_string<WideWireEnum, rfl::EnumSnakeCaseToKebabCase>(
                WideWireEnum::small_val)),
            "small-val");
  EXPECT_EQ((rfl::enum_to_string<WideWireEnum, rfl::EnumSnakeCaseToKebabCase>(
                WideWireEnum::big_val)),
            "one-million");
  ASSERT_EQ(*(rfl::string_to_enum<WideWireEnum, true,
                                  rfl::EnumSnakeCaseToKebabCase>(
                "one-million")),
            WideWireEnum::big_val);
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
