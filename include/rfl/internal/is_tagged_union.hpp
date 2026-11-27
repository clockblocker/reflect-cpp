#ifndef RFL_INTERNAL_IS_TAGGED_UNION_HPP_
#define RFL_INTERNAL_IS_TAGGED_UNION_HPP_

#include <type_traits>
#include <utility>

#include "StringLiteral.hpp"

namespace rfl {

template <internal::StringLiteral _discriminator, class... Ts>
struct TaggedUnion;

namespace internal {

template <class T>
struct remove_all_ptrs {
  using type = std::remove_cvref_t<T>;
};

template <class T>
struct remove_all_ptrs<T*> : remove_all_ptrs<std::remove_cvref_t<T>> {};

template <class T>
using remove_all_ptrs_t =
    typename remove_all_ptrs<std::remove_cvref_t<T>>::type;

template <class T>
decltype(auto) deref_to_ref(T&& _x) {
  if constexpr (std::is_pointer_v<std::remove_cvref_t<T>>) {
    return deref_to_ref(*_x);
  } else {
    return (_x);
  }
}

template <class T>
struct is_tagged_union : std::false_type {};

template <StringLiteral _discriminator, class... Ts>
struct is_tagged_union<TaggedUnion<_discriminator, Ts...>> : std::true_type {};

template <class T>
constexpr bool is_tagged_union_v =
    is_tagged_union<std::remove_cvref_t<T>>::value;

template <StringLiteral _discriminator, class... Ts>
TaggedUnion<_discriminator, Ts...> deduce_tagged_union_base(
    const TaggedUnion<_discriminator, Ts...>&);

template <class T>
concept is_tagged_union_based = requires(const std::remove_cvref_t<T>& t) {
  deduce_tagged_union_base(t);
};

template <class T>
using tagged_union_base_t = decltype(deduce_tagged_union_base(
    std::declval<const std::remove_cvref_t<T>&>()));

template <class Target, class Base>
Target from_tagged_union_base(Base&& _base) {
  if constexpr (std::is_same_v<std::remove_cvref_t<Target>,
                               std::remove_cvref_t<Base>>) {
    return std::move(_base);
  } else if constexpr (std::is_constructible_v<Target, Base&&>) {
    return Target(std::move(_base));
  } else if constexpr (std::is_constructible_v<
                           Target, typename std::remove_cvref_t<
                                       Base>::VariantType&&>) {
    return Target(std::move(_base.variant()));
  } else {
    return Target{std::move(_base)};
  }
}

}  // namespace internal
}  // namespace rfl

#endif
