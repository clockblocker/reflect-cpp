#include <gtest/gtest.h>

#include <array>
#include <bitset>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <rfl.hpp>
#include <rfl/env.hpp>
#include <rfl/from_generic.hpp>
#include <rfl/json.hpp>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace test_json_error_kind {

using Path = std::vector<rfl::ErrorDetail::PathSegment>;

enum class Mode { fast, safe };

struct Item {
  std::string name;
  rfl::Validator<int, rfl::Minimum<1>> count;
};

struct Config {
  std::string host;
  int port;
  Mode mode;
  rfl::Literal<"http", "https"> scheme;
  std::vector<Item> items;
};

struct Circle {
  double radius;
};

struct Square {
  double width;
};

using Shape = rfl::TaggedUnion<"shape", Circle, Square>;

struct EnvGuard {
  std::vector<std::string> keys;

  void set(const std::string& _key, const std::string& _val) {
    keys.push_back(_key);
#ifdef _WIN32
    _putenv_s(_key.c_str(), _val.c_str());
#else
    setenv(_key.c_str(), _val.c_str(), 1);
#endif
  }

  void unset(const std::string& _key) {
#ifdef _WIN32
    _putenv_s(_key.c_str(), "");
#else
    unsetenv(_key.c_str());
#endif
  }

  ~EnvGuard() {
    for (const auto& k : keys) {
      unset(k);
    }
  }
};

TEST(json, test_error_kind_inline_zero_alloc_and_helpers) {
  static_assert(std::is_same_v<std::underlying_type_t<rfl::ErrorKind>, uint8_t>);

  const rfl::Error plain("plain message");
  EXPECT_EQ(plain.kind(), rfl::ErrorKind::other);
  EXPECT_FALSE(rfl::internal::has_allocated_details(plain));
  ASSERT_EQ(plain.details().size(), 1u);
  EXPECT_EQ(plain.details()[0].kind, rfl::ErrorKind::other);

  // Leaf classified errors store kind_ inline without allocating OptionalDetails.
  const rfl::Error leaf_copy(std::string("bad"), rfl::ErrorKind::invalid_value);
  EXPECT_EQ(leaf_copy.kind(), rfl::ErrorKind::invalid_value);
  EXPECT_FALSE(rfl::internal::has_allocated_details(leaf_copy));
  EXPECT_EQ(leaf_copy.details().size(), 1u);
  EXPECT_EQ(leaf_copy.details()[0].kind, rfl::ErrorKind::invalid_value);
  EXPECT_EQ(leaf_copy.details()[0].message, "bad");
  EXPECT_TRUE(leaf_copy.details()[0].path.empty());

  rfl::Error leaf_move(std::string("moved"), rfl::ErrorKind::syntax_error);
  EXPECT_FALSE(rfl::internal::has_allocated_details(leaf_move));
  const auto moved_details = std::move(leaf_move).details();
  ASSERT_EQ(moved_details.size(), 1u);
  EXPECT_EQ(moved_details[0].kind, rfl::ErrorKind::syntax_error);
  EXPECT_EQ(moved_details[0].message, "moved");

  // rfl::error(what, kind) and empty-path overload stay zero-alloc.
  const auto leaf_helper = rfl::error("bad value", rfl::ErrorKind::invalid_value);
  EXPECT_EQ(leaf_helper.error().kind(), rfl::ErrorKind::invalid_value);
  EXPECT_FALSE(rfl::internal::has_allocated_details(leaf_helper.error()));

  const auto empty_path =
      rfl::error("bad value", rfl::ErrorKind::invalid_value, {});
  EXPECT_EQ(empty_path.error().kind(), rfl::ErrorKind::invalid_value);
  EXPECT_FALSE(rfl::internal::has_allocated_details(empty_path.error()));

  // Non-empty path allocates details and preserves path & kind.
  const auto nested_helper =
      rfl::error("bad value", rfl::ErrorKind::invalid_value, {"port"});
  EXPECT_TRUE(rfl::internal::has_allocated_details(nested_helper.error()));
  EXPECT_EQ(nested_helper.error().kind(), rfl::ErrorKind::invalid_value);
  ASSERT_EQ(nested_helper.error().details().size(), 1u);
  EXPECT_EQ(nested_helper.error().details()[0].path, (Path{"port"}));
  EXPECT_EQ(nested_helper.error().details()[0].kind,
            rfl::ErrorKind::invalid_value);
  EXPECT_EQ(nested_helper.error().what(), "bad value");

  // Aggregate initialization without .kind remains backward-compatible.
  const rfl::ErrorDetail compat{.path = {"x"}, .message = "msg"};
  EXPECT_EQ(compat.kind, rfl::ErrorKind::other);

  // Leaf errors produced by JSON & Validator reads do not allocate details.
  const auto leaf_json = rfl::json::read<int>("\"abc\"");
  ASSERT_FALSE(leaf_json);
  EXPECT_EQ(leaf_json.error().kind(), rfl::ErrorKind::invalid_type);
  EXPECT_FALSE(rfl::internal::has_allocated_details(leaf_json.error()));

  const auto leaf_val =
      rfl::json::read<rfl::Validator<int, rfl::Minimum<1>>>("0");
  ASSERT_FALSE(leaf_val);
  EXPECT_EQ(leaf_val.error().kind(), rfl::ErrorKind::constraint_violation);
  EXPECT_FALSE(rfl::internal::has_allocated_details(leaf_val.error()));
}

TEST(json, test_error_kind_syntax_error) {
  const auto res = rfl::json::read<Config>("{not valid json");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().kind(), rfl::ErrorKind::syntax_error);
  EXPECT_FALSE(rfl::internal::has_allocated_details(res.error()));
  ASSERT_EQ(res.error().details().size(), 1u);
  EXPECT_TRUE(res.error().details()[0].path.empty());
  EXPECT_EQ(res.error().details()[0].kind, rfl::ErrorKind::syntax_error);
}

TEST(json, test_error_kind_multi_error_aggregation) {
  const std::string json_str = R"({
    "port": "not_an_int",
    "mode": "turbo",
    "scheme": "ftp",
    "extra": 123,
    "items": [
      {"name": "ok", "count": 0}
    ]
  })";

  const auto res = rfl::json::read<Config, rfl::NoExtraFields>(json_str);
  ASSERT_FALSE(res);

  const auto details = res.error().details();
  ASSERT_EQ(details.size(), 6u) << res.error().what();

  EXPECT_EQ(details[0].path, (Path{"port"}));
  EXPECT_EQ(details[0].kind, rfl::ErrorKind::invalid_type);

  EXPECT_EQ(details[1].path, (Path{"mode"}));
  EXPECT_EQ(details[1].kind, rfl::ErrorKind::invalid_value);

  EXPECT_EQ(details[2].path, (Path{"scheme"}));
  EXPECT_EQ(details[2].kind, rfl::ErrorKind::invalid_value);

  EXPECT_EQ(details[3].path, (Path{"extra"}));
  EXPECT_EQ(details[3].kind, rfl::ErrorKind::unknown_field);

  EXPECT_EQ(details[4].path, (Path{"items", size_t{0}, "count"}));
  EXPECT_EQ(details[4].kind, rfl::ErrorKind::constraint_violation);

  EXPECT_EQ(details[5].path, (Path{"host"}));
  EXPECT_EQ(details[5].kind, rfl::ErrorKind::missing_field);

  // Error::kind() returns the first detail's kind for multi-error results.
  EXPECT_EQ(res.error().kind(), details[0].kind);
}

TEST(json, test_error_kind_invalid_type_vs_invalid_value) {
  // Structural type mismatch -> invalid_type
  const auto bad_obj = rfl::json::read<Item>("[1, 2]");
  ASSERT_FALSE(bad_obj);
  EXPECT_EQ(bad_obj.error().kind(), rfl::ErrorKind::invalid_type);

  const auto bad_arr = rfl::json::read<std::vector<int>>(R"({"a": 1})");
  ASSERT_FALSE(bad_arr);
  EXPECT_EQ(bad_arr.error().kind(), rfl::ErrorKind::invalid_type);

  // Wrong wire type for unsigned integer -> invalid_type
  const auto str_for_uint = rfl::json::read<uint8_t>("\"abc\"");
  ASSERT_FALSE(str_for_uint);
  EXPECT_EQ(str_for_uint.error().kind(), rfl::ErrorKind::invalid_type);

  // Negative integer for unsigned target (right wire type, bad domain) ->
  // invalid_value
  const auto neg_for_uint = rfl::json::read<uint8_t>("-1");
  ASSERT_FALSE(neg_for_uint);
  EXPECT_EQ(neg_for_uint.error().kind(), rfl::ErrorKind::invalid_value);
  EXPECT_EQ(neg_for_uint.error().what(), "Could not cast to uint.");

  // Positive integer out of range for target type -> invalid_value
  const auto overflow = rfl::json::read<uint8_t>("300");
  ASSERT_FALSE(overflow);
  EXPECT_EQ(overflow.error().kind(), rfl::ErrorKind::invalid_value);

  // Base64, bitset, and time_point domain errors -> invalid_value
  const auto bad_b64 = rfl::json::read<rfl::Bytestring>("\"!!!\"");
  ASSERT_FALSE(bad_b64);
  EXPECT_EQ(bad_b64.error().kind(), rfl::ErrorKind::invalid_value);

  const auto bad_bits = rfl::json::read<std::bitset<4>>("\"1020\"");
  ASSERT_FALSE(bad_bits);
  EXPECT_EQ(bad_bits.error().kind(), rfl::ErrorKind::invalid_value);

  const auto bad_tp =
      rfl::json::read<std::chrono::system_clock::time_point>("\"not-a-date\"");
  ASSERT_FALSE(bad_tp);
  EXPECT_EQ(bad_tp.error().kind(), rfl::ErrorKind::invalid_value);
}

TEST(json, test_error_kind_tagged_union_preserves_inner_and_disc_errors) {
  // Unknown discriminator value -> invalid_value at [tag]
  const auto tu_unknown =
      rfl::json::read<Shape>(R"({"shape": "Triangle", "sides": 3})");
  ASSERT_FALSE(tu_unknown);
  EXPECT_EQ(tu_unknown.error().kind(), rfl::ErrorKind::invalid_value);
  ASSERT_EQ(tu_unknown.error().details().size(), 1u);
  EXPECT_EQ(tu_unknown.error().details()[0].path, (Path{"shape"}));

  // Missing discriminator field -> missing_field at [tag] (not overwritten!)
  const auto tu_missing_disc = rfl::json::read<Shape>(R"({"radius": 1.0})");
  ASSERT_FALSE(tu_missing_disc);
  EXPECT_EQ(tu_missing_disc.error().kind(), rfl::ErrorKind::missing_field);
  ASSERT_EQ(tu_missing_disc.error().details().size(), 1u);
  EXPECT_EQ(tu_missing_disc.error().details()[0].path, (Path{"shape"}));

  // Non-string discriminator field -> invalid_type at [tag] (not overwritten!)
  const auto tu_bad_disc_type =
      rfl::json::read<Shape>(R"({"shape": 42, "radius": 1.0})");
  ASSERT_FALSE(tu_bad_disc_type);
  EXPECT_EQ(tu_bad_disc_type.error().kind(), rfl::ErrorKind::invalid_type);
  ASSERT_EQ(tu_bad_disc_type.error().details().size(), 1u);
  EXPECT_EQ(tu_bad_disc_type.error().details()[0].path, (Path{"shape"}));

  // Matched alternative missing a field -> missing_field preserved
  const auto tu_missing = rfl::json::read<Shape>(R"({"shape": "Circle"})");
  ASSERT_FALSE(tu_missing);
  EXPECT_EQ(tu_missing.error().kind(), rfl::ErrorKind::missing_field);
  ASSERT_EQ(tu_missing.error().details().size(), 1u);
  EXPECT_EQ(tu_missing.error().details()[0].path, (Path{"radius"}));

  // Matched alternative wrong field type -> invalid_type preserved
  const auto tu_bad_field =
      rfl::json::read<Shape>(R"({"shape": "Circle", "radius": "big"})");
  ASSERT_FALSE(tu_bad_field);
  EXPECT_EQ(tu_bad_field.error().kind(), rfl::ErrorKind::invalid_type);
  ASSERT_EQ(tu_bad_field.error().details().size(), 1u);
  EXPECT_EQ(tu_bad_field.error().details()[0].path, (Path{"radius"}));
}

TEST(json, test_error_kind_variant_field_variant_array_tuple_and_map) {
  const auto var_res = rfl::json::read<std::variant<int, bool>>("\"abc\"");
  ASSERT_FALSE(var_res);
  EXPECT_EQ(var_res.error().kind(), rfl::ErrorKind::invalid_value);

  const auto rfl_var_res = rfl::json::read<rfl::Variant<int, bool>>("\"abc\"");
  ASSERT_FALSE(rfl_var_res);
  EXPECT_EQ(rfl_var_res.error().kind(), rfl::ErrorKind::invalid_value);

  // Externally tagged FieldVariant
  using FV = rfl::Variant<rfl::Field<"circle", Circle>,
                          rfl::Field<"square", Square>>;
  const auto fv_empty = rfl::json::read<FV>("{}");
  ASSERT_FALSE(fv_empty);
  EXPECT_EQ(fv_empty.error().kind(), rfl::ErrorKind::invalid_value);

  const auto fv_unknown = rfl::json::read<FV>(R"({"tri": {}})");
  ASSERT_FALSE(fv_unknown);
  EXPECT_EQ(fv_unknown.error().kind(), rfl::ErrorKind::invalid_value);

  const auto fv_inner_missing = rfl::json::read<FV>(R"({"circle": {}})");
  ASSERT_FALSE(fv_inner_missing);
  EXPECT_EQ(fv_inner_missing.error().kind(), rfl::ErrorKind::missing_field);

  // Tuple & Array alignment: missing tuple slot -> missing_field;
  // tuple overflow & std::array size mismatch -> invalid_length.
  const auto tup_missing = rfl::json::read<std::tuple<int, std::string>>("[1]");
  ASSERT_FALSE(tup_missing);
  EXPECT_EQ(tup_missing.error().kind(), rfl::ErrorKind::missing_field);

  const auto tup_extra =
      rfl::json::read<std::tuple<int, std::string>>(R"([1, "a", 2])");
  ASSERT_FALSE(tup_extra);
  EXPECT_EQ(tup_extra.error().kind(), rfl::ErrorKind::invalid_length);

  const auto arr_missing = rfl::json::read<std::array<int, 2>>("[1]");
  ASSERT_FALSE(arr_missing);
  EXPECT_EQ(arr_missing.error().kind(), rfl::ErrorKind::invalid_length);

  const auto arr_extra = rfl::json::read<std::array<int, 2>>("[1, 2, 3]");
  ASSERT_FALSE(arr_extra);
  EXPECT_EQ(arr_extra.error().kind(), rfl::ErrorKind::invalid_length);

  // Map with numeric / validated key
  const auto bad_num_key =
      rfl::json::read<std::map<int, std::string>>(R"({"not_an_int": "v"})");
  ASSERT_FALSE(bad_num_key);
  EXPECT_EQ(bad_num_key.error().kind(), rfl::ErrorKind::invalid_value);

  using ValidKey = rfl::Validator<std::string, rfl::Size<rfl::Minimum<3>>>;
  const auto bad_val_key =
      rfl::json::read<std::map<ValidKey, int>>(R"({"ab": 1})");
  ASSERT_FALSE(bad_val_key);
  EXPECT_EQ(bad_val_key.error().kind(), rfl::ErrorKind::constraint_violation);

  // FailFast preserves kind
  const auto ff_res =
      rfl::json::read<Item, rfl::FailFast>(R"({"name": "a", "count": 0})");
  ASSERT_FALSE(ff_res);
  EXPECT_EQ(ff_res.error().kind(), rfl::ErrorKind::constraint_violation);
}

TEST(json, test_error_kind_generic_reader) {
  // Type mismatch -> invalid_type
  rfl::Generic::Object obj;
  obj["name"] = std::string("ok");
  obj["count"] = std::string("not_an_int");
  const auto bad_type = rfl::from_generic<Item>(rfl::Generic(obj));
  ASSERT_FALSE(bad_type);
  EXPECT_EQ(bad_type.error().kind(), rfl::ErrorKind::invalid_type);

  // Missing field -> missing_field
  rfl::Generic::Object incomplete;
  incomplete["name"] = std::string("ok");
  const auto missing = rfl::from_generic<Item>(rfl::Generic(incomplete));
  ASSERT_FALSE(missing);
  EXPECT_EQ(missing.error().kind(), rfl::ErrorKind::missing_field);

  // Out-of-range int64 -> int in Generic -> invalid_value
  const rfl::Generic huge_int(static_cast<int64_t>(1LL << 40));
  const auto overflow = rfl::from_generic<int>(huge_int);
  ASSERT_FALSE(overflow);
  EXPECT_EQ(overflow.error().kind(), rfl::ErrorKind::invalid_value);

  // Direct Generic::Object::get missing key -> missing_field
  const auto missing_key = incomplete.get("count");
  ASSERT_FALSE(missing_key);
  EXPECT_EQ(missing_key.error().kind(), rfl::ErrorKind::missing_field);
}

TEST(json, test_error_kind_env_reader) {
  struct EnvConfig {
    std::string rfl_ek_host;
    int rfl_ek_port;
    bool rfl_ek_debug;
    double rfl_ek_rate;
  };

  EnvGuard guard;
  guard.unset("RFL_EK_HOST");
  guard.unset("RFL_EK_PORT");
  guard.unset("RFL_EK_DEBUG");
  guard.unset("RFL_EK_RATE");

  // Bad lexical int/bool/float parse -> invalid_value (NOT invalid_type!)
  guard.set("RFL_EK_HOST", "localhost");
  guard.set("RFL_EK_PORT", "not_a_port");
  guard.set("RFL_EK_DEBUG", "maybe");
  guard.set("RFL_EK_RATE", "nan_ish");

  const auto res = rfl::env::read<EnvConfig>();
  ASSERT_FALSE(res);
  const auto details = res.error().details();
  ASSERT_EQ(details.size(), 3u) << res.error().what();
  EXPECT_EQ(details[0].kind, rfl::ErrorKind::invalid_value);
  EXPECT_EQ(details[1].kind, rfl::ErrorKind::invalid_value);
  EXPECT_EQ(details[2].kind, rfl::ErrorKind::invalid_value);

  // Missing env variable -> missing_field (both struct-level and direct reader)
  guard.unset("RFL_EK_HOST");
  guard.set("RFL_EK_PORT", "8080");
  guard.set("RFL_EK_DEBUG", "true");
  guard.set("RFL_EK_RATE", "1.5");

  const auto missing = rfl::env::read<EnvConfig>();
  ASSERT_FALSE(missing);
  EXPECT_EQ(missing.error().kind(), rfl::ErrorKind::missing_field);

  const auto missing_direct = rfl::env::Reader{}.to_basic_type<std::string>(
      rfl::env::InputEnvVarType{.path = "RFL_EK_HOST"});
  ASSERT_FALSE(missing_direct);
  EXPECT_EQ(missing_direct.error().kind(), rfl::ErrorKind::missing_field);
}

TEST(json, test_error_kind_io_error_and_wire_names) {
  const auto missing_file =
      rfl::json::load<Item>("/nonexistent/rfl_error_kind_missing_file.json");
  ASSERT_FALSE(missing_file);
  EXPECT_EQ(missing_file.error().kind(), rfl::ErrorKind::io_error);
  EXPECT_FALSE(rfl::internal::has_allocated_details(missing_file.error()));

  const auto unwritable = rfl::json::save(
      "/nonexistent/rfl_error_kind_dir/out.json", Item{.name = "a", .count = 1});
  ASSERT_FALSE(unwritable);
  EXPECT_EQ(unwritable.error().kind(), rfl::ErrorKind::io_error);

  // Verify JSON wire serialization of ErrorDetail::kind names.
  const auto err = rfl::error("bad", rfl::ErrorKind::invalid_length, {"items"});
  const auto serialized = rfl::json::write(err.error().details());
  EXPECT_NE(serialized.find(R"("kind":"invalid_length")"), std::string::npos)
      << serialized;
}

}  // namespace test_json_error_kind
