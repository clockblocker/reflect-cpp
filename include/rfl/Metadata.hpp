#ifndef RFL_METADATA_HPP_
#define RFL_METADATA_HPP_

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

namespace rfl {

/// Out-of-band per-field metadata (description and optional deprecation) that
/// attaches schema/documentation prose to a reflected struct WITHOUT embedding
/// long `StringLiteral` non-type template parameters into C++ field types.
struct FieldMeta {
  /// Processed wire field name, or pre-processor field name (C++ member name,
  /// or explicit `rfl::Rename` name when renamed). Exact wire-name matches
  /// take precedence over pre-processor names.
  std::string_view name;

  /// Optional human-readable description emitted in generated schemas.
  std::string_view description = {};

  /// Optional deprecation message. When non-empty, marks the field deprecated
  /// in generated schemas (with `description`, matching `rfl::Deprecated`).
  std::string_view deprecation_message = {};
};

/// Specialize `rfl::Metadata<T>` to attach zero-bloat out-of-band metadata to
/// struct `T` without wrapping its fields in `rfl::Description` /
/// `rfl::Deprecated`.
///
/// Supported members on a specialization `rfl::Metadata<MyStruct>`:
/// - `static constexpr std::array<rfl::FieldMeta, N> fields = ...;`
///   (or any contiguous collection / `std::span<const rfl::FieldMeta>`), OR
/// - `static std::span<const rfl::FieldMeta> fields();` (or returning a
///   `const` lvalue reference to a contiguous collection; may be defined
///   out-of-line in a `.cpp` file for complete compile-time isolation).
/// - Optional struct-level description:
///   `static constexpr std::string_view description = "...";` (or a non-null
///   `const char*`) or `static std::string_view description();`
///
/// Notes:
/// - `rfl::Metadata<Inner>` attaches to `Inner`'s own schema definition
///   (`$defs/Inner`); fields flattened via `rfl::Flatten<Inner>` are looked up
///   on the enclosing struct's `rfl::Metadata<Outer>`.
/// - For `rfl::TaggedUnion` alternatives wrapped in an internal tagged-union
///   wrapper, `rfl::Metadata<Alternative>` is consulted automatically.
/// - For custom types with `ReflectionType` or `rfl::Reflector<T>`,
///   struct-level `Metadata<T>::description` is honored; `Metadata<T>::fields`
///   applies to reflected aggregate structs.
template <class T>
struct Metadata {};

namespace internal {

template <class T>
struct is_field_meta_span : std::false_type {};

template <std::size_t _extent>
struct is_field_meta_span<std::span<const FieldMeta, _extent>>
    : std::true_type {};

template <std::size_t _extent>
struct is_field_meta_span<std::span<FieldMeta, _extent>> : std::true_type {};

template <class T>
struct is_field_meta_c_array : std::false_type {};

template <std::size_t _n>
struct is_field_meta_c_array<FieldMeta[_n]> : std::true_type {};

template <std::size_t _n>
struct is_field_meta_c_array<const FieldMeta[_n]> : std::true_type {};

template <class R>
concept ContiguousFieldMetaCollection =
    is_field_meta_c_array<std::remove_reference_t<R>>::value ||
    requires(const std::remove_reference_t<R>& _r) {
      { _r.data() };
      { _r.size() };
      requires std::is_pointer_v<decltype(_r.data())>;
      requires std::is_same_v<
          std::remove_cv_t<std::remove_pointer_t<decltype(_r.data())>>,
          FieldMeta>;
      requires std::is_convertible_v<decltype(_r.size()), std::size_t>;
    };

template <class R>
constexpr std::span<const FieldMeta> to_field_meta_span(const R& _r) noexcept {
  if constexpr (is_field_meta_c_array<std::remove_reference_t<R>>::value) {
    return std::span<const FieldMeta>(
        _r, std::extent_v<std::remove_reference_t<R>>);
  } else {
    return std::span<const FieldMeta>(_r.data(),
                                      static_cast<std::size_t>(_r.size()));
  }
}

template <class T>
concept HasFieldMetaVar = requires {
  { Metadata<T>::fields } -> ContiguousFieldMetaCollection;
  requires !std::is_member_object_pointer_v<decltype(&Metadata<T>::fields)>;
};

template <class T>
concept HasAnyFieldMetaFn = requires {
  { Metadata<T>::fields() } -> ContiguousFieldMetaCollection;
};

template <class T>
concept HasFieldMetaFn =
    HasAnyFieldMetaFn<T> &&
    (std::is_lvalue_reference_v<decltype(Metadata<T>::fields())> ||
     is_field_meta_span<
         std::remove_cvref_t<decltype(Metadata<T>::fields())>>::value);

template <class T>
constexpr bool has_field_metadata_v = HasFieldMetaVar<std::remove_cvref_t<T>> ||
                                      HasAnyFieldMetaFn<std::remove_cvref_t<T>>;

template <class T>
std::span<const FieldMeta> get_field_metadata() {
  using U = std::remove_cvref_t<T>;
  static_assert(
      !HasAnyFieldMetaFn<U> || HasFieldMetaFn<U>,
      "rfl::Metadata<T>::fields() must return a borrowed contiguous range "
      "(for example, std::span<const rfl::FieldMeta> or const auto&) so the "
      "returned view does not dangle.");
  if constexpr (HasFieldMetaFn<U>) {
    auto&& r = Metadata<U>::fields();
    return to_field_meta_span(r);
  } else if constexpr (HasFieldMetaVar<U>) {
    return to_field_meta_span(Metadata<U>::fields);
  } else {
    return {};
  }
}

template <class T>
concept HasStructDescriptionVar = requires {
  Metadata<T>::description;
  requires std::is_convertible_v<decltype(Metadata<T>::description),
                                 std::string_view>;
  requires !std::is_member_object_pointer_v<
      decltype(&Metadata<T>::description)>;
};

template <class T>
concept HasStructDescriptionFn = requires {
  Metadata<T>::description();
  requires std::is_convertible_v<decltype(Metadata<T>::description()),
                                 std::string_view>;
};

template <class T>
constexpr bool has_struct_description_v =
    HasStructDescriptionVar<std::remove_cvref_t<T>> ||
    HasStructDescriptionFn<std::remove_cvref_t<T>>;

template <class T>
std::string get_struct_description() {
  using U = std::remove_cvref_t<T>;
  if constexpr (HasStructDescriptionFn<U>) {
    return std::string(std::string_view(Metadata<U>::description()));
  } else if constexpr (HasStructDescriptionVar<U>) {
    return std::string(std::string_view(Metadata<U>::description));
  } else {
    return {};
  }
}

inline std::optional<FieldMeta> find_field_metadata(
    const std::span<const FieldMeta> _meta, const std::string_view _wire_name,
    const std::string_view _raw_name = {}) noexcept {
  for (const auto& entry : _meta) {
    if (entry.name == _wire_name) {
      return entry;
    }
  }
  if (!_raw_name.empty() && _raw_name != _wire_name) {
    for (const auto& entry : _meta) {
      if (entry.name == _raw_name) {
        return entry;
      }
    }
  }
  return std::nullopt;
}

}  // namespace internal
}  // namespace rfl

#endif
