#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <type_traits>
#include <unordered_set>

#include "write_and_read.hpp"

namespace test_alias {

struct TransmissionSettings {
  rfl::Alias<"download-dir", rfl::Alias<"downloadDir", std::string>>
      download_dir;
  rfl::Alias<"speed-limit-down", rfl::Alias<"speedLimitDown", int>>
      speed_limit_down;
};

TEST(json, test_alias_single_and_chained) {
  // Read via canonical primary name.
  const auto from_canonical = rfl::json::read<TransmissionSettings>(
      R"({"download_dir":"/downloads","speed_limit_down":100})");
  ASSERT_TRUE(from_canonical) << from_canonical.error().what();
  EXPECT_EQ(from_canonical->download_dir, "/downloads");
  EXPECT_EQ(from_canonical->speed_limit_down, 100);

  // Read via kebab-case legacy aliases.
  const auto from_kebab = rfl::json::read<TransmissionSettings>(
      R"({"download-dir":"/legacy","speed-limit-down":250})");
  ASSERT_TRUE(from_kebab) << from_kebab.error().what();
  EXPECT_EQ(from_kebab->download_dir, "/legacy");
  EXPECT_EQ(from_kebab->speed_limit_down, 250);

  // Read via camelCase legacy aliases.
  const auto from_camel = rfl::json::read<TransmissionSettings>(
      R"({"downloadDir":"/camel","speedLimitDown":500})");
  ASSERT_TRUE(from_camel) << from_camel.error().what();
  EXPECT_EQ(from_camel->download_dir, "/camel");
  EXPECT_EQ(from_camel->speed_limit_down, 500);

  // Serialization ALWAYS emits the canonical primary wire names.
  EXPECT_EQ(rfl::json::write(*from_kebab),
            R"({"download_dir":"/legacy","speed_limit_down":250})");
  EXPECT_EQ(rfl::json::write(*from_camel),
            R"({"download_dir":"/camel","speed_limit_down":500})");
}

TEST(json, test_alias_no_extra_fields) {
  // Aliases must NOT be rejected by rfl::NoExtraFields.
  const auto ok = rfl::json::read<TransmissionSettings, rfl::NoExtraFields>(
      R"({"download-dir":"/downloads","speedLimitDown":42})");
  ASSERT_TRUE(ok) << ok.error().what();
  EXPECT_EQ(ok->download_dir, "/downloads");
  EXPECT_EQ(ok->speed_limit_down, 42);

  // Truly unknown keys are still rejected.
  const auto bad = rfl::json::read<TransmissionSettings, rfl::NoExtraFields>(
      R"({"download-dir":"/downloads","speedLimitDown":42,"unknown":1})");
  EXPECT_FALSE(bad);
}

TEST(json, test_alias_duplicate_and_conflicting_keys_rejected) {
  // Primary + alias in the same object.
  const auto primary_then_alias = rfl::json::read<TransmissionSettings>(
      R"({"download_dir":"/a","download-dir":"/b","speed_limit_down":1})");
  EXPECT_FALSE(primary_then_alias);

  // Alias + primary in the same object.
  const auto alias_then_primary = rfl::json::read<TransmissionSettings>(
      R"({"download-dir":"/a","download_dir":"/b","speed_limit_down":1})");
  EXPECT_FALSE(alias_then_primary);

  // Two aliases of the same field in the same object.
  const auto two_aliases = rfl::json::read<TransmissionSettings>(
      R"({"download-dir":"/a","downloadDir":"/b","speed_limit_down":1})");
  EXPECT_FALSE(two_aliases);

  // Also rejected under NoExtraFields with clear error.
  const auto with_no_extra =
      rfl::json::read<TransmissionSettings, rfl::NoExtraFields>(
          R"({"download-dir":"/a","downloadDir":"/b","speed_limit_down":1})");
  EXPECT_FALSE(with_no_extra);
}

struct WithRename {
  rfl::Rename<"download_dir", rfl::Alias<"download-dir", std::string>> dir_a;
  rfl::Alias<"upload-dir", rfl::Rename<"upload_dir", std::string>> dir_b;
  rfl::Alias<"old-c", rfl::Rename<"canonical_c", rfl::Alias<"oldC", int>>> c;
};

TEST(json, test_alias_with_rename_both_orders) {
  const auto r1 = rfl::json::read<WithRename, rfl::NoExtraFields>(
      R"({"download-dir":"/a","upload-dir":"/b","oldC":7})");
  ASSERT_TRUE(r1) << r1.error().what();
  EXPECT_EQ(r1->dir_a.value(), "/a");
  EXPECT_EQ(r1->dir_b.value().value(), "/b");
  EXPECT_EQ(r1->c.value().value(), 7);

  EXPECT_EQ(rfl::json::write(*r1),
            R"({"download_dir":"/a","upload_dir":"/b","canonical_c":7})");

  // Rename overrides SnakeCaseToCamelCase regardless of wrapper order.
  const auto r2 =
      rfl::json::read<WithRename, rfl::SnakeCaseToCamelCase, rfl::NoExtraFields>(
          R"({"download_dir":"/x","upload_dir":"/y","old-c":9})");
  ASSERT_TRUE(r2) << r2.error().what();
  EXPECT_EQ(
      rfl::json::write<rfl::SnakeCaseToCamelCase>(*r2),
      R"({"download_dir":"/x","upload_dir":"/y","canonical_c":9})");
}

struct WithCamelCase {
  rfl::Alias<"download-dir", std::string> download_dir;
  rfl::Alias<"max_peers", int> max_peers;
};

TEST(json, test_alias_with_snake_case_to_camel_case) {
  // Primary wire names become downloadDir and maxPeers; aliases stay as written.
  const auto from_alias =
      rfl::json::read<WithCamelCase, rfl::SnakeCaseToCamelCase,
                      rfl::NoExtraFields>(
          R"({"download-dir":"/dl","max_peers":50})");
  ASSERT_TRUE(from_alias) << from_alias.error().what();
  EXPECT_EQ(from_alias->download_dir, "/dl");
  EXPECT_EQ(from_alias->max_peers, 50);

  EXPECT_EQ(rfl::json::write<rfl::SnakeCaseToCamelCase>(*from_alias),
            R"({"downloadDir":"/dl","maxPeers":50})");

  // Conflict between transformed primary name and alias is rejected.
  const auto conflict =
      rfl::json::read<WithCamelCase, rfl::SnakeCaseToCamelCase>(
          R"({"downloadDir":"/a","download-dir":"/b","maxPeers":1})");
  EXPECT_FALSE(conflict);
}

struct WithDefaults {
  rfl::Alias<"old-host", rfl::DefaultVal<std::string>> host =
      std::string("localhost");
  rfl::DefaultVal<rfl::Alias<"old-port", int>> port = 8080;
  rfl::Alias<"old-name", std::string> name = "default-name";
};

TEST(json, test_alias_with_default_val_and_default_if_missing) {
  // Omitted DefaultVal fields keep defaults; present alias overrides.
  const auto r1 = rfl::json::read<WithDefaults, rfl::NoExtraFields>(
      R"({"old-name":"svc"})");
  ASSERT_TRUE(r1) << r1.error().what();
  EXPECT_EQ(r1->host.value().value(), "localhost");
  EXPECT_EQ(r1->port.value(), 8080);
  EXPECT_EQ(r1->name, "svc");

  // All supplied via aliases.
  const auto r2 = rfl::json::read<WithDefaults, rfl::NoExtraFields>(
      R"({"old-host":"127.0.0.1","old-port":9090,"old-name":"api"})");
  ASSERT_TRUE(r2) << r2.error().what();
  EXPECT_EQ(r2->host.value().value(), "127.0.0.1");
  EXPECT_EQ(r2->port.value(), 9090);
  EXPECT_EQ(r2->name, "api");

  // DefaultIfMissing with alias.
  const auto r3 =
      rfl::json::read<WithDefaults, rfl::DefaultIfMissing, rfl::NoExtraFields>(
          R"({"old-port":3000})");
  ASSERT_TRUE(r3) << r3.error().what();
  EXPECT_EQ(r3->host.value().value(), "localhost");
  EXPECT_EQ(r3->port.value(), 3000);
  EXPECT_EQ(r3->name, "default-name");

  // Conflicting primary + alias is rejected in ViewReaderWithDefault too!
  const auto conflict = rfl::json::read<WithDefaults>(
      R"({"host":"a","old-host":"b","name":"x"})");
  EXPECT_FALSE(conflict);

  const auto conflict_dim =
      rfl::json::read<WithDefaults, rfl::DefaultIfMissing>(
          R"({"port":1,"old-port":2})");
  EXPECT_FALSE(conflict_dim);
}

struct WithOptionals {
  rfl::Alias<"old-a", std::optional<std::string>> a;
  std::optional<rfl::Alias<"old-b", int>> b;
  rfl::Description<"Field C", rfl::Alias<"old-c", std::optional<int>>> c;
  rfl::Alias<"old-d", rfl::Deprecated<"Use d", "Field D", std::optional<int>>>
      d;
};

TEST(json, test_alias_with_optionals_description_deprecated) {
  // All omitted -> writes empty object.
  write_and_read(WithOptionals{}, R"({})");

  // All provided via aliases.
  const auto res = rfl::json::read<WithOptionals, rfl::NoExtraFields>(
      R"({"old-a":"hi","old-b":2,"old-c":3,"old-d":4})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->a.value(), "hi");
  ASSERT_TRUE(res->b.has_value());
  EXPECT_EQ(*res->b, 2);
  EXPECT_EQ(res->c.value().value(), 3);
  EXPECT_EQ(res->d.value().value(), 4);

  EXPECT_EQ(rfl::json::write(*res), R"({"a":"hi","b":2,"c":3,"d":4})");
}

struct WithValidator {
  rfl::Alias<"old-port",
             rfl::Validator<int, rfl::Minimum<1>, rfl::Maximum<65535>>>
      port;
};

TEST(json, test_alias_with_validator) {
  const auto ok = rfl::json::read<WithValidator>(R"({"old-port":443})");
  ASSERT_TRUE(ok) << ok.error().what();
  EXPECT_EQ(ok->port.value().value(), 443);
  EXPECT_EQ(rfl::json::write(*ok), R"({"port":443})");

  const auto bad = rfl::json::read<WithValidator>(R"({"old-port":0})");
  EXPECT_FALSE(bad);
}

struct WithExtraFields {
  rfl::Alias<"old-id", int> id;
  rfl::ExtraFields<std::string> extra;
};

TEST(json, test_alias_with_extra_fields) {
  const auto res = rfl::json::read<WithExtraFields>(
      R"({"old-id":7,"other":"kept"})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->id, 7);
  EXPECT_EQ(res->extra.size(), 1u);
  EXPECT_EQ(res->extra.get("other").value(), "kept");
}

struct SchemaStruct {
  rfl::Description<"Download directory",
                   rfl::Alias<"download-dir", std::string>>
      download_dir;
  rfl::Alias<"old-port", rfl::DefaultVal<int>> port = 8080;
  rfl::Alias<"old-limit", rfl::Validator<int, rfl::Minimum<0>>> limit = 10;
};

TEST(json, test_alias_json_schema_canonical_only) {
  const auto schema = rfl::json::to_schema<SchemaStruct>();
  // Primary property names are emitted; legacy alias names are NOT emitted.
  EXPECT_NE(schema.find(R"("download_dir")"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("port")"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("limit")"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("Download directory")"), std::string::npos)
      << schema;
  EXPECT_NE(schema.find(R"("default":8080)"), std::string::npos) << schema;
  EXPECT_EQ(schema.find(R"("download-dir")"), std::string::npos) << schema;
  EXPECT_EQ(schema.find(R"("old-port")"), std::string::npos) << schema;
  EXPECT_EQ(schema.find(R"("old-limit")"), std::string::npos) << schema;
}

struct NonDefaultType {
  explicit NonDefaultType(int _v) : v(_v) {}
  int v;
};

struct NonDefaultWrapperHolder {
  rfl::Alias<"old_x", NonDefaultType> x;
};

TEST(json, test_alias_ergonomics_and_traits) {
  static_assert(std::is_default_constructible_v<rfl::Alias<"a", int>>);
  static_assert(
      !std::is_default_constructible_v<rfl::Alias<"a", NonDefaultType>>);

  rfl::Alias<"old", std::string> a = "hello";
  EXPECT_EQ(a.value(), "hello");
  EXPECT_EQ(a.get(), "hello");
  EXPECT_EQ(*a, "hello");
  EXPECT_EQ(a(), "hello");
  EXPECT_EQ(a->size(), 5u);

  // Implicit conversion to underlying type.
  const std::string copy = a;
  EXPECT_EQ(copy, "hello");

  // Comparison operators (==, !=, <=>).
  rfl::Alias<"old", std::string> b = "world";
  EXPECT_TRUE(a == "hello");
  EXPECT_TRUE("hello" == a);
  EXPECT_TRUE(a != b);
  EXPECT_TRUE(a < b);
  EXPECT_TRUE((a <=> b) < 0);

  // Chained alias ergonomics.
  rfl::Alias<"kebab-case", rfl::Alias<"camelCase", std::string>> chained =
      "value";
  EXPECT_EQ(chained, "value");

  // std::hash specialization.
  std::unordered_set<rfl::Alias<"old", std::string>> set;
  set.insert(a);
  set.insert(b);
  EXPECT_EQ(set.size(), 2u);
  EXPECT_TRUE(set.contains(rfl::Alias<"old", std::string>("hello")));
}

struct WithSkipAndAlias {
  int visible = 1;
  rfl::Alias<"old_skip", rfl::Skip<int>> skipped_inner = 99;
  rfl::Skip<rfl::Alias<"old_skip2", int>> skipped_outer = 100;
};

struct WithValidatorAndPointerWrappers {
  rfl::Validator<rfl::Alias<"old_v", int>, rfl::Minimum<1>> val;
  rfl::Box<rfl::Alias<"old_box", int>> boxed;
  rfl::Ref<rfl::Alias<"old_ref", int>> refed;
  std::unique_ptr<rfl::Alias<"old_uptr", int>> uptr;
  std::shared_ptr<rfl::Alias<"old_sptr", int>> sptr;
};

TEST(json, test_alias_with_skip_validator_and_pointer_wrappers) {
  // Alias wrapping Skip must NOT leak into JSON output or JSON Schema.
  const WithSkipAndAlias ws;
  EXPECT_EQ(rfl::json::write(ws), R"({"visible":1})");
  const auto schema = rfl::json::to_schema<WithSkipAndAlias>();
  EXPECT_NE(schema.find(R"("visible")"), std::string::npos) << schema;
  EXPECT_EQ(schema.find(R"("skipped_inner")"), std::string::npos) << schema;
  EXPECT_EQ(schema.find(R"("skipped_outer")"), std::string::npos) << schema;
  EXPECT_EQ(schema.find(R"("old_skip")"), std::string::npos) << schema;

  // Aliases inside Validator, Box, Ref, unique_ptr, shared_ptr must work.
  const auto parsed =
      rfl::json::read<WithValidatorAndPointerWrappers, rfl::NoExtraFields>(
          R"({"old_v":5,"old_box":10,"old_ref":20,"old_uptr":30,"old_sptr":40})");
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed->val.value().value(), 5);
  EXPECT_EQ(parsed->boxed->value(), 10);
  EXPECT_EQ(parsed->refed->value(), 20);
  ASSERT_NE(parsed->uptr, nullptr);
  EXPECT_EQ(parsed->uptr->value(), 30);
  ASSERT_NE(parsed->sptr, nullptr);
  EXPECT_EQ(parsed->sptr->value(), 40);

  // Validator constraint still enforced when supplied via inner alias.
  EXPECT_FALSE(
      (rfl::json::read<WithValidatorAndPointerWrappers>(
           R"({"old_v":0,"old_box":10,"old_ref":20,"old_uptr":30,"old_sptr":40})"))
          .has_value());
}

}  // namespace test_alias
