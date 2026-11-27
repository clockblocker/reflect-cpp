#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <type_traits>
#include <vector>

#include "write_and_read.hpp"

namespace test_rename_skip {

struct SkipAndRename {
  rfl::Skip<std::vector<int>> items;
  rfl::Rename<"items", std::vector<int>> serialized_items;
};

struct RenameAndSkip {
  rfl::Rename<"items", std::vector<int>> serialized_items;
  rfl::Skip<std::vector<int>> items;
};

struct SkipDifferentTypeAndRename {
  rfl::Skip<std::string> items;
  rfl::Rename<"items", std::vector<int>> serialized_items;
};

struct SkipSerAndRenameSkipDeser {
  rfl::SkipSerialization<std::vector<int>> items;
  rfl::Rename<"items", rfl::SkipDeserialization<std::vector<int>>>
      serialized_items;
};

struct SkipDeserAndRenameSkipSer {
  rfl::SkipDeserialization<std::vector<int>> items;
  rfl::Rename<"items", rfl::SkipSerialization<std::vector<int>>>
      deserialized_items;
};

struct SkipSerAndInnerRenameSkipDeser {
  rfl::SkipSerialization<int> items;
  rfl::SkipDeserialization<rfl::Rename<"items", std::string>> serialized_items;
};

struct WrappedDescription {
  rfl::Description<"internal cache", rfl::Skip<std::string>> items;
  rfl::Rename<"items", rfl::Description<"public items", std::vector<int>>>
      serialized_items;
};

struct WrappedDefaultVal {
  rfl::Skip<std::vector<int>> items = std::vector<int>{99};
  rfl::Rename<"items", rfl::DefaultVal<std::vector<int>>> serialized_items =
      std::vector<int>{1, 2};
};

struct WithInClassDefaultsSplit {
  rfl::SkipSerialization<std::vector<int>> items = std::vector<int>{10};
  rfl::Rename<"items", rfl::SkipDeserialization<std::vector<int>>>
      serialized_items = std::vector<int>{20};
};

struct InnerRenamed {
  rfl::Rename<"items", std::vector<int>> serialized_items;
};

struct FlattenedSkipAndRename {
  rfl::Skip<std::string> items;
  rfl::Flatten<InnerRenamed> inner;
};

TEST(json, test_rename_skip_basic) {
  const auto s = SkipAndRename{.items = std::vector<int>{99},
                               .serialized_items = std::vector<int>{1, 2, 3}};

  const auto json_str = rfl::json::write(s);
  EXPECT_EQ(json_str, R"({"items":[1,2,3]})");

  const auto parsed = rfl::json::read<SkipAndRename>(json_str).value();
  EXPECT_TRUE(parsed.items.value().empty());
  EXPECT_EQ(parsed.serialized_items.value(), (std::vector<int>{1, 2, 3}));

  // Also verify with NoExtraFields, DefaultIfMissing, and NoOptionals.
  const auto parsed_strict =
      rfl::json::read<SkipAndRename, rfl::NoExtraFields>(json_str).value();
  EXPECT_TRUE(parsed_strict.items.value().empty());
  EXPECT_EQ(parsed_strict.serialized_items.value(),
            (std::vector<int>{1, 2, 3}));

  const auto parsed_default =
      rfl::json::read<SkipAndRename, rfl::DefaultIfMissing>(json_str).value();
  EXPECT_TRUE(parsed_default.items.value().empty());
  EXPECT_EQ(parsed_default.serialized_items.value(),
            (std::vector<int>{1, 2, 3}));

  const auto json_no_opt = rfl::json::write<rfl::NoOptionals>(s);
  EXPECT_EQ(json_no_opt, R"({"items":[1,2,3]})");
  const auto parsed_no_opt =
      rfl::json::read<SkipAndRename, rfl::NoOptionals>(json_no_opt).value();
  EXPECT_TRUE(parsed_no_opt.items.value().empty());
  EXPECT_EQ(parsed_no_opt.serialized_items.value(),
            (std::vector<int>{1, 2, 3}));

  // Missing required renamed field must still fail.
  const auto missing = rfl::json::read<SkipAndRename>(R"({})");
  EXPECT_FALSE(missing);
}

TEST(json, test_rename_skip_reversed_order) {
  const auto s = RenameAndSkip{.serialized_items = std::vector<int>{4, 5},
                               .items = std::vector<int>{88}};

  const auto json_str = rfl::json::write(s);
  EXPECT_EQ(json_str, R"({"items":[4,5]})");

  const auto parsed = rfl::json::read<RenameAndSkip>(json_str).value();
  EXPECT_EQ(parsed.serialized_items.value(), (std::vector<int>{4, 5}));
  EXPECT_TRUE(parsed.items.value().empty());
}

TEST(json, test_rename_skip_view_named_tuple_fields_schema) {
  auto s = SkipDifferentTypeAndRename{
      .items = "internal", .serialized_items = std::vector<int>{7, 8}};

  auto view = rfl::to_view(s);
  static_assert(
      std::is_same_v<std::remove_cvref_t<decltype(*view.get<"items">())>,
                     rfl::Rename<"items", std::vector<int>>>);
  EXPECT_EQ(view.get<"items">()->value(), (std::vector<int>{7, 8}));
  EXPECT_EQ(view.get<0>()->value(), "internal");

  const auto nt = rfl::to_named_tuple(SkipDifferentTypeAndRename(s));
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(nt.get<"items">())>,
                               rfl::Rename<"items", std::vector<int>>>);
  EXPECT_EQ(nt.get<"items">().value(), (std::vector<int>{7, 8}));
  EXPECT_EQ(nt.get<0>().value(), "internal");

  const auto meta = rfl::fields<SkipDifferentTypeAndRename>();
  EXPECT_EQ(meta.size(), 2u);

  const auto schema = rfl::json::to_schema<SkipDifferentTypeAndRename>();
  EXPECT_EQ(
      schema,
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_rename_skip__SkipDifferentTypeAndRename","$defs":{"test_rename_skip__SkipDifferentTypeAndRename":{"type":"object","properties":{"items":{"type":"array","items":{"type":"integer"}}},"required":["items"]}}})");

  const auto generic = rfl::to_generic(s);
  const auto from_gen =
      rfl::from_generic<SkipDifferentTypeAndRename>(generic).value();
  EXPECT_TRUE(from_gen.items.value().empty());
  EXPECT_EQ(from_gen.serialized_items.value(), (std::vector<int>{7, 8}));
}

TEST(json, test_rename_skip_serialization_and_deserialization) {
  const auto s1 = SkipSerAndRenameSkipDeser{
      .items = std::vector<int>{1, 2},
      .serialized_items = std::vector<int>{3, 4}};

  // Write uses serialized_items (SkipDeserialization) and skips items
  // (SkipSerialization).
  EXPECT_EQ(rfl::json::write(s1), R"({"items":[3,4]})");

  // Read populates items (SkipSerialization) and skips serialized_items
  // (SkipDeserialization).
  const auto parsed1 =
      rfl::json::read<SkipSerAndRenameSkipDeser>(R"({"items":[5,6]})").value();
  EXPECT_EQ(parsed1.items.value(), (std::vector<int>{5, 6}));
  EXPECT_TRUE(parsed1.serialized_items.value().value().empty());

  // Schema includes the serialized field once.
  const auto schema1 = rfl::json::to_schema<SkipSerAndRenameSkipDeser>();
  EXPECT_EQ(
      schema1,
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_rename_skip__SkipSerAndRenameSkipDeser","$defs":{"test_rename_skip__SkipSerAndRenameSkipDeser":{"type":"object","properties":{"items":{"type":"array","items":{"type":"integer"}}},"required":["items"]}}})");

  // Reverse order: SkipDeserialization declared BEFORE Rename<SkipSerialization>.
  const auto s2 = SkipDeserAndRenameSkipSer{
      .items = std::vector<int>{7, 8},
      .deserialized_items = std::vector<int>{9, 10}};
  EXPECT_EQ(rfl::json::write(s2), R"({"items":[7,8]})");

  const auto parsed2 =
      rfl::json::read<SkipDeserAndRenameSkipSer>(R"({"items":[11,12]})")
          .value();
  EXPECT_TRUE(parsed2.items.value().empty());
  EXPECT_EQ(parsed2.deserialized_items.value().value(),
            (std::vector<int>{11, 12}));

  const auto schema2 = rfl::json::to_schema<SkipDeserAndRenameSkipSer>();
  EXPECT_EQ(
      schema2,
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_rename_skip__SkipDeserAndRenameSkipSer","$defs":{"test_rename_skip__SkipDeserAndRenameSkipSer":{"type":"object","properties":{"items":{"type":"array","items":{"type":"integer"}}},"required":["items"]}}})");

  // Inner Rename inside SkipDeserialization with different wire types.
  const auto s3 = SkipSerAndInnerRenameSkipDeser{.items = 42,
                                                 .serialized_items = "hello"};
  EXPECT_EQ(rfl::json::write(s3), R"({"items":"hello"})");
  const auto parsed3 =
      rfl::json::read<SkipSerAndInnerRenameSkipDeser>(R"({"items":123})")
          .value();
  EXPECT_EQ(parsed3.items.value(), 123);
  EXPECT_TRUE(parsed3.serialized_items.value().value().empty());

  // DefaultIfMissing preserves the in-class default of the skipped field.
  const auto parsed_def =
      rfl::json::read<WithInClassDefaultsSplit, rfl::DefaultIfMissing>(
          R"({"items":[1,2,3]})")
          .value();
  EXPECT_EQ(parsed_def.items.value(), (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(parsed_def.serialized_items.value().value(),
            (std::vector<int>{20}));
}

TEST(json, test_rename_skip_with_description_and_default_val) {
  const auto desc_s = WrappedDescription{
      .items = "secret", .serialized_items = std::vector<int>{1, 2}};
  EXPECT_EQ(rfl::json::write(desc_s), R"({"items":[1,2]})");

  const auto parsed_desc =
      rfl::json::read<WrappedDescription>(R"({"items":[3,4]})").value();
  EXPECT_TRUE(parsed_desc.items.value().value().empty());
  EXPECT_EQ(parsed_desc.serialized_items.value().value(),
            (std::vector<int>{3, 4}));

  const auto desc_schema = rfl::json::to_schema<WrappedDescription>();
  EXPECT_EQ(
      desc_schema,
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_rename_skip__WrappedDescription","$defs":{"test_rename_skip__WrappedDescription":{"type":"object","properties":{"items":{"type":"array","description":"public items","items":{"type":"integer"}}},"required":["items"]}}})");

  // DefaultVal + Skip
  const auto parsed_empty =
      rfl::json::read<WrappedDefaultVal>(R"({})").value();
  EXPECT_EQ(parsed_empty.items.value(), (std::vector<int>{99}));
  EXPECT_EQ(parsed_empty.serialized_items.value().value(),
            (std::vector<int>{1, 2}));

  const auto parsed_filled =
      rfl::json::read<WrappedDefaultVal>(R"({"items":[5,6]})").value();
  EXPECT_EQ(parsed_filled.items.value(), (std::vector<int>{99}));
  EXPECT_EQ(parsed_filled.serialized_items.value().value(),
            (std::vector<int>{5, 6}));
  EXPECT_EQ(rfl::json::write(parsed_filled), R"({"items":[5,6]})");

  const auto default_schema = rfl::json::to_schema<WrappedDefaultVal>();
  EXPECT_EQ(
      default_schema,
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_rename_skip__WrappedDefaultVal","$defs":{"test_rename_skip__WrappedDefaultVal":{"type":"object","properties":{"items":{"type":"array","default":[1,2],"items":{"type":"integer"}}},"required":[]}}})");
}

TEST(json, test_rename_skip_flattened) {
  auto s = FlattenedSkipAndRename{
      .items = "hidden",
      .inner = InnerRenamed{.serialized_items = std::vector<int>{10, 20}}};

  EXPECT_EQ(rfl::json::write(s), R"({"items":[10,20]})");
  const auto parsed =
      rfl::json::read<FlattenedSkipAndRename>(R"({"items":[30,40]})").value();
  EXPECT_TRUE(parsed.items.value().empty());
  EXPECT_EQ(parsed.inner.value().serialized_items.value(),
            (std::vector<int>{30, 40}));

  auto view = rfl::to_view(s);
  EXPECT_EQ(view.get<"items">()->value(), (std::vector<int>{10, 20}));
}

}  // namespace test_rename_skip
