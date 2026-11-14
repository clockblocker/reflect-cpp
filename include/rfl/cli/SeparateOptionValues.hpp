#ifndef RFL_CLI_SEPARATEOPTIONVALUES_HPP_
#define RFL_CLI_SEPARATEOPTIONVALUES_HPP_

#include <type_traits>

#include "../Processors.hpp"

namespace rfl::cli {

/// A processor that lets a long option in rfl::cli::read take its value from
/// the next argument, as in getopt_long and argparse: `--sql "SELECT 1"` as
/// well as `--sql="SELECT 1"`. By default, a long option takes its value only
/// after '=', and `--sql "SELECT 1"` sets `sql` to an empty string and makes
/// "SELECT 1" a positional argument.
/// This is a marker type (doesn't modify data) that only rfl::cli::read looks
/// at. Other formats ignore it.
/// Usage: rfl::cli::read<Config, rfl::cli::SeparateOptionValues>(argc, argv)
struct SeparateOptionValues {
 public:
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

template <class T>
struct separate_option_values : std::false_type {};

template <>
struct separate_option_values<SeparateOptionValues> : std::true_type {};

template <class... Ps>
struct separate_option_values<Processors<Ps...>>
    : std::bool_constant<(separate_option_values<Ps>::value || ...)> {};

/// Whether the processors contain rfl::cli::SeparateOptionValues.
template <class T>
constexpr bool separate_option_values_v = separate_option_values<
    std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace internal

}  // namespace rfl::cli

#endif
