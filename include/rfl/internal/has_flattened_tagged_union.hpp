#ifndef RFL_INTERNAL_HAS_FLATTENED_TAGGED_UNION_HPP_
#define RFL_INTERNAL_HAS_FLATTENED_TAGGED_UNION_HPP_

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "../Tuple.hpp"
#include "../tuple_cat.hpp"
#include "StringLiteral.hpp"
#include "has_reflection_type_v.hpp"
#include "has_reflector.hpp"
#include "is_empty.hpp"
#include "is_flatten_field.hpp"
#include "is_named_tuple.hpp"
#include "is_tagged_union.hpp"
#include "ptr_tuple_t.hpp"
#include "to_ptr_tuple.hpp"

namespace rfl::parsing {
template <class T, class TagType, internal::StringLiteral _discriminator,
          bool _as_const_pointer>
struct TaggedUnionWrapperNoFields;

template <class T, class TagType, internal::StringLiteral _discriminator,
          bool _as_const_pointer>
struct TaggedUnionWrapperWithFields;
}  // namespace rfl::parsing

namespace rfl::internal {

template <class T>
struct is_tagged_union_wrapper : std::false_type {};

template <class T, class TagType, StringLiteral _discriminator,
          bool _as_const_pointer>
struct is_tagged_union_wrapper<parsing::TaggedUnionWrapperNoFields<
    T, TagType, _discriminator, _as_const_pointer>> : std::true_type {};

template <class T, class TagType, StringLiteral _discriminator,
          bool _as_const_pointer>
struct is_tagged_union_wrapper<parsing::TaggedUnionWrapperWithFields<
    T, TagType, _discriminator, _as_const_pointer>> : std::true_type {};

template <class T>
constexpr bool is_tagged_union_wrapper_v =
    is_tagged_union_wrapper<std::remove_cvref_t<T>>::value;

template <class T>
constexpr bool has_flattened_tagged_union_impl();

template <class TupleType>
constexpr bool tuple_has_flattened_tagged_union() {
  const auto check_one = []<int _i>(std::integral_constant<int, _i>) -> bool {
    using Elem = std::remove_cvref_t<
        std::remove_pointer_t<tuple_element_t<_i, TupleType>>>;
    if constexpr (is_flatten_field_v<Elem>) {
      using Inner = remove_all_ptrs_t<typename Elem::Type>;
      if constexpr (is_tagged_union_based<Inner>) {
        return true;
      } else {
        return has_flattened_tagged_union_impl<Inner>();
      }
    } else {
      return false;
    }
  };
  return [&]<int... _is>(std::integer_sequence<int, _is...>) {
    return (false || ... || check_one(std::integral_constant<int, _is>{}));
  }(std::make_integer_sequence<int, rfl::tuple_size_v<TupleType>>());
}

template <class T>
constexpr bool has_flattened_tagged_union_impl() {
  using U = remove_all_ptrs_t<T>;
  if constexpr (is_tagged_union_based<U> || is_named_tuple_v<U> ||
                !std::is_class_v<U> || !std::is_aggregate_v<U> ||
                has_reflection_type_v<U> || has_read_reflector<U> ||
                has_write_reflector<U>) {
    return false;
  } else if constexpr (is_empty<U>()) {
    return false;
  } else {
    return tuple_has_flattened_tagged_union<ptr_tuple_t<U>>();
  }
}

template <class T>
constexpr bool has_flattened_tagged_union_v =
    has_flattened_tagged_union_impl<T>();

/// Represents one slot in declaration order when decomposing a struct that
/// contains flattened TaggedUnion(s): either a regular field or a flattened TU.
struct FlattenedSlot {
  bool is_tu;
  size_t index;
};

template <size_t N1, size_t N2>
consteval std::array<FlattenedSlot, N1 + N2> concat_slot_arrays(
    const std::array<FlattenedSlot, N1>& _a,
    const std::array<FlattenedSlot, N2>& _b) {
  std::array<FlattenedSlot, N1 + N2> res{};
  for (size_t i = 0; i < N1; ++i) {
    res[i] = _a[i];
  }
  for (size_t i = 0; i < N2; ++i) {
    res[N1 + i] = _b[i];
  }
  return res;
}

template <size_t N>
consteval std::array<FlattenedSlot, N> concat_all_slot_arrays(
    const std::array<FlattenedSlot, N>& _a) {
  return _a;
}

template <size_t N1, size_t N2, class... Rest>
consteval auto concat_all_slot_arrays(const std::array<FlattenedSlot, N1>& _a,
                                      const std::array<FlattenedSlot, N2>& _b,
                                      const Rest&... _rest) {
  return concat_all_slot_arrays(concat_slot_arrays(_a, _b), _rest...);
}

consteval inline std::array<FlattenedSlot, 0> concat_all_slot_arrays() {
  return {};
}

template <class T>
consteval auto raw_flattened_slots_unindexed() {
  using U = remove_all_ptrs_t<T>;
  if constexpr (is_named_tuple_v<U>) {
    std::array<FlattenedSlot, U::size()> arr{};
    for (size_t i = 0; i < U::size(); ++i) {
      arr[i] = FlattenedSlot{false, 0};
    }
    return arr;
  } else if constexpr (is_empty<U>()) {
    return std::array<FlattenedSlot, 0>{};
  } else {
    using TupleType = ptr_tuple_t<U>;
    const auto get_one = []<int _i>(std::integral_constant<int, _i>) {
      using Elem = std::remove_cvref_t<
          std::remove_pointer_t<tuple_element_t<_i, TupleType>>>;
      if constexpr (is_flatten_field_v<Elem>) {
        using Inner = remove_all_ptrs_t<typename Elem::Type>;
        if constexpr (is_tagged_union_based<Inner>) {
          return std::array<FlattenedSlot, 1>{FlattenedSlot{true, 0}};
        } else {
          return raw_flattened_slots_unindexed<Inner>();
        }
      } else {
        return std::array<FlattenedSlot, 1>{FlattenedSlot{false, 0}};
      }
    };
    return [&]<int... _is>(std::integer_sequence<int, _is...>) {
      return concat_all_slot_arrays(
          get_one(std::integral_constant<int, _is>{})...);
    }(std::make_integer_sequence<int, rfl::tuple_size_v<TupleType>>());
  }
}

template <class T>
consteval auto flattened_tu_slots() {
  auto arr = raw_flattened_slots_unindexed<T>();
  size_t reg_idx = 0;
  size_t tu_idx = 0;
  for (auto& slot : arr) {
    if (slot.is_tu) {
      slot.index = tu_idx++;
    } else {
      slot.index = reg_idx++;
    }
  }
  return arr;
}

template <class T>
consteval size_t raw_regular_field_count() {
  constexpr auto slots = flattened_tu_slots<T>();
  size_t count = 0;
  for (const auto& slot : slots) {
    if (!slot.is_tu) {
      ++count;
    }
  }
  return count;
}

/// Collects pointers to all flattened TaggedUnion members of `_t` in
/// declaration order.
template <class T>
auto to_flattened_tu_ptrs(T& _t) {
  if constexpr (std::is_pointer_v<std::remove_cvref_t<T>>) {
    return to_flattened_tu_ptrs(*_t);
  } else if constexpr (is_named_tuple_v<std::remove_cvref_t<T>> ||
                       is_empty<std::remove_cvref_t<T>>()) {
    return rfl::Tuple<>();
  } else {
    auto ptr_tuple = to_ptr_tuple(_t);
    using PtrTuple = std::remove_cvref_t<decltype(ptr_tuple)>;
    const auto get_one = [&]<int _i>(std::integral_constant<int, _i>) {
      using Elem = std::remove_cvref_t<
          std::remove_pointer_t<tuple_element_t<_i, PtrTuple>>>;
      if constexpr (is_flatten_field_v<Elem>) {
        using Inner = remove_all_ptrs_t<typename Elem::Type>;
        static_assert(
            is_tagged_union_wrapper_v<T> ||
                !std::is_pointer_v<typename Elem::Type> ||
                (!is_tagged_union_based<Inner> &&
                 !has_flattened_tagged_union_impl<Inner>()),
            "rfl::Flatten with a raw pointer to a TaggedUnion (or a struct "
            "containing a flattened TaggedUnion) is not supported; use value "
            "semantics instead.");
        auto& target = deref_to_ref(rfl::get<_i>(ptr_tuple)->get());
        if constexpr (is_tagged_union_based<Inner>) {
          return rfl::make_tuple(&target);
        } else {
          return to_flattened_tu_ptrs(target);
        }
      } else {
        return rfl::Tuple<>();
      }
    };
    constexpr auto size = rfl::tuple_size_v<PtrTuple>;
    return [&]<int... _is>(std::integer_sequence<int, _is...>) {
      return rfl::tuple_cat(get_one(std::integral_constant<int, _is>{})...);
    }(std::make_integer_sequence<int, size>());
  }
}

template <class T>
using flattened_tu_ptrs_t =
    decltype(to_flattened_tu_ptrs(std::declval<std::remove_cvref_t<T>&>()));

}  // namespace rfl::internal

#endif
