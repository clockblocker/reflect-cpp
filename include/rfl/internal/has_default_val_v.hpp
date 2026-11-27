#ifndef RFL_HASDEFAULTVALV_HPP_
#define RFL_HASDEFAULTVALV_HPP_

#include <array>
#include <type_traits>

#include "../NamedTuple.hpp"
#include "bind_to_tuple.hpp"
#include "is_field.hpp"
#include "is_flatten_field.hpp"
#include "is_default_val_v.hpp"

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

template <class T>
constexpr bool has_default_val_type_v() {
  using U = std::remove_cvref_t<T>;
  if constexpr (is_field_v<U>) {
    return has_default_val_type_v<typename U::Type>();
  } else if constexpr (is_flatten_field_v<U>) {
    return HasDefaultVal<
        std::remove_cvref_t<std::remove_pointer_t<typename U::Type>>>::value;
  } else {
    return is_default_val_v<U>;
  }
}

template <class T>
  requires(std::is_class_v<T> && std::is_aggregate_v<T> && !std::is_union_v<T>)
struct HasDefaultVal<T> {
 private:
  static consteval bool compute() {
    using Result = decltype(apply_to_fields(
        std::declval<T&>(), []<class... Fields>(Fields&...) {
          return std::bool_constant<
              (false || ... || has_default_val_type_v<Fields>())>{};
        }));
    return Result::value;
  }

 public:
  static constexpr bool value = compute();
};

template <class T, size_t N>
struct HasDefaultVal<std::array<T, N>> : HasDefaultVal<std::remove_cvref_t<T>> {
};

template <class T, size_t N>
struct HasDefaultVal<T[N]> : HasDefaultVal<std::remove_cvref_t<T>> {};

template <class T>
constexpr bool has_default_val_v = HasDefaultVal<std::remove_cvref_t<T>>::value;

}  // namespace rfl::internal

#endif
