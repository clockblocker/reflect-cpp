#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <rfl.hpp>
#include <rfl/toml.hpp>
#include <sstream>
#include <string>

namespace test_toml_read_into {

struct TlsConfig {
  bool enabled = false;
  std::string cert_path = "/etc/ssl/default.pem";
  int min_version = 12;
};

struct ServerConfig {
  std::string host = "127.0.0.1";
  int port = 8080;
  std::optional<std::string> label;
  TlsConfig tls;
};

TEST(toml, read_into_partial_nested_and_empty_documents) {
  const ServerConfig base{
      .host = "127.0.0.1",
      .port = 8080,
      .label = "primary",
      .tls = TlsConfig{.enabled = true,
                       .cert_path = "/base.pem",
                       .min_version = 12},
  };

  // Empty and comment-only TOML documents parse as empty tables and preserve
  // every base field.
  const auto empty_res = rfl::toml::read_into(base, "");
  ASSERT_TRUE(empty_res.has_value()) << empty_res.error().what();
  EXPECT_EQ(empty_res->host, "127.0.0.1");
  EXPECT_EQ(empty_res->port, 8080);
  EXPECT_EQ(empty_res->label, "primary");
  EXPECT_TRUE(empty_res->tls.enabled);

  const auto comment_res = rfl::toml::read_into(base, "# just a comment\n");
  ASSERT_TRUE(comment_res.has_value()) << comment_res.error().what();
  EXPECT_EQ(comment_res->port, 8080);

  // Partial nested table overlay preserves untouched fields & optional value.
  const auto merged = rfl::toml::read_into<rfl::NoExtraFields>(
      base, "port = 9090\n[tls]\nmin_version = 13\n");
  ASSERT_TRUE(merged.has_value()) << merged.error().what();
  EXPECT_EQ(merged->host, "127.0.0.1");
  EXPECT_EQ(merged->port, 9090);
  EXPECT_EQ(merged->label, "primary");
  EXPECT_TRUE(merged->tls.enabled);
  EXPECT_EQ(merged->tls.cert_path, "/base.pem");
  EXPECT_EQ(merged->tls.min_version, 13);

  // Unknown key and syntax errors leave lvalue base untouched.
  EXPECT_FALSE(
      rfl::toml::read_into<rfl::NoExtraFields>(base, "unknown_key = 1\n")
          .has_value());
  EXPECT_FALSE(rfl::toml::read_into(base, "port = [unclosed").has_value());
  EXPECT_EQ(base.port, 8080);
}

TEST(toml, read_into_stream_and_load_into) {
  const ServerConfig base{
      .host = "localhost",
      .port = 8000,
      .label = std::nullopt,
      .tls = TlsConfig{.enabled = false,
                       .cert_path = "/default.pem",
                       .min_version = 12},
  };

  std::istringstream iss("label = \"from-stream\"\n");
  const auto from_stream = rfl::toml::read_into(base, iss).value();
  EXPECT_EQ(from_stream.host, "localhost");
  EXPECT_EQ(from_stream.label, "from-stream");

  const auto tmp_path =
      std::filesystem::temp_directory_path() / "rfl_test_read_into_overlay.toml";
  {
    std::ofstream ofs(tmp_path);
    ofs << "port = 8443\n[tls]\nenabled = true\n";
  }
  const auto from_file =
      rfl::toml::load_into(from_stream, tmp_path.string()).value();
  std::filesystem::remove(tmp_path);

  EXPECT_EQ(from_file.host, "localhost");
  EXPECT_EQ(from_file.port, 8443);
  EXPECT_EQ(from_file.label, "from-stream");
  EXPECT_TRUE(from_file.tls.enabled);
  EXPECT_EQ(from_file.tls.cert_path, "/default.pem");

  // Missing file leaves base untouched.
  const auto missing = rfl::toml::load_into(
      base, "/nonexistent/path/rfl_overlay_missing.toml");
  EXPECT_FALSE(missing.has_value());
  EXPECT_EQ(base.port, 8000);
}

}  // namespace test_toml_read_into
