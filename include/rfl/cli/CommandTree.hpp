#ifndef RFL_CLI_COMMANDTREE_HPP_
#define RFL_CLI_COMMANDTREE_HPP_

#include <algorithm>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../Result.hpp"
#include "DynamicOptions.hpp"
#include "suggest.hpp"

namespace rfl::cli {

/// Result of resolving a command path and parsing arguments through a
/// `CommandNode` tree.
struct CommandDispatchResult {
  /// Sequence of matched canonical subcommand names from root to leaf.
  std::vector<std::string> command_path;
  /// Parsed options and positional arguments for the matched leaf command.
  DynamicParseResult parsed;
};

/// Dynamic, arbitrarily nested subcommand router built on `DynamicOptions`.
/// Supports subcommand aliases, per-command option tables, `options_first`
/// prefix commands (e.g. `micromamba run`), and `"Did you mean '...'?"`
/// suggestions for mistyped subcommands at any nesting depth.
class CommandNode {
 public:
  using Callback = std::function<void(const DynamicParseResult&)>;

  CommandNode(std::string _name, std::string _description = {})
      : name_(std::move(_name)), description_(std::move(_description)) {}

  const std::string& name() const noexcept { return name_; }
  const std::string& description() const noexcept { return description_; }
  const std::vector<std::string>& aliases() const noexcept { return aliases_; }

  DynamicOptions& options() noexcept { return options_; }
  const DynamicOptions& options() const noexcept { return options_; }

  CommandNode& alias(std::string _alias) {
    aliases_.push_back(std::move(_alias));
    return *this;
  }

  CommandNode& callback(Callback _cb) {
    callback_ = std::move(_cb);
    return *this;
  }

  /// Controls whether a child subcommand must be selected when children exist.
  CommandNode& require_subcommand(const bool _required = true) {
    require_subcommand_ = _required;
    return *this;
  }

  /// Registers and returns a child subcommand.
  CommandNode& add_subcommand(std::string _name,
                              std::string _description = {}) {
    subcommands_.push_back(std::make_unique<CommandNode>(
        std::move(_name), std::move(_description)));
    return *subcommands_.back();
  }

  const std::vector<std::unique_ptr<CommandNode>>& subcommands() const noexcept {
    return subcommands_;
  }

  /// Returns all direct subcommand names and aliases (used for suggestions).
  std::vector<std::string> subcommand_names() const {
    std::vector<std::string> names;
    for (const auto& sub : subcommands_) {
      names.push_back(sub->name_);
      for (const auto& a : sub->aliases_) {
        names.push_back(a);
      }
    }
    return names;
  }

  /// Finds a direct child matching canonical name or alias, or nullptr.
  CommandNode* find_subcommand(const std::string_view _token) const {
    for (const auto& sub : subcommands_) {
      if (sub->name_ == _token) {
        return sub.get();
      }
      for (const auto& a : sub->aliases_) {
        if (a == _token) {
          return sub.get();
        }
      }
    }
    return nullptr;
  }

  /// Parses `_args` (not including argv[0]), routing down nested subcommands,
  /// validating options at the matched node, and invoking its callback (if
  /// set). On an unknown subcommand, attaches a `"Did you mean '<cmd>'?"`
  /// suggestion when similarity >= `_suggestion_cutoff`.
  rfl::Result<CommandDispatchResult> dispatch(
      const std::vector<std::string>& _args,
      const double _suggestion_cutoff = 0.6) const {
    const CommandNode* current = this;
    std::vector<std::string> path;
    std::vector<std::string> leaf_args;
    leaf_args.reserve(_args.size());

    size_t idx = 0;
    while (idx < _args.size() && !current->subcommands_.empty()) {
      const std::string& tok = _args[idx];
      if (tok == "--") {
        break;
      }
      if (tok.size() >= 2 && tok[0] == '-') {
        // Carry options through to the matched leaf (or current node).
        leaf_args.push_back(tok);
        ++idx;
        // If this option takes a separate value on the current node, carry its
        // value token too so it is not mistaken for a subcommand name.
        if (tok.find('=') == std::string::npos && idx < _args.size()) {
          const bool opt_takes_value = current->option_takes_separate_value(tok);
          if (opt_takes_value) {
            leaf_args.push_back(_args[idx++]);
          }
        }
        continue;
      }
      if (CommandNode* child = current->find_subcommand(tok)) {
        path.push_back(child->name_);
        current = child;
        ++idx;
        continue;
      }
      // First non-option token does not match any subcommand.
      if (current->require_subcommand_) {
        std::string msg =
            "The following argument was not expected: " + tok;
        if (const auto hint =
                did_you_mean(tok, current->subcommand_names(),
                             _suggestion_cutoff)) {
          msg += "\n" + *hint;
        }
        msg += "\nRun with --help for more information.";
        return error(std::move(msg));
      }
      break;
    }

    for (; idx < _args.size(); ++idx) {
      leaf_args.push_back(_args[idx]);
    }

    auto parsed = current->options_.parse(leaf_args);
    if (!parsed) {
      return error(parsed.error().what());
    }
    if (const auto req = current->options_.check_required(parsed->values);
        !req) {
      return error(req.error().what());
    }
    if (current->callback_) {
      current->callback_(*parsed);
    }
    return CommandDispatchResult{std::move(path), std::move(*parsed)};
  }

  rfl::Result<CommandDispatchResult> dispatch(
      const int _argc, const char* const _argv[],
      const double _suggestion_cutoff = 0.6) const {
    std::vector<std::string> args;
    if (_argc > 1) {
      args.reserve(static_cast<size_t>(_argc - 1));
      for (int i = 1; i < _argc; ++i) {
        args.emplace_back(_argv[i]);
      }
    }
    return dispatch(args, _suggestion_cutoff);
  }

  /// Formats help text for this command and its direct subcommands.
  std::string usage() const {
    std::ostringstream oss;
    oss << name_;
    if (!description_.empty()) {
      oss << " - " << description_;
    }
    oss << "\n";
    if (!subcommands_.empty()) {
      oss << "\nSubcommands:\n";
      for (const auto& sub : subcommands_) {
        oss << "  " << sub->name_;
        if (!sub->aliases_.empty()) {
          oss << " (";
          for (size_t i = 0; i < sub->aliases_.size(); ++i) {
            if (i > 0) {
              oss << ", ";
            }
            oss << sub->aliases_[i];
          }
          oss << ")";
        }
        if (!sub->description_.empty()) {
          oss << "  " << sub->description_;
        }
        oss << "\n";
      }
    }
    const auto opt_usage = options_.usage("Options:");
    if (opt_usage != "Options:\n") {
      oss << "\n" << opt_usage;
    }
    return oss.str();
  }

 private:
  bool option_takes_separate_value(const std::string_view _tok) const {
    if (!options_.separate_values) {
      return false;
    }
    if (_tok.starts_with("--")) {
      return options_.is_value_option(_tok.substr(2));
    }
    if (_tok.size() == 2 && _tok[0] == '-') {
      return options_.is_short_value_option(_tok.substr(1));
    }
    return false;
  }

  std::string name_;
  std::string description_;
  std::vector<std::string> aliases_;
  DynamicOptions options_;
  std::vector<std::unique_ptr<CommandNode>> subcommands_;
  Callback callback_;
  bool require_subcommand_ = false;
};

}  // namespace rfl::cli

#endif
