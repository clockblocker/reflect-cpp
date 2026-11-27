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
#include <variant>
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

struct WithOptionalDmi {
  int port = 8080;
  std::optional<int> opt_with_default = 5;
  std::unique_ptr<int> uptr = std::make_unique<int>(7);
  std::shared_ptr<int> sptr = std::make_shared<int>(9);
  rfl::Rename<"opt_renamed", std::optional<int>> opt_renamed = 11;
  const std::optional<int> const_opt = 15;
};

struct PositiveNoDefault {
  static rfl::Result<NoDefault> validate(NoDefault _v) {
    if (_v.v <= 0) {
      return rfl::error("Must be positive");
    }
    return _v;
  }
};

struct NoDefault2 {
  using ReflectionType = std::string;
  std::string s;
  NoDefault2() = delete;
  explicit NoDefault2(std::string _s) : s(std::move(_s)) {}
  const std::string& reflection() const { return s; }
};

struct WrappedNoDefaultFields {
  rfl::Description<"desc", NoDefault> desc;
  rfl::Deprecated<"old", "dep", NoDefault> dep;
  rfl::Commented<NoDefault> commented;
  rfl::Box<NoDefault> box;
  rfl::Ref<NoDefault> ref;
  rfl::Attribute<NoDefault> attr;
  rfl::Validator<NoDefault, PositiveNoDefault> validated;
  rfl::Variant<NoDefault, NoDefault2> variant;
  rfl::SkipSerialization<NoDefault> skipped_ser;
  std::vector<int> tags;
};

struct WithNullableVariants {
  std::variant<int, std::optional<std::string>> std_var = 42;
  rfl::Variant<int, std::optional<std::string>> rfl_var = 42;
  std::variant<int, std::string> plain_std_var = 7;
  rfl::Variant<int, std::string> plain_rfl_var = 9;
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

// Omitted optional with DMI:
// - DefaultIfNull alone resets omitted nullable fields to empty (matching
//   default read without DefaultIfMissing), and explicit null also decodes to
//   empty.
// - DefaultIfMissing + DefaultIfNull preserves the DMI when omitted, while
//   explicit null clears it to empty.
TEST(json, test_default_if_null_optional_omitted_vs_null) {
  // Baseline without processors: omitted nullable fields become empty.
  const auto res_baseline =
      rfl::json::read<WithOptionalDmi>(R"({"port": 80})");
  ASSERT_TRUE(res_baseline.has_value()) << res_baseline.error().what();
  EXPECT_EQ(res_baseline.value().port, 80);
  EXPECT_FALSE(res_baseline.value().opt_with_default.has_value());
  EXPECT_EQ(res_baseline.value().uptr, nullptr);
  EXPECT_EQ(res_baseline.value().sptr, nullptr);
  EXPECT_FALSE(res_baseline.value().opt_renamed.value().has_value());
  EXPECT_FALSE(res_baseline.value().const_opt.has_value());

  // DefaultIfNull alone + omitted nullable fields: must match baseline (empty).
  const auto res_null_only_omitted =
      rfl::json::read<WithOptionalDmi, rfl::DefaultIfNull>(R"({"port": null})");
  ASSERT_TRUE(res_null_only_omitted.has_value())
      << res_null_only_omitted.error().what();
  EXPECT_EQ(res_null_only_omitted.value().port, 8080);
  EXPECT_FALSE(res_null_only_omitted.value().opt_with_default.has_value());
  EXPECT_EQ(res_null_only_omitted.value().uptr, nullptr);
  EXPECT_EQ(res_null_only_omitted.value().sptr, nullptr);
  EXPECT_FALSE(res_null_only_omitted.value().opt_renamed.value().has_value());
  EXPECT_FALSE(res_null_only_omitted.value().const_opt.has_value());

  // DefaultIfNull alone + explicit null: also empty.
  const std::string explicit_null_json = R"({
    "port": null,
    "opt_with_default": null,
    "uptr": null,
    "sptr": null,
    "opt_renamed": null,
    "const_opt": null
  })";
  const auto res_null_only_explicit =
      rfl::json::read<WithOptionalDmi, rfl::DefaultIfNull>(explicit_null_json);
  ASSERT_TRUE(res_null_only_explicit.has_value())
      << res_null_only_explicit.error().what();
  EXPECT_EQ(res_null_only_explicit.value().port, 8080);
  EXPECT_FALSE(res_null_only_explicit.value().opt_with_default.has_value());
  EXPECT_EQ(res_null_only_explicit.value().uptr, nullptr);
  EXPECT_EQ(res_null_only_explicit.value().sptr, nullptr);
  EXPECT_FALSE(res_null_only_explicit.value().opt_renamed.value().has_value());
  EXPECT_FALSE(res_null_only_explicit.value().const_opt.has_value());

  // DefaultIfMissing + DefaultIfNull + omitted: preserves DMIs.
  const auto res_combined_omitted =
      rfl::json::read<WithOptionalDmi, rfl::DefaultIfMissing,
                      rfl::DefaultIfNull>(R"({"port": null})");
  ASSERT_TRUE(res_combined_omitted.has_value())
      << res_combined_omitted.error().what();
  EXPECT_EQ(res_combined_omitted.value().port, 8080);
  ASSERT_TRUE(res_combined_omitted.value().opt_with_default.has_value());
  EXPECT_EQ(*res_combined_omitted.value().opt_with_default, 5);
  ASSERT_NE(res_combined_omitted.value().uptr, nullptr);
  EXPECT_EQ(*res_combined_omitted.value().uptr, 7);
  ASSERT_NE(res_combined_omitted.value().sptr, nullptr);
  EXPECT_EQ(*res_combined_omitted.value().sptr, 9);
  ASSERT_TRUE(res_combined_omitted.value().opt_renamed.value().has_value());
  EXPECT_EQ(*res_combined_omitted.value().opt_renamed.value(), 11);
  ASSERT_TRUE(res_combined_omitted.value().const_opt.has_value());
  EXPECT_EQ(*res_combined_omitted.value().const_opt, 15);

  // DefaultIfMissing + DefaultIfNull + explicit null: resets to empty.
  const auto res_combined_explicit =
      rfl::json::read<WithOptionalDmi, rfl::DefaultIfMissing,
                      rfl::DefaultIfNull>(explicit_null_json);
  ASSERT_TRUE(res_combined_explicit.has_value())
      << res_combined_explicit.error().what();
  EXPECT_EQ(res_combined_explicit.value().port, 8080);
  EXPECT_FALSE(res_combined_explicit.value().opt_with_default.has_value());
  EXPECT_EQ(res_combined_explicit.value().uptr, nullptr);
  EXPECT_EQ(res_combined_explicit.value().sptr, nullptr);
  EXPECT_FALSE(res_combined_explicit.value().opt_renamed.value().has_value());
  EXPECT_FALSE(res_combined_explicit.value().const_opt.has_value());
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

  // With DeferredDefaults, supplying a valid value for a validator whose
  // default constructor throws succeeds, while null falls back to the invalid
  // default and returns a clean error rather than terminating.
  const auto ok_supplied =
      rfl::json::read<WithThrowingValidatorDefault, rfl::DefaultIfNull>(
          R"({"name": "svc", "tags": ["a"]})");
  ASSERT_TRUE(ok_supplied.has_value()) << ok_supplied.error().what();
  EXPECT_EQ(ok_supplied.value().name, "svc");
  EXPECT_EQ(ok_supplied.value().tags.value(), (std::vector<std::string>{"a"}));

  const auto fail_null =
      rfl::json::read<WithThrowingValidatorDefault, rfl::DefaultIfNull>(
          R"({"name": "svc", "tags": null})");
  ASSERT_FALSE(fail_null.has_value());
  EXPECT_NE(fail_null.error().what().find("tags"), std::string::npos)
      << fail_null.error().what();

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

// Regression test: wrappers around non-default-constructible types must report
// std::is_default_constructible_v == false and compile cleanly under
// DefaultIfNull even when reading structs that contain them.
TEST(json, test_default_if_null_wrapped_non_default_constructible) {
  static_assert(
      !std::is_default_constructible_v<rfl::Description<"x", NoDefault>>);
  static_assert(
      !std::is_default_constructible_v<rfl::Deprecated<"m", "x", NoDefault>>);
  static_assert(!std::is_default_constructible_v<rfl::Commented<NoDefault>>);
  static_assert(!std::is_default_constructible_v<
                rfl::Validator<NoDefault, PositiveNoDefault>>);
  static_assert(
      !std::is_default_constructible_v<rfl::Variant<NoDefault, int>>);
  static_assert(
      std::is_default_constructible_v<rfl::Variant<int, NoDefault>>);
  static_assert(
      !std::is_default_constructible_v<rfl::SkipSerialization<NoDefault>>);
  static_assert(!std::is_default_constructible_v<rfl::Attribute<NoDefault>>);
  static_assert(
      !rfl::internal::is_safe_default_constructible_v<rfl::Box<NoDefault>>);
  static_assert(
      !rfl::internal::is_safe_default_constructible_v<rfl::Ref<NoDefault>>);
  static_assert(!std::is_default_constructible_v<rfl::Rename<"r", NoDefault>>);
  static_assert(!std::is_default_constructible_v<rfl::DefaultVal<NoDefault>>);
  static_assert(!std::is_default_constructible_v<rfl::Short<"s", NoDefault>>);
  static_assert(!std::is_default_constructible_v<rfl::Positional<NoDefault>>);
  static_assert(!std::is_default_constructible_v<WrappedNoDefaultFields>);
  static_assert(
      !rfl::internal::is_safe_default_constructible_v<WrappedNoDefaultFields>);

  const std::string valid_json = R"({
    "desc": 1,
    "dep": 2,
    "commented": 3,
    "box": 4,
    "ref": 5,
    "attr": 6,
    "validated": 7,
    "variant": "hello",
    "skipped_ser": 8,
    "tags": null
  })";
  const auto ok =
      rfl::json::read<WrappedNoDefaultFields, rfl::DefaultIfNull>(valid_json);
  ASSERT_TRUE(ok.has_value()) << ok.error().what();
  EXPECT_EQ(ok.value().desc.value().v, 1);
  EXPECT_EQ(ok.value().dep.value().v, 2);
  EXPECT_EQ(ok.value().commented.value().v, 3);
  EXPECT_EQ(ok.value().box->v, 4);
  EXPECT_EQ(ok.value().ref->v, 5);
  EXPECT_EQ(ok.value().attr.value().v, 6);
  EXPECT_EQ(ok.value().validated.value().v, 7);
  ASSERT_TRUE(rfl::holds_alternative<NoDefault2>(ok.value().variant));
  EXPECT_EQ(rfl::get<NoDefault2>(ok.value().variant).s, "hello");
  EXPECT_EQ(ok.value().skipped_ser.value().v, 8);
  EXPECT_TRUE(ok.value().tags.empty());

  // Null for non-default-constructible wrapped fields fails cleanly.
  EXPECT_FALSE(
      (rfl::json::read<WrappedNoDefaultFields, rfl::DefaultIfNull>("null"))
          .has_value());
  EXPECT_FALSE((rfl::json::read<rfl::Box<NoDefault>, rfl::DefaultIfNull>("null"))
                   .has_value());
  EXPECT_FALSE((rfl::json::read<rfl::Ref<NoDefault>, rfl::DefaultIfNull>("null"))
                   .has_value());
  EXPECT_FALSE(
      (rfl::json::read<rfl::Description<"x", NoDefault>, rfl::DefaultIfNull>(
           "null"))
          .has_value());
}

// Variants containing a nullable alternative must decode explicit null as that
// nullable alternative (std::nullopt) rather than default-constructing the
// first alternative.
TEST(json, test_default_if_null_variants_with_nullable_alternative) {
  using StdVar = std::variant<int, std::optional<std::string>>;
  using RflVar = rfl::Variant<int, std::optional<std::string>>;

  const auto sv_null = rfl::json::read<StdVar, rfl::DefaultIfNull>("null");
  ASSERT_TRUE(sv_null.has_value()) << sv_null.error().what();
  ASSERT_TRUE(std::holds_alternative<std::optional<std::string>>(sv_null.value()));
  EXPECT_FALSE(std::get<std::optional<std::string>>(sv_null.value()).has_value());

  const auto rv_null = rfl::json::read<RflVar, rfl::DefaultIfNull>("null");
  ASSERT_TRUE(rv_null.has_value()) << rv_null.error().what();
  ASSERT_TRUE(rfl::holds_alternative<std::optional<std::string>>(rv_null.value()));
  EXPECT_FALSE(rfl::get<std::optional<std::string>>(rv_null.value()).has_value());

  // Non-null values still decode as the matching alternative.
  const auto sv_int = rfl::json::read<StdVar, rfl::DefaultIfNull>("42");
  ASSERT_TRUE(sv_int.has_value()) << sv_int.error().what();
  ASSERT_TRUE(std::holds_alternative<int>(sv_int.value()));
  EXPECT_EQ(std::get<int>(sv_int.value()), 42);

  // Non-nullable variants still default-construct to the first alternative.
  const auto plain_sv =
      rfl::json::read<std::variant<int, std::string>, rfl::DefaultIfNull>(
          "null");
  ASSERT_TRUE(plain_sv.has_value()) << plain_sv.error().what();
  ASSERT_TRUE(std::holds_alternative<int>(plain_sv.value()));
  EXPECT_EQ(std::get<int>(plain_sv.value()), 0);

  const auto plain_rv =
      rfl::json::read<rfl::Variant<int, std::string>, rfl::DefaultIfNull>(
          "null");
  ASSERT_TRUE(plain_rv.has_value()) << plain_rv.error().what();
  ASSERT_TRUE(rfl::holds_alternative<int>(plain_rv.value()));
  EXPECT_EQ(rfl::get<int>(plain_rv.value()), 0);

  // Inside a struct with DMIs: nullable variants decode explicit null as
  // std::nullopt, while non-nullable variants preserve their DMI.
  const auto in_struct =
      rfl::json::read<WithNullableVariants, rfl::DefaultIfNull>(R"({
        "std_var": null,
        "rfl_var": null,
        "plain_std_var": null,
        "plain_rfl_var": null
      })");
  ASSERT_TRUE(in_struct.has_value()) << in_struct.error().what();
  ASSERT_TRUE(std::holds_alternative<std::optional<std::string>>(
      in_struct.value().std_var));
  EXPECT_FALSE(
      std::get<std::optional<std::string>>(in_struct.value().std_var)
          .has_value());
  ASSERT_TRUE(rfl::holds_alternative<std::optional<std::string>>(
      in_struct.value().rfl_var));
  EXPECT_FALSE(
      rfl::get<std::optional<std::string>>(in_struct.value().rfl_var)
          .has_value());
  ASSERT_TRUE(std::holds_alternative<int>(in_struct.value().plain_std_var));
  EXPECT_EQ(std::get<int>(in_struct.value().plain_std_var), 7);
  ASSERT_TRUE(rfl::holds_alternative<int>(in_struct.value().plain_rfl_var));
  EXPECT_EQ(rfl::get<int>(in_struct.value().plain_rfl_var), 9);
}

struct DT;
struct Leaf {
  int x = 0;
};
struct Node {
  rfl::Box<DT> lesser;
};
struct DT {
  std::variant<Leaf, Node> v;
};

struct Tree {
  struct Branch {
    rfl::Box<Tree> child;
  };
  std::optional<Branch> b;
};

struct DirectNode {
  int v = 1;
  rfl::Box<DirectNode> next;
};

struct BoxAndRefOnlyNoDefault {
  rfl::Box<NoDefault> box;
  rfl::Ref<NoDefault> ref;
};

// Regression test: recursive structs with incomplete types in rfl::Box /
// rfl::Ref must remain valid for normal read/write AND DefaultIfNull, while
// cyclic Box/Ref graphs and Box<NoDefault>/Ref<NoDefault> must be rejected as
// unsafe to default-construct without stack overflow or compile errors.
TEST(json, test_default_if_null_recursive_and_cyclic_box_ref) {
  static_assert(rfl::internal::is_safe_default_constructible_v<DT>);
  static_assert(rfl::internal::is_safe_default_constructible_v<Tree>);
  static_assert(!rfl::internal::is_safe_default_constructible_v<DirectNode>);
  static_assert(
      !rfl::internal::is_safe_default_constructible_v<rfl::Box<DirectNode>>);
  static_assert(
      !rfl::internal::is_safe_default_constructible_v<rfl::Ref<DirectNode>>);
  static_assert(
      !rfl::internal::is_safe_default_constructible_v<BoxAndRefOnlyNoDefault>);

  // 1. Recursive variant struct DT: normal write + read and DefaultIfNull read.
  const DT dt_orig{Node{.lesser = rfl::make_box<DT>(DT{Leaf{.x = 42}})}};
  const auto dt_json = rfl::json::write(dt_orig);

  const auto dt_normal = rfl::json::read<DT>(dt_json);
  ASSERT_TRUE(dt_normal.has_value()) << dt_normal.error().what();
  ASSERT_TRUE(std::holds_alternative<Node>(dt_normal.value().v));
  const auto& inner_dt = *std::get<Node>(dt_normal.value().v).lesser;
  ASSERT_TRUE(std::holds_alternative<Leaf>(inner_dt.v));
  EXPECT_EQ(std::get<Leaf>(inner_dt.v).x, 42);

  const auto dt_with_flag = rfl::json::read<DT, rfl::DefaultIfNull>(dt_json);
  ASSERT_TRUE(dt_with_flag.has_value()) << dt_with_flag.error().what();
  ASSERT_TRUE(std::holds_alternative<Node>(dt_with_flag.value().v));
  const auto& inner_dt2 = *std::get<Node>(dt_with_flag.value().v).lesser;
  ASSERT_TRUE(std::holds_alternative<Leaf>(inner_dt2.v));
  EXPECT_EQ(std::get<Leaf>(inner_dt2.v).x, 42);

  const auto dt_null = rfl::json::read<DT, rfl::DefaultIfNull>("null");
  ASSERT_TRUE(dt_null.has_value()) << dt_null.error().what();
  ASSERT_TRUE(std::holds_alternative<Leaf>(dt_null.value().v));
  EXPECT_EQ(std::get<Leaf>(dt_null.value().v).x, 0);

  // 2. Recursive optional-wrapped Tree: normal write + read and DefaultIfNull.
  const Tree tree_orig{
      .b = Tree::Branch{.child = rfl::make_box<Tree>(Tree{.b = std::nullopt})}};
  const auto tree_json = rfl::json::write(tree_orig);

  const auto tree_normal = rfl::json::read<Tree>(tree_json);
  ASSERT_TRUE(tree_normal.has_value()) << tree_normal.error().what();
  ASSERT_TRUE(tree_normal.value().b.has_value());
  EXPECT_FALSE(tree_normal.value().b->child->b.has_value());

  const auto tree_with_flag =
      rfl::json::read<Tree, rfl::DefaultIfNull>(tree_json);
  ASSERT_TRUE(tree_with_flag.has_value()) << tree_with_flag.error().what();
  ASSERT_TRUE(tree_with_flag.value().b.has_value());
  EXPECT_FALSE(tree_with_flag.value().b->child->b.has_value());

  const auto tree_null = rfl::json::read<Tree, rfl::DefaultIfNull>("null");
  ASSERT_TRUE(tree_null.has_value()) << tree_null.error().what();
  EXPECT_FALSE(tree_null.value().b.has_value());

  // 3. Cyclic DirectNode: must NOT stack-overflow on null.
  const auto cyclic_null =
      rfl::json::read<DirectNode, rfl::DefaultIfNull>("null");
  EXPECT_FALSE(cyclic_null.has_value());

  const auto cyclic_field_null =
      rfl::json::read<DirectNode, rfl::DefaultIfNull>(
          R"({"v": 7, "next": null})");
  EXPECT_FALSE(cyclic_field_null.has_value());

  // 4. Struct containing ONLY Box<NoDefault> and Ref<NoDefault> compiles
  // cleanly with DefaultIfNull and handles both valid input and null cleanly.
  const auto box_ref_ok =
      rfl::json::read<BoxAndRefOnlyNoDefault, rfl::DefaultIfNull>(
          R"({"box": 10, "ref": 20})");
  ASSERT_TRUE(box_ref_ok.has_value()) << box_ref_ok.error().what();
  EXPECT_EQ(box_ref_ok.value().box->v, 10);
  EXPECT_EQ(box_ref_ok.value().ref->v, 20);

  EXPECT_FALSE(
      (rfl::json::read<BoxAndRefOnlyNoDefault, rfl::DefaultIfNull>("null"))
          .has_value());
  EXPECT_FALSE((rfl::json::read<BoxAndRefOnlyNoDefault, rfl::DefaultIfNull>(
                    R"({"box": null, "ref": 20})"))
                   .has_value());
}

}  // namespace test_default_if_null
