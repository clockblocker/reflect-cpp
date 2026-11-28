#ifndef RFL_HASDEFAULTVALV_HPP_
#define RFL_HASDEFAULTVALV_HPP_

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "bind_to_tuple.hpp"
#include "is_default_val_v.hpp"
#include "is_field.hpp"
#include "is_flatten_field.hpp"

namespace rfl {

template <class... FieldTypes>
class NamedTuple;

namespace internal {

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
  if constexpr (is_field<U>::value) {
    return is_default_val_v<typename U::Type>;
  } else if constexpr (is_flatten_field<U>::value) {
    return HasDefaultVal<
        std::remove_cvref_t<std::remove_pointer_t<typename U::Type>>>::value;
  } else {
    return is_default_val<U>::value;
  }
}

struct HasDefaultValVisitor {
  template <class... Fields>
  constexpr auto operator()(Fields&...) const noexcept
      -> std::bool_constant<(false || ... ||
                             has_default_val_type_v<Fields>())> {
    return {};
  }
};

template <class T>
  requires(std::is_class_v<T> && std::is_aggregate_v<T>)
struct HasDefaultVal<T> {
  static constexpr bool value =
      decltype(apply_to_fields(std::declval<T&>(),
                               HasDefaultValVisitor{}))::value;
};

template <class T, std::size_t N>
struct HasDefaultVal<std::array<T, N>> : HasDefaultVal<std::remove_cvref_t<T>> {
};

template <class T, std::size_t N>
struct HasDefaultVal<T[N]> : HasDefaultVal<std::remove_cvref_t<T>> {};

template <class T>
constexpr bool has_default_val_v = HasDefaultVal<std::remove_cvref_t<T>>::value;

}  // namespace internal
}  // namespace rfl

#endif
