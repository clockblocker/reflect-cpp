#include <gtest/gtest.h>

#include <array>
#include <bitset>
#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace test_default_if_null {

struct Logging {
  std::string level = "info";
  bool timestamps = true;
};

struct Config {
  int port = 8080;
  std::string host = "localhost";
  std::vector<std::string> tags;
  std::map<std::string, std::string> env;
  std::unordered_map<std::string, std::string> labels;
  Logging logging;
  std::map<std::string, Logging> modules;
};

struct NoDefault {
  using ReflectionType = int;
  int v;
  NoDefault() = delete;
  explicit NoDefault(int _v) : v(_v) {}
  int reflection() const { return v; }
};

struct NonDefaultParent {
  NoDefault req;
  std::vector<int> tags;
};

struct DefaultConstructibleWithNonDefaultField {
  NoDefault req = NoDefault(42);
  int port = 8080;
};

struct WithNullableFields {
  int port = 8080;
  std::optional<int> opt_empty;
  std::optional<int> opt_with_default = 5;
  std::unique_ptr<int> uptr = std::make_unique<int>(7);
  std::shared_ptr<int> sptr = std::make_shared<int>(9);
  rfl::Rename<"opt_renamed", std::optional<int>> opt_renamed = 11;
  rfl::DefaultVal<std::optional<int>> opt_default_val = 13;
  rfl::Generic generic_val = 99;
};

struct WithDefaultValAndRename {
  rfl::DefaultVal<int> retries = 3;
  rfl::DefaultVal<std::string> mode = "fast";
  rfl::Rename<"server_port", int> port = 9090;
  const int fixed_version = 2;
  std::vector<std::string> default_tags = {"alpha", "beta"};
  Logging custom_logging = Logging{.level = "warn", .timestamps = false};
};

struct Outer {
  struct Inner {
    int timeout = 30;
    std::string endpoint = "http://localhost";
  };
  Inner inner;
  std::vector<Logging> sinks;
};

enum class Color { red, green, blue };

using NonEmptyTags =
    rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>>;

struct WithThrowingValidatorDefault {
  std::string name;
  NonEmptyTags tags;
};

struct WithValidValidatorDefault {
  std::string name = "svc";
  NonEmptyTags tags = std::vector<std::string>{"default"};
};

// Case 1 & 2 equivalent in JSON: root null -> default-constructed root struct.
TEST(json, test_default_if_null_root_null) {
  // Default behavior without DefaultIfNull still fails.
  EXPECT_FALSE(rfl::json::read<Config>("null").has_value());

  const auto res = rfl::json::read<Config, rfl::DefaultIfNull>("null");
  ASSERT_TRUE(res.has_value()) << res.error().what();
  const auto& cfg = res.value();
  EXPECT_EQ(cfg.port, 8080);
  EXPECT_EQ(cfg.host, "localhost");
  EXPECT_TRUE(cfg.tags.empty());
  EXPECT_TRUE(cfg.env.empty());
  EXPECT_TRUE(cfg.labels.empty());
  EXPECT_EQ(cfg.logging.level, "info");
  EXPECT_TRUE(cfg.logging.timestamps);
  EXPECT_TRUE(cfg.modules.empty());
}

// Cases 3, 4, 5, 6 + DMI preservation on struct fields.
TEST(json, test_default_if_null_all_cases_and_dmis) {
  const std::string json_str = R"({
    "port": null,
    "host": null,
    "tags": null,
    "env": null,
    "labels": null,
    "logging": null,
    "modules": {
      "network": null,
      "storage": {"level": "debug", "timestamps": false}
    }
  })";

  // Without DefaultIfNull, this fails.
  EXPECT_FALSE(rfl::json::read<Config>(json_str).has_value());

  const auto res = rfl::json::read<Config, rfl::DefaultIfNull>(json_str);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  const auto& cfg = res.value();
  EXPECT_EQ(cfg.port, 8080);
  EXPECT_EQ(cfg.host, "localhost");
  EXPECT_TRUE(cfg.tags.empty());
  EXPECT_TRUE(cfg.env.empty());
  EXPECT_TRUE(cfg.labels.empty());
  EXPECT_EQ(cfg.logging.level, "info");
  EXPECT_TRUE(cfg.logging.timestamps);
  ASSERT_EQ(cfg.modules.size(), 2u);
  EXPECT_EQ(cfg.modules.at("network").level, "info");
  EXPECT_TRUE(cfg.modules.at("network").timestamps);
  EXPECT_EQ(cfg.modules.at("storage").level, "debug");
  EXPECT_FALSE(cfg.modules.at("storage").timestamps);
}

// Case 6b: Null element inside std::vector<Struct> and nested structs.
TEST(json, test_default_if_null_nested_and_vector_elements) {
  const std::string json_str = R"({
    "inner": null,
    "sinks": [null, {"level": "error", "timestamps": false}]
  })";

  EXPECT_FALSE(rfl::json::read<Outer>(json_str).has_value());

  const auto res = rfl::json::read<Outer, rfl::DefaultIfNull>(json_str);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res.value().inner.timeout, 30);
  EXPECT_EQ(res.value().inner.endpoint, "http://localhost");
  ASSERT_EQ(res.value().sinks.size(), 2u);
  EXPECT_EQ(res.value().sinks[0].level, "info");
  EXPECT_TRUE(res.value().sinks[0].timestamps);
  EXPECT_EQ(res.value().sinks[1].level, "error");
  EXPECT_FALSE(res.value().sinks[1].timestamps);
}

// Nullable types (optional, unique_ptr, shared_ptr, Generic) MUST still decode
// explicit null as std::nullopt / nullptr / Generic::Null even if they have
// non-null DMIs.
TEST(json, test_default_if_null_preserves_nullable_semantics) {
  const std::string json_str = R"({
    "port": null,
    "opt_empty": null,
    "opt_with_default": null,
    "uptr": null,
    "sptr": null,
    "opt_renamed": null,
    "opt_default_val": null,
    "generic_val": null
  })";

  const auto res =
      rfl::json::read<WithNullableFields, rfl::DefaultIfNull>(json_str);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  const auto& v = res.value();
  EXPECT_EQ(v.port, 8080);
  EXPECT_FALSE(v.opt_empty.has_value());
  EXPECT_FALSE(v.opt_with_default.has_value());
  EXPECT_EQ(v.uptr, nullptr);
  EXPECT_EQ(v.sptr, nullptr);
  EXPECT_FALSE(v.opt_renamed.value().has_value());
  EXPECT_FALSE(v.opt_default_val.value().has_value());
  EXPECT_TRUE(v.generic_val.is_null());
}

// Omitted optional with DMI keeps DMI under DefaultIfMissing + DefaultIfNull,
// whereas explicit null clears it to std::nullopt.
TEST(json, test_default_if_null_optional_omitted_vs_null) {
  const auto res_omitted =
      rfl::json::read<WithNullableFields, rfl::DefaultIfMissing,
                      rfl::DefaultIfNull>(R"({"port": null})");
  ASSERT_TRUE(res_omitted.has_value()) << res_omitted.error().what();
  EXPECT_EQ(res_omitted.value().port, 8080);
  ASSERT_TRUE(res_omitted.value().opt_with_default.has_value());
  EXPECT_EQ(*res_omitted.value().opt_with_default, 5);
  ASSERT_NE(res_omitted.value().uptr, nullptr);
  EXPECT_EQ(*res_omitted.value().uptr, 7);
  ASSERT_NE(res_omitted.value().sptr, nullptr);
  EXPECT_EQ(*res_omitted.value().sptr, 9);
}

// DefaultVal, Rename, const fields, container DMIs, and field-level struct DMIs.
TEST(json, test_default_if_null_default_val_and_dmis) {
  const std::string json_str = R"({
    "retries": null,
    "mode": null,
    "server_port": null,
    "fixed_version": null,
    "default_tags": null,
    "custom_logging": null
  })";

  const auto res =
      rfl::json::read<WithDefaultValAndRename, rfl::DefaultIfNull>(json_str);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  const auto& v = res.value();
  EXPECT_EQ(v.retries.value(), 3);
  EXPECT_EQ(v.mode.value(), "fast");
  EXPECT_EQ(v.port.value(), 9090);
  EXPECT_EQ(v.fixed_version, 2);
  EXPECT_EQ(v.default_tags, (std::vector<std::string>{"alpha", "beta"}));
  EXPECT_EQ(v.custom_logging.level, "warn");
  EXPECT_FALSE(v.custom_logging.timestamps);
}

// Missing required fields still fail when DefaultIfMissing is NOT passed,
// and succeed when combined with DefaultIfMissing (also with NoFieldNames).
TEST(json, test_default_if_null_missing_fields_vs_combined) {
  const std::string partial_json = R"({"port": null, "tags": null})";

  const auto res_only_null =
      rfl::json::read<Config, rfl::DefaultIfNull>(partial_json);
  EXPECT_FALSE(res_only_null.has_value());

  const auto res_combined =
      rfl::json::read<Config, rfl::DefaultIfMissing, rfl::DefaultIfNull>(
          partial_json);
  ASSERT_TRUE(res_combined.has_value()) << res_combined.error().what();
  EXPECT_EQ(res_combined.value().port, 8080);
  EXPECT_EQ(res_combined.value().host, "localhost");
  EXPECT_TRUE(res_combined.value().tags.empty());
  EXPECT_EQ(res_combined.value().logging.level, "info");

  // NoFieldNames + DefaultIfNull preserves DMIs for null elements.
  struct Pair {
    int port = 8080;
    std::string host = "localhost";
  };
  const auto res_no_names =
      rfl::json::read<Pair, rfl::NoFieldNames, rfl::DefaultIfNull>(
          R"([null, "example.com"])");
  ASSERT_TRUE(res_no_names.has_value()) << res_no_names.error().what();
  EXPECT_EQ(res_no_names.value().port, 8080);
  EXPECT_EQ(res_no_names.value().host, "example.com");
}

// Non-default-constructible structs and fields compile cleanly and fall back to
// normal error reporting when null cannot be default-constructed.
TEST(json, test_default_if_null_non_default_constructible) {
  EXPECT_FALSE(
      (rfl::json::read<NoDefault, rfl::DefaultIfNull>("null")).has_value());
  EXPECT_FALSE(
      (rfl::json::read<NonDefaultParent, rfl::DefaultIfNull>("null"))
          .has_value());

  const auto ok = rfl::json::read<NonDefaultParent, rfl::DefaultIfNull>(
      R"({"req": 42, "tags": null})");
  ASSERT_TRUE(ok.has_value()) << ok.error().what();
  EXPECT_EQ(ok.value().req.v, 42);
  EXPECT_TRUE(ok.value().tags.empty());

  const auto fail = rfl::json::read<NonDefaultParent, rfl::DefaultIfNull>(
      R"({"req": null, "tags": null})");
  EXPECT_FALSE(fail.has_value());

  const auto with_dmi =
      rfl::json::read<DefaultConstructibleWithNonDefaultField,
                      rfl::DefaultIfNull>(R"({"req": null, "port": null})");
  ASSERT_TRUE(with_dmi.has_value()) << with_dmi.error().what();
  EXPECT_EQ(with_dmi.value().req.v, 42);
  EXPECT_EQ(with_dmi.value().port, 8080);
}

// Validators whose default constructor throws return a clean error rather than
// terminating, while validators with a valid DMI succeed.
TEST(json, test_default_if_null_validators) {
  const auto fail_standalone =
      rfl::json::read<NonEmptyTags, rfl::DefaultIfNull>("null");
  EXPECT_FALSE(fail_standalone.has_value());

  const auto fail_struct =
      rfl::json::read<WithThrowingValidatorDefault, rfl::DefaultIfNull>(
          R"({"name": "svc", "tags": ["a"]})");
  ASSERT_FALSE(fail_struct.has_value());
  EXPECT_NE(fail_struct.error().what().find("rfl::DefaultIfNull"),
            std::string::npos)
      << fail_struct.error().what();

  const auto ok_struct =
      rfl::json::read<WithValidValidatorDefault, rfl::DefaultIfNull>(
          R"({"name": null, "tags": null})");
  ASSERT_TRUE(ok_struct.has_value()) << ok_struct.error().what();
  EXPECT_EQ(ok_struct.value().name, "svc");
  EXPECT_EQ(ok_struct.value().tags.value(),
            (std::vector<std::string>{"default"}));
}

// Format-generality: Generic reader, standalone containers, tuples, arrays,
// enums, bitsets, durations, and NamedTuples.
TEST(json, test_default_if_null_generic_and_standalone_types) {
  // Format-general check via rfl::from_generic.
  const auto from_null_generic =
      rfl::from_generic<Config, rfl::DefaultIfNull>(rfl::Generic{});
  ASSERT_TRUE(from_null_generic.has_value())
      << from_null_generic.error().what();
  EXPECT_EQ(from_null_generic.value().port, 8080);
  EXPECT_EQ(from_null_generic.value().host, "localhost");
  EXPECT_EQ(from_null_generic.value().logging.level, "info");

  EXPECT_EQ((rfl::json::read<int, rfl::DefaultIfNull>("null")).value(), 0);
  EXPECT_EQ((rfl::json::read<std::string, rfl::DefaultIfNull>("null")).value(),
            "");
  EXPECT_TRUE(
      (rfl::json::read<std::vector<int>, rfl::DefaultIfNull>("null"))
          .value()
          .empty());
  EXPECT_TRUE(
      (rfl::json::read<std::map<std::string, int>, rfl::DefaultIfNull>("null"))
          .value()
          .empty());
  EXPECT_EQ((rfl::json::read<Color, rfl::DefaultIfNull>("null")).value(),
            Color::red);
  EXPECT_EQ(
      (rfl::json::read<std::bitset<4>, rfl::DefaultIfNull>("null")).value(),
      std::bitset<4>(0));
  EXPECT_EQ(
      (rfl::json::read<std::array<int, 2>, rfl::DefaultIfNull>("null")).value(),
      (std::array<int, 2>{0, 0}));
  EXPECT_EQ(
      (rfl::json::read<std::tuple<int, std::string>, rfl::DefaultIfNull>(
           "null"))
          .value(),
      (std::tuple<int, std::string>{0, ""}));
  EXPECT_EQ(
      (rfl::json::read<std::pair<int, std::string>, rfl::DefaultIfNull>("null"))
          .value(),
      (std::pair<int, std::string>{0, ""}));
  EXPECT_EQ(
      (rfl::json::read<std::chrono::milliseconds, rfl::DefaultIfNull>("null"))
          .value(),
      std::chrono::milliseconds(0));

  using NT =
      rfl::NamedTuple<rfl::Field<"a", int>, rfl::Field<"b", std::string>>;
  const auto nt = rfl::json::read<NT, rfl::DefaultIfNull>("null");
  ASSERT_TRUE(nt.has_value());
  EXPECT_EQ(nt.value().get<"a">(), 0);
  EXPECT_EQ(nt.value().get<"b">(), "");
}

}  // namespace test_default_if_null
