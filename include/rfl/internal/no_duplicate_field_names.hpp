#ifndef RFL_INTERNAL_NO_DUPLICATE_FIELD_NAMES_HPP_
#define RFL_INTERNAL_NO_DUPLICATE_FIELD_NAMES_HPP_

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Tuple.hpp"
#include "is_skip.hpp"

namespace rfl {
namespace internal {

namespace no_duplicate_field_names_helpers {

struct FieldNameInfo {
  std::string_view name;
  bool is_serialized;
  bool is_deserialized;
};

template <class FieldType>
constexpr bool field_is_serialized() {
  if constexpr (requires { typename FieldType::Type; }) {
    return !skip_serialization_v<typename FieldType::Type>;
  } else {
    return true;
  }
}

template <class FieldType>
constexpr bool field_is_deserialized() {
  if constexpr (requires { typename FieldType::Type; }) {
    return !skip_deserialization_v<typename FieldType::Type>;
  } else {
    return true;
  }
}

}  // namespace no_duplicate_field_names_helpers

template <class Fields>
constexpr inline bool no_duplicate_field_names() {
  constexpr auto num_fields = rfl::tuple_size_v<Fields>;

  if constexpr (num_fields <= 1) {
    return true;
  } else {
    constexpr auto infos = []<int... _is>(std::integer_sequence<int, _is...>) {
      return std::array<no_duplicate_field_names_helpers::FieldNameInfo,
                        num_fields>{
          no_duplicate_field_names_helpers::FieldNameInfo{
              std::remove_cvref_t<tuple_element_t<_is, Fields>>::name_
                  .string_view(),
              no_duplicate_field_names_helpers::field_is_serialized<
                  std::remove_cvref_t<tuple_element_t<_is, Fields>>>(),
              no_duplicate_field_names_helpers::field_is_deserialized<
                  std::remove_cvref_t<tuple_element_t<_is, Fields>>>(),
          }...};
    }(std::make_integer_sequence<int, num_fields>());

    for (size_t i = 1; i < num_fields; ++i) {
      for (size_t j = 0; j < i; ++j) {
        if (infos[i].name == infos[j].name) {
          if ((infos[i].is_serialized && infos[j].is_serialized) ||
              (infos[i].is_deserialized && infos[j].is_deserialized)) {
            return false;
          }
        }
      }
    }
    return true;
  }
}

}  // namespace internal
}  // namespace rfl

#endif
