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
    }(std::make_integer_sequence<int, size>());
  }
}

struct FieldMatchMeta {
  std::string_view name;
  bool skip_serialization;
  bool skip_deserialization;
};

template <class FieldType>
constexpr bool field_skip_serialization() {
  if constexpr (requires { typename FieldType::Type; }) {
    return skip_serialization_v<typename FieldType::Type>;
  } else {
    return false;
  }
}

template <class FieldType>
constexpr bool field_skip_deserialization() {
  if constexpr (requires { typename FieldType::Type; }) {
    return skip_deserialization_v<typename FieldType::Type>;
  } else {
    return false;
  }
}

template <class Fields>
constexpr auto extract_field_match_meta() {
  constexpr size_t size = rfl::tuple_size_v<Fields>;
  return []<int... _is>(std::integer_sequence<int, _is...>) {
    return std::array<FieldMatchMeta, size>{FieldMatchMeta{
        std::remove_cvref_t<rfl::tuple_element_t<_is, Fields>>::name_
            .string_view(),
        field_skip_serialization<
            std::remove_cvref_t<rfl::tuple_element_t<_is, Fields>>>(),
        field_skip_deserialization<
            std::remove_cvref_t<rfl::tuple_element_t<_is, Fields>>>(),
    }...};
  }(std::make_integer_sequence<int, size>());
}

struct FieldMatchResult {
  int idx;
  int name_match_count;
  int target_name_match_count;
  int exact_match_count;
  int target_exact_count;
};

template <int _target_idx, class TargetFields, class SourceFields>
constexpr FieldMatchResult match_field_for_conversion() {
  constexpr size_t target_size = rfl::tuple_size_v<TargetFields>;
  constexpr size_t source_size = rfl::tuple_size_v<SourceFields>;
  if constexpr (source_size == 0) {
    return FieldMatchResult{-1, 0, 0, 0, 0};
  } else {
    constexpr auto target_meta = extract_field_match_meta<TargetFields>();
    constexpr auto source_meta = extract_field_match_meta<SourceFields>();
    const auto& target = target_meta[_target_idx];

    int target_name_match_count = 0;
    int target_exact_count = 0;
    int target_occurrence = 0;
    for (size_t i = 0; i < target_size; ++i) {
      if (target_meta[i].name == target.name) {
        ++target_name_match_count;
        if (target_meta[i].skip_serialization == target.skip_serialization &&
            target_meta[i].skip_deserialization ==
                target.skip_deserialization) {
          if (static_cast<int>(i) < _target_idx) {
            ++target_occurrence;
          }
          ++target_exact_count;
        }
      }
    }

    int name_match_count = 0;
    int first_name_match = -1;
    int exact_match_count = 0;
    int exact_match_idx = -1;
    for (size_t j = 0; j < source_size; ++j) {
      if (source_meta[j].name == target.name) {
        if (name_match_count == 0) {
          first_name_match = static_cast<int>(j);
        }
        ++name_match_count;
        if (source_meta[j].skip_serialization == target.skip_serialization &&
            source_meta[j].skip_deserialization ==
                target.skip_deserialization) {
          if (exact_match_count == target_occurrence) {
            exact_match_idx = static_cast<int>(j);
          }
          ++exact_match_count;
        }
      }
    }

    if (name_match_count == 1 && target_name_match_count == 1) {
      return FieldMatchResult{first_name_match, name_match_count,
                              target_name_match_count, exact_match_count,
                              target_exact_count};
    }

    if (exact_match_count == 1 && target_exact_count == 1) {
      return FieldMatchResult{exact_match_idx, name_match_count,
                              target_name_match_count, exact_match_count,
                              target_exact_count};
    }

    if (exact_match_count > 1 && exact_match_count == target_exact_count) {
      return FieldMatchResult{exact_match_idx, name_match_count,
                              target_name_match_count, exact_match_count,
                              target_exact_count};
    }

    return FieldMatchResult{-1, name_match_count, target_name_match_count,
                            exact_match_count, target_exact_count};
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

/// Finds the index in SourceFields matching field _target_idx of TargetFields,
/// disambiguating shared wire names by (skip_serialization,
/// skip_deserialization) and occurrence order.
template <int _target_idx, class TargetFields, class SourceFields>
constexpr int find_matching_field_index() {
  constexpr auto res =
      find_index_helpers::match_field_for_conversion<_target_idx, TargetFields,
                                                     SourceFields>();
  static_assert(res.name_match_count > 0, "No matching field found.");
  static_assert(res.idx != -1,
                "Could not uniquely match field by name and skip traits.");
  return res.idx;
}

}  // namespace internal
}  // namespace rfl

#endif
