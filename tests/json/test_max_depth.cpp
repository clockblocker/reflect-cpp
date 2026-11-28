#include <gtest/gtest.h>

#include <array>
#include <map>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <tuple>
#include <vector>

namespace test_max_depth {

struct Node {
  int value;
  std::optional<rfl::Box<Node>> next;
};

struct DefaultNode {
  int value = 0;
  std::optional<rfl::Box<DefaultNode>> next = std::nullopt;
};

std::string make_nested_arrays(const std::size_t _depth) {
  std::string s;
  s.reserve(_depth * 2 + 1);
  s.append(_depth, '[');
  s.push_back('1');
  s.append(_depth, ']');
  return s;
}

std::string make_nested_objects(const std::size_t _depth) {
  std::string s;
  for (std::size_t i = 0; i < _depth; ++i) {
    s += "{\"a\":";
  }
  s += "1";
  s.append(_depth, '}');
  return s;
}

std::string make_nested_nodes(const std::size_t _depth) {
  std::string s;
  for (std::size_t i = 0; i < _depth; ++i) {
    s += "{\"value\":" + std::to_string(i);
    if (i + 1 < _depth) {
      s += ",\"next\":";
    }
  }
  s.append(_depth, '}');
  return s;
}

TEST(json, test_max_depth_generic_arrays_and_objects) {
  // Exact boundary: depth 3 succeeds with MaxDepth<3>, depth 4 fails.
  const auto arr_ok =
      rfl::json::read<rfl::Generic, rfl::MaxDepth<3>>(make_nested_arrays(3));
  ASSERT_TRUE(arr_ok) << arr_ok.error().what();

  const auto arr_fail =
      rfl::json::read<rfl::Generic, rfl::MaxDepth<3>>(make_nested_arrays(4));
  ASSERT_FALSE(arr_fail);
  EXPECT_NE(arr_fail.error().what().find("Maximum nesting depth of 3 exceeded"),
            std::string::npos);

  const auto obj_ok =
      rfl::json::read<rfl::Generic, rfl::MaxDepth<3>>(make_nested_objects(3));
  ASSERT_TRUE(obj_ok) << obj_ok.error().what();

  const auto obj_fail =
      rfl::json::read<rfl::Generic, rfl::MaxDepth<3>>(make_nested_objects(4));
  ASSERT_FALSE(obj_fail);
  EXPECT_NE(obj_fail.error().what().find("Maximum nesting depth of 3 exceeded"),
            std::string::npos);

  // Deeply nested input (5,000 levels) must fail cleanly without stack overflow.
  const auto dos_res = rfl::json::read<rfl::Generic, rfl::MaxDepth<32>>(
      make_nested_arrays(5000));
  ASSERT_FALSE(dos_res);
  EXPECT_NE(dos_res.error().what().find("Maximum nesting depth of 32 exceeded"),
            std::string::npos);

  // Thread-local depth counter must recover cleanly after a failure.
  const auto after_ok =
      rfl::json::read<rfl::Generic, rfl::MaxDepth<3>>(make_nested_arrays(3));
  ASSERT_TRUE(after_ok) << after_ok.error().what();
}

TEST(json, test_max_depth_recursive_structs_and_default_if_missing) {
  const auto ok =
      rfl::json::read<Node, rfl::MaxDepth<3>>(make_nested_nodes(3));
  ASSERT_TRUE(ok) << ok.error().what();
  EXPECT_EQ(ok->value, 0);
  ASSERT_TRUE(ok->next.has_value());
  EXPECT_EQ((*ok->next)->value, 1);

  const auto fail =
      rfl::json::read<Node, rfl::MaxDepth<3>>(make_nested_nodes(4));
  ASSERT_FALSE(fail);
  EXPECT_NE(fail.error().what().find("Maximum nesting depth of 3 exceeded"),
            std::string::npos);

  // Deep recursive struct input (5,000 levels) fails cleanly without crash.
  const auto dos_node =
      rfl::json::read<Node, rfl::MaxDepth<32>>(make_nested_nodes(5000));
  ASSERT_FALSE(dos_node);
  EXPECT_NE(
      dos_node.error().what().find("Maximum nesting depth of 32 exceeded"),
      std::string::npos);

  // Also works with DefaultIfMissing (read_view_with_default) and nested
  // Processors<...> bundles.
  using Procs = rfl::Processors<rfl::DefaultIfMissing, rfl::MaxDepth<3>>;
  const auto def_ok =
      rfl::json::read<DefaultNode, Procs>(make_nested_nodes(3));
  ASSERT_TRUE(def_ok) << def_ok.error().what();

  const auto def_fail =
      rfl::json::read<DefaultNode, Procs>(make_nested_nodes(4));
  ASSERT_FALSE(def_fail);
  EXPECT_NE(def_fail.error().what().find("Maximum nesting depth of 3 exceeded"),
            std::string::npos);
}

TEST(json, test_max_depth_maps_arrays_and_tuples) {
  using NestedMap = std::map<std::string, std::map<std::string, int>>;
  ASSERT_TRUE((rfl::json::read<NestedMap, rfl::MaxDepth<2>>(
      R"({"a":{"b":1}})")));
  ASSERT_FALSE((rfl::json::read<NestedMap, rfl::MaxDepth<1>>(
      R"({"a":{"b":1}})")));

  using NestedArray = std::array<std::array<int, 1>, 1>;
  ASSERT_TRUE((rfl::json::read<NestedArray, rfl::MaxDepth<2>>(R"([[1]])")));
  ASSERT_FALSE((rfl::json::read<NestedArray, rfl::MaxDepth<1>>(R"([[1]])")));

  using NestedTuple = std::tuple<std::tuple<int>>;
  ASSERT_TRUE((rfl::json::read<NestedTuple, rfl::MaxDepth<2>>(R"([[1]])")));
  ASSERT_FALSE((rfl::json::read<NestedTuple, rfl::MaxDepth<1>>(R"([[1]])")));
}

}  // namespace test_max_depth
