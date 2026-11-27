#include <gtest/gtest.h>

#include <array>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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

struct PlainDeprecatedOnly {
  bool old_flag = false;
};

struct CircleAlt {
  double radius_m = 1.0;
};

struct SquareAlt {
  double side_m = 2.0;
};

struct FlattenedBase {
  int base_id = 1;
};

struct FlattenedOuter {
  rfl::Flatten<FlattenedBase> base;
  std::string label = "x";
};

struct EmptyMetaStruct {};

struct VecRefMetaConfig {
  int value = 0;
};

struct DanglingVecMeta {};

struct NonStaticMeta {};

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

template <>
struct rfl::Metadata<test_metadata::PlainDeprecatedOnly> {
  static constexpr std::array<rfl::FieldMeta, 1> fields = {{
      {.name = "old_flag", .deprecation_message = "Use new_flag"},
  }};
};

template <>
struct rfl::Metadata<test_metadata::CircleAlt> {
  static constexpr std::string_view description = "A circle";
  static constexpr std::array<rfl::FieldMeta, 1> fields = {{
      {"radius_m", "Radius in meters"},
  }};
};

template <>
struct rfl::Metadata<test_metadata::FlattenedOuter> {
  static constexpr std::array<rfl::FieldMeta, 2> fields = {{
      {"base_id", "Flattened identifier"},
      {"label", "Outer label"},
  }};
};

template <>
struct rfl::Metadata<test_metadata::EmptyMetaStruct> {
  static constexpr std::string_view description = "Marker struct";
  static constexpr std::array<rfl::FieldMeta, 0> fields = {};
};

template <>
struct rfl::Metadata<test_metadata::VecRefMetaConfig> {
  static const std::vector<rfl::FieldMeta>& fields() {
    static const std::vector<rfl::FieldMeta> kFields = {
        {"value", "Returned via const vector lvalue reference"},
    };
    return kFields;
  }
};

template <>
struct rfl::Metadata<test_metadata::DanglingVecMeta> {
  static std::vector<rfl::FieldMeta> fields() { return {}; }
};

template <>
struct rfl::Metadata<test_metadata::NonStaticMeta> {
  std::array<rfl::FieldMeta, 1> fields = {{{"x", "bad"}}};
  std::string_view description = "bad";
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

TEST(json, test_metadata_plain_deprecated_omits_empty_description) {
  const auto schema = rfl::json::to_schema<PlainDeprecatedOnly>();
  const auto props =
      sole_def_object(schema).get("properties").value().to_object().value();
  const auto flag = props.get("old_flag").value().to_object().value();
  EXPECT_EQ(flag.get("deprecated").value().to_bool().value(), true);
  EXPECT_EQ(flag.get("deprecationMessage").value().to_string().value(),
            "Use new_flag");
  EXPECT_FALSE(flag.get("description").has_value());
}

TEST(json, test_metadata_tagged_union_alternatives) {
  using Shape = rfl::TaggedUnion<"shape", CircleAlt, SquareAlt>;
  const auto schema =
      rfl::json::to_schema<Shape, rfl::SnakeCaseToCamelCase>();
  const auto root =
      rfl::json::read<rfl::Generic>(schema).value().to_object().value();
  const auto defs = root.get("$defs").value().to_object().value();
  bool found_circle = false;
  bool found_square = false;
  for (const auto& [def_name, def_val] : defs) {
    if (def_name.find("CircleAlt") != std::string::npos) {
      found_circle = true;
      const auto def_obj = def_val.to_object().value();
      EXPECT_EQ(def_obj.get("description").value().to_string().value(),
                "A circle");
      const auto props = def_obj.get("properties").value().to_object().value();
      EXPECT_EQ(props.get("radiusM")
                    .value()
                    .to_object()
                    .value()
                    .get("description")
                    .value()
                    .to_string()
                    .value(),
                "Radius in meters");
    } else if (def_name.find("SquareAlt") != std::string::npos) {
      found_square = true;
      const auto def_obj = def_val.to_object().value();
      EXPECT_FALSE(def_obj.get("description").has_value());
    }
  }
  EXPECT_TRUE(found_circle);
  EXPECT_TRUE(found_square);
}

TEST(json, test_metadata_flatten_empty_no_field_names_and_concepts) {
  // Concepts: dangling by-value vector rejected; const vector& accepted;
  // non-static members rejected.
  static_assert(rfl::internal::HasAnyFieldMetaFn<DanglingVecMeta>);
  static_assert(!rfl::internal::HasFieldMetaFn<DanglingVecMeta>);
  static_assert(rfl::internal::HasFieldMetaFn<VecRefMetaConfig>);
  static_assert(!rfl::internal::HasFieldMetaVar<NonStaticMeta>);
  static_assert(!rfl::internal::HasStructDescriptionVar<NonStaticMeta>);

  const auto vec_schema = rfl::json::to_schema<VecRefMetaConfig>();
  EXPECT_NE(
      vec_schema.find(
          "\"description\":\"Returned via const vector lvalue reference\""),
      std::string::npos);

  // Flatten: fields looked up on enclosing struct's Metadata<Outer>.
  const auto flat_schema =
      rfl::json::to_schema<FlattenedOuter, rfl::SnakeCaseToCamelCase>();
  const auto flat_props =
      sole_def_object(flat_schema).get("properties").value().to_object().value();
  EXPECT_EQ(flat_props.get("baseId").value().to_object().value()
                .get("description").value().to_string().value(),
            "Flattened identifier");
  EXPECT_EQ(flat_props.get("label").value().to_object().value()
                .get("description").value().to_string().value(),
            "Outer label");

  // Empty struct with Metadata<Empty>::description and empty fields array.
  const auto empty_schema = rfl::json::to_schema<EmptyMetaStruct>();
  EXPECT_EQ(
      sole_def_object(empty_schema).get("description").value().to_string().value(),
      "Marker struct");

  // NoFieldNames emits tuple schema with per-element descriptions intact.
  const auto tuple_schema =
      rfl::json::to_schema<OutOfBandConfig, rfl::NoFieldNames>();
  EXPECT_NE(tuple_schema.find("\"description\":\"Root cache directory\""),
            std::string::npos);
  EXPECT_NE(tuple_schema.find("\"description\":\"Max number of files\""),
            std::string::npos);
}

}  // namespace test_metadata
