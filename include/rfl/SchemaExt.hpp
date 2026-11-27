#ifndef RFL_SCHEMAEXT_HPP_
#define RFL_SCHEMAEXT_HPP_

#include <concepts>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Generic.hpp"
#include "Metadata.hpp"
#include "Object.hpp"
#include "Processors.hpp"

namespace rfl {

/// Optional Draft 2020-12 JSON Schema annotations (`title`, `examples`, and
/// arbitrary `x-*` extension properties) attachable via `rfl::SchemaExtension`,
/// `rfl::Metadata<T>`, or `rfl::FieldMeta`.
struct SchemaAnnotations {
  std::optional<std::string> title = std::nullopt;
  std::optional<std::vector<Generic>> examples = std::nullopt;
  Object<Generic> extensions = {};

  bool empty() const noexcept {
    return !title.has_value() &&
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
template <class T>
struct SchemaExtension {};

/// Specialize `rfl::DefaultIfMissingFor<T> : std::true_type {};` (or set
/// `rfl::Metadata<T>::default_if_missing = true`) to enable `DefaultIfMissing`
/// semantics (both read and schema defaults) for a specific struct `T` without
/// forcing global `rfl::DefaultIfMissing` onto the entire schema graph.
template <class T>
struct DefaultIfMissingFor : std::false_type {};

/// Opt-in JSON Schema processor: emit `minimum` and `maximum` bounds matching
/// C++ integer widths (`int8_t`..`int64_t`, `uint8_t`..`uint64_t`).
struct IntegerBounds {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

/// Opt-in JSON Schema processor: emit `minItems` and `maxItems` equal to
/// `prefixItems.size()` on fixed-arity tuple schemas.
struct TupleBounds {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

/// Opt-in JSON Schema processor: omit `"required": []` on object schemas that
/// have no required properties.
struct OmitEmptyRequired {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

namespace internal {

template <class ProcessorsType>
struct has_integer_bounds : std::false_type {};

template <>
struct has_integer_bounds<Processors<>> : std::false_type {};

template <class Head, class... Tail>
struct has_integer_bounds<Processors<Head, Tail...>>
    : std::bool_constant<std::is_same_v<Head, IntegerBounds> ||
                         has_integer_bounds<Processors<Tail...>>::value> {};

template <class ProcessorsType>
constexpr bool integer_bounds_v =
    has_integer_bounds<std::remove_cvref_t<ProcessorsType>>::value;

template <class ProcessorsType>
struct has_tuple_bounds : std::false_type {};

template <>
struct has_tuple_bounds<Processors<>> : std::false_type {};

template <class Head, class... Tail>
struct has_tuple_bounds<Processors<Head, Tail...>>
    : std::bool_constant<std::is_same_v<Head, TupleBounds> ||
                         has_tuple_bounds<Processors<Tail...>>::value> {};

template <class ProcessorsType>
constexpr bool tuple_bounds_v =
    has_tuple_bounds<std::remove_cvref_t<ProcessorsType>>::value;

template <class ProcessorsType>
struct has_omit_empty_required : std::false_type {};

template <>
struct has_omit_empty_required<Processors<>> : std::false_type {};

template <class Head, class... Tail>
struct has_omit_empty_required<Processors<Head, Tail...>>
    : std::bool_constant<std::is_same_v<Head, OmitEmptyRequired> ||
                         has_omit_empty_required<Processors<Tail...>>::value> {};

template <class ProcessorsType>
constexpr bool omit_empty_required_v =
    has_omit_empty_required<std::remove_cvref_t<ProcessorsType>>::value;

// --- SchemaName detection (SchemaName<T> or Metadata<T>::schema_name) ---

template <class T>
concept HasSchemaNameTraitVar = requires {
  { SchemaName<T>::value } -> std::convertible_to<std::string_view>;
};

template <class T>
concept HasSchemaNameTraitFn = requires {
  { SchemaName<T>::value() } -> std::convertible_to<std::string_view>;
};

template <class T>
concept HasMetadataSchemaNameVar = requires {
  { Metadata<T>::schema_name } -> std::convertible_to<std::string_view>;
};

template <class T>
concept HasMetadataSchemaNameFn = requires {
  { Metadata<T>::schema_name() } -> std::convertible_to<std::string_view>;
};

template <class T>
constexpr bool has_schema_name_v =
    HasSchemaNameTraitVar<std::remove_cvref_t<T>> ||
    HasSchemaNameTraitFn<std::remove_cvref_t<T>> ||
    HasMetadataSchemaNameVar<std::remove_cvref_t<T>> ||
    HasMetadataSchemaNameFn<std::remove_cvref_t<T>>;

template <class T>
inline std::string get_schema_name() {
  using U = std::remove_cvref_t<T>;
  if constexpr (HasSchemaNameTraitFn<U>) {
    return std::string(std::string_view(SchemaName<U>::value()));
  } else if constexpr (HasSchemaNameTraitVar<U>) {
    return std::string(std::string_view(SchemaName<U>::value));
  } else if constexpr (HasMetadataSchemaNameFn<U>) {
    return std::string(std::string_view(Metadata<U>::schema_name()));
  } else if constexpr (HasMetadataSchemaNameVar<U>) {
    return std::string(std::string_view(Metadata<U>::schema_name));
  } else {
    return {};
  }
}

// --- Per-struct DefaultIfMissing detection ---

template <class T>
concept HasMetadataDefaultIfMissing = requires {
  { Metadata<T>::default_if_missing } -> std::convertible_to<bool>;
};

template <class T>
consteval bool struct_default_if_missing_opt_in() {
  using U = std::remove_cvref_t<T>;
  if constexpr (HasMetadataDefaultIfMissing<U>) {
    return static_cast<bool>(Metadata<U>::default_if_missing);
  } else {
    return DefaultIfMissingFor<U>::value;
  }
}

template <class T>
constexpr bool struct_default_if_missing_v =
    struct_default_if_missing_opt_in<T>();

// --- SchemaExtension & Metadata struct-level annotations ---

template <class T>
concept HasSchemaExtensionAnnotationsFn = requires {
  { SchemaExtension<T>::annotations() } -> std::convertible_to<SchemaAnnotations>;
};

template <class T>
concept HasSchemaExtensionExtensionsFn = requires {
  { SchemaExtension<T>::extensions() } -> std::convertible_to<Object<Generic>>;
};

template <class T>
concept HasMetadataTitleVar = requires {
  { Metadata<T>::title } -> std::convertible_to<std::string_view>;
};

template <class T>
concept HasMetadataTitleFn = requires {
  { Metadata<T>::title() } -> std::convertible_to<std::string_view>;
};

template <class T>
concept HasMetadataExamplesFn = requires {
  { Metadata<T>::examples() } -> std::convertible_to<std::vector<Generic>>;
};

template <class T>
concept HasMetadataExtensionsFn = requires {
  { Metadata<T>::extensions() } -> std::convertible_to<Object<Generic>>;
};

template <class T>
concept HasMetadataFieldAnnotationsFn = requires {
  { Metadata<T>::field_annotations() }
      -> std::convertible_to<Object<SchemaAnnotations>>;
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
constexpr bool schema_extension_inline_v = schema_extension_inline<T>();

template <class T>
constexpr bool has_schema_annotations_v =
    HasSchemaExtensionAnnotationsFn<std::remove_cvref_t<T>> ||
    HasSchemaExtensionExtensionsFn<std::remove_cvref_t<T>> ||
    HasMetadataTitleVar<std::remove_cvref_t<T>> ||
    HasMetadataTitleFn<std::remove_cvref_t<T>> ||
    HasMetadataExamplesFn<std::remove_cvref_t<T>> ||
    HasMetadataExtensionsFn<std::remove_cvref_t<T>>;

template <class T>
inline SchemaAnnotations get_schema_annotations() {
  using U = std::remove_cvref_t<T>;
  SchemaAnnotations out;
  if constexpr (HasMetadataTitleFn<U>) {
    auto t = std::string(std::string_view(Metadata<U>::title()));
    if (!t.empty()) out.title = std::move(t);
  } else if constexpr (HasMetadataTitleVar<U>) {
    auto t = std::string(std::string_view(Metadata<U>::title));
    if (!t.empty()) out.title = std::move(t);
  }
  if constexpr (HasMetadataExamplesFn<U>) {
    auto ex = Metadata<U>::examples();
    if (!ex.empty()) out.examples = std::move(ex);
  }
  if constexpr (HasMetadataExtensionsFn<U>) {
    for (const auto& [k, v] : Metadata<U>::extensions()) {
      out.extensions[k] = v;
    }
  }
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

template <class T>
inline Object<SchemaAnnotations> get_field_schema_annotations() {
  using U = std::remove_cvref_t<T>;
  if constexpr (HasMetadataFieldAnnotationsFn<U>) {
    return Metadata<U>::field_annotations();
  } else {
    return {};
  }
}

}  // namespace internal
}  // namespace rfl

#endif
