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

template <class T>
struct HasDefaultVal;

template <class T>
struct HasDefaultVal {
  static constexpr bool value = false;
};

template <class... Fields>
struct HasDefaultVal<NamedTuple<Fields...>> {
  static constexpr bool value =
      (false || ... ||
       is_default_val_v<
           std::remove_cvref_t<std::remove_pointer_t<typename Fields::Type>>>);
};

template <class... PtrTypes>
struct HasDefaultValInTuple;

template <class... PtrTypes>
struct HasDefaultValInTuple<rfl::Tuple<PtrTypes...>> {
  static constexpr bool value =
      (false || ... || HasDefaultVal<remove_all_ptrs_t<PtrTypes>>::value);
};

template <StringLiteral _discriminator, class... Ts>
struct HasDefaultVal<TaggedUnion<_discriminator, Ts...>> {
  static constexpr bool value =
      (false || ... || HasDefaultVal<std::remove_cvref_t<Ts>>::value);
};

template <class T>
  requires(is_tagged_union_based<T> && !is_tagged_union_v<T>)
struct HasDefaultVal<T> : HasDefaultVal<tagged_union_base_t<T>> {};

template <class T>
  requires(has_flattened_tagged_union_v<T>)
struct HasDefaultVal<T> {
  static constexpr bool value =
      HasDefaultVal<named_tuple_t<T>>::value ||
      HasDefaultValInTuple<flattened_tu_ptrs_t<T>>::value;
};

template <class T>
  requires(std::is_class_v<T> && std::is_aggregate_v<T> &&
           !is_tagged_union_based<T> && !has_flattened_tagged_union_v<T>)
struct HasDefaultVal<T> : HasDefaultVal<named_tuple_t<T>> {};

template <class T, size_t N>
struct HasDefaultVal<std::array<T, N>> : HasDefaultVal<std::remove_cvref_t<T>> {
};

template <class T, size_t N>
struct HasDefaultVal<T[N]> : HasDefaultVal<std::remove_cvref_t<T>> {};

template <class T>
constexpr bool has_default_val_v = HasDefaultVal<std::remove_cvref_t<T>>::value;

}  // namespace rfl::internal

#endif
