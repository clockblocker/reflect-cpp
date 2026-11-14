#ifndef RFL_CLI_KEEPEMPTYITEMS_HPP_
#define RFL_CLI_KEEPEMPTYITEMS_HPP_

#include <type_traits>

#include "../Processors.hpp"

namespace rfl::cli {

/// A setting that keeps the empty items of split lists (`--tags=a,,b`),
/// reading each one as a missing value at its index. The element type then
/// decides, as it does for a missing field: std::optional<T> becomes
/// std::nullopt, rfl::DefaultVal<T> becomes T{}, std::shared_ptr<T> and
/// std::unique_ptr<T> become null, and a required type, such as std::string,
/// std::wstring or int, is an error. Without this setting, empty items are
/// skipped.
/// The index counts the items of every occurrence of a repeated list option,
/// or of a list positional argument. With ListDelimiter<"">, lists are not
/// split: every occurrence is one item as it is, even if it is empty, and
/// this setting changes nothing.
/// This is a marker type (doesn't modify data) and other formats ignore it.
/// Usage: rfl::cli::read<MyStruct, rfl::cli::KeepEmptyItems>(argc, argv)
struct KeepEmptyItems {
 public:
  /// Identity process function - returns the named tuple unchanged.
  /// The empty items are handled by the reader, not here.
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
struct keep_empty_items : std::false_type {};

template <>
struct keep_empty_items<KeepEmptyItems> : std::true_type {};

template <class... Ps>
struct keep_empty_items<Processors<Ps...>>
    : std::bool_constant<(keep_empty_items<Ps>::value || ...)> {};

/// Whether the processors contain rfl::cli::KeepEmptyItems.
template <class T>
constexpr bool keep_empty_items_v = keep_empty_items<
    std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace internal

}  // namespace rfl::cli

#endif
