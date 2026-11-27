#ifndef RFL_CLI_LAST_VALUE_WINS_HPP_
#define RFL_CLI_LAST_VALUE_WINS_HPP_

#include <type_traits>

#include "../Processors.hpp"

namespace rfl::cli {

/// A setting that makes repeated non-list options overwrite their previous
/// value instead of failing with a duplicate/conflict error (last occurrence
/// in argv wins, as in GNU/compiler CLIs such as gcc, cppcheck, and aria2).
/// Use `rfl::cli::read_detailed` to also inspect every occurrence in argv order.
struct LastValueWins {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

namespace internal {

template <class T>
struct last_value_wins : std::false_type {};

template <>
struct last_value_wins<LastValueWins> : std::true_type {};

template <class... Ps>
struct last_value_wins<Processors<Ps...>>
    : std::disjunction<last_value_wins<Ps>...> {};

template <class T>
constexpr bool last_value_wins_v =
    last_value_wins<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace internal

}  // namespace rfl::cli

#endif
