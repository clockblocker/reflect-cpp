#ifndef RFL_CLI_COMMANDTREE_HPP_
#define RFL_CLI_COMMANDTREE_HPP_

#include <algorithm>
#include <any>
#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../Result.hpp"
#include "DynamicOptions.hpp"
#include "Settings.hpp"
#include "describe.hpp"
#include "read.hpp"
#include "suggest.hpp"
#include "usage.hpp"

namespace rfl::cli {

/// Result of resolving a command path and parsing arguments through a
/// `CommandNode` tree.
struct CommandDispatchResult {
  /// Sequence of matched canonical subcommand names from root to leaf.
  std::vector<std::string> command_path;
  /// Parsed options and positional arguments for the matched leaf command
  /// (populated for dynamic nodes; `occurrences` is also populated for static
  /// reflected nodes).
  DynamicParseResult parsed;
  /// Sorted canonical names of options explicitly provided on the command line
  /// (populated for static reflected nodes and dynamic nodes).
  std::vector<std::string> provided;

  /// Returns a pointer to the parsed static options struct `T`, or `nullptr`
  /// if the matched command did not host `T`. Supports both copyable and
  /// move-only `T` (e.g. structs holding `std::unique_ptr` or `rfl::Box`).
  template <class T>
  const T* as() const noexcept {
    using CleanT = std::remove_cv_t<T>;
    if (const auto* ptr =
            std::any_cast<const std::shared_ptr<CleanT>>(&typed_storage_)) {
      return ptr->get();
    }
    return nullptr;
  }

  template <class T>
  T* as() noexcept {
    using CleanT = std::remove_cv_t<T>;
    if (auto* ptr = std::any_cast<std::shared_ptr<CleanT>>(&typed_storage_)) {
      return ptr->get();
    }
    return nullptr;
  }

  bool has_typed_value() const noexcept { return typed_storage_.has_value(); }

 private:
  friend class CommandNode;
  std::any typed_storage_;
};

/// Arbitrarily nested subcommand router that can host BOTH runtime
/// `DynamicOptions` AND static reflected `<Cmd>Options` C++ structs directly.
/// Supports subcommand aliases, per-command option tables, `options_first`
/// prefix commands (e.g. `micromamba run`), `"Did you mean '...'?"`
/// suggestions at any nesting depth, and uniform `describe()` introspection
/// across the whole command tree.
class CommandNode {
 public:
  using Callback = std::function<void(const DynamicParseResult&)>;

  CommandNode(std::string _name, std::string _description = {})
      : name_(std::move(_name)), description_(std::move(_description)) {}

  const std::string& name() const noexcept { return name_; }
  const std::string& description() const noexcept { return description_; }
  const std::vector<std::string>& aliases() const noexcept { return aliases_; }
  bool has_static_options() const noexcept {
    return static_binding_ != nullptr;
  }

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

  /// Binds this command node to a static reflected options struct `CmdOpts`
  /// (with optional processors `Ps...` and optional runtime `Settings`).
  /// `_cb` may accept `const CmdOpts&`, `const ReadResult<CmdOpts>&`, or be
  /// omitted (in which case the parsed struct is returned via
  /// `CommandDispatchResult::as<CmdOpts>()`).
  template <class CmdOpts, class... Ps, class Fn>
    requires(std::invocable<Fn&, const CmdOpts&> ||
             std::invocable<Fn&, const ReadResult<CmdOpts>&>)
  CommandNode& bind(Fn _cb,
                    std::optional<Settings> _settings = std::nullopt) {
    auto binding = std::make_shared<StaticBinding>();
    auto shared_cb = std::make_shared<std::decay_t<Fn>>(std::move(_cb));
    binding->parse = [_settings, shared_cb](
                         const std::vector<std::string>& _args)
        -> rfl::Result<StaticParseOutput> {
      auto res = _settings
                     ? rfl::cli::read_detailed<CmdOpts, Ps...>(_args,
                                                               *_settings)
                     : rfl::cli::read_detailed<CmdOpts, Ps...>(_args);
      if (!res) {
        return error(res.error().what());
      }
      if constexpr (std::invocable<std::decay_t<Fn>&, const CmdOpts&>) {
        (*shared_cb)(res->value);
      } else {
        (*shared_cb)(*res);
      }
      StaticParseOutput out;
      out.provided = std::move(res->provided);
      out.occurrences = std::move(res->occurrences);
      out.typed_storage =
          std::make_shared<CmdOpts>(std::move(res->value));
      return out;
    };
    init_static_introspection<CmdOpts, Ps...>(binding.get(), _settings);
    static_binding_ = std::move(binding);
    return *this;
  }

  /// Binds this command node to a static reflected options struct `CmdOpts`
  /// without a callback (inspect the parsed value via
  /// `CommandDispatchResult::as<CmdOpts>()`).
  template <class CmdOpts, class... Ps>
  CommandNode& bind(std::optional<Settings> _settings = std::nullopt) {
    auto binding = std::make_shared<StaticBinding>();
    binding->parse =
        [_settings](const std::vector<std::string>& _args)
        -> rfl::Result<StaticParseOutput> {
      auto res = _settings
                     ? rfl::cli::read_detailed<CmdOpts, Ps...>(_args,
                                                               *_settings)
                     : rfl::cli::read_detailed<CmdOpts, Ps...>(_args);
      if (!res) {
        return error(res.error().what());
      }
      StaticParseOutput out;
      out.provided = std::move(res->provided);
      out.occurrences = std::move(res->occurrences);
      out.typed_storage =
          std::make_shared<CmdOpts>(std::move(res->value));
      return out;
    };
    init_static_introspection<CmdOpts, Ps...>(binding.get(), _settings);
    static_binding_ = std::move(binding);
    return *this;
  }

  /// Registers and returns a dynamic child subcommand.
  CommandNode& add_subcommand(std::string _name,
                              std::string _description = {}) {
    subcommands_.push_back(std::make_unique<CommandNode>(
        std::move(_name), std::move(_description)));
    return *subcommands_.back();
  }

  /// Registers and returns a child subcommand bound directly to a static
  /// reflected `<Cmd>Options` struct `CmdOpts` with a typed callback.
  template <class CmdOpts, class... Ps, class Fn>
    requires(std::invocable<Fn&, const CmdOpts&> ||
             std::invocable<Fn&, const ReadResult<CmdOpts>&>)
  CommandNode& add_subcommand(std::string _name, std::string _description,
                              Fn&& _cb,
                              std::optional<Settings> _settings = std::nullopt) {
    auto& node = add_subcommand(std::move(_name), std::move(_description));
    node.bind<CmdOpts, Ps...>(std::forward<Fn>(_cb), std::move(_settings));
    return node;
  }

  /// Registers and returns a child subcommand bound directly to a static
  /// reflected `<Cmd>Options` struct `CmdOpts` (without a callback).
  template <class CmdOpts, class... Ps>
  CommandNode& add_subcommand(std::string _name,
                              std::string _description = {},
                              std::optional<Settings> _settings = std::nullopt) {
    auto& node = add_subcommand(std::move(_name), std::move(_description));
    node.bind<CmdOpts, Ps...>(std::move(_settings));
    return node;
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
          const bool opt_takes_value =
              current->option_takes_separate_value(tok, _args[idx]);
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

    if (current->static_binding_) {
      auto parsed = current->static_binding_->parse(leaf_args);
      if (!parsed) {
        return error(parsed.error().what());
      }
      DynamicParseResult dyn;
      dyn.occurrences = parsed->occurrences;
      for (const auto& [k, v] : parsed->occurrences) {
        dyn.values[k] = v;
      }
      if (current->callback_) {
        current->callback_(dyn);
      }
      CommandDispatchResult result;
      result.command_path = std::move(path);
      result.parsed = std::move(dyn);
      result.provided = std::move(parsed->provided);
      result.typed_storage_ = std::move(parsed->typed_storage);
      return result;
    }

    auto parsed = current->options_.parse(leaf_args);
    if (!parsed) {
      return error(parsed.error().what());
    }
    if (const auto req = current->options_.check_required(parsed->values);
        !req) {
      return error(req.error().what());
    }
    std::vector<std::string> provided;
    provided.reserve(parsed->values.size());
    for (const auto& [k, _v] : parsed->values) {
      provided.push_back(k);
    }
    if (current->callback_) {
      current->callback_(*parsed);
    }
    CommandDispatchResult result;
    result.command_path = std::move(path);
    result.parsed = std::move(*parsed);
    result.provided = std::move(provided);
    return result;
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

  /// Builds a uniform public `CommandDescriptor` tree covering this command
  /// (whether backed by a static reflected `<Cmd>Options` struct or runtime
  /// `DynamicOptions`) and all descendant subcommands recursively.
  rfl::Result<CommandDescriptor> describe() const {
    CommandDescriptor desc;
    if (static_binding_) {
      auto static_desc = static_binding_->describe(name_, description_);
      if (!static_desc) {
        return error(static_desc.error().what());
      }
      desc = std::move(*static_desc);
    } else {
      desc.name = name_;
      desc.description = description_;
      desc.options_first = options_.options_first;
      const auto& compiled = options_.compiled_options();
      for (const auto& m : options_.registered_options()) {
        OptionDescriptor od;
        od.name = m.long_name;
        od.short_name = m.short_name;
        od.aliases = internal::collect_aliases_for(compiled, m.long_name);
        od.negated_names = internal::collect_negated_for(compiled, m.long_name);
        od.kind = m.kind;
        od.implicit_value = m.implicit_value;
        od.is_list = m.is_list;
        od.is_counter = m.is_counter;
        od.multi_token = m.multi_token;
        od.sensitive = m.sensitive;
        od.required = m.required;
        od.bare = (options_.pure_flags || m.is_counter) &&
                  m.kind == OptionKind::flag;
        od.hidden = m.hidden;
        od.value_name = (m.kind == OptionKind::flag) ? "<bool>" : "<value>";
        od.value_placeholder = od.value_name;
        od.description = m.description;
        od.default_hint = m.default_hint;
        if (!m.default_hint.empty()) {
          od.default_value = m.default_hint;
        }
        od.group = m.group;
        od.excludes =
            internal::collect_relations_for(compiled.exclusions, m.long_name);
        od.needs =
            internal::collect_relations_for(compiled.requirements, m.long_name);
        desc.options.push_back(std::move(od));
      }
    }
    desc.name = name_;
    if (!description_.empty()) {
      desc.description = description_;
    }
    desc.aliases = aliases_;
    desc.require_subcommand = require_subcommand_;
    desc.subcommands.reserve(subcommands_.size());
    for (const auto& sub : subcommands_) {
      auto child_desc = sub->describe();
      if (!child_desc) {
        return error(child_desc.error().what());
      }
      desc.subcommands.push_back(std::move(*child_desc));
    }
    return desc;
  }

  /// Formats help text for this command and its direct subcommands.
  std::string usage() const {
    std::ostringstream oss;
    if (static_binding_) {
      const auto static_help = static_binding_->usage(name_, description_);
      if (static_help) {
        oss << *static_help;
      } else {
        oss << name_;
        if (!description_.empty()) {
          oss << " - " << description_;
        }
        oss << "\n";
      }
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
      return oss.str();
    }

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
  struct StaticParseOutput {
    std::vector<std::string> provided;
    std::vector<std::pair<std::string, std::string>> occurrences;
    std::any typed_storage;
  };

  struct StaticBinding {
    std::function<rfl::Result<StaticParseOutput>(
        const std::vector<std::string>&)>
        parse;
    std::function<bool(std::string_view, std::string_view)>
        takes_separate_value;
    std::function<rfl::Result<CommandDescriptor>(const std::string&,
                                                 const std::string&)>
        describe;
    std::function<rfl::Result<std::string>(const std::string&,
                                           const std::string&)>
        usage;
  };

  template <class CmdOpts, class... Ps>
  static void init_static_introspection(
      StaticBinding* _binding, const std::optional<Settings>& _settings) {
    using ProcessorsType = internal::cli_processors_t<Ps...>;
    _binding->takes_separate_value = [_settings](const std::string_view _tok,
                                                 const std::string_view _next) {
      const auto& base =
          internal::cached_options<CmdOpts, ProcessorsType,
                                   /*_all_paths=*/true>();
      if (_settings) {
        const auto opts = internal::apply_settings(base, *_settings);
        return internal::token_consumes_next_arg(_tok, _next, opts);
      }
      return internal::token_consumes_next_arg(_tok, _next, base);
    };
    _binding->describe = [_settings](const std::string& _name,
                                     const std::string& _desc) {
      UsageOptions uo;
      uo.detailed = true;
      uo.description = _desc;
      return _settings
                 ? rfl::cli::describe<CmdOpts, Ps...>(_name, *_settings, uo)
                 : rfl::cli::describe<CmdOpts, Ps...>(_name, uo);
    };
    _binding->usage = [_settings](const std::string& _name,
                                  const std::string& _desc) {
      UsageOptions uo;
      uo.detailed = true;
      uo.description = _desc;
      return _settings
                 ? rfl::cli::usage<CmdOpts, Ps...>(_name, *_settings, uo)
                 : rfl::cli::usage<CmdOpts, Ps...>(_name, uo);
    };
  }

  bool option_takes_separate_value(const std::string_view _tok,
                                   const std::string_view _next) const {
    if (static_binding_) {
      return static_binding_->takes_separate_value(_tok, _next);
    }
    return internal::token_consumes_next_arg(_tok, _next,
                                             options_.compiled_options());
  }

  std::string name_;
  std::string description_;
  std::vector<std::string> aliases_;
  DynamicOptions options_;
  std::shared_ptr<StaticBinding> static_binding_;
  std::vector<std::unique_ptr<CommandNode>> subcommands_;
  Callback callback_;
  bool require_subcommand_ = false;
};

}  // namespace rfl::cli

#endif
