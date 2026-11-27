#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

namespace test_parser_isolation_edge_cases {

struct EmptyStruct {};
struct OneField {
  int a;
};
struct OneByte {
  char a;
};
struct FourBytes {
  char a, b, c, d;
};
struct NonDefaultable {
  int x;
  explicit NonDefaultable(int _x) : x(_x) {}
};
struct MixedAggregate {
  int a;
  NonDefaultable b;
  std::string c;
  bool d;
};
struct WithCArray {
  int a[3];
  int b;
};
struct WithConstRefMember {
  const int& ref;
  int val;
};
struct WideStruct32 {
  int f00, f01, f02, f03, f04, f05, f06, f07;
  int f08, f09, f10, f11, f12, f13, f14, f15;
  int f16, f17, f18, f19, f20, f21, f22, f23;
  int f24, f25, f26, f27, f28, f29, f30, f31;
};

struct SmallWithExtra {
  int a = 1;
  rfl::ExtraFields<int> extra;
};

struct SmallStrict {
  int a = 1;
  int b = 2;
};

struct WideWithExtra {
  int f0 = 0;
  int f1 = 1;
  int f2 = 2;
  int f3 = 3;
  int f4 = 4;
  int f5 = 5;
  int f6 = 6;
  int f7 = 7;
  int f8 = 8;
  int f9 = 9;
  rfl::ExtraFields<int> extra;
};

struct WideStrict {
  int f0 = 0;
  int f1 = 1;
  int f2 = 2;
  int f3 = 3;
  int f4 = 4;
  int f5 = 5;
  int f6 = 6;
  int f7 = 7;
  int f8 = 8;
  int f9 = 9;
};

struct DefaultValEdgeScalars {
  rfl::DefaultVal<uint64_t> big_u64 =
      static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 42ULL;
  rfl::DefaultVal<char> tag_char = 'Z';
  rfl::DefaultVal<bool> flag = true;
  rfl::DefaultVal<float> ratio = 1.5f;
  rfl::DefaultVal<std::string> label = "hello";
  rfl::DefaultVal<std::vector<std::string>> tags =
      std::vector<std::string>{"a", "b"};
  rfl::DefaultVal<std::optional<int>> opt_set = 9;
  rfl::DefaultVal<std::optional<int>> opt_empty = std::nullopt;
  rfl::Rename<"renamedCount", rfl::DefaultVal<int>> renamed_count = 7;
};

struct AddStructNameWithDefault {
  rfl::DefaultVal<int> count = 7;
  std::string name = "ok";
};

TEST(json, test_num_fields_binary_search_edge_cases) {
  static_assert(rfl::num_fields<EmptyStruct> == 0);
  static_assert(rfl::num_fields<OneField> == 1);
  static_assert(rfl::num_fields<OneByte> == 1);
  static_assert(rfl::num_fields<FourBytes> == 4);
  static_assert(rfl::num_fields<MixedAggregate> == 4);
#if defined(REFLECT_CPP_C_ARRAYS_OR_INHERITANCE) || \
    defined(REFLECTCPP_C_ARRAYS_OR_INHERITANCE)
  static_assert(rfl::num_fields<WithCArray> == 2);
#else
  static_assert(rfl::num_fields<WithCArray> == 4);
#endif
  static_assert(rfl::num_fields<WithConstRefMember> == 2);
  static_assert(rfl::num_fields<WideStruct32> == 32);
  SUCCEED();
}

TEST(json, test_view_reader_duplicate_keys_small_and_wide) {
  // Small struct (<= 8 fields, linear scan path)
  const auto dup_strict_small =
      rfl::json::read<SmallStrict, rfl::NoExtraFields>(
          R"({"a":10,"b":20,"a":30})");
  EXPECT_FALSE(dup_strict_small.has_value());

  const auto dup_def_small =
      rfl::json::read<SmallStrict, rfl::DefaultIfMissing, rfl::NoExtraFields>(
          R"({"a":10,"a":30})");
  EXPECT_FALSE(dup_def_small.has_value());

  const auto dup_extra_small =
      rfl::json::read<SmallWithExtra>(R"({"a":10,"a":30})").value();
  EXPECT_EQ(dup_extra_small.a, 10);
  EXPECT_EQ(dup_extra_small.extra.at("a"), 30);

  // Wide struct (> 8 fields, sorted binary-search path)
  const auto dup_strict_wide = rfl::json::read<WideStrict, rfl::NoExtraFields>(
      R"({"f0":0,"f1":1,"f2":2,"f3":3,"f4":4,"f5":5,"f6":6,"f7":7,"f8":8,"f9":9,"f5":55})");
  EXPECT_FALSE(dup_strict_wide.has_value());

  const auto dup_def_wide =
      rfl::json::read<WideStrict, rfl::DefaultIfMissing, rfl::NoExtraFields>(
          R"({"f5":5,"f5":55})");
  EXPECT_FALSE(dup_def_wide.has_value());

  const auto dup_extra_wide =
      rfl::json::read<WideWithExtra, rfl::DefaultIfMissing>(
          R"({"f5":5,"f5":55,"z":99})")
          .value();
  EXPECT_EQ(dup_extra_wide.f5, 5);
  EXPECT_EQ(dup_extra_wide.extra.at("f5"), 55);
  EXPECT_EQ(dup_extra_wide.extra.at("z"), 99);
}

TEST(json, test_to_default_generic_edge_values_and_add_struct_name) {
  const auto schema = rfl::json::to_schema<DefaultValEdgeScalars>();
  EXPECT_NE(schema.find("\"default\":90"), std::string::npos);
  EXPECT_NE(schema.find("\"default\":true"), std::string::npos);
  EXPECT_NE(schema.find("\"default\":1.5"), std::string::npos);
  EXPECT_NE(schema.find("\"default\":\"hello\""), std::string::npos);
  EXPECT_NE(schema.find("\"default\":[\"a\",\"b\"]"), std::string::npos);
  EXPECT_NE(schema.find("\"default\":9"), std::string::npos);
  EXPECT_NE(schema.find("\"default\":null"), std::string::npos);
  EXPECT_NE(schema.find("\"renamedCount\":{\"type\":\"integer\",\"default\":7}"),
            std::string::npos);
  // uint64_t > INT64_MAX must match rfl::to_generic fallback:
  const auto expected_u64 = rfl::json::write(rfl::to_generic(
      static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 42ULL));
  EXPECT_NE(schema.find("\"default\":" + expected_u64), std::string::npos);

  const auto asn_schema =
      rfl::json::to_schema<AddStructNameWithDefault,
                           rfl::AddStructName<"type">>();
  EXPECT_NE(
      asn_schema.find(
          "\"type\":{\"type\":\"string\",\"enum\":[\"AddStructNameWithDefault\"]}"),
      std::string::npos);
  EXPECT_NE(asn_schema.find("\"default\":7"), std::string::npos);

  const auto parsed =
      rfl::json::read<AddStructNameWithDefault, rfl::AddStructName<"type">>(
          R"({"type":"AddStructNameWithDefault","name":"hi"})")
          .value();
  EXPECT_EQ(parsed.count.get(), 7);
  EXPECT_EQ(parsed.name, "hi");
}

}  // namespace test_parser_isolation_edge_cases
