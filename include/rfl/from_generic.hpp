#ifndef RFL_FROM_GENERIC_HPP_
#define RFL_FROM_GENERIC_HPP_

#include <utility>

#include "Generic.hpp"
#include "Result.hpp"
#include "generic/read.hpp"

namespace rfl {

/// Generates the struct T from a named tuple.
template <class T, class... Ps>
auto from_generic(const Generic& _g) {
  return rfl::generic::read<T, Ps...>(_g);
}

/// Overlays a Generic value onto an existing base object, preserving missing
/// fields and recursively overlaying nested structs. Atomic on error.
template <class... Ps, class T>
Result<T> from_generic_into(T _base, const Generic& _g) {
  return rfl::generic::read_into<Ps...>(std::move(_base), _g);
}

}  // namespace rfl

#endif
