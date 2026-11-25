#include "rfl/yaml/Reader.hpp"

#include <exception>
#include <string>
#include <utility>
#include <vector>

namespace rfl::yaml {

namespace {

std::vector<YAML::Node>& active_nodes() noexcept {
  static thread_local std::vector<YAML::Node> nodes;
  return nodes;
}

bool is_active_node(const std::vector<YAML::Node>& _nodes,
                    const YAML::Node& _node) noexcept {
  if (!_node.IsDefined()) {
    return false;
  }
  for (const auto& active : _nodes) {
    if (active.is(_node)) {
      return true;
    }
  }
  return false;
}

void walk_node(const YAML::Node& _node, const SourceMap::NodeId _node_id,
               SourceMap::Builder& _builder) {
  const Reader::ActiveNodeGuard guard(_node);
  if (guard.is_cycle()) {
    return;
  }
  if (_node.IsMap()) {
    for (const auto& p : _node) {
      std::string k;
      try {
        k = p.first.as<std::string>();
      } catch (const std::exception&) {
        continue;
      }
      const auto child_id = _builder.add_field(
          _node_id, std::move(k),
          SourceMap::Entry{.value = Reader::node_location(p.second),
                           .key = Reader::node_location(p.first)});
      if (child_id && (p.second.IsMap() || p.second.IsSequence())) {
        walk_node(p.second, child_id, _builder);
      }
    }
  } else if (_node.IsSequence()) {
    for (size_t i = 0; i < _node.size(); ++i) {
      const auto elem = _node[i];
      const auto child_id = _builder.add_element(
          _node_id, SourceMap::Entry{.value = Reader::node_location(elem),
                                     .key = std::nullopt});
      if (child_id && (elem.IsMap() || elem.IsSequence())) {
        walk_node(elem, child_id, _builder);
      }
    }
  }
}

}  // namespace

Reader::ActiveNodeGuard::ActiveNodeGuard(const YAML::Node& _node)
    : is_cycle_(false), pushed_(false) {
  auto& nodes = active_nodes();
  if (is_active_node(nodes, _node)) {
    is_cycle_ = true;
    return;
  }
  if (_node.IsDefined()) {
    nodes.push_back(_node);
    pushed_ = true;
  }
}

Reader::ActiveNodeGuard::~ActiveNodeGuard() {
  if (pushed_) {
    active_nodes().pop_back();
  }
}

SourceMap build_source_map(const YAML::Node& _root) {
  if (!_root.IsDefined()) {
    return SourceMap{};
  }
  SourceMap::Builder builder(SourceMap::Entry{
      .value = Reader::node_location(_root), .key = std::nullopt});
  walk_node(_root, builder.root(), builder);
  return std::move(builder).build();
}

}  // namespace rfl::yaml
