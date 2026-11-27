#ifndef RFL_INTERNAL_CASEINSENSITIVEENUMS_HPP_
#define RFL_INTERNAL_CASEINSENSITIVEENUMS_HPP_

#include <type_traits>

#include "../CaseInsensitiveEnums.hpp"
#include "../Processors.hpp"
#include "../cli/CaseInsensitiveEnums.hpp"

namespace rfl::internal {

template <class T>
struct case_insensitive_enums : std::false_type {};

template <>
struct case_insensitive_enums<CaseInsensitiveEnums> : std::true_type {};

template <>
struct case_insensitive_enums<rfl::cli::CaseInsensitiveEnums>
    : std::true_type {};

template <class Head, class... Tail>
struct case_insensitive_enums<Processors<Head, Tail...>> {
  static constexpr bool value =
      (case_insensitive_enums<Head>::value || ... ||
       case_insensitive_enums<Tail>::value);
};

template <class T>
constexpr bool case_insensitive_enums_v =
    case_insensitive_enums<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace rfl::internal

#endif
