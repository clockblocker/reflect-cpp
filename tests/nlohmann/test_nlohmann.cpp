#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
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

struct StrictOptInUser {
  const std::string name;
  rfl::Validator<int, rfl::Minimum<0>> age;
};

struct BinaryFields {
  rfl::Bytestring bytes;
  rfl::Vectorstring vec;
};

struct WithDynamicFields {
  std::string title;
  nlohmann::json payload;
  nlohmann::ordered_json ordered_payload;
};

}  // namespace test_nlohmann

template <>
struct rfl::nlohmann::enable_adl_serializer<test_nlohmann::OptInUser>
    : std::true_type {};

template <>
struct rfl::nlohmann::enable_adl_serializer<test_nlohmann::StrictOptInUser>
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

TEST(nlohmann, bridge1_bytestring_vectorstring_and_binary_t) {
  const BinaryFields src{
      .bytes = {std::byte{0x00}, std::byte{0x41}, std::byte{0xFF}},
      .vec = {'h', 'i', '\0', static_cast<char>(0xFE)},
  };
  const nlohmann::json j = rfl::nlohmann::write(src);
  ASSERT_TRUE(j["bytes"].is_string());
  ASSERT_TRUE(j["vec"].is_string());

  const auto back = rfl::nlohmann::read<BinaryFields>(j);
  ASSERT_TRUE(back.has_value()) << back.error().what();
  EXPECT_EQ(back->bytes, src.bytes);
  EXPECT_EQ(back->vec, src.vec);

  // Direct nlohmann::json::binary_t -> Bytestring & Vectorstring on read.
  nlohmann::json raw_bin = {
      {"bytes", nlohmann::json::binary({0xDE, 0xAD, 0xBE, 0xEF})},
      {"vec", nlohmann::json::binary({'a', 'b', 'c'})},
  };
  const auto from_bin = rfl::nlohmann::read<BinaryFields>(raw_bin);
  ASSERT_TRUE(from_bin.has_value()) << from_bin.error().what();
  EXPECT_EQ(from_bin->bytes,
            (rfl::Bytestring{std::byte{0xDE}, std::byte{0xAD},
                             std::byte{0xBE}, std::byte{0xEF}}));
  EXPECT_EQ(from_bin->vec, (rfl::Vectorstring{'a', 'b', 'c'}));

  // Invalid type for Vectorstring returns error without throwing.
  nlohmann::json bad_vec = {{"bytes", "AA=="}, {"vec", 12345}};
  EXPECT_NO_THROW(
      EXPECT_FALSE(rfl::nlohmann::read<BinaryFields>(bad_vec).has_value()));
}

TEST(nlohmann, bridge1_opt_in_adl_serializer) {
  static_assert(rfl::nlohmann::enable_adl_serializer_v<OptInUser>);
  static_assert(rfl::nlohmann::enable_adl_serializer_v<StrictOptInUser>);
  static_assert(!rfl::nlohmann::enable_adl_serializer_v<NonOptInUser>);
  static_assert(
      !::nlohmann::detail::is_getable<nlohmann::json, NonOptInUser>::value);

  const nlohmann::json j = OptInUser{.name = "Grace", .id = 42};
  EXPECT_EQ(j.get<OptInUser>().name, "Grace");
  EXPECT_EQ(j.get<OptInUser>().id, 42);
  EXPECT_THROW((void)nlohmann::json({{"name", 999}}).get<OptInUser>(),
               nlohmann::json::exception);
  EXPECT_THROW((void)nlohmann::json({{"name", 999}}).get<OptInUser>(),
               nlohmann::json::other_error);

  // Struct with const field + rfl::Validator (non-default-constructible &
  // non-assignable) works via returning 1-arg from_json.
  const nlohmann::json sj = StrictOptInUser{.name = "Linus", .age = 30};
  const auto parsed = sj.get<StrictOptInUser>();
  EXPECT_EQ(parsed.name, "Linus");
  EXPECT_EQ(parsed.age.value(), 30);
  EXPECT_THROW(
      (void)nlohmann::json({{"name", "Linus"}, {"age", -1}})
          .get<StrictOptInUser>(),
      nlohmann::json::exception);

  const nlohmann::json stl_j = std::vector<int>{1, 2, 3};
  EXPECT_EQ((stl_j.get<std::vector<int>>()), (std::vector<int>{1, 2, 3}));
}

TEST(nlohmann, bridge2_cross_basic_json_and_binary_t) {
  constexpr uint64_t kHugeU64 = std::numeric_limits<uint64_t>::max();
  nlohmann::ordered_json ordered = nlohmann::ordered_json::object();
  ordered["z"] = 1ULL;
  ordered["big"] = kHugeU64;
  ordered["a"] = "two";
  ordered["bin"] = nlohmann::ordered_json::binary({0x01, 0x02, 0x03}, 42);
  ordered["m"] = nlohmann::ordered_json::array({true, nullptr, -7, 3.5});

  const WithDynamicFields src{
      "doc",
      {{"big", kHugeU64},
       {"small_u", 7ULL},
       {"bin", nlohmann::json::binary({0xAB, 0xCD}, 9)},
       {"arr", {1, 2, "x"}},
       {"nil", nullptr}},
      ordered};

  // Cross-BasicJson: nlohmann::ordered_json field written to & read from
  // nlohmann::json preserves exact unsignedness and binary_t (with subtype).
  const nlohmann::json as_json = rfl::nlohmann::write(src);
  ASSERT_TRUE(as_json["ordered_payload"]["z"].is_number_unsigned());
  ASSERT_TRUE(as_json["ordered_payload"]["big"].is_number_unsigned());
  ASSERT_TRUE(as_json["ordered_payload"]["bin"].is_binary());
  EXPECT_EQ(as_json["ordered_payload"]["bin"].get_binary().subtype(), 42U);

  const auto from_nj = rfl::nlohmann::read<WithDynamicFields>(as_json);
  ASSERT_TRUE(from_nj.has_value()) << from_nj.error().what();
  EXPECT_EQ(from_nj->payload, src.payload);
  EXPECT_TRUE(from_nj->payload["big"].is_number_unsigned());
  EXPECT_EQ(from_nj->payload["big"].get<uint64_t>(), kHugeU64);
  EXPECT_TRUE(from_nj->ordered_payload["z"].is_number_unsigned());
  EXPECT_TRUE(from_nj->ordered_payload["big"].is_number_unsigned());
  EXPECT_EQ(from_nj->ordered_payload["big"].get<uint64_t>(), kHugeU64);
  ASSERT_TRUE(from_nj->ordered_payload["bin"].is_binary());
  EXPECT_EQ(from_nj->ordered_payload["bin"].get_binary(),
            ordered["bin"].get_binary());
  EXPECT_EQ(from_nj->ordered_payload["bin"].get_binary().subtype(), 42U);

  // Cross-BasicJson in reverse: nlohmann::json field written to & read from
  // nlohmann::ordered_json preserves key order, unsignedness, and binary_t.
  const auto as_ordered = rfl::nlohmann::write<nlohmann::ordered_json>(src);
  ASSERT_TRUE(as_ordered["payload"]["small_u"].is_number_unsigned());
  ASSERT_TRUE(as_ordered["payload"]["bin"].is_binary());
  EXPECT_EQ(as_ordered["payload"]["bin"].get_binary().subtype(), 9U);

  const auto from_noj = rfl::nlohmann::read<WithDynamicFields>(as_ordered);
  ASSERT_TRUE(from_noj.has_value()) << from_noj.error().what();
  EXPECT_EQ(object_keys(from_noj->ordered_payload),
            (std::vector<std::string>{"z", "big", "a", "bin", "m"}));
  EXPECT_TRUE(from_noj->payload["small_u"].is_number_unsigned());
  ASSERT_TRUE(from_noj->payload["bin"].is_binary());
  EXPECT_EQ(from_noj->payload["bin"].get_binary().subtype(), 9U);

  // Generic writer encodes binary_t as Base64 string instead of null.
  const WithDynamicFields gen_src{
      "gen",
      {{"flag", true},
       {"num", -42},
       {"dbl", 1.5},
       {"nil", nullptr},
       {"bin", nlohmann::json::binary({'A', 'B', 'C'})}},
      nlohmann::ordered_json::array({10, "hi"})};
  const auto from_g =
      rfl::generic::read<WithDynamicFields>(rfl::generic::write(gen_src));
  ASSERT_TRUE(from_g.has_value()) << from_g.error().what();
  EXPECT_EQ(from_g->payload["flag"], true);
  EXPECT_EQ(from_g->payload["num"], -42);
  EXPECT_DOUBLE_EQ(from_g->payload["dbl"].get<double>(), 1.5);
  EXPECT_TRUE(from_g->payload["nil"].is_null());
  ASSERT_TRUE(from_g->payload["bin"].is_string());
  EXPECT_EQ(from_g->payload["bin"], "QUJD");
  EXPECT_EQ(from_g->ordered_payload, gen_src.ordered_payload);
}

struct MockYamlNode {
  std::string tag_;
  std::string raw_;
  const std::string& Tag() const { return tag_; }
};

struct MockYamlVar {
  MockYamlNode node_;
};

struct MockYamlReader {
  using InputArrayType = int;
  using InputObjectType = int;
  using InputVarType = MockYamlVar;
  bool is_empty(const InputVarType&) const { return false; }
  rfl::Result<InputObjectType> to_object(const InputVarType&) const {
    return rfl::error("not object");
  }
  rfl::Result<InputArrayType> to_array(const InputVarType&) const {
    return rfl::error("not array");
  }
  template <class ArrayReader>
  std::optional<rfl::Error> read_array(const ArrayReader&,
                                       const InputArrayType&) const {
    return std::nullopt;
  }
  template <class ObjectReader>
  std::optional<rfl::Error> read_object(const ObjectReader&,
                                        const InputObjectType&) const {
    return std::nullopt;
  }
  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _v) const {
    if constexpr (std::is_same_v<T, std::string>) {
      return _v.node_.raw_;
    } else if constexpr (std::is_same_v<T, bool>) {
      if (_v.node_.raw_ == "true") return true;
      return rfl::error("not bool");
    } else if constexpr (std::is_integral_v<T>) {
      if (_v.node_.raw_ == "42") return static_cast<T>(42);
      return rfl::error("not int");
    } else {
      return rfl::error("unsupported");
    }
  }
};

TEST(nlohmann, bridge2_yaml_quoted_tag_not_coerced) {
  // Minimal mock matching yaml-cpp InputVarType's node_.Tag() == "!" contract
  // to verify read_from_generic_reader does not coerce explicitly quoted
  // YAML strings like "true" or "42" into bools/ints.
  const MockYamlReader r;
  const auto quoted_bool =
      rfl::nlohmann::internal::read_from_generic_reader<nlohmann::json>(
          r, MockYamlVar{MockYamlNode{"!", "true"}});
  ASSERT_TRUE(quoted_bool.has_value());
  EXPECT_TRUE(quoted_bool->is_string());
  EXPECT_EQ(*quoted_bool, "true");

  const auto quoted_int =
      rfl::nlohmann::internal::read_from_generic_reader<nlohmann::json>(
          r, MockYamlVar{MockYamlNode{"!", "42"}});
  ASSERT_TRUE(quoted_int.has_value());
  EXPECT_TRUE(quoted_int->is_string());
  EXPECT_EQ(*quoted_int, "42");

  const auto unquoted_bool =
      rfl::nlohmann::internal::read_from_generic_reader<nlohmann::json>(
          r, MockYamlVar{MockYamlNode{"?", "true"}});
  ASSERT_TRUE(unquoted_bool.has_value());
  EXPECT_TRUE(unquoted_bool->is_boolean());
  EXPECT_EQ(*unquoted_bool, true);
}

struct MockMsgpackObject {
  enum Type {
    NIL = 0x00,
    BOOLEAN = 0x01,
    POSITIVE_INTEGER = 0x02,
    NEGATIVE_INTEGER = 0x03,
  } type = NIL;
  union {
    bool boolean;
    uint64_t u64;
    int64_t i64;
  } via{};
};

struct MockMsgpackReader {
  using InputArrayType = int;
  using InputObjectType = int;
  using InputVarType = MockMsgpackObject;
  bool is_empty(const InputVarType& _v) const {
    return _v.type == MockMsgpackObject::NIL;
  }
  rfl::Result<InputObjectType> to_object(const InputVarType&) const {
    return rfl::error("not object");
  }
  rfl::Result<InputArrayType> to_array(const InputVarType&) const {
    return rfl::error("not array");
  }
  template <class ArrayReader>
  std::optional<rfl::Error> read_array(const ArrayReader&,
                                       const InputArrayType&) const {
    return std::nullopt;
  }
  template <class ObjectReader>
  std::optional<rfl::Error> read_object(const ObjectReader&,
                                        const InputObjectType&) const {
    return std::nullopt;
  }
  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _v) const {
    if constexpr (std::is_same_v<T, bool>) {
      if (_v.type == MockMsgpackObject::BOOLEAN) return _v.via.boolean;
      return rfl::error("not bool");
    } else if constexpr (std::is_integral_v<T>) {
      if (_v.type == MockMsgpackObject::POSITIVE_INTEGER) {
        return static_cast<T>(_v.via.u64);
      }
      if (_v.type == MockMsgpackObject::NEGATIVE_INTEGER) {
        return static_cast<T>(_v.via.i64);
      }
      return rfl::error("not int");
    } else {
      return rfl::error("unsupported");
    }
  }
};

struct MockFlexbufVar {
  bool is_uint_ = false;
  uint64_t u64_ = 0;
  int64_t i64_ = 0;
  bool IsUInt() const { return is_uint_; }
  uint64_t AsUInt64() const { return u64_; }
};

struct MockFlexbufReader {
  using InputArrayType = int;
  using InputObjectType = int;
  using InputVarType = MockFlexbufVar;
  bool is_empty(const InputVarType&) const { return false; }
  rfl::Result<InputObjectType> to_object(const InputVarType&) const {
    return rfl::error("not object");
  }
  rfl::Result<InputArrayType> to_array(const InputVarType&) const {
    return rfl::error("not array");
  }
  template <class ArrayReader>
  std::optional<rfl::Error> read_array(const ArrayReader&,
                                       const InputArrayType&) const {
    return std::nullopt;
  }
  template <class ObjectReader>
  std::optional<rfl::Error> read_object(const ObjectReader&,
                                        const InputObjectType&) const {
    return std::nullopt;
  }
  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _v) const {
    if constexpr (std::is_same_v<T, bool>) {
      return rfl::error("not bool");
    } else if constexpr (std::is_integral_v<T>) {
      // Mirrors flexbuf::Reader casting via AsInt64().
      const int64_t as_i64 =
          _v.is_uint_ ? static_cast<int64_t>(_v.u64_) : _v.i64_;
      return static_cast<T>(as_i64);
    } else {
      return rfl::error("unsupported");
    }
  }
};

TEST(nlohmann, bridge2_msgpack_and_flexbuf_uint64_max_preserved) {
  constexpr uint64_t kMaxU64 = std::numeric_limits<uint64_t>::max();
  constexpr int64_t kMinI64 = std::numeric_limits<int64_t>::min();

  const MockMsgpackReader mp_reader;
  MockMsgpackObject mp_max;
  mp_max.type = MockMsgpackObject::POSITIVE_INTEGER;
  mp_max.via.u64 = kMaxU64;
  const auto mp_u =
      rfl::nlohmann::internal::read_from_generic_reader<nlohmann::json>(
          mp_reader, mp_max);
  ASSERT_TRUE(mp_u.has_value());
  EXPECT_TRUE(mp_u->is_number_unsigned());
  EXPECT_EQ(mp_u->get<uint64_t>(), kMaxU64);

  MockMsgpackObject mp_neg;
  mp_neg.type = MockMsgpackObject::NEGATIVE_INTEGER;
  mp_neg.via.i64 = kMinI64;
  const auto mp_i =
      rfl::nlohmann::internal::read_from_generic_reader<nlohmann::json>(
          mp_reader, mp_neg);
  ASSERT_TRUE(mp_i.has_value());
  EXPECT_TRUE(mp_i->is_number_integer());
  EXPECT_FALSE(mp_i->is_number_unsigned());
  EXPECT_EQ(mp_i->get<int64_t>(), kMinI64);

  const MockFlexbufReader fb_reader;
  const auto fb_u =
      rfl::nlohmann::internal::read_from_generic_reader<nlohmann::json>(
          fb_reader, MockFlexbufVar{.is_uint_ = true, .u64_ = kMaxU64});
  ASSERT_TRUE(fb_u.has_value());
  EXPECT_TRUE(fb_u->is_number_unsigned());
  EXPECT_EQ(fb_u->get<uint64_t>(), kMaxU64);

  const auto fb_i =
      rfl::nlohmann::internal::read_from_generic_reader<nlohmann::json>(
          fb_reader, MockFlexbufVar{.is_uint_ = false, .i64_ = kMinI64});
  ASSERT_TRUE(fb_i.has_value());
  EXPECT_TRUE(fb_i->is_number_integer());
  EXPECT_FALSE(fb_i->is_number_unsigned());
  EXPECT_EQ(fb_i->get<int64_t>(), kMinI64);
}

struct MockBinaryWriter {
  struct OutputArrayType {};
  struct OutputObjectType {};
  struct OutputVarType {};

  mutable bool saw_bytestring = false;
  mutable rfl::Bytestring captured_bytes;

  OutputArrayType array_as_root(const size_t) const { return {}; }
  OutputObjectType object_as_root(const size_t) const { return {}; }
  OutputVarType null_as_root() const { return {}; }
  template <class T>
  OutputVarType value_as_root(const T& _v) const {
    if constexpr (std::is_same_v<std::remove_cvref_t<T>, rfl::Bytestring>) {
      saw_bytestring = true;
      captured_bytes = _v;
    }
    return {};
  }
  OutputArrayType add_array_to_array(const size_t, OutputArrayType*) const {
    return {};
  }
  OutputArrayType add_array_to_object(const std::string_view&, const size_t,
                                      OutputObjectType*) const {
    return {};
  }
  OutputObjectType add_object_to_array(const size_t, OutputArrayType*) const {
    return {};
  }
  OutputObjectType add_object_to_object(const std::string_view&, const size_t,
                                        OutputObjectType*) const {
    return {};
  }
  template <class T>
  OutputVarType add_value_to_array(const T& _v, OutputArrayType*) const {
    return value_as_root(_v);
  }
  template <class T>
  OutputVarType add_value_to_object(const std::string_view&, const T& _v,
                                    OutputObjectType*) const {
    return value_as_root(_v);
  }
  OutputVarType add_null_to_array(OutputArrayType*) const { return {}; }
  OutputVarType add_null_to_object(const std::string_view&,
                                   OutputObjectType*) const {
    return {};
  }
  void end_array(OutputArrayType*) const noexcept {}
  void end_object(OutputObjectType*) const noexcept {}
};

TEST(nlohmann, bridge2_binary_t_passes_bytestring_to_binary_writers) {
  const auto bin = nlohmann::json::binary({0xDE, 0xAD, 0xBE, 0xEF});
  const MockBinaryWriter w;
  rfl::nlohmann::internal::write_to_writer(
      w, bin, typename rfl::parsing::Parent<MockBinaryWriter>::Root{});
  EXPECT_TRUE(w.saw_bytestring);
  EXPECT_EQ(w.captured_bytes,
            (rfl::Bytestring{std::byte{0xDE}, std::byte{0xAD},
                             std::byte{0xBE}, std::byte{0xEF}}));
}

#ifdef REFLECTCPP_HAS_JSON
TEST(nlohmann, bridge2_dynamic_fields_rfl_json_and_schema) {
  constexpr uint64_t kHugeU64 = std::numeric_limits<uint64_t>::max();
  nlohmann::ordered_json ordered = nlohmann::ordered_json::object();
  ordered["z"] = kHugeU64;
  ordered["a"] = -99;
  ordered["m"] = nlohmann::ordered_json::array({true, nullptr, 2.25, "str"});

  const WithDynamicFields src{
      "yyjson-fast-path",
      {{"u64", kHugeU64},
       {"nested", {{"k", "v"}}},
       {"bin", nlohmann::json::binary({'A', 'B', 'C'})}},
      ordered};
  const auto parsed =
      rfl::json::read<WithDynamicFields>(rfl::json::write(src));
  ASSERT_TRUE(parsed.has_value()) << parsed.error().what();
  EXPECT_TRUE(parsed->payload["u64"].is_number_unsigned());
  EXPECT_EQ(parsed->payload["u64"].get<uint64_t>(), kHugeU64);
  EXPECT_EQ(parsed->payload["nested"], src.payload["nested"]);
  EXPECT_EQ(parsed->payload["bin"], "QUJD");
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

struct ErrorKindParityStruct {
  std::string str_val;
  bool bool_val;
  double dbl_val;
  int8_t i8_val;
  int64_t i64_val;
  uint8_t u8_val;
  uint64_t u64_val;
  std::vector<int> arr_val;
  rfl::Bytestring bytes_val;
  rfl::Validator<int, rfl::Minimum<0>> age;
  rfl::TaggedUnion<"kind", Circle, Square> shape;
};

TEST(nlohmann, error_kind_parity_with_json) {
  const nlohmann::json valid = {
      {"str_val", "ok"},
      {"bool_val", true},
      {"dbl_val", 1.5},
      {"i8_val", 10},
      {"i64_val", -100},
      {"u8_val", 42},
      {"u64_val", 1000ULL},
      {"arr_val", {1, 2}},
      {"bytes_val", "AA=="},
      {"age", 5},
      {"shape", {{"kind", "Circle"}, {"radius", 1.0}}},
  };
  ASSERT_TRUE(rfl::nlohmann::read<ErrorKindParityStruct>(valid).has_value());
  ASSERT_TRUE(
      rfl::json::read<ErrorKindParityStruct>(valid.dump()).has_value());

  const auto expect_parity = [](const nlohmann::json& _j,
                                const rfl::ErrorKind _expected) {
    const auto nj_res =
        rfl::nlohmann::read<ErrorKindParityStruct, rfl::NoExtraFields>(_j);
    const auto yy_res =
        rfl::json::read<ErrorKindParityStruct, rfl::NoExtraFields>(_j.dump());
    ASSERT_FALSE(nj_res.has_value()) << _j.dump();
    ASSERT_FALSE(yy_res.has_value()) << _j.dump();
    EXPECT_EQ(nj_res.error().kind(), _expected)
        << "nlohmann mismatch for: " << _j.dump() << " -> "
        << nj_res.error().what();
    EXPECT_EQ(yy_res.error().kind(), _expected)
        << "json mismatch for: " << _j.dump() << " -> "
        << yy_res.error().what();
    EXPECT_EQ(nj_res.error().kind(), yy_res.error().kind());
  };

  // Primitive type mismatches -> invalid_type
  { auto j = valid; j["str_val"] = 123; expect_parity(j, rfl::ErrorKind::invalid_type); }
  { auto j = valid; j["bool_val"] = "true"; expect_parity(j, rfl::ErrorKind::invalid_type); }
  { auto j = valid; j["dbl_val"] = "not-a-num"; expect_parity(j, rfl::ErrorKind::invalid_type); }
  { auto j = valid; j["i8_val"] = "not-an-int"; expect_parity(j, rfl::ErrorKind::invalid_type); }
  { auto j = valid; j["i64_val"] = 3.14; expect_parity(j, rfl::ErrorKind::invalid_type); }
  { auto j = valid; j["u8_val"] = false; expect_parity(j, rfl::ErrorKind::invalid_type); }
  { auto j = valid; j["u64_val"] = "nope"; expect_parity(j, rfl::ErrorKind::invalid_type); }
  { auto j = valid; j["arr_val"] = "not-arr"; expect_parity(j, rfl::ErrorKind::invalid_type); }
  { auto j = valid; j["bytes_val"] = 42; expect_parity(j, rfl::ErrorKind::invalid_type); }
  expect_parity(nlohmann::json::array(), rfl::ErrorKind::invalid_type);
  expect_parity(nlohmann::json(nullptr), rfl::ErrorKind::invalid_type);

  // Numeric overflows / range errors & bad Base64 -> invalid_value
  { auto j = valid; j["u8_val"] = 256; expect_parity(j, rfl::ErrorKind::invalid_value); }
  { auto j = valid; j["u8_val"] = -1; expect_parity(j, rfl::ErrorKind::invalid_value); }
  { auto j = valid; j["u64_val"] = -5; expect_parity(j, rfl::ErrorKind::invalid_value); }
  { auto j = valid; j["i8_val"] = 200; expect_parity(j, rfl::ErrorKind::invalid_value); }
  { auto j = valid; j["i8_val"] = -200; expect_parity(j, rfl::ErrorKind::invalid_value); }
  {
    auto j = valid;
    j["i64_val"] = std::numeric_limits<uint64_t>::max();
    expect_parity(j, rfl::ErrorKind::invalid_value);
  }
  { auto j = valid; j["bytes_val"] = "!!!not-base64!!!"; expect_parity(j, rfl::ErrorKind::invalid_value); }

  // Structural / constraint / tagged union errors
  { auto j = valid; j.erase("str_val"); expect_parity(j, rfl::ErrorKind::missing_field); }
  { auto j = valid; j["extra"] = 1; expect_parity(j, rfl::ErrorKind::unknown_field); }
  { auto j = valid; j["age"] = -1; expect_parity(j, rfl::ErrorKind::constraint_violation); }
  { auto j = valid; j["shape"] = nlohmann::json::object(); expect_parity(j, rfl::ErrorKind::missing_field); }
  { auto j = valid; j["shape"] = {{"kind", 123}, {"radius", 1.0}}; expect_parity(j, rfl::ErrorKind::invalid_type); }
  { auto j = valid; j["shape"] = {{"kind", "Triangle"}, {"radius", 1.0}}; expect_parity(j, rfl::ErrorKind::invalid_value); }

  // Direct Vectorstring non-string check
  const auto vec_err =
      rfl::nlohmann::read<BinaryFields>(nlohmann::json{{"bytes", "AA=="}, {"vec", 123}});
  ASSERT_FALSE(vec_err.has_value());
  EXPECT_EQ(vec_err.error().kind(), rfl::ErrorKind::invalid_type);

  // Direct array out-of-bounds check (NoFieldNames)
  struct Pair { int a; int b; };
  const auto oob_nj =
      rfl::nlohmann::read<Pair, rfl::NoFieldNames>(nlohmann::json::array({1}));
  ASSERT_FALSE(oob_nj.has_value());
  EXPECT_EQ(oob_nj.error().kind(), rfl::ErrorKind::missing_field);
}
#endif

}  // namespace test_nlohmann
