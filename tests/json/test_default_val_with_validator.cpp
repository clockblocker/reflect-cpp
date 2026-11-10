#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

// Structs with default values are read by default-constructing them first.
// A Validator without a valid default value used to make that fail, so such
// structs could not be read at all (#700). Now its default value is only
// checked if the field is missing from the input.
namespace test_default_val_with_validator {

using Path = std::vector<rfl::ErrorDetail::PathSegment>;

std::vector<Path> paths(const rfl::Error& _err) {
  std::vector<Path> result;
  for (const auto& detail : _err.details()) {
    result.push_back(detail.path);
  }
  return result;
}

using Foods =
    rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>>;
using Positive = rfl::Validator<int, rfl::Minimum<1>>;

// The test from #700.
struct Person {
  rfl::DefaultVal<std::string> name = "Homer Simpson";
  Foods favorite_foods;
};

TEST(json, test_default_val_with_validator_issue_700) {
  static_assert(rfl::internal::has_default_val_v<Person>);

  const auto should_fail = rfl::json::read<Person>(R"({})");
  ASSERT_FALSE(should_fail);
  EXPECT_EQ(should_fail.error().what(),
            "Field named 'favorite_foods' not found.");

  const auto homer =
      rfl::json::read<Person>(R"({"favorite_foods": ["donuts", "pizza"]})");
  ASSERT_TRUE(homer) << homer.error().what();
  EXPECT_EQ(homer->name.value(), "Homer Simpson");
  ASSERT_EQ(homer->favorite_foods->size(), 2u);
  EXPECT_EQ(homer->favorite_foods->at(0), "donuts");
  EXPECT_EQ(rfl::json::write(*homer),
            R"({"name":"Homer Simpson","favorite_foods":["donuts","pizza"]})");
}

TEST(json, test_default_val_with_validator_present_but_invalid) {
  const auto res = rfl::json::read<Person>(R"({"favorite_foods": []})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Failed to parse field 'favorite_foods': Size validation failed: "
            "Value expected to be greater than or equal to 1, but got 0.");
}

// The same bug without any DefaultVal.
struct PersonWithoutDefaultVal {
  std::string name = "Homer Simpson";
  Foods favorite_foods;
};

TEST(json, test_default_val_with_validator_default_if_missing) {
  const auto res =
      rfl::json::read<PersonWithoutDefaultVal, rfl::DefaultIfMissing>(
          R"({"favorite_foods": ["donuts"]})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->name, "Homer Simpson");
  EXPECT_EQ(res->favorite_foods->size(), 1u);
}

TEST(json, test_default_val_with_validator_default_if_missing_invalid_default) {
  const auto res =
      rfl::json::read<PersonWithoutDefaultVal, rfl::DefaultIfMissing>(
          R"({"name": "Marge"})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Field named 'favorite_foods' not found, and its default value is "
            "invalid: Size validation failed: Value expected to be greater "
            "than or equal to 1, but got 0.");
  const auto expected = std::vector<rfl::ErrorDetail>(
      {{.path = {"favorite_foods"},
        .message = "Field named 'favorite_foods' not found, and its default "
                   "value is invalid: Size validation failed: Value expected "
                   "to be greater than or equal to 1, but got 0."}});
  EXPECT_EQ(res.error().details(), expected);
}

// A DefaultVal whose own default is invalid must not make the field optional.
struct Counters {
  std::string id;
  rfl::DefaultVal<Positive> first;
  rfl::DefaultVal<Positive> second;
};

TEST(json, test_default_val_with_validator_invalid_default_val) {
  const auto res = rfl::json::read<Counters>(R"({"id": "a", "second": 3})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Field named 'first' not found, and its default value is invalid: "
            "Value expected to be greater than or equal to 1, but got 0.");
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"first"}}));
}

TEST(json, test_default_val_with_validator_invalid_default_vals) {
  const auto res = rfl::json::read<Counters>(R"({"id": "a"})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Found 2 errors:\n"
            "1) Field named 'first' not found, and its default value is "
            "invalid: Value expected to be greater than or equal to 1, but got "
            "0.\n"
            "2) Field named 'second' not found, and its default value is "
            "invalid: Value expected to be greater than or equal to 1, but got "
            "0.");
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"first"}, {"second"}}));
}

TEST(json, test_default_val_with_validator_invalid_default_vals_fail_fast) {
  const auto res = rfl::json::read<Counters, rfl::FailFast>(R"({"id": "a"})");
  ASSERT_FALSE(res);
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"first"}}));
}

TEST(json, test_default_val_with_validator_default_vals_present) {
  const auto res =
      rfl::json::read<Counters>(R"({"id": "a", "first": 2, "second": 3})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->first.value().value(), 2);
  EXPECT_EQ(res->second.value().value(), 3);
}

// Only a Validator without an initializer is checked lazily. An explicit
// initializer must be valid, as before.
struct ExplicitInvalidDefault {
  rfl::DefaultVal<Positive> explicit_zero = 0;
};

TEST(json, test_default_val_with_validator_explicit_invalid_default) {
  const auto res =
      rfl::json::read<ExplicitInvalidDefault>(R"({"explicit_zero": 3})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Value expected to be greater than or equal to 1, but got 0.");
}

struct ValidDefaults {
  rfl::DefaultVal<Positive> number = 5;
  Foods foods = std::vector<std::string>({"beer"});
};

TEST(json, test_default_val_with_validator_valid_defaults) {
  const auto res = rfl::json::read<ValidDefaults, rfl::DefaultIfMissing>("{}");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->number.value().value(), 5);
  EXPECT_EQ(res->foods->at(0), "beer");
}

struct Inner {
  Positive v;
  std::string tag = "t";
};

struct Outer {
  rfl::DefaultVal<int> x = 1;
  Inner inner;
};

TEST(json, test_default_val_with_validator_nested) {
  const auto res = rfl::json::read<Outer>(R"({"inner": {"v": 4, "tag": "q"}})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->inner.v.value(), 4);
}

TEST(json, test_default_val_with_validator_nested_struct_missing) {
  const auto res = rfl::json::read<Outer, rfl::DefaultIfMissing>(R"({})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Field named 'inner' not found, and its default value is invalid "
            "at 'inner.v': Value expected to be greater than or equal to 1, "
            "but got 0.");
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"inner", "v"}}));
}

TEST(json, test_default_val_with_validator_nested_field_missing) {
  const auto res =
      rfl::json::read<Outer, rfl::DefaultIfMissing>(R"({"inner": {}})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Failed to parse field 'inner': Field named 'v' not found, and its "
            "default value is invalid: Value expected to be greater than or "
            "equal to 1, but got 0.");
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"inner", "v"}}));
}

struct People {
  rfl::DefaultVal<int> x;
  std::vector<Person> people;
};

TEST(json, test_default_val_with_validator_vector) {
  const auto res = rfl::json::read<People>(
      R"({"people": [{"favorite_foods": ["a"]}, {"name": "B", "favorite_foods": ["b"]}]})");
  ASSERT_TRUE(res) << res.error().what();
  ASSERT_EQ(res->people.size(), 2u);
  EXPECT_EQ(res->people[0].name.value(), "Homer Simpson");
  EXPECT_EQ(res->people[1].favorite_foods->at(0), "b");
}

TEST(json, test_default_val_with_validator_vector_error_path) {
  const auto res =
      rfl::json::read<People, rfl::DefaultIfMissing>(R"({"people": [{}]})");
  ASSERT_FALSE(res);
  EXPECT_EQ(paths(res.error()),
            std::vector<Path>({{"people", size_t{0}, "favorite_foods"}}));
}

// An invalid default on the heap cannot be attributed to a field, so it fails
// every read, as before.
struct HeapDefault {
  rfl::DefaultVal<int> x;
  std::vector<Positive> v = std::vector<Positive>(1);
};

TEST(json, test_default_val_with_validator_heap_default) {
  const auto res = rfl::json::read<HeapDefault>(R"({"v": [1]})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Value expected to be greater than or equal to 1, but got 0.");
}

struct WithOptional {
  rfl::DefaultVal<int> x;
  std::optional<Positive> engaged{std::in_place};
  std::optional<Positive> empty;
};

TEST(json, test_default_val_with_validator_optional) {
  const auto res = rfl::json::read<WithOptional>(R"({"engaged": 3})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->engaged->value(), 3);
  EXPECT_FALSE(res->empty);
}

TEST(json, test_default_val_with_validator_optional_null) {
  const auto res = rfl::json::read<WithOptional>(R"({"engaged": null})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_FALSE(res->engaged);
}

TEST(json, test_default_val_with_validator_optional_missing) {
  const auto res = rfl::json::read<WithOptional>(R"({})");
  ASSERT_FALSE(res);
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"engaged"}}));
}

struct HasValue {
  template <class T>
  static rfl::Result<T> validate(T _value) {
    if (!_value) {
      return rfl::error("Must have a value.");
    }
    return _value;
  }

  template <class T>
  static rfl::parsing::schema::ValidationType to_schema() {
    return {};
  }
};

struct ValidatedOptional {
  rfl::DefaultVal<int> x;
  rfl::Validator<std::optional<int>, HasValue> v;
};

TEST(json, test_default_val_with_validator_validated_optional) {
  const auto res = rfl::json::read<ValidatedOptional>(R"({})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Field named 'v' not found, and its default value is invalid: "
            "Must have a value.");
  const auto res2 = rfl::json::read<ValidatedOptional>(R"({"v": 3})");
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_EQ(res2->v.value(), 3);
}

struct Combinators {
  rfl::DefaultVal<int> x;
  rfl::Validator<int, rfl::AllOf<rfl::Minimum<1>, rfl::Maximum<10>>> all_of;
  rfl::Validator<int, rfl::Minimum<1>, rfl::Maximum<10>> several;
  rfl::Validator<int, rfl::AnyOf<rfl::Minimum<5>, rfl::Maximum<-5>>> any_of;
  rfl::Validator<int, rfl::OneOf<rfl::Minimum<5>, rfl::Maximum<-5>>> one_of;
  rfl::Email email;
};

TEST(json, test_default_val_with_validator_combinators) {
  const auto res = rfl::json::read<Combinators>(
      R"({"all_of": 2, "several": 3, "any_of": 7, "one_of": -9, "email": "a@b.cd"})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->one_of.value(), -9);
  EXPECT_EQ(res->email.value(), "a@b.cd");
}

TEST(json, test_default_val_with_validator_combinators_missing) {
  const auto res =
      rfl::json::read<Combinators, rfl::DefaultIfMissing>(R"({"all_of": 2})");
  ASSERT_FALSE(res);
  EXPECT_EQ(
      paths(res.error()),
      std::vector<Path>({{"several"}, {"any_of"}, {"one_of"}, {"email"}}));
}

struct CamelCase {
  rfl::DefaultVal<int> some_number;
  rfl::Rename<"renamedFoods", Foods> foods_field;
  Positive positive_value;
};

TEST(json, test_default_val_with_validator_field_names) {
  const auto res = rfl::json::read<CamelCase, rfl::SnakeCaseToCamelCase>(
      R"({"renamedFoods": ["a"], "positiveValue": 3})");
  ASSERT_TRUE(res) << res.error().what();
  const auto res2 = rfl::json::read<CamelCase, rfl::SnakeCaseToCamelCase,
                                    rfl::DefaultIfMissing>(R"({})");
  ASSERT_FALSE(res2);
  EXPECT_EQ(paths(res2.error()),
            std::vector<Path>({{"renamedFoods"}, {"positiveValue"}}));
}

TEST(json, test_default_val_with_validator_no_extra_fields) {
  const auto res = rfl::json::read<Person, rfl::NoExtraFields>(
      R"({"favorite_foods": ["a"], "extra": 1})");
  ASSERT_FALSE(res);
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"extra"}}));
  const auto res2 = rfl::json::read<Person, rfl::NoExtraFields>(
      R"({"favorite_foods": ["a"]})");
  ASSERT_TRUE(res2) << res2.error().what();
}

TEST(json, test_default_val_with_validator_no_optionals) {
  const auto res = rfl::json::read<WithOptional, rfl::NoOptionals>(
      R"({"x": 1, "engaged": 3, "empty": null})");
  ASSERT_TRUE(res) << res.error().what();
  const auto res2 = rfl::json::read<WithOptional, rfl::NoOptionals>(
      R"({"x": 1, "engaged": 3})");
  ASSERT_FALSE(res2);
}

TEST(json, test_default_val_with_validator_no_field_names) {
  const auto res =
      rfl::json::read<Person, rfl::NoFieldNames>(R"(["Marge", ["donuts"]])");
  ASSERT_TRUE(res) << res.error().what();
  const auto res2 = rfl::json::read<Counters, rfl::NoFieldNames>(R"(["a", 2])");
  ASSERT_FALSE(res2);
  EXPECT_EQ(paths(res2.error()), std::vector<Path>({{"second"}}));
}

struct FlatPart {
  Positive flat_v;
};

struct WithFlatten {
  rfl::DefaultVal<int> x;
  rfl::Flatten<FlatPart> part;
};

TEST(json, test_default_val_with_validator_flatten) {
  const auto res = rfl::json::read<WithFlatten>(R"({"flat_v": 2})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->part.get().flat_v.value(), 2);
  const auto res2 =
      rfl::json::read<WithFlatten, rfl::DefaultIfMissing>(R"({})");
  ASSERT_FALSE(res2);
  EXPECT_EQ(paths(res2.error()), std::vector<Path>({{"flat_v"}}));
}

TEST(json, test_default_val_with_validator_add_struct_name) {
  const auto res = rfl::json::read<Person, rfl::AddStructName<"type">>(
      R"({"type": "Person", "favorite_foods": ["a"]})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->name.value(), "Homer Simpson");
}

// Copies of an invalid default to the heap are overwritten like the
// original when their fields are in the input.
struct CopiesIntoVector {
  rfl::DefaultVal<int> x;
  Positive a;
  std::vector<Positive> v = {a};
  Positive b;
};

TEST(json, test_default_val_with_validator_copy_in_vector) {
  const auto res =
      rfl::json::read<CopiesIntoVector>(R"({"a": 1, "v": [2], "b": 3})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->v.at(0).value(), 2);
  const auto res2 = rfl::json::read<CopiesIntoVector, rfl::DefaultIfMissing>(
      R"({"a": 1, "b": 3})");
  ASSERT_FALSE(res2);
  EXPECT_EQ(paths(res2.error()), std::vector<Path>({{"v", size_t{0}}}));
}

}  // namespace test_default_val_with_validator
