#include <gtest/gtest.h>

#include <bitset>
#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/yaml.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace test_default_if_null {

using namespace std::string_view_literals;

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

enum class LogFormat { text, json };

struct Extras {
  LogFormat format = LogFormat::json;
  std::array<int, 3> dims = {1, 2, 3};
  std::bitset<4> flags;
  std::chrono::milliseconds timeout = std::chrono::milliseconds(500);
};

// Case 1: Empty YAML document -> default-constructed root struct.
TEST(yaml, test_default_if_null_empty_document) {
  // Default behavior without DefaultIfNull still fails.
  EXPECT_FALSE(rfl::yaml::read<Config>(""sv).has_value());

  const auto res = rfl::yaml::read<Config, rfl::DefaultIfNull>(""sv);
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

// Case 2: All-commented YAML document -> default-constructed root struct.
TEST(yaml, test_default_if_null_comment_only_document) {
  const std::string yaml_str =
      "# just a comment\n"
      "# another comment line\n";
  EXPECT_FALSE(rfl::yaml::read<Config>(yaml_str).has_value());

  const auto res = rfl::yaml::read<Config, rfl::DefaultIfNull>(yaml_str);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  const auto& cfg = res.value();
  EXPECT_EQ(cfg.port, 8080);
  EXPECT_EQ(cfg.host, "localhost");
  EXPECT_TRUE(cfg.tags.empty());
  EXPECT_TRUE(cfg.env.empty());
  EXPECT_EQ(cfg.logging.level, "info");
}

// Cases 3, 4, 5, 6 + DMI preservation on struct fields.
TEST(yaml, test_default_if_null_all_cases_and_dmis) {
  const std::string yaml_str =
      "port: null\n"
      "host:\n"
      "tags:\n"
      "env:\n"
      "labels: ~\n"
      "logging:\n"
      "modules:\n"
      "  network:\n"
      "  storage:\n"
      "    level: debug\n"
      "    timestamps: false\n";

  // Without DefaultIfNull, this fails.
  EXPECT_FALSE(rfl::yaml::read<Config>(yaml_str).has_value());

  const auto res = rfl::yaml::read<Config, rfl::DefaultIfNull>(yaml_str);
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
TEST(yaml, test_default_if_null_nested_and_vector_elements) {
  const std::string yaml_str =
      "inner:\n"
      "sinks:\n"
      "  - null\n"
      "  - ~\n"
      "  - level: error\n"
      "    timestamps: false\n";

  EXPECT_FALSE(rfl::yaml::read<Outer>(yaml_str).has_value());

  const auto res = rfl::yaml::read<Outer, rfl::DefaultIfNull>(yaml_str);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res.value().inner.timeout, 30);
  EXPECT_EQ(res.value().inner.endpoint, "http://localhost");
  ASSERT_EQ(res.value().sinks.size(), 3u);
  EXPECT_EQ(res.value().sinks[0].level, "info");
  EXPECT_TRUE(res.value().sinks[0].timestamps);
  EXPECT_EQ(res.value().sinks[1].level, "info");
  EXPECT_TRUE(res.value().sinks[1].timestamps);
  EXPECT_EQ(res.value().sinks[2].level, "error");
  EXPECT_FALSE(res.value().sinks[2].timestamps);
}

// Nullable types (optional, unique_ptr, shared_ptr, Generic) MUST still decode
// explicit null as std::nullopt / nullptr / Generic::Null even if they have
// non-null DMIs.
TEST(yaml, test_default_if_null_preserves_nullable_semantics) {
  const std::string yaml_str =
      "port: null\n"
      "opt_empty: null\n"
      "opt_with_default: null\n"
      "uptr: ~\n"
      "sptr:\n"
      "opt_renamed: null\n"
      "opt_default_val: null\n"
      "generic_val: null\n";

  const auto res =
      rfl::yaml::read<WithNullableFields, rfl::DefaultIfNull>(yaml_str);
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
TEST(yaml, test_default_if_null_optional_omitted_vs_null) {
  const std::string yaml_omitted = "port: null\n";
  const std::string yaml_explicit_null =
      "port: null\n"
      "opt_with_default: null\n"
      "uptr: ~\n"
      "sptr:\n"
      "opt_renamed: null\n"
      "const_opt: null\n";

  // DefaultIfNull alone + omitted: resets nullable fields to empty.
  const auto res_null_only_omitted =
      rfl::yaml::read<WithOptionalDmi, rfl::DefaultIfNull>(yaml_omitted);
  ASSERT_TRUE(res_null_only_omitted.has_value())
      << res_null_only_omitted.error().what();
  EXPECT_EQ(res_null_only_omitted.value().port, 8080);
  EXPECT_FALSE(res_null_only_omitted.value().opt_with_default.has_value());
  EXPECT_EQ(res_null_only_omitted.value().uptr, nullptr);
  EXPECT_EQ(res_null_only_omitted.value().sptr, nullptr);
  EXPECT_FALSE(res_null_only_omitted.value().opt_renamed.value().has_value());
  EXPECT_FALSE(res_null_only_omitted.value().const_opt.has_value());

  // DefaultIfNull alone + explicit null: also empty.
  const auto res_null_only_explicit =
      rfl::yaml::read<WithOptionalDmi, rfl::DefaultIfNull>(yaml_explicit_null);
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
      rfl::yaml::read<WithOptionalDmi, rfl::DefaultIfMissing,
                      rfl::DefaultIfNull>(yaml_omitted);
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
      rfl::yaml::read<WithOptionalDmi, rfl::DefaultIfMissing,
                      rfl::DefaultIfNull>(yaml_explicit_null);
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
TEST(yaml, test_default_if_null_default_val_and_dmis) {
  const std::string yaml_str =
      "retries: null\n"
      "mode:\n"
      "server_port: ~\n"
      "fixed_version: null\n"
      "default_tags: null\n"
      "custom_logging:\n";

  const auto res =
      rfl::yaml::read<WithDefaultValAndRename, rfl::DefaultIfNull>(yaml_str);
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
// and succeed when combined with DefaultIfMissing.
TEST(yaml, test_default_if_null_missing_fields_vs_combined) {
  const std::string partial_yaml =
      "port: null\n"
      "tags:\n";

  // Without DefaultIfMissing, omitted non-optional fields must still fail.
  const auto res_only_null =
      rfl::yaml::read<Config, rfl::DefaultIfNull>(partial_yaml);
  EXPECT_FALSE(res_only_null.has_value());

  // Combined DefaultIfMissing + DefaultIfNull succeeds for both missing and null.
  const auto res_combined =
      rfl::yaml::read<Config, rfl::DefaultIfMissing, rfl::DefaultIfNull>(
          partial_yaml);
  ASSERT_TRUE(res_combined.has_value()) << res_combined.error().what();
  EXPECT_EQ(res_combined.value().port, 8080);
  EXPECT_EQ(res_combined.value().host, "localhost");
  EXPECT_TRUE(res_combined.value().tags.empty());
  EXPECT_EQ(res_combined.value().logging.level, "info");
}

// Non-default-constructible structs and fields compile cleanly and fall back to
// normal error reporting when null cannot be default-constructed.
TEST(yaml, test_default_if_null_non_default_constructible) {
  // Root null for non-default-constructible types fails cleanly.
  EXPECT_FALSE(
      (rfl::yaml::read<NoDefault, rfl::DefaultIfNull>(""sv)).has_value());
  EXPECT_FALSE((rfl::yaml::read<NonDefaultParent, rfl::DefaultIfNull>(""sv))
                   .has_value());

  // Non-default-constructible parent with valid req and null tags succeeds.
  const auto ok = rfl::yaml::read<NonDefaultParent, rfl::DefaultIfNull>(
      "req: 42\ntags:\n"sv);
  ASSERT_TRUE(ok.has_value()) << ok.error().what();
  EXPECT_EQ(ok.value().req.v, 42);
  EXPECT_TRUE(ok.value().tags.empty());

  // Null for non-default-constructible field fails cleanly.
  const auto fail = rfl::yaml::read<NonDefaultParent, rfl::DefaultIfNull>(
      "req: null\ntags:\n"sv);
  EXPECT_FALSE(fail.has_value());

  // Default-constructible struct with DMI on non-default-constructible field
  // preserves its DMI on null.
  const auto with_dmi =
      rfl::yaml::read<DefaultConstructibleWithNonDefaultField,
                      rfl::DefaultIfNull>("req: null\nport: null\n"sv);
  ASSERT_TRUE(with_dmi.has_value()) << with_dmi.error().what();
  EXPECT_EQ(with_dmi.value().req.v, 42);
  EXPECT_EQ(with_dmi.value().port, 8080);
}

// Standalone containers, enums, bitsets, durations, and NamedTuples.
TEST(yaml, test_default_if_null_standalone_types) {
  const auto vec =
      rfl::yaml::read<std::vector<int>, rfl::DefaultIfNull>(""sv);
  ASSERT_TRUE(vec.has_value());
  EXPECT_TRUE(vec.value().empty());

  const auto m =
      rfl::yaml::read<std::map<std::string, int>, rfl::DefaultIfNull>(""sv);
  ASSERT_TRUE(m.has_value());
  EXPECT_TRUE(m.value().empty());

  const auto extras = rfl::yaml::read<Extras, rfl::DefaultIfNull>(
      "format: null\ndims: null\nflags: null\ntimeout: null\n"sv);
  ASSERT_TRUE(extras.has_value()) << extras.error().what();
  EXPECT_EQ(extras.value().format, LogFormat::json);
  EXPECT_EQ(extras.value().dims, (std::array<int, 3>{1, 2, 3}));
  EXPECT_EQ(extras.value().flags, std::bitset<4>(0));
  EXPECT_EQ(extras.value().timeout, std::chrono::milliseconds(500));

  const auto root_extras = rfl::yaml::read<Extras, rfl::DefaultIfNull>(""sv);
  ASSERT_TRUE(root_extras.has_value());
  EXPECT_EQ(root_extras.value().format, LogFormat::json);
  EXPECT_EQ(root_extras.value().timeout, std::chrono::milliseconds(500));

  const auto standalone_enum =
      rfl::yaml::read<LogFormat, rfl::DefaultIfNull>("null"sv);
  ASSERT_TRUE(standalone_enum.has_value());
  EXPECT_EQ(standalone_enum.value(), LogFormat::text);

  const auto standalone_bitset =
      rfl::yaml::read<std::bitset<4>, rfl::DefaultIfNull>("null"sv);
  ASSERT_TRUE(standalone_bitset.has_value());
  EXPECT_EQ(standalone_bitset.value(), std::bitset<4>(0));

  using NT =
      rfl::NamedTuple<rfl::Field<"a", int>, rfl::Field<"b", std::string>>;
  const auto nt = rfl::yaml::read<NT, rfl::DefaultIfNull>(""sv);
  ASSERT_TRUE(nt.has_value());
  EXPECT_EQ(nt.value().get<"a">(), 0);
  EXPECT_EQ(nt.value().get<"b">(), "");
}

TEST(yaml, test_default_if_null_wrapped_non_default_and_nullable_variants) {
  const std::string valid_yaml =
      "desc: 1\n"
      "dep: 2\n"
      "commented: 3\n"
      "box: 4\n"
      "ref: 5\n"
      "attr: 6\n"
      "validated: 7\n"
      "variant: hello\n"
      "skipped_ser: 8\n"
      "tags:\n";
  const auto ok =
      rfl::yaml::read<WrappedNoDefaultFields, rfl::DefaultIfNull>(valid_yaml);
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

  using StdVar = std::variant<int, std::optional<std::string>>;
  using RflVar = rfl::Variant<int, std::optional<std::string>>;

  const auto sv_null = rfl::yaml::read<StdVar, rfl::DefaultIfNull>("null"sv);
  ASSERT_TRUE(sv_null.has_value()) << sv_null.error().what();
  ASSERT_TRUE(
      std::holds_alternative<std::optional<std::string>>(sv_null.value()));
  EXPECT_FALSE(
      std::get<std::optional<std::string>>(sv_null.value()).has_value());

  const auto rv_null = rfl::yaml::read<RflVar, rfl::DefaultIfNull>(""sv);
  ASSERT_TRUE(rv_null.has_value()) << rv_null.error().what();
  ASSERT_TRUE(
      rfl::holds_alternative<std::optional<std::string>>(rv_null.value()));
  EXPECT_FALSE(
      rfl::get<std::optional<std::string>>(rv_null.value()).has_value());
}

}  // namespace test_default_if_null
