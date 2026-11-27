#include <gtest/gtest.h>

#include <array>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
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
  bool operator<(const NonWritable& _other) const {
    return copied < _other.copied;
  }
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

static_assert(
    !rfl::internal::is_safe_default_constructible_v<rfl::Box<NoDefaultCtor>>);
static_assert(!rfl::internal::is_safe_default_constructible_v<
              rfl::CopyableBox<NoDefaultCtor>>);
static_assert(
    !rfl::internal::is_safe_default_constructible_v<rfl::Ref<NoDefaultCtor>>);
static_assert(!std::is_default_constructible_v<rfl::Attribute<NoDefaultCtor>>);
static_assert(!std::is_default_constructible_v<rfl::Short<"a", NoDefaultCtor>>);
static_assert(
    !std::is_default_constructible_v<rfl::Positional<NoDefaultCtor>>);
static_assert(
    !std::is_default_constructible_v<rfl::Tuple<int, NoDefaultCtor>>);

struct WithBoxNoDefault {
  rfl::Box<NoDefaultCtor> boxed;
  int port = 8080;
};

struct WithRefNoDefault {
  rfl::Ref<NoDefaultCtor> ref;
  int port = 8080;
};

struct WithAttributeNoDefault {
  rfl::Attribute<NoDefaultCtor> attr;
  int port = 8080;
};

TEST(json, test_schema_default_if_missing_wrapped_no_default) {
  const auto box_schema =
      rfl::json::to_schema<WithBoxNoDefault, rfl::DefaultIfMissing>();
  EXPECT_EQ(box_schema.find("\"default\""), std::string::npos) << box_schema;
  EXPECT_NE(box_schema.find("\"required\":[]"), std::string::npos)
      << box_schema;
  EXPECT_NE(box_schema.find(R"("port":{"type":"integer"})"), std::string::npos)
      << box_schema;

  const auto ref_schema =
      rfl::json::to_schema<WithRefNoDefault, rfl::DefaultIfMissing>();
  EXPECT_EQ(ref_schema.find("\"default\""), std::string::npos) << ref_schema;
  EXPECT_NE(ref_schema.find("\"required\":[]"), std::string::npos)
      << ref_schema;
  EXPECT_NE(ref_schema.find(R"("port":{"type":"integer"})"), std::string::npos)
      << ref_schema;

  const auto attr_schema =
      rfl::json::to_schema<WithAttributeNoDefault, rfl::DefaultIfMissing>();
  EXPECT_EQ(attr_schema.find("\"default\""), std::string::npos) << attr_schema;
  EXPECT_NE(attr_schema.find("\"required\":[]"), std::string::npos)
      << attr_schema;
  EXPECT_NE(attr_schema.find(R"("port":{"type":"integer"})"), std::string::npos)
      << attr_schema;
}

struct AlwaysValidNonWritable {
  static rfl::Result<NonWritable> validate(const NonWritable& _v) {
    return _v;
  }
  template <class T>
  static rfl::parsing::schema::ValidationType to_schema() {
    return rfl::parsing::schema::ValidationType{
        rfl::parsing::schema::ValidationType::AllOf{}};
  }
};

struct NestedWithNonWritable {
  int inner_val = 7;
  NonWritable nw;
};

struct TaggedGoodAlt {
  int val = 1;
};

struct TaggedBadAlt {
  NonWritable nw;
};

struct WithWrappedNonWritable {
  int port = 8080;
  rfl::Box<NonWritable> boxed;
  rfl::Ref<NonWritable> ref;
  rfl::Commented<NonWritable> commented;
  rfl::Attribute<NonWritable> attr;
  rfl::Validator<NonWritable, AlwaysValidNonWritable> validated;
};

struct WithCompositeNonWritable {
  int port = 8080;
  std::vector<NonWritable> vec;
  std::deque<NonWritable> deq;
  std::set<NonWritable> set_nw;
  std::map<std::string, NonWritable> str_map;
  std::array<NonWritable, 2> arr;
  std::pair<int, NonWritable> pair_nw;
  std::tuple<int, NonWritable> std_tup;
  rfl::Tuple<int, NonWritable> rfl_tup;
  std::variant<int, NonWritable> std_var;
  rfl::Variant<int, NonWritable> rfl_var;
  rfl::TaggedUnion<"tag", TaggedGoodAlt, TaggedBadAlt> tagged_union;
  NestedWithNonWritable nested;
};

TEST(json, test_schema_default_if_missing_wrapped_and_composite_non_writable) {
  static_assert(!rfl::internal::CanWriteGeneric<NonWritable>);
  static_assert(!rfl::internal::CanWriteGeneric<rfl::Box<NonWritable>>);
  static_assert(!rfl::internal::CanWriteGeneric<rfl::Ref<NonWritable>>);
  static_assert(!rfl::internal::CanWriteGeneric<rfl::Commented<NonWritable>>);
  static_assert(!rfl::internal::CanWriteGeneric<rfl::Attribute<NonWritable>>);
  static_assert(
      !rfl::internal::CanWriteGeneric<
          rfl::Validator<NonWritable, AlwaysValidNonWritable>>);
  static_assert(!rfl::internal::CanWriteGeneric<std::vector<NonWritable>>);
  static_assert(!rfl::internal::CanWriteGeneric<std::deque<NonWritable>>);
  static_assert(!rfl::internal::CanWriteGeneric<std::set<NonWritable>>);
  static_assert(
      !rfl::internal::CanWriteGeneric<std::map<std::string, NonWritable>>);
  static_assert(!rfl::internal::CanWriteGeneric<std::array<NonWritable, 2>>);
  static_assert(!rfl::internal::CanWriteGeneric<std::pair<int, NonWritable>>);
  static_assert(!rfl::internal::CanWriteGeneric<std::tuple<int, NonWritable>>);
  static_assert(!rfl::internal::CanWriteGeneric<rfl::Tuple<int, NonWritable>>);
  static_assert(
      !rfl::internal::CanWriteGeneric<std::variant<int, NonWritable>>);
  static_assert(
      !rfl::internal::CanWriteGeneric<rfl::Variant<int, NonWritable>>);
  static_assert(
      !rfl::internal::CanWriteGeneric<
          rfl::TaggedUnion<"tag", TaggedGoodAlt, TaggedBadAlt>>);
  static_assert(!rfl::internal::CanWriteGeneric<NestedWithNonWritable>);
  static_assert(!rfl::internal::CanWriteGeneric<WithWrappedNonWritable>);
  static_assert(!rfl::internal::CanWriteGeneric<WithCompositeNonWritable>);
  static_assert(rfl::internal::CanWriteGeneric<rfl::Literal<"a", "b">>);
  static_assert(rfl::internal::CanWriteGeneric<rfl::Generic>);
  static_assert(rfl::internal::CanWriteGeneric<rfl::Skip<NonWritable>>);
  static_assert(
      !rfl::internal::CanWriteGeneric<rfl::SkipDeserialization<NonWritable>>);

  const auto wrapped_schema =
      rfl::json::to_schema<WithWrappedNonWritable, rfl::DefaultIfMissing>();
  EXPECT_NE(
      wrapped_schema.find(
          R"("test_json_schema_default_if_missing__WithWrappedNonWritable":{"type":"object","properties":{"port":{"type":"integer","default":8080},"boxed":{"$ref":"#/$defs/test_json_schema_default_if_missing__NonWritable"},"ref":{"$ref":"#/$defs/test_json_schema_default_if_missing__NonWritable"},"commented":{"$ref":"#/$defs/test_json_schema_default_if_missing__NonWritable"},"attr":{"$ref":"#/$defs/rfl__Attribute_test_json_schema_default_if_missing__NonWritable_"},"validated":{"allOf":[]}},"required":[]})"),
      std::string::npos)
      << wrapped_schema;

  const auto composite_schema =
      rfl::json::to_schema<WithCompositeNonWritable, rfl::DefaultIfMissing>();
  EXPECT_NE(
      composite_schema.find(R"("port":{"type":"integer","default":8080})"),
      std::string::npos)
      << composite_schema;
  // Writable fields inside nested/tagged definitions still get their own
  // defaults, while none of the non-writable composite fields on
  // WithCompositeNonWritable get a default.
  EXPECT_NE(
      composite_schema.find(R"("inner_val":{"type":"integer","default":7})"),
      std::string::npos)
      << composite_schema;
  const auto composite_def_pos = composite_schema.find(
      R"("test_json_schema_default_if_missing__WithCompositeNonWritable":)");
  ASSERT_NE(composite_def_pos, std::string::npos) << composite_schema;
  const auto first_composite_default =
      composite_schema.find("\"default\"", composite_def_pos);
  ASSERT_NE(first_composite_default, std::string::npos) << composite_schema;
  // Only `port` inside WithCompositeNonWritable gets a default.
  EXPECT_EQ(composite_schema.find("\"default\"", first_composite_default + 1),
            std::string::npos)
      << composite_schema;
}

struct Node {
  int v = 1;
  std::unique_ptr<Node> next;
};

struct BoxNode {
  int v = 2;
  std::optional<rfl::Box<BoxNode>> next;
};

struct NonWritableNode {
  int v = 3;
  NonWritable nw;
  std::unique_ptr<NonWritableNode> next;
};

TEST(json, test_schema_default_if_missing_recursive_structs) {
  static_assert(rfl::internal::CanWriteGeneric<Node>);
  static_assert(rfl::internal::CanWriteGeneric<BoxNode>);
  static_assert(!rfl::internal::CanWriteGeneric<NonWritableNode>);

  const auto node_schema =
      rfl::json::to_schema<Node, rfl::DefaultIfMissing>();
  EXPECT_NE(node_schema.find(R"("v":{"type":"integer","default":1})"),
            std::string::npos)
      << node_schema;
  EXPECT_NE(
      node_schema.find(
          R"("next":{"anyOf":[{"$ref":"#/$defs/test_json_schema_default_if_missing__Node"},{"type":"null"}]})"),
      std::string::npos)
      << node_schema;

  const auto box_node_schema =
      rfl::json::to_schema<BoxNode, rfl::DefaultIfMissing>();
  EXPECT_NE(box_node_schema.find(R"("v":{"type":"integer","default":2})"),
            std::string::npos)
      << box_node_schema;

  const auto nw_node_schema =
      rfl::json::to_schema<NonWritableNode, rfl::DefaultIfMissing>();
  EXPECT_NE(nw_node_schema.find(R"("v":{"type":"integer","default":3})"),
            std::string::npos)
      << nw_node_schema;
  EXPECT_NE(
      nw_node_schema.find(
          R"("nw":{"$ref":"#/$defs/test_json_schema_default_if_missing__NonWritable"})"),
      std::string::npos)
      << nw_node_schema;
}

namespace recursive_incomplete {

struct DT;
struct Leaf {
  int x = 0;
};
struct Node {
  rfl::Box<DT> lesser;
};
struct DT {
  std::variant<Leaf, Node> v;
};

struct Tree {
  struct Branch {
    rfl::Box<Tree> child;
  };
  std::optional<Branch> b;
};

struct DirectNode {
  int v = 1;
  rfl::Box<DirectNode> next;
};

}  // namespace recursive_incomplete

TEST(json, test_schema_default_if_missing_recursive_incomplete_box_ref) {
  using recursive_incomplete::DirectNode;
  using recursive_incomplete::DT;
  using recursive_incomplete::Leaf;
  using recursive_incomplete::Node;
  using recursive_incomplete::Tree;

  static_assert(rfl::internal::is_safe_default_constructible_v<DT>);
  static_assert(rfl::internal::is_safe_default_constructible_v<Node>);
  static_assert(rfl::internal::is_safe_default_constructible_v<Tree>);
  static_assert(rfl::internal::is_safe_default_constructible_v<Tree::Branch>);
  static_assert(!rfl::internal::is_safe_default_constructible_v<DirectNode>);
  static_assert(
      !rfl::internal::is_safe_default_constructible_v<WithBoxNoDefault>);
  static_assert(
      !rfl::internal::is_safe_default_constructible_v<WithRefNoDefault>);

  // Normal read/write roundtrip for forward-declared recursive variant + Box.
  const DT dt_val{Node{rfl::make_box<DT>(DT{Leaf{42}})}};
  const auto dt_json = rfl::json::write(dt_val);
  const auto dt_parsed = rfl::json::read<DT>(dt_json);
  ASSERT_TRUE(dt_parsed.has_value()) << dt_parsed.error().what();
  ASSERT_TRUE(std::holds_alternative<Node>(dt_parsed->v));
  ASSERT_TRUE(std::holds_alternative<Leaf>(std::get<Node>(dt_parsed->v).lesser->v));
  EXPECT_EQ(std::get<Leaf>(std::get<Node>(dt_parsed->v).lesser->v).x, 42);

  // Schema generation with DefaultIfMissing for forward-declared DT.
  const auto dt_schema = rfl::json::to_schema<DT, rfl::DefaultIfMissing>();
  EXPECT_NE(dt_schema.find(R"("x":{"type":"integer","default":0})"),
            std::string::npos)
      << dt_schema;
  EXPECT_NE(dt_schema.find(R"("default":{"x":0})"), std::string::npos)
      << dt_schema;

  // Normal read/write roundtrip for nested struct + Box + optional.
  const Tree tree_val{Tree::Branch{rfl::make_box<Tree>()}};
  const auto tree_json = rfl::json::write(tree_val);
  const auto tree_parsed = rfl::json::read<Tree>(tree_json);
  ASSERT_TRUE(tree_parsed.has_value()) << tree_parsed.error().what();
  ASSERT_TRUE(tree_parsed->b.has_value());
  EXPECT_FALSE(tree_parsed->b->child->b.has_value());

  // Schema generation with DefaultIfMissing for Tree.
  const auto tree_schema = rfl::json::to_schema<Tree, rfl::DefaultIfMissing>();
  EXPECT_NE(tree_schema.find(R"("required":[])"), std::string::npos)
      << tree_schema;

  // Cyclic DirectNode must compile and NOT stack-overflow; default is omitted.
  const auto direct_schema =
      rfl::json::to_schema<DirectNode, rfl::DefaultIfMissing>();
  EXPECT_EQ(direct_schema.find("\"default\""), std::string::npos)
      << direct_schema;
  EXPECT_NE(direct_schema.find(R"("v":{"type":"integer"})"), std::string::npos)
      << direct_schema;
  EXPECT_NE(direct_schema.find(R"("required":[])"), std::string::npos)
      << direct_schema;
}

}  // namespace test_json_schema_default_if_missing
