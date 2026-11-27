#ifndef RFL_JSON_READ_HPP_
#define RFL_JSON_READ_HPP_

#if __has_include(<yyjson.h>)
#include <yyjson.h>
#else
#include "../thirdparty/yyjson.h"
#endif

#include <istream>
#include <string_view>
#include <utility>

#include "../Processors.hpp"
#include "../internal/wrap_in_rfl_array_t.hpp"
#include "../parsing/Parser_base.hpp"
#include "Parser.hpp"
#include "Reader.hpp"

namespace rfl {
namespace json {

/// Stop when done instead of issuing an error if there is additional content
/// after a JSON document (e.g. NDJSON).
inline constexpr yyjson_read_flag stop_when_done = YYJSON_READ_STOP_WHEN_DONE;

/// Allow a single trailing comma at the end of an object or array.
inline constexpr yyjson_read_flag allow_trailing_commas =
    YYJSON_READ_ALLOW_TRAILING_COMMAS;

/// Allow C-style single-line (//) and multi-line (/* */) comments.
inline constexpr yyjson_read_flag allow_comments = YYJSON_READ_ALLOW_COMMENTS;

/// Allow Inf/NaN numbers and literals (e.g. 1e999, NaN, inf, -Infinity).
inline constexpr yyjson_read_flag read_allow_inf_and_nan =
    YYJSON_READ_ALLOW_INF_AND_NAN;

/// Allow invalid UTF-8 byte sequences when parsing string values.
inline constexpr yyjson_read_flag read_allow_invalid_unicode =
    YYJSON_READ_ALLOW_INVALID_UNICODE;

#if defined(YYJSON_VERSION_HEX) && YYJSON_VERSION_HEX >= 0x000B00
/// Allow and skip a leading UTF-8 BOM before parsing.
/// Requires yyjson >= 0.11.0.
inline constexpr yyjson_read_flag allow_bom = YYJSON_READ_ALLOW_BOM;
#endif

/// Convenience flag for JSONC (comments and trailing commas).
inline constexpr yyjson_read_flag jsonc =
    allow_comments | allow_trailing_commas;

#if defined(YYJSON_VERSION_HEX) && YYJSON_VERSION_HEX >= 0x000C00
/// Allow extended number formats (hexadecimal, leading/trailing decimal point,
/// leading plus sign). Requires yyjson >= 0.12.0.
inline constexpr yyjson_read_flag allow_ext_number =
    YYJSON_READ_ALLOW_EXT_NUMBER;

/// Allow extended escape sequences in strings. Requires yyjson >= 0.12.0.
inline constexpr yyjson_read_flag allow_ext_escape =
    YYJSON_READ_ALLOW_EXT_ESCAPE;

/// Allow extended whitespace characters. Requires yyjson >= 0.12.0.
inline constexpr yyjson_read_flag allow_ext_whitespace =
    YYJSON_READ_ALLOW_EXT_WHITESPACE;

/// Allow strings enclosed in single quotes. Requires yyjson >= 0.12.0.
inline constexpr yyjson_read_flag allow_single_quoted_str =
    YYJSON_READ_ALLOW_SINGLE_QUOTED_STR;

/// Allow unquoted object keys. Requires yyjson >= 0.12.0.
inline constexpr yyjson_read_flag allow_unquoted_key =
    YYJSON_READ_ALLOW_UNQUOTED_KEY;

/// Convenience flag for JSON5. Requires yyjson >= 0.12.0.
inline constexpr yyjson_read_flag json5 = YYJSON_READ_JSON5;
#endif

using InputObjectType = typename Reader::InputObjectType;
using InputVarType = typename Reader::InputVarType;

/// Parses an object from a JSON var.
/// A JSON var is the internal representation used by the yyjson library.
/// @tparam T The type to parse into
/// @tparam Ps Processors to apply during parsing (transforms the data)
/// @param _obj The JSON variant object to parse from
/// @return Result containing either the parsed object of type T or an error
template <class T, class... Ps>
auto read(const InputVarType& _obj) {
  const auto r = Reader();
  return Parser<T, Processors<Ps...>>::read(r, _obj);
}

/// Parses an object from JSON using reflection.
/// This function reads a JSON string and constructs a C++ object using compile-time reflection.
/// @tparam T The type to parse into
/// @tparam Ps Processors to apply during parsing (transforms the data)
/// @param _json_str The JSON string to parse
/// @param _flag Optional yyjson flags for parsing (default: 0). Note: YYJSON_READ_INSITU is not supported.
/// @return Result containing either the parsed object (or array of objects) or an error message
template <class T, class... Ps>
Result<internal::wrap_in_rfl_array_t<T>> read(
    const std::string_view _json_str, const yyjson_read_flag _flag = 0) {
  if (_flag & YYJSON_READ_INSITU) {
    return error("YYJSON_READ_INSITU is not supported");
  }
  yyjson_read_err err;
  // According to yyjson's doc, it's safe castaway constness as long as
  // YYJSON_READ_INSITU is not set
  yyjson_doc* doc = yyjson_read_opts(const_cast<char*>(_json_str.data()),
                                     _json_str.size(), _flag, NULL, &err);
  if (!doc) {
    return error("Could not parse document: " + std::string(err.msg),
                 ErrorKind::syntax_error);
  }
  yyjson_val* root = yyjson_doc_get_root(doc);
  const auto r = Reader();
  auto res = Parser<T, Processors<Ps...>>::read(r, InputVarType(root));
  yyjson_doc_free(doc);
  return res;
}

/// Parses an object from a stringstream.
/// Reads the entire stream content and parses it as JSON.
/// @tparam T The type to parse into
/// @tparam Ps Processors to apply during parsing (transforms the data)
/// @param _stream The input stream containing JSON data
/// @param _flag Optional yyjson flags for parsing (default: 0)
/// @return Result containing either the parsed object (or array of objects) or an error message
template <class T, class... Ps>
auto read(std::istream& _stream, const yyjson_read_flag _flag = 0) {
  const auto json_str = std::string(std::istreambuf_iterator<char>(_stream),
                                    std::istreambuf_iterator<char>());
  return read<T, Ps...>(json_str, _flag);
}

/// Overlays a JSON var onto an existing object, preserving missing fields and
/// recursively overlaying nested structs. Atomic on error.
/// @tparam Ps Processors to apply during parsing
/// @tparam T The type to overlay onto (deduced from `_base`)
/// @param _base The base object to overlay onto
/// @param _obj The JSON variant object to overlay from
/// @return Result containing either the overlaid object or an error
template <class... Ps, class T>
Result<T> read_into(T _base, const InputVarType& _obj) {
  const auto r = Reader();
  return parsing::read_into_or_read<Reader, Writer, T, Processors<Ps...>>(
      r, _obj, std::move(_base));
}

/// Overlays a JSON string onto an existing object, preserving missing fields
/// and recursively overlaying nested structs. Atomic on error.
/// @tparam Ps Processors to apply during parsing
/// @tparam T The type to overlay onto (deduced from `_base`)
/// @param _base The base object to overlay onto
/// @param _json_str The JSON string to overlay
/// @param _flag Optional yyjson flags for parsing (default: 0). Note: YYJSON_READ_INSITU is not supported.
/// @return Result containing either the overlaid object or an error message
template <class... Ps, class T>
Result<T> read_into(T _base, const std::string_view _json_str,
                    const yyjson_read_flag _flag = 0) {
  if (_flag & YYJSON_READ_INSITU) {
    return error("YYJSON_READ_INSITU is not supported");
  }
  yyjson_read_err err;
  yyjson_doc* doc = yyjson_read_opts(const_cast<char*>(_json_str.data()),
                                     _json_str.size(), _flag, NULL, &err);
  if (!doc) {
    return error("Could not parse document: " + std::string(err.msg));
  }
  struct DocGuard {
    yyjson_doc* doc_;
    ~DocGuard() { yyjson_doc_free(doc_); }
  } guard{doc};
  yyjson_val* root = yyjson_doc_get_root(doc);
  const auto r = Reader();
  return parsing::read_into_or_read<Reader, Writer, T, Processors<Ps...>>(
      r, InputVarType(root), std::move(_base));
}

/// Overlays JSON from an input stream onto an existing object. Atomic on error.
/// @tparam Ps Processors to apply during parsing
/// @tparam T The type to overlay onto (deduced from `_base`)
/// @param _base The base object to overlay onto
/// @param _stream The input stream containing JSON data
/// @param _flag Optional yyjson flags for parsing (default: 0)
/// @return Result containing either the overlaid object or an error message
template <class... Ps, class T>
Result<T> read_into(T _base, std::istream& _stream,
                    const yyjson_read_flag _flag = 0) {
  const auto json_str = std::string(std::istreambuf_iterator<char>(_stream),
                                    std::istreambuf_iterator<char>());
  return read_into<Ps...>(std::move(_base), json_str, _flag);
}

}  // namespace json
}  // namespace rfl

#endif
