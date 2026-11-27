#include <gtest/gtest.h>

#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <rfl/json.hpp>
#include <rfl/nlohmann.hpp>
#include <rfl/xml.hpp>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace test_nullable_and_tristate {

struct YulExpr {
  std::string nodeType = "YulLiteral";
  std::string value = "0";
};

struct YulVariableDeclaration {
  std::string nodeType = "YulVariableDeclaration";
  std::vector<std::string> variables = {};
  rfl::Nullable<YulExpr> value = std::nullopt;
  std::optional<std::string> debug_comment = std::nullopt;
};

struct JsonRpcEnvelope {
  std::string jsonrpc = "2.0";
  std::string method = "";
  rfl::TriState<std::variant<int64_t, std::string>> id = rfl::omitted;
};

struct NestedLayer {
  rfl::TriState<int> width = rfl::omitted;
  rfl::TriState<std::string> mode = rfl::omitted;
};

struct LayeredConfig {
  rfl::TriState<std::string> build_dir = rfl::omitted;
  rfl::TriState<int> threads = rfl::omitted;
  rfl::TriState<NestedLayer> formatting = rfl::omitted;
  rfl::Nullable<std::string> cache_path = std::nullopt;
  std::optional<int> legacy_opt = std::nullopt;
};

struct WrappedPresence {
  rfl::Description<"Always serialized nullable", rfl::Nullable<int>>
      explicit_null = std::nullopt;
  rfl::Rename<"reqId", rfl::TriState<int>> request_id = rfl::omitted;
  rfl::DefaultVal<rfl::Nullable<int>> with_default = 7;
  rfl::DefaultVal<rfl::TriState<int>> with_default_null = std::nullopt;
  rfl::DefaultVal<rfl::TriState<int>> with_default_omitted = rfl::omitted;
};

struct AliasedPresence {
  rfl::Nullable<rfl::Alias<"old_n", int>> n = std::nullopt;
  rfl::TriState<rfl::Alias<"old_t", int>> t = rfl::omitted;
};

struct AddNullFieldsStruct {
  std::optional<int> a = std::nullopt;
  std::optional<std::string> b = "ok";
  rfl::TriState<int> c = rfl::omitted;
  rfl::TriState<int> d = std::nullopt;
};

struct XmlContainerAddNull {
  std::vector<int> items = {};
  std::string name = "x";
};

struct DeferredNullableValidator {
  rfl::DefaultVal<rfl::Nullable<rfl::Validator<int, rfl::Minimum<1>>>> count =
      rfl::Validator<int, rfl::Minimum<1>>{};
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
  YulVariableDeclaration decl;
  decl.variables = {"x"};
  const auto out = rfl::json::write(decl);
  EXPECT_EQ(
      out,
      R"({"nodeType":"YulVariableDeclaration","variables":["x"],"value":null})");

  const auto from_omitted =
      rfl::json::read<YulVariableDeclaration>(
          R"({"nodeType":"YulVariableDeclaration","variables":["x"]})")
          .value();
  EXPECT_TRUE(from_omitted.value.is_null());
  EXPECT_FALSE(from_omitted.value.has_value());

  const auto from_null =
      rfl::json::read<YulVariableDeclaration>(
          R"({"nodeType":"YulVariableDeclaration","variables":["x"],"value":null})")
          .value();
  EXPECT_TRUE(from_null.value.is_null());

  const auto from_val =
      rfl::json::read<YulVariableDeclaration>(
          R"({"nodeType":"YulVariableDeclaration","variables":["x"],"value":{"nodeType":"YulLiteral","value":"42"}})")
          .value();
  ASSERT_TRUE(from_val.value.has_value());
  EXPECT_EQ(from_val.value->value, "42");
  EXPECT_EQ(
      rfl::json::write(from_val),
      R"({"nodeType":"YulVariableDeclaration","variables":["x"],"value":{"nodeType":"YulLiteral","value":"42"}})");
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
      rfl::json::read<JsonRpcEnvelope, rfl::DefaultIfMissing,
                      rfl::DefaultIfNull>(
          R"({"method":"ping","id":null})")
          .value();
  EXPECT_TRUE(with_dim.id.is_null());
  const auto with_dim_omitted =
      rfl::json::read<JsonRpcEnvelope, rfl::DefaultIfMissing,
                      rfl::DefaultIfNull>(R"({"method":"ping"})")
          .value();
  EXPECT_TRUE(with_dim_omitted.id.is_omitted());
}

TEST(json, test_tristate_and_nullable_read_into_layered_overlay) {
  LayeredConfig base;
  base = rfl::json::read_into(
             std::move(base),
             R"({"build_dir":"/a","threads":4,"formatting":{"width":80},"cache_path":"/c"})")
             .value();
  EXPECT_EQ(base.build_dir.value(), "/a");
  EXPECT_EQ(base.threads.value(), 4);
  ASSERT_TRUE(base.formatting.has_value());
  EXPECT_EQ(base.formatting->width.value(), 80);
  EXPECT_TRUE(base.formatting->mode.is_omitted());
  EXPECT_EQ(base.cache_path.value(), "/c");

  base = rfl::json::read_into(
             std::move(base),
             R"({"threads":null,"formatting":{"mode":"compact"},"cache_path":null})")
             .value();
  EXPECT_TRUE(base.build_dir.has_value());
  EXPECT_EQ(base.build_dir.value(), "/a");
  EXPECT_TRUE(base.threads.is_null());
  EXPECT_TRUE(base.threads.is_present());
  ASSERT_TRUE(base.formatting.has_value());
  EXPECT_EQ(base.formatting->width.value(), 80);
  EXPECT_EQ(base.formatting->mode.value(), "compact");
  EXPECT_TRUE(base.cache_path.is_null());
  EXPECT_FALSE(base.legacy_opt.has_value());

  base = rfl::json::read_into(std::move(base), R"({"threads":8})").value();
  EXPECT_EQ(base.threads.value(), 8);
  EXPECT_EQ(base.build_dir.value(), "/a");
}

TEST(json, test_add_null_fields_processor_and_xml_empty_container) {
  AddNullFieldsStruct s;
  EXPECT_EQ(rfl::json::write(s), R"({"b":"ok","d":null})");
  EXPECT_EQ(rfl::json::write<rfl::AddNullFields>(s),
            R"({"a":null,"b":"ok","d":null})");

  const auto read_back =
      rfl::json::read<AddNullFieldsStruct, rfl::AddNullFields>(R"({"b":"hi"})")
          .value();
  EXPECT_FALSE(read_back.a.has_value());
  EXPECT_EQ(read_back.b, "hi");
  EXPECT_TRUE(read_back.c.is_omitted());
  EXPECT_TRUE(read_back.d.is_omitted());

  // Under XML (_ignore_empty_containers=true), AddNullFields must NOT treat
  // non-nullable empty vectors as nullable fields.
  XmlContainerAddNull xc;
  const auto xml_plain = rfl::xml::write<"root">(xc);
  const auto xml_anf = rfl::xml::write<"root", rfl::AddNullFields>(xc);
  EXPECT_EQ(xml_anf, xml_plain);
}

TEST(json, test_nullable_and_tristate_schema_aliases_and_wrappers) {
  const auto schema = rfl::json::to_schema<YulVariableDeclaration>();
  EXPECT_NE(
      schema.find(
          R"("value":{"anyOf":[{"$ref":"#/$defs/test_nullable_and_tristate__YulExpr"},{"type":"null"}]})"),
      std::string::npos);
  EXPECT_NE(schema.find(R"("required":["nodeType","variables"])"),
            std::string::npos);

  const auto rpc_schema = rfl::json::to_schema<JsonRpcEnvelope>();
  EXPECT_NE(rpc_schema.find(R"("type":"null")"), std::string::npos);
  EXPECT_NE(rpc_schema.find(R"("required":["jsonrpc","method"])"),
            std::string::npos);

  const auto wrapped_schema =
      rfl::json::to_schema<WrappedPresence, rfl::DefaultIfMissing>();
  EXPECT_NE(wrapped_schema.find("\"description\":\"Always serialized nullable\""),
            std::string::npos);
  EXPECT_NE(wrapped_schema.find("\"default\":7"), std::string::npos);
  EXPECT_NE(wrapped_schema.find("\"default\":null"), std::string::npos);
  EXPECT_NE(wrapped_schema.find("\"reqId\""), std::string::npos);
  // DefaultVal<TriState<int>> with State::omitted must NOT emit "default":null.
  const auto pos_omitted = wrapped_schema.find("\"with_default_omitted\"");
  ASSERT_NE(pos_omitted, std::string::npos);
  const auto omitted_slice = wrapped_schema.substr(pos_omitted, 120);
  EXPECT_EQ(omitted_slice.find("\"default\""), std::string::npos);

  // Alias inside Nullable / TriState.
  const auto aliased =
      rfl::json::read<AliasedPresence>(R"({"old_n":11,"old_t":22})").value();
  EXPECT_EQ(aliased.n.value().value(), 11);
  EXPECT_EQ(aliased.t.value().value(), 22);

  // Deferred validator inside DefaultVal<Nullable<Validator<...>>>.
  const auto err = rfl::json::read<DeferredNullableValidator>("{}");
  EXPECT_FALSE(err.has_value());
  const auto ok_null =
      rfl::json::read<DeferredNullableValidator>(R"({"count":null})").value();
  EXPECT_TRUE(ok_null.count.value().is_null());
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
  static_assert(!std::is_copy_constructible_v<rfl::Nullable<std::unique_ptr<int>>>);
  static_assert(!std::is_copy_constructible_v<rfl::TriState<std::unique_ptr<int>>>);
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

TEST(json, test_nullable_and_tristate_generic_nlohmann_and_cli) {
  const auto env =
      rfl::json::read<JsonRpcEnvelope>(
          R"({"jsonrpc":"2.0","method":"ping","id":null})")
          .value();
  const auto g = rfl::to_generic(env);
  const auto from_g = rfl::from_generic<JsonRpcEnvelope>(g).value();
  EXPECT_TRUE(from_g.id.is_null());

  const auto nj = rfl::nlohmann::write(env);
  EXPECT_TRUE(nj.contains("id"));
  EXPECT_TRUE(nj["id"].is_null());
  const auto from_nj = rfl::nlohmann::read<JsonRpcEnvelope>(nj).value();
  EXPECT_TRUE(from_nj.id.is_null());

  nlohmann::json nj_omitted = {{"jsonrpc", "2.0"}, {"method", "notify"}};
  const auto from_nj_omitted =
      rfl::nlohmann::read<JsonRpcEnvelope>(nj_omitted).value();
  EXPECT_TRUE(from_nj_omitted.id.is_omitted());

  // CLI flag + nested struct option + usage generation.
  const std::vector<std::string> argv = {"--verbose", "--server.port=9090"};
  const auto cli_res = rfl::cli::read<CliPresenceOpts>(argv).value();
  ASSERT_TRUE(cli_res.verbose.has_value());
  EXPECT_TRUE(*cli_res.verbose);
  ASSERT_TRUE(cli_res.server.has_value());
  EXPECT_EQ(cli_res.server->port, 9090);

  const auto help = rfl::cli::usage<CliPresenceOpts>("prog").value();
  EXPECT_NE(help.find("--verbose"), std::string::npos);
  EXPECT_NE(help.find("--server.port"), std::string::npos);
}

}  // namespace test_nullable_and_tristate
