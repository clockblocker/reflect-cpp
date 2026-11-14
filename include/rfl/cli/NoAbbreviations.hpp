#ifndef RFL_CLI_NOABBREVIATIONS_HPP_
#define RFL_CLI_NOABBREVIATIONS_HPP_

#include <type_traits>

#include "../Processors.hpp"

namespace rfl::cli {

/// A processor that turns off abbreviated long options in rfl::cli::read,
/// like argparse's allow_abbrev=False. By default, a long option may be
/// shortened to any prefix that matches exactly one option, so `--rep`
/// stands for `--report`. With NoAbbreviations, a long option must be
/// spelled out, and `--rep` is an unknown option.
/// This is a marker type (doesn't modify data) that only rfl::cli::read looks
/// at. Other formats ignore it.
/// Usage: rfl::cli::read<Config, rfl::cli::NoAbbreviations>(argc, argv)
struct NoAbbreviations {
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
struct no_abbreviations : std::false_type {};

template <>
struct no_abbreviations<NoAbbreviations> : std::true_type {};

template <class... Ps>
struct no_abbreviations<Processors<Ps...>>
    : std::bool_constant<(no_abbreviations<Ps>::value || ...)> {};

/// Whether the processors contain rfl::cli::NoAbbreviations.
template <class T>
constexpr bool no_abbreviations_v =
    no_abbreviations<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace internal

}  // namespace rfl::cli

#endif
