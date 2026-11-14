#ifndef RFL_CLI_OPTIONSFIRST_HPP_
#define RFL_CLI_OPTIONSFIRST_HPP_

#include <type_traits>

#include "../Processors.hpp"

namespace rfl::cli {

/// A setting for rfl::cli::read that ends the options at the first
/// positional argument, like getopt's '+' mode (POSIXLY_CORRECT),
/// argparse's REMAINDER and clap's trailing_var_arg. Every later token is a
/// positional argument, taken as it is, even if it looks like an option:
/// `--x`, `-x`, unknown options and `--`. A `--` before the first
/// positional argument still ends the options and is dropped. Without
/// OptionsFirst, options and positional arguments can be mixed.
/// With a list as the last rfl::Positional field and ListDelimiter<"">, the
/// list gets the remaining arguments unchanged:
///   struct Recipe {
///     rfl::Positional<std::string> name;
///     rfl::Positional<std::vector<std::string>> args;
///   };
///   rfl::cli::read<Recipe, rfl::cli::OptionsFirst, rfl::cli::ListDelimiter<"">>
///   // recipe build --jobs 4 -- x  ->  name = "build",
///   //                                 args = {"--jobs", "4", "--", "x"}
/// Without ListDelimiter<"">, the list splits each argument at the commas
/// and drops empty items, like any other list:
///   rfl::cli::read<Recipe, rfl::cli::OptionsFirst>
///   // recipe build --jobs 4,5 "" x  ->  args = {"--jobs", "4", "5", "x"}
/// This is a marker type (doesn't modify data) that only rfl::cli::read looks
/// at. Other formats ignore it.
/// Usage: rfl::cli::read<Config, rfl::cli::OptionsFirst>(argc, argv)
struct OptionsFirst {
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
struct options_first : std::false_type {};

template <>
struct options_first<OptionsFirst> : std::true_type {};

template <class... Ps>
struct options_first<Processors<Ps...>>
    : std::bool_constant<(options_first<Ps>::value || ...)> {};

/// Whether the processors contain rfl::cli::OptionsFirst.
template <class T>
constexpr bool options_first_v =
    options_first<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace internal

}  // namespace rfl::cli

#endif
