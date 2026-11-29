#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <rfl.hpp>
#include <rfl/internal/bind_to_tuple.hpp>
#include <rfl/internal/has_default_val_v.hpp>
#include <rfl/json.hpp>
#include <rfl/parsing/TaggedUnionWrapper.hpp>
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

struct Incomplete;

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

struct FieldWrapperRenamedDefault {
  rfl::Field<"a", rfl::Rename<"val", rfl::DefaultVal<int>>> a;
};

struct FieldWrapperNoDefault {
  rfl::Field<"a", int> a;
  rfl::Field<"b", std::string> b;
};

struct RawPointerToDefault {
  rfl::DefaultVal<int>* p = nullptr;
};

struct RawPointerToField {
  rfl::Field<"a", rfl::DefaultVal<int>>* p = nullptr;
};

struct RawPointerToFlatten {
  rfl::Flatten<DirectDefault>* p = nullptr;
};

struct RawPointerToIncompleteFlatten {
  rfl::Flatten<Incomplete>* p = nullptr;
};

struct RawPointerToIncompleteField {
  rfl::Field<"a", Incomplete>* p = nullptr;
};

struct FieldWrappingFlatten {
  rfl::Field<"a", rfl::Flatten<DirectDefault>> f;
};

struct NestedFieldWrapper {
  rfl::Field<"a", rfl::Field<"b", rfl::DefaultVal<int>>> f;
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

struct WithExtraFields {
  int a;
  rfl::ExtraFields<rfl::Generic> extra;
};

struct WithExtraFieldsAndDefault {
  rfl::DefaultVal<int> a = 1;
  rfl::ExtraFields<rfl::Generic> extra;
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

struct RvalueOnlyVisitor {
  int& operator()(int& _a, std::string&) && { return _a; }
};

TEST(json, test_has_default_val_trait_and_apply_to_fields) {
  using rfl::internal::apply_to_fields;
  using rfl::internal::has_default_val_v;

  struct LocalPlain {
    int a;
  };
  struct LocalDefault {
    rfl::DefaultVal<int> a = 3;
  };

  static_assert(!has_default_val_v<int>);
  static_assert(!has_default_val_v<Empty>);
  static_assert(!has_default_val_v<Plain>);
  static_assert(!has_default_val_v<AnonPlain>);
  static_assert(!has_default_val_v<LocalPlain>);
  static_assert(!has_default_val_v<NonAggregate>);
  static_assert(!has_default_val_v<SimpleUnion>);
  static_assert(!has_default_val_v<WithNonDefaultConstructible>);
  static_assert(!has_default_val_v<FieldWrapperNoDefault>);
  static_assert(!has_default_val_v<RawPointerToDefault>);
  static_assert(!has_default_val_v<RawPointerToField>);
  static_assert(!has_default_val_v<RawPointerToFlatten>);
  static_assert(!has_default_val_v<RawPointerToIncompleteFlatten>);
  static_assert(!has_default_val_v<RawPointerToIncompleteField>);
  static_assert(!has_default_val_v<FieldWrappingFlatten>);
  static_assert(!has_default_val_v<NestedFieldWrapper>);
  static_assert(!has_default_val_v<NonFlattenedNested>);
  static_assert(!has_default_val_v<FlattenedNoDefault>);
  static_assert(!has_default_val_v<WithExtraFields>);
  static_assert(!has_default_val_v<rfl::Tuple<int, rfl::DefaultVal<int>>>);
  static_assert(!has_default_val_v<std::array<Plain, 2>>);
  static_assert(!has_default_val_v<Plain[2]>);

  static_assert(has_default_val_v<DirectDefault>);
  static_assert(has_default_val_v<const DirectDefault&>);
  static_assert(has_default_val_v<AnonDefault>);
  static_assert(has_default_val_v<LocalDefault>);
  static_assert(has_default_val_v<RenamedDefault>);
  static_assert(has_default_val_v<FieldWrapperDefault>);
  static_assert(has_default_val_v<FieldWrapperRenamedDefault>);
  static_assert(has_default_val_v<FlattenedWithDefault>);
  static_assert(has_default_val_v<DeepFlattenedWithDefault>);
  static_assert(has_default_val_v<FlattenedPtrWithDefault>);
  static_assert(has_default_val_v<FlattenedNamedTupleWithDefault>);
  static_assert(has_default_val_v<WithExtraFieldsAndDefault>);
  static_assert(has_default_val_v<WithConst>);
  static_assert(has_default_val_v<DerivedEmpty>);
  static_assert(has_default_val_v<std::array<DirectDefault, 2>>);
  static_assert(has_default_val_v<DirectDefault[2]>);
  static_assert(has_default_val_v<
                rfl::NamedTuple<rfl::Field<"a", rfl::DefaultVal<int>>>>);
  static_assert(!has_default_val_v<rfl::NamedTuple<rfl::Field<"a", int>>>);

  // TaggedUnion wrappers (both pointer and value forms).
  static_assert(has_default_val_v<rfl::parsing::TaggedUnionWrapperWithFields<
                    DirectDefault, rfl::Literal<"kind">, "d", true>>);
  static_assert(has_default_val_v<rfl::parsing::TaggedUnionWrapperWithFields<
                    DirectDefault, rfl::Literal<"kind">, "d", false>>);
  static_assert(!has_default_val_v<rfl::parsing::TaggedUnionWrapperWithFields<
                    Plain, rfl::Literal<"kind">, "p", true>>);
  static_assert(has_default_val_v<rfl::parsing::TaggedUnionWrapperNoFields<
                    DirectDefault, rfl::Literal<"kind">, "d", true>>);
  static_assert(has_default_val_v<rfl::parsing::TaggedUnionWrapperNoFields<
                    DirectDefault, rfl::Literal<"kind">, "d", false>>);
  static_assert(!has_default_val_v<rfl::parsing::TaggedUnionWrapperNoFields<
                    Plain, rfl::Literal<"kind">, "p", true>>);
  static_assert(!has_default_val_v<rfl::parsing::TaggedUnionWrapperNoFields<
                    Plain, rfl::Literal<"kind">, "p", false>>);

  // Verify apply_to_fields directly on empty, mutable, const, reference return,
  // and rvalue-only callable cases.
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

  int& a_ref = apply_to_fields(plain, RvalueOnlyVisitor{});
  a_ref = 25;
  EXPECT_EQ(plain.a, 25);

  const Plain& cplain = plain;
  const auto sum_len =
      apply_to_fields(cplain, [](const int& _a, const std::string& _b) {
        return _a + static_cast<int>(_b.size());
      });
  EXPECT_EQ(sum_len, 28);

  // End-to-end JSON read using FlattenedWithDefault (exercises DefaultVal
  // path).
  const auto parsed =
      rfl::json::read<FlattenedWithDefault>(R"({"x":3,"a":4})").value();
  EXPECT_EQ(parsed.x, 3);
  EXPECT_EQ(parsed.inner.value().a, 4);
  EXPECT_EQ(parsed.inner.value().b.value(), 7);
}

}  // namespace test_has_default_val
