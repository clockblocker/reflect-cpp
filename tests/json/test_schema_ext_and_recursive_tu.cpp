#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <tuple>
#include <vector>

namespace test_schema_ext_and_recursive_tu {

// --- #115: Boolean "type" before "default" ---
struct BoolDefaultStruct {
  rfl::DefaultVal<bool> enabled = true;
  rfl::Description<"Debug flag", rfl::DefaultVal<bool>> debug = false;
};

// --- #100: $defs naming via SchemaName<T> and Metadata<T>::schema_name ---
struct CustomNamedA {
  int value = 1;
};

struct CustomNamedB {
  std::string label = "ok";
};

// --- #100: Custom wrapper x-* extension hooks (inline & $defs) ---
template <class T>
struct InlineConfigOption {
  using ReflectionType = T;
  T val{};
  const T& reflection() const { return val; }
  InlineConfigOption() = default;
  InlineConfigOption(T _v) : val(std::move(_v)) {}
};

template <class T>
struct DefsConfigOption {
  using ReflectionType = T;
  T val{};
  const T& reflection() const { return val; }
  DefsConfigOption() = default;
  DefsConfigOption(T _v) : val(std::move(_v)) {}
};

struct WrapperHost {
  InlineConfigOption<int> timeout = 30;
  DefsConfigOption<std::string> endpoint = std::string("https://example.com");
};

// --- #100: title, examples, x-* via Metadata<T> + field_annotations ---
struct AnnotatedConfig {
  int port = 8080;
  std::string host = "127.0.0.1";
};

// --- #100: Integer width bounds (#683) ---
struct IntegerWidths {
  int8_t i8;
  uint8_t u8;
  int16_t i16;
  uint16_t u16;
  int32_t i32;
  uint32_t u32;
  int64_t i64;
  uint64_t u64;
};

// --- #100: Tuple bounds ---
struct TupleHolder {
  std::tuple<int, std::string, bool> triple;
  std::pair<int, double> pair_val;
};

// --- #100: OmitEmptyRequired ---
struct AllOptionalStruct {
  std::optional<int> a;
  std::optional<std::string> b;
};

// --- #100: Per-struct DefaultIfMissing ---
struct StrictNested {
  int must_exist;
};

struct LooseStructTrait {
  int retries = 3;
  std::string mode = "fast";
  StrictNested required_child;
};

struct LooseStructMeta {
  int workers = 4;
  bool verbose = false;
};

// --- #103: Direct recursive TaggedUnion via rfl::Box ---
struct Expr;

struct LitExpr {
  int value;
};

struct NegExpr {
  rfl::Box<Expr> operand;
};

struct BinExpr {
  std::string op;
  rfl::Box<Expr> lhs;
  rfl::Box<Expr> rhs;
};

struct Expr : rfl::TaggedUnion<"kind", LitExpr, NegExpr, BinExpr> {
  using Base = rfl::TaggedUnion<"kind", LitExpr, NegExpr, BinExpr>;
  using Base::Base;
  Expr(Base&& _b) : Base(std::move(_b)) {}
};

// --- #103: Direct recursive TaggedUnion via std::vector ---
struct TreeNode;

struct LeafNode {
  std::string text;
};

struct GroupNode {
  std::vector<TreeNode> children;
};

struct TreeNode : rfl::TaggedUnion<"type", LeafNode, GroupNode> {
  using Base = rfl::TaggedUnion<"type", LeafNode, GroupNode>;
  using Base::Base;
  TreeNode(const Base& _b) : Base(_b) {}
  TreeNode(Base&& _b) : Base(std::move(_b)) {}
};

// --- #103: Named TaggedUnion subclass with SchemaName ---
struct YulExpr;

struct YulLit {
  std::string value;
};

struct YulCall {
  std::string fn_name;
  std::vector<YulExpr> arguments;
};

struct YulExpr : rfl::TaggedUnion<"nodeType", YulLit, YulCall> {
  using Base = rfl::TaggedUnion<"nodeType", YulLit, YulCall>;
  using Base::Base;
  YulExpr(const Base& _b) : Base(_b) {}
  YulExpr(Base&& _b) : Base(std::move(_b)) {}
};

// --- #103: Mutually recursive TaggedUnions (Stmt <-> ValueExpr) ---
struct Stmt;
struct ValueExpr;

struct IntLit {
  int v;
};

struct BlockExpr {
  rfl::Box<Stmt> body;
};

struct ValueExpr : rfl::TaggedUnion<"exprKind", IntLit, BlockExpr> {
  using Base = rfl::TaggedUnion<"exprKind", IntLit, BlockExpr>;
  using Base::Base;
  ValueExpr(Base&& _b) : Base(std::move(_b)) {}
};

struct ReturnStmt {
  ValueExpr value;
};

struct SeqStmt {
  std::vector<Stmt> items;
};

struct Stmt : rfl::TaggedUnion<"stmtKind", ReturnStmt, SeqStmt> {
  using Base = rfl::TaggedUnion<"stmtKind", ReturnStmt, SeqStmt>;
  using Base::Base;
  Stmt(Base&& _b) : Base(std::move(_b)) {}
};

// --- #103: Bare TaggedUnion inside recursive wrapper struct ---
struct AstWrapper;

struct AstLeaf {
  int num;
};

struct AstBranch {
  rfl::Box<AstWrapper> left;
  rfl::Box<AstWrapper> right;
};

using AstUnion = rfl::TaggedUnion<"astKind", AstLeaf, AstBranch>;

struct AstWrapper {
  AstUnion node;
};

// --- #103: Bare TaggedUnion alias with SchemaName + Metadata description ---
struct Tri {
  double base;
};
struct Rect {
  double width;
};
using AnnotatedShape = rfl::TaggedUnion<"kind", Tri, Rect>;

// --- #100: Validator + IntegerBounds ---
struct ValidatedPort {
  rfl::Validator<uint16_t, rfl::Minimum<1>> port;
};

// --- #100: Per-struct DefaultIfMissing with read-only / throwing-default field ---
struct ReadOnlyInner {
  int v = 7;
};

struct LooseWithReadOnly {
  int a = 1;
  ReadOnlyInner ro;
};

// --- #103: Non-recursive TaggedUnion (must remain inline) ---
struct Circle {
  double radius;
};
struct Square {
  double side;
};
using Shape = rfl::TaggedUnion<"shape", Circle, Square>;
struct Drawing {
  Shape primary;
};

}  // namespace test_schema_ext_and_recursive_tu

namespace rfl {

template <>
struct SchemaName<test_schema_ext_and_recursive_tu::CustomNamedA> {
  static constexpr std::string_view value = "CleanExportA";
};

template <>
struct Metadata<test_schema_ext_and_recursive_tu::CustomNamedB> {
  static constexpr std::string_view schema_name = "CleanExportB";
};

template <class T>
struct SchemaExtension<test_schema_ext_and_recursive_tu::InlineConfigOption<T>> {
  static constexpr bool inline_schema = true;
  static SchemaAnnotations annotations() {
    SchemaAnnotations ann;
    ann.title = "Inline Config Option";
    ann.extensions["x-source"] = Generic("env-or-file");
    ann.extensions["x-hot-reload"] = Generic(true);
    return ann;
  }
};

template <class T>
struct SchemaExtension<test_schema_ext_and_recursive_tu::DefsConfigOption<T>> {
  static Object<Generic> extensions() {
    Object<Generic> ext;
    ext["x-scope"] = Generic("service");
    return ext;
  }
};

template <>
struct SchemaName<
    test_schema_ext_and_recursive_tu::DefsConfigOption<std::string>> {
  static constexpr std::string_view value = "EndpointOption";
};

template <>
struct Metadata<test_schema_ext_and_recursive_tu::AnnotatedConfig> {
  static constexpr std::string_view schema_name = "AnnotatedConfig";
  static constexpr std::string_view title = "Server Annotated Config";
  static constexpr std::string_view description = "Root server settings";
  static std::vector<Generic> examples() {
    Generic::Object ex;
    ex["port"] = 9090;
    ex["host"] = "0.0.0.0";
    return {Generic(std::move(ex))};
  }
  static Object<Generic> extensions() {
    Object<Generic> ext;
    ext["x-schema-version"] = Generic(2);
    return ext;
  }
  static constexpr std::array<FieldMeta, 2> fields = {{
      {.name = "port", .description = "Listening TCP port", .title = "Port"},
      {.name = "host", .description = "Bind interface"},
  }};
  static Object<SchemaAnnotations> field_annotations() {
    Object<SchemaAnnotations> out;
    SchemaAnnotations host_ann;
    host_ann.title = "Bind Host";
    host_ann.examples = std::vector<Generic>{Generic("127.0.0.1"), Generic("0.0.0.0")};
    host_ann.extensions["x-cli-flag"] = Generic("--host");
    out["host"] = std::move(host_ann);
    return out;
  }
};

template <>
struct DefaultIfMissingFor<test_schema_ext_and_recursive_tu::LooseStructTrait>
    : std::true_type {};

template <>
struct Metadata<test_schema_ext_and_recursive_tu::LooseStructMeta> {
  static constexpr bool default_if_missing = true;
};

template <>
struct SchemaName<test_schema_ext_and_recursive_tu::YulExpr> {
  static constexpr std::string_view value = "YulExpression";
};

template <>
struct SchemaName<test_schema_ext_and_recursive_tu::AnnotatedShape> {
  static constexpr std::string_view value = "AnnotatedShape";
};

template <>
struct Metadata<test_schema_ext_and_recursive_tu::AnnotatedShape> {
  static constexpr std::string_view description = "Tagged union of shapes";
};

template <>
struct Reflector<test_schema_ext_and_recursive_tu::ReadOnlyInner> {
  using ReflType = int;
  static test_schema_ext_and_recursive_tu::ReadOnlyInner to(int _v) {
    return {_v};
  }
};

template <>
struct DefaultIfMissingFor<test_schema_ext_and_recursive_tu::LooseWithReadOnly>
    : std::true_type {};

}  // namespace rfl

namespace test_schema_ext_and_recursive_tu {

TEST(json, test_issue_115_boolean_type_before_default) {
  const auto schema = rfl::json::to_schema<BoolDefaultStruct>();
  const auto enabled_pos = schema.find(R"("enabled":{"type":"boolean","default":true})");
  EXPECT_NE(enabled_pos, std::string::npos) << schema;
  const auto debug_pos = schema.find(R"("debug":{"type":"boolean","description":"Debug flag","default":false})");
  EXPECT_NE(debug_pos, std::string::npos) << schema;
}

TEST(json, test_schema_name_overrides) {
  const auto schema_a = rfl::json::to_schema<CustomNamedA>();
  EXPECT_NE(schema_a.find(R"("$ref":"#/$defs/CleanExportA")"), std::string::npos) << schema_a;
  EXPECT_NE(schema_a.find(R"("$defs":{"CleanExportA":)"), std::string::npos) << schema_a;

  const auto schema_b = rfl::json::to_schema<CustomNamedB>();
  EXPECT_NE(schema_b.find(R"("$ref":"#/$defs/CleanExportB")"), std::string::npos) << schema_b;
  EXPECT_NE(schema_b.find(R"("$defs":{"CleanExportB":)"), std::string::npos) << schema_b;
}

TEST(json, test_schema_extension_custom_wrappers) {
  const auto schema = rfl::json::to_schema<WrapperHost>();
  // Inline wrapper emits annotations directly on property without polluting $defs.
  EXPECT_NE(schema.find(R"("timeout":{"type":"integer","title":"Inline Config Option","x-source":"env-or-file","x-hot-reload":true})"),
            std::string::npos)
      << schema;
  EXPECT_EQ(schema.find("InlineConfigOption"), std::string::npos) << schema;
  // Non-inline wrapper emits $defs entry with SchemaName + x-scope.
  EXPECT_NE(schema.find(R"("endpoint":{"$ref":"#/$defs/EndpointOption"})"),
            std::string::npos)
      << schema;
  EXPECT_NE(schema.find(R"("EndpointOption":{"type":"string","x-scope":"service"})"),
            std::string::npos)
      << schema;
}

TEST(json, test_schema_annotations_title_examples_extensions) {
  const auto schema = rfl::json::to_schema<AnnotatedConfig>();
  const auto parsed = rfl::json::read<rfl::Generic>(schema);
  ASSERT_TRUE(parsed) << schema;

  EXPECT_NE(schema.find(R"("title":"Server Annotated Config")"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("description":"Root server settings")"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("examples":[{"port":9090,"host":"0.0.0.0"}])"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("x-schema-version":2)"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("title":"Port")"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("title":"Bind Host")"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("examples":["127.0.0.1","0.0.0.0"])"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("x-cli-flag":"--host")"), std::string::npos) << schema;
}

TEST(json, test_integer_bounds_processor) {
  // Without IntegerBounds: no minimum/maximum emitted.
  const auto plain = rfl::json::to_schema<IntegerWidths>();
  EXPECT_EQ(plain.find("\"minimum\""), std::string::npos) << plain;
  EXPECT_EQ(plain.find("\"maximum\""), std::string::npos) << plain;

  // With IntegerBounds: exact C++ width bounds emitted for 8..64-bit signed & unsigned.
  const auto bounded = rfl::json::to_schema<IntegerWidths, rfl::IntegerBounds>();
  EXPECT_NE(bounded.find(R"("i8":{"type":"integer","minimum":-128,"maximum":127})"), std::string::npos) << bounded;
  EXPECT_NE(bounded.find(R"("u8":{"type":"integer","minimum":0,"maximum":255})"), std::string::npos) << bounded;
  EXPECT_NE(bounded.find(R"("i16":{"type":"integer","minimum":-32768,"maximum":32767})"), std::string::npos) << bounded;
  EXPECT_NE(bounded.find(R"("u16":{"type":"integer","minimum":0,"maximum":65535})"), std::string::npos) << bounded;
  EXPECT_NE(bounded.find(R"("i32":{"type":"integer","minimum":-2147483648,"maximum":2147483647})"), std::string::npos) << bounded;
  EXPECT_NE(bounded.find(R"("u32":{"type":"integer","minimum":0,"maximum":4294967295})"), std::string::npos) << bounded;
  EXPECT_NE(bounded.find(R"("i64":{"type":"integer","minimum":-9223372036854775808,"maximum":9223372036854775807})"), std::string::npos) << bounded;
  EXPECT_NE(bounded.find(R"("u64":{"type":"integer","minimum":0,"maximum":18446744073709551615})"), std::string::npos) << bounded;
}

TEST(json, test_tuple_bounds_and_omit_empty_required) {
  const auto plain_tuple = rfl::json::to_schema<TupleHolder>();
  EXPECT_EQ(plain_tuple.find("\"minItems\""), std::string::npos) << plain_tuple;

  const auto bounded_tuple = rfl::json::to_schema<TupleHolder, rfl::TupleBounds>();
  EXPECT_NE(bounded_tuple.find(R"("items":false,"minItems":3,"maxItems":3)"), std::string::npos) << bounded_tuple;
  EXPECT_NE(bounded_tuple.find(R"("items":false,"minItems":2,"maxItems":2)"), std::string::npos) << bounded_tuple;

  const auto with_empty_req = rfl::json::to_schema<AllOptionalStruct>();
  EXPECT_NE(with_empty_req.find(R"("required":[])"), std::string::npos) << with_empty_req;

  const auto without_empty_req = rfl::json::to_schema<AllOptionalStruct, rfl::OmitEmptyRequired>();
  EXPECT_EQ(without_empty_req.find(R"("required")"), std::string::npos) << without_empty_req;
}

TEST(json, test_per_struct_default_if_missing) {
  // LooseStructTrait opts into DefaultIfMissing, while StrictNested does NOT!
  const auto ok_res = rfl::json::read<LooseStructTrait>(
      R"({"required_child":{"must_exist":42}})");
  ASSERT_TRUE(ok_res) << ok_res.error().what();
  EXPECT_EQ(ok_res->retries, 3);
  EXPECT_EQ(ok_res->mode, "fast");
  EXPECT_EQ(ok_res->required_child.must_exist, 42);

  // Omitting StrictNested::must_exist still fails because DefaultIfMissing is per-struct!
  const auto bad_res = rfl::json::read<LooseStructTrait>(
      R"({"required_child":{}})");
  EXPECT_FALSE(bad_res);

  // Schema emits defaults on LooseStructTrait, NOT on StrictNested.
  const auto schema = rfl::json::to_schema<LooseStructTrait>();
  EXPECT_NE(schema.find(R"("retries":{"type":"integer","default":3})"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("mode":{"type":"string","default":"fast"})"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("required":["must_exist"])"), std::string::npos) << schema;

  // Metadata<T>::default_if_missing also works for read + schema.
  const auto meta_res = rfl::json::read<LooseStructMeta>("{}");
  ASSERT_TRUE(meta_res) << meta_res.error().what();
  EXPECT_EQ(meta_res->workers, 4);
  EXPECT_EQ(meta_res->verbose, false);

  const auto meta_schema = rfl::json::to_schema<LooseStructMeta, rfl::OmitEmptyRequired>();
  EXPECT_NE(meta_schema.find(R"("workers":{"type":"integer","default":4})"), std::string::npos) << meta_schema;
  EXPECT_NE(meta_schema.find(R"("verbose":{"type":"boolean","default":false})"), std::string::npos) << meta_schema;
  EXPECT_EQ(meta_schema.find(R"("required")"), std::string::npos) << meta_schema;
}

TEST(json, test_issue_103_recursive_tagged_union_box) {
  const auto schema = rfl::json::to_schema<Expr>();
  const auto parsed = rfl::json::read<rfl::Generic>(schema);
  ASSERT_TRUE(parsed) << schema;
  // Must break cycle via $defs and $ref, with oneOf inside the hoisted definition.
  EXPECT_NE(schema.find(R"("$ref":"#/$defs/)"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("oneOf":[)"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("operand":{"$ref":"#/$defs/)"), std::string::npos) << schema;

  // Verify JSON round-trip still works on recursive Expr.
  const Expr expr = BinExpr{
      .op = "+",
      .lhs = rfl::make_box<Expr>(LitExpr{1}),
      .rhs = rfl::make_box<Expr>(NegExpr{
          .operand = rfl::make_box<Expr>(LitExpr{2})}),
  };
  const auto serialized = rfl::json::write(expr);
  const auto round_tripped = rfl::json::read<Expr>(serialized);
  ASSERT_TRUE(round_tripped) << round_tripped.error().what();
  EXPECT_EQ(rfl::json::write(*round_tripped), serialized);
}

TEST(json, test_issue_103_recursive_tagged_union_vector) {
  const auto schema = rfl::json::to_schema<TreeNode>();
  const auto parsed = rfl::json::read<rfl::Generic>(schema);
  ASSERT_TRUE(parsed) << schema;
  EXPECT_NE(schema.find(R"("$ref":"#/$defs/)"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("children":{"type":"array","items":{"$ref":"#/$defs/)"),
            std::string::npos)
      << schema;
}

TEST(json, test_issue_103_mutually_recursive_tagged_unions) {
  const auto schema = rfl::json::to_schema<Stmt>();
  const auto parsed = rfl::json::read<rfl::Generic>(schema);
  ASSERT_TRUE(parsed) << schema;
  EXPECT_NE(schema.find(R"("$ref":"#/$defs/)"), std::string::npos) << schema;
  EXPECT_NE(schema.find("Stmt"), std::string::npos) << schema;
  EXPECT_NE(schema.find("ValueExpr"), std::string::npos) << schema;
}

TEST(json, test_issue_103_bare_tagged_union_in_recursive_wrapper) {
  const auto schema = rfl::json::to_schema<AstWrapper>();
  const auto parsed = rfl::json::read<rfl::Generic>(schema);
  ASSERT_TRUE(parsed) << schema;
  EXPECT_NE(schema.find(R"("$ref":"#/$defs/)"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("oneOf":[)"), std::string::npos) << schema;
}

TEST(json, test_issue_103_recursive_named_subclass_tagged_union) {
  const auto schema = rfl::json::to_schema<YulExpr>();
  const auto parsed = rfl::json::read<rfl::Generic>(schema);
  ASSERT_TRUE(parsed) << schema;
  EXPECT_NE(schema.find(R"("$ref":"#/$defs/YulExpression")"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("$defs":{)"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("YulExpression":{"oneOf":[)"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("arguments":{"type":"array","items":{"$ref":"#/$defs/YulExpression"}})"),
            std::string::npos)
      << schema;

  // Verify round-trip of named subclass YulExpr.
  const YulExpr call = YulCall{
      .fn_name = "add",
      .arguments = {YulExpr(YulLit{"1"}), YulExpr(YulLit{"2"})},
  };
  const auto json_str = rfl::json::write(call);
  const auto parsed_call = rfl::json::read<YulExpr>(json_str);
  ASSERT_TRUE(parsed_call) << parsed_call.error().what();
  EXPECT_EQ(rfl::json::write(*parsed_call), json_str);
}

TEST(json, test_issue_103_non_recursive_tagged_union_stays_inline) {
  const auto schema = rfl::json::to_schema<Drawing>();
  EXPECT_NE(schema.find(R"("primary":{"oneOf":[)"), std::string::npos) << schema;
  EXPECT_EQ(schema.find("TaggedUnion"), std::string::npos) << schema;
}

TEST(json, test_issue_103_bare_tagged_union_with_schema_name_and_description) {
  const auto schema = rfl::json::to_schema<AnnotatedShape>();
  const auto parsed = rfl::json::read<rfl::Generic>(schema);
  ASSERT_TRUE(parsed) << schema;
  // Must NOT be a degenerate self-reference; $defs["AnnotatedShape"] must hold oneOf.
  EXPECT_NE(schema.find(R"("$ref":"#/$defs/AnnotatedShape")"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("AnnotatedShape":{"description":"Tagged union of shapes","oneOf":[)"),
            std::string::npos)
      << schema;
  EXPECT_EQ(schema.find(R"("AnnotatedShape":{"$ref")"), std::string::npos) << schema;
}

TEST(json, test_issue_100_integer_bounds_preserved_with_validator) {
  const auto schema = rfl::json::to_schema<ValidatedPort, rfl::IntegerBounds>();
  // Both uint16_t width bounds (0..65535) and Minimum<1> must be preserved in allOf.
  EXPECT_NE(schema.find(R"("allOf":[{"type":"integer","minimum":0,"maximum":65535},{"minimum":1,"type":"integer"}])"),
            std::string::npos)
      << schema;
}

TEST(json, test_issue_100_per_struct_default_if_missing_non_writable_not_required) {
  const auto read_res = rfl::json::read<LooseWithReadOnly>("{}");
  ASSERT_TRUE(read_res) << read_res.error().what();
  EXPECT_EQ(read_res->a, 1);
  EXPECT_EQ(read_res->ro.v, 7);

  const auto schema = rfl::json::to_schema<LooseWithReadOnly>();
  EXPECT_NE(schema.find(R"("required":[])"), std::string::npos) << schema;
  EXPECT_EQ(schema.find(R"("required":["ro"])"), std::string::npos) << schema;
}

}  // namespace test_schema_ext_and_recursive_tu
