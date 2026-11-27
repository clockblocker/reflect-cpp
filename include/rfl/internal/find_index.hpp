#ifndef RFL_FIND_INDEX_HPP_
#define RFL_FIND_INDEX_HPP_

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Tuple.hpp"
#include "StringLiteral.hpp"
#include "is_skip.hpp"

namespace rfl {
namespace internal {

namespace find_index_helpers {

struct MatchResult {
  int live_idx;
  int live_count;
  int skipped_idx;
  int skipped_count;
};

template <class FieldType>
constexpr bool field_is_fully_skipped() {
  if constexpr (requires { typename FieldType::Type; }) {
    return is_fully_skipped_v<typename FieldType::Type>;
  } else {
    return false;
  }
}

template <StringLiteral _field_name, class Fields>
constexpr MatchResult match_fields() {
  constexpr size_t size = rfl::tuple_size_v<Fields>;
  if constexpr (size == 0) {
    return MatchResult{-1, 0, -1, 0};
  } else {
    return []<int... _is>(std::integer_sequence<int, _is...>) {
      constexpr std::array<std::string_view, size> names{
          std::remove_cvref_t<rfl::tuple_element_t<_is, Fields>>::name_
              .string_view()...};
      constexpr std::array<bool, size> skipped{
          field_is_fully_skipped<
              std::remove_cvref_t<rfl::tuple_element_t<_is, Fields>>>()...};
      MatchResult res{-1, 0, -1, 0};
      for (size_t i = 0; i < size; ++i) {
        if (names[i] == _field_name.string_view()) {
          if (skipped[i]) {
            if (res.skipped_count == 0) {
              res.skipped_idx = static_cast<int>(i);
            }
            ++res.skipped_count;
          } else {
            if (res.live_count == 0) {
              res.live_idx = static_cast<int>(i);
            }
            ++res.live_count;
          }
        }
      }
      return res;
    }(std::make_integer_sequence<int, static_cast<int>(size)>());
  }
}

}  // namespace find_index_helpers

/// Finds the index of the field signified by _field_name
template <StringLiteral _field_name, class Fields>
constexpr int find_index() {
  constexpr auto res = find_index_helpers::match_fields<_field_name, Fields>();
  static_assert(res.live_count + res.skipped_count > 0,
                "No matching field found.");
  static_assert(res.live_count <= 1,
                "Ambiguous field name: multiple active fields share this "
                "name. Access by index instead.");
  static_assert(res.live_count == 1 || res.skipped_count <= 1,
                "Ambiguous field name: multiple skipped fields share this "
                "name. Access by index instead.");
  if constexpr (res.live_count == 1) {
    return res.live_idx;
  } else {
    return res.skipped_idx;
  }
}

/// Finds the index of the field signified by _field_name or -1.
template <StringLiteral _field_name, class Fields>
constexpr int find_index_or_minus_one() {
  constexpr auto res = find_index_helpers::match_fields<_field_name, Fields>();
  if constexpr (res.live_count > 0) {
    return res.live_idx;
  } else if constexpr (res.skipped_count > 0) {
    return res.skipped_idx;
  } else {
    return -1;
  }
}

}  // namespace internal
}  // namespace rfl

#endif
