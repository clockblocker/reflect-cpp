#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/Deprecated.hpp>
#include <rfl/json.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace test_read_into {

struct TlsConfig {
  bool enabled = false;
  std::string cert_path = "/etc/ssl/default.pem";
  int min_version = 12;
};

struct ServerConfig {
  std::string host = "127.0.0.1";
  int port = 8080;
  TlsConfig tls;
};

struct AppConfig {
  std::string name = "app";
  int workers = 4;
  ServerConfig server;
  std::vector<std::string> tags = {"default"};
  std::map<std::string, int> limits = {{"max_conn", 100}};
};

TEST(json, read_into_layered_config_and_then) {
  const std::string workspace_json = R"({
    "name": "slang-server",
    "server": {
      "host": "0.0.0.0",
      "tls": { "enabled": true }
    },
    "tags": ["workspace", "ci"]
  })";

  const std::string user_json = R"({
    "workers": 16,
    "server": {
      "port": 9090,
      "tls": { "cert_path": "/home/user/cert.pem" }
    }
  })";

  const std::string local_json = R"({
    "server": {
      "tls": { "min_version": 13 }
    },
    "limits": { "max_conn": 500 }
  })";

  const auto cfg =
      rfl::json::read_into(AppConfig{}, workspace_json)
          .and_then([&](AppConfig c) {
            return rfl::json::read_into(std::move(c), user_json);
          })
          .and_then([&](AppConfig c) {
            return rfl::json::read_into(std::move(c), local_json);
          })
          .value();

  EXPECT_EQ(cfg.name, "slang-server");
  EXPECT_EQ(cfg.workers, 16);
  EXPECT_EQ(cfg.server.host, "0.0.0.0");
  EXPECT_EQ(cfg.server.port, 9090);
  EXPECT_TRUE(cfg.server.tls.enabled);
  EXPECT_EQ(cfg.server.tls.cert_path, "/home/user/cert.pem");
  EXPECT_EQ(cfg.server.tls.min_version, 13);
  EXPECT_EQ(cfg.tags, (std::vector<std::string>{"workspace", "ci"}));
  EXPECT_EQ(cfg.limits, (std::map<std::string, int>{{"max_conn", 500}}));
}

TEST(json, read_into_empty_object_preserves_base) {
  AppConfig base;
  base.name = "custom";
  base.workers = 8;
  base.server.port = 1234;

  const auto merged = rfl::json::read_into(base, "{}").value();
  EXPECT_EQ(merged.name, "custom");
  EXPECT_EQ(merged.workers, 8);
  EXPECT_EQ(merged.server.port, 1234);
  EXPECT_EQ(merged.server.tls.cert_path, "/etc/ssl/default.pem");
}

struct SubWithRequired {
  std::string label;
  int count;
};

struct OptionalHolder {
  std::optional<int> threshold;
  std::optional<SubWithRequired> sub;
};

TEST(json, read_into_optionals) {
  OptionalHolder base{
      .threshold = 42,
      .sub = SubWithRequired{.label = "initial", .count = 10},
  };

  // Absent preserves both optionals.
  {
    const auto res = rfl::json::read_into(base, "{}").value();
    ASSERT_TRUE(res.threshold.has_value());
    EXPECT_EQ(*res.threshold, 42);
    ASSERT_TRUE(res.sub.has_value());
    EXPECT_EQ(res.sub->label, "initial");
    EXPECT_EQ(res.sub->count, 10);
  }

  // Present object overlays existing optional nested struct.
  {
    const auto res =
        rfl::json::read_into(base, R"({"sub":{"count":99}})").value();
    ASSERT_TRUE(res.threshold.has_value());
    EXPECT_EQ(*res.threshold, 42);
    ASSERT_TRUE(res.sub.has_value());
    EXPECT_EQ(res.sub->label, "initial");
    EXPECT_EQ(res.sub->count, 99);
  }

  // Explicit null clears optional values to nullopt.
  {
    const auto res =
        rfl::json::read_into(base, R"({"threshold":null,"sub":null})").value();
    EXPECT_FALSE(res.threshold.has_value());
    EXPECT_FALSE(res.sub.has_value());
  }

  // nullopt base + present full object parses fresh nested struct.
  {
    OptionalHolder empty_base{};
    const auto res =
        rfl::json::read_into(
            empty_base, R"({"threshold":7,"sub":{"label":"fresh","count":3}})")
            .value();
    ASSERT_TRUE(res.threshold.has_value());
    EXPECT_EQ(*res.threshold, 7);
    ASSERT_TRUE(res.sub.has_value());
    EXPECT_EQ(res.sub->label, "fresh");
    EXPECT_EQ(res.sub->count, 3);
  }

  // nullopt base + incomplete nested object fails cleanly (no base to inherit).
  {
    OptionalHolder empty_base{};
    const auto res =
        rfl::json::read_into(empty_base, R"({"sub":{"count":3}})");
    EXPECT_FALSE(res.has_value());
    EXPECT_FALSE(empty_base.sub.has_value());
  }
}

struct ValidatedConfig {
  rfl::Validator<int, rfl::Minimum<1>, rfl::Maximum<65535>> port = 8080;
  std::string host = "localhost";
};

TEST(json, read_into_validator_and_atomicity) {
  ValidatedConfig base{.port = 443, .host = "example.com"};

  // Missing validated field preserves base.
  {
    const auto res =
        rfl::json::read_into(base, R"({"host":"api.example.com"})").value();
    EXPECT_EQ(res.port.value(), 443);
    EXPECT_EQ(res.host, "api.example.com");
  }

  // Present valid field updates value.
  {
    const auto res = rfl::json::read_into(base, R"({"port":8443})").value();
    EXPECT_EQ(res.port.value(), 8443);
    EXPECT_EQ(res.host, "example.com");
  }

  // Present invalid field fails atomically, leaving base untouched even when
  // an earlier field ("host") succeeded.
  {
    const auto res = rfl::json::read_into(
        base, R"({"host":"mutated.example.com","port":70000})");
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(base.port.value(), 443);
    EXPECT_EQ(base.host, "example.com");
  }
}

TEST(json, read_into_no_extra_fields_and_error_atomicity) {
  AppConfig base;
  base.name = "original";
  base.workers = 4;
  base.server.port = 8080;

  // Unknown top-level field rejected under NoExtraFields.
  {
    const auto res = rfl::json::read_into<rfl::NoExtraFields>(
        base, R"({"name":"changed","unknown_key":123})");
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(base.name, "original");
  }

  // Unknown nested field rejected under NoExtraFields.
  {
    const auto res = rfl::json::read_into<rfl::NoExtraFields>(
        base, R"({"server":{"port":9999,"bogus":true}})");
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(base.server.port, 8080);
  }

  // Syntax error leaves base untouched.
  {
    const auto res = rfl::json::read_into(base, R"({"name":"changed", )");
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(base.name, "original");
  }

  // Type mismatch mid-overlay leaves base untouched.
  {
    const auto res =
        rfl::json::read_into(base, R"({"name":"changed","workers":"not_int"})");
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(base.name, "original");
    EXPECT_EQ(base.workers, 4);
  }
}

struct NamingInner {
  int max_retries = 3;
  std::string endpoint_url = "https://default";
};

struct NamingConfig {
  int request_timeout = 30;
  rfl::Rename<"customName", std::string> display_name = "init";
  rfl::Rename<"innerCfg", NamingInner> inner;
  rfl::Description<"Number of threads", int> thread_count = 2;
  rfl::DefaultVal<int> batch_size = 64;
  rfl::Skip<std::string> runtime_token = "secret-token";
  rfl::SkipDeserialization<int> internal_id = 777;
  rfl::SkipSerialization<int> write_only_override = 1;
};

TEST(json, read_into_processors_rename_and_skip) {
  NamingConfig base;
  base.runtime_token = "keep-me";
  base.internal_id = 999;

  const std::string overlay = R"({
    "requestTimeout": 60,
    "customName": "overridden",
    "innerCfg": { "maxRetries": 5 },
    "threadCount": 8,
    "runtimeToken": "should-be-ignored",
    "internalId": 12345,
    "writeOnlyOverride": 42
  })";

  const auto merged =
      rfl::json::read_into<rfl::SnakeCaseToCamelCase>(base, overlay).value();

  EXPECT_EQ(merged.request_timeout, 60);
  EXPECT_EQ(merged.display_name.value(), "overridden");
  EXPECT_EQ(merged.inner.value().max_retries, 5);
  EXPECT_EQ(merged.inner.value().endpoint_url, "https://default");
  EXPECT_EQ(merged.thread_count.value(), 8);
  EXPECT_EQ(merged.batch_size.value(), 64);
  EXPECT_EQ(merged.runtime_token.value(), "keep-me");
  EXPECT_EQ(merged.internal_id.value(), 999);
  EXPECT_EQ(merged.write_only_override.value(), 42);
}

struct MoveOnlySub {
  std::unique_ptr<int> a;
  std::unique_ptr<int> b;
};

struct MoveOnlyConfig {
  std::string name;
  std::unique_ptr<MoveOnlySub> sub;
  rfl::Box<TlsConfig> tls;
};

TEST(json, read_into_move_only_struct) {
  MoveOnlyConfig base{
      .name = "init",
      .sub = std::make_unique<MoveOnlySub>(MoveOnlySub{
          .a = std::make_unique<int>(10),
          .b = std::make_unique<int>(20),
      }),
      .tls = rfl::Box<TlsConfig>::make(TlsConfig{
          .enabled = true,
          .cert_path = "/base.pem",
          .min_version = 12,
      }),
  };

  const auto merged =
      rfl::json::read_into(
          std::move(base),
          R"({"sub":{"b":99},"tls":{"min_version":13}})")
          .value();

  EXPECT_EQ(merged.name, "init");
  ASSERT_NE(merged.sub, nullptr);
  ASSERT_NE(merged.sub->a, nullptr);
  ASSERT_NE(merged.sub->b, nullptr);
  EXPECT_EQ(*merged.sub->a, 10);
  EXPECT_EQ(*merged.sub->b, 99);
  EXPECT_TRUE(merged.tls->enabled);
  EXPECT_EQ(merged.tls->cert_path, "/base.pem");
  EXPECT_EQ(merged.tls->min_version, 13);
}

struct SharedConfig {
  std::shared_ptr<TlsConfig> tls;
  rfl::Ref<ServerConfig> server;
};

TEST(json, read_into_shared_ptr_and_ref_do_not_mutate_original) {
  SharedConfig base{
      .tls = std::make_shared<TlsConfig>(
          TlsConfig{.enabled = false, .cert_path = "/orig.pem", .min_version = 12}),
      .server = rfl::Ref<ServerConfig>::make(ServerConfig{.host = "orig", .port = 80}),
  };

  // Even on success, original shared pointees remain unmodified.
  const auto merged =
      rfl::json::read_into(
          base, R"({"tls":{"enabled":true},"server":{"port":443}})")
          .value();

  EXPECT_FALSE(base.tls->enabled);
  EXPECT_EQ(base.server->port, 80);

  EXPECT_TRUE(merged.tls->enabled);
  EXPECT_EQ(merged.tls->cert_path, "/orig.pem");
  EXPECT_EQ(merged.server->host, "orig");
  EXPECT_EQ(merged.server->port, 443);

  // On failure, original shared pointees remain unmodified.
  const auto failed = rfl::json::read_into(
      base, R"({"tls":{"enabled":true},"server":{"port":"bad"}})");
  EXPECT_FALSE(failed.has_value());
  EXPECT_FALSE(base.tls->enabled);
  EXPECT_EQ(base.server->port, 80);
}

struct ExtraFieldsConfig {
  std::string name = "base";
  rfl::ExtraFields<int> extras;
};

TEST(json, read_into_extra_fields_updates_existing_and_preserves_rest) {
  ExtraFieldsConfig base;
  base.extras["a"] = 1;
  base.extras["b"] = 2;

  const auto merged =
      rfl::json::read_into(base, R"({"b":20,"c":30})").value();

  EXPECT_EQ(merged.name, "base");
  EXPECT_EQ(merged.extras.size(), 3u);
  EXPECT_EQ(merged.extras.at("a"), 1);
  EXPECT_EQ(merged.extras.at("b"), 20);
  EXPECT_EQ(merged.extras.at("c"), 30);
}

TEST(json, read_into_istream_flags_and_load_into) {
  AppConfig base;
  base.name = "from-base";
  base.workers = 2;

  // istream + yyjson flags (comments and trailing commas)
  std::istringstream iss(R"({
    // override workers
    "workers": 12,
  })");
  const auto from_stream =
      rfl::json::read_into(
          base, iss,
          YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS)
          .value();
  EXPECT_EQ(from_stream.name, "from-base");
  EXPECT_EQ(from_stream.workers, 12);

  // load_into from a temporary file
  const auto tmp_path = std::filesystem::temp_directory_path() /
                        "rfl_test_read_into_overlay.json";
  {
    std::ofstream ofs(tmp_path);
    ofs << R"({"server":{"host":"10.0.0.1"}})";
  }
  const auto from_file =
      rfl::json::load_into(from_stream, tmp_path.string()).value();
  std::filesystem::remove(tmp_path);

  EXPECT_EQ(from_file.name, "from-base");
  EXPECT_EQ(from_file.workers, 12);
  EXPECT_EQ(from_file.server.host, "10.0.0.1");
  EXPECT_EQ(from_file.server.port, 8080);

  // load_into missing file returns error without touching base
  const auto missing =
      rfl::json::load_into(base, "/nonexistent/path/rfl_overlay_missing.json");
  EXPECT_FALSE(missing.has_value());
  EXPECT_EQ(base.workers, 2);
}

struct NonDefaultInner {
  int a;
  std::string label;
  NonDefaultInner() = delete;
  NonDefaultInner(int _a, std::string _label)
      : a(_a), label(std::move(_label)) {}
};

struct NonDefaultOuter {
  NonDefaultInner inner;
  int port;
  NonDefaultOuter() = delete;
  NonDefaultOuter(NonDefaultInner _inner, int _port)
      : inner(std::move(_inner)), port(_port) {}
};

struct FlattenedMeta {
  std::string env = "prod";
  int version = 1;
};

struct WithFlattenedAndWrappers {
  rfl::Flatten<FlattenedMeta> meta;
  rfl::Commented<int> timeout = 30;
  rfl::Deprecated<"use new_port", "old port", int> legacy_port = 8000;
  rfl::Attribute<std::string> id = std::string("a1");
  rfl::Generic raw_extra;
};

struct ReflectedEndpoint {
  std::string host;
  int port;
};

struct ReadOnlyReflected {
  int x;
};

struct CustomReflectionObj {
  struct Repr {
    std::string host = "h";
    int port = 80;
  };
  using ReflectionType = Repr;

  std::string host;
  int port;
  bool throw_on_reflect = false;

  CustomReflectionObj(const Repr& _r) : host(_r.host), port(_r.port) {}
  CustomReflectionObj(std::string _h, int _p, bool _throw = false)
      : host(std::move(_h)), port(_p), throw_on_reflect(_throw) {}

  Repr reflection() const {
    if (throw_on_reflect) {
      throw std::runtime_error("reflection() threw");
    }
    return Repr{.host = host, .port = port};
  }
};

struct ThrowingCopyPayload {
  int value = 1;
  bool fail_copy = false;
  mutable int copy_budget = 0;

  ThrowingCopyPayload() = default;
  ThrowingCopyPayload(int _v, bool _fail, int _budget = 0)
      : value(_v), fail_copy(_fail), copy_budget(_budget) {}
  ThrowingCopyPayload(const ThrowingCopyPayload& _o)
      : value(_o.value), fail_copy(_o.fail_copy), copy_budget(_o.copy_budget) {
    if (_o.fail_copy) {
      if (_o.copy_budget <= 0) {
        throw std::runtime_error("copy ctor threw");
      }
      --_o.copy_budget;
      copy_budget = _o.copy_budget;
    }
  }
  ThrowingCopyPayload(ThrowingCopyPayload&&) noexcept = default;
  ThrowingCopyPayload& operator=(const ThrowingCopyPayload&) = default;
  ThrowingCopyPayload& operator=(ThrowingCopyPayload&&) noexcept = default;
};

}  // namespace test_read_into

namespace rfl {

template <>
struct Reflector<test_read_into::ReflectedEndpoint> {
  struct ReflType {
    std::string host;
    int port;
  };
  static test_read_into::ReflectedEndpoint to(const ReflType& _v) {
    return test_read_into::ReflectedEndpoint{.host = _v.host, .port = _v.port};
  }
  static ReflType from(const test_read_into::ReflectedEndpoint& _v) {
    if (_v.host == "THROW_FROM") {
      throw std::runtime_error("Reflector::from threw");
    }
    return ReflType{.host = _v.host, .port = _v.port};
  }
};

template <>
struct Reflector<test_read_into::ReadOnlyReflected> {
  struct ReflType {
    int x;
  };
  static test_read_into::ReadOnlyReflected to(const ReflType& _v) {
    return test_read_into::ReadOnlyReflected{.x = _v.x};
  }
};

}  // namespace rfl

namespace test_read_into {

TEST(json, read_into_non_default_no_field_names_named_tuple_fail_fast) {
  // Non-default-constructible nested struct
  const NonDefaultOuter base(NonDefaultInner(10, "base"), 8080);
  const auto merged =
      rfl::json::read_into(base, R"({"inner":{"a":42}})");
  ASSERT_TRUE(merged.has_value()) << merged.error().what();
  EXPECT_EQ(merged->inner.a, 42);
  EXPECT_EQ(merged->inner.label, "base");
  EXPECT_EQ(merged->port, 8080);

  // rfl::NoFieldNames positional array overlay (prefix overrides only)
  const auto positional =
      rfl::json::read_into<rfl::NoFieldNames>(base, R"([[77]])");
  ASSERT_TRUE(positional.has_value()) << positional.error().what();
  EXPECT_EQ(positional->inner.a, 77);
  EXPECT_EQ(positional->inner.label, "base");
  EXPECT_EQ(positional->port, 8080);

  // rfl::NamedTuple overlay
  using NT = rfl::NamedTuple<rfl::Field<"a", int>, rfl::Field<"b", std::string>>;
  const NT nt_base(1, std::string("keep"));
  const auto nt_merged = rfl::json::read_into(nt_base, R"({"a":9})");
  ASSERT_TRUE(nt_merged.has_value()) << nt_merged.error().what();
  EXPECT_EQ(nt_merged->get<"a">(), 9);
  EXPECT_EQ(nt_merged->get<"b">(), "keep");

  // rfl::FailFast stops at first field error and leaves base untouched
  const ServerConfig srv;
  const auto ff = rfl::json::read_into<rfl::FailFast>(
      srv, R"({"port":"bad","host":123})");
  EXPECT_FALSE(ff.has_value());
  EXPECT_EQ(srv.port, 8080);
  EXPECT_EQ(srv.host, "127.0.0.1");
}

TEST(json, read_into_flatten_wrappers_generic_reflector_and_throw_safety) {
  // Flatten + Commented + Deprecated + Attribute + Generic fast-path
  WithFlattenedAndWrappers w;
  w.raw_extra = rfl::Generic(std::string("old_raw"));
  const auto w_merged = rfl::json::read_into(
      w, R"({"version":2,"timeout":60,"raw_extra":{"k":1}})");
  ASSERT_TRUE(w_merged.has_value()) << w_merged.error().what();
  EXPECT_EQ(w_merged->meta.value().env, "prod");
  EXPECT_EQ(w_merged->meta.value().version, 2);
  EXPECT_EQ(w_merged->timeout.value(), 60);
  EXPECT_EQ(w_merged->legacy_port.value(), 8000);
  EXPECT_EQ(w_merged->id.value(), "a1");
  EXPECT_TRUE(w_merged->raw_extra.to_object().has_value());

  // Reflector<T> (read+write): partial overlay preserves untouched field
  const ReflectedEndpoint ep{.host = "alpha", .port = 8080};
  const auto ep_merged = rfl::json::read_into(ep, R"({"port":9090})");
  ASSERT_TRUE(ep_merged.has_value()) << ep_merged.error().what();
  EXPECT_EQ(ep_merged->host, "alpha");
  EXPECT_EQ(ep_merged->port, 9090);

  // Reflector<T>::from throwing returns Error instead of std::terminate()
  const ReflectedEndpoint bad_ep{.host = "THROW_FROM", .port = 8080};
  const auto bad_ep_res = rfl::json::read_into(bad_ep, R"({"port":9090})");
  EXPECT_FALSE(bad_ep_res.has_value());

  // Read-only Reflector<T> falls back to read()
  const ReadOnlyReflected ro{.x = 5};
  const auto ro_res = rfl::json::read_into(ro, R"({"x":12})");
  ASSERT_TRUE(ro_res.has_value()) << ro_res.error().what();
  EXPECT_EQ(ro_res->x, 12);

  // Custom ReflectionType: partial overlay + throwing reflection() safety
  const CustomReflectionObj cr("db.local", 5432, false);
  const auto cr_merged = rfl::json::read_into(cr, R"({"port":5433})");
  ASSERT_TRUE(cr_merged.has_value()) << cr_merged.error().what();
  EXPECT_EQ(cr_merged->host, "db.local");
  EXPECT_EQ(cr_merged->port, 5433);

  const CustomReflectionObj bad_cr("db.local", 5432, true);
  const auto bad_cr_res = rfl::json::read_into(bad_cr, R"({"port":5433})");
  EXPECT_FALSE(bad_cr_res.has_value());

  // Throwing copy in Ref<T> and std::shared_ptr<T> returns Error (no terminate)
  const auto bad_ref = rfl::Ref<ThrowingCopyPayload>::make(7, true);
  const auto bad_ref_res = rfl::json::read_into(bad_ref, R"({"value":9})");
  EXPECT_FALSE(bad_ref_res.has_value());

  const auto bad_sp = std::make_shared<ThrowingCopyPayload>(7, true);
  const auto bad_sp_res = rfl::json::read_into(bad_sp, R"({"value":9})");
  EXPECT_FALSE(bad_sp_res.has_value());
}

struct InnerWithConst {
  const int id;
  int port;
};

struct OuterWithConstFields {
  const std::string tag;
  InnerWithConst inner;
};

struct WithThrowingConstField {
  const ThrowingCopyPayload payload;
};

TEST(json, read_into_top_level_and_nested_const_fields) {
  const OuterWithConstFields base{
      .tag = "v1",
      .inner = InnerWithConst{.id = 100, .port = 8080},
  };

  // Overlay only inner.port -> preserves top-level const tag and inner const id
  const auto merged1 =
      rfl::json::read_into(base, R"({"inner":{"port":9090}})");
  ASSERT_TRUE(merged1.has_value()) << merged1.error().what();
  EXPECT_EQ(merged1->tag, "v1");
  EXPECT_EQ(merged1->inner.id, 100);
  EXPECT_EQ(merged1->inner.port, 9090);

  // Overlay top-level const tag AND inner const id -> both updated cleanly,
  // while original lvalue base remains untouched.
  const auto merged2 = rfl::json::read_into(
      base, R"({"tag":"v2","inner":{"id":200}})");
  ASSERT_TRUE(merged2.has_value()) << merged2.error().what();
  EXPECT_EQ(merged2->tag, "v2");
  EXPECT_EQ(merged2->inner.id, 200);
  EXPECT_EQ(merged2->inner.port, 8080);
  EXPECT_EQ(base.tag, "v1");
  EXPECT_EQ(base.inner.id, 100);

  // Positional NoFieldNames overlay on struct with nested const field.
  const auto pos = rfl::json::read_into<rfl::NoFieldNames>(
      base, R"(["v3",[300]])");
  ASSERT_TRUE(pos.has_value()) << pos.error().what();
  EXPECT_EQ(pos->tag, "v3");
  EXPECT_EQ(pos->inner.id, 300);
  EXPECT_EQ(pos->inner.port, 8080);

  // Throwing copy on a const field inside ViewReaderWithDefault returns an
  // Error instead of calling std::terminate(). Sweep copy_budget across every
  // copy hop so the copy inside ViewReaderWithDefault is guaranteed to throw.
  bool caught_as_result_error = false;
  for (int budget = 0; budget <= 8; ++budget) {
    try {
      const auto res = rfl::json::read_into(
          WithThrowingConstField{
              .payload = ThrowingCopyPayload(5, true, budget)},
          R"({"payload":{"value":9}})");
      if (!res.has_value()) {
        caught_as_result_error = true;
      }
    } catch (const std::runtime_error&) {
      // Thrown in non-noexcept caller parameter copy before entering
      // ViewReaderWithDefault.
    }
  }
  EXPECT_TRUE(caught_as_result_error);
}

struct OuterWithThrowingInner {
  WithThrowingConstField inner;
};

struct OuterWithOptionalThrowingInner {
  std::optional<WithThrowingConstField> inner;
};

TEST(json, read_into_nested_and_optional_throwing_const_fields) {
  // Non-const nested struct containing a throwing const field must convert
  // copy exceptions into Result::error rather than calling std::terminate().
  bool nested_caught_as_error = false;
  for (int budget = 0; budget <= 12; ++budget) {
    try {
      const auto res = rfl::json::read_into(
          OuterWithThrowingInner{
              .inner = WithThrowingConstField{
                  .payload = ThrowingCopyPayload(5, true, budget)}},
          R"({"inner":{"payload":{"value":9}}})");
      if (!res.has_value()) {
        nested_caught_as_error = true;
      }
    } catch (const std::runtime_error&) {
      // Thrown in non-noexcept caller parameter copy before entering parser.
    }
  }
  EXPECT_TRUE(nested_caught_as_error);

  // std::optional<WithThrowingConstField> must also never std::terminate().
  bool opt_caught_as_error = false;
  for (int budget = 0; budget <= 12; ++budget) {
    try {
      const auto res = rfl::json::read_into(
          OuterWithOptionalThrowingInner{
              .inner = WithThrowingConstField{
                  .payload = ThrowingCopyPayload(5, true, budget)}},
          R"({"inner":{"payload":{"value":9}}})");
      if (!res.has_value()) {
        opt_caught_as_error = true;
      }
    } catch (const std::runtime_error&) {
      // Thrown in non-noexcept caller parameter copy before entering parser.
    }
  }
  EXPECT_TRUE(opt_caught_as_error);
}

}  // namespace test_read_into
