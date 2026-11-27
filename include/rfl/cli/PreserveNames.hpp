#ifndef RFL_CLI_PRESERVENAMES_HPP_
#define RFL_CLI_PRESERVENAMES_HPP_

#include <type_traits>

#include "../Processors.hpp"
#include "../SnakeCaseToKebabCase.hpp"

namespace rfl::cli {

/// Processor that disables the automatic `rfl::SnakeCaseToKebabCase` conversion
/// applied by `rfl::cli::read`, `rfl::cli::read_detailed`, `rfl::cli::read_app`,
/// and `rfl::cli::usage`, preserving C++ field names verbatim (e.g.
/// `--database_path`, `--SiftExtraction.use_gpu`).
struct PreserveNames {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

namespace internal {

template <class ProcessorsType>
struct preserve_names;

template <template <class...> class ProcessorsType, class... Ps>
struct preserve_names<ProcessorsType<Ps...>> {
  static constexpr bool value = (std::is_same_v<PreserveNames, Ps> || ...);
};

template <class ProcessorsType>
constexpr bool preserve_names_v = preserve_names<ProcessorsType>::value;

/// Flattens CLI processors, prepending `SnakeCaseToKebabCase` unless
/// `PreserveNames` is present.
template <class... Ps>
using cli_processors_t = std::conditional_t<
    preserve_names_v<rfl::internal::flatten_processors_t<Ps...>>,
    rfl::internal::flatten_processors_t<Ps...>,
    rfl::internal::flatten_processors_t<SnakeCaseToKebabCase, Ps...>>;

}  // namespace internal

}  // namespace rfl::cli

#endif
