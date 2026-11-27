#ifndef RFL_XML_WRITE_HPP_
#define RFL_XML_WRITE_HPP_

#include <ostream>
#include <pugixml.hpp>
#include <sstream>
#include <string>
#include <type_traits>

#include "../Processors.hpp"
#include "../internal/StringLiteral.hpp"
#include "../internal/get_type_name.hpp"
#include "../internal/no_field_names_v.hpp"
#include "../internal/remove_namespaces.hpp"
#include "../parsing/Parent.hpp"
#include "Parser.hpp"

namespace rfl {
namespace xml {

/// Helper function to determine the XML root element name at compile time.
/// Uses explicit root name if provided, otherwise derives it from the type name.
/// @tparam _root Explicitly specified root name (empty string means auto-derive)
/// @tparam T The type being serialized
/// @return The root element name to use
template <internal::StringLiteral _root, class T>
consteval auto get_root_name() {
  if constexpr (_root != internal::StringLiteral("")) {
    return _root;
  } else {
    return internal::remove_namespaces<
        internal::get_type_name<std::remove_cvref_t<T>>()>();
  }
}

/// Writes an object as a child XML element under an existing pugixml node
/// using a runtime root element name.
/// Existing siblings, comments, and declarations in `_parent` are preserved.
/// @tparam Ps Processors to apply during serialization (transforms the data)
/// @param _obj The object to serialize to XML
/// @param _parent The parent pugixml node (or document) to append under
/// @param _root_name The name of the appended child element
/// @return The newly appended pugixml node (or an empty node if `_parent` is empty)
template <class... Ps>
pugi::xml_node write(const auto& _obj, pugi::xml_node _parent,
                     const std::string& _root_name) {
  if (!_parent) {
    return pugi::xml_node();
  }
  using T = std::remove_cvref_t<decltype(_obj)>;
  using ParentType = parsing::Parent<Writer>;

  const auto prev_last = _parent.last_child();
  const auto parent_ref = Ref<pugi::xml_node>::make(_parent);
  auto w = Writer(parent_ref, _root_name);

  using ProcessorsType = Processors<Ps...>;
  static_assert(!internal::no_field_names_v<ProcessorsType>,
                "The NoFieldNames processor is not supported for BSON, XML, "
                "TOML, or YAML.");
  Parser<T, ProcessorsType>::write(w, _obj, typename ParentType::Root{});

  return prev_last ? prev_last.next_sibling() : _parent.first_child();
}

/// Writes an object as a child XML element under an existing pugixml node.
/// Existing siblings, comments, and declarations in `_parent` are preserved.
/// @tparam _root The name of the XML root element (defaults to type name if empty)
/// @tparam Ps Processors to apply during serialization (transforms the data)
/// @param _obj The object to serialize to XML
/// @param _parent The parent pugixml node (or document) to append under
/// @return The newly appended pugixml node (or an empty node if `_parent` is empty)
template <internal::StringLiteral _root = internal::StringLiteral(""),
          class... Ps>
pugi::xml_node write(const auto& _obj, pugi::xml_node _parent) {
  using T = std::remove_cvref_t<decltype(_obj)>;

  constexpr auto root_name = get_root_name<_root, T>();

  static_assert(root_name.string_view().find("<") == std::string_view::npos &&
                    root_name.string_view().find(">") == std::string_view::npos,
                "The name of an XML root node cannot contain '<' or '>'. "
                "Please assign an "
                "explicit root name to rfl::xml::write(...) like this: "
                "rfl::xml::write<\"root_name\">(...).");

  return write<Ps...>(_obj, _parent, root_name.str());
}

/// Writes an XML representation into an ostream.
/// Uses compile-time reflection to serialize a C++ object to XML and write to a stream.
/// @tparam _root The name of the XML root element (defaults to type name if empty)
/// @tparam Ps Processors to apply during serialization (transforms the data)
/// @param _obj The object to serialize to XML
/// @param _stream The output stream to write XML to
/// @param _indent The indentation string for formatting (default: 4 spaces)
/// @return The output stream (for chaining)
template <internal::StringLiteral _root = internal::StringLiteral(""),
          class... Ps>
std::ostream& write(const auto& _obj, std::ostream& _stream,
                    const std::string& _indent = "    ") {
  pugi::xml_document doc;

  auto declaration_node = doc.append_child(pugi::node_declaration);
  declaration_node.append_attribute("version") = "1.0";
  declaration_node.append_attribute("encoding") = "UTF-8";

  write<_root, Ps...>(_obj, doc);

  doc.save(_stream, _indent.c_str());

  return _stream;
}

/// Returns an XML string representation of the object.
/// Uses compile-time reflection to serialize a C++ object to XML format.
/// @tparam _root The name of the XML root element (defaults to type name if empty)
/// @tparam Ps Processors to apply during serialization (transforms the data)
/// @param _obj The object to serialize to XML
/// @param _indent The indentation string for formatting (default: 4 spaces)
/// @return XML string representation of the object
template <internal::StringLiteral _root = internal::StringLiteral(""),
          class... Ps>
std::string write(const auto& _obj, const std::string& _indent = "    ") {
  std::stringstream stream;
  write<_root, Ps...>(_obj, stream, _indent);
  return stream.str();
}

}  // namespace xml
}  // namespace rfl

#endif
