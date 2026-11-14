#ifndef RFL_CLI_ALLOWABBREVIATIONS_HPP_
#define RFL_CLI_ALLOWABBREVIATIONS_HPP_

#include <type_traits>

#include "../Processors.hpp"

namespace rfl::cli {

/// A processor that turns on abbreviated long options in rfl::cli::read,
/// like argparse's allow_abbrev. With AllowAbbreviations, a long option may
/// be shortened to any prefix that matches exactly one option, so `--rep`
/// stands for `--report`. By default, a long option must be spelled out, and
/// `--rep` is an unknown option.
/// This is a marker type (doesn't modify data) that only rfl::cli::read looks
/// at. Other formats ignore it.
/// Usage: rfl::cli::read<Config, rfl::cli::AllowAbbreviations>(argc, argv)
struct AllowAbbreviations {
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
struct allow_abbreviations : std::false_type {};

template <>
struct allow_abbreviations<AllowAbbreviations> : std::true_type {};

template <class... Ps>
struct allow_abbreviations<Processors<Ps...>>
    : std::bool_constant<(allow_abbreviations<Ps>::value || ...)> {};

/// Whether the processors contain rfl::cli::AllowAbbreviations.
template <class T>
constexpr bool allow_abbreviations_v =
    allow_abbreviations<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace internal

}  // namespace rfl::cli

#endif
