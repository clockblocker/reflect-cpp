#include <gtest/gtest.h>

#include <array>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <span>
#include <string>
#include <string_view>

namespace test_metadata {

struct InlineConfig {
  rfl::Description<"Root cache directory", std::string> cache_dir =
      "/var/cache/app";
  rfl::Description<"Max number of files", int> max_files = 100;
  rfl::Description<"Enable compression", bool> compression = true;
  rfl::Deprecated<"Use compression_codec instead", "Legacy compression flag",
                  bool>
      legacy_gzip = false;
};

struct OutOfBandConfig {
  std::string cache_dir = "/var/cache/app";
  int max_files = 100;
  bool compression = true;
  bool legacy_gzip = false;
};

struct OutOfBandDefaultValConfig {
  rfl::DefaultVal<std::string> cache_dir = "/var/cache/app";
  rfl::DefaultVal<int> max_files = 100;
  rfl::DefaultVal<bool> legacy_gzip = false;
};

struct RuntimeMetaConfig {
  std::string host = "localhost";
  int port = 8080;
};

struct EmptyDescConfig {
  int x = 1;
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

struct NamePriorityConfig {
  int foo_bar = 1;
};

struct InlineDescPlusOobDeprecated {
  rfl::Description<"Original inline description", int> value = 5;
};

struct CustomReflectedId {
  using ReflectionType = std::string;
  std::string value;
  const std::string& reflection() const { return value; }
};

struct ContainerWithReflectedId {
  CustomReflectedId id;
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
struct rfl::Metadata<test_metadata::OutOfBandDefaultValConfig> {
  static constexpr std::array<rfl::FieldMeta, 3> fields = {{
      {"cache_dir", "Root cache directory"},
      {"max_files", "Max number of files"},
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
struct rfl::Metadata<test_metadata::EmptyDescConfig> {
  static constexpr std::string_view description = "";
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
      // Raw name listed BEFORE wire name; exact wire-name match must win:
      {"foo_bar", "Raw fallback description"},
      {"fooBar", "Wire name wins"},
  }};
};

template <>
struct rfl::Metadata<test_metadata::InlineDescPlusOobDeprecated> {
  static constexpr std::array<rfl::FieldMeta, 1> fields = {{
      {"value", "", "Use replacement instead"},
  }};
};

template <>
struct rfl::Metadata<test_metadata::CustomReflectedId> {
  static constexpr std::string_view description = "Opaque resource identifier";
};

namespace test_metadata {

namespace {

rfl::Generic::Object sole_def_object(const std::string& _schema) {
  const auto root =
      rfl::json::read<rfl::Generic>(_schema).value().to_object().value();
  const auto defs = root.get("$defs").value().to_object().value();
  EXPECT_EQ(defs.size(), 1u);
  return defs.begin()->second.to_object().value();
}

}  // namespace

TEST(json, test_metadata_matches_inline_description_and_deprecated) {
  const auto inline_schema =
      rfl::json::to_schema<InlineConfig, rfl::DefaultIfMissing>();
  const auto oob_schema =
      rfl::json::to_schema<OutOfBandConfig, rfl::DefaultIfMissing>();

  const auto inline_props =
      sole_def_object(inline_schema).get("properties").value().to_object().value();
  const auto oob_props =
      sole_def_object(oob_schema).get("properties").value().to_object().value();

  EXPECT_EQ(rfl::json::write(inline_props), rfl::json::write(oob_props));
}

TEST(json, test_metadata_with_default_val_fields) {
  const auto schema = rfl::json::to_schema<OutOfBandDefaultValConfig>();
  const auto def = sole_def_object(schema);
  const auto props = def.get("properties").value().to_object().value();

  const auto cache_dir = props.get("cache_dir").value().to_object().value();
  EXPECT_EQ(cache_dir.get("description").value().to_string().value(),
            "Root cache directory");
  EXPECT_EQ(cache_dir.get("default").value().to_string().value(),
            "/var/cache/app");

  const auto max_files = props.get("max_files").value().to_object().value();
  EXPECT_EQ(max_files.get("description").value().to_string().value(),
            "Max number of files");
  EXPECT_EQ(max_files.get("default").value().to_int().value(), 100);

  const auto legacy = props.get("legacy_gzip").value().to_object().value();
  EXPECT_EQ(legacy.get("description").value().to_string().value(),
            "Legacy compression flag");
  EXPECT_EQ(legacy.get("deprecated").value().to_bool().value(), true);
  EXPECT_EQ(legacy.get("deprecationMessage").value().to_string().value(),
            "Use compression_codec instead");
  EXPECT_EQ(legacy.get("default").value().to_bool().value(), false);

  // DefaultVal fields remain optional even when wrapped in Description/Deprecated.
  if (const auto req = def.get("required"); req.has_value()) {
    EXPECT_TRUE(req.value().to_array().value().empty());
  }
}

TEST(json, test_metadata_runtime_function_and_struct_description) {
  const auto schema =
      rfl::json::to_schema<RuntimeMetaConfig, rfl::DefaultIfMissing>();
  const auto def = sole_def_object(schema);
  EXPECT_EQ(def.get("description").value().to_string().value(),
            "Network endpoint configuration");
  const auto props = def.get("properties").value().to_object().value();
  EXPECT_EQ(props.get("host").value().to_object().value()
                .get("description").value().to_string().value(),
            "Server hostname");
  EXPECT_EQ(props.get("port").value().to_object().value()
                .get("description").value().to_string().value(),
            "Listening TCP port");

  // Empty struct description must not emit a bogus empty "description" key.
  const auto empty_schema = rfl::json::to_schema<EmptyDescConfig>();
  const auto empty_def = sole_def_object(empty_schema);
  EXPECT_FALSE(empty_def.get("description").has_value());
}

TEST(json, test_metadata_with_processors_and_rename) {
  const auto schema =
      rfl::json::to_schema<ProcessedConfig, rfl::SnakeCaseToCamelCase,
                           rfl::DefaultIfMissing>();
  const auto props =
      sole_def_object(schema).get("properties").value().to_object().value();
  EXPECT_EQ(props.get("cacheDir").value().to_object().value()
                .get("description").value().to_string().value(),
            "Directory on disk");
  EXPECT_EQ(props.get("maxEntries").value().to_object().value()
                .get("description").value().to_string().value(),
            "Upper bound on entries");
}

TEST(json, test_metadata_with_add_struct_name_and_defaults) {
  const auto schema_dv =
      rfl::json::to_schema<AddStructNameWithDefault,
                           rfl::AddStructName<"type">>();
  const auto props_dv =
      sole_def_object(schema_dv).get("properties").value().to_object().value();
  EXPECT_EQ(rfl::json::write(props_dv.get("type").value()),
            R"({"type":"string","enum":["AddStructNameWithDefault"]})");
  const auto count_obj = props_dv.get("count").value().to_object().value();
  EXPECT_EQ(count_obj.get("default").value().to_int().value(), 7);
  EXPECT_EQ(count_obj.get("description").value().to_string().value(),
            "Item count");

  const auto schema_sc =
      rfl::json::to_schema<AddStructNameWithDefault,
                           rfl::AddStructName<"type">,
                           rfl::SnakeCaseToCamelCase, rfl::DefaultIfMissing>();
  const auto props_sc =
      sole_def_object(schema_sc).get("properties").value().to_object().value();
  EXPECT_EQ(props_sc.get("cacheDir").value().to_object().value()
                .get("description").value().to_string().value(),
            "Cache path under AddStructName + SnakeCaseToCamelCase");
}

TEST(json, test_metadata_wire_name_precedence_and_inline_desc_preservation) {
  const auto prio_schema =
      rfl::json::to_schema<NamePriorityConfig, rfl::SnakeCaseToCamelCase,
                           rfl::DefaultIfMissing>();
  const auto prio_props =
      sole_def_object(prio_schema).get("properties").value().to_object().value();
  EXPECT_EQ(prio_props.get("fooBar").value().to_object().value()
                .get("description").value().to_string().value(),
            "Wire name wins");
  EXPECT_EQ(prio_schema.find("Raw fallback description"), std::string::npos);

  const auto dep_schema =
      rfl::json::to_schema<InlineDescPlusOobDeprecated,
                           rfl::DefaultIfMissing>();
  const auto dep_val =
      sole_def_object(dep_schema).get("properties").value().to_object().value()
          .get("value").value().to_object().value();
  EXPECT_EQ(dep_val.get("description").value().to_string().value(),
            "Original inline description");
  EXPECT_EQ(dep_val.get("deprecated").value().to_bool().value(), true);
  EXPECT_EQ(dep_val.get("deprecationMessage").value().to_string().value(),
            "Use replacement instead");
}

TEST(json, test_metadata_nested_and_reflection_type_structs) {
  const auto schema = rfl::json::to_schema<OuterMeta, rfl::DefaultIfMissing>();
  EXPECT_NE(schema.find("\"description\":\"Application name\""),
            std::string::npos);
  EXPECT_NE(schema.find("\"description\":\"Nested settings object\""),
            std::string::npos);
  EXPECT_NE(schema.find("\"description\":\"Nested retry settings\""),
            std::string::npos);
  EXPECT_NE(schema.find("\"description\":\"Number of retry attempts\""),
            std::string::npos);

  const auto refl_schema = rfl::json::to_schema<ContainerWithReflectedId>();
  EXPECT_NE(refl_schema.find("\"description\":\"Opaque resource identifier\""),
            std::string::npos);
}

TEST(json, test_metadata_roundtrip_unchanged) {
  const auto parsed = rfl::json::read<OutOfBandConfig, rfl::DefaultIfMissing>(
                          R"({"max_files":250})")
                          .value();
  EXPECT_EQ(parsed.cache_dir, "/var/cache/app");
  EXPECT_EQ(parsed.max_files, 250);
  EXPECT_EQ(parsed.compression, true);
  EXPECT_EQ(parsed.legacy_gzip, false);
}

}  // namespace test_metadata
