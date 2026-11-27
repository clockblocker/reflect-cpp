#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

namespace test_describe_and_command_tree {

enum class OutputFormat { osm, pbf, o5m };

using MyApp = rfl::cli::App<"osmium-demo", "1.16.0", "OSM data tool">;

struct SortOptions {
  rfl::cli::Flag<rfl::cli::attr::Short<"v">, rfl::cli::attr::Group<"Common">,
                 rfl::cli::attr::Desc<"Enable verbose output">>
      verbose = false;
  rfl::cli::Opt<std::optional<OutputFormat>, rfl::cli::attr::Short<"f">,
                rfl::cli::attr::Group<"Output">,
                rfl::cli::attr::Alias<"output-fmt">,
                rfl::cli::attr::ValueName<"FORMAT">,
                rfl::cli::attr::Desc<"Output file format">>
      output_format;
  rfl::Hidden<rfl::Short<"s", rfl::Description<"Sort strategy",
                                               std::optional<std::string>>>>
      strategy;
  rfl::cli::Opt<std::vector<std::string>, rfl::cli::attr::Short<"t">,
                rfl::cli::attr::Group<"Output">,
                rfl::cli::attr::Desc<"Tags to keep">>
      tags;
  rfl::Positional<rfl::Hidden<
      rfl::Description<"Input OSM files",
                       std::optional<std::vector<std::string>>>>>
      input_filenames;
};

struct MergeOptions {
  rfl::cli::Opt<std::optional<std::string>, rfl::cli::attr::Short<"o">,
                rfl::cli::attr::Desc<"Output filename">>
      output;
  rfl::cli::Flag<rfl::cli::attr::Short<"O">,
                 rfl::cli::attr::Desc<"Allow overwriting existing file">>
      overwrite = false;
  rfl::Positional<std::vector<std::string>> input_files;
};

TEST(cli, test_issue_98_describe_preserves_hidden_positionals_and_metadata) {
  const auto desc_res =
      rfl::cli::describe<SortOptions, MyApp, rfl::DefaultIfMissing>(
          "osmium-demo", rfl::cli::Settings::gnu(),
          rfl::cli::UsageOptions{.detailed = true, .help_group = "Common"});
  ASSERT_TRUE(desc_res) << desc_res.error().what();
  const auto& desc = *desc_res;

  EXPECT_EQ(desc.name, "osmium-demo");
  EXPECT_EQ(desc.version, "1.16.0");
  EXPECT_EQ(desc.description, "OSM data tool");
  EXPECT_EQ(desc.list_delimiter, ",");

  // Public OptionKind and built-in options exposed without internal::* reach-ins.
  const auto find_opt = [&](const std::string_view _n)
      -> const rfl::cli::OptionDescriptor* {
    for (const auto& o : desc.options) {
      if (o.name == _n) return &o;
    }
    return nullptr;
  };

  const auto* help_opt = find_opt("help");
  ASSERT_NE(help_opt, nullptr);
  EXPECT_TRUE(help_opt->builtin);
  EXPECT_EQ(help_opt->short_name, "h");
  EXPECT_EQ(help_opt->kind, rfl::cli::OptionKind::flag);
  EXPECT_EQ(help_opt->group, "Common");

  const auto* ver_opt = find_opt("version");
  ASSERT_NE(ver_opt, nullptr);
  EXPECT_TRUE(ver_opt->builtin);

  const auto* verbose_opt = find_opt("verbose");
  ASSERT_NE(verbose_opt, nullptr);
  EXPECT_FALSE(verbose_opt->builtin);
  EXPECT_FALSE(verbose_opt->hidden);
  EXPECT_TRUE(verbose_opt->bare);
  EXPECT_EQ(verbose_opt->short_name, "v");
  EXPECT_EQ(verbose_opt->group, "Common");

  const auto* fmt_opt = find_opt("output-format");
  ASSERT_NE(fmt_opt, nullptr);
  EXPECT_EQ(fmt_opt->short_name, "f");
  EXPECT_EQ(fmt_opt->kind, rfl::cli::OptionKind::value);
  EXPECT_EQ(fmt_opt->value_name, "<FORMAT>");
  EXPECT_EQ(fmt_opt->value_placeholder, "<FORMAT>");
  EXPECT_TRUE(fmt_opt->has_custom_value_name);
  EXPECT_EQ(fmt_opt->choices, (std::vector<std::string>{"osm", "pbf", "o5m"}));
  EXPECT_EQ(fmt_opt->aliases, (std::vector<std::string>{"output-fmt"}));

  // Hidden named option is preserved with hidden=true!
  const auto* strat_opt = find_opt("strategy");
  ASSERT_NE(strat_opt, nullptr);
  EXPECT_TRUE(strat_opt->hidden);
  EXPECT_EQ(strat_opt->short_name, "s");
  EXPECT_EQ(strat_opt->description, "Sort strategy");

  // List option formats placeholder using public helper without internal::*.
  const auto* tags_opt = find_opt("tags");
  ASSERT_NE(tags_opt, nullptr);
  EXPECT_TRUE(tags_opt->is_list);
  EXPECT_EQ(tags_opt->value_placeholder, "<v1,v2,...>");
  EXPECT_EQ(rfl::cli::format_value_placeholder("<value>", false, true, ','),
            "<v1,v2,...>");

  // Hidden positional is PRESERVED with hidden=true, is_list=true, takes_rest=true!
  ASSERT_EQ(desc.positionals.size(), 1u);
  const auto& pos = desc.positionals[0];
  EXPECT_EQ(pos.name, "input-filenames");
  EXPECT_TRUE(pos.hidden);
  EXPECT_TRUE(pos.is_list);
  EXPECT_TRUE(pos.takes_rest);
  EXPECT_EQ(pos.description, "Input OSM files");

  // Descriptor serializes cleanly to JSON with zero custom code.
  const auto json_str = rfl::json::write(desc);
  EXPECT_NE(json_str.find(R"("name":"input-filenames")"), std::string::npos)
      << json_str;
  EXPECT_NE(json_str.find(R"("hidden":true)"), std::string::npos) << json_str;

  // Regular usage() STILL omits hidden options and hidden positionals.
  const auto help_text =
      rfl::cli::usage<SortOptions, MyApp, rfl::DefaultIfMissing>(
          "osmium-demo", rfl::cli::Settings::gnu());
  ASSERT_TRUE(help_text) << help_text.error().what();
  EXPECT_EQ(help_text->find("strategy"), std::string::npos) << *help_text;
  EXPECT_EQ(help_text->find("input-filenames"), std::string::npos) << *help_text;
}

TEST(cli, test_issue_98_command_node_hosts_static_reflected_structs) {
  rfl::cli::CommandNode root("osmium", "OSM multipurpose command line tool");
  root.require_subcommand(true);

  bool sort_called = false;
  SortOptions captured_sort;
  std::vector<std::string> captured_provided;

  auto& sort_node =
      root.add_subcommand<SortOptions, rfl::DefaultIfMissing>(
              "sort", "Sort OSM data files",
              [&](const rfl::cli::ReadResult<SortOptions>& _r) {
                sort_called = true;
                captured_sort = _r.value;
                captured_provided = _r.provided;
              },
              rfl::cli::Settings::gnu())
          .alias("s");
  EXPECT_TRUE(sort_node.has_static_options());

  // Static subcommand without callback (inspect via CommandDispatchResult::as<T>()).
  root.add_subcommand<MergeOptions, rfl::DefaultIfMissing>(
      "merge", "Merge several sorted OSM files", rfl::cli::Settings::gnu());

  // Mixed tree: also host a DynamicOptions subcommand alongside static ones.
  auto& info_node = root.add_subcommand("fileinfo", "Show info about OSM file");
  info_node.options().add_flag("extended", "e",
                               {.description = "Show extended info"});
  EXPECT_FALSE(info_node.has_static_options());

  // 1. Dispatch to static reflected `sort` via alias with separate option values + hidden positional.
  const auto res1 =
      root.dispatch({"s", "-v", "-f", "pbf", "-s", "multipass", "a.osm", "b.osm"});
  ASSERT_TRUE(res1) << res1.error().what();
  EXPECT_TRUE(sort_called);
  EXPECT_EQ(res1->command_path, (std::vector<std::string>{"sort"}));
  EXPECT_TRUE(captured_sort.verbose.value());
  ASSERT_TRUE(captured_sort.output_format.value().has_value());
  EXPECT_EQ(*captured_sort.output_format.value(), OutputFormat::pbf);
  EXPECT_EQ(captured_sort.strategy.value().value().value(), "multipass");
  ASSERT_TRUE(captured_sort.input_filenames.value().value().value().has_value());
  EXPECT_EQ(*captured_sort.input_filenames.value().value().value(),
            (std::vector<std::string>{"a.osm", "b.osm"}));
  const auto* typed_sort = res1->as<SortOptions>();
  ASSERT_NE(typed_sort, nullptr);
  EXPECT_TRUE(typed_sort->verbose.value());

  // 2. Dispatch to static reflected `merge` without callback; retrieve typed value via as<MergeOptions>().
  const auto res2 = root.dispatch({"merge", "-O", "-o", "out.pbf", "1.pbf", "2.pbf"});
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_EQ(res2->command_path, (std::vector<std::string>{"merge"}));
  const auto* merged = res2->as<MergeOptions>();
  ASSERT_NE(merged, nullptr);
  EXPECT_TRUE(merged->overwrite.value());
  EXPECT_EQ(merged->output.value(), "out.pbf");
  EXPECT_EQ(merged->input_files.value(),
            (std::vector<std::string>{"1.pbf", "2.pbf"}));

  // 3. Dispatch to dynamic subcommand in the same tree still works identically.
  const auto res3 = root.dispatch({"fileinfo", "-e", "map.pbf"});
  ASSERT_TRUE(res3) << res3.error().what();
  EXPECT_EQ(res3->command_path, (std::vector<std::string>{"fileinfo"}));
  EXPECT_EQ(res3->as<MergeOptions>(), nullptr);
  EXPECT_TRUE(res3->parsed.values.count("extended") == 1u);

  // 4. Typo suggestion works across static and dynamic subcommands.
  const auto typo = root.dispatch({"merg"});
  ASSERT_FALSE(typo);
  EXPECT_NE(typo.error().what().find("Did you mean 'merge'?"), std::string::npos);

  // 5. Uniform CommandNode::describe() covers the entire mixed tree + serializes to JSON!
  const auto tree_desc_res = root.describe();
  ASSERT_TRUE(tree_desc_res) << tree_desc_res.error().what();
  const auto& tree_desc = *tree_desc_res;
  EXPECT_EQ(tree_desc.name, "osmium");
  EXPECT_TRUE(tree_desc.require_subcommand);
  ASSERT_EQ(tree_desc.subcommands.size(), 3u);
  EXPECT_EQ(tree_desc.subcommands[0].name, "sort");
  EXPECT_EQ(tree_desc.subcommands[0].aliases, (std::vector<std::string>{"s"}));
  ASSERT_EQ(tree_desc.subcommands[0].positionals.size(), 1u);
  EXPECT_TRUE(tree_desc.subcommands[0].positionals[0].hidden);
  EXPECT_EQ(tree_desc.subcommands[1].name, "merge");
  ASSERT_EQ(tree_desc.subcommands[1].positionals.size(), 1u);
  EXPECT_EQ(tree_desc.subcommands[1].positionals[0].name, "input-files");
  EXPECT_EQ(tree_desc.subcommands[2].name, "fileinfo");
  ASSERT_EQ(tree_desc.subcommands[2].options.size(), 1u);
  EXPECT_EQ(tree_desc.subcommands[2].options[0].name, "extended");

  const auto tree_json = rfl::json::write(tree_desc);
  EXPECT_NE(tree_json.find(R"("name":"merge")"), std::string::npos) << tree_json;
  EXPECT_NE(tree_json.find(R"("name":"fileinfo")"), std::string::npos) << tree_json;

  // 6. CommandNode::usage() works on static-bound nodes too.
  const auto sort_help = sort_node.usage();
  EXPECT_NE(sort_help.find("--output-format=<FORMAT>"), std::string::npos)
      << sort_help;
}

struct MoveOnlyOptions {
  rfl::cli::Opt<std::unique_ptr<int>, rfl::cli::attr::Short<"n">> count;
  rfl::cli::Opt<std::string, rfl::cli::attr::Short<"o">> output = "a.out";
};

struct HiddenChild {
  int secret_a = 1;
  std::string secret_b = "x";
};

struct WithHiddenNested {
  rfl::cli::Opt<int, rfl::cli::attr::Group<"Debug">> debug_port = 9999;
  rfl::Hidden<HiddenChild> hidden_nested;
};

TEST(cli, test_issue_98_move_only_command_node_and_pre_subcommand_routing) {
  rfl::cli::CommandNode root("tool", "Tool with move-only & pre-subcommand opts");
  root.require_subcommand(true);

  // Move-only options struct with generic lambda (must receive const MoveOnlyOptions&).
  int seen_count = -1;
  root.add_subcommand<MoveOnlyOptions, rfl::DefaultIfMissing>(
      "run", "Run with move-only opts",
      [&](const auto& _opts) {
        if (_opts.count.value()) {
          seen_count = *_opts.count.value();
        }
      },
      rfl::cli::Settings::gnu());

  const auto r_move = root.dispatch({"run", "-n", "42", "-o", "res.bin"});
  ASSERT_TRUE(r_move) << r_move.error().what();
  EXPECT_EQ(seen_count, 42);
  const auto* mo = r_move->as<MoveOnlyOptions>();
  ASSERT_NE(mo, nullptr);
  ASSERT_TRUE(mo->count.value() != nullptr);
  EXPECT_EQ(*mo->count.value(), 42);
  EXPECT_EQ(mo->output.value(), "res.bin");

  // Pre-subcommand routing with short value option (separate_values=false!),
  // short cluster (-vo merge), and single_dash_long (-output merge).
  struct RootOpts {
    rfl::cli::Flag<rfl::cli::attr::Short<"v">> verbose = false;
    rfl::cli::Opt<std::string, rfl::cli::attr::Short<"o">> output = "out";
  };
  rfl::cli::CommandNode routed("routed", "Routed root");
  routed.require_subcommand(true);
  // Default settings: separate_values=false, but short -o ALWAYS consumes next token!
  routed.bind<RootOpts, rfl::DefaultIfMissing>(rfl::cli::Settings::default_settings());
  routed.add_subcommand<RootOpts, rfl::DefaultIfMissing>(
      "merge", "Merge subcommand", rfl::cli::Settings::default_settings());
  routed.add_subcommand<RootOpts, rfl::DefaultIfMissing>(
      "sort", "Sort subcommand", rfl::cli::Settings::default_settings());

  // "merge" is the value of -o, NOT a subcommand; "sort" is the subcommand!
  const auto r_short = routed.dispatch({"-o", "merge", "sort"});
  ASSERT_TRUE(r_short) << r_short.error().what();
  EXPECT_EQ(r_short->command_path, (std::vector<std::string>{"sort"}));
  ASSERT_NE(r_short->as<RootOpts>(), nullptr);
  EXPECT_EQ(r_short->as<RootOpts>()->output.value(), "merge");

  // Short cluster `-vo merge sort` also consumes "merge" as -o's value!
  const auto r_cluster = routed.dispatch({"-vo", "merge", "sort"});
  ASSERT_TRUE(r_cluster) << r_cluster.error().what();
  EXPECT_EQ(r_cluster->command_path, (std::vector<std::string>{"sort"}));
  EXPECT_TRUE(r_cluster->as<RootOpts>()->verbose.value());
  EXPECT_EQ(r_cluster->as<RootOpts>()->output.value(), "merge");

  // single_dash_long `-output merge sort` also consumes "merge"!
  rfl::cli::CommandNode sd_root("sd", "Single dash root");
  sd_root.require_subcommand(true);
  sd_root.bind<RootOpts, rfl::DefaultIfMissing>(rfl::cli::Settings::single_dash());
  sd_root.add_subcommand<RootOpts, rfl::DefaultIfMissing>(
      "merge", "Merge", rfl::cli::Settings::single_dash());
  sd_root.add_subcommand<RootOpts, rfl::DefaultIfMissing>(
      "sort", "Sort", rfl::cli::Settings::single_dash());
  const auto r_sd = sd_root.dispatch({"-output", "merge", "sort"});
  ASSERT_TRUE(r_sd) << r_sd.error().what();
  EXPECT_EQ(r_sd->command_path, (std::vector<std::string>{"sort"}));
  EXPECT_EQ(r_sd->as<RootOpts>()->output.value(), "merge");
}

TEST(cli, test_issue_98_describe_hidden_groups_and_nested_hidden_struct) {
  const auto desc = rfl::cli::describe<WithHiddenNested, rfl::DefaultIfMissing>(
      "nested-app",
      rfl::cli::UsageOptions{.detailed = true, .hidden_groups = {"Debug"}});
  ASSERT_TRUE(desc) << desc.error().what();

  bool found_debug_port = false;
  bool found_secret_a = false;
  bool found_secret_b = false;
  for (const auto& opt : desc->options) {
    if (opt.name == "debug-port") {
      found_debug_port = true;
      EXPECT_TRUE(opt.hidden);
    }
    if (opt.name == "hidden-nested.secret-a") {
      found_secret_a = true;
      EXPECT_TRUE(opt.hidden);
    }
    if (opt.name == "hidden-nested.secret-b") {
      found_secret_b = true;
      EXPECT_TRUE(opt.hidden);
    }
  }
  EXPECT_TRUE(found_debug_port);
  EXPECT_TRUE(found_secret_a);
  EXPECT_TRUE(found_secret_b);
}

}  // namespace test_describe_and_command_tree
