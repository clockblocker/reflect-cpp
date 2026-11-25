#include "rfl/SourceMap.hpp"

#include <limits>
#include <map>
#include <vector>

namespace rfl {

namespace {

constexpr size_t kInvalidNodeIndex = std::numeric_limits<size_t>::max();

}  // namespace

struct SourceMap::Impl {
  struct Node {
    Entry entry;
    std::map<std::string, size_t> fields;
    std::vector<size_t> elements;

    bool operator==(const Node&) const = default;
  };

  std::vector<Node> nodes;

  std::optional<size_t> step(const size_t _node_id,
                             const PathSegment& _segment) const noexcept {
    const auto& node = nodes[_node_id];
    if (const auto* name = std::get_if<std::string>(&_segment)) {
      const auto it = node.fields.find(*name);
      if (it == node.fields.end()) {
        return std::nullopt;
      }
      return it->second;
    }
    const auto idx = std::get<size_t>(_segment);
    if (idx >= node.elements.size()) {
      return std::nullopt;
    }
    return node.elements[idx];
  }

  bool operator==(const Impl&) const = default;
};

SourceMap::Builder::Builder() : Builder(Entry{}) {}

SourceMap::Builder::Builder(const Entry& _root)
    : impl_(std::make_unique<Impl>()) {
  impl_->nodes.push_back(Impl::Node{.entry = _root});
}

SourceMap::Builder::~Builder() = default;

SourceMap::Builder::Builder(Builder&& _other) noexcept = default;

SourceMap::Builder& SourceMap::Builder::operator=(Builder&& _other) noexcept =
    default;

SourceMap::NodeId SourceMap::Builder::root() const noexcept {
  if (!impl_ || impl_->nodes.empty()) {
    return NodeId(kInvalidNodeIndex);
  }
  return NodeId(0);
}

SourceMap::NodeId SourceMap::Builder::add_field(const NodeId _parent,
                                                std::string _name,
                                                const Entry& _entry) {
  if (!impl_ || _parent.index_ >= impl_->nodes.size()) {
    return NodeId(kInvalidNodeIndex);
  }
  const auto id = impl_->nodes.size();
  impl_->nodes.push_back(Impl::Node{.entry = _entry});
  impl_->nodes[_parent.index_].fields.emplace(std::move(_name), id);
  return NodeId(id);
}

SourceMap::NodeId SourceMap::Builder::add_element(const NodeId _parent,
                                                  const Entry& _entry) {
  if (!impl_ || _parent.index_ >= impl_->nodes.size()) {
    return NodeId(kInvalidNodeIndex);
  }
  const auto id = impl_->nodes.size();
  impl_->nodes.push_back(Impl::Node{.entry = _entry});
  impl_->nodes[_parent.index_].elements.push_back(id);
  return NodeId(id);
}

SourceMap SourceMap::Builder::build() && noexcept {
  return SourceMap(std::move(impl_));
}

SourceMap::SourceMap() noexcept = default;

SourceMap::SourceMap(std::unique_ptr<Impl> _impl) noexcept
    : impl_(std::move(_impl)) {}

SourceMap::~SourceMap() = default;

SourceMap::SourceMap(const SourceMap& _other)
    : impl_(_other.impl_ ? std::make_unique<Impl>(*_other.impl_) : nullptr) {}

SourceMap::SourceMap(SourceMap&& _other) noexcept = default;

SourceMap& SourceMap::operator=(const SourceMap& _other) {
  if (this != &_other) {
    impl_ = _other.impl_ ? std::make_unique<Impl>(*_other.impl_) : nullptr;
  }
  return *this;
}

SourceMap& SourceMap::operator=(SourceMap&& _other) noexcept = default;

bool SourceMap::empty() const noexcept {
  return !impl_ || impl_->nodes.empty();
}

void SourceMap::clear() noexcept { impl_.reset(); }

std::optional<SourceMap::Entry> SourceMap::find_entry(
    const std::span<const PathSegment> _path) const {
  if (empty()) {
    return std::nullopt;
  }
  size_t current = 0;
  for (const auto& segment : _path) {
    const auto next = impl_->step(current, segment);
    if (!next) {
      return std::nullopt;
    }
    current = *next;
  }
  return impl_->nodes[current].entry;
}

std::optional<SourceLocation> SourceMap::find(
    const std::span<const PathSegment> _path) const {
  const auto entry = find_entry(_path);
  return entry ? entry->value : std::nullopt;
}

std::optional<SourceLocation> SourceMap::find_key(
    const std::span<const PathSegment> _path) const {
  const auto entry = find_entry(_path);
  return entry ? entry->key : std::nullopt;
}

std::optional<SourceLocation> SourceMap::find_nearest(
    const std::span<const PathSegment> _path) const {
  if (empty()) {
    return std::nullopt;
  }
  size_t current = 0;
  std::optional<SourceLocation> last_known = impl_->nodes[0].entry.value;
  bool exact = true;
  for (const auto& segment : _path) {
    const auto next = impl_->step(current, segment);
    if (!next) {
      exact = false;
      break;
    }
    current = *next;
    if (impl_->nodes[current].entry.value.has_value()) {
      last_known = impl_->nodes[current].entry.value;
    }
  }
  if (!last_known) {
    return std::nullopt;
  }
  if (!exact) {
    last_known->exact = false;
  }
  return last_known;
}

bool SourceMap::operator==(const SourceMap& _other) const {
  if (empty() && _other.empty()) {
    return true;
  }
  if (empty() != _other.empty()) {
    return false;
  }
  return *impl_ == *_other.impl_;
}

}  // namespace rfl
