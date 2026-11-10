#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

// How the fields that the input does not contain are checked after a struct
// was default-constructed with the check of invalid default values deferred.

namespace test_default_val_with_validator_walk {

using Path = std::vector<rfl::ErrorDetail::PathSegment>;

std::vector<Path> paths(const rfl::Error& _err) {
  std::vector<Path> result;
  for (const auto& d : _err.details()) {
    result.push_back(d.path);
  }
  return result;
}

using Positive = rfl::Validator<int, rfl::Minimum<1>>;
using Name = rfl::Validator<std::string, rfl::Size<rfl::Minimum<1>>>;
using Foods =
    rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>>;

using JsonReader = rfl::json::Reader;
using JsonWriter = rfl::json::Writer;

template <class T>
constexpr bool may_contain_validator =
    rfl::parsing::missing_defaults::may_contain_validator_v<
        JsonReader, JsonWriter, rfl::Processors<>, T>;

// rfl::ExtraFields is read as a map, not as a struct.
struct ExtraNoValidator {
  rfl::DefaultVal<std::string> name = "x";
  rfl::ExtraFields<rfl::Generic> extra;
};

struct ExtraWithValidator {
  rfl::DefaultVal<std::string> name = "x";
  Positive p;
  rfl::ExtraFields<int> extra;
  std::optional<rfl::ExtraFields<int>> more;
};

TEST(json, test_default_val_with_validator_walk_extra_fields) {
  const auto res = rfl::json::read<ExtraNoValidator>(R"({"name":"a","foo":1})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->extra.size(), 1);
  const auto res2 =
      rfl::json::read<ExtraWithValidator>(R"({"name":"a","p":2,"foo":1})");
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_EQ(res2->extra.get("foo").value(), 1);
  EXPECT_FALSE(rfl::json::read<ExtraWithValidator>(R"({"name":"a"})"));
}

// An aggregate with its own Parser is not decomposed.
struct Tags : std::vector<std::string> {};

}  // namespace test_default_val_with_validator_walk

namespace rfl::parsing {

template <class R, class W, class P>
  requires AreReaderAndWriter<R, W, test_default_val_with_validator_walk::Tags>
struct Parser<R, W, test_default_val_with_validator_walk::Tags, P> {
  using Tags = test_default_val_with_validator_walk::Tags;
  using InputVarType = typename R::InputVarType;
  using Vec = std::vector<std::string>;

  static Result<Tags> read(const R& _r, const InputVarType& _var) noexcept {
    return Parser<R, W, Vec, P>::read(_r, _var).transform([](Vec&& _v) {
      Tags t;
      static_cast<Vec&>(t) = std::move(_v);
      return t;
    });
  }

  template <class Parent>
  static void write(const W& _w, const Tags& _tags, const Parent& _parent) {
    Parser<R, W, Vec, P>::write(_w, static_cast<const Vec&>(_tags), _parent);
  }

  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    return Parser<R, W, Vec, P>::to_schema(_definitions);
  }
};

}  // namespace rfl::parsing

namespace test_default_val_with_validator_walk {

struct WithTags {
  rfl::DefaultVal<int> x;
  Positive p;
  Tags tags;
};

TEST(json, test_default_val_with_validator_walk_custom_parser) {
  const auto res = rfl::json::read<WithTags>(R"({"p":1,"tags":["a"]})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->tags.at(0), "a");
  const auto res2 =
      rfl::json::read<WithTags, rfl::DefaultIfMissing>(R"({"p":1})");
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_TRUE(res2->tags.empty());
}

// Structs without a Validator are read exactly as before.
struct Plain {
  rfl::DefaultVal<int> x;
  std::vector<bool> flags;
  std::shared_ptr<Positive> shared;
  rfl::ExtraFields<rfl::Generic> extra;
};

static_assert(!may_contain_validator<Plain>);
static_assert(may_contain_validator<WithTags>);
static_assert(may_contain_validator<std::vector<std::map<int, Positive>>>);

TEST(json, test_default_val_with_validator_walk_no_validator) {
  const auto res = rfl::json::read<Plain>(R"({"flags":[true,false]})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->flags.size(), 2);
}

// Only an invalid Validator default makes the parser defer. Another
// exception, even one thrown after a Validator's exception was caught, is
// returned as it is, and does not change how later reads construct T.
Name default_host() {
  try {
    return Name();
  } catch (std::exception&) {
    return Name("localhost");
  }
}

bool cert_missing = true;
int cert_loads = 0;

std::string load_cert() {
  ++cert_loads;
  if (cert_missing) {
    throw std::runtime_error("cert.pem not found");
  }
  return "CERT";
}

struct Config {
  Name host = default_host();
  std::string cert = load_cert();
  int port = 80;
};

TEST(json, test_default_val_with_validator_walk_other_exception) {
  const auto res =
      rfl::json::read<Config, rfl::DefaultIfMissing>(R"({"port":1})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(), "cert.pem not found");
  EXPECT_EQ(cert_loads, 1);
  cert_missing = false;
  for (int i = 0; i < 2; ++i) {
    const auto res2 =
        rfl::json::read<Config, rfl::DefaultIfMissing>(R"({"port":1})");
    ASSERT_TRUE(res2) << res2.error().what();
    EXPECT_EQ(res2->host.value(), "localhost");
  }
  EXPECT_EQ(cert_loads, 3);
}

// The exception of an invalid default is still what Result::value() throws.
TEST(json, test_default_val_with_validator_walk_exception_type) {
#ifdef REFLECTCPP_USE_STD_EXPECTED
  EXPECT_THROW((void)Positive(), std::bad_expected_access<rfl::Error>);
#else
  EXPECT_THROW((void)Positive(), std::runtime_error);
#endif
  try {
    (void)Positive();
  } catch (std::exception& e) {
    EXPECT_STREQ(e.what(),
                 "Value expected to be greater than or equal to 1, but got 0.");
  }
}

// The check does not touch objects that the result shares with other code.
struct Shared {
  Foods foods = std::vector<std::string>({"a", "b"});
};

std::shared_ptr<Shared> shared_default = std::make_shared<Shared>();

struct WithShared {
  Positive p;
  std::shared_ptr<Shared> shared = shared_default;
  rfl::Ref<Shared> ref = rfl::Ref<Shared>::make();
};

struct Node {
  int v = 1;
  std::shared_ptr<Node> next;
};

std::shared_ptr<Node> ring() {
  auto n = std::make_shared<Node>();
  n->next = n;
  return n;
}

struct WithCycle {
  Positive p;
  std::shared_ptr<Node> ring_ = ring();
  ~WithCycle() {
    if (ring_) {
      ring_->next.reset();
    }
  }
};

TEST(json, test_default_val_with_validator_walk_shared) {
  const auto* before = shared_default->foods.value().data();
  const auto res =
      rfl::json::read<WithShared, rfl::DefaultIfMissing>(R"({"p":3})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->shared.get(), shared_default.get());
  EXPECT_EQ(shared_default->foods.value().data(), before);
  EXPECT_EQ(shared_default->foods.value().size(), 2);
  const auto res2 =
      rfl::json::read<WithCycle, rfl::DefaultIfMissing>(R"({"p":3})");
  ASSERT_TRUE(res2) << res2.error().what();
}

// Validating a missing field leaves its value as it was.
struct WithCopies {
  Positive p;
  Foods foods = std::vector<std::string>({"x"});
  std::vector<Foods> more = {foods, foods};
};

TEST(json, test_default_val_with_validator_walk_values_kept) {
  const auto res =
      rfl::json::read<WithCopies, rfl::DefaultIfMissing>(R"({"p":3})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->foods.value(), std::vector<std::string>({"x"}));
  EXPECT_EQ(res->more.at(1).value(), std::vector<std::string>({"x"}));
}

// A class with a custom reflection type is not looked into, even if it gives
// access to part of its state.
struct Range {
  using ReflectionType = int;
  Range() = default;
  Range(int _lo) : lo_(_lo), hi_(1) {}
  int reflection() const { return lo_; }
  int& get() { return lo_; }
  const int& get() const { return lo_; }
  int lo_ = 0;
  Positive hi_;
};

struct WithRange {
  rfl::DefaultVal<int> x;
  Range range;
};

// It may replace a Validator it holds while it is constructed. Its field is
// then reported with the error of that Validator's default value, without
// looking at the memory again.
struct Replaces {
  using ReflectionType = std::string;
  Replaces() { v = std::string(30, 's'); }
  Replaces(const std::string& _s) : v(_s) {}
  std::string reflection() const { return std::get<std::string>(v); }
  std::variant<Foods, std::string> v;
};

struct WithReplaces {
  Positive p;
  Replaces replaces;
};

TEST(json, test_default_val_with_validator_walk_opaque) {
  const auto res =
      rfl::json::read<WithRange, rfl::DefaultIfMissing>(R"({"x":1})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Field named 'range' not found, and its default value is invalid: "
            "Value expected to be greater than or equal to 1, but got 0.");
  const auto res2 = rfl::json::read<WithRange>(R"({"x":1,"range":4})");
  ASSERT_TRUE(res2) << res2.error().what();
  const auto res3 =
      rfl::json::read<WithReplaces, rfl::DefaultIfMissing>(R"({"p":3})");
  ASSERT_FALSE(res3);
  EXPECT_EQ(paths(res3.error()), std::vector<Path>({{"replaces"}}));
  const auto res4 = rfl::json::read<WithReplaces, rfl::DefaultIfMissing>(
      R"({"p":3,"replaces":"r"})");
  ASSERT_TRUE(res4) << res4.error().what();
  EXPECT_EQ(res4->replaces.reflection(), "r");
}

// rfl::Object, maps and variants whose first alternative is a Validator.
struct In {
  Positive p;
};

struct WithMaps {
  rfl::DefaultVal<int> x;
  Positive a;
  rfl::Object<Positive> obj = [this] {
    rfl::Object<Positive> o;
    o.insert(std::string("k"), Positive(a));
    return o;
  }();
  std::map<std::string, In> m = {{"key", In{a}}};
  std::map<int, In> by_int = {{7, In{a}}};
};

struct Other {
  int n = 0;
};

struct WithVariants {
  rfl::DefaultVal<int> x;
  rfl::Variant<Positive, int> v;
  rfl::TaggedUnion<"t", In, Other> tu;
};

TEST(json, test_default_val_with_validator_walk_maps_and_variants) {
  const auto res =
      rfl::json::read<WithMaps, rfl::DefaultIfMissing>(R"({"x":1,"a":5})");
  ASSERT_FALSE(res);
  EXPECT_EQ(paths(res.error()),
            std::vector<Path>(
                {{"obj", "k"}, {"m", "key", "p"}, {"by_int", "7", "p"}}));
  const auto res2 = rfl::json::read<WithMaps>(
      R"({"x":1,"a":5,"obj":{"k":1},"m":{"key":{"p":2}},"by_int":{"7":{"p":3}}})");
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_EQ(res2->by_int.at(7).p.value(), 3);
  const auto res3 =
      rfl::json::read<WithVariants, rfl::DefaultIfMissing>(R"({"x":1})");
  ASSERT_FALSE(res3);
  EXPECT_EQ(paths(res3.error()), std::vector<Path>({{"v"}, {"tu", "p"}}));
  const auto res4 =
      rfl::json::read<WithVariants>(R"({"x":1,"v":2,"tu":{"t":"In","p":3}})");
  ASSERT_TRUE(res4) << res4.error().what();
  EXPECT_EQ(rfl::get<Positive>(res4->v).value(), 2);
}

}  // namespace test_default_val_with_validator_walk
