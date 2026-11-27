#ifndef RFL_ERROR_FORMAT_HPP_
#define RFL_ERROR_FORMAT_HPP_

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "Result.hpp"

namespace rfl {

namespace internal::error_format {

inline bool needs_bracket_quoting(const std::string_view _key) {
  if (_key.empty()) return true;
  for (const unsigned char ch : _key) {
    if (ch == '.' || ch == '[' || ch == ']' || ch == '"' || ch == '\\' ||
        ch <= 0x20 || ch == 0x7f) {
      return true;
    }
  }
  return false;
}

inline std::string quote_bracket_key(const std::string_view _key) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string out = "[\"";
  for (const unsigned char ch : _key) {
    if (ch == '\\' || ch == '"') {
      out.push_back('\\');
      out.push_back(static_cast<char>(ch));
    } else if (ch == '\n') {
      out += "\\n";
    } else if (ch == '\r') {
      out += "\\r";
    } else if (ch == '\t') {
      out += "\\t";
    } else if (ch < 0x20 || ch == 0x7f) {
      out += "\\x";
      out.push_back(kHex[(ch >> 4) & 0x0f]);
      out.push_back(kHex[ch & 0x0f]);
    } else {
      out.push_back(static_cast<char>(ch));
    }
  }
  return out + "\"]";
}

inline std::string format_segment_label(const ErrorDetail::PathSegment& _seg) {
  if (const auto* key = std::get_if<std::string>(&_seg)) {
    return needs_bracket_quoting(*key) ? quote_bracket_key(*key) : *key;
  }
  return "[" + std::to_string(std::get<size_t>(_seg)) + "]";
}

struct TreeNode {
  std::vector<std::string> messages;
  std::vector<std::pair<ErrorDetail::PathSegment, size_t>> children;
};

inline void append_multiline(std::string& _out, const std::string_view _prefix,
                             const std::string_view _connector,
                             const std::string_view _cont_prefix,
                             const std::string_view _text) {
  if (!_out.empty()) _out.push_back('\n');
  _out += _prefix;
  _out += _connector;
  size_t pos = 0;
  while (true) {
    const size_t nl = _text.find('\n', pos);
    if (nl == std::string_view::npos) {
      _out.append(_text.substr(pos));
      break;
    }
    _out.append(_text.substr(pos, nl - pos));
    _out.push_back('\n');
    _out += _cont_prefix;
    pos = nl + 1;
  }
}

inline void render_children(const std::vector<TreeNode>& _nodes,
                            const size_t _node_idx,
                            const std::string& _prefix, std::string& _out) {
  static constexpr std::string_view kBranchMid = "\xE2\x94\x9C\xE2\x94\x80 ";
  static constexpr std::string_view kBranchLast = "\xE2\x94\x94\xE2\x94\x80 ";
  static constexpr std::string_view kIndentMid = "\xE2\x94\x82  ";
  static constexpr std::string_view kIndentLast = "   ";
  const auto& node = _nodes[_node_idx];
  const size_t total = node.messages.size() + node.children.size();
  size_t item = 0;
  for (const auto& msg : node.messages) {
    const bool is_last = (++item == total);
    append_multiline(_out, _prefix, is_last ? kBranchLast : kBranchMid,
                     _prefix + std::string(is_last ? kIndentLast : kIndentMid),
                     msg);
  }
  for (const auto& [segment, child_idx] : node.children) {
    const bool is_last = (++item == total);
    if (!_out.empty()) _out.push_back('\n');
    _out += _prefix;
    _out += is_last ? kBranchLast : kBranchMid;
    _out += format_segment_label(segment);
    render_children(_nodes, child_idx,
                    _prefix + std::string(is_last ? kIndentLast : kIndentMid),
                    _out);
  }
}

}  // namespace internal::error_format

/// Formats an ErrorDetail path using dot/bracket notation.
inline std::string format_path(
    const std::vector<ErrorDetail::PathSegment>& _path) {
  std::string out;
  for (const auto& segment : _path) {
    if (const auto* idx = std::get_if<size_t>(&segment)) {
      out += "[" + std::to_string(*idx) + "]";
    } else {
      const auto& key = std::get<std::string>(segment);
      if (internal::error_format::needs_bracket_quoting(key)) {
        out += internal::error_format::quote_bracket_key(key);
      } else {
        if (!out.empty()) out.push_back('.');
        out += key;
      }
    }
  }
  return out;
}

/// Formats an ErrorDetail path as an RFC 6901 JSON Pointer.
inline std::string format_json_pointer(
    const std::vector<ErrorDetail::PathSegment>& _path) {
  std::string out;
  for (const auto& segment : _path) {
    out.push_back('/');
    if (const auto* idx = std::get_if<size_t>(&segment)) {
      out += std::to_string(*idx);
    } else {
      for (const char ch : std::get<std::string>(segment)) {
        if (ch == '~') {
          out += "~0";
        } else if (ch == '/') {
          out += "~1";
        } else {
          out.push_back(ch);
        }
      }
    }
  }
  return out;
}

/// Formats an Error as a grouped multi-line diagnostic tree, invoking
/// _hook(detail) to render each leaf message.
template <class LeafHook>
  requires std::invocable<LeafHook&, const ErrorDetail&> &&
           std::constructible_from<
               std::string, std::invoke_result_t<LeafHook&, const ErrorDetail&>>
inline std::string format_error_tree(const Error& _err, LeafHook&& _hook) {
  const auto details = _err.details();
  if (details.empty()) return {};

  std::vector<internal::error_format::TreeNode> nodes(1);
  for (const auto& detail : details) {
    size_t cur = 0;
    for (const auto& segment : detail.path) {
      size_t next = nodes.size();
      for (const auto& [existing_seg, child_idx] : nodes[cur].children) {
        if (existing_seg == segment) {
          next = child_idx;
          break;
        }
      }
      if (next == nodes.size()) {
        nodes[cur].children.emplace_back(segment, next);
        nodes.emplace_back();
      }
      cur = next;
    }
    nodes[cur].messages.emplace_back(_hook(detail));
  }

  if (nodes[0].children.empty() && nodes[0].messages.size() == 1) {
    return nodes[0].messages[0];
  }
  std::string out;
  internal::error_format::render_children(nodes, 0, "", out);
  return out;
}

/// Formats an Error as a grouped multi-line diagnostic tree.
inline std::string format_error_tree(const Error& _err) {
  return format_error_tree(_err, [](const ErrorDetail& _detail) -> std::string {
    return _detail.message;
  });
}

}  // namespace rfl

#endif
