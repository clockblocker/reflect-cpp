#include <gtest/gtest.h>

#include <nlohmann/json.hpp>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <rfl/nlohmann.hpp>
#include <string>
#include <variant>
#include <vector>

namespace test_nullable_and_tristate {

struct YulExpr {
  std::string nodeType = "YulLiteral";
  std::string value = "0";
};

// Mirrors solidity#3 YulVariableDeclaration::value without any Reflector hack.
struct YulVariableDeclaration {
  std::string nodeType = "YulVariableDeclaration";
  std::vector<std::string> variables = {};
  rfl::Nullable<YulExpr> value = std::nullopt;
  std::optional<std::string> debug_comment = std::nullopt;
};

// Mirrors transmission#1 JSON-RPC 2.0 request/notification id without a custom
// Parser specialization.
struct JsonRpcEnvelope {
  std::string jsonrpc = "2.0";
  std::string method = "";
  rfl::TriState<std::variant<int64_t, std::string>> id = rfl::omitted;
};

struct NestedLayer {
  rfl::TriState<int> width = rfl::omitted;
  rfl::TriState<std::string> mode = rfl::omitted;
};

// Mirrors slang-server#1 layered config overlay with read_into.
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
};

struct AddNullFieldsStruct {
  std::optional<int> a = std::nullopt;
  std::optional<std::string> b = "ok";
  rfl::TriState<int> c = rfl::omitted;
  rfl::TriState<int> d = std::nullopt;
};

struct DeferredNullableValidator {
  rfl::DefaultVal<rfl::Nullable<rfl::Validator<int, rfl::Minimum<1>>>> count =
      rfl::Validator<int, rfl::Minimum<1>>{};
};

TEST(json, test_nullable_always_emits_null_while_optional_omits) {
  YulVariableDeclaration decl;
  decl.variables = {"x"};
  // `value` is null -> MUST serialize `"value":null`; `debug_comment` is
  // std::nullopt -> MUST be omitted.
  const auto out = rfl::json::write(decl);
  EXPECT_EQ(
      out,
      R"({"nodeType":"YulVariableDeclaration","variables":["x"],"value":null})");

  // Omitted key on read becomes null.
  const auto from_omitted =
      rfl::json::read<YulVariableDeclaration>(
          R"({"nodeType":"YulVariableDeclaration","variables":["x"]})")
          .value();
  EXPECT_TRUE(from_omitted.value.is_null());
  EXPECT_FALSE(from_omitted.value.has_value());

  // Explicit null on read becomes null.
  const auto from_null =
      rfl::json::read<YulVariableDeclaration>(
          R"({"nodeType":"YulVariableDeclaration","variables":["x"],"value":null})")
          .value();
  EXPECT_TRUE(from_null.value.is_null());

  // Value on read populates Nullable<T> and round-trips cleanly.
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
  // 1) Notification: "id" omitted -> State::omitted, omitted on write.
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

  // 2) Explicit "id": null -> State::null, emits "id":null on write.
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

  // 3) Value "id": 42 -> State::value, emits "id":42 on write.
  const auto val_req =
      rfl::json::read<JsonRpcEnvelope>(
          R"({"jsonrpc":"2.0","method":"get","id":42})")
          .value();
  EXPECT_TRUE(val_req.id.is_present());
  EXPECT_TRUE(val_req.id.has_value());
  EXPECT_EQ(std::get<int64_t>(*val_req.id), 42);
  EXPECT_EQ(rfl::json::write(val_req),
            R"({"jsonrpc":"2.0","method":"get","id":42})");

  // Also verify with rfl::DefaultIfMissing and rfl::DefaultIfNull.
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
  // Layer 1: set build_dir, threads, formatting.width, cache_path.
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

  // Layer 2: omit build_dir (preserves "/a"), explicitly null threads,
  // recursively overlay formatting.mode while preserving formatting.width,
  // explicitly null cache_path.
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

  // Layer 3: override threads from explicit null back to a value, leave
  // build_dir untouched.
  base = rfl::json::read_into(std::move(base), R"({"threads":8})").value();
  EXPECT_EQ(base.threads.value(), 8);
  EXPECT_EQ(base.build_dir.value(), "/a");
}

TEST(json, test_add_null_fields_processor) {
  AddNullFieldsStruct s;
  // Default write: omits optional `a` and omitted TriState `c`, emits `b` and
  // explicit-null TriState `d`.
  EXPECT_EQ(rfl::json::write(s), R"({"b":"ok","d":null})");

  // With rfl::AddNullFields: emits `"a":null` for empty std::optional, while
  // still keeping TriState::omitted (`c`) omitted and keeping `a` optional on
  // read!
  EXPECT_EQ(rfl::json::write<rfl::AddNullFields>(s),
            R"({"a":null,"b":"ok","d":null})");

  const auto read_back =
      rfl::json::read<AddNullFieldsStruct, rfl::AddNullFields>(R"({"b":"hi"})")
          .value();
  EXPECT_FALSE(read_back.a.has_value());
  EXPECT_EQ(read_back.b, "hi");
  EXPECT_TRUE(read_back.c.is_omitted());
  EXPECT_TRUE(read_back.d.is_omitted());
}

TEST(json, test_nullable_and_tristate_schema_and_wrappers) {
  const auto schema = rfl::json::to_schema<YulVariableDeclaration>();
  // Draft 2020-12 nullable schema (`anyOf` with `null`) and NOT in `required`.
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

  // Wrapped fields (Description, Rename, DefaultVal) + DefaultIfMissing.
  const auto wrapped_schema =
      rfl::json::to_schema<WrappedPresence, rfl::DefaultIfMissing>();
  EXPECT_NE(wrapped_schema.find("\"description\":\"Always serialized nullable\""),
            std::string::npos);
  EXPECT_NE(wrapped_schema.find("\"default\":7"), std::string::npos);
  EXPECT_NE(wrapped_schema.find("\"default\":null"), std::string::npos);
  EXPECT_NE(wrapped_schema.find("\"reqId\""), std::string::npos);

  // Deferred validator inside DefaultVal<Nullable<Validator<...>>>.
  const auto err = rfl::json::read<DeferredNullableValidator>("{}");
  EXPECT_FALSE(err.has_value());
  const auto ok_null =
      rfl::json::read<DeferredNullableValidator>(R"({"count":null})").value();
  EXPECT_TRUE(ok_null.count.value().is_null());
}

TEST(json, test_nullable_and_tristate_generic_and_nlohmann) {
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
}

}  // namespace test_nullable_and_tristate
