#ifndef GENERIC_READ_HPP_
#define GENERIC_READ_HPP_


#include <utility>

#include "../Generic.hpp"
#include "../Processors.hpp"
#include "../Result.hpp"
#include "../parsing/Parser_base.hpp"
#include "Parser.hpp"

namespace rfl {
namespace generic {

/// Parses a C++ object from a Generic value using reflection.
/// This is useful for converting between different serialization formats:
/// format A → Generic → format B, providing format independence.
/// @tparam T The C++ type to deserialize into
/// @tparam Ps Optional processors to apply during deserialization
/// @param _g The Generic value to parse
/// @return A Result containing the deserialized object or an error
template <class T, class... Ps>
auto read(const Generic& _g) {
  const auto r = Reader();
  return Parser<T, Processors<Ps...>>::read(r, _g);
}

/// Overlays a Generic value onto an existing C++ object, preserving fields
/// absent from `_g` and recursively overlaying nested structs.
/// Atomic on error: `_base` is taken by value and only returned on success.
/// @tparam Ps Optional processors to apply during deserialization
/// @tparam T The C++ type to overlay onto (deduced from `_base`)
/// @param _base The base object to overlay onto
/// @param _g The Generic value to overlay
/// @return A Result containing the merged object or an error
template <class... Ps, class T>
Result<T> read_into(T _base, const Generic& _g) {
  const auto r = Reader();
  return parsing::read_into_or_read<Reader, Writer, T, Processors<Ps...>>(
      r, _g, std::move(_base));
}

}  // namespace generic
}  // namespace rfl

#endif
