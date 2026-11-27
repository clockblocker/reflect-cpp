#ifndef RFL_CLI_REQUIRE_EQUALS_HPP_
#define RFL_CLI_REQUIRE_EQUALS_HPP_

#include <type_traits>

#include "../Processors.hpp"

namespace rfl::cli {

/// A setting that requires long options taking a value (`OptionKind::value`)
/// to be given with `=` (`--key=value`) when `SeparateOptionValues` is not
/// enabled, failing with a missing-value error if bare `--key` is passed.
/// Flags and `rfl::ImplicitValue` options still work without `=`.
struct RequireEquals {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

namespace internal {

template <class T>
struct require_equals : std::false_type {};

template <>
struct require_equals<RequireEquals> : std::true_type {};

template <class... Ps>
struct require_equals<Processors<Ps...>>
    : std::disjunction<require_equals<Ps>...> {};

template <class T>
constexpr bool require_equals_v =
    require_equals<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace internal

}  // namespace rfl::cli

#endif
