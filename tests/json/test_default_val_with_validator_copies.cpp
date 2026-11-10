#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <exception>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

// A Validator whose default value is invalid must not reach the result of a
// read unnoticed, not even as a copy in another field.
namespace test_default_val_with_validator_copies {

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

struct Person {
  rfl::DefaultVal<std::string> name = "Homer Simpson";
  Foods favorite_foods;
};

struct CopiesAcrossFields {
  Positive a;
  rfl::DefaultVal<Positive> b = a;
  std::optional<Positive> c = a;
};

TEST(json, test_default_val_with_validator_copies_missing) {
  const auto res = rfl::json::read<CopiesAcrossFields>(R"({"a": 5})");
  ASSERT_FALSE(res);
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"b"}, {"c"}}));
}

TEST(json, test_default_val_with_validator_copies_present) {
  const auto res =
      rfl::json::read<CopiesAcrossFields>(R"({"a": 5, "b": 6, "c": 7})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->a.value(), 5);
  EXPECT_EQ(res->b.value().value(), 6);
  EXPECT_EQ(res->c->value(), 7);
}

struct MovesAcrossFields {
  Positive a;
  rfl::DefaultVal<Positive> b = 7;
  rfl::DefaultVal<Positive> c = std::move(a);
  rfl::Skip<int> side_effect = [this] {
    b.value() = std::move(a);
    return 0;
  }();
};

TEST(json, test_default_val_with_validator_copies_moved_missing) {
  const auto res = rfl::json::read<MovesAcrossFields>(R"({"a": 5})");
  ASSERT_FALSE(res);
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"b"}, {"c"}}));
}

TEST(json, test_default_val_with_validator_copies_moved_present) {
  const auto res =
      rfl::json::read<MovesAcrossFields>(R"({"a": 5, "b": 6, "c": 7})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->b.value().value(), 6);
  EXPECT_EQ(res->c.value().value(), 7);
}

// Copied in a loop.
struct CopiesArray {
  std::array<Positive, 64> a;
  rfl::DefaultVal<std::array<Positive, 64>> b = a;
};

std::string array_json(const char* _name) {
  std::string json = std::string("\"") + _name + "\": [";
  for (int i = 1; i <= 64; ++i) {
    json += std::to_string(i) + (i < 64 ? "," : "]");
  }
  return json;
}

TEST(json, test_default_val_with_validator_copies_array) {
  const auto res = rfl::json::read<CopiesArray>("{" + array_json("a") + "}");
  ASSERT_FALSE(res);
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"b", size_t{0}}}));
  const auto res2 = rfl::json::read<CopiesArray>("{" + array_json("a") + "," +
                                                 array_json("b") + "}");
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_EQ(res2->b.value()[63].value(), 64);
}

// Copied to the heap.
struct CopiesToHeap {
  Positive a;
  rfl::DefaultVal<std::vector<Positive>> v = std::vector<Positive>({a});
  rfl::DefaultVal<std::optional<rfl::Box<Positive>>> box =
      rfl::make_box<Positive>(a);
};

TEST(json, test_default_val_with_validator_copies_to_heap) {
  const auto res =
      rfl::json::read<CopiesToHeap>(R"({"a": 5, "v": [1], "box": 2})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->v.value().at(0).value(), 1);
  const auto res2 = rfl::json::read<CopiesToHeap>(R"({"a": 5})");
  ASSERT_FALSE(res2);
  EXPECT_EQ(res2.error().what(),
            "Found 2 errors:\n"
            "1) Field named 'v' not found, and its default value is invalid at "
            "'v[0]': Value expected to be greater than or equal to 1, but got "
            "0.\n"
            "2) Field named 'box' not found, and its default value is invalid: "
            "Value expected to be greater than or equal to 1, but got 0.");
  EXPECT_EQ(paths(res2.error()),
            std::vector<Path>({{"v", size_t{0}}, {"box"}}));
}

// A struct read inside another struct's default member initializer can be
// constructed normally, but copy the other struct's unchecked default.
Positive* outer_a = nullptr;

struct CopiesFromOuter {
  rfl::DefaultVal<int> x;
  rfl::DefaultVal<Positive> b = *outer_a;
};

struct ReadsCopyOfOwnDefault {
  rfl::DefaultVal<int> x;
  Positive a;
  rfl::DefaultVal<bool> inner_ok = [this] {
    outer_a = &a;
    return bool(rfl::json::read<CopiesFromOuter>(R"({})"));
  }();
};

TEST(json, test_default_val_with_validator_copies_from_outer_struct) {
  const auto res = rfl::json::read<ReadsCopyOfOwnDefault>(R"({"a": 5})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_FALSE(res->inner_ok.value());
}

// A class with a custom reflection type is opaque to the check, but the
// Validators constructed inside it were recorded.
class Opaque {
 public:
  using ReflectionType = int;
  Opaque() = default;
  Opaque(int _v) : v_(_v) {}
  int reflection() const { return v_.value(); }

 private:
  Positive v_;
};

struct WithOpaque {
  rfl::DefaultVal<int> x;
  Opaque opaque;
};

TEST(json, test_default_val_with_validator_copies_opaque) {
  const auto res = rfl::json::read<WithOpaque>(R"({"opaque": 3})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->opaque.reflection(), 3);
  const auto res2 = rfl::json::read<WithOpaque, rfl::DefaultIfMissing>(R"({})");
  ASSERT_FALSE(res2);
  EXPECT_EQ(res2.error().what(),
            "Field named 'opaque' not found, and its default value is invalid: "
            "Value expected to be greater than or equal to 1, but got 0.");
}

// The first read of a struct that cannot be default-constructed finds that
// out, and later reads defer the check right away. Both give the same result.
struct ReadTwice {
  rfl::DefaultVal<int> x;
  Positive p;
};

TEST(json, test_default_val_with_validator_copies_read_twice) {
  for (int i = 0; i < 2; ++i) {
    const auto res = rfl::json::read<ReadTwice>(R"({"p": 4})");
    ASSERT_TRUE(res) << res.error().what();
    EXPECT_EQ(res->p.value(), 4);
    const auto res2 = rfl::json::read<ReadTwice, rfl::DefaultIfMissing>("{}");
    ASSERT_FALSE(res2);
    EXPECT_EQ(paths(res2.error()), std::vector<Path>({{"p"}}));
  }
}

// A read inside a default member initializer.
struct ReadsInDefault {
  rfl::DefaultVal<int> x;
  Foods own;
  Person nested =
      rfl::json::read<Person>(R"({"favorite_foods": ["from-default"]})")
          .value();
};

TEST(json, test_default_val_with_validator_copies_read_in_default) {
  const auto res = rfl::json::read<ReadsInDefault, rfl::DefaultIfMissing>(
      R"({"own": ["x"]})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->nested.favorite_foods->at(0), "from-default");
  EXPECT_EQ(res->own->at(0), "x");
}

struct Node {
  rfl::DefaultVal<int> weight = 1;
  rfl::Validator<std::string, rfl::Size<rfl::Minimum<1>>> name;
  std::vector<Node> children;
};

TEST(json, test_default_val_with_validator_copies_recursive) {
  const auto res = rfl::json::read<Node>(
      R"({"name": "root", "children": [{"name": "a", "children": []}, {"name": "b", "children": [{"name": "c", "children": []}]}]})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->children[1].children[0].name.value(), "c");
  EXPECT_EQ(res->children[1].children[0].weight.value(), 1);
  const auto res2 = rfl::json::read<Node>(
      R"({"name": "root", "children": [{"children": []}]})");
  ASSERT_FALSE(res2);
  EXPECT_EQ(paths(res2.error()),
            std::vector<Path>({{"children", size_t{0}, "name"}}));
}

TEST(json, test_default_val_with_validator_copies_threads) {
  std::atomic<int> failures = 0;
  std::vector<std::thread> threads;
  for (int i = 0; i < 8; ++i) {
    threads.emplace_back([&failures, i] {
      for (int j = 0; j < 200; ++j) {
        if (i % 4 == 0) {
          const auto res =
              rfl::json::read<Person>(R"({"favorite_foods": ["x"]})");
          if (!res || res->favorite_foods->at(0) != "x") {
            ++failures;
          }
        } else if (i % 4 == 1) {
          try {
            (void)Foods();
            ++failures;
          } catch (std::exception&) {
          }
        } else if (i % 4 == 2) {
          const auto v = std::vector<Positive>(64, Positive(2));
          auto w = v;
          const auto x = std::move(w);
          if (x.size() != 64 || x[63].value() != 2) {
            ++failures;
          }
        } else {
          const auto res = rfl::json::read<CopiesAcrossFields>(
              R"({"a": 5, "b": 6, "c": 7})");
          if (!res || res->c->value() != 7) {
            ++failures;
          }
          if (rfl::json::read<CopiesAcrossFields>(R"({"a": 5})")) {
            ++failures;
          }
        }
      }
    });
  }
  for (auto& t : threads) {
    t.join();
  }
  EXPECT_EQ(failures.load(), 0);
}

static_assert(std::is_nothrow_copy_constructible_v<Positive>);
static_assert(std::is_nothrow_copy_assignable_v<Positive>);
static_assert(std::is_nothrow_move_constructible_v<Positive>);
static_assert(std::is_nothrow_move_assignable_v<Positive>);
static_assert(std::is_nothrow_move_constructible_v<Foods>);
static_assert(std::is_nothrow_move_assignable_v<Foods>);
static_assert(!std::is_nothrow_copy_constructible_v<Foods>);

// std::is_copy_constructible_v is true for this vector, but copying it does
// not compile, so the default value must not be copied.
using MoveOnly = rfl::Validator<std::vector<std::unique_ptr<int>>,
                                rfl::Size<rfl::Minimum<1>>>;

static_assert(std::is_nothrow_move_constructible_v<MoveOnly>);

struct WithMoveOnly {
  rfl::DefaultVal<int> x;
  MoveOnly v;
};

TEST(json, test_default_val_with_validator_copies_move_only) {
  EXPECT_THROW((void)MoveOnly(), std::exception);
  auto elements = std::vector<std::unique_ptr<int>>();
  elements.push_back(std::make_unique<int>(1));
  auto v = MoveOnly::from_value(std::move(elements)).value();
  const auto w = std::move(v);
  EXPECT_EQ(*w->at(0), 1);
  const auto res = rfl::json::read<WithMoveOnly>(R"({"v": [3]})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(*res->v->at(0), 3);
  EXPECT_FALSE(rfl::json::read<WithMoveOnly>(R"({})"));
}

}  // namespace test_default_val_with_validator_copies
