#ifndef RFL_PARSING_FIND_FIELD_INDEX_HPP_
#define RFL_PARSING_FIND_FIELD_INDEX_HPP_

#include <array>
#include <cstddef>
#include <string_view>

#include "../Literal.hpp"
#include "../internal/StringLiteral.hpp"

namespace rfl::internal::parsing {

struct FieldNameEntry {
  std::string_view name;
  int field_idx;
};

/// O(N log N) constexpr heapsort for FieldNameEntry tables (avoids relying on
/// C++20 constexpr std::sort support in older libc++ releases).
template <std::size_t _n>
constexpr void sort_field_name_table(
    std::array<FieldNameEntry, _n>& _table) noexcept {
  auto sift_down = [&](std::size_t _root, const std::size_t _end) {
    while (true) {
      std::size_t child = _root * 2 + 1;
      if (child >= _end) {
        break;
      }
      if (child + 1 < _end && _table[child].name < _table[child + 1].name) {
        ++child;
      }
      if (_table[_root].name < _table[child].name) {
        const auto tmp = _table[_root];
        _table[_root] = _table[child];
        _table[child] = tmp;
        _root = child;
      } else {
        break;
      }
    }
  };
  for (std::size_t start = _n / 2; start > 0; --start) {
    sift_down(start - 1, _n);
  }
  for (std::size_t end = _n; end > 1; --end) {
    const auto tmp = _table[0];
    _table[0] = _table[end - 1];
    _table[end - 1] = tmp;
    sift_down(0, end - 1);
  }
}

/// Shared lookup helper operating on descriptor table slices (never templated
/// on `T` or `N`).
inline int find_in_table(const FieldNameEntry* _lookup, const std::size_t _size,
                         const std::string_view _name) noexcept {
  if (_size <= 8) {
    for (std::size_t i = 0; i < _size; ++i) {
      if (_lookup[i].name == _name) {
        return _lookup[i].field_idx;
      }
    }
    return -1;
  }
  std::size_t lo = 0;
  std::size_t hi = _size;
  while (lo < hi) {
    const std::size_t mid = lo + (hi - lo) / 2;
    if (_lookup[mid].name < _name) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  return (lo < _size && _lookup[lo].name == _name) ? _lookup[lo].field_idx : -1;
}

/// Compile-time field name lookup keyed solely on a NamedTuple's `Names`
/// (`rfl::Literal<_names...>`), so the lookup table is independent of field
/// value types (e.g., `rfl::Description` wrappers) and shared across
/// `ViewReader`, `ViewReaderWithDefault`, readers, writers, and processors.
template <class NamesLiteral>
struct FieldNameLookup;

template <StringLiteral... _names>
struct FieldNameLookup<Literal<_names...>> {
  static constexpr std::size_t size_ = sizeof...(_names);

  static constexpr auto make_table() {
    std::array<FieldNameEntry, size_> table{};
    int idx = 0;
    ((table[static_cast<std::size_t>(idx)] =
          FieldNameEntry{_names.string_view(), idx},
      ++idx),
     ...);
    if constexpr (size_ > 8) {
      sort_field_name_table(table);
    }
    return table;
  }

  static int find(const std::string_view _name) noexcept {
    if constexpr (size_ == 0) {
      return -1;
    } else {
      static constexpr auto table = make_table();
      return find_in_table(table.data(), table.size(), _name);
    }
  }
};

}  // namespace rfl::internal::parsing

#endif
