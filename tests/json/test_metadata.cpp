#include <gtest/gtest.h>

#include <array>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <span>
#include <string>
#include <vector>

namespace test_metadata {

struct InlineConfig {
  rfl::Description<"Root cache directory", rfl::DefaultVal<std::string>>
      cache_dir = "/var/cache/app";
  rfl::Description<"Max number of files", rfl::DefaultVal<int>> max_files = 100;
  rfl::Description<"Enable compression", rfl::DefaultVal<bool>> compression =
      true;
  rfl::Deprecated<"Use compression_codec instead", "Legacy compression flag",
                  rfl::DefaultVal<bool>>
      legacy_gzip = false;
};

struct OutOfBandConfig {
  std::string cache_dir = "/var/cache/app";
  int max_files = 100;
  bool compression = true;
  bool legacy_gzip = false;
};

struct RuntimeMetaConfig {
  std::string host = "localhost";
  int port = 8080;
};

struct ProcessedConfig {
  std::string cache_dir = "/tmp";
  rfl::Rename<"maxEntries", int> max_entries = 42;
};

struct InnerMeta {
  int retries = 3;
};

struct OuterMeta {
  std::string name = "app";
  InnerMeta inner = {};
};

struct EmptyStruct {};
struct OneField {
  int a;
};
struct NonDefaultable {
  int x;
  explicit NonDefaultable(int _x) : x(_x) {}
};
struct MixedAggregate {
  int a;
  NonDefaultable b;
  std::string c;
  bool d;
};
struct WideStruct32 {
  int f00, f01, f02, f03, f04, f05, f06, f07;
  int f08, f09, f10, f11, f12, f13, f14, f15;
  int f16, f17, f18, f19, f20, f21, f22, f23;
  int f24, f25, f26, f27, f28, f29, f30, f31;
};

}  // namespace test_metadata

template <>
struct rfl::Metadata<test_metadata::OutOfBandConfig> {
  static constexpr std::array<rfl::FieldMeta, 4> fields = {{
      {"cache_dir", "Root cache directory"},
      {"max_files", "Max number of files"},
      {"compression", "Enable compression"},
      {"legacy_gzip", "Legacy compression flag",
       "Use compression_codec instead"},
  }};
};

template <>
struct rfl::Metadata<test_metadata::RuntimeMetaConfig> {
  static std::span<const rfl::FieldMeta> fields() {
    static const std::array<rfl::FieldMeta, 2> kFields = {{
        {"host", "Server hostname"},
        {"port", "Listening TCP port"},
    }};
    return kFields;
  }

  static std::string_view description() {
    return "Network endpoint configuration";
  }
};

template <>
struct rfl::Metadata<test_metadata::ProcessedConfig> {
  static constexpr std::array<rfl::FieldMeta, 2> fields = {{
      // Matches raw C++ member name even under SnakeCaseToCamelCase:
      {"cache_dir", "Directory on disk"},
      // Matches renamed wire name:
      {"maxEntries", "Upper bound on entries"},
  }};
};

template <>
struct rfl::Metadata<test_metadata::InnerMeta> {
  static constexpr std::string_view description = "Nested retry settings";
  static constexpr std::array<rfl::FieldMeta, 1> fields = {{
      {"retries", "Number of retry attempts"},
  }};
};

template <>
struct rfl::Metadata<test_metadata::OuterMeta> {
  static constexpr std::array<rfl::FieldMeta, 2> fields = {{
      {"name", "Application name"},
      {"inner", "Nested settings object"},
  }};
};

namespace test_metadata {

TEST(json, test_metadata_parity_with_inline_description_and_deprecated) {
  const auto inline_schema = rfl::json::to_schema<InlineConfig>();
  const auto oob_schema =
      rfl::json::to_schema<OutOfBandConfig, rfl::DefaultIfMissing>();

  // Replace struct name in $ref/definitions so we can verify exact schema
  // parity.
  auto normalized = oob_schema;
  for (size_t pos = 0;
       (pos = normalized.find("OutOfBandConfig", pos)) != std::string::npos;) {
    normalized.replace(pos, std::string_view("OutOfBandConfig").size(),
                       "InlineConfig");
  }
  EXPECT_EQ(normalized, inline_schema);
}

TEST(json, test_metadata_function_and_struct_description) {
  const auto schema =
      rfl::json::to_schema<RuntimeMetaConfig, rfl::DefaultIfMissing>();
  EXPECT_NE(schema.find("\"description\":\"Network endpoint configuration\""),
            std::string::npos);
  EXPECT_NE(schema.find("\"description\":\"Server hostname\""),
            std::string::npos);
  EXPECT_NE(schema.find("\"description\":\"Listening TCP port\""),
            std::string::npos);
  EXPECT_NE(schema.find("\"default\":\"localhost\""), std::string::npos);
  EXPECT_NE(schema.find("\"default\":8080"), std::string::npos);
}

TEST(json, test_metadata_matches_raw_and_wire_names_with_processors) {
  const auto schema =
      rfl::json::to_schema<ProcessedConfig, rfl::SnakeCaseToCamelCase,
                           rfl::DefaultIfMissing>();
  EXPECT_NE(schema.find("\"cacheDir\""), std::string::npos);
  EXPECT_NE(schema.find("\"description\":\"Directory on disk\""),
            std::string::npos);
  EXPECT_NE(schema.find("\"maxEntries\""), std::string::npos);
  EXPECT_NE(schema.find("\"description\":\"Upper bound on entries\""),
            std::string::npos);
}

TEST(json, test_metadata_nested_structs) {
  const auto schema = rfl::json::to_schema<OuterMeta, rfl::DefaultIfMissing>();
  EXPECT_NE(schema.find("\"description\":\"Application name\""),
            std::string::npos);
  EXPECT_NE(schema.find("\"description\":\"Nested settings object\""),
            std::string::npos);
  EXPECT_NE(schema.find("\"description\":\"Nested retry settings\""),
            std::string::npos);
  EXPECT_NE(schema.find("\"description\":\"Number of retry attempts\""),
            std::string::npos);
}

TEST(json, test_metadata_roundtrip_unaffected) {
  const auto parsed = rfl::json::read<OutOfBandConfig, rfl::DefaultIfMissing>(
                          R"({"max_files":250})")
                          .value();
  EXPECT_EQ(parsed.cache_dir, "/var/cache/app");
  EXPECT_EQ(parsed.max_files, 250);
  EXPECT_EQ(parsed.compression, true);
  EXPECT_EQ(parsed.legacy_gzip, false);
}

TEST(json, test_num_fields_binary_search_edge_cases) {
  static_assert(rfl::num_fields<EmptyStruct> == 0);
  static_assert(rfl::num_fields<OneField> == 1);
  static_assert(rfl::num_fields<MixedAggregate> == 4);
  static_assert(rfl::num_fields<WideStruct32> == 32);
  SUCCEED();
}

}  // namespace test_metadata
