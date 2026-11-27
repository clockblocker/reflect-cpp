#ifndef RFL_JSON_SCHEMA_TYPE_HPP_
#define RFL_JSON_SCHEMA_TYPE_HPP_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../../ExtraFields.hpp"
#include "../../Flatten.hpp"
#include "../../Generic.hpp"
#include "../../Literal.hpp"
#include "../../Object.hpp"
#include "../../Ref.hpp"
#include "../../Rename.hpp"
#include "../../Variant.hpp"

namespace rfl::json::schema {

/// The JSON representation of internal::schema::Type.
struct Type {
  /// Common annotation fields shared by all schema subtypes.
  struct Annotations {
    std::optional<std::string> title{};
    std::optional<std::string> description{};
    std::optional<bool> deprecated{};
    std::optional<std::string> deprecationMessage{};
    rfl::Rename<"default", std::optional<rfl::Generic>> defaultValue{};
    std::optional<std::vector<rfl::Generic>> examples{};
    rfl::ExtraFields<rfl::Generic> extensions{};
  };

  struct Boolean {
    Literal<"boolean"> type{};
    rfl::Flatten<Annotations> annotations{};
  };

  struct Integer {
    Literal<"integer"> type{};
    rfl::Flatten<Annotations> annotations{};
    std::optional<int64_t> minimum{};
    std::optional<rfl::Variant<int64_t, uint64_t>> maximum{};
  };

  struct Number {
    Literal<"number"> type{};
    rfl::Flatten<Annotations> annotations{};
  };

  struct String {
    Literal<"string"> type{};
    rfl::Flatten<Annotations> annotations{};
    rfl::Rename<"minLength", std::optional<size_t>> minSize{};
    rfl::Rename<"maxLength", std::optional<size_t>> maxSize{};
    std::optional<std::string> contentEncoding{};
  };

  using NumericType = rfl::Variant<Integer, Number>;

  struct AllOf {
    rfl::Flatten<Annotations> annotations{};
    std::vector<Type> allOf{};
  };

  struct AnyOf {
    rfl::Flatten<Annotations> annotations{};
    std::vector<Type> anyOf{};
  };

  struct ExclusiveMaximum {
    rfl::Flatten<Annotations> annotations{};
    rfl::Variant<double, int> exclusiveMaximum{};
    std::string type{};
  };

  struct ExclusiveMinimum {
    rfl::Flatten<Annotations> annotations{};
    rfl::Variant<double, int> exclusiveMinimum{};
    std::string type{};
  };

  struct FixedSizeTypedArray {
    Literal<"array"> type{};
    rfl::Flatten<Annotations> annotations{};
    rfl::Ref<Type> items{};
    size_t minItems{};
    size_t maxItems{};
  };

  struct Maximum {
    rfl::Flatten<Annotations> annotations{};
    rfl::Variant<double, int> maximum{};
    std::string type{};
  };

  struct Minimum {
    rfl::Flatten<Annotations> annotations{};
    rfl::Variant<double, int> minimum{};
    std::string type{};
  };

  struct Null {
    Literal<"null"> type{};
    rfl::Flatten<Annotations> annotations{};
  };

  struct Object {
    Literal<"object"> type{};
    rfl::Flatten<Annotations> annotations{};
    rfl::Object<Type> properties{};
    std::optional<std::vector<std::string>> required{};
    std::optional<rfl::Variant<std::shared_ptr<Type>, bool>>
        additionalProperties{};
  };

  struct OneOf {
    rfl::Flatten<Annotations> annotations{};
    std::vector<Type> oneOf{};
  };

  struct Reference {
    Rename<"$ref", std::optional<std::string>> ref{};
    rfl::Flatten<Annotations> annotations{};
  };

  struct Regex {
    Literal<"string"> type{};
    rfl::Flatten<Annotations> annotations{};
    std::string pattern{};
  };

  struct StringConst {
    rfl::Flatten<Annotations> annotations{};
    rfl::Rename<"const", std::string> value{};
  };

  struct StringEnum {
    Literal<"string"> type{};
    rfl::Flatten<Annotations> annotations{};
    rfl::Rename<"enum", std::vector<std::string>> values{};
  };

  struct StringMap {
    Literal<"object"> type{};
    rfl::Flatten<Annotations> annotations{};
    rfl::Ref<Type> additionalProperties{};
  };

  struct Tuple {
    Literal<"array"> type{};
    rfl::Flatten<Annotations> annotations{};
    std::vector<Type> prefixItems{};
    bool items = false;
    std::optional<size_t> minItems{};
    std::optional<size_t> maxItems{};
  };

  struct TypedArray {
    Literal<"array"> type{};
    rfl::Flatten<Annotations> annotations{};
    rfl::Ref<Type> items{};
    rfl::Rename<"minItems", std::optional<size_t>> minSize{};
    rfl::Rename<"maxItems", std::optional<size_t>> maxSize{};
  };

  using ReflectionType =
      rfl::Variant<AllOf, AnyOf, Boolean, ExclusiveMaximum, ExclusiveMinimum,
                   FixedSizeTypedArray, Integer, Maximum, Minimum, Number, Null,
                   Object, OneOf, Reference, Regex, String, StringConst,
                   StringEnum, StringMap, Tuple, TypedArray>;

  const auto& reflection() const { return value; }

  ReflectionType value;
};

}  // namespace rfl::json::schema

#endif
