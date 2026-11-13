#ifndef RFL_CLI_BLANKITEMSASMISSING_HPP_
#define RFL_CLI_BLANKITEMSASMISSING_HPP_

#include <type_traits>

#include "../Processors.hpp"

namespace rfl::cli {

/// A setting that keeps blank items in comma-separated lists, reading each
/// one as a missing value at its index. The element type then decides, as it
/// does for a missing field: std::optional<T> becomes std::nullopt,
/// rfl::DefaultVal<T> becomes T{}, std::shared_ptr<T> and std::unique_ptr<T>
/// become null, and a required type, such as std::string, std::wstring or
/// int, is an error. Without this setting, blank items are skipped.
/// The index counts the items of every occurrence of a repeated list option,
/// or of a list positional argument. With ListDelimiter<"">, an empty
/// occurrence is a blank item.
/// This is a marker type (doesn't modify data) and other formats ignore it.
/// Usage: rfl::cli::read<MyStruct, rfl::cli::BlankItemsAsMissing>(argc, argv)
struct BlankItemsAsMissing {
 public:
  /// Identity process function - returns the named tuple unchanged.
  /// The blank items are handled by the reader, not here.
  /// @tparam StructType The struct type being processed
  /// @param _named_tuple The named tuple representation of the struct
  /// @return The same named tuple (unchanged)
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

namespace internal {

template <class T>
struct blank_items_as_missing : std::false_type {};

template <>
struct blank_items_as_missing<BlankItemsAsMissing> : std::true_type {};

template <class... Ps>
struct blank_items_as_missing<Processors<Ps...>>
    : std::bool_constant<(blank_items_as_missing<Ps>::value || ...)> {};

template <class T>
constexpr bool blank_items_as_missing_v = blank_items_as_missing<
    std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace internal

}  // namespace rfl::cli

#endif
