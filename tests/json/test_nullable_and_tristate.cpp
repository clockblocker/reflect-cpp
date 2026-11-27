#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <rfl/json.hpp>
#include <rfl/xml.hpp>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace test_nullable_and_tristate {

struct Expr {
  std::string nodeType = "Literal";
  std::string value = "0";
};

struct VariableDeclaration {
  std::string nodeType = "VariableDeclaration";
  std::vector<std::string> variables = {};
  rfl::Nullable<Expr> value = std::nullopt;
  std::optional<std::string> debug_comment = std::nullopt;
};

struct JsonRpcEnvelope {
  std::string jsonrpc = "2.0";
  std::string method = "";
  rfl::TriState<std::variant<int64_t, std::string>> id = rfl::omitted;
};

struct WrappedPresence {
  rfl::Description<"Always serialized nullable", rfl::Nullable<int>>
      explicit_null = std::nullopt;
  rfl::Rename<"reqId", rfl::TriState<int>> request_id = rfl::omitted;
  rfl::DefaultVal<rfl::Nullable<int>> with_default = 7;
  rfl::DefaultVal<rfl::TriState<int>> with_default_null = std::nullopt;
  rfl::DefaultVal<rfl::TriState<int>> with_default_omitted = rfl::omitted;
  rfl::Rename<"renamedOmitted", rfl::DefaultVal<rfl::TriState<int>>>
      renamed_default_omitted = rfl::omitted;
};

struct AddNullFieldsStruct {
  std::optional<int> a = std::nullopt;
  std::optional<std::string> b = "ok";
  rfl::TriState<int> c = rfl::omitted;
  rfl::TriState<int> d = std::nullopt;
  std::unique_ptr<int> ptr = nullptr;
  std::shared_ptr<int> sptr = nullptr;
  rfl::Skip<int> skipped = 42;
  rfl::SkipSerialization<int> skip_ser = 99;
};

struct XmlContainerAddNull {
  std::vector<int> items = {};
  std::map<std::string, int> mapping = {};
  std::string name = "x";
};

struct NonComparable {
  int v = 0;
};

struct MoveOnlyHolder {
  rfl::Nullable<std::unique_ptr<int>> ptr = std::nullopt;
  rfl::TriState<std::unique_ptr<int>> ts_ptr = rfl::omitted;
};

struct NonDefaultConstructible {
  int v;
  explicit NonDefaultConstructible(int _v) : v(_v) {}
  bool operator==(const NonDefaultConstructible&) const = default;
};

struct ThrowingCtor {
  int v;
  explicit ThrowingCtor(int _v) : v(_v) {
    if (_v < 0) {
      throw std::runtime_error("negative");
    }
  }
};

struct CliNested {
  int port = 80;
};

struct CliPresenceOpts {
  rfl::TriState<bool> verbose = rfl::omitted;
  rfl::Nullable<CliNested> server = std::nullopt;
};

TEST(json, test_nullable_always_emits_null_while_optional_omits) {
  VariableDeclaration decl;
  decl.variables = {"x"};
  const auto out = rfl::json::write(decl);
  EXPECT_EQ(
      out,
      R"({"nodeType":"VariableDeclaration","variables":["x"],"value":null})");

  const auto from_omitted =
      rfl::json::read<VariableDeclaration>(
          R"({"nodeType":"VariableDeclaration","variables":["x"]})")
          .value();
  EXPECT_TRUE(from_omitted.value.is_null());
  EXPECT_FALSE(from_omitted.value.has_value());

  const auto from_null =
      rfl::json::read<VariableDeclaration>(
          R"({"nodeType":"VariableDeclaration","variables":["x"],"value":null})")
          .value();
  EXPECT_TRUE(from_null.value.is_null());

  const auto from_val =
      rfl::json::read<VariableDeclaration>(
          R"({"nodeType":"VariableDeclaration","variables":["x"],"value":{"nodeType":"Literal","value":"42"}})")
          .value();
  ASSERT_TRUE(from_val.value.has_value());
  EXPECT_EQ(from_val.value->value, "42");
  EXPECT_EQ(
      rfl::json::write(from_val),
      R"({"nodeType":"VariableDeclaration","variables":["x"],"value":{"nodeType":"Literal","value":"42"}})");
}

TEST(json, test_tristate_jsonrpc_omitted_vs_null_vs_value) {
  const auto notif =
      rfl::json::read<JsonRpcEnvelope>(
          R"({"jsonrpc":"2.0","method":"notify"})")
          .value();
  EXPECT_TRUE(notif.id.is_omitted());
  EXPECT_FALSE(notif.id.is_present());
  EXPECT_FALSE(notif.id.is_null());
  EXPECT_FALSE(notif.id.has_value());
  EXPECT_EQ(rfl::json::write(notif),
            R"({"jsonrpc":"2.0","method":"notify"})");

  const auto null_req =
      rfl::json::read<JsonRpcEnvelope>(
          R"({"jsonrpc":"2.0","method":"ping","id":null})")
          .value();
  EXPECT_FALSE(null_req.id.is_omitted());
  EXPECT_TRUE(null_req.id.is_present());
  EXPECT_TRUE(null_req.id.is_null());
  EXPECT_FALSE(null_req.id.has_value());
  EXPECT_EQ(rfl::json::write(null_req),
            R"({"jsonrpc":"2.0","method":"ping","id":null})");

  const auto val_req =
      rfl::json::read<JsonRpcEnvelope>(
          R"({"jsonrpc":"2.0","method":"get","id":42})")
          .value();
  EXPECT_TRUE(val_req.id.is_present());
  EXPECT_TRUE(val_req.id.has_value());
  EXPECT_EQ(std::get<int64_t>(*val_req.id), 42);
  EXPECT_EQ(rfl::json::write(val_req),
            R"({"jsonrpc":"2.0","method":"get","id":42})");

  const auto with_dim =
      rfl::json::read<JsonRpcEnvelope, rfl::DefaultIfMissing>(
          R"({"method":"ping","id":null})")
          .value();
  EXPECT_TRUE(with_dim.id.is_null());
  const auto with_dim_omitted =
      rfl::json::read<JsonRpcEnvelope, rfl::DefaultIfMissing>(
          R"({"method":"ping"})")
          .value();
  EXPECT_TRUE(with_dim_omitted.id.is_omitted());
}

TEST(json, test_add_null_fields_processor_and_xml_empty_container) {
  static_assert(rfl::internal::add_null_fields_v<rfl::AddNullFields>);
  static_assert(rfl::internal::add_null_fields_v<
                rfl::Processors<rfl::Processors<rfl::AddNullFields>>>);

  AddNullFieldsStruct s;
  EXPECT_EQ(rfl::json::write(s), R"({"b":"ok","d":null})");
  EXPECT_EQ(rfl::json::write<rfl::AddNullFields>(s),
            R"({"a":null,"b":"ok","d":null,"ptr":null,"sptr":null})");
  // Nested Processors<...> pack must still honor AddNullFields.
  EXPECT_EQ(rfl::json::write<rfl::Processors<rfl::AddNullFields>>(s),
            R"({"a":null,"b":"ok","d":null,"ptr":null,"sptr":null})");

  const auto read_back =
      rfl::json::read<AddNullFieldsStruct, rfl::AddNullFields>(R"({"b":"hi"})")
          .value();
  EXPECT_FALSE(read_back.a.has_value());
  EXPECT_EQ(read_back.b, "hi");
  EXPECT_TRUE(read_back.c.is_omitted());
  EXPECT_TRUE(read_back.d.is_omitted());
  EXPECT_EQ(read_back.ptr, nullptr);
  EXPECT_EQ(read_back.sptr, nullptr);

  // Under NoOptionals, omitted Nullable and TriState keys must fail on read.
  EXPECT_FALSE(
      (rfl::json::read<JsonRpcEnvelope, rfl::NoOptionals>(
           R"({"jsonrpc":"2.0","method":"notify"})"))
          .has_value());
  EXPECT_FALSE(
      (rfl::json::read<VariableDeclaration, rfl::NoOptionals>(
           R"({"nodeType":"VariableDeclaration","variables":[]})"))
          .has_value());
  // Invalid value type propagates parse error.
  EXPECT_FALSE(
      rfl::json::read<WrappedPresence>(R"({"explicit_null":"not_an_int"})")
          .has_value());
  EXPECT_FALSE(
      rfl::json::read<WrappedPresence>(R"({"reqId":"not_an_int"})")
          .has_value());

  // Under XML (_ignore_empty_containers=true), AddNullFields must NOT treat
  // non-nullable empty vectors or maps as nullable fields.
  XmlContainerAddNull xc;
  const auto xml_plain = rfl::xml::write<"root">(xc);
  const auto xml_anf = rfl::xml::write<"root", rfl::AddNullFields>(xc);
  EXPECT_EQ(xml_anf, xml_plain);
}

TEST(json, test_nullable_and_tristate_schema_and_wrappers) {
  // DefaultVal<TriState> in State::omitted must omit on write (not emit null).
  WrappedPresence wp;
  EXPECT_EQ(
      rfl::json::write(wp),
      R"({"explicit_null":null,"with_default":7,"with_default_null":null})");

  const auto schema = rfl::json::to_schema<VariableDeclaration>();
  EXPECT_NE(
      schema.find(
          R"("value":{"anyOf":[{"$ref":"#/$defs/test_nullable_and_tristate__Expr"},{"type":"null"}]})"),
      std::string::npos);
  EXPECT_NE(schema.find(R"("required":["nodeType","variables"])"),
            std::string::npos);

  const auto rpc_schema = rfl::json::to_schema<JsonRpcEnvelope>();
  EXPECT_NE(rpc_schema.find(R"("type":"null")"), std::string::npos);
  EXPECT_NE(rpc_schema.find(R"("required":["jsonrpc","method"])"),
            std::string::npos);

  const auto wrapped_schema = rfl::json::to_schema<WrappedPresence>();
  EXPECT_NE(wrapped_schema.find("\"description\":\"Always serialized nullable\""),
            std::string::npos);
  EXPECT_NE(wrapped_schema.find("\"default\":7"), std::string::npos);
  EXPECT_NE(wrapped_schema.find("\"default\":null"), std::string::npos);
  EXPECT_NE(wrapped_schema.find("\"reqId\""), std::string::npos);
  // DefaultVal<TriState<int>> with State::omitted must NOT emit "default":null,
  // including when wrapped inside rfl::Rename.
  const auto pos_omitted = wrapped_schema.find("\"with_default_omitted\"");
  ASSERT_NE(pos_omitted, std::string::npos);
  const auto omitted_slice = wrapped_schema.substr(pos_omitted, 120);
  EXPECT_EQ(omitted_slice.find("\"default\""), std::string::npos);

  const auto pos_renamed_omitted = wrapped_schema.find("\"renamedOmitted\"");
  ASSERT_NE(pos_renamed_omitted, std::string::npos);
  const auto renamed_slice = wrapped_schema.substr(pos_renamed_omitted, 120);
  EXPECT_EQ(renamed_slice.find("\"default\""), std::string::npos);
}

TEST(json, test_nullable_and_tristate_type_safety_and_exception_safety) {
  // Implicit bool conversions from optional-like types are forbidden.
  static_assert(!std::is_convertible_v<std::optional<bool>, rfl::TriState<bool>>);
  static_assert(!std::is_convertible_v<rfl::Nullable<bool>, rfl::TriState<bool>>);
  static_assert(!std::is_convertible_v<std::optional<int>, rfl::Nullable<bool>>);
  static_assert(!std::is_convertible_v<rfl::TriState<bool>, rfl::Nullable<bool>>);
  static_assert(!std::is_convertible_v<int, rfl::Nullable<std::vector<int>>>);
  static_assert(std::is_constructible_v<rfl::Nullable<std::vector<int>>, int>);

  // Move-only types properly disable copy construction traits.
  static_assert(
      !std::is_copy_constructible_v<rfl::Nullable<std::unique_ptr<int>>>);
  static_assert(
      !std::is_copy_constructible_v<rfl::TriState<std::unique_ptr<int>>>);
  static_assert(!std::is_constructible_v<rfl::Nullable<std::unique_ptr<int>>,
                                         const std::unique_ptr<int>&>);
  static_assert(!std::is_constructible_v<rfl::TriState<std::unique_ptr<int>>,
                                         const std::unique_ptr<int>&>);

  // Move-only JSON round-trip.
  const auto moved =
      rfl::json::read<MoveOnlyHolder>(R"({"ptr":5,"ts_ptr":null})").value();
  ASSERT_TRUE(moved.ptr.has_value());
  EXPECT_EQ(**moved.ptr, 5);
  EXPECT_TRUE(moved.ts_ptr.is_null());
  EXPECT_EQ(rfl::json::write(moved), R"({"ptr":5,"ts_ptr":null})");

  // Non-default-constructible T works in Nullable and TriState.
  rfl::Nullable<NonDefaultConstructible> ndc_n;
  EXPECT_TRUE(ndc_n.is_null());
  ndc_n.emplace(42);
  EXPECT_EQ(ndc_n->v, 42);

  rfl::TriState<NonDefaultConstructible> ndc_t;
  EXPECT_TRUE(ndc_t.is_omitted());
  ndc_t.emplace(99);
  EXPECT_EQ(ndc_t->v, 99);

  // Exception safety in TriState::emplace preserves invariant.
  rfl::TriState<ThrowingCtor> ts_throw;
  ts_throw.emplace(10);
  ASSERT_TRUE(ts_throw.has_value());
  EXPECT_THROW(ts_throw.emplace(-1), std::runtime_error);
  EXPECT_FALSE(ts_throw.has_value());
  EXPECT_TRUE(ts_throw.is_omitted());

  static_assert(!std::equality_comparable<rfl::TriState<NonComparable>>);
  static_assert(!std::equality_comparable<rfl::Nullable<NonComparable>>);

  // Unambiguous comparisons and braced empty assignment.
  rfl::Nullable<std::string> ns;
  EXPECT_FALSE(ns == "abc");
  ns = "abc";
  EXPECT_TRUE(ns == "abc");
  ns = {};
  EXPECT_TRUE(ns.is_null());

  rfl::TriState<std::string> ts = "hi";
  ts = {};
  EXPECT_TRUE(ts.is_omitted());
}

TEST(json, test_nullable_and_tristate_generic_and_cli) {
  const auto env =
      rfl::json::read<JsonRpcEnvelope>(
          R"({"jsonrpc":"2.0","method":"ping","id":null})")
          .value();
  const auto g = rfl::to_generic(env);
  const auto from_g = rfl::from_generic<JsonRpcEnvelope>(g).value();
  EXPECT_TRUE(from_g.id.is_null());

  // CLI flag + nested struct option.
  const std::vector<std::string> argv = {"--verbose", "--server.port=9090"};
  const auto cli_res = rfl::cli::read<CliPresenceOpts>(argv).value();
  ASSERT_TRUE(cli_res.verbose.has_value());
  EXPECT_TRUE(*cli_res.verbose);
  ASSERT_TRUE(cli_res.server.has_value());
  EXPECT_EQ(cli_res.server->port, 9090);
}

}  // namespace test_nullable_and_tristate
