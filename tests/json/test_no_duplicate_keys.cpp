#include <gtest/gtest.h>

#include <map>
#include <rfl.hpp>
#include <rfl/generic/Reader.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

namespace test_json_no_duplicate_keys {

static_assert(rfl::internal::reader_supports_no_duplicate_keys_v<
              rfl::json::Reader>);
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

struct Circle {
  using Tag = rfl::Literal<"circle">;
  double radius;
};

struct Square {
  using Tag = rfl::Literal<"square">;
  double side;
};

using Shape = rfl::TaggedUnion<"kind", Circle, Square>;

TEST(json, test_duplicate_keys_accepted_by_default_for_backwards_compat) {
  const std::string json_str = R"([{"name":"a","code":1,"code":2}])";

  const auto res = rfl::json::read<std::vector<Item>>(json_str);
  ASSERT_TRUE(res) << res.error().what();
  ASSERT_EQ(res->size(), 1u);
  EXPECT_EQ((*res)[0].name, "a");
  EXPECT_EQ((*res)[0].code, 1);
}

TEST(json, test_no_duplicate_keys_rejects_repeated_struct_fields) {
  const std::string json_str = R"([{"name":"a","code":1,"code":2}])";

  const auto res1 =
      rfl::json::read<std::vector<Item>, rfl::NoDuplicateKeys>(json_str);
  ASSERT_FALSE(res1);
  const auto details1 = res1.error().details();
  ASSERT_EQ(details1.size(), 1u) << res1.error().what();
  EXPECT_EQ(details1[0].path, (Path{size_t{0}, "code"}));
  EXPECT_EQ(details1[0].message, "Duplicate key 'code'.");
  EXPECT_EQ(details1[0].kind, rfl::ErrorKind::duplicate_field);

  // With NoExtraFields + NoDuplicateKeys: reported as duplicate_field, NOT
  // unknown_field.
  const auto res2 =
      rfl::json::read<std::vector<Item>, rfl::NoExtraFields,
                      rfl::NoDuplicateKeys>(json_str);
  ASSERT_FALSE(res2);
  const auto details2 = res2.error().details();
  ASSERT_EQ(details2.size(), 1u) << res2.error().what();
  EXPECT_EQ(details2[0].path, (Path{size_t{0}, "code"}));
  EXPECT_EQ(details2[0].message, "Duplicate key 'code'.");
  EXPECT_EQ(details2[0].kind, rfl::ErrorKind::duplicate_field);
}

TEST(json, test_no_duplicate_keys_aggregated_with_other_errors_and_fail_fast) {
  const std::string json_str = R"({"code":1,"code":2,"port":"bad"})";

  const auto res =
      rfl::json::read<Entry, rfl::NoExtraFields, rfl::NoDuplicateKeys>(
          json_str);
  ASSERT_FALSE(res);

  const auto details = res.error().details();
  ASSERT_EQ(details.size(), 3u) << res.error().what();
  EXPECT_EQ(details[0].path, (Path{"code"}));
  EXPECT_EQ(details[0].message, "Duplicate key 'code'.");
  EXPECT_EQ(details[0].kind, rfl::ErrorKind::duplicate_field);
  EXPECT_EQ(details[1].path, (Path{"port"}));
  EXPECT_EQ(details[2].path, (Path{"name"}));
  EXPECT_EQ(details[2].kind, rfl::ErrorKind::missing_field);

  const auto ff =
      rfl::json::read<Entry, rfl::NoDuplicateKeys, rfl::FailFast>(json_str);
  ASSERT_FALSE(ff);
  ASSERT_EQ(ff.error().details().size(), 1u) << ff.error().what();
  EXPECT_EQ(ff.error().details()[0].message, "Duplicate key 'code'.");
  EXPECT_EQ(ff.error().details()[0].kind, rfl::ErrorKind::duplicate_field);
}

TEST(json, test_no_duplicate_keys_maps_defaults_generic_and_tagged_union) {
  // std::map
  const auto map_res =
      rfl::json::read<std::map<std::string, int>, rfl::NoDuplicateKeys>(
          R"({"a":1,"b":2,"a":3})");
  ASSERT_FALSE(map_res);
  ASSERT_EQ(map_res.error().details().size(), 1u) << map_res.error().what();
  EXPECT_EQ(map_res.error().details()[0].path, (Path{"a"}));
  EXPECT_EQ(map_res.error().details()[0].kind, rfl::ErrorKind::duplicate_field);

  // rfl::Object
  const auto obj_res =
      rfl::json::read<rfl::Object<int>, rfl::NoDuplicateKeys>(
          R"({"x":1,"x":2})");
  ASSERT_FALSE(obj_res);
  EXPECT_EQ(obj_res.error().details()[0].kind, rfl::ErrorKind::duplicate_field);

  // DefaultIfMissing + NoExtraFields + NoDuplicateKeys
  const auto def_res =
      rfl::json::read<WithDefault, rfl::DefaultIfMissing, rfl::NoExtraFields,
                      rfl::NoDuplicateKeys>(
          R"({"code":1,"code":2,"extra":3})");
  ASSERT_FALSE(def_res);
  ASSERT_EQ(def_res.error().details().size(), 2u) << def_res.error().what();
  EXPECT_EQ(def_res.error().details()[0].kind, rfl::ErrorKind::duplicate_field);
  EXPECT_EQ(def_res.error().details()[1].kind, rfl::ErrorKind::unknown_field);

  // rfl::Generic (top-level and nested)
  const auto gen_ok = rfl::json::read<rfl::Generic, rfl::NoDuplicateKeys>(
      R"({"outer":{"a":1,"b":2},"list":[{"k":1},{"k":2}]})");
  ASSERT_TRUE(gen_ok) << gen_ok.error().what();

  const auto gen_dup_top = rfl::json::read<rfl::Generic, rfl::NoDuplicateKeys>(
      R"({"a":1,"a":2})");
  ASSERT_FALSE(gen_dup_top);
  EXPECT_NE(gen_dup_top.error().what().find("Duplicate key 'a'"),
            std::string::npos);

  const auto gen_dup_nested =
      rfl::json::read<rfl::Generic, rfl::NoDuplicateKeys>(
          R"({"outer":{"k":1,"k":2}})");
  ASSERT_FALSE(gen_dup_nested);
  EXPECT_NE(gen_dup_nested.error().what().find("Duplicate key 'k'"),
            std::string::npos);

  // rfl::TaggedUnion (duplicate payload key and duplicate discriminator key)
  const auto tu_dup = rfl::json::read<Shape, rfl::NoDuplicateKeys>(
      R"({"kind":"circle","radius":1.0,"radius":2.0})");
  ASSERT_FALSE(tu_dup);
  ASSERT_EQ(tu_dup.error().details().size(), 1u) << tu_dup.error().what();
  EXPECT_EQ(tu_dup.error().details()[0].kind, rfl::ErrorKind::duplicate_field);

  const auto tu_dup_disc = rfl::json::read<Shape, rfl::NoDuplicateKeys>(
      R"({"kind":"circle","kind":"square","radius":1.0})");
  ASSERT_FALSE(tu_dup_disc);
  ASSERT_EQ(tu_dup_disc.error().details().size(), 1u)
      << tu_dup_disc.error().what();
  EXPECT_EQ(tu_dup_disc.error().details()[0].kind,
            rfl::ErrorKind::duplicate_field);
}

}  // namespace test_json_no_duplicate_keys
