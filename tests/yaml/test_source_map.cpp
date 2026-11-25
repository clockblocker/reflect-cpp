#include <gtest/gtest.h>

#include <optional>
#include <rfl/yaml.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace test_source_map {

struct Endpoint {
  std::string host;
  int port;
  std::optional<std::string> tls;
};

struct Config {
  std::string name;
  std::vector<Endpoint> endpoints;
};

struct Pair {
  Endpoint primary;
  Endpoint backup;
};

TEST(yaml, test_source_map_nested_paths_and_keys) {
  const std::string yaml_str =
      "name: demo\n"
      "endpoints:\n"
      "  - host: alpha\n"
      "    port: 8080\n"
      "  - host: beta\n"
      "    port: 9090\n"
      "    tls: strict\n";

  rfl::SourceMap source_map;
  const auto res = rfl::yaml::read<Config>(yaml_str, &source_map);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_FALSE(source_map.empty());

  // Root node.
  EXPECT_EQ(source_map.find({}),
            (rfl::SourceLocation{.line = 1, .column = 1, .exact = true}));
  EXPECT_EQ(source_map.find_key({}), std::nullopt);

  // Top-level scalar value and key.
  EXPECT_EQ(source_map.find({"name"}),
            (rfl::SourceLocation{.line = 1, .column = 7, .exact = true}));
  EXPECT_EQ(source_map.find_key({"name"}),
            (rfl::SourceLocation{.line = 1, .column = 1, .exact = true}));

  // Nested sequence element fields.
  EXPECT_EQ(source_map.find({"endpoints", size_t{0}, "host"}),
            (rfl::SourceLocation{.line = 3, .column = 11, .exact = true}));
  EXPECT_EQ(source_map.find_key({"endpoints", size_t{0}, "host"}),
            (rfl::SourceLocation{.line = 3, .column = 5, .exact = true}));
  EXPECT_EQ(source_map.find({"endpoints", size_t{1}, "port"}),
            (rfl::SourceLocation{.line = 6, .column = 11, .exact = true}));
  EXPECT_EQ(source_map.find_key({"endpoints", size_t{1}, "port"}),
            (rfl::SourceLocation{.line = 6, .column = 5, .exact = true}));
  EXPECT_EQ(source_map.find({"endpoints", size_t{1}, "tls"}),
            (rfl::SourceLocation{.line = 7, .column = 10, .exact = true}));

  // find_entry returns both value and key locations together.
  const auto entry = source_map.find_entry({"endpoints", size_t{1}, "port"});
  ASSERT_TRUE(entry.has_value());
  EXPECT_EQ(entry->value,
            (rfl::SourceLocation{.line = 6, .column = 11, .exact = true}));
  EXPECT_EQ(entry->key,
            (rfl::SourceLocation{.line = 6, .column = 5, .exact = true}));

  // Path vector interoperability with ErrorDetail::PathSegment.
  const std::vector<rfl::SourceMap::PathSegment> path = {"endpoints", size_t{1},
                                                         "host"};
  EXPECT_EQ(source_map.find(path),
            (rfl::SourceLocation{.line = 5, .column = 11, .exact = true}));

  // Omitted optional field: find() returns nullopt, find_nearest() falls back
  // to the enclosing element mapping with exact = false.
  EXPECT_EQ(source_map.find({"endpoints", size_t{0}, "tls"}), std::nullopt);
  EXPECT_EQ(source_map.find_key({"endpoints", size_t{0}, "tls"}), std::nullopt);
  EXPECT_EQ(source_map.find_nearest({"endpoints", size_t{0}, "tls"}),
            (rfl::SourceLocation{.line = 3, .column = 5, .exact = false}));

  // Out-of-bounds sequence index falls back to the sequence node.
  EXPECT_EQ(source_map.find({"endpoints", size_t{5}, "host"}), std::nullopt);
  EXPECT_EQ(source_map.find_nearest({"endpoints", size_t{5}, "host"}),
            (rfl::SourceLocation{.line = 3, .column = 3, .exact = false}));
}

TEST(yaml, test_source_map_dag_aliases_and_stream) {
  const std::string yaml_str =
      "primary: &ep\n"
      "  host: shared\n"
      "  port: 443\n"
      "backup: *ep\n";

  std::istringstream iss(yaml_str);
  rfl::SourceMap source_map;
  const auto res = rfl::yaml::read<Pair>(iss, &source_map);
  ASSERT_TRUE(res.has_value()) << res.error().what();

  EXPECT_EQ(source_map.find({"primary", "port"}),
            (rfl::SourceLocation{.line = 3, .column = 9, .exact = true}));
  EXPECT_EQ(source_map.find({"backup", "port"}),
            (rfl::SourceLocation{.line = 3, .column = 9, .exact = true}));
  EXPECT_EQ(source_map.find_key({"backup"}),
            (rfl::SourceLocation{.line = 4, .column = 1, .exact = true}));
}

TEST(yaml, test_source_map_cleared_on_failure_and_nullptr) {
  const std::string valid_yaml =
      "name: ok\n"
      "endpoints: []\n";
  const std::string invalid_yaml =
      "name: bad\n"
      "endpoints:\n"
      "  - host: alpha\n"
      "    port: nope\n";

  // nullptr source_map is accepted without error.
  const auto ok_null =
      rfl::yaml::read<Config>(valid_yaml, static_cast<rfl::SourceMap*>(nullptr));
  ASSERT_TRUE(ok_null.has_value()) << ok_null.error().what();

  rfl::SourceMap source_map;
  ASSERT_TRUE(rfl::yaml::read<Config>(valid_yaml, &source_map).has_value());
  EXPECT_FALSE(source_map.empty());

  // Failed read clears any previous entries.
  const auto bad = rfl::yaml::read<Config>(invalid_yaml, &source_map);
  ASSERT_FALSE(bad.has_value());
  EXPECT_TRUE(source_map.empty());
  EXPECT_EQ(source_map.find({"name"}), std::nullopt);
  EXPECT_EQ(source_map.find_nearest({"name"}), std::nullopt);
}

TEST(yaml, test_source_map_copy_move_equality_and_builder) {
  rfl::SourceMap::Builder builder(
      {.value = rfl::SourceLocation{.line = 1, .column = 1, .exact = true}});
  const auto root = builder.root();
  const auto items = builder.add_field(
      root, "items",
      {.value = rfl::SourceLocation{.line = 2, .column = 3, .exact = true},
       .key = rfl::SourceLocation{.line = 2, .column = 1, .exact = true}});
  builder.add_element(
      items,
      {.value = rfl::SourceLocation{.line = 3, .column = 5, .exact = true}});

  // Adding a duplicate field preserves the first occurrence.
  builder.add_field(
      root, "items",
      {.value = rfl::SourceLocation{.line = 99, .column = 1, .exact = true}});

  rfl::SourceMap map = std::move(builder).build();
  EXPECT_EQ(map.find({"items", size_t{0}}),
            (rfl::SourceLocation{.line = 3, .column = 5, .exact = true}));

  // Copy & equality.
  rfl::SourceMap copy = map;
  EXPECT_EQ(copy, map);

  // Move & empty equality.
  rfl::SourceMap moved = std::move(copy);
  EXPECT_EQ(moved, map);
  EXPECT_TRUE(copy.empty());
  EXPECT_EQ(copy, rfl::SourceMap{});

  // Clearing makes it equal to default-constructed empty SourceMap.
  moved.clear();
  EXPECT_TRUE(moved.empty());
  EXPECT_EQ(moved, rfl::SourceMap{});
}

}  // namespace test_source_map
