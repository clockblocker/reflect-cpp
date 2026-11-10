#include <gtest/gtest.h>

#include <atomic>
#include <bit>
#include <cstdint>
#include <exception>
#include <map>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

// Behaviour that must not change while structs with defaults learn to hold a
// Validator without a valid default value. The first cases are the review
// findings on an earlier attempt to fix #700 (#737).
namespace test_default_val_with_validator_regressions {

using Path = std::vector<rfl::ErrorDetail::PathSegment>;

std::vector<Path> paths(const rfl::Error& _err) {
  std::vector<Path> result;
  for (const auto& detail : _err.details()) {
    result.push_back(detail.path);
  }
  return result;
}

using Foods =
    rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>>;
using Positive = rfl::Validator<int, rfl::Minimum<1>>;

// An invalid map key is an error, not std::terminate.
using KeyMap = std::map<Positive, std::string>;

struct WithMap {
  rfl::DefaultVal<int> x;
  KeyMap m;
};

TEST(json, test_default_val_with_validator_regressions_map_keys) {
  EXPECT_FALSE(rfl::json::read<KeyMap>(R"({"0": "zero"})"));
  EXPECT_FALSE(rfl::json::read<WithMap>(R"({"m": {"0": "zero"}})"));
  const auto res = rfl::json::read<WithMap>(R"({"m": {"1": "one"}})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->m.begin()->first.value(), 1);
}

// Structs without a default constructor can still be read.
struct NoDefaultConstructor {
  NoDefaultConstructor(int _v) : v_(_v) {}
  using ReflectionType = int;
  int reflection() const { return v_; }
  int v_;
};

struct NotDefaultConstructible {
  NoDefaultConstructor n;
  Positive p;
};

static_assert(!std::is_default_constructible_v<NotDefaultConstructible>);

TEST(json, test_default_val_with_validator_regressions_no_default_constructor) {
  const auto res =
      rfl::json::read<NotDefaultConstructible>(R"({"n": 3, "p": 4})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->n.v_, 3);
  EXPECT_EQ(res->p.value(), 4);
}

// A const Validator keeps its parsed value, with and without defaults.
struct ConstValidator {
  const rfl::Validator<unsigned, rfl::Minimum<1>> age;
  rfl::DefaultVal<std::string> name = "Maggie";
};

TEST(json, test_default_val_with_validator_regressions_const_validator) {
  const auto res = rfl::json::read<ConstValidator>(R"({"age": 8})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->age.value(), 8u);
  EXPECT_EQ(rfl::json::write(*res), R"({"age":8,"name":"Maggie"})");
}

struct ConstFields {
  const rfl::Validator<unsigned, rfl::Minimum<0>> age;
  const std::string name;
};

TEST(json, test_default_val_with_validator_regressions_const_fields) {
  const auto res =
      rfl::json::read<ConstFields>(R"({"age": 8, "name": "Maggie"})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->age.value(), 8u);
  EXPECT_EQ(res->name, "Maggie");
}

// A throwing default constructor is still an error, and the struct is still
// constructed only once.
struct Throws {
  static inline int constructions = 0;
  Throws() {
    ++constructions;
    throw std::runtime_error("boom");
  }
  Throws(int) {}
  using ReflectionType = int;
  int reflection() const { return 0; }
};

struct WithThrowing {
  rfl::DefaultVal<int> x;
  Throws t;
};

TEST(json, test_default_val_with_validator_regressions_throwing_constructor) {
  Throws::constructions = 0;
  const auto res = rfl::json::read<WithThrowing>(R"({"t": 1})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(), "boom");
  EXPECT_EQ(Throws::constructions, 1);
}

struct WithThrowingNested {
  rfl::DefaultVal<int> x;
  std::vector<WithThrowing> nested;
};

TEST(json,
     test_default_val_with_validator_regressions_throwing_constructor_nested) {
  const auto res =
      rfl::json::read<WithThrowingNested>(R"({"nested": [{"t": 1}]})");
  ASSERT_FALSE(res);
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"nested", size_t{0}}}));
  EXPECT_NE(res.error().what().find("boom"), std::string::npos);
}

// The default value is only checked lazily inside rfl::json::read. Outside of
// it, a Validator still always holds a valid value (#707).
struct Person {
  rfl::DefaultVal<std::string> name = "Homer Simpson";
  Foods favorite_foods;
};

TEST(json, test_default_val_with_validator_regressions_public_construction) {
  EXPECT_THROW((void)Foods(), std::exception);
  EXPECT_THROW((void)Positive(0), std::exception);
  EXPECT_THROW((void)Person(), std::exception);
  EXPECT_THROW((void)Person{}, std::exception);
  Positive p = 3;
  Positive q = p;
  q = Positive(4);
  EXPECT_EQ(p.value(), 3);
  EXPECT_EQ(q.value(), 4);
}

// A fallback in a default member initializer keeps working if the Validator
// is constructed from a value.
Positive port_or_fallback() {
  try {
    return Positive(0);
  } catch (std::exception&) {
    return Positive(8080);
  }
}

struct Fallback {
  rfl::DefaultVal<std::string> host = "localhost";
  Positive port = port_or_fallback();
};

TEST(json, test_default_val_with_validator_regressions_fallback) {
  const auto res = rfl::json::read<Fallback, rfl::DefaultIfMissing>(R"({})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->port.value(), 8080);
}

// The same with a default-constructed Validator: the struct can be
// constructed, so nothing is deferred and the fallback is taken.
Positive default_port_or_fallback() {
  try {
    return Positive();
  } catch (std::exception&) {
    return Positive(8080);
  }
}

struct DefaultConstructedFallback {
  rfl::DefaultVal<std::string> host = "localhost";
  Positive port = default_port_or_fallback();
};

TEST(json,
     test_default_val_with_validator_regressions_default_constructed_fallback) {
  EXPECT_EQ(DefaultConstructedFallback{}.port.value(), 8080);
  const auto res =
      rfl::json::read<DefaultConstructedFallback, rfl::DefaultIfMissing>(
          R"({})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->port.value(), 8080);
  const auto res2 =
      rfl::json::read<DefaultConstructedFallback>(R"({"port": 1})");
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_EQ(res2->port.value(), 1);
}

// Validator stays trivially copyable, so it works with std::atomic and
// std::bit_cast.
static_assert(std::is_trivially_copyable_v<Positive>);
static_assert(std::is_trivially_copyable_v<rfl::DefaultVal<Positive>>);

TEST(json, test_default_val_with_validator_regressions_trivially_copyable) {
  std::atomic<Positive> a{Positive(3)};
  const Positive p = a.load();
  EXPECT_EQ(p.value(), 3);
  EXPECT_EQ(std::bit_cast<int>(p), 3);
}

// An rfl::Skip field that is in the input is not read. Its default value
// threw inside a noexcept function.
struct SkipsValidator {
  rfl::DefaultVal<int> x = 1;
  rfl::Skip<Positive> s;
  int y = 0;
};

TEST(json, test_default_val_with_validator_regressions_skip) {
  const auto res = rfl::json::read<SkipsValidator>(R"({"y": 1, "s": 5})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Failed to parse field 's': Value expected to be greater than or "
            "equal to 1, but got 0.");
  const auto res2 = rfl::json::read<SkipsValidator>(R"({"y": 1})");
  ASSERT_FALSE(res2);
  EXPECT_EQ(paths(res2.error()), std::vector<Path>({{"s"}}));
}

// [[no_unique_address]] lets members share storage, so a member's address
// does not tell which field it belongs to.
struct Tag {};

struct SharesAddress {
  [[no_unique_address]] Tag tag;
  rfl::DefaultVal<Positive> p;
};

struct InTailPadding {
  rfl::Validator<int64_t, rfl::Minimum<0>> a;
  int8_t c = 0;
};

struct SharesPadding {
  [[no_unique_address]] InTailPadding inner;
  rfl::DefaultVal<Positive> p;
};

TEST(json, test_default_val_with_validator_regressions_no_unique_address) {
  const auto res = rfl::json::read<SharesAddress>(R"({"tag": {}})");
  ASSERT_FALSE(res);
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"p"}}));
  const auto res2 = rfl::json::read<SharesAddress>(R"({"tag": {}, "p": 3})");
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_EQ(res2->p.value().value(), 3);

  const auto res3 =
      rfl::json::read<SharesPadding>(R"({"inner": {"a": 1, "c": 2}})");
  ASSERT_FALSE(res3);
  EXPECT_EQ(paths(res3.error()), std::vector<Path>({{"p"}}));
  const auto res4 =
      rfl::json::read<SharesPadding>(R"({"inner": {"a": 1, "c": 2}, "p": 3})");
  ASSERT_TRUE(res4) << res4.error().what();
  EXPECT_EQ(res4->p.value().value(), 3);
}

struct SharesAddressNoDefaultVal {
  [[no_unique_address]] Tag tag;
  Positive p;
};

TEST(json,
     test_default_val_with_validator_regressions_no_unique_address_missing) {
  using S = SharesAddressNoDefaultVal;
  const auto res = rfl::json::read<S, rfl::DefaultIfMissing>(R"({"tag": {}})");
  ASSERT_FALSE(res);
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"p"}}));
  const auto res2 = rfl::json::read<S, rfl::DefaultIfMissing>(R"({"p": 3})");
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_EQ(res2->p.value(), 3);
}

// Only a Validator that is default-constructed in place inside the struct is
// checked lazily. A temporary, as in `= Positive()` passed to a DefaultVal,
// is checked when it is constructed, as before.
struct DefaultValNoInitializer {
  rfl::DefaultVal<int> x;
  rfl::DefaultVal<Positive> p;
};

struct DefaultValBraces {
  rfl::DefaultVal<int> x;
  rfl::DefaultVal<Positive> p{};
};

struct DefaultValDefault {
  rfl::DefaultVal<int> x;
  rfl::DefaultVal<Positive> p = rfl::Default{};
};

struct ValidatorFromTemporary {
  rfl::DefaultVal<int> x;
  Positive p = Positive();
};

struct DefaultValFromTemporary {
  rfl::DefaultVal<int> x;
  rfl::DefaultVal<Positive> p = Positive();
};

TEST(json, test_default_val_with_validator_regressions_initializer_spellings) {
  const auto json = R"({"p": 3})";
  EXPECT_TRUE(rfl::json::read<DefaultValNoInitializer>(json));
  EXPECT_TRUE(rfl::json::read<DefaultValBraces>(json));
  EXPECT_TRUE(rfl::json::read<DefaultValDefault>(json));
  EXPECT_TRUE(rfl::json::read<ValidatorFromTemporary>(json));
  const auto res = rfl::json::read<DefaultValFromTemporary>(json);
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Value expected to be greater than or equal to 1, but got 0.");
}

}  // namespace test_default_val_with_validator_regressions
