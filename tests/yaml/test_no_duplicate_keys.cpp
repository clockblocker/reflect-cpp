#include <gtest/gtest.h>

#include <map>
#include <rfl.hpp>
#include <rfl/generic/Reader.hpp>
#include <rfl/yaml.hpp>
#include <string>
#include <vector>

namespace test_yaml_no_duplicate_keys {

static_assert(rfl::internal::reader_supports_no_duplicate_keys_v<
              rfl::yaml::Reader>);
static_assert(!rfl::internal::reader_supports_no_duplicate_keys_v<
              rfl::generic::Reader>);

using Path = std::vector<rfl::ErrorDetail::PathSegment>;

struct Item {
  std::string name;
  int code;
};

struct Entry {
  std::string name;
  int code;
  int port;
};

struct WithDefault {
  int code = 0;
  std::string name = "default";
};

TEST(yaml, test_duplicate_keys_accepted_by_default_for_backwards_compat) {
  const std::string yaml_str =
      "- name: a\n"
      "  code: 1\n"
      "  code: 2\n";

  const auto res = rfl::yaml::read<std::vector<Item>>(yaml_str);
  ASSERT_TRUE(res) << res.error().what();
  ASSERT_EQ(res->size(), 1u);
  EXPECT_EQ((*res)[0].name, "a");
  EXPECT_EQ((*res)[0].code, 1);
}

TEST(yaml, test_no_duplicate_keys_processor_rejects_repeated_keys) {
  const std::string yaml_str =
      "- name: a\n"
      "  code: 1\n"
      "  code: 2\n";

  const auto res1 =
      rfl::yaml::read<std::vector<Item>, rfl::NoDuplicateKeys>(yaml_str);
  ASSERT_FALSE(res1);
  const auto details1 = res1.error().details();
  ASSERT_EQ(details1.size(), 1u) << res1.error().what();
  EXPECT_EQ(details1[0].path, (Path{size_t{0}, "code"}));
  EXPECT_EQ(details1[0].message, "Duplicate key 'code'.");

  const auto res2 =
      rfl::yaml::read<std::vector<Item>, rfl::NoExtraFields,
                      rfl::NoDuplicateKeys>(yaml_str);
  ASSERT_FALSE(res2);
  const auto details2 = res2.error().details();
  ASSERT_EQ(details2.size(), 1u) << res2.error().what();
  EXPECT_EQ(details2[0].path, (Path{size_t{0}, "code"}));
  EXPECT_EQ(details2[0].message, "Duplicate key 'code'.");
}

TEST(yaml, test_no_duplicate_keys_aggregated_with_other_errors_and_fail_fast) {
  const std::string yaml_str =
      "code: 1\n"
      "code: 2\n"
      "port: bad\n";

  const auto result =
      rfl::yaml::read<Entry, rfl::NoExtraFields, rfl::NoDuplicateKeys>(
          yaml_str);
  ASSERT_FALSE(result);

  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 3u) << result.error().what();
  EXPECT_EQ(details[0].path, (Path{"code"}));
  EXPECT_EQ(details[0].message, "Duplicate key 'code'.");
  EXPECT_EQ(details[1].path, (Path{"port"}));
  EXPECT_EQ(details[2].path, (Path{"name"}));
  EXPECT_EQ(details[2].message, "Field named 'name' not found.");

  const auto ff_res =
      rfl::yaml::read<Entry, rfl::NoDuplicateKeys, rfl::FailFast>(yaml_str);
  ASSERT_FALSE(ff_res);
  ASSERT_EQ(ff_res.error().details().size(), 1u) << ff_res.error().what();
  EXPECT_EQ(ff_res.error().details()[0].message, "Duplicate key 'code'.");
}

TEST(yaml, test_no_duplicate_keys_in_map_and_default_if_missing) {
  const std::string map_yaml =
      "a: 1\n"
      "b: 2\n"
      "a: 3\n";
  const auto map_res =
      rfl::yaml::read<std::map<std::string, int>, rfl::NoDuplicateKeys>(
          map_yaml);
  ASSERT_FALSE(map_res);
  const auto map_details = map_res.error().details();
  ASSERT_EQ(map_details.size(), 1u) << map_res.error().what();
  EXPECT_EQ(map_details[0].path, (Path{"a"}));
  EXPECT_EQ(map_details[0].message, "Duplicate key 'a'.");

  const std::string def_yaml =
      "code: 1\n"
      "code: 2\n"
      "extra: 3\n";
  const auto def_res =
      rfl::yaml::read<WithDefault, rfl::DefaultIfMissing, rfl::NoExtraFields,
                      rfl::NoDuplicateKeys>(def_yaml);
  ASSERT_FALSE(def_res);
  const auto def_details = def_res.error().details();
  ASSERT_EQ(def_details.size(), 2u) << def_res.error().what();
  EXPECT_EQ(def_details[0].path, (Path{"code"}));
  EXPECT_EQ(def_details[0].message, "Duplicate key 'code'.");
  EXPECT_EQ(def_details[1].path, (Path{"extra"}));
}

}  // namespace test_yaml_no_duplicate_keys
