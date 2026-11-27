#ifndef RFL_SCHEMAEXT_HPP_
#define RFL_SCHEMAEXT_HPP_

#include <concepts>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "Generic.hpp"
#include "Object.hpp"
#include "Processors.hpp"

namespace rfl {

/// Optional Draft 2020-12 JSON Schema annotations (`title`, `examples`, and
/// arbitrary `x-*` extension properties) attachable via `rfl::SchemaExtension`.
struct SchemaAnnotations {
  std::optional<std::string> title = std::nullopt;
  std::optional<std::vector<Generic>> examples = std::nullopt;
  Object<Generic> extensions = {};

  bool empty() const noexcept {
    return (!title.has_value() || title->empty()) &&
           (!examples.has_value() || examples->empty()) &&
           extensions.empty();
  }
};

/// Specialize `rfl::SchemaName<T>` to override the `$defs` identifier emitted
/// for type `T` (instead of the default mangled C++ type name).
///
/// Supported members:
/// - `static constexpr std::string_view value = "ExportConfig";` OR
/// - `static std::string_view value();` (or returning `std::string`).
template <class T>
struct SchemaName {};

/// Specialize `rfl::SchemaExtension<T>` on a custom wrapper type (such as
/// `ConfigOption<T>`) or struct to attach `title`, `examples`, and arbitrary
/// `x-*` extension keywords to its generated JSON Schema node.
///
/// Supported members:
/// - `static rfl::SchemaAnnotations annotations();` AND/OR
/// - `static rfl::Object<rfl::Generic> extensions();`
/// - `static constexpr bool inline_schema = true;` (optional, for reflection
///   wrappers that should emit their schema inline rather than via `$defs`).
template <class T>
struct SchemaExtension {};

/// Opt-in JSON Schema processor: emit `minimum` and `maximum` bounds matching
/// C++ integer widths (`int8_t`..`int64_t`, `uint8_t`..`uint64_t`).
struct IntegerBounds {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return std::forward<decltype(_named_tuple)>(_named_tuple);
  }
};

/// Opt-in JSON Schema processor: emit `minItems` and `maxItems` equal to
/// `prefixItems.size()` on fixed-arity tuple schemas.
struct TupleBounds {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return std::forward<decltype(_named_tuple)>(_named_tuple);
  }
};

/// Opt-in JSON Schema processor: omit `"required": []` on object schemas that
/// have no required properties.
struct OmitEmptyRequired {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return std::forward<decltype(_named_tuple)>(_named_tuple);
  }
};

namespace internal {

template <class Tag, class T>
struct has_schema_processor : std::false_type {};

template <class Tag>
struct has_schema_processor<Tag, Tag> : std::true_type {};

template <class Tag, class... Ps>
struct has_schema_processor<Tag, Processors<Ps...>>
    : std::bool_constant<(has_schema_processor<Tag, Ps>::value || ...)> {};

template <class ProcessorsType>
inline constexpr bool integer_bounds_v =
    has_schema_processor<IntegerBounds,
                         std::remove_cvref_t<ProcessorsType>>::value;

template <class ProcessorsType>
inline constexpr bool tuple_bounds_v =
    has_schema_processor<TupleBounds,
                         std::remove_cvref_t<ProcessorsType>>::value;

template <class ProcessorsType>
inline constexpr bool omit_empty_required_v =
    has_schema_processor<OmitEmptyRequired,
                         std::remove_cvref_t<ProcessorsType>>::value;

// --- SchemaName detection ---

template <class T>
concept HasSchemaNameTraitVar = requires {
  { SchemaName<T>::value } -> std::convertible_to<std::string_view>;
};

template <class T>
concept HasSchemaNameTraitFn = requires {
  { SchemaName<T>::value() } -> std::convertible_to<std::string_view>;
};

template <class T>
inline constexpr bool has_schema_name_v =
    HasSchemaNameTraitVar<std::remove_cvref_t<T>> ||
    HasSchemaNameTraitFn<std::remove_cvref_t<T>>;

template <class T>
inline std::string get_schema_name() {
  using U = std::remove_cvref_t<T>;
  if constexpr (HasSchemaNameTraitFn<U>) {
    return std::string(std::string_view(SchemaName<U>::value()));
  } else if constexpr (HasSchemaNameTraitVar<U>) {
    return std::string(std::string_view(SchemaName<U>::value));
  } else {
    return {};
  }
}

// --- SchemaExtension annotations ---

template <class T>
concept HasSchemaExtensionAnnotationsFn = requires {
  { SchemaExtension<T>::annotations() } -> std::convertible_to<SchemaAnnotations>;
};

template <class T>
concept HasSchemaExtensionExtensionsFn = requires {
  { SchemaExtension<T>::extensions() } -> std::convertible_to<Object<Generic>>;
};

template <class T>
concept HasSchemaExtensionInlineFlag = requires {
  { SchemaExtension<T>::inline_schema } -> std::convertible_to<bool>;
};

template <class T>
consteval bool schema_extension_inline() {
  using U = std::remove_cvref_t<T>;
  if constexpr (HasSchemaExtensionInlineFlag<U>) {
    return static_cast<bool>(SchemaExtension<U>::inline_schema);
  } else {
    return false;
  }
}

template <class T>
inline constexpr bool schema_extension_inline_v = schema_extension_inline<T>();

template <class T>
inline constexpr bool has_schema_annotations_v =
    HasSchemaExtensionAnnotationsFn<std::remove_cvref_t<T>> ||
    HasSchemaExtensionExtensionsFn<std::remove_cvref_t<T>>;

template <class T>
inline SchemaAnnotations get_schema_annotations() {
  using U = std::remove_cvref_t<T>;
  SchemaAnnotations out;
  if constexpr (HasSchemaExtensionAnnotationsFn<U>) {
    auto ext = SchemaExtension<U>::annotations();
    if (ext.title && !ext.title->empty()) out.title = std::move(ext.title);
    if (ext.examples && !ext.examples->empty()) {
      out.examples = std::move(ext.examples);
    }
    for (const auto& [k, v] : ext.extensions) {
      out.extensions[k] = v;
    }
  }
  if constexpr (HasSchemaExtensionExtensionsFn<U>) {
    for (const auto& [k, v] : SchemaExtension<U>::extensions()) {
      out.extensions[k] = v;
    }
  }
  return out;
}

}  // namespace internal
}  // namespace rfl

#endif
