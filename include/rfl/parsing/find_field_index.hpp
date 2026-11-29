#ifndef RFL_PARSING_FIND_FIELD_INDEX_HPP_
#define RFL_PARSING_FIND_FIELD_INDEX_HPP_

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

#include "../Literal.hpp"
#include "../internal/StringLiteral.hpp"
#include "../internal/always_inline.hpp"

namespace rfl::internal::parsing {

struct FieldNameEntry {
  std::string_view name;
  int field_idx;
};

RFL_ALWAYS_INLINE bool field_name_equals(const std::string_view _a,
                                         const std::string_view _b) noexcept {
  const std::size_t n = _a.size();
  if (n != _b.size()) {
    return false;
  }
  if (n == 0) {
    return true;
  }
  const char* p = _a.data();
  const char* q = _b.data();
  if (n >= 8) {
    if (n <= 16) {
      std::uint64_t a0 = 0;
      std::uint64_t b0 = 0;
      std::uint64_t a1 = 0;
      std::uint64_t b1 = 0;
      std::memcpy(&a0, p, 8);
      std::memcpy(&b0, q, 8);
      std::memcpy(&a1, p + n - 8, 8);
      std::memcpy(&b1, q + n - 8, 8);
      return ((a0 ^ b0) | (a1 ^ b1)) == 0;
    }
    return std::memcmp(p, q, n) == 0;
  }
  if (n >= 4) {
    std::uint32_t a0 = 0;
    std::uint32_t b0 = 0;
    std::uint32_t a1 = 0;
    std::uint32_t b1 = 0;
    std::memcpy(&a0, p, 4);
    std::memcpy(&b0, q, 4);
    std::memcpy(&a1, p + n - 4, 4);
    std::memcpy(&b1, q + n - 4, 4);
    return ((a0 ^ b0) | (a1 ^ b1)) == 0;
  }
  return p[0] == q[0] && p[n >> 1] == q[n >> 1] && p[n - 1] == q[n - 1];
}

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

/// Compile-time field name lookup keyed solely on a NamedTuple's `Names`
/// (`rfl::Literal<_names...>`), so the lookup table is independent of field
/// value types (e.g., `rfl::Description` wrappers) and shared across
/// `ViewReader`, `ViewReaderWithDefault`, readers, writers, and processors.
template <class NamesLiteral>
struct FieldNameLookup;

template <StringLiteral... _names>
struct FieldNameLookup<Literal<_names...>> {
  static constexpr std::size_t size_ = sizeof...(_names);

  static constexpr std::array<std::string_view, size_> decl_names_ = {
      _names.string_view()...};

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
      if constexpr (size_ <= 8) {
        for (const auto& entry : table) {
          if (field_name_equals(entry.name, _name)) {
            return entry.field_idx;
          }
        }
        return -1;
      } else {
        std::size_t lo = 0;
        std::size_t hi = table.size();
        while (lo < hi) {
          const std::size_t mid = lo + (hi - lo) / 2;
          if (table[mid].name < _name) {
            lo = mid + 1;
          } else {
            hi = mid;
          }
        }
        if (lo < table.size() && field_name_equals(table[lo].name, _name)) {
          return table[lo].field_idx;
        }
        return -1;
      }
    }
  }

  static RFL_ALWAYS_INLINE int find_with_hint(const std::string_view _name,
                                              std::size_t& _next_idx) noexcept {
    if constexpr (size_ == 0) {
      return -1;
    } else {
      if (_next_idx < size_ && field_name_equals(decl_names_[_next_idx], _name))
          [[likely]] {
        const int idx = static_cast<int>(_next_idx);
        ++_next_idx;
        return idx;
      }
      const int idx = find(_name);
      if (idx >= 0) {
        _next_idx = static_cast<std::size_t>(idx) + 1;
      }
      return idx;
    }
  }
};

}  // namespace rfl::internal::parsing

#endif
