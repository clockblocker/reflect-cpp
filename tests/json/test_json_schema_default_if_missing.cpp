#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace test_json_schema_default_if_missing {

enum class LogLevel { debug, info, warn, error };

struct ServerConfig {
  int port = 8080;
  std::string host = "localhost";
  bool verbose = true;
  double ratio = 0.75;
  LogLevel level = LogLevel::info;
  std::vector<std::string> tags = {"web", "api"};
  std::vector<int> empty_vec;
};

struct Inner {
  int retries = 3;
  std::string retry_mode = "fast";
};

struct Outer {
  std::string app_name = "demo";
  Inner inner = Inner{.retries = 5, .retry_mode = "safe"};
};

struct OptionalAndPtrFields {
  std::optional<int> empty_opt;
  std::optional<int> init_opt = 5;
  std::optional<int> zero_opt = 0;
  std::optional<bool> false_opt = false;
  std::optional<std::string> empty_str_opt = std::string("");
  std::shared_ptr<int> null_shared;
  std::shared_ptr<int> init_shared = std::make_shared<int>(42);
  std::unique_ptr<int> null_unique;
  std::unique_ptr<int> init_unique = std::make_unique<int>(7);
};

using Port = rfl::Validator<int, rfl::Minimum<1>, rfl::Maximum<65535>>;

struct WrappedFields {
  rfl::Description<"Server port", int> port = 8080;
  rfl::Description<"Optional label", std::optional<std::string>> label;
  rfl::Description<"Optional count", std::optional<int>> count = 10;
  rfl::Deprecated<"Use port", "Legacy port", int> old_port = 80;
  rfl::Rename<"hostName", std::string> host_name = "localhost";
  rfl::Rename<"optTag", std::optional<std::string>> opt_tag;
  Port valid_port = 443;
  rfl::DefaultVal<int> explicit_default = 99;
  rfl::Rename<"renamedDefault", rfl::DefaultVal<int>> renamed_default = 77;
  rfl::Description<"Described default", rfl::DefaultVal<int>> described_default =
      55;
  rfl::Skip<int> skipped = 123;
};

struct NoDefaultCtor {
  using ReflectionType = int;
  explicit NoDefaultCtor(int _v) : value(_v) {}
  int reflection() const { return value; }
  int value;
};

struct WithNonDefaultConstructibleMember {
  NoDefaultCtor member;
  int port = 8080;
};

struct NonWritable {
  int copied;
  int missing;
  struct InnerVal {
    int value;
  };
  using ReflectionType = InnerVal;
  NonWritable() : copied(0), missing(0) {}
  explicit NonWritable(ReflectionType&& _r) : copied(_r.value), missing(0) {}
};

struct WithNonWritableFields {
  int port = 8080;
  NonWritable raw;
  rfl::Description<"desc", NonWritable> described;
  std::optional<NonWritable> optional_nw;
};

inline int throwing_init() {
  throw std::runtime_error("cannot default construct");
}

struct ThrowingDefaultCtor {
  int port = 8080;
  int bad = throwing_init();
};

struct ThrowingValidatorField {
  int port = 8080;
  Port bad_port;  // default 0 fails Minimum<1> and throws
};

struct ThrowingWithDefaultVal {
  rfl::DefaultVal<int> count = 10;
  Port bad_port;  // default 0 fails Minimum<1> and throws
};

struct ThrowingToGeneric {
  int value = 1;
};

struct ReadOnlyReflected {
  int value = 0;
};

}  // namespace test_json_schema_default_if_missing

namespace rfl {
template <>
struct Reflector<test_json_schema_default_if_missing::ThrowingToGeneric> {
  using ReflType = int;
  static test_json_schema_default_if_missing::ThrowingToGeneric to(
      const ReflType& _v) {
    return {_v};
  }
  static ReflType from(
      const test_json_schema_default_if_missing::ThrowingToGeneric&) {
    throw std::runtime_error("cannot serialize default");
  }
};

template <>
struct Reflector<test_json_schema_default_if_missing::ReadOnlyReflected> {
  using ReflType = int;
  static test_json_schema_default_if_missing::ReadOnlyReflected to(
      const ReflType& _v) {
    return {_v};
  }
};
}  // namespace rfl

namespace test_json_schema_default_if_missing {

struct WithThrowingToGenericField {
  int port = 8080;
  ThrowingToGeneric bad_serialize;
};

TEST(json, test_schema_default_if_missing_basic) {
  const auto schema =
      rfl::json::to_schema<ServerConfig, rfl::DefaultIfMissing>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_json_schema_default_if_missing__ServerConfig","$defs":{"test_json_schema_default_if_missing__ServerConfig":{"type":"object","properties":{"port":{"type":"integer","default":8080},"host":{"type":"string","default":"localhost"},"verbose":{"default":true,"type":"boolean"},"ratio":{"type":"number","default":0.75},"level":{"type":"string","default":"info","enum":["debug","info","warn","error"]},"tags":{"type":"array","default":["web","api"],"items":{"type":"string"}},"empty_vec":{"type":"array","default":[],"items":{"type":"integer"}}},"required":[]}}})";

  EXPECT_EQ(schema, expected) << schema;
}

TEST(json, test_schema_without_default_if_missing_has_no_defaults) {
  const auto schema = rfl::json::to_schema<ServerConfig>();

  EXPECT_EQ(schema.find("\"default\""), std::string::npos) << schema;
  EXPECT_NE(
      schema.find(
          R"("required":["port","host","verbose","ratio","level","tags","empty_vec"])"),
      std::string::npos)
      << schema;
}

TEST(json, test_schema_default_if_missing_nested_structs) {
  const auto schema = rfl::json::to_schema<Outer, rfl::DefaultIfMissing>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_json_schema_default_if_missing__Outer","$defs":{"test_json_schema_default_if_missing__Inner":{"type":"object","properties":{"retries":{"type":"integer","default":3},"retry_mode":{"type":"string","default":"fast"}},"required":[]},"test_json_schema_default_if_missing__Outer":{"type":"object","properties":{"app_name":{"type":"string","default":"demo"},"inner":{"$ref":"#/$defs/test_json_schema_default_if_missing__Inner","default":{"retries":5,"retry_mode":"safe"}}},"required":[]}}})";

  EXPECT_EQ(schema, expected) << schema;

  // With field-name processors, nested default keys match the renamed schema.
  const auto camel_schema =
      rfl::json::to_schema<Outer, rfl::DefaultIfMissing,
                           rfl::SnakeCaseToCamelCase>();

  EXPECT_NE(
      camel_schema.find(R"("default":{"retries":5,"retryMode":"safe"})"),
      std::string::npos)
      << camel_schema;
  EXPECT_EQ(camel_schema.find("retry_mode"), std::string::npos) << camel_schema;
}

TEST(json, test_schema_default_if_missing_optionals_and_ptrs) {
  const auto schema =
      rfl::json::to_schema<OptionalAndPtrFields, rfl::DefaultIfMissing>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_json_schema_default_if_missing__OptionalAndPtrFields","$defs":{"test_json_schema_default_if_missing__OptionalAndPtrFields":{"type":"object","properties":{"empty_opt":{"anyOf":[{"type":"integer"},{"type":"null"}]},"init_opt":{"default":5,"anyOf":[{"type":"integer"},{"type":"null"}]},"zero_opt":{"default":0,"anyOf":[{"type":"integer"},{"type":"null"}]},"false_opt":{"default":false,"anyOf":[{"type":"boolean"},{"type":"null"}]},"empty_str_opt":{"default":"","anyOf":[{"type":"string"},{"type":"null"}]},"null_shared":{"anyOf":[{"type":"integer"},{"type":"null"}]},"init_shared":{"default":42,"anyOf":[{"type":"integer"},{"type":"null"}]},"null_unique":{"anyOf":[{"type":"integer"},{"type":"null"}]},"init_unique":{"default":7,"anyOf":[{"type":"integer"},{"type":"null"}]}},"required":[]}}})";

  EXPECT_EQ(schema, expected) << schema;
}

TEST(json, test_schema_default_if_missing_wrappers) {
  const auto schema =
      rfl::json::to_schema<WrappedFields, rfl::DefaultIfMissing>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_json_schema_default_if_missing__WrappedFields","$defs":{"test_json_schema_default_if_missing__WrappedFields":{"type":"object","properties":{"port":{"type":"integer","description":"Server port","default":8080},"label":{"description":"Optional label","anyOf":[{"type":"string"},{"type":"null"}]},"count":{"description":"Optional count","default":10,"anyOf":[{"type":"integer"},{"type":"null"}]},"old_port":{"type":"integer","description":"Legacy port","deprecated":true,"deprecationMessage":"Use port","default":80},"hostName":{"type":"string","default":"localhost"},"optTag":{"anyOf":[{"type":"string"},{"type":"null"}]},"valid_port":{"default":443,"allOf":[{"minimum":1,"type":"integer"},{"maximum":65535,"type":"integer"}]},"explicit_default":{"type":"integer","default":99},"renamedDefault":{"type":"integer","default":77},"described_default":{"type":"integer","description":"Described default","default":55}},"required":[]}}})";

  EXPECT_EQ(schema, expected) << schema;
}

TEST(json, test_schema_default_if_missing_non_default_constructible) {
  const auto schema =
      rfl::json::to_schema<WithNonDefaultConstructibleMember,
                           rfl::DefaultIfMissing>();

  EXPECT_EQ(schema.find("\"default\""), std::string::npos) << schema;
  EXPECT_NE(schema.find("\"required\":[]"), std::string::npos) << schema;
  EXPECT_NE(schema.find("\"port\":{\"type\":\"integer\"}"), std::string::npos)
      << schema;
}

TEST(json, test_schema_default_if_missing_non_writable_fields) {
  const auto schema =
      rfl::json::to_schema<WithNonWritableFields, rfl::DefaultIfMissing>();

  EXPECT_NE(schema.find(R"("port":{"type":"integer","default":8080})"),
            std::string::npos)
      << schema;
  EXPECT_NE(
      schema.find(
          R"("raw":{"$ref":"#/$defs/test_json_schema_default_if_missing__NonWritable"})"),
      std::string::npos)
      << schema;
  EXPECT_NE(
      schema.find(
          R"("described":{"$ref":"#/$defs/test_json_schema_default_if_missing__NonWritable","description":"desc"})"),
      std::string::npos)
      << schema;
}

TEST(json, test_schema_default_if_missing_throwing_constructors) {
  const auto schema1 =
      rfl::json::to_schema<ThrowingDefaultCtor, rfl::DefaultIfMissing>();
  EXPECT_EQ(schema1.find("\"default\""), std::string::npos) << schema1;
  EXPECT_NE(schema1.find(R"("port":{"type":"integer"})"), std::string::npos)
      << schema1;

  const auto schema2 =
      rfl::json::to_schema<ThrowingValidatorField, rfl::DefaultIfMissing>();
  EXPECT_EQ(schema2.find("\"default\""), std::string::npos) << schema2;
  EXPECT_NE(schema2.find(R"("port":{"type":"integer"})"), std::string::npos)
      << schema2;

  const auto schema3 =
      rfl::json::to_schema<ThrowingWithDefaultVal, rfl::DefaultIfMissing>();
  EXPECT_EQ(schema3.find("\"default\""), std::string::npos) << schema3;
  EXPECT_NE(schema3.find(R"("count":{"type":"integer"})"), std::string::npos)
      << schema3;

  const auto schema4 =
      rfl::json::to_schema<WithThrowingToGenericField, rfl::DefaultIfMissing>();
  EXPECT_NE(schema4.find(R"("port":{"type":"integer","default":8080})"),
            std::string::npos)
      << schema4;
  EXPECT_NE(
      schema4.find(
          R"("bad_serialize":{"$ref":"#/$defs/test_json_schema_default_if_missing__ThrowingToGeneric"})"),
      std::string::npos)
      << schema4;
}

struct WithReadOnlyReflector {
  int port = 8080;
  ReadOnlyReflected ro;
};

TEST(json, test_schema_default_if_missing_read_only_reflector) {
  const auto schema =
      rfl::json::to_schema<WithReadOnlyReflector, rfl::DefaultIfMissing>();

  EXPECT_NE(schema.find(R"("port":{"type":"integer","default":8080})"),
            std::string::npos)
      << schema;
  EXPECT_NE(
      schema.find(
          R"("ro":{"$ref":"#/$defs/test_json_schema_default_if_missing__ReadOnlyReflected"})"),
      std::string::npos)
      << schema;
}

TEST(json, test_schema_default_if_missing_no_field_names) {
  const auto schema = rfl::json::to_schema<Inner, rfl::DefaultIfMissing,
                                           rfl::NoFieldNames>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_json_schema_default_if_missing__Inner","$defs":{"test_json_schema_default_if_missing__Inner":{"type":"array","prefixItems":[{"type":"integer","default":3},{"type":"string","default":"fast"}],"items":false}}})";

  EXPECT_EQ(schema, expected) << schema;
}

}  // namespace test_json_schema_default_if_missing
