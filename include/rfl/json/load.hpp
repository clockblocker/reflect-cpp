#ifndef RFL_JSON_LOAD_HPP_
#define RFL_JSON_LOAD_HPP_

#include <utility>

#include "../Result.hpp"
#include "../io/load_string.hpp"
#include "read.hpp"

namespace rfl {
namespace json {

/// Loads an object from a JSON file.
/// Reads a file from disk and parses its JSON content into a C++ object using compile-time reflection.
/// @tparam T The type to parse into
/// @tparam Ps Processors to apply during parsing (transforms the data)
/// @param _fname The filename/path of the JSON file to load
/// @param _flag Optional yyjson flags for parsing (default: 0)
/// @return Result containing either the parsed object of type T or an error message
template <class T, class... Ps>
Result<T> load(const std::string& _fname, const yyjson_read_flag _flag = 0) {
  const auto read_string = [_flag](const auto& _str) {
    return read<T, Ps...>(_str, _flag);
  };
  return rfl::io::load_string(_fname).and_then(read_string);
}

/// Loads a JSON file from disk and overlays it onto an existing object.
/// Missing fields preserve values from `_base`; atomic on error.
/// @tparam Ps Processors to apply during parsing
/// @tparam T The type to overlay onto (deduced from `_base`)
/// @param _base The base object to overlay onto
/// @param _fname The filename/path of the JSON file to load
/// @param _flag Optional yyjson flags for parsing (default: 0)
/// @return Result containing either the overlaid object or an error message
template <class... Ps, class T>
Result<T> load_into(T _base, const std::string& _fname,
                    const yyjson_read_flag _flag = 0) {
  auto str = rfl::io::load_string(_fname);
  if (!str) {
    return error(std::move(str).error());
  }
  return read_into<Ps...>(std::move(_base), *str, _flag);
}

}  // namespace json
}  // namespace rfl

#endif
