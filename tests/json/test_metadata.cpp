#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <limits>
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

struct AddStructNameWithDefault {
  rfl::DefaultVal<int> count = 7;
  std::string cache_dir = "/tmp";
};

struct EdgeDefaultsConfig {
  uint64_t big_u64 =
      static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 42ULL;
  char tag_char = 'Z';
  float ratio = 1.5f;
  std::vector<std::string> tags = {"a", "b"};
  std::optional<int> opt_set = 9;
  std::optional<int> opt_empty = std::nullopt;
};

struct NamePriorityConfig {
  // Under SnakeCaseToCamelCase, `foo_bar` -> wire `"fooBar"`.
  // `fooBar` stays `"fooBar"`? Avoid duplicate wire names; instead test
  // Rename swapping a raw name into another field's wire name:
  rfl::Rename<"alpha", int> first_field = 1;
  rfl::Rename<"first_field", int> second_field = 2;
};

struct InlineDescPlusOobDeprecated {
  rfl::Description<"Original inline description", int> value = 5;
};

struct AliasAndSkipConfig {
  rfl::SkipDeserialization<int> shadowed = 0;
  rfl::Alias<"old_val", int> value;
};

struct EmptyStruct {};
struct OneField {
  int a;
};
struct OneByte {
  char a;
};
struct FourBytes {
  char a, b, c, d;
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
struct WithCArray {
  int a[3];
  int b;
};
struct WithConstRefMember {
  const int& ref;
  int val;
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
      {"cache_dir", "Directory on disk"},
      {"maxEntries", "Upper bound on entries"},
  }};
};

template <>
struct rfl::Metadata<test_metadata::AddStructNameWithDefault> {
  static constexpr std::array<rfl::FieldMeta, 2> fields = {{
      {"count", "Item count"},
      {"cache_dir", "Cache path under AddStructName + SnakeCaseToCamelCase"},
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

template <>
struct rfl::Metadata<test_metadata::NamePriorityConfig> {
  static constexpr std::array<rfl::FieldMeta, 2> fields = {{
      {"first_field", "Matches second_field wire name"},
      {"alpha", "Matches first_field wire name"},
  }};
};

template <>
struct rfl::Metadata<test_metadata::InlineDescPlusOobDeprecated> {
  static constexpr std::array<rfl::FieldMeta, 1> fields = {{
      {"value", "", "Use replacement instead"},
  }};
};

namespace test_metadata {

TEST(json, test_metadata_parity_with_inline_description_and_deprecated) {
  const auto inline_schema = rfl::json::to_schema<InlineConfig>();
  const auto oob_schema =
      rfl::json::to_schema<OutOfBandConfig, rfl::DefaultIfMissing>();

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

TEST(json, test_metadata_with_add_struct_name_and_defaults) {
  const auto schema_dv =
      rfl::json::to_schema<AddStructNameWithDefault,
                           rfl::AddStructName<"type">>();
  EXPECT_NE(
      schema_dv.find("\"type\":{\"type\":\"string\",\"enum\":[\"AddStructNameWithDefault\"]}"),
      std::string::npos);
  EXPECT_NE(schema_dv.find("\"default\":7"), std::string::npos);
  EXPECT_NE(schema_dv.find("\"description\":\"Item count\""),
            std::string::npos);

  const auto schema_dim =
      rfl::json::to_schema<AddStructNameWithDefault,
                           rfl::AddStructName<"type">,
                           rfl::SnakeCaseToCamelCase, rfl::DefaultIfMissing>();
  EXPECT_NE(schema_dim.find("\"cacheDir\""), std::string::npos);
  EXPECT_NE(
      schema_dim.find(
          "\"description\":\"Cache path under AddStructName + SnakeCaseToCamelCase\""),
      std::string::npos);
  EXPECT_NE(schema_dim.find("\"default\":\"/tmp\""), std::string::npos);
}

TEST(json, test_to_default_generic_edge_values) {
  const auto schema =
      rfl::json::to_schema<EdgeDefaultsConfig, rfl::DefaultIfMissing>();
  EXPECT_NE(schema.find("\"default\":[\"a\",\"b\"]"), std::string::npos);
  EXPECT_NE(schema.find("\"default\":9"), std::string::npos);
  EXPECT_NE(schema.find("\"default\":1.5"), std::string::npos);
  // char 'Z' (ASCII 90) matches rfl::to_generic behavior:
  EXPECT_NE(schema.find("\"default\":90"), std::string::npos);
}

TEST(json, test_metadata_wire_name_precedence_and_inline_desc_preservation) {
  const auto prio_schema =
      rfl::json::to_schema<NamePriorityConfig, rfl::DefaultIfMissing>();
  EXPECT_NE(
      prio_schema.find(
          "\"alpha\":{\"type\":\"integer\",\"description\":\"Matches first_field wire name\""),
      std::string::npos);
  EXPECT_NE(
      prio_schema.find(
          "\"first_field\":{\"type\":\"integer\",\"description\":\"Matches second_field wire name\""),
      std::string::npos);

  const auto dep_schema =
      rfl::json::to_schema<InlineDescPlusOobDeprecated,
                           rfl::DefaultIfMissing>();
  EXPECT_NE(dep_schema.find("\"description\":\"Original inline description\""),
            std::string::npos);
  EXPECT_NE(dep_schema.find("\"deprecated\":true"), std::string::npos);
  EXPECT_NE(dep_schema.find("\"deprecationMessage\":\"Use replacement instead\""),
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

TEST(json, test_metadata_roundtrip_and_view_reader_aliases) {
  const auto parsed = rfl::json::read<OutOfBandConfig, rfl::DefaultIfMissing>(
                          R"({"max_files":250})")
                          .value();
  EXPECT_EQ(parsed.cache_dir, "/var/cache/app");
  EXPECT_EQ(parsed.max_files, 250);
  EXPECT_EQ(parsed.compression, true);
  EXPECT_EQ(parsed.legacy_gzip, false);

  const auto via_alias =
      rfl::json::read<AliasAndSkipConfig>(R"({"old_val":99})").value();
  EXPECT_EQ(via_alias.value.value(), 99);
  EXPECT_EQ(via_alias.shadowed.value(), 0);

  const auto dup_err =
      rfl::json::read<AliasAndSkipConfig>(R"({"value":1,"old_val":2})");
  EXPECT_FALSE(dup_err.has_value());
}

TEST(json, test_num_fields_binary_search_edge_cases) {
  static_assert(rfl::num_fields<EmptyStruct> == 0);
  static_assert(rfl::num_fields<OneField> == 1);
  static_assert(rfl::num_fields<OneByte> == 1);
  static_assert(rfl::num_fields<FourBytes> == 4);
  static_assert(rfl::num_fields<MixedAggregate> == 4);
#ifdef REFLECT_CPP_C_ARRAYS_OR_INHERITANCE
  static_assert(rfl::num_fields<WithCArray> == 2);
#else
  // Without REFLECT_CPP_C_ARRAYS_OR_INHERITANCE, brace elision counts
  // flattened aggregate initializer elements (3 + 1 = 4).
  static_assert(rfl::num_fields<WithCArray> == 4);
#endif
  static_assert(rfl::num_fields<WithConstRefMember> == 2);
  static_assert(rfl::num_fields<WideStruct32> == 32);
  SUCCEED();
}

}  // namespace test_metadata
