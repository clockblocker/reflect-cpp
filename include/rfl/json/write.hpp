#ifndef RFL_JSON_WRITE_HPP_
#define RFL_JSON_WRITE_HPP_

#if __has_include(<yyjson.h>)
#include <yyjson.h>
#else
#include "../thirdparty/yyjson.h"
#endif

#include <ostream>
#include <stdexcept>
#include <string>

#include "../Processors.hpp"
#include "../parsing/Parent.hpp"
#include "Parser.hpp"

namespace rfl {
namespace json {

/// Write JSON pretty-printed with 4-space indentation.
inline constexpr yyjson_write_flag pretty = YYJSON_WRITE_PRETTY;

/// Write JSON pretty-printed with 2-space indentation.
/// Overrides `pretty` if both are set.
inline constexpr yyjson_write_flag pretty_two_spaces =
    YYJSON_WRITE_PRETTY_TWO_SPACES;

/// Escape non-ASCII Unicode characters as `\uXXXX` for ASCII-only output.
inline constexpr yyjson_write_flag escape_unicode = YYJSON_WRITE_ESCAPE_UNICODE;

/// Escape forward slashes `/` as `\/`.
inline constexpr yyjson_write_flag escape_slashes = YYJSON_WRITE_ESCAPE_SLASHES;

/// Write Inf and NaN numbers as `Infinity` and `NaN` literals (non-standard).
inline constexpr yyjson_write_flag write_allow_inf_and_nan =
    YYJSON_WRITE_ALLOW_INF_AND_NAN;

/// Write Inf and NaN numbers as `null`.
/// Overrides `write_allow_inf_and_nan` if both are set.
inline constexpr yyjson_write_flag inf_and_nan_as_null =
    YYJSON_WRITE_INF_AND_NAN_AS_NULL;

/// Allow invalid UTF-8 byte sequences when encoding string values.
inline constexpr yyjson_write_flag write_allow_invalid_unicode =
    YYJSON_WRITE_ALLOW_INVALID_UNICODE;

#if defined(YYJSON_VERSION_HEX) && YYJSON_VERSION_HEX >= 0x000900
/// Append a trailing newline `\n` at the end of the JSON output.
/// Requires yyjson >= 0.9.0.
inline constexpr yyjson_write_flag newline_at_end = YYJSON_WRITE_NEWLINE_AT_END;
#endif

/// Returns a JSON string representation of the object.
/// Uses compile-time reflection to serialize a C++ object to JSON format.
/// @tparam Ps Processors to apply during serialization (transforms the data)
/// @param _obj The object to serialize to JSON
/// @param _flag Optional yyjson flags for formatting (use `pretty` for pretty-printing, default: 0)
/// @return JSON string representation of the object
/// @throws std::runtime_error if serialization fails
template <class... Ps>
std::string write(const auto& _obj, const yyjson_write_flag _flag = 0) {
  using T = std::remove_cvref_t<decltype(_obj)>;
  using ParentType = parsing::Parent<Writer>;
  auto w = Writer();
  Parser<T, Processors<Ps...>>::write(w, _obj, typename ParentType::Root{});
  yyjson_write_err err;
  const char* json_c_str =
      yyjson_mut_write_opts(w.doc(), _flag, NULL, NULL, &err);
  if (!json_c_str) {
    throw std::runtime_error("An error occured while writing to JSON: " +
                             std::string(err.msg));
  }
  const auto json_str = std::string(json_c_str);
  free((void*)json_c_str);
  return json_str;
}

/// Writes a JSON representation into an ostream.
/// Uses compile-time reflection to serialize a C++ object to JSON and write to a stream.
/// @tparam Ps Processors to apply during serialization (transforms the data)
/// @param _obj The object to serialize to JSON
/// @param _stream The output stream to write JSON to
/// @param _flag Optional yyjson flags for formatting (use `pretty` for pretty-printing, default: 0)
/// @return The output stream (for chaining)
/// @throws std::runtime_error if serialization fails
template <class... Ps>
std::ostream& write(const auto& _obj, std::ostream& _stream,
                    const yyjson_write_flag _flag = 0) {
  using T = std::remove_cvref_t<decltype(_obj)>;
  using ParentType = parsing::Parent<Writer>;
  auto w = Writer();
  Parser<T, Processors<Ps...>>::write(w, _obj, typename ParentType::Root{});
  yyjson_write_err err;
  const char* json_c_str =
      yyjson_mut_write_opts(w.doc(), _flag, NULL, NULL, &err);
  if (!json_c_str) {
    throw std::runtime_error("An error occured while writing to JSON: " +
                             std::string(err.msg));
  }
  _stream << json_c_str;
  free((void*)json_c_str);
  return _stream;
}

}  // namespace json
}  // namespace rfl

#endif
