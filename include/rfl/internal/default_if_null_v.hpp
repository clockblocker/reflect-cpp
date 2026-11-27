#ifndef RFL_INTERNAL_DEFAULTIFNULL_HPP_
#define RFL_INTERNAL_DEFAULTIFNULL_HPP_

#include <type_traits>

#include "../DefaultIfNull.hpp"
#include "../Processors.hpp"

namespace rfl::internal {

template <class T>
class default_if_null;

template <class T>
class default_if_null : public std::false_type {};

template <>
class default_if_null<DefaultIfNull> : public std::true_type {};

template <class Head, class... Tail>
struct default_if_null<Processors<Head, Tail...>> {
  static constexpr bool value =
      (default_if_null<Head>::value || ... || default_if_null<Tail>::value);
};

template <class T>
constexpr bool default_if_null_v =
    default_if_null<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace rfl::internal

#endif
