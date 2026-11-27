#ifndef RFL_CLI_RESPONSE_FILE_HPP_
#define RFL_CLI_RESPONSE_FILE_HPP_

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "../Result.hpp"

namespace rfl::cli {

/// Configuration for `expand_response_files(...)`.
struct ResponseFileOptions {
  /// Expand `@path` tokens as response files.
  bool expand_at_prefix = true;
  /// Additional option flags that introduce a response file as the next token
  /// (e.g. `{"-f", "-F"}`).
  std::vector<std::string> flag_triggers = {};
  /// Strip `# ...` comments to end of line inside response files.
  bool strip_hash_comments = true;
  /// Strip `// ...` comments to end of line inside response files.
  bool strip_slash_comments = false;
  /// Maximum recursive response file nesting depth (guards against cycles).
  size_t max_depth = 32;
};

namespace internal {

inline rfl::Result<std::vector<std::string>> tokenize_response_stream(
    std::istream& _in, const ResponseFileOptions& _opts) {
  std::vector<std::string> tokens;
  std::string line;
  while (std::getline(_in, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    std::string current;
    bool in_token = false;
    char quote = '\0';
    for (size_t i = 0; i < line.size(); ++i) {
      const char c = line[i];
      if (quote == '\0') {
        if (_opts.strip_hash_comments && c == '#') {
          break;
        }
        if (_opts.strip_slash_comments && c == '/' && i + 1 < line.size()
            && line[i + 1] == '/') {
          break;
        }
        if (std::isspace(static_cast<unsigned char>(c))) {
          if (in_token) {
            tokens.push_back(std::move(current));
            current.clear();
            in_token = false;
          }
          continue;
        }
        if (c == '"' || c == '\'') {
          quote = c;
          in_token = true;
          continue;
        }
        if (c == '\\' && i + 1 < line.size()) {
          current.push_back(line[++i]);
          in_token = true;
          continue;
        }
        current.push_back(c);
        in_token = true;
      } else {
        if (c == quote) {
          quote = '\0';
          continue;
        }
        if (quote == '"' && c == '\\' && i + 1 < line.size()) {
          current.push_back(line[++i]);
          continue;
        }
        current.push_back(c);
      }
    }
    if (quote != '\0') {
      return error("Unterminated quote in response file.");
    }
    if (in_token) {
      tokens.push_back(std::move(current));
    }
  }
  return tokens;
}

inline rfl::Result<bool> expand_response_files_impl(
    std::span<const std::string> _args, const ResponseFileOptions& _opts,
    const size_t _depth, std::vector<std::string>& _out) {
  if (_depth > _opts.max_depth) {
    return error("Response file expansion exceeded maximum depth ("
                 + std::to_string(_opts.max_depth) + ").");
  }
  bool after_double_dash = false;
  for (size_t i = 0; i < _args.size(); ++i) {
    const std::string& arg = _args[i];
    if (after_double_dash) {
      _out.push_back(arg);
      continue;
    }
    if (arg == "--") {
      after_double_dash = true;
      _out.push_back(arg);
      continue;
    }
    std::string rsp_path;
    if (_opts.expand_at_prefix && arg.size() >= 2 && arg[0] == '@') {
      rsp_path = arg.substr(1);
    } else if (std::find(_opts.flag_triggers.begin(), _opts.flag_triggers.end(),
                         arg)
               != _opts.flag_triggers.end()) {
      if (i + 1 >= _args.size()) {
        return error("Missing response file path after '" + arg + "'.");
      }
      rsp_path = _args[++i];
    }
    if (rsp_path.empty()) {
      _out.push_back(arg);
      continue;
    }
    std::ifstream file(rsp_path);
    if (!file.is_open()) {
      return error("Could not open response file '" + rsp_path + "'.");
    }
    auto nested_tokens = tokenize_response_stream(file, _opts);
    if (!nested_tokens) {
      return error(nested_tokens.error().what());
    }
    const auto rec = expand_response_files_impl(*nested_tokens, _opts,
                                                _depth + 1, _out);
    if (!rec) {
      return error(rec.error().what());
    }
  }
  return true;
}

}  // namespace internal

/// Recursively expands response files (`@file` and any configured flag
/// triggers) in `_args`, returning the flattened argument list.
inline rfl::Result<std::vector<std::string>> expand_response_files(
    std::span<const std::string> _args,
    const ResponseFileOptions& _opts = {}) {
  std::vector<std::string> out;
  out.reserve(_args.size());
  const auto ok = internal::expand_response_files_impl(_args, _opts, 0, out);
  if (!ok) {
    return error(ok.error().what());
  }
  return out;
}

/// Overload accepting `argc`/`argv` (skipping `argv[0]`).
inline rfl::Result<std::vector<std::string>> expand_response_files(
    const int _argc, const char* const _argv[],
    const ResponseFileOptions& _opts = {}) {
  std::vector<std::string> args;
  if (_argc > 1) {
    args.reserve(static_cast<size_t>(_argc - 1));
    for (int i = 1; i < _argc; ++i) {
      args.emplace_back(_argv[i]);
    }
  }
  return expand_response_files(args, _opts);
}

inline rfl::Result<std::vector<std::string>> expand_response_files(
    const int _argc, char* _argv[], const ResponseFileOptions& _opts = {}) {
  return expand_response_files(_argc, const_cast<const char* const*>(_argv),
                               _opts);
}

}  // namespace rfl::cli

#endif
