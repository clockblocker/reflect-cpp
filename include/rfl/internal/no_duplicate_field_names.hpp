#ifndef RFL_INTERNAL_NO_DUPLICATE_FIELD_NAMES_HPP_
#define RFL_INTERNAL_NO_DUPLICATE_FIELD_NAMES_HPP_

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Tuple.hpp"

namespace rfl {
namespace internal {

template <class Fields>
constexpr inline bool no_duplicate_field_names() {
  constexpr size_t num_fields = rfl::tuple_size_v<Fields>;

  if constexpr (num_fields <= 1) {
    return true;
  } else {
    constexpr auto names = []<size_t... _is>(std::index_sequence<_is...>) {
      std::array<std::string_view, sizeof...(_is)> arr{
          std::remove_cvref_t<tuple_element_t<_is, Fields>>::name_
              .string_view()...};
      for (size_t i = 1; i < arr.size(); ++i) {
        const auto key = arr[i];
        size_t j = i;
        while (j > 0 && key < arr[j - 1]) {
          arr[j] = arr[j - 1];
          --j;
        }
        arr[j] = key;
      }
      return arr;
    }(std::make_index_sequence<num_fields>());

    for (size_t i = 1; i < names.size(); ++i) {
      if (names[i] == names[i - 1]) {
        return false;
      }
    }
    return true;
  }
}

}  // namespace internal
}  // namespace rfl

#endif
