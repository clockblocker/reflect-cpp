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
  constexpr size_t num_fields = rfl::tuple_size_v<Fields>;

  if constexpr (num_fields <= 1) {
    return true;
  } else {
    constexpr auto infos = []<size_t... _is>(std::index_sequence<_is...>) {
      std::array<no_duplicate_field_names_helpers::FieldNameInfo, num_fields>
          arr{no_duplicate_field_names_helpers::FieldNameInfo{
              std::remove_cvref_t<tuple_element_t<_is, Fields>>::name_
                  .string_view(),
              no_duplicate_field_names_helpers::field_is_serialized<
                  std::remove_cvref_t<tuple_element_t<_is, Fields>>>(),
              no_duplicate_field_names_helpers::field_is_deserialized<
                  std::remove_cvref_t<tuple_element_t<_is, Fields>>>(),
          }...};
      for (size_t i = 1; i < arr.size(); ++i) {
        const auto key = arr[i];
        size_t j = i;
        while (j > 0 && key.name < arr[j - 1].name) {
          arr[j] = arr[j - 1];
          --j;
        }
        arr[j] = key;
      }
      return arr;
    }(std::make_index_sequence<num_fields>());

    for (size_t i = 0; i < num_fields;) {
      size_t j = i + 1;
      bool any_ser = infos[i].is_serialized;
      bool any_deser = infos[i].is_deserialized;
      while (j < num_fields && infos[j].name == infos[i].name) {
        if ((any_ser && infos[j].is_serialized) ||
            (any_deser && infos[j].is_deserialized)) {
          return false;
        }
        any_ser = any_ser || infos[j].is_serialized;
        any_deser = any_deser || infos[j].is_deserialized;
        ++j;
      }
      i = j;
    }
    return true;
  }
}

}  // namespace internal
}  // namespace rfl

#endif
