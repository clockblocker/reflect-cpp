#ifndef RFL_ENUMNAMES_HPP_
#define RFL_ENUMNAMES_HPP_

#include <array>
#include <cctype>
#include <concepts>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "Literal.hpp"
#include "Processors.hpp"
#include "internal/StringLiteral.hpp"

namespace rfl {

/// Built-in wire-name transforms applicable to C++ enum enumerator names via
/// `rfl::EnumNames<E>::transform` or `rfl::TransformEnumNames<...>` processors.
enum class EnumNameTransform : uint8_t {
  none = 0,
  snake_to_kebab,
  snake_to_camel,
  snake_to_pascal,
  to_lower,
  to_upper,
};

/// Applies a built-in `EnumNameTransform` to an enumerator identifier.
inline std::string apply_enum_name_transform(
    const std::string_view _name, const EnumNameTransform _transform) {
  switch (_transform) {
    case EnumNameTransform::none:
      return std::string(_name);
    case EnumNameTransform::snake_to_kebab: {
      std::string out(_name);
      for (char& ch : out) {
        if (ch == '_') ch = '-';
      }
      return out;
    }
    case EnumNameTransform::snake_to_camel: {
      std::string out;
      out.reserve(_name.size());
      bool cap_next = false;
      for (const char ch : _name) {
        if (ch == '_') {
          if (!out.empty()) {
            cap_next = true;
          }
        } else if (cap_next) {
          out.push_back(
              static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
          cap_next = false;
        } else {
          out.push_back(ch);
        }
      }
      return out;
    }
    case EnumNameTransform::snake_to_pascal: {
      std::string out;
      out.reserve(_name.size());
      bool cap_next = true;
      for (const char ch : _name) {
        if (ch == '_') {
          cap_next = true;
        } else if (cap_next) {
          out.push_back(
              static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
          cap_next = false;
        } else {
          out.push_back(ch);
        }
      }
      return out;
    }
    case EnumNameTransform::to_lower: {
      std::string out;
      out.reserve(_name.size());
      for (const char ch : _name) {
        out.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
      }
      return out;
    }
    case EnumNameTransform::to_upper: {
      std::string out;
      out.reserve(_name.size());
      for (const char ch : _name) {
        out.push_back(
            static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
      }
      return out;
    }
  }
  return std::string(_name);
}

/// Pair of `(enumerator, wire_name)` for per-enumerator overrides in
/// `rfl::EnumNames<E>`.
template <class EnumType>
using EnumOverride = std::pair<EnumType, std::string_view>;

/// Specialize `rfl::EnumNames<E>` to customize wire names for enum `E` across
/// JSON, CLI, Generic, nlohmann, YAML, XML, and JSON Schema without duplicating
/// enumerators as `rfl::Literal<...>`.
///
/// Supported members (individually or combined; `overrides` takes precedence):
/// - `static constexpr EnumNameTransform transform = EnumNameTransform::snake_to_kebab;`
///   OR `static std::string transform(std::string_view _raw_name);`
/// - `static constexpr std::array<rfl::EnumOverride<E>, N> overrides = {{ ... }};`
///   OR `static std::span<const rfl::EnumOverride<E>> overrides();`
template <class EnumType>
struct EnumNames {};

/// Processor that applies a whole-enum wire-name transform to all enums without
/// an explicit `rfl::EnumNames<E>` override/transform.
template <EnumNameTransform _transform>
struct TransformEnumNames {
  static constexpr EnumNameTransform value = _transform;

  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

using EnumSnakeCaseToKebabCase =
    TransformEnumNames<EnumNameTransform::snake_to_kebab>;
using EnumSnakeCaseToCamelCase =
    TransformEnumNames<EnumNameTransform::snake_to_camel>;
using EnumSnakeCaseToPascalCase =
    TransformEnumNames<EnumNameTransform::snake_to_pascal>;
using EnumToLowerCase = TransformEnumNames<EnumNameTransform::to_lower>;
using EnumToUpperCase = TransformEnumNames<EnumNameTransform::to_upper>;

namespace internal {

template <const auto& _table, size_t _i>
consteval auto string_literal_from_table_entry() {
  constexpr std::string_view sv = std::string_view(_table[_i]);
  return StringLiteral<sv.size() + 1>(sv);
}

}  // namespace internal

/// Derives an `rfl::Literal<...>` type directly from a `constexpr` table
/// (`std::array<std::string_view, N>`, `std::array<const char*, N>`, or
/// `constexpr std::string_view kOps[] = { ... }`) so wire-string tables never
/// need to be duplicated as `rfl::Literal<"...">`.
///
/// Example:
///   inline constexpr std::array<std::string_view, 3> kModes = {
///       "fast", "balanced", "max-compression"};
///   using ModeLiteral = rfl::literal_from_table_t<kModes>;
template <const auto& _table,
          class _seq = std::make_index_sequence<std::size(_table)>>
struct literal_from_table;

template <const auto& _table, size_t... _is>
struct literal_from_table<_table, std::index_sequence<_is...>> {
  using type = Literal<internal::string_literal_from_table_entry<_table, _is>()...>;
};

template <const auto& _table>
using literal_from_table_t = typename literal_from_table<_table>::type;

namespace internal {

template <class ProcessorsType>
struct enum_name_transform_from_processors {
  static constexpr EnumNameTransform value = EnumNameTransform::none;
};

template <>
struct enum_name_transform_from_processors<Processors<>> {
  static constexpr EnumNameTransform value = EnumNameTransform::none;
};

template <EnumNameTransform _t, class... Tail>
struct enum_name_transform_from_processors<
    Processors<TransformEnumNames<_t>, Tail...>> {
  static constexpr EnumNameTransform value = _t;
};

template <class Head, class... Tail>
struct enum_name_transform_from_processors<Processors<Head, Tail...>>
    : enum_name_transform_from_processors<Processors<Tail...>> {};

template <class ProcessorsType>
constexpr EnumNameTransform enum_name_transform_v =
    enum_name_transform_from_processors<
        std::remove_cvref_t<ProcessorsType>>::value;

template <class E>
concept HasEnumOverridesVar = requires {
  { EnumNames<E>::overrides } -> std::ranges::contiguous_range;
  requires std::same_as<std::remove_cvref_t<std::ranges::range_value_t<
                            decltype(EnumNames<E>::overrides)>>,
                        EnumOverride<E>>;
};

template <class E>
concept HasAnyEnumOverridesFn = requires {
  { EnumNames<E>::overrides() } -> std::ranges::contiguous_range;
  requires std::same_as<std::remove_cvref_t<std::ranges::range_value_t<
                            decltype(EnumNames<E>::overrides())>>,
                        EnumOverride<E>>;
};

template <class E>
concept HasEnumOverridesFn =
    HasAnyEnumOverridesFn<E> &&
    std::ranges::borrowed_range<decltype(EnumNames<E>::overrides())>;

template <class E>
concept HasEnumTransformConst = requires {
  { EnumNames<E>::transform } -> std::convertible_to<EnumNameTransform>;
};

template <class E>
concept HasEnumTransformFn = requires(std::string_view _s) {
  { EnumNames<E>::transform(_s) } -> std::convertible_to<std::string>;
};

template <class E, class ProcessorsType = Processors<>>
constexpr bool has_custom_enum_names_v =
    HasEnumOverridesVar<std::remove_cvref_t<E>> ||
    HasEnumOverridesFn<std::remove_cvref_t<E>> ||
    HasEnumTransformConst<std::remove_cvref_t<E>> ||
    HasEnumTransformFn<std::remove_cvref_t<E>> ||
    (enum_name_transform_v<ProcessorsType> != EnumNameTransform::none);

template <class E>
inline std::span<const EnumOverride<std::remove_cvref_t<E>>>
get_enum_overrides() {
  using U = std::remove_cvref_t<E>;
  static_assert(
      !HasAnyEnumOverridesFn<U> || HasEnumOverridesFn<U>,
      "rfl::EnumNames<E>::overrides() must return a borrowed contiguous range "
      "(for example, std::span<const rfl::EnumOverride<E>> or const auto&).");
  if constexpr (HasEnumOverridesFn<U>) {
    auto&& r = EnumNames<U>::overrides();
    return std::span<const EnumOverride<U>>(std::ranges::data(r),
                                            std::ranges::size(r));
  } else if constexpr (HasEnumOverridesVar<U>) {
    return std::span<const EnumOverride<U>>(
        std::ranges::data(EnumNames<U>::overrides),
        std::ranges::size(EnumNames<U>::overrides));
  } else {
    return {};
  }
}

template <class E, class ProcessorsType = Processors<>>
inline std::string resolve_enum_wire_name(const E _val,
                                          const std::string_view _raw_name) {
  using U = std::remove_cvref_t<E>;
  for (const auto& [e, wire] : get_enum_overrides<U>()) {
    if (e == _val) {
      return std::string(wire);
    }
  }
  if constexpr (HasEnumTransformFn<U>) {
    return std::string(EnumNames<U>::transform(_raw_name));
  } else if constexpr (HasEnumTransformConst<U>) {
    return apply_enum_name_transform(_raw_name, EnumNames<U>::transform);
  } else if constexpr (enum_name_transform_v<ProcessorsType> !=
                       EnumNameTransform::none) {
    return apply_enum_name_transform(_raw_name,
                                     enum_name_transform_v<ProcessorsType>);
  } else {
    return std::string(_raw_name);
  }
}

}  // namespace internal
}  // namespace rfl

#endif
