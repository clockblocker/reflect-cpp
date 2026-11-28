#ifndef RFL_INTERNAL_CPP26_BIND_TO_TUPLE_HPP_
#define RFL_INTERNAL_CPP26_BIND_TO_TUPLE_HPP_

#ifdef REFLECTCPP_USE_CPP26_REFLECTION

#include <array>
#include <cassert>
#include <meta>
#include <ranges>
#include <tuple>
#include <utility>

#include "../../Tuple.hpp"
#include "get_members.hpp"

namespace rfl::internal::cpp26 {

template <class T, class F>
decltype(auto) apply_to_fields(T& _t, F&& _f) {
  using Type = std::remove_cvref_t<T>;
  constexpr auto members = std::define_static_array(get_members<Type>());
  constexpr auto [... indices] = std::make_index_sequence<members.size()>();
  return std::forward<F>(_f)(_t.[:members[indices]:]...);
}

template <class T>
auto bind_to_tuple(T& _t) {
  return apply_to_fields(
      _t, [](auto&... _refs) { return rfl::make_tuple(&_refs...); });
}

}  // namespace rfl::internal::cpp26

#endif
#endif
