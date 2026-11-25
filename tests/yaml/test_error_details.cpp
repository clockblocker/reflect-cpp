#include <gtest/gtest.h>

#include <array>
#include <bitset>
#include <map>
#include <optional>
#include <rfl.hpp>
#include <rfl/yaml.hpp>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace test_yaml_error_details {

using Path = std::vector<rfl::ErrorDetail::PathSegment>;

struct Item {
  std::string name;
  rfl::Validator<int, rfl::Minimum<1>> code;
};

struct Config {
  std::string title;
  std::vector<Item> items;
};

enum class Mode { fast, safe };

struct Settings {
  int port;
  Mode mode;
  std::bitset<4> mask;
};

struct Circle {
  rfl::Validator<int, rfl::Minimum<1>> radius;
};

struct Square {
  double width;
};

using Shape = rfl::TaggedUnion<"shape", Circle, Square>;

struct Drawing {
  Shape missing_tag;
  Shape bad_tag_type;
  Shape unknown_tag;
  Shape bad_field;
  Shape missing_field;
  std::variant<int, bool> id;
};

TEST(yaml, test_error_details_source_locations_value_key_parent) {
  const std::string yaml_str =
      "title: ok\n"
      "items:\n"
      "  - name: first\n"
      "    code: 0\n"
      "    unknown_field: 42\n";

  const auto result = rfl::yaml::read<Config, rfl::NoExtraFields>(yaml_str);
  ASSERT_FALSE(result);

  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 2) << result.error().what();

  // Validator error points directly at the bad value (line 4, col 11: "0").
  EXPECT_EQ(details[0].path, (Path{"items", size_t{0}, "code"}));
  EXPECT_EQ(details[0].message,
            "Value expected to be greater than or equal to 1, but got 0.");
  EXPECT_EQ(details[0].location,
            (rfl::SourceLocation{.line = 4, .column = 11, .exact = true}));

  // Unknown field points directly at the key (line 5, col 5: "unknown_field"),
  // not at its value (col 20: "42").
  EXPECT_EQ(details[1].path, (Path{"items", size_t{0}, "unknown_field"}));
  EXPECT_EQ(details[1].message, "Value named 'unknown_field' not used.");
  EXPECT_EQ(details[1].location,
            (rfl::SourceLocation{.line = 5, .column = 5, .exact = true}));

  // Missing field points at its parent mapping (line 3, col 5) with exact=false.
  const std::string missing_yaml =
      "title: ok\n"
      "items:\n"
      "  - name: only_name\n";
  const auto missing_res = rfl::yaml::read<Config>(missing_yaml);
  ASSERT_FALSE(missing_res);
  ASSERT_EQ(missing_res.error().details().size(), 1);
  EXPECT_EQ(missing_res.error().details()[0].path,
            (Path{"items", size_t{0}, "code"}));
  EXPECT_EQ(missing_res.error().details()[0].message,
            "Field named 'code' not found.");
  EXPECT_EQ(missing_res.error().details()[0].location,
            (rfl::SourceLocation{.line = 3, .column = 5, .exact = false}));
}

TEST(yaml, test_error_details_type_conversion_and_enum_locations) {
  const std::string yaml_str =
      "port: not_a_number\n"
      "mode: turbo\n"
      "mask: 1020\n";

  const auto result = rfl::yaml::read<Settings>(yaml_str);
  ASSERT_FALSE(result);

  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 3) << result.error().what();

  EXPECT_EQ(details[0].path, (Path{"port"}));
  EXPECT_EQ(details[0].location,
            (rfl::SourceLocation{.line = 1, .column = 7, .exact = true}));

  EXPECT_EQ(details[1].path, (Path{"mode"}));
  EXPECT_EQ(details[1].location,
            (rfl::SourceLocation{.line = 2, .column = 7, .exact = true}));

  EXPECT_EQ(details[2].path, (Path{"mask"}));
  EXPECT_EQ(details[2].location,
            (rfl::SourceLocation{.line = 3, .column = 7, .exact = true}));
}

TEST(yaml, test_error_details_syntax_error_location) {
  const std::string yaml_str =
      "title: ok\n"
      "items: [1, 2\n";

  const auto result = rfl::yaml::read<Config>(yaml_str);
  ASSERT_FALSE(result);

  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 1);
  EXPECT_TRUE(details[0].path.empty());
  EXPECT_EQ(details[0].message, result.error().what());
  ASSERT_TRUE(details[0].location.has_value());
  EXPECT_TRUE(details[0].location->exact);
  EXPECT_GE(details[0].location->line, 2u);
}

TEST(yaml, test_error_details_map_value_and_key_locations) {
  const std::string yaml_str =
      "1: bad_val\n"
      "bad_key: 2\n";

  const auto result = rfl::yaml::read<std::map<int, int>>(yaml_str);
  ASSERT_FALSE(result);

  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 2) << result.error().what();

  // Map value error points at the value.
  EXPECT_EQ(details[0].path, (Path{"1"}));
  EXPECT_EQ(details[0].location,
            (rfl::SourceLocation{.line = 1, .column = 4, .exact = true}));

  // Map key error points at the key.
  EXPECT_EQ(details[1].path, (Path{"bad_key"}));
  EXPECT_EQ(details[1].location,
            (rfl::SourceLocation{.line = 2, .column = 1, .exact = true}));
}

TEST(yaml, test_error_details_tagged_union_and_variant) {
  const std::string yaml_str =
      "missing_tag:\n"
      "  width: 2.0\n"
      "bad_tag_type:\n"
      "  shape: [1, 2]\n"
      "unknown_tag:\n"
      "  shape: Triangle\n"
      "bad_field:\n"
      "  shape: Circle\n"
      "  radius: -5\n"
      "missing_field:\n"
      "  shape: Square\n"
      "id: not_int_or_bool\n";

  const auto result = rfl::yaml::read<Drawing>(yaml_str);
  ASSERT_FALSE(result);

  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 6) << result.error().what();

  // Missing discriminator points at parent mapping with exact=false.
  EXPECT_EQ(details[0].path, (Path{"missing_tag"}));
  EXPECT_EQ(details[0].location,
            (rfl::SourceLocation{.line = 2, .column = 3, .exact = false}));

  // Non-string discriminator points at the discriminator value with exact=true.
  EXPECT_EQ(details[1].path, (Path{"bad_tag_type"}));
  EXPECT_EQ(details[1].location,
            (rfl::SourceLocation{.line = 4, .column = 10, .exact = true}));

  // Unknown discriminator points at the tag value with exact=true.
  EXPECT_EQ(details[2].path, (Path{"unknown_tag"}));
  EXPECT_EQ(details[2].location,
            (rfl::SourceLocation{.line = 6, .column = 10, .exact = true}));

  // Bad field inside matched alternative keeps the leaf value location.
  EXPECT_EQ(details[3].path, (Path{"bad_field", "radius"}));
  EXPECT_EQ(details[3].location,
            (rfl::SourceLocation{.line = 9, .column = 11, .exact = true}));

  // Missing field inside matched alternative points at parent with exact=false.
  EXPECT_EQ(details[4].path, (Path{"missing_field", "width"}));
  EXPECT_EQ(details[4].location,
            (rfl::SourceLocation{.line = 11, .column = 3, .exact = false}));

  // Untagged variant points at the variant value with exact=true.
  EXPECT_EQ(details[5].path, (Path{"id"}));
  EXPECT_EQ(details[5].location,
            (rfl::SourceLocation{.line = 12, .column = 5, .exact = true}));
}

TEST(yaml, test_error_details_field_variant_and_array_tuple_bounds) {
  using FieldVar = rfl::Variant<rfl::Field<"circle", Circle>,
                                rfl::Field<"square", Square>>;

  // Nested error inside externally tagged variant keeps leaf location.
  const auto fv_nested =
      rfl::yaml::read<FieldVar>(std::string("circle:\n  radius: 0\n"));
  ASSERT_FALSE(fv_nested);
  ASSERT_EQ(fv_nested.error().details().size(), 1);
  EXPECT_EQ(fv_nested.error().details()[0].path, (Path{"circle", "radius"}));
  EXPECT_EQ(fv_nested.error().details()[0].location,
            (rfl::SourceLocation{.line = 2, .column = 11, .exact = true}));

  // Unknown field in externally tagged variant points at key.
  const auto fv_unknown =
      rfl::yaml::read<FieldVar>(std::string("triangle:\n  sides: 3\n"));
  ASSERT_FALSE(fv_unknown);
  ASSERT_EQ(fv_unknown.error().details().size(), 1);
  EXPECT_EQ(fv_unknown.error().details()[0].location,
            (rfl::SourceLocation{.line = 1, .column = 1, .exact = true}));

  // Too few elements in std::array points at sequence with exact=false.
  const auto arr_short =
      rfl::yaml::read<std::array<int, 3>>(std::string("- 1\n- 2\n"));
  ASSERT_FALSE(arr_short);
  ASSERT_EQ(arr_short.error().details().size(), 1);
  EXPECT_EQ(arr_short.error().details()[0].location,
            (rfl::SourceLocation{.line = 1, .column = 1, .exact = false}));

  // Too many elements in std::array points at the extra element with exact=true.
  const auto arr_long =
      rfl::yaml::read<std::array<int, 2>>(std::string("- 1\n- 2\n- 3\n"));
  ASSERT_FALSE(arr_long);
  ASSERT_EQ(arr_long.error().details().size(), 1);
  EXPECT_EQ(arr_long.error().details()[0].location,
            (rfl::SourceLocation{.line = 3, .column = 3, .exact = true}));

  // Missing element in std::tuple points at sequence with exact=false.
  const auto tup_short =
      rfl::yaml::read<std::tuple<int, std::string>>(std::string("- 1\n"));
  ASSERT_FALSE(tup_short);
  ASSERT_EQ(tup_short.error().details().size(), 1);
  EXPECT_EQ(tup_short.error().details()[0].path, (Path{size_t{1}}));
  EXPECT_EQ(tup_short.error().details()[0].location,
            (rfl::SourceLocation{.line = 1, .column = 1, .exact = false}));
}

TEST(yaml, test_what_unchanged_for_single_and_multi_errors) {
  const std::string single_err =
      "name: ok\n"
      "code: 0\n";
  const auto res = rfl::yaml::read<Item>(single_err);
  ASSERT_FALSE(res);
  EXPECT_EQ(
      res.error().what(),
      "Failed to parse field 'code': Value expected to be greater than or "
      "equal to 1, but got 0.");
}

}  // namespace test_yaml_error_details
