#ifndef RFL_XML_READ_HPP_
#define RFL_XML_READ_HPP_

#include <cctype>
#include <istream>
#include <pugixml.hpp>
#include <string>
#include <string_view>

#include "../Processors.hpp"
#include "../internal/get_type_name.hpp"
#include "../internal/no_field_names_v.hpp"
#include "../internal/remove_namespaces.hpp"
#include "Parser.hpp"
#include "Reader.hpp"

namespace rfl ::xml {

using InputVarType = typename Reader::InputVarType;

/// Parses an object from an XML var (internal XML node representation).
/// An XML var is the internal representation used by the pugixml library.
/// @tparam T The type to parse into
/// @tparam Ps Processors to apply during parsing (transforms the data)
/// @param _var The XML variant object to parse from
/// @return Result containing the parsed object of type T
template <class T, class... Ps>
auto read(const InputVarType& _var) {
  const auto r = Reader();
  using ProcessorsType = Processors<Ps...>;
  static_assert(!internal::no_field_names_v<ProcessorsType>,
                "The NoFieldNames processor is not supported for BSON, XML, "
                "TOML, or YAML.");
  return Parser<T, ProcessorsType>::read(r, _var);
}

/// Parses an object from a pugixml node (or `pugi::xml_document`, which derives
/// from `pugi::xml_node`). If the node is a document node, its root element is
/// parsed (skipping declarations, comments, etc.).
/// @tparam T The type to parse into
/// @tparam Ps Processors to apply during parsing (transforms the data)
/// @param _node The pugixml node or document to parse from
/// @return Result containing either the parsed object of type T or an error message
template <class T, class... Ps>
Result<T> read(const pugi::xml_node& _node) {
  if (!_node) {
    return error("XML node is empty.", ErrorKind::syntax_error);
  }
  if (_node.type() == pugi::node_document) {
    for (auto child = _node.first_child(); child;
         child = child.next_sibling()) {
      if (child.type() == pugi::node_element) {
        return read<T, Ps...>(InputVarType(child));
      }
    }
    return error("XML document has no root element.", ErrorKind::syntax_error);
  }
  return read<T, Ps...>(InputVarType(_node));
}

/// Parses an object from XML using reflection.
/// This function reads an XML string and constructs a C++ object using compile-time reflection.
/// @tparam T The type to parse into
/// @tparam Ps Processors to apply during parsing (transforms the data)
/// @param _xml_str The XML string to parse
/// @return Result containing either the parsed object of type T or an error message
template <class T, class... Ps>
Result<T> read(const std::string_view _xml_str) {
  pugi::xml_document doc;
  const auto result = doc.load_buffer(_xml_str.data(), _xml_str.size());
  if (!result) {
    if (result.status == pugi::status_no_document_element) {
      auto trimmed = _xml_str;
      if (trimmed.size() >= 3 &&
          static_cast<unsigned char>(trimmed[0]) == 0xEF &&
          static_cast<unsigned char>(trimmed[1]) == 0xBB &&
          static_cast<unsigned char>(trimmed[2]) == 0xBF) {
        trimmed.remove_prefix(3);
      }
      bool all_ws = true;
      for (const char c : trimmed) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
          all_ws = false;
          break;
        }
      }
      if (all_ws) {
        return error("XML document is empty.", ErrorKind::syntax_error);
      }
      return error("XML document has no root element.",
                   ErrorKind::syntax_error);
    }
    return error("XML string could not be parsed: " +
                     std::string(result.description()),
                 ErrorKind::syntax_error);
  }
  return read<T, Ps...>(doc);
}

/// Parses an object from a stringstream containing XML.
/// Reads the entire stream content and parses it as XML.
/// @tparam T The type to parse into
/// @tparam Ps Processors to apply during parsing (transforms the data)
/// @param _stream The input stream containing XML data
/// @return Result containing either the parsed object of type T or an error message
template <class T, class... Ps>
auto read(std::istream& _stream) {
  const auto xml_str = std::string(std::istreambuf_iterator<char>(_stream),
                                   std::istreambuf_iterator<char>());
  return read<T, Ps...>(xml_str);
}

}  // namespace rfl::xml

#endif
