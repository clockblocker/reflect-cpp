#ifndef RFL_METADATA_HPP_
#define RFL_METADATA_HPP_

#include <concepts>
#include <optional>
#include <ranges>
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
  /// in generated schemas (alongside `description`, matching `rfl::Deprecated`).
  std::string_view deprecation_message = {};
};

/// Specialize `rfl::Metadata<T>` to attach zero-bloat out-of-band metadata to
/// struct `T` without wrapping its fields in `rfl::Description` /
/// `rfl::Deprecated`.
///
/// Supported members on a specialization `rfl::Metadata<MyStruct>`:
/// - `static constexpr std::array<rfl::FieldMeta, N> fields = ...;`
///   (or any contiguous range / `std::span<const rfl::FieldMeta>`), OR
/// - `static std::span<const rfl::FieldMeta> fields();` (or returning a
///   `const` lvalue reference to a contiguous range; may be defined out-of-line
///   in a `.cpp` file for complete compile-time isolation).
/// - Optional struct-level description:
///   `static constexpr std::string_view description = "...";` or
///   `static std::string_view description();`
///
/// Note: `rfl::Metadata<Inner>` attaches to `Inner`'s own schema definition
/// (`$defs/Inner`); fields flattened via `rfl::Flatten<Inner>` are looked up on
/// the enclosing struct's `rfl::Metadata<Outer>`.
template <class T>
struct Metadata {};

namespace internal {

template <class T>
concept HasFieldMetaVar = requires {
  { Metadata<T>::fields } -> std::ranges::contiguous_range;
  requires std::same_as<std::remove_cvref_t<std::ranges::range_value_t<
                            decltype(Metadata<T>::fields)>>,
                        FieldMeta>;
};

template <class T>
concept HasAnyFieldMetaFn = requires {
  { Metadata<T>::fields() } -> std::ranges::contiguous_range;
  requires std::same_as<std::remove_cvref_t<std::ranges::range_value_t<
                            decltype(Metadata<T>::fields())>>,
                        FieldMeta>;
};

template <class T>
concept HasFieldMetaFn =
    HasAnyFieldMetaFn<T> &&
    std::ranges::borrowed_range<decltype(Metadata<T>::fields())>;

template <class T>
constexpr bool has_field_metadata_v =
    HasFieldMetaVar<std::remove_cvref_t<T>> ||
    HasAnyFieldMetaFn<std::remove_cvref_t<T>>;

template <class T>
inline std::span<const FieldMeta> get_field_metadata() {
  using U = std::remove_cvref_t<T>;
  static_assert(
      !HasAnyFieldMetaFn<U> || HasFieldMetaFn<U>,
      "rfl::Metadata<T>::fields() must return a borrowed contiguous range "
      "(for example, std::span<const rfl::FieldMeta> or const auto&) so the "
      "returned view does not dangle.");
  if constexpr (HasFieldMetaFn<U>) {
    auto&& r = Metadata<U>::fields();
    return std::span<const FieldMeta>(std::ranges::data(r),
                                      std::ranges::size(r));
  } else if constexpr (HasFieldMetaVar<U>) {
    return std::span<const FieldMeta>(std::ranges::data(Metadata<U>::fields),
                                      std::ranges::size(Metadata<U>::fields));
  } else {
    return {};
  }
}

template <class T>
concept HasStructDescriptionVar = requires {
  { Metadata<T>::description } -> std::convertible_to<std::string_view>;
};

template <class T>
concept HasStructDescriptionFn = requires {
  { Metadata<T>::description() } -> std::convertible_to<std::string_view>;
};

template <class T>
constexpr bool has_struct_description_v =
    HasStructDescriptionVar<std::remove_cvref_t<T>> ||
    HasStructDescriptionFn<std::remove_cvref_t<T>>;

template <class T>
inline std::string get_struct_description() {
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
