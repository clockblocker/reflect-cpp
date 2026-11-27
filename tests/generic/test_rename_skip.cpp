#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <rfl.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace test_generic_rename_skip {

struct StrictCountingWriter {
  struct ObjectRecord {
    size_t expected_size = 0;
    size_t actual_count = 0;
    std::vector<std::string> keys;
  };

  struct ArrayRecord {
    size_t expected_size = 0;
    size_t actual_count = 0;
  };

  struct OutputObjectType {
    size_t index = 0;
  };

  struct OutputArrayType {
    size_t index = 0;
  };

  struct OutputVarType {};

  mutable std::vector<ObjectRecord> objects;
  mutable std::vector<ArrayRecord> arrays;

  OutputArrayType array_as_root(const size_t _size) const {
    arrays.push_back(ArrayRecord{_size, 0});
    return OutputArrayType{arrays.size() - 1};
  }

  OutputObjectType object_as_root(const size_t _size) const {
    objects.push_back(ObjectRecord{_size, 0, {}});
    return OutputObjectType{objects.size() - 1};
  }

  OutputVarType null_as_root() const { return {}; }

  template <class T>
  OutputVarType value_as_root(const T&) const {
    return {};
  }

  OutputArrayType add_array_to_array(const size_t _size,
                                     OutputArrayType* _parent) const {
    ++arrays[_parent->index].actual_count;
    arrays.push_back(ArrayRecord{_size, 0});
    return OutputArrayType{arrays.size() - 1};
  }

  OutputArrayType add_array_to_object(const std::string_view& _name,
                                      const size_t _size,
                                      OutputObjectType* _parent) const {
    auto& rec = objects[_parent->index];
    ++rec.actual_count;
    rec.keys.emplace_back(_name);
    arrays.push_back(ArrayRecord{_size, 0});
    return OutputArrayType{arrays.size() - 1};
  }

  OutputObjectType add_object_to_array(const size_t _size,
                                       OutputArrayType* _parent) const {
    ++arrays[_parent->index].actual_count;
    objects.push_back(ObjectRecord{_size, 0, {}});
    return OutputObjectType{objects.size() - 1};
  }

  OutputObjectType add_object_to_object(const std::string_view& _name,
                                        const size_t _size,
                                        OutputObjectType* _parent) const {
    auto& rec = objects[_parent->index];
    ++rec.actual_count;
    rec.keys.emplace_back(_name);
    objects.push_back(ObjectRecord{_size, 0, {}});
    return OutputObjectType{objects.size() - 1};
  }

  template <class T>
  OutputVarType add_value_to_array(const T&, OutputArrayType* _parent) const {
    ++arrays[_parent->index].actual_count;
    return {};
  }

  template <class T>
  OutputVarType add_value_to_object(const std::string_view& _name, const T&,
                                    OutputObjectType* _parent) const {
    auto& rec = objects[_parent->index];
    ++rec.actual_count;
    rec.keys.emplace_back(_name);
    return {};
  }

  OutputVarType add_null_to_array(OutputArrayType* _parent) const {
    ++arrays[_parent->index].actual_count;
    return {};
  }

  OutputVarType add_null_to_object(const std::string_view& _name,
                                   OutputObjectType* _parent) const {
    auto& rec = objects[_parent->index];
    ++rec.actual_count;
    rec.keys.emplace_back(_name);
    return {};
  }

  void end_array(OutputArrayType*) const noexcept {}
  void end_object(OutputObjectType*) const noexcept {}
};

}  // namespace test_generic_rename_skip

namespace rfl::parsing {

// MessagePack-style specialization where _all_required is true and the writer
// relies on add_object receiving the exact number of serialized entries.
template <class ProcessorsType, class... FieldTypes>
  requires AreReaderAndWriter<generic::Reader,
                              test_generic_rename_skip::StrictCountingWriter,
                              NamedTuple<FieldTypes...>>
struct Parser<generic::Reader, test_generic_rename_skip::StrictCountingWriter,
              NamedTuple<FieldTypes...>, ProcessorsType>
    : public NamedTupleParser<
          generic::Reader, test_generic_rename_skip::StrictCountingWriter,
          /*_ignore_empty_containers=*/false,
          /*_all_required=*/true,
          /*_no_field_names=*/internal::no_field_names_v<ProcessorsType>,
          ProcessorsType, FieldTypes...> {};

}  // namespace rfl::parsing

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

struct RegularWithOptional {
  int a;
  std::optional<std::string> b;
};

struct SkipDifferentTypes {
  rfl::Skip<std::string> items;
  rfl::Rename<"items", std::vector<int>> serialized_items;
};

struct SkipWithExtraFields {
  rfl::Skip<std::string> items;
  rfl::Rename<"items", int> serialized_items;
  rfl::ExtraFields<int> extra;
};

struct AllSkipped {
  rfl::Skip<int> a;
  rfl::SkipSerialization<std::string> b;
};

struct NestedSkipAndRename {
  rfl::Skip<int> inner;
  rfl::Rename<"inner", SkipDifferentTypes> child;
};

template <class... Ps, class T>
StrictCountingWriter write_strict(const T& _obj) {
  StrictCountingWriter writer;
  using ParentType = rfl::parsing::Parent<StrictCountingWriter>;
  rfl::parsing::Parser<rfl::generic::Reader, StrictCountingWriter, T,
                       rfl::Processors<Ps...>>::write(
      writer, _obj, typename ParentType::Root{});
  return writer;
}

TEST(generic, test_rename_skip_strict_object_count) {
  // Regular struct with empty optional under _all_required = true (msgpack-style)
  {
    const auto w = write_strict(RegularWithOptional{.a = 1, .b = std::nullopt});
    ASSERT_EQ(w.objects.size(), 1u);
    EXPECT_EQ(w.objects[0].expected_size, 2u);
    EXPECT_EQ(w.objects[0].actual_count, 2u);
    EXPECT_EQ(w.objects[0].keys, (std::vector<std::string>{"a", "b"}));
  }

  // Skip + Rename sharing wire name
  {
    const auto w = write_strict(SkipAndRename{
        .items = std::vector<int>{99},
        .serialized_items = std::vector<int>{1, 2, 3}});
    ASSERT_EQ(w.objects.size(), 1u);
    EXPECT_EQ(w.objects[0].expected_size, 1u);
    EXPECT_EQ(w.objects[0].actual_count, 1u);
    EXPECT_EQ(w.objects[0].keys, (std::vector<std::string>{"items"}));
    ASSERT_EQ(w.arrays.size(), 1u);
    EXPECT_EQ(w.arrays[0].expected_size, 3u);
    EXPECT_EQ(w.arrays[0].actual_count, 3u);
  }

  // Split SkipSerialization + SkipDeserialization (both orders)
  {
    const auto w1 = write_strict(SplitSkipSerFirst{
        .items = std::vector<int>{1},
        .serialized_items = std::vector<int>{2, 3}});
    ASSERT_EQ(w1.objects.size(), 1u);
    EXPECT_EQ(w1.objects[0].expected_size, 1u);
    EXPECT_EQ(w1.objects[0].actual_count, 1u);
    EXPECT_EQ(w1.objects[0].keys, (std::vector<std::string>{"items"}));

    const auto w2 = write_strict(SplitSkipDeserFirst{
        .items = std::vector<int>{4, 5},
        .deserialized_items = std::vector<int>{6}});
    ASSERT_EQ(w2.objects.size(), 1u);
    EXPECT_EQ(w2.objects[0].expected_size, 1u);
    EXPECT_EQ(w2.objects[0].actual_count, 1u);
    EXPECT_EQ(w2.objects[0].keys, (std::vector<std::string>{"items"}));
  }

  // Skip + Rename + ExtraFields
  {
    auto s = SkipWithExtraFields{.items = "hidden", .serialized_items = 42};
    s.extra["x"] = 10;
    s.extra["y"] = 20;
    const auto w = write_strict(s);
    ASSERT_EQ(w.objects.size(), 1u);
    EXPECT_EQ(w.objects[0].expected_size, 3u);
    EXPECT_EQ(w.objects[0].actual_count, 3u);
    EXPECT_EQ(w.objects[0].keys, (std::vector<std::string>{"items", "x", "y"}));
  }

  // All fields skipped
  {
    const auto w = write_strict(AllSkipped{.a = 1, .b = "x"});
    ASSERT_EQ(w.objects.size(), 1u);
    EXPECT_EQ(w.objects[0].expected_size, 0u);
    EXPECT_EQ(w.objects[0].actual_count, 0u);
    EXPECT_TRUE(w.objects[0].keys.empty());
  }

  // Nested Skip + Rename
  {
    const auto w = write_strict(NestedSkipAndRename{
        .inner = 7,
        .child = SkipDifferentTypes{
            .items = "secret", .serialized_items = std::vector<int>{8, 9}}});
    ASSERT_EQ(w.objects.size(), 2u);
    for (const auto& obj : w.objects) {
      EXPECT_EQ(obj.expected_size, obj.actual_count);
    }
    EXPECT_EQ(w.objects[0].expected_size, 1u);
    EXPECT_EQ(w.objects[0].keys, (std::vector<std::string>{"inner"}));
    EXPECT_EQ(w.objects[1].expected_size, 1u);
    EXPECT_EQ(w.objects[1].keys, (std::vector<std::string>{"items"}));
  }

  // NoFieldNames emits all tuple slots into an array (including Skip as null)
  {
    const auto w = write_strict<rfl::NoFieldNames>(SkipAndRename{
        .items = std::vector<int>{99},
        .serialized_items = std::vector<int>{1, 2}});
    ASSERT_EQ(w.objects.size(), 0u);
    ASSERT_EQ(w.arrays.size(), 2u);
    EXPECT_EQ(w.arrays[0].expected_size, 2u);
    EXPECT_EQ(w.arrays[0].actual_count, 2u);
    EXPECT_EQ(w.arrays[1].expected_size, 2u);
    EXPECT_EQ(w.arrays[1].actual_count, 2u);
  }
}

struct RenameAndSkipSameType {
  rfl::Rename<"items", std::vector<int>> serialized_items;
  rfl::Skip<std::vector<int>> items;
};

struct RenameAndSkipDifferentTypes {
  rfl::Rename<"items", std::vector<int>> serialized_items;
  rfl::Skip<std::string> items;
};

struct SplitSkipSerFirstDiffTypes {
  rfl::SkipSerialization<int> items;
  rfl::Rename<"items", rfl::SkipDeserialization<std::string>> serialized_items;
};

struct SplitSkipDeserFirstDiffTypes {
  rfl::Rename<"items", rfl::SkipDeserialization<std::string>> serialized_items;
  rfl::SkipSerialization<int> items;
};

struct FieldStructSkipSameType {
  rfl::Field<"items", rfl::Skip<std::vector<int>>> items;
  rfl::Field<"items", std::vector<int>> other;
};

struct FieldStructSkipDiffType {
  rfl::Field<"items", rfl::Skip<std::string>> items;
  rfl::Field<"items", std::vector<int>> other;
};

struct FieldStructSplitSkip {
  rfl::Field<"items", rfl::SkipSerialization<int>> items;
  rfl::Field<"items", rfl::SkipDeserialization<std::string>> other;
};

struct FieldSubStruct {
  rfl::Field<"items", std::vector<int>> other;
  rfl::Field<"tag", std::string> tag;
};

struct FieldFlattenedStruct {
  rfl::Field<"items", rfl::Skip<std::string>> items;
  rfl::Flatten<FieldSubStruct> sub;
};

struct TwoSkipsSameName {
  rfl::Skip<int> items;
  rfl::Rename<"items", rfl::Skip<std::string>> other_items;
};

TEST(generic, test_rename_skip_from_named_tuple) {
  // 1. Plain struct: Skip<T> + Rename<"items", T> (T1 == T2)
  {
    const auto orig = SkipAndRename{
        .items = std::vector<int>{99},
        .serialized_items = std::vector<int>{1, 2, 3}};
    const auto nt = rfl::to_named_tuple(orig);
    const auto roundtrip = rfl::from_named_tuple<SkipAndRename>(nt);
    EXPECT_EQ(roundtrip.items.value(), (std::vector<int>{99}));
    EXPECT_EQ(roundtrip.serialized_items.value(), (std::vector<int>{1, 2, 3}));

    // Cross-type NamedTuple conversion with reversed field order (lvalue & rvalue)
    const auto converted_copy =
        rfl::from_named_tuple<RenameAndSkipSameType>(nt);
    EXPECT_EQ(converted_copy.items.value(), (std::vector<int>{99}));
    EXPECT_EQ(converted_copy.serialized_items.value(),
              (std::vector<int>{1, 2, 3}));

    auto nt_move = rfl::to_named_tuple(orig);
    const auto converted_move =
        rfl::from_named_tuple<RenameAndSkipSameType>(std::move(nt_move));
    EXPECT_EQ(converted_move.items.value(), (std::vector<int>{99}));
    EXPECT_EQ(converted_move.serialized_items.value(),
              (std::vector<int>{1, 2, 3}));
  }

  // 2. Plain struct: Skip<T1> + Rename<"items", T2> (T1 != T2)
  {
    const auto orig = SkipDifferentTypes{
        .items = "internal", .serialized_items = std::vector<int>{4, 5}};
    const auto nt = rfl::to_named_tuple(orig);
    const auto roundtrip = rfl::from_named_tuple<SkipDifferentTypes>(nt);
    EXPECT_EQ(roundtrip.items.value(), "internal");
    EXPECT_EQ(roundtrip.serialized_items.value(), (std::vector<int>{4, 5}));

    // Cross-type NamedTuple conversion with reversed order
    const auto converted =
        rfl::from_named_tuple<RenameAndSkipDifferentTypes>(nt);
    EXPECT_EQ(converted.items.value(), "internal");
    EXPECT_EQ(converted.serialized_items.value(), (std::vector<int>{4, 5}));

    // Cross-type NamedTuple conversion from manual Field<"items", T2> (no Rename)
    auto manual_nt = rfl::make_named_tuple(
        rfl::make_field<"items">(rfl::Skip<std::string>("manual")),
        rfl::make_field<"items">(std::vector<int>{7, 8}));
    const auto from_manual =
        rfl::from_named_tuple<SkipDifferentTypes>(std::move(manual_nt));
    EXPECT_EQ(from_manual.items.value(), "manual");
    EXPECT_EQ(from_manual.serialized_items.value(), (std::vector<int>{7, 8}));
  }

  // 3. Plain struct: SkipSerialization<T1> + Rename<"items", SkipDeserialization<T2>>
  {
    const auto orig = SplitSkipSerFirstDiffTypes{.items = 42,
                                                 .serialized_items = "hello"};
    const auto nt = rfl::to_named_tuple(orig);
    const auto roundtrip =
        rfl::from_named_tuple<SplitSkipSerFirstDiffTypes>(nt);
    EXPECT_EQ(roundtrip.items.value(), 42);
    EXPECT_EQ(roundtrip.serialized_items.value().value(), "hello");

    const auto reversed =
        rfl::from_named_tuple<SplitSkipDeserFirstDiffTypes>(nt);
    EXPECT_EQ(reversed.items.value(), 42);
    EXPECT_EQ(reversed.serialized_items.value().value(), "hello");

    const auto same_t = SplitSkipSerFirst{
        .items = std::vector<int>{1, 2},
        .serialized_items = std::vector<int>{3, 4}};
    const auto same_t_rt =
        rfl::from_named_tuple<SplitSkipSerFirst>(rfl::to_named_tuple(same_t));
    EXPECT_EQ(same_t_rt.items.value(), (std::vector<int>{1, 2}));
    EXPECT_EQ(same_t_rt.serialized_items.value().value(),
              (std::vector<int>{3, 4}));
  }

  // 4. rfl::Field structs (exercises internal::move_from_named_tuple / make_ptr_fields)
  {
    const auto s_same = FieldStructSkipSameType{
        .items = rfl::Skip<std::vector<int>>(std::vector<int>{10, 20}),
        .other = std::vector<int>{30, 40}};
    const auto nt_same = rfl::to_named_tuple(s_same);
    const auto rt_same = rfl::from_named_tuple<FieldStructSkipSameType>(nt_same);
    EXPECT_EQ(rt_same.items.value().value(), (std::vector<int>{10, 20}));
    EXPECT_EQ(rt_same.other.value(), (std::vector<int>{30, 40}));

    const auto s_diff = FieldStructSkipDiffType{
        .items = rfl::Skip<std::string>("secret"),
        .other = std::vector<int>{50, 60}};
    const auto nt_diff = rfl::to_named_tuple(s_diff);
    const auto rt_diff = rfl::from_named_tuple<FieldStructSkipDiffType>(nt_diff);
    EXPECT_EQ(rt_diff.items.value().value(), "secret");
    EXPECT_EQ(rt_diff.other.value(), (std::vector<int>{50, 60}));

    // Reversed NamedTuple -> FieldStructSkipDiffType (exercises both retrieve_fields and make_ptr_fields)
    auto rev_nt = rfl::make_named_tuple(
        rfl::make_field<"items">(std::vector<int>{70, 80}),
        rfl::make_field<"items">(rfl::Skip<std::string>("rev")));
    const auto from_rev =
        rfl::from_named_tuple<FieldStructSkipDiffType>(std::move(rev_nt));
    EXPECT_EQ(from_rev.items.value().value(), "rev");
    EXPECT_EQ(from_rev.other.value(), (std::vector<int>{70, 80}));

    const auto s_split = FieldStructSplitSkip{
        .items = rfl::SkipSerialization<int>(123),
        .other = rfl::SkipDeserialization<std::string>("abc")};
    const auto nt_split = rfl::to_named_tuple(s_split);
    const auto rt_split =
        rfl::from_named_tuple<FieldStructSplitSkip>(nt_split);
    EXPECT_EQ(rt_split.items.value().value(), 123);
    EXPECT_EQ(rt_split.other.value().value(), "abc");

    // Flattened rfl::Field struct with shared wire name
    const auto s_flat = FieldFlattenedStruct{
        .items = rfl::Skip<std::string>("outer_skip"),
        .sub = FieldSubStruct{.other = std::vector<int>{1, 2}, .tag = "ok"}};
    const auto nt_flat = rfl::to_named_tuple(s_flat);
    const auto rt_flat = rfl::from_named_tuple<FieldFlattenedStruct>(nt_flat);
    EXPECT_EQ(rt_flat.items.value().value(), "outer_skip");
    EXPECT_EQ(rt_flat.sub.value().other.value(), (std::vector<int>{1, 2}));
    EXPECT_EQ(rt_flat.sub.value().tag.value(), "ok");
  }

  // 5. Multiple Skip fields with same wire name
  {
    const auto two =
        TwoSkipsSameName{.items = 7, .other_items = std::string("seven")};
    const auto nt = rfl::to_named_tuple(two);
    const auto rt = rfl::from_named_tuple<TwoSkipsSameName>(nt);
    EXPECT_EQ(rt.items.value(), 7);
    EXPECT_EQ(rt.other_items.value().value(), "seven");

    auto manual_two = rfl::make_named_tuple(
        rfl::make_field<"items">(rfl::Skip<int>(11)),
        rfl::make_field<"items">(rfl::Skip<std::string>("eleven")));
    const auto from_manual_two =
        rfl::from_named_tuple<TwoSkipsSameName>(std::move(manual_two));
    EXPECT_EQ(from_manual_two.items.value(), 11);
    EXPECT_EQ(from_manual_two.other_items.value().value(), "eleven");
  }
}

}  // namespace test_generic_rename_skip
