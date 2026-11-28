#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <rfl.hpp>
#include <rfl/internal/bind_to_tuple.hpp>
#include <rfl/internal/has_default_val_v.hpp>
#include <rfl/json.hpp>
#include <string>

namespace {
struct AnonPlain {
  int a;
  double b;
};
struct AnonDefault {
  int a;
  rfl::DefaultVal<int> b = 1;
};
}  // namespace

namespace test_has_default_val {

struct Empty {};

struct Plain {
  int a;
  std::string b;
};

struct DirectDefault {
  int a;
  rfl::DefaultVal<int> b = 7;
};

struct RenamedDefault {
  rfl::Rename<"val", rfl::DefaultVal<int>> b = 9;
};

struct FieldWrapperDefault {
  rfl::Field<"a", int> a;
  rfl::Field<"b", rfl::DefaultVal<int>> b;
};

struct FieldWrapperNoDefault {
  rfl::Field<"a", int> a;
  rfl::Field<"b", std::string> b;
};

struct NonFlattenedNested {
  DirectDefault inner;
};

struct FlattenedWithDefault {
  int x;
  rfl::Flatten<DirectDefault> inner;
};

struct DeepFlattenedWithDefault {
  rfl::Flatten<FlattenedWithDefault> outer;
};

struct FlattenedPtrWithDefault {
  rfl::Flatten<const DirectDefault*> inner;
};

struct FlattenedNoDefault {
  rfl::Flatten<Plain> inner;
};

struct FlattenedNamedTupleWithDefault {
  rfl::Flatten<rfl::NamedTuple<rfl::Field<"a", rfl::DefaultVal<int>>>> nt;
};

struct NonDefaultConstructible {
  int x;
  explicit NonDefaultConstructible(int _x) : x(_x) {}
};

struct WithNonDefaultConstructible {
  NonDefaultConstructible ndc;
  std::unique_ptr<int> ptr;
};

struct WithConst {
  const int c;
  std::array<int, 3> arr;
  rfl::DefaultVal<int> d = 5;
};

struct BaseWithDefault {
  rfl::DefaultVal<int> b = 42;
};

struct DerivedEmpty : BaseWithDefault {};

class NonAggregate {
  int x_ = 0;

 public:
  int x() const { return x_; }
};

union SimpleUnion {
  int a;
  double b;
};

TEST(json, test_has_default_val_trait_and_apply_to_fields) {
  using rfl::internal::apply_to_fields;
  using rfl::internal::has_default_val_v;

  static_assert(!has_default_val_v<int>);
  static_assert(!has_default_val_v<Empty>);
  static_assert(!has_default_val_v<Plain>);
  static_assert(!has_default_val_v<AnonPlain>);
  static_assert(has_default_val_v<AnonDefault>);
  static_assert(!has_default_val_v<NonAggregate>);
  static_assert(!has_default_val_v<SimpleUnion>);
  static_assert(!has_default_val_v<WithNonDefaultConstructible>);
  static_assert(!has_default_val_v<FieldWrapperNoDefault>);
  static_assert(!has_default_val_v<NonFlattenedNested>);
  static_assert(!has_default_val_v<FlattenedNoDefault>);
  static_assert(!has_default_val_v<std::array<Plain, 2>>);
  static_assert(!has_default_val_v<Plain[2]>);

  static_assert(has_default_val_v<DirectDefault>);
  static_assert(has_default_val_v<const DirectDefault&>);
  static_assert(has_default_val_v<RenamedDefault>);
  static_assert(has_default_val_v<FieldWrapperDefault>);
  static_assert(has_default_val_v<FlattenedWithDefault>);
  static_assert(has_default_val_v<DeepFlattenedWithDefault>);
  static_assert(has_default_val_v<FlattenedPtrWithDefault>);
  static_assert(has_default_val_v<FlattenedNamedTupleWithDefault>);
  static_assert(has_default_val_v<WithConst>);
  static_assert(has_default_val_v<DerivedEmpty>);
  static_assert(has_default_val_v<std::array<DirectDefault, 2>>);
  static_assert(has_default_val_v<DirectDefault[2]>);
  static_assert(
      has_default_val_v<rfl::NamedTuple<rfl::Field<"a", rfl::DefaultVal<int>>>>);
  static_assert(
      !has_default_val_v<rfl::NamedTuple<rfl::Field<"a", int>>>);

  // Verify apply_to_fields directly on empty, mutable, and const aggregates.
  Empty empty{};
  const auto empty_count =
      apply_to_fields(empty, [](auto&... _fs) { return sizeof...(_fs); });
  EXPECT_EQ(empty_count, 0U);

  Plain plain{1, "hi"};
  apply_to_fields(plain, [](int& _a, std::string& _b) {
    _a = 10;
    _b = "bye";
  });
  EXPECT_EQ(plain.a, 10);
  EXPECT_EQ(plain.b, "bye");

  const Plain& cplain = plain;
  const auto sum_len = apply_to_fields(
      cplain, [](const int& _a, const std::string& _b) {
        return _a + static_cast<int>(_b.size());
      });
  EXPECT_EQ(sum_len, 13);

  // End-to-end JSON read using FlattenedWithDefault (exercises DefaultVal path).
  const auto parsed =
      rfl::json::read<FlattenedWithDefault>(R"({"x":3,"a":4})").value();
  EXPECT_EQ(parsed.x, 3);
  EXPECT_EQ(parsed.inner.value().a, 4);
  EXPECT_EQ(parsed.inner.value().b.value(), 7);
}

}  // namespace test_has_default_val
