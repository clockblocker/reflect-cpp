#ifndef RFL_TOML_LOAD_HPP_
#define RFL_TOML_LOAD_HPP_

#include <utility>

#include "../Result.hpp"
#include "../io/load_string.hpp"
#include "read.hpp"

namespace rfl::toml {

/// Loads an object from a TOML file.
/// Reads a file from disk and parses its TOML content into a C++ object using compile-time reflection.
/// @tparam T The type to parse into
/// @tparam Ps Processors to apply during parsing (transforms the data)
/// @param _fname The filename/path of the TOML file to load
/// @return Result containing either the parsed object of type T or an error message
template <class T, class... Ps>
Result<T> load(const std::string& _fname) {
  const auto read_string = [](const auto& _str) {
    return read<T, Ps...>(_str);
  };
  return rfl::io::load_string(_fname).and_then(read_string);
}

/// Loads a TOML file from disk and overlays it onto an existing object.
/// Missing fields preserve values from `_base`; atomic on error.
/// @tparam Ps Processors to apply during parsing
/// @tparam T The type to overlay onto (deduced from `_base`)
/// @param _base The base object to overlay onto
/// @param _fname The filename/path of the TOML file to load
/// @return Result containing either the overlaid object or an error message
template <class... Ps, class T>
Result<T> load_into(T _base, const std::string& _fname) {
  auto str = rfl::io::load_string(_fname);
  if (!str) {
    return error(std::move(str).error());
  }
  return read_into<Ps...>(std::move(_base), std::string_view(*str));
}

}  // namespace rfl::toml

#endif
