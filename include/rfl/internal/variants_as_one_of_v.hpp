#ifndef RFL_INTERNAL_VARIANTSASONEOF_HPP_
#define RFL_INTERNAL_VARIANTSASONEOF_HPP_

#include <type_traits>

#include "../Processors.hpp"
#include "../VariantsAsOneOf.hpp"

namespace rfl::internal {

template <class T>
class variants_as_one_of;

template <class T>
class variants_as_one_of : public std::false_type {};

template <>
class variants_as_one_of<VariantsAsOneOf> : public std::true_type {};

template <class Head, class... Tail>
struct variants_as_one_of<Processors<Head, Tail...>> {
  static constexpr bool value = (variants_as_one_of<Head>::value || ... ||
                                 variants_as_one_of<Tail>::value);
};

template <class T>
constexpr bool variants_as_one_of_v =
    variants_as_one_of<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace rfl::internal

#endif
