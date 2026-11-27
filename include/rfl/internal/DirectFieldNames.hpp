#ifndef RFL_INTERNAL_DIRECTFIELDNAMES_HPP_
#define RFL_INTERNAL_DIRECTFIELDNAMES_HPP_

#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../num_fields.hpp"
#include "../parsing/find_field_index.hpp"
#include "get_field_names.hpp"
#ifndef REFLECTCPP_USE_CPP26_REFLECTION
#include "cpp20/get_ith_field_from_fake_object.hpp"
#endif

namespace rfl::internal {

/// Non-templated runtime view over a struct's field names and optional sorted
/// lookup table (`lookup == nullptr` when `size <= 8`, where linear scan over
/// `names` avoids emitting a separate sorted table in `.rodata`).
struct DirectFieldNamesDesc {
  const std::string_view* names;
  const parsing::detail::FieldNameEntry* lookup;
  std::size_t size;

  constexpr int find(const std::string_view _name) const noexcept {
    if (size <= 8) {
      for (std::size_t i = 0; i < size; ++i) {
        if (names[i] == _name) {
          return static_cast<int>(i);
        }
      }
      return -1;
    }
    std::size_t lo = 0;
    std::size_t hi = size;
    while (lo < hi) {
      const auto mid = lo + (hi - lo) / 2;
      if (lookup[mid].name < _name) {
        lo = mid + 1;
      } else {
        hi = mid;
      }
    }
    return lo < size && lookup[lo].name == _name ? lookup[lo].field_idx : -1;
  }

  constexpr std::span<const std::string_view> span() const noexcept {
    return {names, size};
  }
};

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-var-template"
#pragma clang diagnostic ignored "-Wundefined-inline"
#endif

/// Value-level field name storage and lookup table keyed solely by struct type
/// `T` — never by `StringLiteral` non-type template parameters. All names are
/// copied into a compact null-terminated char buffer so `__PRETTY_FUNCTION__`
/// strings never leak into `.rodata` and every `std::string_view` is
/// null-terminated.
template <class T>
struct DirectFieldNames {
  using Type = std::remove_cvref_t<T>;
  static constexpr std::size_t size_ = num_fields<Type>;

 private:
  // Intentionally invoked inside consteval helpers rather than stored as a
  // `static constexpr` data member of raw `std::string_view`s so compiler
  // `__PRETTY_FUNCTION__` strings are never retained in `.rodata`.
#if __GNUC__
#ifndef __clang__
  [[gnu::no_sanitize_undefined]]
#endif
#endif
  static consteval auto extract_names() {
    return []<std::size_t... Is>(std::index_sequence<Is...>) {
#ifndef REFLECTCPP_USE_CPP26_REFLECTION
#ifdef __clang__
      return std::array<std::string_view, sizeof...(Is)>{
          cpp20::get_direct_field_name<
              Type, cpp20::wrap(
                        cpp20::get_ith_field_from_fake_object<Type, Is>())>()...};
#else
      return std::array<std::string_view, sizeof...(Is)>{
          cpp20::get_direct_field_name<
              Type, cpp20::get_ith_field_from_fake_object<Type, Is>()>()...};
#endif
#else
      return std::array<std::string_view, sizeof...(Is)>{
          cpp26::get_direct_field_name<Type, Is>()...};
#endif
    }(std::make_index_sequence<size_>{});
  }

  static consteval auto compute_lengths() {
    const auto src = extract_names();
    std::array<std::size_t, size_> lens{};
    for (std::size_t i = 0; i < size_; ++i) {
      lens[i] = src[i].size();
    }
    return lens;
  }

  static constexpr auto lengths_ = compute_lengths();

  static consteval std::size_t buffer_size() {
    std::size_t total = size_;
    for (const auto len : lengths_) {
      total += len;
    }
    return total;
  }

  static consteval auto make_buffer() {
    const auto src = extract_names();
    std::array<char, buffer_size()> buffer{};
    std::size_t offset = 0;
    for (const auto name : src) {
      for (const char ch : name) {
        buffer[offset++] = ch;
      }
      buffer[offset++] = '\0';
    }
    return buffer;
  }

  static constexpr auto buffer_ = make_buffer();

  static consteval auto make_names() {
    std::array<std::string_view, size_> names{};
    std::size_t offset = 0;
    for (std::size_t i = 0; i < size_; ++i) {
      names[i] = std::string_view(buffer_.data() + offset, lengths_[i]);
      offset += lengths_[i] + 1;
    }
    return names;
  }

  static consteval auto make_sorted_lookup() {
    if constexpr (size_ <= 8) {
      return std::array<parsing::detail::FieldNameEntry, 0>{};
    } else {
      std::array<parsing::detail::FieldNameEntry, size_> table{};
      for (std::size_t i = 0; i < size_; ++i) {
        table[i] = {names_[i], static_cast<int>(i)};
      }
      parsing::detail::sort_field_name_table(table);
      return table;
    }
  }

  static consteval bool has_unique_names() {
    if constexpr (size_ <= 8) {
      for (std::size_t i = 0; i < size_; ++i) {
        for (std::size_t j = i + 1; j < size_; ++j) {
          if (names_[i] == names_[j]) {
            return false;
          }
        }
      }
      return true;
    } else {
      for (std::size_t i = 1; i < size_; ++i) {
        if (lookup_[i - 1].name == lookup_[i].name) {
          return false;
        }
      }
      return true;
    }
  }

 public:
  static constexpr auto names_ = make_names();
  static constexpr auto lookup_ = make_sorted_lookup();
  static_assert(has_unique_names(), "Duplicate field names are not allowed.");

  static constexpr DirectFieldNamesDesc desc_{
      names_.data(), size_ <= 8 ? nullptr : lookup_.data(), size_};

  static constexpr int find(const std::string_view _name) noexcept {
    return desc_.find(_name);
  }
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif

}  // namespace rfl::internal

#endif
