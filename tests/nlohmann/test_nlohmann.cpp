#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <rfl/generic/read.hpp>
#include <rfl/generic/write.hpp>
#include <rfl/nlohmann.hpp>

#ifdef REFLECTCPP_HAS_JSON
#include <rfl/json.hpp>
#endif

namespace test_nlohmann {

enum class Color { Red, Green, Blue };
struct Circle { double radius; };
struct Square { double side; };

struct RichStruct {
  std::string first_name;
  rfl::Validator<int, rfl::Minimum<0>> age;
  Color favorite_color;
  std::optional<std::string> nickname;
  std::vector<int> scores;
  std::map<std::string, double> metrics;
  std::variant<Circle, Square> variant_shape;
  rfl::TaggedUnion<"kind", Circle, Square> tagged_shape;
};

struct OrderedFields { int z_field; int a_field; int m_field; };
struct BigInts { uint64_t u64; int64_t i64; uint8_t u8; int8_t i8; };
struct OptInUser { std::string name; int id; };
struct NonOptInUser { std::string name; };

struct WithDynamicFields {
  std::string title;
  nlohmann::json payload;
  nlohmann::ordered_json ordered_payload;
};

}  // namespace test_nlohmann

template <>
struct rfl::nlohmann::enable_adl_serializer<test_nlohmann::OptInUser>
    : std::true_type {};

namespace test_nlohmann {

static std::vector<std::string> object_keys(const nlohmann::ordered_json& j) {
  std::vector<std::string> keys;
  for (auto it = j.begin(); it != j.end(); ++it) keys.push_back(it.key());
  return keys;
}

TEST(nlohmann, bridge1_rich_roundtrip_and_processors) {
  const RichStruct src{
      .first_name = "Ada",
      .age = 36,
      .favorite_color = Color::Green,
      .nickname = "countess",
      .scores = {10, 20, 30},
      .metrics = {{"cpu", 0.5}, {"mem", 1.25}},
      .variant_shape = Circle{.radius = 2.5},
      .tagged_shape = Square{.side = 4.0},
  };
  const nlohmann::json j = rfl::nlohmann::write<rfl::SnakeCaseToCamelCase>(src);
  EXPECT_EQ(j["firstName"], "Ada");
  EXPECT_EQ(j["favoriteColor"], "Green");
  EXPECT_EQ(j["taggedShape"]["kind"], "Square");

  const auto res =
      rfl::nlohmann::read<RichStruct, rfl::SnakeCaseToCamelCase,
                          rfl::NoExtraFields>(j);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->first_name, "Ada");
  EXPECT_EQ(res->age.value(), 36);
  EXPECT_EQ(res->favorite_color, Color::Green);
  EXPECT_EQ(res->nickname, "countess");
  EXPECT_EQ(res->scores, (std::vector<int>{10, 20, 30}));
  EXPECT_DOUBLE_EQ(res->metrics.at("mem"), 1.25);
  EXPECT_DOUBLE_EQ(std::get<Circle>(res->variant_shape).radius, 2.5);
  EXPECT_DOUBLE_EQ(rfl::get<Square>(res->tagged_shape.variant()).side, 4.0);

  nlohmann::json with_extra = j;
  with_extra["unexpectedKey"] = 123;
  EXPECT_FALSE((rfl::nlohmann::read<RichStruct, rfl::SnakeCaseToCamelCase,
                                    rfl::NoExtraFields>(with_extra)
                    .has_value()));
}

TEST(nlohmann, bridge1_ordered_json_preserves_key_order) {
  const OrderedFields src{.z_field = 1, .a_field = 2, .m_field = 3};
  const auto oj = rfl::nlohmann::write<nlohmann::ordered_json>(src);
  EXPECT_EQ(object_keys(oj),
            (std::vector<std::string>{"z_field", "a_field", "m_field"}));
  const auto back = rfl::nlohmann::read<OrderedFields>(oj);
  ASSERT_TRUE(back.has_value());
  EXPECT_EQ(back->z_field, 1);
  EXPECT_EQ(back->a_field, 2);
  EXPECT_EQ(back->m_field, 3);
}

TEST(nlohmann, bridge1_integer_bounds_and_no_exceptions) {
  constexpr uint64_t kHugeU64 =
      static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 12345ULL;
  const BigInts src{kHugeU64, std::numeric_limits<int64_t>::min(), 255, -128};
  const nlohmann::json j = rfl::nlohmann::write(src);
  EXPECT_TRUE(j["u64"].is_number_unsigned());
  EXPECT_EQ(j["u64"].get<uint64_t>(), kHugeU64);

  const auto back = rfl::nlohmann::read<BigInts>(j);
  ASSERT_TRUE(back.has_value());
  EXPECT_EQ(back->u64, kHugeU64);
  EXPECT_EQ(back->i64, std::numeric_limits<int64_t>::min());

  const auto check_errors = [&] {
    auto neg = j; neg["u64"] = -1;
    EXPECT_FALSE(rfl::nlohmann::read<BigInts>(neg).has_value());
    auto big_i64 = j; big_i64["i64"] = kHugeU64;
    EXPECT_FALSE(rfl::nlohmann::read<BigInts>(big_i64).has_value());
    auto u8_over = j; u8_over["u8"] = 256;
    EXPECT_FALSE(rfl::nlohmann::read<BigInts>(u8_over).has_value());
    auto wrong = j; wrong["i64"] = "str";
    EXPECT_FALSE(rfl::nlohmann::read<BigInts>(wrong).has_value());
    auto flt = j; flt["i64"] = 3.14;
    EXPECT_FALSE(rfl::nlohmann::read<BigInts>(flt).has_value());
    EXPECT_FALSE(rfl::nlohmann::read<BigInts>(nlohmann::json::array()).has_value());
    EXPECT_FALSE(rfl::nlohmann::read<BigInts>(nlohmann::json(nullptr)).has_value());
    nlohmann::json bad_age = {{"first_name", "Bob"},
                              {"age", -5},
                              {"favorite_color", "Red"},
                              {"scores", nlohmann::json::array()},
                              {"metrics", nlohmann::json::object()},
                              {"variant_shape", {{"radius", 1.0}}},
                              {"tagged_shape", {{"kind", "Circle"}, {"radius", 1.0}}}};
    EXPECT_FALSE(rfl::nlohmann::read<RichStruct>(bad_age).has_value());
  };
  EXPECT_NO_THROW(check_errors());
}

TEST(nlohmann, bridge1_opt_in_adl_serializer) {
  static_assert(rfl::nlohmann::enable_adl_serializer_v<OptInUser>);
  static_assert(!rfl::nlohmann::enable_adl_serializer_v<NonOptInUser>);
  static_assert(
      !::nlohmann::detail::is_getable<nlohmann::json, NonOptInUser>::value);

  const nlohmann::json j = OptInUser{.name = "Grace", .id = 42};
  EXPECT_EQ(j.get<OptInUser>().name, "Grace");
  EXPECT_EQ(j.get<OptInUser>().id, 42);
  OptInUser target{};
  j.get_to(target);
  EXPECT_EQ(target.id, 42);
  EXPECT_THROW((void)nlohmann::json({{"name", 999}}).get<OptInUser>(),
               std::runtime_error);
  const nlohmann::json stl_j = std::vector<int>{1, 2, 3};
  EXPECT_EQ((stl_j.get<std::vector<int>>()), (std::vector<int>{1, 2, 3}));
}

TEST(nlohmann, bridge2_dynamic_fields_nlohmann_and_generic) {
  constexpr uint64_t kHugeU64 = std::numeric_limits<uint64_t>::max();
  nlohmann::ordered_json ordered = nlohmann::ordered_json::object();
  ordered["z"] = 1;
  ordered["a"] = "two";
  ordered["m"] = nlohmann::ordered_json::array({true, nullptr, -7, 3.5});

  const WithDynamicFields src{
      "doc", {{"big", kHugeU64}, {"arr", {1, 2, "x"}}, {"nil", nullptr}},
      ordered};
  const auto from_nj =
      rfl::nlohmann::read<WithDynamicFields>(rfl::nlohmann::write(src));
  ASSERT_TRUE(from_nj.has_value());
  EXPECT_EQ(from_nj->payload, src.payload);
  EXPECT_EQ(from_nj->payload["big"].get<uint64_t>(), kHugeU64);

  const auto from_noj = rfl::nlohmann::read<WithDynamicFields>(
      rfl::nlohmann::write<nlohmann::ordered_json>(src));
  ASSERT_TRUE(from_noj.has_value());
  EXPECT_EQ(object_keys(from_noj->ordered_payload),
            (std::vector<std::string>{"z", "a", "m"}));

  const WithDynamicFields gen_src{
      "gen", {{"flag", true}, {"num", -42}, {"dbl", 1.5}, {"nil", nullptr}},
      nlohmann::ordered_json::array({10, "hi"})};
  const auto from_g =
      rfl::generic::read<WithDynamicFields>(rfl::generic::write(gen_src));
  ASSERT_TRUE(from_g.has_value());
  EXPECT_EQ(from_g->payload, gen_src.payload);
  EXPECT_EQ(from_g->ordered_payload, gen_src.ordered_payload);
}

#ifdef REFLECTCPP_HAS_JSON
TEST(nlohmann, bridge2_dynamic_fields_rfl_json_and_schema) {
  constexpr uint64_t kHugeU64 = std::numeric_limits<uint64_t>::max();
  nlohmann::ordered_json ordered = nlohmann::ordered_json::object();
  ordered["z"] = kHugeU64;
  ordered["a"] = -99;
  ordered["m"] = nlohmann::ordered_json::array({true, nullptr, 2.25, "str"});

  const WithDynamicFields src{
      "yyjson-fast-path", {{"u64", kHugeU64}, {"nested", {{"k", "v"}}}},
      ordered};
  const auto parsed =
      rfl::json::read<WithDynamicFields>(rfl::json::write(src));
  ASSERT_TRUE(parsed.has_value()) << parsed.error().what();
  EXPECT_EQ(parsed->payload, src.payload);
  EXPECT_TRUE(parsed->payload["u64"].is_number_unsigned());
  EXPECT_EQ(parsed->payload["u64"].get<uint64_t>(), kHugeU64);
  EXPECT_EQ(parsed->ordered_payload, src.ordered_payload);
  EXPECT_EQ(object_keys(parsed->ordered_payload),
            (std::vector<std::string>{"z", "a", "m"}));

  const auto schema_json =
      nlohmann::json::parse(rfl::json::to_schema<WithDynamicFields>());
  ASSERT_TRUE(schema_json.contains("$defs"));
  const auto& def = schema_json["$defs"]["test_nlohmann__WithDynamicFields"];
  EXPECT_TRUE(def["properties"].contains("payload"));
  EXPECT_TRUE(def["properties"].contains("ordered_payload"));
}
#endif

}  // namespace test_nlohmann
