#ifndef RFL_CLI_SINGLEDASHLONG_HPP_
#define RFL_CLI_SINGLEDASHLONG_HPP_

#include <type_traits>

namespace rfl::cli {

/// Processor telling the CLI parser to accept single-dash long options
/// (`-datadir=/tmp`, `-lint-only`) as equivalent to double-dash long options
/// (`--datadir=/tmp`, `--lint-only`), as used by Bitcoin Core, Verilator, and
/// classic compiler/linker CLIs.
struct SingleDashLong {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

namespace internal {

template <class ProcessorsType>
struct single_dash_long;

template <template <class...> class ProcessorsType, class... Ps>
struct single_dash_long<ProcessorsType<Ps...>> {
  static constexpr bool value =
      (std::is_same_v<SingleDashLong, Ps> || ...);
};

template <class ProcessorsType>
constexpr bool single_dash_long_v = single_dash_long<ProcessorsType>::value;

}  // namespace internal

}  // namespace rfl::cli

#endif
