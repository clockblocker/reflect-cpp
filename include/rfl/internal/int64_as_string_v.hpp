#ifndef RFL_INTERNAL_INT64ASSTRING_HPP_
#define RFL_INTERNAL_INT64ASSTRING_HPP_

#include <type_traits>

#include "../Int64AsString.hpp"
#include "../Processors.hpp"

namespace rfl::internal {

template <class T>
class int64_as_string;

template <class T>
class int64_as_string : public std::false_type {};

template <>
class int64_as_string<Int64AsString> : public std::true_type {};

template <class Head, class... Tail>
struct int64_as_string<Processors<Head, Tail...>> {
  static constexpr bool value =
      (int64_as_string<Head>::value || ... || int64_as_string<Tail>::value);
};

template <class T>
constexpr bool int64_as_string_v =
    int64_as_string<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace rfl::internal

#endif
