#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <limits>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace test_json_flags {

struct Config {
  std::string name;
  int port;
  std::vector<std::string> hosts;
};

struct TextWrapper {
  std::string text;
};

struct FloatingValues {
  double pos_inf;
  double neg_inf;
  double nan_val;
};

struct OptionalFloatingValues {
  std::optional<double> pos_inf;
  std::optional<double> neg_inf;
  std::optional<double> nan_val;
};

TEST(json, test_json_flags_values_disambiguated) {
  static_assert(rfl::json::pretty == YYJSON_WRITE_PRETTY);
  static_assert(rfl::json::pretty_two_spaces == YYJSON_WRITE_PRETTY_TWO_SPACES);
  static_assert(rfl::json::escape_unicode == YYJSON_WRITE_ESCAPE_UNICODE);
  static_assert(rfl::json::escape_slashes == YYJSON_WRITE_ESCAPE_SLASHES);
  static_assert(rfl::json::write_allow_inf_and_nan ==
                YYJSON_WRITE_ALLOW_INF_AND_NAN);
  static_assert(rfl::json::inf_and_nan_as_null ==
                YYJSON_WRITE_INF_AND_NAN_AS_NULL);
  static_assert(rfl::json::write_allow_invalid_unicode ==
                YYJSON_WRITE_ALLOW_INVALID_UNICODE);
  static_assert(rfl::json::newline_at_end == YYJSON_WRITE_NEWLINE_AT_END);

  static_assert(rfl::json::stop_when_done == YYJSON_READ_STOP_WHEN_DONE);
  static_assert(rfl::json::allow_trailing_commas ==
                YYJSON_READ_ALLOW_TRAILING_COMMAS);
  static_assert(rfl::json::allow_comments == YYJSON_READ_ALLOW_COMMENTS);
  static_assert(rfl::json::read_allow_inf_and_nan ==
                YYJSON_READ_ALLOW_INF_AND_NAN);
  static_assert(rfl::json::read_allow_invalid_unicode ==
                YYJSON_READ_ALLOW_INVALID_UNICODE);
#if defined(YYJSON_VERSION_HEX) && YYJSON_VERSION_HEX >= 0x000B00
  static_assert(rfl::json::allow_bom == YYJSON_READ_ALLOW_BOM);
#endif
  static_assert(rfl::json::jsonc ==
                (YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS));
#if defined(YYJSON_VERSION_HEX) && YYJSON_VERSION_HEX >= 0x000C00
  static_assert(rfl::json::allow_ext_number == YYJSON_READ_ALLOW_EXT_NUMBER);
  static_assert(rfl::json::allow_ext_escape == YYJSON_READ_ALLOW_EXT_ESCAPE);
  static_assert(rfl::json::allow_ext_whitespace ==
                YYJSON_READ_ALLOW_EXT_WHITESPACE);
  static_assert(rfl::json::allow_single_quoted_str ==
                YYJSON_READ_ALLOW_SINGLE_QUOTED_STR);
  static_assert(rfl::json::allow_unquoted_key ==
                YYJSON_READ_ALLOW_UNQUOTED_KEY);
  static_assert(rfl::json::json5 == YYJSON_READ_JSON5);
#endif

  // Confirm read/write flags with similar names have distinct bit values.
  static_assert(rfl::json::read_allow_inf_and_nan !=
                rfl::json::write_allow_inf_and_nan);
  static_assert(rfl::json::read_allow_invalid_unicode !=
                rfl::json::write_allow_invalid_unicode);

  SUCCEED();
}

TEST(json, test_pretty_two_spaces_vs_pretty) {
  const auto cfg =
      Config{.name = "app", .port = 8080, .hosts = {"localhost", "127.0.0.1"}};

  const std::string expected_four_spaces =
      "{\n"
      "    \"name\": \"app\",\n"
      "    \"port\": 8080,\n"
      "    \"hosts\": [\n"
      "        \"localhost\",\n"
      "        \"127.0.0.1\"\n"
      "    ]\n"
      "}";

  const std::string expected_two_spaces =
      "{\n"
      "  \"name\": \"app\",\n"
      "  \"port\": 8080,\n"
      "  \"hosts\": [\n"
      "    \"localhost\",\n"
      "    \"127.0.0.1\"\n"
      "  ]\n"
      "}";

  EXPECT_EQ(rfl::json::write(cfg, rfl::json::pretty), expected_four_spaces);
  EXPECT_EQ(rfl::json::write(cfg, rfl::json::pretty_two_spaces),
            expected_two_spaces);
  EXPECT_EQ(
      rfl::json::write(cfg, rfl::json::pretty | rfl::json::pretty_two_spaces),
      expected_two_spaces);

  // Also verify to_schema accepts pretty_two_spaces.
  const auto schema = rfl::json::to_schema<Config>(
      rfl::json::pretty_two_spaces | rfl::json::newline_at_end);
  EXPECT_NE(schema.find("\n  \"$schema\""), std::string::npos);
  EXPECT_EQ(schema.back(), '\n');
}

TEST(json, test_newline_at_end) {
  const auto wrapper = TextWrapper{.text = "hello"};

  EXPECT_EQ(rfl::json::write(wrapper, rfl::json::newline_at_end),
            "{\"text\":\"hello\"}\n");
  EXPECT_EQ(rfl::json::write(
                wrapper, rfl::json::pretty_two_spaces | rfl::json::newline_at_end),
            "{\n  \"text\": \"hello\"\n}\n");

  std::ostringstream oss;
  rfl::json::write(wrapper, oss,
                   rfl::json::pretty_two_spaces | rfl::json::newline_at_end);
  EXPECT_EQ(oss.str(), "{\n  \"text\": \"hello\"\n}\n");
}

TEST(json, test_escape_slashes) {
  const auto wrapper = TextWrapper{.text = "https://example.com/a/b"};

  EXPECT_EQ(rfl::json::write(wrapper),
            R"({"text":"https://example.com/a/b"})");
  EXPECT_EQ(rfl::json::write(wrapper, rfl::json::escape_slashes),
            R"({"text":"https:\/\/example.com\/a\/b"})");

  const auto roundtrip = rfl::json::read<TextWrapper>(
      rfl::json::write(wrapper, rfl::json::escape_slashes));
  ASSERT_TRUE(roundtrip.has_value());
  EXPECT_EQ(roundtrip.value().text, wrapper.text);
}

TEST(json, test_escape_unicode) {
  // "café" in UTF-8: c a f \xC3 \xA9 (U+00E9)
  const auto wrapper = TextWrapper{.text = "caf\xC3\xA9"};

  const auto escaped = rfl::json::write(wrapper, rfl::json::escape_unicode);
  EXPECT_EQ(escaped, R"({"text":"caf\u00E9"})");

  const auto roundtrip = rfl::json::read<TextWrapper>(escaped);
  ASSERT_TRUE(roundtrip.has_value());
  EXPECT_EQ(roundtrip.value().text, wrapper.text);
}

TEST(json, test_allow_comments) {
  const std::string json_with_comments = R"({
    // Single-line comment
    "name": "svc",
    /* Multi-line
       block comment */
    "port": 443,
    "hosts": ["example.com"]
  })";

  EXPECT_FALSE(rfl::json::read<Config>(json_with_comments).has_value());

  const auto res =
      rfl::json::read<Config>(json_with_comments, rfl::json::allow_comments);
  ASSERT_TRUE(res.has_value());
  EXPECT_EQ(res.value().name, "svc");
  EXPECT_EQ(res.value().port, 443);
  EXPECT_EQ(res.value().hosts, std::vector<std::string>{"example.com"});
}

TEST(json, test_allow_trailing_commas) {
  const std::string json_with_trailing_commas = R"({
    "name": "svc",
    "port": 443,
    "hosts": ["a", "b",],
  })";

  EXPECT_FALSE(rfl::json::read<Config>(json_with_trailing_commas).has_value());

  const auto res = rfl::json::read<Config>(json_with_trailing_commas,
                                           rfl::json::allow_trailing_commas);
  ASSERT_TRUE(res.has_value());
  EXPECT_EQ(res.value().name, "svc");
  EXPECT_EQ(res.value().port, 443);
  EXPECT_EQ(res.value().hosts, (std::vector<std::string>{"a", "b"}));
}

TEST(json, test_jsonc_combined_flags) {
  const std::string jsonc_doc = R"({
    // Configuration for server
    "name": "slang",
    /* Listen port */
    "port": 9000,
    "hosts": [
      "127.0.0.1", // loopback
      "::1",
    ],
  })";

  EXPECT_FALSE(rfl::json::read<Config>(jsonc_doc).has_value());
  EXPECT_FALSE(
      rfl::json::read<Config>(jsonc_doc, rfl::json::allow_comments).has_value());
  EXPECT_FALSE(
      rfl::json::read<Config>(jsonc_doc, rfl::json::allow_trailing_commas)
          .has_value());

  const auto res_or = rfl::json::read<Config>(
      jsonc_doc, rfl::json::allow_comments | rfl::json::allow_trailing_commas);
  ASSERT_TRUE(res_or.has_value());
  EXPECT_EQ(res_or.value().name, "slang");
  EXPECT_EQ(res_or.value().port, 9000);
  EXPECT_EQ(res_or.value().hosts, (std::vector<std::string>{"127.0.0.1", "::1"}));

  const auto res_jsonc = rfl::json::read<Config>(jsonc_doc, rfl::json::jsonc);
  ASSERT_TRUE(res_jsonc.has_value());
  EXPECT_EQ(res_jsonc.value().name, "slang");

  // Also test istream overload with jsonc flag.
  std::istringstream iss(jsonc_doc);
  const auto res_stream = rfl::json::read<Config>(iss, rfl::json::jsonc);
  ASSERT_TRUE(res_stream.has_value());
  EXPECT_EQ(res_stream.value().port, 9000);
}

#if defined(YYJSON_VERSION_HEX) && YYJSON_VERSION_HEX >= 0x000B00
TEST(json, test_allow_bom) {
  const std::string json_with_bom = "\xEF\xBB\xBF{\"text\":\"bom\"}";

  EXPECT_FALSE(rfl::json::read<TextWrapper>(json_with_bom).has_value());

  const auto res =
      rfl::json::read<TextWrapper>(json_with_bom, rfl::json::allow_bom);
  ASSERT_TRUE(res.has_value());
  EXPECT_EQ(res.value().text, "bom");
}
#endif

TEST(json, test_stop_when_done) {
  const std::string ndjson = "{\"text\":\"first\"}\n{\"text\":\"second\"}\n";

  EXPECT_FALSE(rfl::json::read<TextWrapper>(ndjson).has_value());

  const auto res =
      rfl::json::read<TextWrapper>(ndjson, rfl::json::stop_when_done);
  ASSERT_TRUE(res.has_value());
  EXPECT_EQ(res.value().text, "first");
}

TEST(json, test_inf_and_nan_flags) {
  const auto vals = FloatingValues{
      .pos_inf = std::numeric_limits<double>::infinity(),
      .neg_inf = -std::numeric_limits<double>::infinity(),
      .nan_val = std::numeric_limits<double>::quiet_NaN(),
  };

  // Default write rejects Inf/NaN.
  EXPECT_THROW(rfl::json::write(vals), std::runtime_error);

  // write_allow_inf_and_nan emits Infinity/-Infinity/NaN literals.
  const auto serialized =
      rfl::json::write(vals, rfl::json::write_allow_inf_and_nan);
  EXPECT_EQ(serialized,
            R"({"pos_inf":Infinity,"neg_inf":-Infinity,"nan_val":NaN})");

  // Default read rejects Inf/NaN.
  EXPECT_FALSE(rfl::json::read<FloatingValues>(serialized).has_value());

  // read_allow_inf_and_nan parses them back accurately.
  const auto parsed = rfl::json::read<FloatingValues>(
      serialized, rfl::json::read_allow_inf_and_nan);
  ASSERT_TRUE(parsed.has_value());
  EXPECT_TRUE(std::isinf(parsed.value().pos_inf) && parsed.value().pos_inf > 0);
  EXPECT_TRUE(std::isinf(parsed.value().neg_inf) && parsed.value().neg_inf < 0);
  EXPECT_TRUE(std::isnan(parsed.value().nan_val));

  // inf_and_nan_as_null serializes Inf/NaN as null.
  const auto as_null = rfl::json::write(vals, rfl::json::inf_and_nan_as_null);
  EXPECT_EQ(as_null, R"({"pos_inf":null,"neg_inf":null,"nan_val":null})");

  const auto parsed_null = rfl::json::read<OptionalFloatingValues>(as_null);
  ASSERT_TRUE(parsed_null.has_value());
  EXPECT_FALSE(parsed_null.value().pos_inf.has_value());
  EXPECT_FALSE(parsed_null.value().neg_inf.has_value());
  EXPECT_FALSE(parsed_null.value().nan_val.has_value());
}

TEST(json, test_invalid_unicode_flags) {
  const std::string invalid_utf8_json = "{\"text\":\"bad\xFF\"}";

  EXPECT_FALSE(rfl::json::read<TextWrapper>(invalid_utf8_json).has_value());

  const auto parsed = rfl::json::read<TextWrapper>(
      invalid_utf8_json, rfl::json::read_allow_invalid_unicode);
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(parsed.value().text, std::string("bad\xFF"));

  EXPECT_THROW(rfl::json::write(parsed.value()), std::runtime_error);
  const auto written =
      rfl::json::write(parsed.value(), rfl::json::write_allow_invalid_unicode);
  EXPECT_EQ(written, invalid_utf8_json);
}

#if defined(YYJSON_VERSION_HEX) && YYJSON_VERSION_HEX >= 0x000C00
TEST(json, test_json5_flags) {
  const std::string json5_doc = R"({
    // JSON5 document
    name: 'json5-app',
    port: 0x1F90,
    hosts: ['localhost',],
  })";

  EXPECT_FALSE(rfl::json::read<Config>(json5_doc).has_value());

  const auto res = rfl::json::read<Config>(json5_doc, rfl::json::json5);
  ASSERT_TRUE(res.has_value());
  EXPECT_EQ(res.value().name, "json5-app");
  EXPECT_EQ(res.value().port, 0x1F90);
  EXPECT_EQ(res.value().hosts, std::vector<std::string>{"localhost"});

  // Also test composing individual JSON5 extension flags.
  const auto res_composed = rfl::json::read<Config>(
      json5_doc, rfl::json::allow_comments | rfl::json::allow_trailing_commas |
                     rfl::json::allow_unquoted_key |
                     rfl::json::allow_single_quoted_str |
                     rfl::json::allow_ext_number);
  ASSERT_TRUE(res_composed.has_value());
  EXPECT_EQ(res_composed.value().name, "json5-app");
  EXPECT_EQ(res_composed.value().port, 0x1F90);
}
#endif

TEST(json, test_save_and_load_with_flags) {
  const auto path =
      (std::filesystem::temp_directory_path() / "rfl_test_json_flags.json")
          .string();

  const auto cfg =
      Config{.name = "saved", .port = 8081, .hosts = {"127.0.0.1"}};

  const auto save_res = rfl::json::save(
      path, cfg, rfl::json::pretty_two_spaces | rfl::json::newline_at_end);
  ASSERT_TRUE(save_res.has_value());

  const auto load_res = rfl::json::load<Config>(path, rfl::json::jsonc);
  ASSERT_TRUE(load_res.has_value());
  EXPECT_EQ(load_res.value().name, "saved");
  EXPECT_EQ(load_res.value().port, 8081);
  EXPECT_EQ(load_res.value().hosts, std::vector<std::string>{"127.0.0.1"});

  std::filesystem::remove(path);
}

}  // namespace test_json_flags
