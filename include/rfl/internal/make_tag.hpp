#ifndef RFL_INTERNAL_MAKE_TAG_HPP_
#define RFL_INTERNAL_MAKE_TAG_HPP_

#include <type_traits>

#include "../Literal.hpp"
#include "../field_names_t.hpp"
#include "../named_tuple_t.hpp"
#include "../to_view.hpp"
#include "StringLiteral.hpp"
#include "get_type_name.hpp"
#include "has_reflection_type_v.hpp"
#include "has_reflector.hpp"
#include "has_tag_v.hpp"
#include "is_named_tuple.hpp"
#include "is_tagged_union.hpp"
#include "remove_namespaces.hpp"

namespace rfl::internal {

template <class Acc, StringLiteral... Rest>
struct append_unique_literals;

template <class Acc>
struct append_unique_literals<Acc> {
  using type = Acc;
};

template <StringLiteral... AccNames, StringLiteral Head, StringLiteral... Tail>
struct append_unique_literals<Literal<AccNames...>, Head, Tail...> {
  using NextAcc =
      std::conditional_t<Literal<AccNames...>::template contains<Head>(),
                         Literal<AccNames...>, Literal<AccNames..., Head>>;
  using type = typename append_unique_literals<NextAcc, Tail...>::type;
};

template <class... Lits>
struct unique_literal_concat;

template <StringLiteral... Names>
struct unique_literal_concat<Literal<Names...>> {
  using type = typename append_unique_literals<Literal<>, Names...>::type;
};

template <StringLiteral... N1, StringLiteral... N2, class... Rest>
struct unique_literal_concat<Literal<N1...>, Literal<N2...>, Rest...>
    : unique_literal_concat<Literal<N1..., N2...>, Rest...> {};

template <class... Lits>
using unique_literal_t = typename unique_literal_concat<Lits...>::type;

template <class T, StringLiteral _discriminator>
struct ContainsDiscriminator;

template <class T, StringLiteral _discriminator>
constexpr bool contains_discriminator_v =
    ContainsDiscriminator<remove_all_ptrs_t<T>, _discriminator>::value;

template <class T, StringLiteral _discriminator>
struct ContainsDiscriminator {
  template <class Base>
  static constexpr bool contains_in_tu() {
    return []<StringLiteral InnerDisc, class... Alts>(
               const TaggedUnion<InnerDisc, Alts...>*) {
      if constexpr (InnerDisc == _discriminator || sizeof...(Alts) == 0) {
        return false;
      } else {
        return (true && ... &&
                contains_discriminator_v<Alts, _discriminator>);
      }
    }(static_cast<const Base*>(nullptr));
  }

  static constexpr bool compute() {
    if constexpr (is_tagged_union_based<T>) {
      return contains_in_tu<tagged_union_base_t<T>>();
    } else if constexpr (has_reflection_type_v<T>) {
      return contains_discriminator_v<
          std::remove_cvref_t<typename T::ReflectionType>, _discriminator>;
    } else if constexpr (is_named_tuple_v<T> ||
                         (std::is_class_v<T> && std::is_aggregate_v<T> &&
                          !has_read_reflector<T> && !has_write_reflector<T>)) {
      return named_tuple_t<T>::Names::template contains<_discriminator>();
    } else {
      return false;
    }
  }

  static constexpr bool value = compute();
};

template <internal::StringLiteral _discriminator, class T>
static inline auto make_tag(const T& _t) noexcept;

template <internal::StringLiteral _discriminator,
          internal::StringLiteral _inner_disc, class... Alts>
static inline auto make_tu_alternatives_tag(
    const TaggedUnion<_inner_disc, Alts...>*) noexcept {
  using Combined = unique_literal_t<
      decltype(make_tag<_discriminator, Alts>(std::declval<Alts>()))...>;
  return Combined();
}

template <internal::StringLiteral _discriminator, class T>
static inline auto make_tag(const T& _t) noexcept {
  using U = std::remove_cvref_t<T>;
  if constexpr (is_tagged_union_based<U>) {
    if constexpr (internal::has_tag_v<U>) {
      using LiteralType = typename U::Tag;
      return LiteralType::template name_of<0>();
    } else if constexpr (contains_discriminator_v<U, _discriminator>) {
      using Base = tagged_union_base_t<U>;
      return make_tu_alternatives_tag<_discriminator>(
          static_cast<const Base*>(nullptr));
    } else {
      return rfl::Literal<
          internal::remove_namespaces<internal::get_type_name<U>()>()>();
    }
  } else if constexpr (internal::has_reflection_type_v<U>) {
    if constexpr (internal::has_tag_v<U> &&
                  !contains_discriminator_v<
                      std::remove_cvref_t<typename U::ReflectionType>,
                      _discriminator>) {
      using LiteralType = typename U::Tag;
      return LiteralType::template name_of<0>();
    } else {
      return make_tag<_discriminator>(_t.reflection());
    }
  } else if constexpr (contains_discriminator_v<U, _discriminator>) {
    return *to_view(_t).template get<_discriminator>();
  } else if constexpr (internal::has_tag_v<U>) {
    using LiteralType = typename U::Tag;
    return LiteralType::template name_of<0>();
  } else {
    return rfl::Literal<
        internal::remove_namespaces<internal::get_type_name<U>()>()>();
  }
}

}  // namespace rfl::internal

#endif
