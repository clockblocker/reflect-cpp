#ifndef RFL_MAXDEPTH_HPP_
#define RFL_MAXDEPTH_HPP_

#include <cstddef>
#include <utility>

namespace rfl {

/// Opt-in processor that bounds container and struct nesting depth during
/// deserialization to prevent stack-overflow DoS on deeply nested untrusted
/// inputs (such as `rfl::Generic` or recursive `rfl::Box<Node>` structs).
///
/// Behavior:
/// - Zero runtime overhead when not passed (`if constexpr`).
/// - Applies uniformly across all formats (JSON, YAML, TOML, XML, CBOR, etc.).
/// - Returns an `rfl::Error` ("Maximum nesting depth of N exceeded.") instead
///   of overflowing the C++ call stack.
/// - Thread-safe and exception-safe (uses a thread-local RAII depth counter).
///
/// Example:
///   auto res = rfl::json::read<rfl::Generic, rfl::MaxDepth<64>>(untrusted);
template <std::size_t _max_depth>
struct MaxDepth {
  static_assert(_max_depth > 0,
                "rfl::MaxDepth requires a positive depth limit.");

  static constexpr std::size_t value = _max_depth;

  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return std::forward<decltype(_named_tuple)>(_named_tuple);
  }
};

}  // namespace rfl

#endif
