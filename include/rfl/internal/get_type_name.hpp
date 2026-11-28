#ifndef RFL_INTERNAL_GETTYPENAME_HPP_
#define RFL_INTERNAL_GETTYPENAME_HPP_

#include <string_view>

#ifndef REFLECTCPP_USE_CPP26_REFLECTION
#include "cpp20/get_type_name.hpp"
#else
#include "cpp26/get_type_name.hpp"
#endif

namespace rfl::internal {

template <class T>
consteval auto get_type_name() {
#ifndef REFLECTCPP_USE_CPP26_REFLECTION
  return cpp20::get_type_name<T>();
#else
  return cpp26::get_type_name<T>();
#endif
}

template <class T>
inline constexpr auto type_name_literal_v = get_type_name<T>();

template <class T>
consteval std::string_view get_type_name_str_view() noexcept {
  return type_name_literal_v<T>.string_view();
}

}  // namespace rfl::internal

#endif
