#ifndef RFL_SOURCEMAP_HPP_
#define RFL_SOURCEMAP_HPP_

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>

#include "Result.hpp"
#include "common.hpp"

namespace rfl {

/// Maps parsed field/element paths (matching rfl::ErrorDetail::path) to their
/// source locations in the input document, enabling post-parse semantic
/// diagnostics to report line and column numbers.
class RFL_API SourceMap {
  struct Impl;

 public:
  using PathSegment = ErrorDetail::PathSegment;

  /// Source locations recorded for a single node in the input document.
  struct Entry {
    /// Location of the node's value (1-based line and column).
    std::optional<SourceLocation> value = std::nullopt;

    /// Location of the mapping key introducing this node, if it is a mapping
    /// field; std::nullopt for root nodes and sequence elements.
    std::optional<SourceLocation> key = std::nullopt;

    bool operator==(const Entry&) const = default;
  };

  class Builder;

  /// Opaque handle referencing a node owned by a SourceMap::Builder.
  class NodeId {
   public:
    constexpr bool is_valid() const noexcept {
      return index_ != kInvalidIndex;
    }
    explicit constexpr operator bool() const noexcept { return is_valid(); }
    bool operator==(const NodeId&) const = default;

   private:
    friend class Builder;
    static constexpr size_t kInvalidIndex = static_cast<size_t>(-1);
    explicit constexpr NodeId(const size_t _index) noexcept : index_(_index) {}
    size_t index_;
  };

  /// Bounds-safe builder for constructing a SourceMap tree.
  class RFL_API Builder {
   public:
    Builder();
    explicit Builder(const Entry& _root);
    ~Builder();
    Builder(Builder&& _other) noexcept;
    Builder& operator=(Builder&& _other) noexcept;
    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;

    /// Returns the NodeId of the root node.
    NodeId root() const noexcept;

    /// Adds a named field child under _parent and returns its NodeId.
    /// If a field with the same name already exists under _parent (or _parent
    /// is invalid), the existing tree is left unchanged and an invalid NodeId
    /// is returned.
    NodeId add_field(NodeId _parent, std::string _name, const Entry& _entry);

    /// Appends a sequence element child under _parent and returns its NodeId.
    NodeId add_element(NodeId _parent, const Entry& _entry);

    /// Finalizes construction and returns the resulting SourceMap.
    SourceMap build() && noexcept;

   private:
    std::unique_ptr<Impl> impl_;
  };

  SourceMap() noexcept;
  ~SourceMap();
  SourceMap(const SourceMap& _other);
  SourceMap(SourceMap&& _other) noexcept;
  SourceMap& operator=(const SourceMap& _other);
  SourceMap& operator=(SourceMap&& _other) noexcept;

  /// Returns true if no source nodes have been recorded.
  bool empty() const noexcept;

  /// Clears all recorded source locations.
  void clear() noexcept;

  /// Looks up the full Entry (value and key locations) for an exact path.
  std::optional<Entry> find_entry(
      std::span<const PathSegment> _path) const noexcept;

  std::optional<Entry> find_entry(
      std::initializer_list<PathSegment> _path) const noexcept {
    return find_entry(std::span<const PathSegment>(_path.begin(), _path.size()));
  }

  /// Looks up the value SourceLocation for an exact path.
  std::optional<SourceLocation> find(
      std::span<const PathSegment> _path) const noexcept;

  std::optional<SourceLocation> find(
      std::initializer_list<PathSegment> _path) const noexcept {
    return find(std::span<const PathSegment>(_path.begin(), _path.size()));
  }

  /// Looks up the mapping key SourceLocation for an exact path.
  std::optional<SourceLocation> find_key(
      std::span<const PathSegment> _path) const noexcept;

  std::optional<SourceLocation> find_key(
      std::initializer_list<PathSegment> _path) const noexcept {
    return find_key(std::span<const PathSegment>(_path.begin(), _path.size()));
  }

  /// Looks up the SourceLocation for _path, falling back to the deepest
  /// existing ancestor with exact = false if _path is not present in the
  /// input (for example, an omitted optional or defaulted field) or has no
  /// recorded value location.
  std::optional<SourceLocation> find_nearest(
      std::span<const PathSegment> _path) const noexcept;

  std::optional<SourceLocation> find_nearest(
      std::initializer_list<PathSegment> _path) const noexcept {
    return find_nearest(
        std::span<const PathSegment>(_path.begin(), _path.size()));
  }

  bool operator==(const SourceMap& _other) const noexcept;

 private:
  explicit SourceMap(std::unique_ptr<Impl> _impl) noexcept;

  std::unique_ptr<Impl> impl_;
};

}  // namespace rfl

#endif
