#ifndef RFL_CLI_LISTDELIMITER_HPP_
#define RFL_CLI_LISTDELIMITER_HPP_

#include <cstddef>
#include <optional>
#include <type_traits>

#include "../Processors.hpp"
#include "../internal/StringLiteral.hpp"

namespace rfl::cli {

/// A setting for rfl::cli::read that sets the character at which the values
/// of list options and list positional arguments are split into items. The
/// default is ",": `--report=a,b` gives two items. ListDelimiter<";">
/// splits at ';' instead, and ListDelimiter<""> does not split, so every
/// value, and every positional argument, is exactly one item. The setting
/// applies to every list in the struct.
/// The delimiter is a single ASCII character, as in clap and CLI11. Several
/// characters would make the splitting of a value such as "a;;;b" with ";;"
/// ambiguous; repeat the option instead.
/// This is a marker type (doesn't modify data) that only rfl::cli::read looks
/// at. Other formats ignore it.
/// Usage: rfl::cli::read<Config, rfl::cli::ListDelimiter<";">>(argc, argv)
template <rfl::internal::StringLiteral _delimiter>
struct ListDelimiter {
  static_assert(_delimiter.length == 0
                    || (_delimiter.length == 1 && _delimiter.arr_[0] != '\0'
                        && static_cast<unsigned char>(_delimiter.arr_[0])
                               < 0x80),
                "rfl::cli::ListDelimiter takes a single ASCII character, "
                "such as \";\", or \"\" to not split lists.");

  /// The character at which list values are split, or std::nullopt if they
  /// are not split.
  static constexpr std::optional<char> delimiter =
      _delimiter.length == 0 ? std::nullopt
                             : std::optional<char>(_delimiter.arr_[0]);

  /// Identity process function - returns the named tuple unchanged.
  /// @tparam StructType The struct type being processed
  /// @param _named_tuple The named tuple representation of the struct
  /// @return The same named tuple (unchanged)
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

namespace internal {

/// `count` is the number of ListDelimiters in T, and `value` the delimiter
/// of the one there is.
template <class T>
struct list_delimiter {
  static constexpr size_t count = 0;
  static constexpr std::optional<char> value = std::nullopt;
};

template <rfl::internal::StringLiteral _delimiter>
struct list_delimiter<ListDelimiter<_delimiter>> {
  static constexpr size_t count = 1;
  static constexpr std::optional<char> value =
      ListDelimiter<_delimiter>::delimiter;
};

template <class... Ps>
struct list_delimiter<Processors<Ps...>> {
  static constexpr size_t count = (list_delimiter<Ps>::count + ... + 0);
  static_assert(count <= 1, "Pass at most one rfl::cli::ListDelimiter.");
  // The other processors have no value, so this is the one ListDelimiter's.
  static constexpr char code = static_cast<char>(
      (list_delimiter<Ps>::value.value_or('\0') + ... + 0));
  static constexpr std::optional<char> value =
      code == '\0' ? std::nullopt : std::optional<char>(code);
};

/// The character at which the processors say to split list values (',',
/// rfl::cli::array_delimiter, by default), or std::nullopt if they are not
/// split (ListDelimiter<"">).
template <class T>
constexpr std::optional<char> list_delimiter_v =
    list_delimiter<std::remove_cvref_t<std::remove_pointer_t<T>>>::count == 0
        ? std::optional<char>(',')
        : list_delimiter<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace internal

}  // namespace rfl::cli

#endif
