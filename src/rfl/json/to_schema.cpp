/*

MIT License

Copyright (c) 2023-2024 Code17 GmbH

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "rfl/json/to_schema.hpp"

#include "rfl/json/schema/Type.hpp"
#include "rfl/json/write.hpp"

namespace rfl::json {

schema::Type type_to_json_schema_type(const parsing::schema::Type& _type,
                                      const SchemaRenderOptions& _options);

bool is_optional(const parsing::schema::Type& _t) {
  return _t.variant_.visit([&](const auto& _v) -> bool {
    using T = std::remove_cvref_t<decltype(_v)>;
    if constexpr (std::is_same_v<T, parsing::schema::Type::Deprecated>) {
      return is_optional(*_v.type_);

    } else if constexpr (std::is_same_v<T,
                                        parsing::schema::Type::Description>) {
      return is_optional(*_v.type_);

    } else if constexpr (std::is_same_v<T, parsing::schema::Type::Annotated>) {
      return is_optional(*_v.type_);

    } else if constexpr (std::is_same_v<T, parsing::schema::Type::Validated>) {
      return is_optional(*_v.type_);

    } else {
      return std::is_same_v<T, parsing::schema::Type::Optional> ||
             std::is_same_v<T, parsing::schema::Type::DefaultVal>;
    }
  });
}

std::string numeric_type_to_string(const parsing::schema::Type& _type) {
  const auto handle_variant = [](const auto& _t) -> std::string {
    using T = std::remove_cvref_t<decltype(_t)>;
    using Type = parsing::schema::Type;
    if constexpr (std::is_same<T, Type::Int32>() ||
                  std::is_same<T, Type::Int64>() ||
                  std::is_same<T, Type::UInt32>() ||
                  std::is_same<T, Type::UInt64>() ||
                  std::is_same<T, Type::Integer>() ||
                  std::is_same<T, Type::BoundedInteger>()) {
      return schema::Type::Integer{}.type.str();
    } else {
      return schema::Type::Number{}.type.str();
    }
  };
  return rfl::visit(handle_variant, _type.variant_);
}

schema::Type handle_validation_type(
    const parsing::schema::Type& _type,
    const parsing::schema::ValidationType& _validation_type,
    const SchemaRenderOptions& _options) {
  auto handle_variant = [&](const auto& _v) -> schema::Type {
    using T = std::remove_cvref_t<decltype(_v)>;
    using ValidationType = parsing::schema::ValidationType;
    if constexpr (std::is_same<T, ValidationType::AllOf>()) {
      auto all_of = std::vector<schema::Type>();
      for (const auto& t : _v.types_) {
        all_of.emplace_back(handle_validation_type(_type, t, _options));
      }
      return schema::Type{.value = schema::Type::AllOf{.allOf = all_of}};

    } else if constexpr (std::is_same<T, ValidationType::AnyOf>()) {
      auto any_of = std::vector<schema::Type>();
      for (const auto& t : _v.types_) {
        any_of.emplace_back(handle_validation_type(_type, t, _options));
      }
      return schema::Type{.value = schema::Type::AnyOf{.anyOf = any_of}};

    } else if constexpr (std::is_same<T, ValidationType::OneOf>()) {
      auto one_of = std::vector<schema::Type>();
      for (const auto& t : _v.types_) {
        one_of.emplace_back(handle_validation_type(_type, t, _options));
      }
      return schema::Type{.value = schema::Type::OneOf{.oneOf = one_of}};

    } else if constexpr (std::is_same<T, ValidationType::Regex>()) {
      return schema::Type{.value = schema::Type::Regex{.pattern = _v.pattern_}};

    } else if constexpr (std::is_same<T, ValidationType::Size>()) {
      auto t = type_to_json_schema_type(_type, _options);
      // Size validates decoded bytes, not the length of the Base64 text.
      if (const auto* str = rfl::get_if<schema::Type::String>(&t.value);
          str && str->contentEncoding) {
        return t;
      }
      const auto to_size = [](const auto _val) {
        return static_cast<size_t>(_val);
      };
      auto handle_size_variant = [&](auto& _t, const auto& _size_limit) {
        using U = std::remove_cvref_t<decltype(_t)>;
        using V = std::remove_cvref_t<decltype(_size_limit)>;
        if constexpr (std::is_same<U, schema::Type::TypedArray>() ||
                      std::is_same<U, schema::Type::String>()) {
          if constexpr (std::is_same<V, ValidationType::Minimum>()) {
            _t.minSize = _size_limit.value_.visit(to_size);
            return t;

          } else if constexpr (std::is_same<V, ValidationType::Maximum>()) {
            _t.maxSize = _size_limit.value_.visit(to_size);
            return t;

          } else if constexpr (std::is_same<V, ValidationType::EqualTo>()) {
            _t.minSize = _size_limit.value_.visit(to_size);
            _t.maxSize = _size_limit.value_.visit(to_size);
            return t;

          } else if constexpr (std::is_same<V, ValidationType::AnyOf>() ||
                               std::is_same<V, ValidationType::AllOf>()) {
            V v;
            for (const auto& limiter : _size_limit.types_) {
              v.types_.push_back(ValidationType{ValidationType::Size{
                  .size_limit_ = rfl::Ref<ValidationType>::make(limiter)}});
            }
            return handle_validation_type(_type, ValidationType{.variant_ = v},
                                          _options);
          }
        }
        return t;
      };

      return rfl::visit(handle_size_variant, t.value, _v.size_limit_->variant_);

    } else if constexpr (std::is_same<T, ValidationType::ExclusiveMaximum>()) {
      return schema::Type{.value = schema::Type::ExclusiveMaximum{
                              .exclusiveMaximum = _v.value_,
                              .type = numeric_type_to_string(_type)}};

    } else if constexpr (std::is_same<T, ValidationType::ExclusiveMinimum>()) {
      return schema::Type{.value = schema::Type::ExclusiveMinimum{
                              .exclusiveMinimum = _v.value_,
                              .type = numeric_type_to_string(_type)}};

    } else if constexpr (std::is_same<T, ValidationType::Maximum>()) {
      return schema::Type{
          .value = schema::Type::Maximum{
              .maximum = _v.value_, .type = numeric_type_to_string(_type)}};

    } else if constexpr (std::is_same<T, ValidationType::Minimum>()) {
      return schema::Type{
          .value = schema::Type::Minimum{
              .minimum = _v.value_, .type = numeric_type_to_string(_type)}};

    } else if constexpr (std::is_same<T, ValidationType::EqualTo>()) {
      const auto maximum = schema::Type{
          .value = schema::Type::Maximum{
              .maximum = _v.value_, .type = numeric_type_to_string(_type)}};
      const auto minimum = schema::Type{
          .value = schema::Type::Minimum{
              .minimum = _v.value_, .type = numeric_type_to_string(_type)}};
      return schema::Type{.value =
                              schema::Type::AllOf{.allOf = {maximum, minimum}}};

    } else if constexpr (std::is_same<T, ValidationType::NotEqualTo>()) {
      const auto excl_maximum =
          schema::Type{.value = schema::Type::ExclusiveMaximum{
                           .exclusiveMaximum = _v.value_,
                           .type = numeric_type_to_string(_type)}};
      const auto excl_minimum =
          schema::Type{.value = schema::Type::ExclusiveMinimum{
                           .exclusiveMinimum = _v.value_,
                           .type = numeric_type_to_string(_type)}};
      return schema::Type{
          .value = schema::Type::AnyOf{.anyOf = {excl_maximum, excl_minimum}}};

    } else {
      static_assert(rfl::always_false_v<T>, "Not all cases were covered.");
    }
  };

  return rfl::visit(handle_variant, _validation_type.variant_);
}

schema::Type type_to_json_schema_type(const parsing::schema::Type& _type,
                                      const SchemaRenderOptions& _options) {
  auto handle_variant = [&](const auto& _t) -> schema::Type {
    using T = std::remove_cvref_t<decltype(_t)>;
    using Type = parsing::schema::Type;
    if constexpr (std::is_same<T, Type::Boolean>()) {
      return schema::Type{.value = schema::Type::Boolean{}};

    } else if constexpr (std::is_same<T, Type::BoundedInteger>()) {
      return schema::Type{.value = schema::Type::Integer{
                              .minimum = _t.minimum_, .maximum = _t.maximum_}};

    } else if constexpr (std::is_same<T, Type::Int32>() ||
                         std::is_same<T, Type::Int64>() ||
                         std::is_same<T, Type::UInt32>() ||
                         std::is_same<T, Type::UInt64>() ||
                         std::is_same<T, Type::Integer>()) {
      return schema::Type{.value = schema::Type::Integer{}};

    } else if constexpr (std::is_same<T, Type::Float>() ||
                         std::is_same<T, Type::Double>()) {
      return schema::Type{.value = schema::Type::Number{}};

    } else if constexpr (std::is_same<T, Type::String>() ||
                         std::is_same<T, Type::Vectorstring>()) {
      return schema::Type{.value = schema::Type::String{}};

    } else if constexpr (std::is_same<T, Type::Bytestring>()) {
      return schema::Type{
          .value = schema::Type::String{.contentEncoding = "base64"}};

    } else if constexpr (std::is_same<T, Type::AnyOf>()) {
      auto any_of = std::vector<schema::Type>();
      for (const auto& t : _t.types_) {
        any_of.emplace_back(type_to_json_schema_type(t, _options));
      }
      if (_t.mutually_exclusive_) {
        return schema::Type{.value = schema::Type::OneOf{.oneOf = any_of}};
      }
      return schema::Type{.value = schema::Type::AnyOf{.anyOf = any_of}};

    } else if constexpr (std::is_same<T, Type::Annotated>()) {
      auto res = type_to_json_schema_type(*_t.type_, _options);
      const auto update_annotations = [&](auto _v) -> schema::Type {
        if (_t.title_ && !_t.title_->empty()) {
          _v.annotations.value_.title = *_t.title_;
        }
        if (_t.examples_ && !_t.examples_->empty()) {
          _v.annotations.value_.examples = *_t.examples_;
        }
        for (const auto& [k, val] : _t.extensions_) {
          _v.annotations.value_.extensions[k] = val;
        }
        return schema::Type{std::move(_v)};
      };
      return rfl::visit(update_annotations, std::move(res.value));

    } else if constexpr (std::is_same<T, Type::DefaultVal>()) {
      auto res = type_to_json_schema_type(*_t.type_, _options);
      if (!_t.default_value_.has_value()) {
        return res;
      }
      const auto update_prediction = [&](auto _v) -> schema::Type {
        _v.annotations.value_.defaultValue = *_t.default_value_;
        return schema::Type{_v};
      };
      return rfl::visit(update_prediction, res.value);

    } else if constexpr (std::is_same<T, Type::Deprecated>()) {
      auto res = type_to_json_schema_type(*_t.type_, _options);
      const auto update_prediction = [&](auto _v) -> schema::Type {
        if (!_t.description_.empty() ||
            !_v.annotations.value_.description.has_value()) {
          _v.annotations.value_.description = _t.description_;
        }
        _v.annotations.value_.deprecated = true;
        _v.annotations.value_.deprecationMessage = _t.deprecation_message_;
        return schema::Type{_v};
      };
      return rfl::visit(update_prediction, res.value);

    } else if constexpr (std::is_same<T, Type::Description>()) {
      auto res = type_to_json_schema_type(*_t.type_, _options);
      const auto update_prediction = [&](auto _v) -> schema::Type {
        _v.annotations.value_.description = _t.description_;
        return schema::Type{_v};
      };
      return rfl::visit(update_prediction, res.value);

    } else if constexpr (std::is_same<T, Type::FixedSizeTypedArray>()) {
      return schema::Type{
          .value = schema::Type::FixedSizeTypedArray{
              .items = Ref<schema::Type>::make(
                  type_to_json_schema_type(*_t.type_, _options)),
              .minItems = _t.size_,
              .maxItems = _t.size_}};

    } else if constexpr (std::is_same<T, Type::Literal>()) {
      return schema::Type{.value =
                              schema::Type::StringEnum{.values = _t.values_}};

    } else if constexpr (std::is_same<T, Type::DescribedLiteral>()) {
      // Convert to OneOf with StringConst for each described value
      auto one_of = std::vector<schema::Type>();
      for (const auto& v : _t.values_) {
        one_of.push_back(schema::Type{
            .value = schema::Type::StringConst{
                .annotations =
                    schema::Type::Annotations{
                        .description =
                            v.description_.empty()
                                ? std::nullopt
                                : std::optional<std::string>(v.description_)},
                .value = v.value_}});
      }
      return schema::Type{.value = schema::Type::OneOf{.oneOf = one_of}};

    } else if constexpr (std::is_same<T, Type::Object>()) {
      auto properties = rfl::Object<schema::Type>();
      auto required = std::vector<std::string>();
      for (const auto& [k, v] : _t.types_) {
        properties[k] = type_to_json_schema_type(v, _options);
        if (!is_optional(v) && !_options.no_required) {
          required.push_back(k);
        }
      }
      auto additional_properties = rfl::visit(
          [&](const auto& _a)
              -> std::optional<
                  rfl::Variant<std::shared_ptr<schema::Type>, bool>> {
            using A = std::remove_cvref_t<decltype(_a)>;
            if constexpr (std::is_same<A, bool>()) {
              return _a;
            } else {
              if (!_a) {
                return std::nullopt;
              }
              return std::make_shared<schema::Type>(
                  type_to_json_schema_type(*_a, _options));
            }
          },
          _t.additional_properties_);
      std::optional<std::vector<std::string>> req_field =
          (_options.omit_empty_required && required.empty())
              ? std::nullopt
              : std::optional<std::vector<std::string>>(std::move(required));
      return schema::Type{.value = schema::Type::Object{
                              .properties = properties,
                              .required = std::move(req_field),
                              .additionalProperties = additional_properties}};

    } else if constexpr (std::is_same<T, Type::Optional>()) {
      auto inner = type_to_json_schema_type(*_t.type_, _options);
      if (auto* any_of = rfl::get_if<schema::Type::AnyOf>(&inner.value);
          any_of && !any_of->annotations.value_.title &&
          !any_of->annotations.value_.description &&
          !any_of->annotations.value_.deprecated &&
          !any_of->annotations.value_.deprecationMessage &&
          !any_of->annotations.value_.defaultValue.value().has_value() &&
          !any_of->annotations.value_.examples &&
          any_of->annotations.value_.extensions.empty()) {
        bool has_null = false;
        for (const auto& branch : any_of->anyOf) {
          if (rfl::holds_alternative<schema::Type::Null>(branch.value)) {
            has_null = true;
            break;
          }
        }
        if (!has_null) {
          any_of->anyOf.emplace_back(schema::Type{schema::Type::Null{}});
        }
        return inner;
      }
      return schema::Type{
          .value = schema::Type::AnyOf{
              .anyOf = {std::move(inner), schema::Type{schema::Type::Null{}}}}};

    } else if constexpr (std::is_same<T, Type::Reference>()) {
      return schema::Type{
          .value = schema::Type::Reference{.ref = "#/$defs/" + _t.name_}};

    } else if constexpr (std::is_same<T, Type::StringMap>()) {
      return schema::Type{
          .value = schema::Type::StringMap{
              .additionalProperties = Ref<schema::Type>::make(
                  type_to_json_schema_type(*_t.value_type_, _options))}};

    } else if constexpr (std::is_same<T, Type::Tuple>()) {
      auto items = std::vector<schema::Type>();
      for (const auto& t : _t.types_) {
        items.emplace_back(type_to_json_schema_type(t, _options));
      }
      auto tuple_schema = schema::Type::Tuple{.prefixItems = items};
      if (_options.tuple_bounds) {
        tuple_schema.minItems = items.size();
        tuple_schema.maxItems = items.size();
      }
      return schema::Type{.value = std::move(tuple_schema)};

    } else if constexpr (std::is_same<T, Type::TypedArray>()) {
      return schema::Type{
          .value = schema::Type::TypedArray{
              .items = Ref<schema::Type>::make(
                  type_to_json_schema_type(*_t.type_, _options))}};

    } else if constexpr (std::is_same<T, Type::Validated>()) {
      return handle_validation_type(*_t.type_, _t.validation_, _options);

    } else {
      static_assert(rfl::always_false_v<T>, "Not all cases were covered.");
    }
  };

  return rfl::visit(handle_variant, _type.variant_);
}

std::string to_schema_internal_schema(
    const parsing::schema::Definition& internal_schema,
    const yyjson_write_flag _flag, const SchemaRenderOptions& _options,
    const std::string& comment) {
  auto definitions = std::map<std::string, schema::Type>();
  for (const auto& [k, v] : internal_schema.definitions_) {
    definitions[k] = type_to_json_schema_type(v, _options);
  }
  using JSONSchemaType =
      typename TypeHelper<schema::Type::ReflectionType>::JSONSchemaType;
  const auto to_schema = [&](auto&& _root) -> JSONSchemaType {
    using U = std::decay_t<decltype(_root)>;
    return schema::JSONSchema<U>{
        .comment =
            !comment.empty() ? std::optional(std::move(comment)) : std::nullopt,
        .root = std::forward<decltype(_root)>(_root),
        .definitions = definitions,
    };
  };
  auto root = type_to_json_schema_type(internal_schema.root_, _options);
  const auto json_schema = rfl::visit(to_schema, std::move(root.value));
  return write(json_schema, _flag);
}

std::string to_schema_internal_schema(
    const parsing::schema::Definition& internal_schema,
    const yyjson_write_flag _flag, const bool _no_required,
    const std::string& comment) {
  return to_schema_internal_schema(
      internal_schema, _flag,
      SchemaRenderOptions{.no_required = _no_required}, comment);
}

}  // namespace rfl::json
