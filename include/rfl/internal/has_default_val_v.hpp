#ifndef RFL_HASDEFAULTVALV_HPP_
#define RFL_HASDEFAULTVALV_HPP_

#include <array>
#include <type_traits>

#include "../NamedTuple.hpp"
#include "../Tuple.hpp"
#include "../named_tuple_t.hpp"
#include "has_flattened_tagged_union.hpp"
#include "is_default_val_v.hpp"
#include "is_tagged_union.hpp"

namespace rfl::internal {

/// With _unwrapped_only, only counts fields for which
/// is_unwrapped_default_val_v is true.
template <class T, bool _unwrapped_only = false>
struct HasDefaultVal;

template <class T, bool _unwrapped_only>
struct HasDefaultVal {
  static constexpr bool value = false;
};

template <class... Fields, bool _unwrapped_only>
struct HasDefaultVal<NamedTuple<Fields...>, _unwrapped_only> {
  template <class F,
            class U =
                std::remove_cvref_t<std::remove_pointer_t<typename F::Type>>>
  static constexpr bool is_default_val_field =
      _unwrapped_only ? is_unwrapped_default_val_v<U> : is_default_val_v<U>;

  static constexpr bool value = (false || ... || is_default_val_field<Fields>);
};

template <class PtrTuple, bool _unwrapped_only = false>
struct HasDefaultValInTuple;

template <class... PtrTypes, bool _unwrapped_only>
struct HasDefaultValInTuple<rfl::Tuple<PtrTypes...>, _unwrapped_only> {
  static constexpr bool value =
      (false || ... ||
       HasDefaultVal<remove_all_ptrs_t<PtrTypes>, _unwrapped_only>::value);
};

template <StringLiteral _discriminator, class... Ts, bool _unwrapped_only>
struct HasDefaultVal<TaggedUnion<_discriminator, Ts...>, _unwrapped_only> {
  static constexpr bool value =
      (false || ... ||
       HasDefaultVal<std::remove_cvref_t<Ts>, _unwrapped_only>::value);
};

template <class T, bool _unwrapped_only>
  requires(is_tagged_union_based<T> && !is_tagged_union_v<T>)
struct HasDefaultVal<T, _unwrapped_only>
    : HasDefaultVal<tagged_union_base_t<T>, _unwrapped_only> {};

template <class T, bool _unwrapped_only>
  requires(has_flattened_tagged_union_v<T>)
struct HasDefaultVal<T, _unwrapped_only> {
  static constexpr bool value =
      HasDefaultVal<named_tuple_t<T>, _unwrapped_only>::value ||
      HasDefaultValInTuple<flattened_tu_ptrs_t<T>, _unwrapped_only>::value;
};

template <class T, bool _unwrapped_only>
  requires(std::is_class_v<T> && std::is_aggregate_v<T> &&
           !is_tagged_union_based<T> && !has_flattened_tagged_union_v<T>)
struct HasDefaultVal<T, _unwrapped_only>
    : HasDefaultVal<named_tuple_t<T>, _unwrapped_only> {};

template <class T, size_t N, bool _unwrapped_only>
struct HasDefaultVal<std::array<T, N>, _unwrapped_only>
    : HasDefaultVal<std::remove_cvref_t<T>, _unwrapped_only> {};

template <class T, size_t N, bool _unwrapped_only>
struct HasDefaultVal<T[N], _unwrapped_only>
    : HasDefaultVal<std::remove_cvref_t<T>, _unwrapped_only> {};

template <class T>
constexpr bool has_default_val_v = HasDefaultVal<std::remove_cvref_t<T>>::value;

/// Whether T has an rfl::DefaultVal field that is not inside rfl::Short,
/// rfl::Positional, rfl::Description or rfl::Deprecated.
template <class T>
constexpr bool has_unwrapped_default_val_v =
    HasDefaultVal<std::remove_cvref_t<T>, true>::value;

}  // namespace rfl::internal

#endif
