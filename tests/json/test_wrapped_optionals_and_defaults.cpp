#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

#include "write_and_read.hpp"

// rfl::Short, rfl::Positional and rfl::Description are transparent for JSON,
// so std::optional and rfl::DefaultVal must behave the same inside them as
// they do on their own: omitted when empty, filled in when missing, and not
// required in the schema.
namespace test_wrapped_optionals_and_defaults {

struct Optionals {
  rfl::Short<"f", std::optional<std::string>> format;
  rfl::Positional<std::optional<std::string>> input;
};

struct NestedOptionals {
  rfl::Description<"Log file", rfl::Short<"l", std::optional<std::string>>> log;
  rfl::Short<"t", rfl::Description<"Title", std::optional<std::string>>> title;
  rfl::Positional<rfl::Description<"Config file", std::optional<std::string>>>
      config;
  rfl::Rename<"outDir", rfl::Positional<std::optional<std::string>>> out_dir;
};

TEST(json, test_wrapped_optionals_write_and_read) {
  write_and_read(Optionals{}, R"({})");
  write_and_read(
      Optionals{.format = std::string("csv"), .input = std::string("in.txt")},
      R"({"format":"csv","input":"in.txt"})");

  write_and_read(NestedOptionals{}, R"({})");
  write_and_read(
      NestedOptionals{.log = std::string("log.txt"),
                      .title = std::string("Report"),
                      .config = std::string("config.json"),
                      .out_dir = std::string("out")},
      R"({"log":"log.txt","title":"Report","config":"config.json","outDir":"out"})");
}

TEST(json, test_wrapped_optionals_schema) {
  EXPECT_EQ(
      rfl::json::to_schema<Optionals>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_wrapped_optionals_and_defaults__Optionals","$defs":{"test_wrapped_optionals_and_defaults__Optionals":{"type":"object","properties":{"format":{"anyOf":[{"type":"string"},{"type":"null"}]},"input":{"anyOf":[{"type":"string"},{"type":"null"}]}},"required":[]}}})");

  EXPECT_EQ(
      rfl::json::to_schema<NestedOptionals>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_wrapped_optionals_and_defaults__NestedOptionals","$defs":{"test_wrapped_optionals_and_defaults__NestedOptionals":{"type":"object","properties":{"log":{"description":"Log file","anyOf":[{"type":"string"},{"type":"null"}]},"title":{"description":"Title","anyOf":[{"type":"string"},{"type":"null"}]},"config":{"description":"Config file","anyOf":[{"type":"string"},{"type":"null"}]},"outDir":{"anyOf":[{"type":"string"},{"type":"null"}]}},"required":[]}}})");
}

struct Defaults {
  rfl::Short<"o", rfl::DefaultVal<std::string>> output = std::string("out.csv");
  rfl::Positional<rfl::DefaultVal<std::string>> input = std::string("in.txt");
  rfl::Description<"Worker threads", rfl::DefaultVal<int>> jobs = 4;
};

struct NestedDefaults {
  rfl::Description<"Log file", rfl::Short<"l", rfl::DefaultVal<std::string>>>
      log = std::string("log.txt");
  rfl::Short<"t", rfl::Description<"Title", rfl::DefaultVal<std::string>>>
      title = std::string("Report");
  rfl::Positional<rfl::Description<"Config file", rfl::DefaultVal<std::string>>>
      config = std::string("config.json");
  rfl::Deprecated<"Use jobs instead.", "Worker threads", rfl::DefaultVal<int>>
      threads = 4;
  rfl::Rename<"outDir", rfl::Short<"d", rfl::DefaultVal<std::string>>> out_dir =
      std::string("out");
};

TEST(json, test_wrapped_defaults_write_and_read) {
  write_and_read(Defaults{},
                 R"({"output":"out.csv","input":"in.txt","jobs":4})");
  write_and_read(Defaults{.output = std::string("x.csv"),
                          .input = std::string("x.txt"),
                          .jobs = 8},
                 R"({"output":"x.csv","input":"x.txt","jobs":8})");

  write_and_read(
      NestedDefaults{},
      R"({"log":"log.txt","title":"Report","config":"config.json","threads":4,"outDir":"out"})");
}

TEST(json, test_wrapped_defaults_read_missing_fields) {
  const auto defaults = rfl::json::read<Defaults>(R"({})");
  ASSERT_TRUE(defaults) << defaults.error().what();
  EXPECT_EQ(defaults.value().output().value(), "out.csv");
  EXPECT_EQ(defaults.value().input().value(), "in.txt");
  EXPECT_EQ(defaults.value().jobs().value(), 4);

  const auto partial = rfl::json::read<Defaults>(R"({"jobs":8})");
  ASSERT_TRUE(partial) << partial.error().what();
  EXPECT_EQ(partial.value().output().value(), "out.csv");
  EXPECT_EQ(partial.value().jobs().value(), 8);

  const auto nested = rfl::json::read<NestedDefaults>(R"({})");
  ASSERT_TRUE(nested) << nested.error().what();
  EXPECT_EQ(nested.value().log().value().value(), "log.txt");
  EXPECT_EQ(nested.value().title().value().value(), "Report");
  EXPECT_EQ(nested.value().config().value().value(), "config.json");
  EXPECT_EQ(nested.value().threads().value(), 4);
  EXPECT_EQ(nested.value().out_dir().value().value(), "out");
}

TEST(json, test_wrapped_defaults_schema) {
  EXPECT_EQ(
      rfl::json::to_schema<Defaults>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_wrapped_optionals_and_defaults__Defaults","$defs":{"test_wrapped_optionals_and_defaults__Defaults":{"type":"object","properties":{"output":{"type":"string","default":"out.csv"},"input":{"type":"string","default":"in.txt"},"jobs":{"type":"integer","description":"Worker threads","default":4}},"required":[]}}})");

  EXPECT_EQ(
      rfl::json::to_schema<NestedDefaults>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_wrapped_optionals_and_defaults__NestedDefaults","$defs":{"test_wrapped_optionals_and_defaults__NestedDefaults":{"type":"object","properties":{"log":{"type":"string","description":"Log file","default":"log.txt"},"title":{"type":"string","description":"Title","default":"Report"},"config":{"type":"string","description":"Config file","default":"config.json"},"threads":{"type":"integer","description":"Worker threads","deprecated":true,"deprecationMessage":"Use jobs instead.","default":4},"outDir":{"type":"string","default":"out"}},"required":[]}}})");
}

// The schema reads the defaults of a TaggedUnion alternative from a
// default-constructed instance of that alternative.
struct WrappedDefaultAlternative {
  rfl::Description<"Output file", rfl::DefaultVal<std::string>> output =
      std::string("out.csv");
  int k = 0;
};

struct BareDefaultAlternative {
  rfl::DefaultVal<std::string> output = std::string("out.csv");
  int k = 0;
};

struct OtherAlternative {
  std::string q;
};

TEST(json, test_wrapped_defaults_tagged_union_schema) {
  EXPECT_EQ(
      (rfl::json::to_schema<rfl::TaggedUnion<"type", WrappedDefaultAlternative,
                                             OtherAlternative>>()),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[{"$ref":"#/$defs/test_wrapped_optionals_and_defaults__WrappedDefaultAlternative__tagged"},{"$ref":"#/$defs/test_wrapped_optionals_and_defaults__OtherAlternative__tagged"}],"$defs":{"test_wrapped_optionals_and_defaults__OtherAlternative__tagged":{"type":"object","properties":{"type":{"type":"string","enum":["OtherAlternative"]},"q":{"type":"string"}},"required":["type","q"]},"test_wrapped_optionals_and_defaults__WrappedDefaultAlternative__tagged":{"type":"object","properties":{"type":{"type":"string","enum":["WrappedDefaultAlternative"]},"output":{"type":"string","description":"Output file","default":"out.csv"},"k":{"type":"integer"}},"required":["type","k"]}}})");

  EXPECT_EQ(
      (rfl::json::to_schema<rfl::TaggedUnion<"type", BareDefaultAlternative,
                                             OtherAlternative>>()),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[{"$ref":"#/$defs/test_wrapped_optionals_and_defaults__BareDefaultAlternative__tagged"},{"$ref":"#/$defs/test_wrapped_optionals_and_defaults__OtherAlternative__tagged"}],"$defs":{"test_wrapped_optionals_and_defaults__BareDefaultAlternative__tagged":{"type":"object","properties":{"type":{"type":"string","enum":["BareDefaultAlternative"]},"output":{"type":"string","default":"out.csv"},"k":{"type":"integer"}},"required":["type","k"]},"test_wrapped_optionals_and_defaults__OtherAlternative__tagged":{"type":"object","properties":{"type":{"type":"string","enum":["OtherAlternative"]},"q":{"type":"string"}},"required":["type","q"]}}})");
}

// Counts every use of a Tracker whose constructor never ran.
struct Tracker {
  static constexpr int alive_ = 0x7ac4e1;
  static inline int unconstructed_uses_ = 0;

  using ReflectionType = int;

  Tracker() = default;
  Tracker(int _v) : v_(_v) {}
  Tracker(const Tracker& _other) : v_(_other.checked()) {}
  Tracker(Tracker&& _other) noexcept : v_(_other.checked()) {}
  ~Tracker() { state_ = 0; }
  Tracker& operator=(const Tracker& _other) {
    v_ = _other.checked();
    return *this;
  }
  Tracker& operator=(Tracker&& _other) noexcept {
    v_ = _other.checked();
    return *this;
  }

  int reflection() const { return checked(); }

  int checked() const {
    if (state_ != alive_) {
      ++unconstructed_uses_;
    }
    return v_;
  }

  int v_ = 0;
  int state_ = alive_;
};

// A NamedTuple has no in-class initializers, so a missing bare rfl::DefaultVal
// field is value-initialized. It is constructed before it is used. A wrapped
// rfl::DefaultVal in a NamedTuple is required.
TEST(json, test_wrapped_defaults_named_tuple_missing_field) {
  Tracker::unconstructed_uses_ = 0;

  using Wrapped = rfl::NamedTuple<
      rfl::Field<"o", rfl::Short<"o", rfl::DefaultVal<Tracker>>>,
      rfl::Field<"z", int>>;
  const auto wrapped = rfl::json::read<Wrapped>(R"({"z":1})");
  ASSERT_FALSE(wrapped);
  EXPECT_EQ(wrapped.error().what(), std::string("Field named 'o' not found."));

  using Bare = rfl::NamedTuple<rfl::Field<"o", rfl::DefaultVal<Tracker>>,
                               rfl::Field<"z", int>>;
  const auto bare = rfl::json::read<Bare>(R"({"z":1})");
  ASSERT_TRUE(bare) << bare.error().what();
  EXPECT_EQ(rfl::get<"o">(bare.value()).value().v_, 0);
  EXPECT_EQ(rfl::get<"z">(bare.value()), 1);

  using Scalar = rfl::NamedTuple<
      rfl::Field<"n", rfl::Description<"Count", rfl::DefaultVal<int>>>,
      rfl::Field<"s", rfl::DefaultVal<std::string>>>;
  const auto scalar = rfl::json::read<Scalar>(R"({"n":3})");
  ASSERT_TRUE(scalar) << scalar.error().what();
  EXPECT_EQ(rfl::get<"n">(scalar.value()).value().value(), 3);
  EXPECT_EQ(rfl::get<"s">(scalar.value()).value(), "");

  const auto present = rfl::json::read<Wrapped>(R"({"o":7,"z":1})");
  ASSERT_TRUE(present) << present.error().what();
  EXPECT_EQ(rfl::get<"o">(present.value()).value().value().v_, 7);

  EXPECT_EQ(Tracker::unconstructed_uses_, 0);
}

struct NoDefaultConstructor {
  using ReflectionType = int;
  NoDefaultConstructor(int _v) : v_(_v) {}
  int reflection() const { return v_; }
  int v_;
};

// If the value cannot be value-initialized, or its constructor throws, the
// missing field is an error.
TEST(json, test_wrapped_defaults_named_tuple_missing_field_without_zero_value) {
  using NoZero =
      rfl::NamedTuple<rfl::Field<"o", rfl::DefaultVal<NoDefaultConstructor>>,
                      rfl::Field<"z", int>>;
  const auto no_zero = rfl::json::read<NoZero>(R"({"z":1})");
  ASSERT_FALSE(no_zero);
  EXPECT_EQ(no_zero.error().what(), std::string("Field named 'o' not found."));

  using Invalid = rfl::NamedTuple<
      rfl::Field<"v", rfl::DefaultVal<rfl::Validator<int, rfl::Minimum<1>>>>,
      rfl::Field<"z", int>>;
  const auto invalid = rfl::json::read<Invalid>(R"({"z":1})");
  ASSERT_FALSE(invalid);
  EXPECT_EQ(invalid.error().what(), std::string("Field named 'v' not found."));

  const auto present = rfl::json::read<Invalid>(R"({"v":3,"z":1})");
  ASSERT_TRUE(present) << present.error().what();
  EXPECT_EQ(rfl::get<"v">(present.value()).value().value(), 3);
}

struct NotDefaultConstructible {
  using ReflectionType = int;
  NotDefaultConstructible(int _v) : v_(_v) {}
  int reflection() const { return v_; }
  int v_;
};

// A struct that cannot be default-constructed is read into raw memory, so the
// in-class default of a rfl::DefaultVal field is not available and the field
// is required.
struct WrappedDefaultAndNoDefaultConstructor {
  rfl::Description<"Worker threads", rfl::DefaultVal<int>> jobs = 4;
  NotDefaultConstructible n;
};

struct BareDefaultAndNoDefaultConstructor {
  rfl::DefaultVal<int> jobs = 4;
  NotDefaultConstructible n;
};

TEST(json, test_wrapped_defaults_not_default_constructible) {
  const auto wrapped = rfl::json::read<WrappedDefaultAndNoDefaultConstructor>(
      R"({"jobs":8,"n":3})");
  ASSERT_TRUE(wrapped) << wrapped.error().what();
  EXPECT_EQ(wrapped.value().jobs().value(), 8);
  EXPECT_EQ(wrapped.value().n.v_, 3);

  const auto wrapped_missing =
      rfl::json::read<WrappedDefaultAndNoDefaultConstructor>(R"({"n":3})");
  ASSERT_FALSE(wrapped_missing);
  EXPECT_EQ(wrapped_missing.error().what(),
            std::string("Field named 'jobs' not found."));

  const auto bare = rfl::json::read<BareDefaultAndNoDefaultConstructor>(
      R"({"jobs":8,"n":3})");
  ASSERT_TRUE(bare) << bare.error().what();
  EXPECT_EQ(bare.value().jobs.value(), 8);

  EXPECT_EQ(
      rfl::json::to_schema<WrappedDefaultAndNoDefaultConstructor>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_wrapped_optionals_and_defaults__WrappedDefaultAndNoDefaultConstructor","$defs":{"test_wrapped_optionals_and_defaults__NotDefaultConstructible":{"type":"integer"},"test_wrapped_optionals_and_defaults__WrappedDefaultAndNoDefaultConstructor":{"type":"object","properties":{"jobs":{"type":"integer","description":"Worker threads"},"n":{"$ref":"#/$defs/test_wrapped_optionals_and_defaults__NotDefaultConstructible"}},"required":["n"]}}})");
}

// The schema leaves out the default values if the struct cannot be
// default-constructed because a validator rejects a field's default value.
struct WrappedDefaultAndInvalidDefault {
  rfl::Description<"Worker threads", rfl::DefaultVal<int>> jobs = 4;
  rfl::Validator<int, rfl::Minimum<1>> n;
};

struct BareDefaultAndInvalidDefault {
  rfl::DefaultVal<int> jobs = 4;
  rfl::Validator<int, rfl::Minimum<1>> n;
};

TEST(json, test_wrapped_defaults_schema_with_invalid_default) {
  EXPECT_EQ(
      rfl::json::to_schema<WrappedDefaultAndInvalidDefault>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_wrapped_optionals_and_defaults__WrappedDefaultAndInvalidDefault","$defs":{"test_wrapped_optionals_and_defaults__WrappedDefaultAndInvalidDefault":{"type":"object","properties":{"jobs":{"type":"integer","description":"Worker threads"},"n":{"minimum":1,"type":"integer"}},"required":["n"]}}})");

  EXPECT_EQ(
      rfl::json::to_schema<BareDefaultAndInvalidDefault>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_wrapped_optionals_and_defaults__BareDefaultAndInvalidDefault","$defs":{"test_wrapped_optionals_and_defaults__BareDefaultAndInvalidDefault":{"type":"object","properties":{"jobs":{"type":"integer"},"n":{"minimum":1,"type":"integer"}},"required":["n"]}}})");
}

// If default construction throws, a struct whose only rfl::DefaultVal fields
// are wrapped is read into raw memory, so those fields are required. With a
// bare rfl::DefaultVal, the read fails.
TEST(json, test_wrapped_defaults_read_with_invalid_default) {
  const auto wrapped =
      rfl::json::read<WrappedDefaultAndInvalidDefault>(R"({"jobs":8,"n":3})");
  ASSERT_TRUE(wrapped) << wrapped.error().what();
  EXPECT_EQ(wrapped.value().jobs().value(), 8);
  EXPECT_EQ(wrapped.value().n.value(), 3);

  const auto wrapped_missing =
      rfl::json::read<WrappedDefaultAndInvalidDefault>(R"({"n":3})");
  ASSERT_FALSE(wrapped_missing);
  EXPECT_EQ(wrapped_missing.error().what(),
            std::string("Field named 'jobs' not found."));

  const auto bare =
      rfl::json::read<BareDefaultAndInvalidDefault>(R"({"jobs":8,"n":3})");
  ASSERT_FALSE(bare);
  EXPECT_TRUE(std::string(bare.error().what())
                  .starts_with("Could not default-construct"))
      << bare.error().what();
}

// Like a struct with a bare rfl::DefaultVal, a struct with a wrapped one is
// default-constructed before it is read, so the in-class defaults of its other
// fields survive a read too.
struct WrappedDefaultAndOptional {
  rfl::Short<"j", rfl::DefaultVal<int>> jobs = 4;
  std::optional<int> retries = 5;
};

TEST(json, test_wrapped_defaults_keep_sibling_defaults) {
  const auto res = rfl::json::read<WrappedDefaultAndOptional>(R"({})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res.value().jobs().value(), 4);
  EXPECT_EQ(res.value().retries, 5);
}

// A struct with an atomic field is read through a NamedTuple, which has no
// in-class initializers, so a wrapped rfl::DefaultVal there is required. A
// bare one does not compile.
struct AtomicAndWrappedDefault {
  std::atomic<int> counter;
  rfl::Description<"d", rfl::DefaultVal<int>> x = 1;
};

template <class P>
void check_atomic_and_wrapped_default() {
  const auto res = rfl::json::read<P>(R"({"counter":3,"x":2})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ((*res.value()).counter.load(), 3);
  EXPECT_EQ((*res.value()).x().value(), 2);

  const auto missing = rfl::json::read<P>(R"({"counter":3})");
  ASSERT_FALSE(missing);
  EXPECT_EQ(missing.error().what(), std::string("Field named 'x' not found."));
}

TEST(json, test_wrapped_defaults_atomic) {
  check_atomic_and_wrapped_default<rfl::Box<AtomicAndWrappedDefault>>();
  check_atomic_and_wrapped_default<rfl::Ref<AtomicAndWrappedDefault>>();
  check_atomic_and_wrapped_default<std::unique_ptr<AtomicAndWrappedDefault>>();
  check_atomic_and_wrapped_default<std::shared_ptr<AtomicAndWrappedDefault>>();
}

}  // namespace test_wrapped_optionals_and_defaults
