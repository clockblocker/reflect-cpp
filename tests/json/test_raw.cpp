#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "write_and_read.hpp"

namespace test_raw {

struct Envelope {
  std::string type;
  rfl::json::Raw payload;
};

struct OptionalEnvelope {
  int id;
  std::optional<rfl::json::Raw> payload;
};

TEST(json, test_raw_constructors_and_accessors) {
  const rfl::json::Raw def;
  EXPECT_EQ(def.str(), "null");
  EXPECT_EQ(def.value(), "null");
  EXPECT_STREQ(def.c_str(), "null");

  static_assert(!std::is_convertible_v<std::string, rfl::json::Raw>);
  static_assert(!std::is_convertible_v<std::string_view, rfl::json::Raw>);
  static_assert(!std::is_convertible_v<const char*, rfl::json::Raw>);

  const rfl::json::Raw from_cstr(R"({"a":1})");
  const rfl::json::Raw from_sv(std::string_view{R"({"a":1})"});
  const rfl::json::Raw from_str(std::string{R"({"a":1})"});
  EXPECT_EQ(from_cstr, from_sv);
  EXPECT_EQ(from_sv, from_str);
  EXPECT_NE(from_cstr, def);

  const rfl::json::Raw from_null(static_cast<const char*>(nullptr));
  EXPECT_EQ(from_null, def);

  const rfl::json::Raw empty_raw("");
  EXPECT_EQ(empty_raw.str(), "null");
  EXPECT_EQ(empty_raw, def);
  EXPECT_EQ(rfl::json::write(empty_raw), "null");
  EXPECT_EQ(rfl::json::write(def), "null");
  EXPECT_EQ(rfl::json::write(std::vector<rfl::json::Raw>{empty_raw, def}),
            "[null,null]");
  EXPECT_EQ(rfl::json::write(Envelope{.type = "empty", .payload = empty_raw}),
            R"({"type":"empty","payload":null})");

  // Validating factory rejects malformed JSON and preserves valid JSON verbatim.
  const auto valid = rfl::json::Raw::from_string(R"({ "k" : [1, 2] })");
  ASSERT_TRUE(valid.has_value());
  EXPECT_EQ(valid->str(), R"({ "k" : [1, 2] })");
  EXPECT_FALSE(rfl::json::Raw::from_string("").has_value());
  EXPECT_FALSE(rfl::json::Raw::from_string("{not json").has_value());
  EXPECT_FALSE(rfl::json::Raw::from_string("1 2").has_value());
}

TEST(json, test_raw_root_values) {
  for (const char* sample :
       {R"({"a":1,"b":[true,false,null,"hi"]})", R"([1,2,3])", R"("hello")",
        "42", "3.5", "true", "false", "null"}) {
    write_and_read(rfl::json::Raw(sample), sample);
  }
  const auto bad = rfl::json::Raw::from_json_obj(rfl::json::Reader::YYJSONInputVar{});
  EXPECT_FALSE(bad.has_value());
}

TEST(json, test_raw_struct_field) {
  const Envelope env{
      .type = "event",
      .payload = rfl::json::Raw(R"({"nested":{"k":[1,"two",true,null]}})")};
  write_and_read(
      env,
      R"({"type":"event","payload":{"nested":{"k":[1,"two",true,null]}}})");

  const auto null_field =
      rfl::json::read<Envelope>(R"({"type":"nil","payload":null})");
  ASSERT_TRUE(null_field.has_value());
  EXPECT_EQ(null_field->payload, rfl::json::Raw("null"));

  const auto missing = rfl::json::read<Envelope>(R"({"type":"missing"})");
  EXPECT_FALSE(missing.has_value());
}

TEST(json, test_raw_optional_field) {
  const auto missing = rfl::json::read<OptionalEnvelope>(R"({"id":1})");
  ASSERT_TRUE(missing.has_value());
  EXPECT_FALSE(missing->payload.has_value());
  EXPECT_EQ(rfl::json::write(*missing), R"({"id":1})");

  const auto explicit_null =
      rfl::json::read<OptionalEnvelope>(R"({"id":2,"payload":null})");
  ASSERT_TRUE(explicit_null.has_value());
  EXPECT_FALSE(explicit_null->payload.has_value());

  const auto present =
      rfl::json::read<OptionalEnvelope>(R"({"id":3,"payload":{"x":[1,2]}})");
  ASSERT_TRUE(present.has_value());
  ASSERT_TRUE(present->payload.has_value());
  EXPECT_EQ(*present->payload, rfl::json::Raw(R"({"x":[1,2]})"));
  EXPECT_EQ(rfl::json::write(*present), R"({"id":3,"payload":{"x":[1,2]}})");
}

TEST(json, test_raw_containers) {
  const std::vector<rfl::json::Raw> vec{
      rfl::json::Raw("1"),
      rfl::json::Raw(R"("two")"),
      rfl::json::Raw(R"({"three":3})"),
      rfl::json::Raw("[4,5]"),
      rfl::json::Raw("true"),
      rfl::json::Raw("null"),
  };
  write_and_read(vec, R"([1,"two",{"three":3},[4,5],true,null])");

  const std::map<std::string, rfl::json::Raw> mp{
      {"a", rfl::json::Raw("123")},
      {"b", rfl::json::Raw(R"({"nested":true})")},
      {"c", rfl::json::Raw(R"(["x","y"])")},
  };
  write_and_read(mp, R"({"a":123,"b":{"nested":true},"c":["x","y"]})");
}

TEST(json, test_raw_passthrough_and_pretty) {
  // Non-canonical whitespace and key order must pass through untouched on write.
  const std::string unformatted = R"({  "z" : 2, "a" : [ 1, 2 ] })";
  const Envelope env{.type = "pass", .payload = rfl::json::Raw(unformatted)};
  EXPECT_EQ(rfl::json::write(env),
            R"({"type":"pass","payload":)" + unformatted + "}");
  EXPECT_EQ(rfl::json::write(rfl::json::Raw(unformatted)), unformatted);

  const auto pretty_out = rfl::json::write(env, rfl::json::pretty);
  EXPECT_NE(pretty_out.find('\n'), std::string::npos);
  EXPECT_NE(pretty_out.find(unformatted), std::string::npos);

  const auto reparsed = rfl::json::read<Envelope>(pretty_out);
  ASSERT_TRUE(reparsed.has_value());
  EXPECT_EQ(reparsed->type, "pass");
  EXPECT_EQ(reparsed->payload, rfl::json::Raw(R"({"z":2,"a":[1,2]})"));
}

TEST(json, test_raw_to_schema) {
  const auto schema = rfl::json::to_schema<OptionalEnvelope>();
  EXPECT_NE(schema.find("rfl__Generic"), std::string::npos);
  EXPECT_NE(schema.find(R"("required":["id"])"), std::string::npos);

  const auto req_schema = rfl::json::to_schema<Envelope>();
  EXPECT_NE(req_schema.find("rfl__Generic"), std::string::npos);
  EXPECT_NE(req_schema.find(R"("required":["type","payload"])"),
            std::string::npos);
}

TEST(json, test_raw_bignum_and_nested_reparse) {
  // With YYJSON_READ_BIGNUM_AS_RAW, integers wider than 64 bits survive verbatim.
  const std::string big = "1234567890123456789012345678901234567890";
  const auto big_env = rfl::json::read<Envelope>(
      R"({"type":"bignum","payload":{"amount":)" + big + "}}",
      YYJSON_READ_BIGNUM_AS_RAW);
  ASSERT_TRUE(big_env.has_value());
  EXPECT_EQ(big_env->payload.str(), R"({"amount":)" + big + "}");

  // Nested Raw inside a Raw payload can be re-parsed into a typed struct.
  const auto outer = rfl::json::read<Envelope>(
      R"({"type":"outer","payload":{"type":"inner","payload":[1,2,3]}})");
  ASSERT_TRUE(outer.has_value());
  const auto inner = rfl::json::read<Envelope>(outer->payload.str());
  ASSERT_TRUE(inner.has_value());
  EXPECT_EQ(inner->type, "inner");
  EXPECT_EQ(inner->payload.str(), "[1,2,3]");
}

}  // namespace test_raw
