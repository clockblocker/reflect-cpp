#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

namespace test_json_default_tag {

struct ConfigV1 {
  using Tag = rfl::Literal<"1">;
  std::string name;
  int retries = 3;
};

struct ConfigV2 {
  using Tag = rfl::Literal<"2">;
  std::string name;
  bool strict;
};

using VersionedConfig = rfl::TaggedUnion<"version", ConfigV1, ConfigV2>;

struct EmptyAlt {
  using Tag = rfl::Literal<"empty">;
};

struct DataAlt {
  using Tag = rfl::Literal<"data">;
  int value;
};

using EmptyUnion = rfl::TaggedUnion<"kind", EmptyAlt, DataAlt>;

struct OuterWithBoth {
  VersionedConfig cfg;
  EmptyUnion payload;
};

struct FlattenedEnvelope {
  std::string id;
  rfl::Flatten<VersionedConfig> cfg;
};

TEST(json, test_default_tag_missing_discriminator_falls_back) {
  // Without DefaultTag, missing discriminator fails.
  const auto without_proc =
      rfl::json::read<VersionedConfig>(R"({"name":"legacy","retries":5})");
  ASSERT_FALSE(without_proc);

  // With DefaultTag<"version", "1">, missing discriminator selects ConfigV1.
  const auto res =
      rfl::json::read<VersionedConfig, rfl::NoExtraFields,
                      rfl::DefaultTag<"version", "1">>(
          R"({"name":"legacy","retries":5})");
  ASSERT_TRUE(res) << res.error().what();
  const auto* v1 = rfl::get_if<ConfigV1>(&res->variant());
  ASSERT_NE(v1, nullptr);
  EXPECT_EQ(v1->name, "legacy");
  EXPECT_EQ(v1->retries, 5);

  // Writing still emits the explicit discriminator.
  const auto written =
      rfl::json::write<rfl::DefaultTag<"version", "1">>(*res);
  EXPECT_EQ(written, R"({"version":"1","name":"legacy","retries":5})");
}

TEST(json, test_default_tag_explicit_invalid_and_wrong_type_still_checked) {
  using Procs =
      rfl::Processors<rfl::NoExtraFields, rfl::DefaultTag<"version", "1">>;

  // Explicit "2" dispatches to ConfigV2.
  const auto v2_res = rfl::json::read<VersionedConfig, Procs>(
      R"({"version":"2","name":"modern","strict":true})");
  ASSERT_TRUE(v2_res) << v2_res.error().what();
  const auto* v2 = rfl::get_if<ConfigV2>(&v2_res->variant());
  ASSERT_NE(v2, nullptr);
  EXPECT_EQ(v2->name, "modern");
  EXPECT_TRUE(v2->strict);

  // Explicit "1" dispatches to ConfigV1.
  const auto v1_res = rfl::json::read<VersionedConfig, Procs>(
      R"({"version":"1","name":"explicit","retries":1})");
  ASSERT_TRUE(v1_res) << v1_res.error().what();
  ASSERT_NE(rfl::get_if<ConfigV1>(&v1_res->variant()), nullptr);

  // Unknown explicit tag MUST still fail.
  const auto bad_tag = rfl::json::read<VersionedConfig, Procs>(
      R"({"version":"99","name":"x","retries":1})");
  ASSERT_FALSE(bad_tag);
  EXPECT_EQ(bad_tag.error().details()[0].kind, rfl::ErrorKind::invalid_value);

  // Present discriminator with non-string type MUST still fail (not default).
  const auto wrong_type = rfl::json::read<VersionedConfig, Procs>(
      R"({"version":1,"name":"x","retries":1})");
  ASSERT_FALSE(wrong_type);
  EXPECT_EQ(wrong_type.error().details()[0].kind, rfl::ErrorKind::invalid_type);

  // Present discriminator with null MUST still fail.
  const auto null_tag = rfl::json::read<VersionedConfig, Procs>(
      R"({"version":null,"name":"x","retries":1})");
  ASSERT_FALSE(null_tag);

  // Extra fields are still rejected when discriminator is defaulted.
  const auto extra_field = rfl::json::read<VersionedConfig, Procs>(
      R"({"name":"x","retries":1,"unknown":true})");
  ASSERT_FALSE(extra_field);
  EXPECT_EQ(extra_field.error().details()[0].kind,
            rfl::ErrorKind::unknown_field);
}

TEST(json, test_default_tag_multiple_unions_empty_alt_and_flattened) {
  using Combined =
      rfl::Processors<rfl::NoExtraFields, rfl::DefaultTag<"version", "1">,
                      rfl::DefaultTag<"kind", "empty">>;

  const auto outer = rfl::json::read<OuterWithBoth, Combined>(
      R"({"cfg":{"name":"a","retries":2},"payload":{}})");
  ASSERT_TRUE(outer) << outer.error().what();
  ASSERT_NE(rfl::get_if<ConfigV1>(&outer->cfg.variant()), nullptr);
  ASSERT_NE(rfl::get_if<EmptyAlt>(&outer->payload.variant()), nullptr);

  // Empty alt with extra field rejected under NoExtraFields.
  const auto bad_empty =
      rfl::json::read<EmptyUnion, rfl::NoExtraFields,
                      rfl::DefaultTag<"kind", "empty">>(R"({"bogus":1})");
  ASSERT_FALSE(bad_empty);

  // Flattened TaggedUnion with missing discriminator falls back cleanly.
  const auto flat = rfl::json::read<FlattenedEnvelope, rfl::NoExtraFields,
                                    rfl::DefaultTag<"version", "1">>(
      R"({"id":"env-1","name":"flat-legacy","retries":7})");
  ASSERT_TRUE(flat) << flat.error().what();
  EXPECT_EQ(flat->id, "env-1");
  const auto* flat_v1 = rfl::get_if<ConfigV1>(&flat->cfg.get().variant());
  ASSERT_NE(flat_v1, nullptr);
  EXPECT_EQ(flat_v1->name, "flat-legacy");
  EXPECT_EQ(flat_v1->retries, 7);

  // Flattened TaggedUnion with explicit "2" dispatches to ConfigV2.
  const auto flat_v2 = rfl::json::read<FlattenedEnvelope, rfl::NoExtraFields,
                                       rfl::DefaultTag<"version", "1">>(
      R"({"id":"env-2","version":"2","name":"flat-v2","strict":false})");
  ASSERT_TRUE(flat_v2) << flat_v2.error().what();
  const auto* fv2 = rfl::get_if<ConfigV2>(&flat_v2->cfg.get().variant());
  ASSERT_NE(fv2, nullptr);
  EXPECT_FALSE(fv2->strict);
}

}  // namespace test_json_default_tag
