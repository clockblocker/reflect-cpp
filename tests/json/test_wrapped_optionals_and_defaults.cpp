#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <optional>
#include <stdexcept>
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

TEST(json, test_wrapped_defaults_read_missing_fields) {
  const auto partial = rfl::json::read<Defaults>(R"({"jobs":8})");
  ASSERT_TRUE(partial) << partial.error().what();
  EXPECT_EQ(partial.value().output().value(), "out.csv");
  EXPECT_EQ(partial.value().input().value(), "in.txt");
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

struct OtherAlternative {
  std::string q;
};

TEST(json, test_wrapped_defaults_tagged_union_schema) {
  const auto schema = rfl::json::to_schema<
      rfl::TaggedUnion<"type", WrappedDefaultAlternative, OtherAlternative>>();
  EXPECT_NE(
      schema.find(
          R"("output":{"type":"string","description":"Output file","default":"out.csv"})"),
      std::string::npos)
      << schema;
}

// A wrapped rfl::DefaultVal in a NamedTuple is required: a NamedTuple has no
// in-class initializers.
TEST(json, test_wrapped_defaults_named_tuple_missing_field) {
  using Wrapped = rfl::NamedTuple<
      rfl::Field<"o", rfl::Short<"o", rfl::DefaultVal<std::string>>>,
      rfl::Field<"z", int>>;
  const auto wrapped = rfl::json::read<Wrapped>(R"({"z":1})");
  ASSERT_FALSE(wrapped);
  EXPECT_EQ(wrapped.error().what(), std::string("Field named 'o' not found."));

  const auto present = rfl::json::read<Wrapped>(R"({"o":"x","z":1})");
  ASSERT_TRUE(present) << present.error().what();
  EXPECT_EQ(rfl::get<"o">(present.value()).value().value(), "x");
}

struct NotDefaultConstructible {
  using ReflectionType = int;
  NotDefaultConstructible(int _v) : v_(_v) {}
  int reflection() const { return v_; }
  int v_;
};

// A struct that cannot be default-constructed is read into raw memory, so the
// in-class default of a wrapped rfl::DefaultVal field is not available and the
// field is required.
struct WrappedDefaultAndNoDefaultConstructor {
  rfl::Description<"Worker threads", rfl::DefaultVal<int>> jobs = 4;
  NotDefaultConstructible n;
};

TEST(json, test_wrapped_defaults_not_default_constructible) {
  const auto wrapped = rfl::json::read<WrappedDefaultAndNoDefaultConstructor>(
      R"({"jobs":8,"n":3})");
  ASSERT_TRUE(wrapped) << wrapped.error().what();
  EXPECT_EQ(wrapped.value().jobs().value(), 8);

  const auto missing =
      rfl::json::read<WrappedDefaultAndNoDefaultConstructor>(R"({"n":3})");
  ASSERT_FALSE(missing);
  EXPECT_EQ(missing.error().what(),
            std::string("Field named 'jobs' not found."));
}

// If default construction throws, a struct whose only rfl::DefaultVal fields
// are wrapped is read without its defaults, like a struct that cannot be
// default-constructed, so those fields are required and the schema leaves out
// their defaults. With a bare rfl::DefaultVal, the read fails.
struct ThrowsOnDefault {
  using ReflectionType = int;
  ThrowsOnDefault() { throw std::runtime_error("No default."); }
  ThrowsOnDefault(int _v) : v_(_v) {}
  int reflection() const { return v_; }
  int v_;
};

struct WrappedDefaultAndThrowingDefault {
  rfl::Description<"Worker threads", rfl::DefaultVal<int>> jobs = 4;
  ThrowsOnDefault t;
};

struct BareDefaultAndThrowingDefault {
  rfl::DefaultVal<int> jobs = 4;
  ThrowsOnDefault t;
};

TEST(json, test_wrapped_defaults_read_with_throwing_default) {
  const auto wrapped =
      rfl::json::read<WrappedDefaultAndThrowingDefault>(R"({"jobs":8,"t":3})");
  ASSERT_TRUE(wrapped) << wrapped.error().what();
  EXPECT_EQ(wrapped.value().jobs().value(), 8);
  EXPECT_EQ(wrapped.value().t.v_, 3);

  const auto wrapped_missing =
      rfl::json::read<WrappedDefaultAndThrowingDefault>(R"({"t":3})");
  ASSERT_FALSE(wrapped_missing);
  EXPECT_EQ(wrapped_missing.error().what(),
            std::string("Field named 'jobs' not found."));

  const auto bare =
      rfl::json::read<BareDefaultAndThrowingDefault>(R"({"jobs":8,"t":3})");
  ASSERT_FALSE(bare);
  EXPECT_TRUE(std::string(bare.error().what())
                  .starts_with("Could not default-construct"))
      << bare.error().what();

  const auto schema = rfl::json::to_schema<WrappedDefaultAndThrowingDefault>();
  EXPECT_NE(
      schema.find(R"("jobs":{"type":"integer","description":"Worker threads"})"),
      std::string::npos)
      << schema;
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
// bare one does not compile; a wrapped one does.
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
