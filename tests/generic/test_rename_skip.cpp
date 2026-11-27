#include <gtest/gtest.h>

#include <rfl.hpp>
#include <string>
#include <type_traits>
#include <vector>

namespace test_generic_rename_skip {

using rfl::DefaultVal;
using rfl::Description;
using rfl::Field;
using rfl::Rename;
using rfl::Skip;
using rfl::SkipDeserialization;
using rfl::SkipSerialization;
using rfl::Tuple;
using rfl::internal::find_index;
using rfl::internal::no_duplicate_field_names;

// --- Compile-time duplicate wire-name checks ---

// Genuine duplicates MUST be rejected:
static_assert(!no_duplicate_field_names<
              Tuple<Field<"items", int>, Field<"items", int>>>());
static_assert(!no_duplicate_field_names<
              Tuple<Field<"items", int*>,
                    Field<"items", Rename<"items", int>*>>>());

// Two fields that are BOTH serialized MUST be rejected:
static_assert(!no_duplicate_field_names<
              Tuple<Field<"items", SkipDeserialization<int>>,
                    Field<"items", Rename<"items", SkipDeserialization<int>>>>>());
static_assert(!no_duplicate_field_names<
              Tuple<Field<"items", int>,
                    Field<"items", Rename<"items", SkipDeserialization<int>>>>>());
static_assert(!no_duplicate_field_names<
              Tuple<Field<"items", SkipDeserialization<int>>,
                    Field<"items", Rename<"items", int>>>>());
static_assert(
    !no_duplicate_field_names<
        Tuple<Field<"items", Description<"a", int>>,
              Field<"items",
                    Rename<"items", Description<"b", SkipDeserialization<int>>>>>>());

// Two fields that are BOTH deserialized MUST be rejected:
static_assert(!no_duplicate_field_names<
              Tuple<Field<"items", SkipSerialization<int>>,
                    Field<"items", Rename<"items", SkipSerialization<int>>>>>());
static_assert(!no_duplicate_field_names<
              Tuple<Field<"items", int>,
                    Field<"items", Rename<"items", SkipSerialization<int>>>>>());
static_assert(!no_duplicate_field_names<
              Tuple<Field<"items", SkipSerialization<int>>,
                    Field<"items", Rename<"items", int>>>>());
static_assert(
    !no_duplicate_field_names<
        Tuple<Field<"items", DefaultVal<SkipSerialization<int>>>,
              Field<"items", Rename<"items", SkipSerialization<int>>>>>());

// Literal duplicate detection MUST still work:
static_assert(rfl::Literal<"a", "a">::has_duplicates());
static_assert(!rfl::Literal<"a", "b">::has_duplicates());

// Non-conflicting Skip / SkipSerialization / SkipDeserialization pairs MUST be
// allowed:
static_assert(no_duplicate_field_names<
              Tuple<Field<"items", Skip<std::vector<int>>>,
                    Field<"items", Rename<"items", std::vector<int>>>>>());
static_assert(no_duplicate_field_names<
              Tuple<Field<"items", Rename<"items", std::vector<int>>>,
                    Field<"items", Skip<std::vector<int>>>>>());
static_assert(no_duplicate_field_names<
              Tuple<Field<"items", Skip<std::string>*>,
                    Field<"items", Rename<"items", std::vector<int>>*>>>());
static_assert(
    no_duplicate_field_names<
        Tuple<Field<"items", SkipSerialization<std::string>>,
              Field<"items", Rename<"items", SkipDeserialization<int>>>>>());
static_assert(
    no_duplicate_field_names<
        Tuple<Field<"items", SkipDeserialization<std::string>>,
              Field<"items", Rename<"items", SkipSerialization<int>>>>>());
static_assert(
    no_duplicate_field_names<
        Tuple<Field<"items", SkipSerialization<int>>,
              Field<"items", SkipDeserialization<Rename<"items", std::string>>>>>());
static_assert(
    no_duplicate_field_names<
        Tuple<Field<"items", Description<"d", Skip<int>>>,
              Field<"items", Rename<"items", DefaultVal<int>>>>>());
static_assert(
    no_duplicate_field_names<
        Tuple<Field<"items", Description<"d", SkipSerialization<int>>>,
              Field<"items",
                    Rename<"items",
                           Description<"e", SkipDeserialization<std::string>>>>>>());
static_assert(no_duplicate_field_names<
              Tuple<Field<"items", Skip<int>>, Field<"items", Skip<std::string>>>>());
static_assert(no_duplicate_field_names<
              Tuple<Field<"items", Skip<int>>,
                    Field<"items", SkipSerialization<std::string>>>>());
static_assert(no_duplicate_field_names<
              Tuple<Field<"items", Skip<int>>,
                    Field<"items", SkipDeserialization<std::string>>>>());

// find_index must pick the live field over a fully skipped field sharing the
// same name, and still find a unique skipped field.
static_assert(
    find_index<"items",
               Tuple<Field<"items", Skip<std::string>>,
                     Field<"items", Rename<"items", std::vector<int>>>>>() == 1);
static_assert(
    find_index<"items",
               Tuple<Field<"items", Rename<"items", std::vector<int>>>,
                     Field<"items", Skip<std::string>>>>() == 0);
static_assert(
    find_index<"town",
               Tuple<Field<"name", std::string>, Field<"town", Skip<std::string>>>>() ==
    1);

struct SkipAndRename {
  rfl::Skip<std::vector<int>> items;
  rfl::Rename<"items", std::vector<int>> serialized_items;
};

struct SplitSkipSerFirst {
  rfl::SkipSerialization<std::vector<int>> items;
  rfl::Rename<"items", rfl::SkipDeserialization<std::vector<int>>>
      serialized_items;
};

struct SplitSkipDeserFirst {
  rfl::SkipDeserialization<std::vector<int>> items;
  rfl::Rename<"items", rfl::SkipSerialization<std::vector<int>>>
      deserialized_items;
};

TEST(generic, test_rename_skip_roundtrip) {
  auto s = SkipAndRename{.items = std::vector<int>{99},
                         .serialized_items = std::vector<int>{1, 2, 3}};

  const auto gen = rfl::to_generic(s);
  const auto obj = gen.to_object().value();
  EXPECT_EQ(obj.size(), 1u);

  const auto parsed = rfl::from_generic<SkipAndRename>(gen).value();
  EXPECT_TRUE(parsed.items.value().empty());
  EXPECT_EQ(parsed.serialized_items.value(), (std::vector<int>{1, 2, 3}));

  auto view = rfl::to_view(s);
  EXPECT_EQ(view.get<"items">()->value(), (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(view.get<0>()->value(), (std::vector<int>{99}));

  const auto nt = rfl::to_named_tuple(s);
  EXPECT_EQ(nt.get<"items">().value(), (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(nt.get<0>().value(), (std::vector<int>{99}));

  EXPECT_EQ(rfl::fields<SkipAndRename>().size(), 2u);
}

TEST(generic, test_rename_skip_serialization_deserialization) {
  const auto s1 = SplitSkipSerFirst{
      .items = std::vector<int>{1, 2},
      .serialized_items = std::vector<int>{3, 4}};
  const auto gen1 = rfl::to_generic(s1);
  const auto obj1 = gen1.to_object().value();
  EXPECT_EQ(obj1.size(), 1u);

  const auto parsed1 = rfl::from_generic<SplitSkipSerFirst>(gen1).value();
  EXPECT_EQ(parsed1.items.value(), (std::vector<int>{3, 4}));
  EXPECT_TRUE(parsed1.serialized_items.value().value().empty());

  const auto s2 = SplitSkipDeserFirst{
      .items = std::vector<int>{5, 6},
      .deserialized_items = std::vector<int>{7, 8}};
  const auto gen2 = rfl::to_generic(s2);
  const auto obj2 = gen2.to_object().value();
  EXPECT_EQ(obj2.size(), 1u);

  const auto parsed2 = rfl::from_generic<SplitSkipDeserFirst>(gen2).value();
  EXPECT_TRUE(parsed2.items.value().empty());
  EXPECT_EQ(parsed2.deserialized_items.value().value(),
            (std::vector<int>{5, 6}));
}

}  // namespace test_generic_rename_skip
