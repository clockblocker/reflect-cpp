#ifndef RFL_PARSING_FIND_FIELD_INDEX_HPP_
#define RFL_PARSING_FIND_FIELD_INDEX_HPP_

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>

#include "../Literal.hpp"
#include "../internal/StringLiteral.hpp"

namespace rfl::parsing::detail {

struct FieldNameEntry {
  std::string_view name;
  int field_idx;
};

/// Compile-time field name lookup keyed solely on a NamedTuple's `Names`
/// (`rfl::Literal<_names...>`), so the lookup table is independent of field
/// value types (e.g., `rfl::Description` wrappers) and shared across
/// `ViewReader`, `ViewReaderWithDefault`, readers, writers, and processors.
template <class NamesLiteral>
struct FieldNameLookup;

template <internal::StringLiteral... _names>
struct FieldNameLookup<Literal<_names...>> {
  static constexpr size_t size_ = sizeof...(_names);

  static constexpr auto make_table() {
    std::array<FieldNameEntry, size_> table{};
    int idx = 0;
    ((table[static_cast<size_t>(idx)] =
          FieldNameEntry{_names.string_view(), idx},
      ++idx),
     ...);
    if constexpr (size_ > 8) {
      std::sort(table.begin(), table.end(),
                [](const FieldNameEntry& _a, const FieldNameEntry& _b) {
                  return _a.name < _b.name;
                });
    }
    return table;
  }

  static int find(const std::string_view _name) noexcept {
    if constexpr (size_ == 0) {
      return -1;
    } else {
      static constexpr auto table = make_table();
      if constexpr (size_ <= 8) {
        for (const auto& entry : table) {
          if (entry.name == _name) {
            return entry.field_idx;
          }
        }
        return -1;
      } else {
        size_t lo = 0;
        size_t hi = table.size();
        while (lo < hi) {
          const size_t mid = lo + (hi - lo) / 2;
          if (table[mid].name < _name) {
            lo = mid + 1;
          } else {
            hi = mid;
          }
        }
        if (lo < table.size() && table[lo].name == _name) {
          return table[lo].field_idx;
        }
        return -1;
      }
    }
  }
};

}  // namespace rfl::parsing::detail

#endif
