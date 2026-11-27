#ifndef RFL_CLI_BOOLVALUETOKENS_HPP_
#define RFL_CLI_BOOLVALUETOKENS_HPP_

#include <type_traits>

namespace rfl::cli {

/// Processor allowing long boolean flags to consume a separate following
/// boolean literal token (`--use_gpu 0`, `--use_gpu 1`, `--use_gpu true`,
/// `--use_gpu false`), matching Boost.Program_options `po::value<bool>()`.
/// Non-boolean following tokens are left untouched.
struct BoolValueTokens {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

namespace internal {

template <class ProcessorsType>
struct bool_value_tokens;

template <template <class...> class ProcessorsType, class... Ps>
struct bool_value_tokens<ProcessorsType<Ps...>> {
  static constexpr bool value = (std::is_same_v<BoolValueTokens, Ps> || ...);
};

template <class ProcessorsType>
constexpr bool bool_value_tokens_v = bool_value_tokens<ProcessorsType>::value;

}  // namespace internal

}  // namespace rfl::cli

#endif
