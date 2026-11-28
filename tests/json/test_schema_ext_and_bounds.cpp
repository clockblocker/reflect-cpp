#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <tuple>
#include <vector>

namespace test_schema_ext_and_bounds {

struct CustomNamedConfig {
  std::string host = "localhost";
  int port = 8080;
};

struct AnnotatedType {
  std::string name;
};

struct InlineToken {
  using ReflectionType = std::string;
  std::string value;
  const std::string& reflection() const { return value; }
};

struct InlineHost {
  InlineToken token;
};

struct BoundedIntegers {
  int8_t i8;
  uint8_t u8;
  int16_t i16;
  uint16_t u16;
  int32_t i32;
  uint32_t u32;
  int64_t i64;
  uint64_t u64;
  rfl::Validator<uint16_t, rfl::Minimum<1>> port;
};

struct TupleAndOptional {
  std::optional<int> opt;
  std::tuple<int, std::string> pair;
};

struct FnNamedConfig {
  int v = 0;
};

using AnnotatedPort = rfl::Validator<int, rfl::Minimum<1>, rfl::Maximum<65535>>;

struct WithAnnotatedValidator {
  AnnotatedPort port = 80;
};

struct WithRequiredAndOptional {
  std::string required_field;
  std::optional<int> opt;
};

struct BooleanDefaultOrder {
  rfl::DefaultVal<bool> enabled = true;
};

}  // namespace test_schema_ext_and_bounds

namespace rfl {

template <>
struct SchemaName<test_schema_ext_and_bounds::CustomNamedConfig> {
  static constexpr std::string_view value = "ExportedConfig";
};

template <>
struct SchemaExtension<test_schema_ext_and_bounds::AnnotatedType> {
  static SchemaAnnotations annotations() {
    SchemaAnnotations ann;
    ann.title = "Annotated Title";
    ann.examples = std::vector<Generic>{Generic("ex1")};
    ann.extensions["x-category"] = Generic("core");
    return ann;
  }
  static Object<Generic> extensions() {
    Object<Generic> ext;
    ext["x-stability"] = Generic("stable");
    return ext;
  }
};

template <>
struct SchemaExtension<test_schema_ext_and_bounds::InlineToken> {
  static constexpr bool inline_schema = true;
  static SchemaAnnotations annotations() {
    SchemaAnnotations ann;
    ann.title = "Inline Token";
    ann.extensions["x-sensitive"] = Generic(true);
    return ann;
  }
};

template <>
struct SchemaName<test_schema_ext_and_bounds::FnNamedConfig> {
  static std::string value() { return "DynamicConfig"; }
};

template <>
struct SchemaExtension<test_schema_ext_and_bounds::AnnotatedPort> {
  static constexpr bool inline_schema = true;
  static SchemaAnnotations annotations() {
    SchemaAnnotations ann;
    ann.title = "TCP Port";
    return ann;
  }
};

}  // namespace rfl

namespace test_schema_ext_and_bounds {

TEST(json, test_schema_name_override) {
  const auto schema = rfl::json::to_schema<CustomNamedConfig>();
  EXPECT_NE(schema.find(R"("$ref":"#/$defs/ExportedConfig")"),
            std::string::npos)
      << schema;
  EXPECT_NE(schema.find(R"("ExportedConfig":{"type":"object")"),
            std::string::npos)
      << schema;

  using Tagged = rfl::TaggedUnion<"kind", CustomNamedConfig>;
  const auto tagged_schema = rfl::json::to_schema<Tagged>();
  EXPECT_NE(tagged_schema.find(R"(ExportedConfig__tagged)"), std::string::npos)
      << tagged_schema;

  const auto fn_schema = rfl::json::to_schema<FnNamedConfig>();
  EXPECT_NE(fn_schema.find(R"("$ref":"#/$defs/DynamicConfig")"),
            std::string::npos)
      << fn_schema;
}

TEST(json, test_schema_extension_title_examples_extensions_and_inline) {
  const auto schema = rfl::json::to_schema<AnnotatedType>();
  EXPECT_NE(schema.find(R"("title":"Annotated Title")"), std::string::npos)
      << schema;
  EXPECT_NE(schema.find(R"("examples":["ex1"])"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("x-category":"core")"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("x-stability":"stable")"), std::string::npos)
      << schema;

  const auto inline_schema = rfl::json::to_schema<InlineHost>();
  EXPECT_EQ(inline_schema.find("InlineToken"), std::string::npos)
      << inline_schema;
  EXPECT_NE(inline_schema.find(
                R"("token":{"type":"string","title":"Inline Token","x-sensitive":true})"),
            std::string::npos)
      << inline_schema;

  // SchemaExtension with inline_schema = true on a Validator must preserve
  // validation constraints (and apply annotations).
  const auto val_schema = rfl::json::to_schema<WithAnnotatedValidator>();
  EXPECT_NE(val_schema.find(R"("title":"TCP Port")"), std::string::npos)
      << val_schema;
  EXPECT_NE(val_schema.find(R"("minimum":1)"), std::string::npos) << val_schema;
  EXPECT_NE(val_schema.find(R"("maximum":65535)"), std::string::npos)
      << val_schema;
}

TEST(json, test_schema_integer_bounds_processor) {
  // Without IntegerBounds, plain integers have no minimum/maximum.
  const auto plain = rfl::json::to_schema<BoundedIntegers>();
  EXPECT_NE(plain.find(R"("u8":{"type":"integer"})"), std::string::npos)
      << plain;

  const auto bounded =
      rfl::json::to_schema<BoundedIntegers, rfl::IntegerBounds>();
  EXPECT_NE(bounded.find(R"("i8":{"type":"integer","minimum":-128,"maximum":127})"),
            std::string::npos)
      << bounded;
  EXPECT_NE(bounded.find(R"("u8":{"type":"integer","minimum":0,"maximum":255})"),
            std::string::npos)
      << bounded;
  EXPECT_NE(
      bounded.find(R"("i16":{"type":"integer","minimum":-32768,"maximum":32767})"),
      std::string::npos)
      << bounded;
  EXPECT_NE(
      bounded.find(R"("u16":{"type":"integer","minimum":0,"maximum":65535})"),
      std::string::npos)
      << bounded;
  EXPECT_NE(
      bounded.find(
          R"("i32":{"type":"integer","minimum":-2147483648,"maximum":2147483647})"),
      std::string::npos)
      << bounded;
  EXPECT_NE(
      bounded.find(
          R"("u32":{"type":"integer","minimum":0,"maximum":4294967295})"),
      std::string::npos)
      << bounded;
  EXPECT_NE(
      bounded.find(
          R"("i64":{"type":"integer","minimum":-9223372036854775808,"maximum":9223372036854775807})"),
      std::string::npos)
      << bounded;
  EXPECT_NE(
      bounded.find(
          R"("u64":{"type":"integer","minimum":0,"maximum":18446744073709551615})"),
      std::string::npos)
      << bounded;
  // Validated integer combines width bounds and validator via allOf.
  EXPECT_NE(
      bounded.find(
          R"("port":{"allOf":[{"type":"integer","minimum":0,"maximum":65535},{"minimum":1,"type":"integer"}]})"),
      std::string::npos)
      << bounded;
}

TEST(json, test_schema_tuple_bounds_omit_empty_required_and_bool_order) {
  const auto schema =
      rfl::json::to_schema<TupleAndOptional, rfl::TupleBounds,
                           rfl::OmitEmptyRequired, rfl::DefaultIfMissing>();
  EXPECT_EQ(schema.find(R"("required")"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("minItems":2,"maxItems":2)"), std::string::npos)
      << schema;

  // OmitEmptyRequired keeps non-empty required arrays intact.
  const auto non_empty =
      rfl::json::to_schema<WithRequiredAndOptional, rfl::OmitEmptyRequired>();
  EXPECT_NE(non_empty.find(R"("required":["required_field"])"),
            std::string::npos)
      << non_empty;

  // Boolean schema emits "type":"boolean" before "default":true.
  const auto bool_schema = rfl::json::to_schema<BooleanDefaultOrder>();
  EXPECT_NE(bool_schema.find(R"("enabled":{"type":"boolean","default":true})"),
            std::string::npos)
      << bool_schema;
}

}  // namespace test_schema_ext_and_bounds
