#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <sstream>
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

}  // namespace test_read_into
