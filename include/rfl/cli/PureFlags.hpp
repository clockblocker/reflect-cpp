#ifndef RFL_CLI_PUREFLAGS_HPP_
#define RFL_CLI_PUREFLAGS_HPP_

#include <type_traits>

#include "../Processors.hpp"

namespace rfl::cli {

/// A setting for rfl::cli::read and rfl::cli::read_app that prevents short
/// boolean flags from consuming a following `true`, `false`, `1`, or `0` token
/// as their value. Without it, `-v 1` sets `verbose = true` and consumes `1`,
/// whereas `--verbose 1` leaves `1` as a positional argument. With PureFlags,
/// both short and long boolean flags behave as pure switches and never consume
/// the next token (explicit `=` syntax like `--verbose=false` and `-v=false`
/// still works).
///
/// This is essential for CLIs whose positional arguments can be numeric IDs
/// such as `0` or `1` (e.g., `tool in.osm -H 1`).
///
/// This is a marker type (doesn't modify data) that only rfl::cli looks at.
/// Other formats ignore it.
struct PureFlags {
 public:
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

namespace internal {

template <class T>
struct pure_flags : std::false_type {};

template <>
struct pure_flags<PureFlags> : std::true_type {};

template <class... Ps>
struct pure_flags<Processors<Ps...>>
    : std::bool_constant<(pure_flags<Ps>::value || ...)> {};

/// Whether the processors contain rfl::cli::PureFlags.
template <class T>
constexpr bool pure_flags_v =
    pure_flags<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace internal

}  // namespace rfl::cli

#endif
