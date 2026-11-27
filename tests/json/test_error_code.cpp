#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace test_json_error_code {

using Path = std::vector<rfl::ErrorDetail::PathSegment>;

struct MustBeEven {
  static constexpr std::string_view code = "even";
  static rfl::Result<int> validate(const int _v) {
    if (_v % 2 != 0) {
      return rfl::error("Expected an even integer.");
    }
    return _v;
  }
  template <class T>
  static rfl::parsing::schema::ValidationType to_schema() {
    return rfl::parsing::schema::ValidationType{
        rfl::parsing::schema::ValidationType::AllOf{}};
  }
};

struct EvenLength {
  static constexpr const char* code = "even_length";
  static rfl::Result<size_t> validate(const size_t _n) {
    if (_n % 2 != 0) {
      return rfl::error("Expected an even length.");
    }
    return _n;
  }
  template <class T>
  static rfl::parsing::schema::ValidationType to_schema() {
    return rfl::parsing::schema::ValidationType{
        rfl::parsing::schema::ValidationType::AllOf{}};
  }
};

struct UncodedSizeRule {
  static rfl::Result<size_t> validate(const size_t _n) {
    if (_n == 3) {
      return rfl::error("Length 3 is forbidden.");
    }
    return _n;
  }
  template <class T>
  static rfl::parsing::schema::ValidationType to_schema() {
    return rfl::parsing::schema::ValidationType{
        rfl::parsing::schema::ValidationType::AllOf{}};
  }
};

struct DynamicCodeValidator {
  static constexpr std::string_view code = "fallback_code";
  static rfl::Result<int> validate(const int _v) {
    if (_v < 0) {
      return rfl::error("Must not be negative.",
                        rfl::ErrorKind::constraint_violation, "non_negative");
    }
    if (_v == 13) {
      return rfl::error("Unlucky number.");
    }
    return _v;
  }
  template <class T>
  static rfl::parsing::schema::ValidationType to_schema() {
    return rfl::parsing::schema::ValidationType{
        rfl::parsing::schema::ValidationType::AllOf{}};
  }
};

struct ThrowingCustomValidator {
  static constexpr rfl::internal::StringLiteral code = "boom";
  static rfl::Result<int> validate(const int _v) {
    if (_v == 0) {
      throw std::runtime_error("cannot be zero");
    }
    return _v;
  }
  template <class T>
  static rfl::parsing::schema::ValidationType to_schema() {
    return rfl::parsing::schema::ValidationType{
        rfl::parsing::schema::ValidationType::AllOf{}};
  }
};

struct ObsLikeRequest {
  rfl::Validator<std::string, rfl::Size<rfl::Minimum<1>>> scene_name;
  rfl::Validator<int, rfl::Minimum<0>, rfl::Maximum<100>> volume;
};

TEST(json, test_error_code_builtin_validators_zero_alloc) {
  // sizeof(Error) must match the pre-code layout exactly (builtin_code_ packs
  // into ErrorKind's existing tail padding).
  struct PreCodeErrorLayout {
    std::string what_;
    void* details_ptr_;
    rfl::ErrorKind kind_;
  };
  static_assert(sizeof(rfl::Error) == sizeof(PreCodeErrorLayout));

  {
    const auto res = rfl::json::read<rfl::Validator<int, rfl::Minimum<5>>>("4");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::constraint_violation);
    EXPECT_EQ(res.error().code(), "minimum");
    EXPECT_FALSE(rfl::internal::has_allocated_details(res.error()));
    EXPECT_EQ(res.error().details().at(0).code, "minimum");
  }
  {
    const auto res =
        rfl::json::read<rfl::Validator<int, rfl::ExclusiveMinimum<5>>>("5");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), "exclusive_minimum");
    EXPECT_FALSE(rfl::internal::has_allocated_details(res.error()));
  }
  {
    const auto res =
        rfl::json::read<rfl::Validator<int, rfl::Maximum<10>>>("11");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), "maximum");
    EXPECT_FALSE(rfl::internal::has_allocated_details(res.error()));
  }
  {
    const auto res =
        rfl::json::read<rfl::Validator<int, rfl::ExclusiveMaximum<10>>>("10");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), "exclusive_maximum");
    EXPECT_FALSE(rfl::internal::has_allocated_details(res.error()));
  }
  {
    const auto res =
        rfl::json::read<rfl::Validator<int, rfl::EqualTo<42>>>("41");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), "equal_to");
    EXPECT_FALSE(rfl::internal::has_allocated_details(res.error()));
  }
  {
    const auto res =
        rfl::json::read<rfl::Validator<int, rfl::NotEqualTo<0>>>("0");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), "not_equal_to");
    EXPECT_FALSE(rfl::internal::has_allocated_details(res.error()));
  }
  {
    const auto res = rfl::json::read<rfl::Email>("\"not-an-email\"");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::constraint_violation);
    EXPECT_EQ(res.error().code(), "pattern");
    EXPECT_FALSE(rfl::internal::has_allocated_details(res.error()));
  }
  {
    const auto res = rfl::json::read<
        rfl::Validator<int, rfl::AnyOf<rfl::EqualTo<1>, rfl::EqualTo<2>>>>("3");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), "any_of");
    const auto details = res.error().details();
    ASSERT_EQ(details.size(), 1u);
    ASSERT_EQ(details[0].causes.size(), 2u);
    EXPECT_EQ(details[0].causes[0].code, "equal_to");
    EXPECT_EQ(details[0].causes[1].code, "equal_to");
  }
  {
    using OneOfRange =
        rfl::Validator<int, rfl::OneOf<rfl::Minimum<0>, rfl::Maximum<10>>>;
    // Both constraints pass -> OneOf fails with code "one_of".
    const auto both_pass = rfl::json::read<OneOfRange>("5");
    ASSERT_FALSE(both_pass);
    EXPECT_EQ(both_pass.error().code(), "one_of");
    EXPECT_FALSE(rfl::internal::has_allocated_details(both_pass.error()));

    // Exactly one constraint passes -> succeeds.
    const auto one_pass_hi = rfl::json::read<OneOfRange>("15");
    ASSERT_TRUE(one_pass_hi) << one_pass_hi.error().what();
    EXPECT_EQ(one_pass_hi->value(), 15);
    const auto one_pass_lo = rfl::json::read<OneOfRange>("-5");
    ASSERT_TRUE(one_pass_lo) << one_pass_lo.error().what();
    EXPECT_EQ(one_pass_lo->value(), -5);

    // Zero constraints pass -> OneOf fails with code "one_of".
    const auto none_pass = rfl::json::read<
        rfl::Validator<int, rfl::OneOf<rfl::EqualTo<1>, rfl::EqualTo<2>>>>("3");
    ASSERT_FALSE(none_pass);
    EXPECT_EQ(none_pass.error().code(), "one_of");
    EXPECT_FALSE(rfl::internal::has_allocated_details(none_pass.error()));
  }
}

TEST(json, test_error_code_size_and_code_points_composition) {
  {
    const auto res = rfl::json::read<
        rfl::Validator<std::string, rfl::Size<rfl::Minimum<1>>>>("\"\"");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::constraint_violation);
    EXPECT_EQ(res.error().code(), "size.minimum");
    EXPECT_FALSE(rfl::internal::has_allocated_details(res.error()));
  }
  {
    const auto res = rfl::json::read<rfl::Validator<
        std::vector<int>,
        rfl::Size<rfl::AllOf<rfl::Minimum<1>, rfl::Maximum<2>>>>>("[1,2,3]");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), "size.maximum");
    EXPECT_FALSE(rfl::internal::has_allocated_details(res.error()));
  }
  {
    const auto res = rfl::json::read<
        rfl::Validator<std::string, rfl::CodePoints<rfl::Maximum<2>>>>(
        "\"abc\"");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), "code_points.maximum");
    EXPECT_FALSE(rfl::internal::has_allocated_details(res.error()));
  }
  {
    const auto res =
        rfl::json::read<rfl::Validator<std::string, rfl::Size<EvenLength>>>(
            "\"abc\"");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), "size.even_length");
  }
  {
    const auto res = rfl::json::read<
        rfl::Validator<std::string, rfl::Size<UncodedSizeRule>>>("\"abc\"");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), "size");
    EXPECT_FALSE(rfl::internal::has_allocated_details(res.error()));
  }
  {
    const auto res = rfl::json::read<
        rfl::Validator<std::string, rfl::CodePoints<UncodedSizeRule>>>(
        "\"abc\"");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), "code_points");
    EXPECT_FALSE(rfl::internal::has_allocated_details(res.error()));
  }
}

TEST(json, test_error_code_custom_validators_and_all_of) {
  {
    const auto res =
        rfl::json::read<rfl::Validator<int, MustBeEven, rfl::Minimum<0>>>("3");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::constraint_violation);
    EXPECT_EQ(res.error().code(), "even");
  }
  {
    const auto res =
        rfl::json::read<rfl::Validator<int, MustBeEven, rfl::Minimum<0>>>("-2");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::constraint_violation);
    EXPECT_EQ(res.error().code(), "minimum");
    EXPECT_FALSE(rfl::internal::has_allocated_details(res.error()));
  }
  {
    // Dynamic explicit code overrides declarative static fallback_code.
    const auto res1 =
        rfl::json::read<rfl::Validator<int, DynamicCodeValidator>>("-5");
    ASSERT_FALSE(res1);
    EXPECT_EQ(res1.error().code(), "non_negative");

    // Uncoded error falls back to static declarative code.
    const auto res2 =
        rfl::json::read<rfl::Validator<int, DynamicCodeValidator>>("13");
    ASSERT_FALSE(res2);
    EXPECT_EQ(res2.error().code(), "fallback_code");
  }
  {
    // Exception thrown inside validate() still attaches declarative code.
    const auto res =
        rfl::json::read<rfl::Validator<int, ThrowingCustomValidator>>("0");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::constraint_violation);
    EXPECT_EQ(res.error().code(), "boom");
  }
}

TEST(json, test_error_code_nested_struct_obs_mapping_and_helpers) {
  // Mirrors obs-websocket RequestCodec mapping: 403 (RequestFieldEmpty) vs
  // 402 (RequestFieldOutOfRange) purely via ErrorDetail::code without string
  // prefix matching.
  const auto classify_obs = [](const rfl::ErrorDetail& _d) -> int {
    if (_d.kind == rfl::ErrorKind::constraint_violation) {
      if (_d.code == "size.minimum" || _d.code == "code_points.minimum") {
        return 403;
      }
      return 402;
    }
    return 400;
  };

  const auto res = rfl::json::read<ObsLikeRequest>(
      R"({"scene_name": "", "volume": 150})");
  ASSERT_FALSE(res);
  const auto details = res.error().details();
  ASSERT_EQ(details.size(), 2u);

  EXPECT_EQ(details[0].path, (Path{"scene_name"}));
  EXPECT_EQ(details[0].kind, rfl::ErrorKind::constraint_violation);
  EXPECT_EQ(details[0].code, "size.minimum");
  EXPECT_EQ(classify_obs(details[0]), 403);

  EXPECT_EQ(details[1].path, (Path{"volume"}));
  EXPECT_EQ(details[1].kind, rfl::ErrorKind::constraint_violation);
  EXPECT_EQ(details[1].code, "maximum");
  EXPECT_EQ(classify_obs(details[1]), 402);

  // Error::code() returns the first detail's code on multi-error results.
  EXPECT_EQ(res.error().code(), "size.minimum");

  // rfl::error helpers with code + path and JSON round-trip.
  const auto helper_leaf =
      rfl::error("bad", rfl::ErrorKind::constraint_violation, "custom_rule");
  EXPECT_EQ(helper_leaf.error().kind(), rfl::ErrorKind::constraint_violation);
  EXPECT_EQ(helper_leaf.error().code(), "custom_rule");

  const auto helper_nested = rfl::error(
      "bad", rfl::ErrorKind::constraint_violation, {"items", size_t{1}},
      "custom_rule");
  ASSERT_EQ(helper_nested.error().details().size(), 1u);
  EXPECT_EQ(helper_nested.error().details()[0].path, (Path{"items", size_t{1}}));
  EXPECT_EQ(helper_nested.error().details()[0].code, "custom_rule");

  // Backward-compatible empty/braced path overloads remain unambiguous.
  const auto empty_path = rfl::error("bad", rfl::ErrorKind::invalid_value, {});
  EXPECT_EQ(empty_path.error().code(), "");
  const auto braced_path =
      rfl::error("bad", rfl::ErrorKind::invalid_value, {"port"});
  EXPECT_EQ(braced_path.error().details()[0].path, (Path{"port"}));
  EXPECT_EQ(braced_path.error().details()[0].code, "");

  // JSON serialization round-trip preserves ErrorDetail::code.
  const auto json_str = rfl::json::write(details);
  EXPECT_NE(json_str.find(R"("code":"size.minimum")"), std::string::npos)
      << json_str;
  const auto roundtrip =
      rfl::json::read<std::vector<rfl::ErrorDetail>>(json_str);
  ASSERT_TRUE(roundtrip) << roundtrip.error().what();
  EXPECT_EQ(*roundtrip, details);

  // Runtime null const char* and std::string_view 4-arg helper safety.
  const char* null_code = nullptr;
  const auto null_err =
      rfl::error("bad", rfl::ErrorKind::constraint_violation, null_code);
  EXPECT_EQ(null_err.error().code(), "");
  EXPECT_FALSE(rfl::internal::has_allocated_details(null_err.error()));

  const std::string_view sv_code = "sv_rule";
  const auto sv_nested = rfl::error(
      "bad", rfl::ErrorKind::constraint_violation, {"a"}, sv_code);
  EXPECT_EQ(sv_nested.error().code(), "sv_rule");
  EXPECT_EQ(sv_nested.error().details()[0].path, (Path{"a"}));

  // Propagation through wrap_error (prefix-only zero-alloc), in_field, at, and
  // attach_source_location.
  auto leaf = rfl::Error("too small", rfl::ErrorKind::constraint_violation,
                         "minimum");
  EXPECT_FALSE(rfl::internal::has_allocated_details(leaf));
  auto prefixed = rfl::parsing::wrap_error("ctx: ", std::move(leaf));
  EXPECT_EQ(prefixed.code(), "minimum");
  EXPECT_EQ(prefixed.kind(), rfl::ErrorKind::constraint_violation);
  EXPECT_FALSE(rfl::internal::has_allocated_details(prefixed));

  auto chained = std::move(prefixed).in_field("port").at(size_t{0});
  ASSERT_EQ(chained.details().size(), 1u);
  EXPECT_EQ(chained.details()[0].path, (Path{size_t{0}, "port"}));
  EXPECT_EQ(chained.details()[0].code, "minimum");

  auto located = rfl::parsing::attach_source_location(
      rfl::SourceLocation{.line = 4, .column = 9, .exact = true},
      std::move(chained));
  ASSERT_EQ(located.details().size(), 1u);
  EXPECT_EQ(located.details()[0].code, "minimum");
  ASSERT_TRUE(located.details()[0].location.has_value());
  EXPECT_EQ(located.details()[0].location->line, 4u);

  // Validated map keys preserve ErrorDetail::code via MapReader.
  using ValidKey = rfl::Validator<std::string, rfl::Size<rfl::Minimum<3>>>;
  const auto bad_map_key =
      rfl::json::read<std::map<ValidKey, int>>(R"({"ab": 1})");
  ASSERT_FALSE(bad_map_key);
  EXPECT_EQ(bad_map_key.error().kind(), rfl::ErrorKind::constraint_violation);
  EXPECT_EQ(bad_map_key.error().code(), "size.minimum");
}

}  // namespace test_json_error_code
