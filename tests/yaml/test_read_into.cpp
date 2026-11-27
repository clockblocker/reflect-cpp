#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/yaml.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace test_yaml_read_into {

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
};

TEST(yaml, read_into_layered_config_and_then) {
  const std::string workspace_yaml = R"(
name: slang-server
server:
  host: 0.0.0.0
  tls:
    enabled: true
tags:
  - workspace
  - ci
)";

  const std::string user_yaml = R"(
workers: 16
server:
  port: 9090
  tls:
    cert_path: /home/user/cert.pem
)";

  const std::string local_yaml = R"(
server:
  tls:
    min_version: 13
)";

  const auto cfg =
      rfl::yaml::read_into(AppConfig{}, workspace_yaml)
          .and_then([&](AppConfig c) {
            return rfl::yaml::read_into(std::move(c), user_yaml);
          })
          .and_then([&](AppConfig c) {
            return rfl::yaml::read_into(std::move(c), local_yaml);
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
}

struct SubWithRequired {
  std::string label;
  int count;
};

struct OptionalHolder {
  std::optional<int> threshold;
  std::optional<SubWithRequired> sub;
};

TEST(yaml, read_into_optionals) {
  OptionalHolder base{
      .threshold = 42,
      .sub = SubWithRequired{.label = "initial", .count = 10},
  };

  // Present mapping overlays existing optional struct.
  {
    const auto res =
        rfl::yaml::read_into(base, "sub:\n  count: 99\n").value();
    ASSERT_TRUE(res.threshold.has_value());
    EXPECT_EQ(*res.threshold, 42);
    ASSERT_TRUE(res.sub.has_value());
    EXPECT_EQ(res.sub->label, "initial");
    EXPECT_EQ(res.sub->count, 99);
  }

  // Explicit null clears optional values to nullopt.
  {
    const auto res =
        rfl::yaml::read_into(base, "threshold: null\nsub: null\n").value();
    EXPECT_FALSE(res.threshold.has_value());
    EXPECT_FALSE(res.sub.has_value());
  }

  // nullopt base + complete mapping parses fresh nested struct.
  {
    OptionalHolder empty_base{};
    const auto res =
        rfl::yaml::read_into(empty_base,
                             "threshold: 7\nsub:\n  label: fresh\n  count: 3\n")
            .value();
    ASSERT_TRUE(res.threshold.has_value());
    EXPECT_EQ(*res.threshold, 7);
    ASSERT_TRUE(res.sub.has_value());
    EXPECT_EQ(res.sub->label, "fresh");
    EXPECT_EQ(res.sub->count, 3);
  }
}

struct ValidatedConfig {
  rfl::Validator<int, rfl::Minimum<1>, rfl::Maximum<65535>> port = 8080;
  std::string host = "localhost";
  rfl::Rename<"displayName", std::string> name = "init";
  rfl::Skip<std::string> token = "keep";
};

TEST(yaml, read_into_validators_processors_and_atomicity) {
  ValidatedConfig base;
  base.port = 443;
  base.host = "example.com";
  base.token = "secret";

  // Naming processor + Rename + Skip preservation
  {
    const auto merged =
        rfl::yaml::read_into<rfl::SnakeCaseToCamelCase, rfl::NoExtraFields>(
            base, "displayName: updated\ntoken: ignored\n")
            .value();
    EXPECT_EQ(merged.port.value(), 443);
    EXPECT_EQ(merged.host, "example.com");
    EXPECT_EQ(merged.name.value(), "updated");
    EXPECT_EQ(merged.token.value(), "secret");
  }

  // Validator failure leaves base untouched
  {
    const auto res =
        rfl::yaml::read_into(base, "host: mutated\nport: 0\n");
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(base.host, "example.com");
    EXPECT_EQ(base.port.value(), 443);
  }

  // Unknown key rejected under NoExtraFields, leaving base untouched
  {
    const auto res = rfl::yaml::read_into<rfl::NoExtraFields>(
        base, "host: mutated\nunknown_field: 1\n");
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(base.host, "example.com");
  }

  // Syntax error leaves base untouched
  {
    const auto res = rfl::yaml::read_into(base, "host: [unclosed");
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(base.host, "example.com");
  }
}

struct MoveOnlyConfig {
  std::string name;
  std::unique_ptr<ServerConfig> server;
};

TEST(yaml, read_into_move_only_and_stream_and_load_into) {
  MoveOnlyConfig base{
      .name = "svc",
      .server = std::make_unique<ServerConfig>(ServerConfig{
          .host = "127.0.0.1",
          .port = 8080,
          .tls = TlsConfig{.enabled = true, .cert_path = "/base.pem", .min_version = 12},
      }),
  };

  std::istringstream iss("server:\n  port: 9000\n");
  auto merged = rfl::yaml::read_into(std::move(base), iss).value();
  EXPECT_EQ(merged.name, "svc");
  ASSERT_NE(merged.server, nullptr);
  EXPECT_EQ(merged.server->host, "127.0.0.1");
  EXPECT_EQ(merged.server->port, 9000);
  EXPECT_TRUE(merged.server->tls.enabled);
  EXPECT_EQ(merged.server->tls.cert_path, "/base.pem");

  const auto tmp_path = std::filesystem::temp_directory_path() /
                        "rfl_test_read_into_overlay.yaml";
  {
    std::ofstream ofs(tmp_path);
    ofs << "server:\n  tls:\n    min_version: 13\n";
  }
  const auto from_file =
      rfl::yaml::load_into(std::move(merged), tmp_path.string()).value();
  std::filesystem::remove(tmp_path);

  EXPECT_EQ(from_file.server->port, 9000);
  EXPECT_EQ(from_file.server->tls.min_version, 13);
}

TEST(yaml, read_into_empty_document_vs_empty_mapping) {
  const ServerConfig base{
      .host = "localhost",
      .port = 8080,
      .tls = TlsConfig{.enabled = true, .cert_path = "/a.pem", .min_version = 12},
  };

  // Empty YAML document ("") parses as a null node and fails cleanly without
  // mutating the base struct.
  const auto empty_doc = rfl::yaml::read_into(base, "");
  EXPECT_FALSE(empty_doc.has_value());
  EXPECT_EQ(base.host, "localhost");
  EXPECT_EQ(base.port, 8080);

  // Explicit empty mapping ("{}") preserves all base fields.
  const auto empty_map = rfl::yaml::read_into(base, "{}");
  ASSERT_TRUE(empty_map.has_value()) << empty_map.error().what();
  EXPECT_EQ(empty_map->host, "localhost");
  EXPECT_EQ(empty_map->port, 8080);
  EXPECT_TRUE(empty_map->tls.enabled);
}

}  // namespace test_yaml_read_into
