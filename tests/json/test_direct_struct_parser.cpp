#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace test_direct_struct_parser {

struct PlainStruct {
  int a = 1;
  std::string b = "hello";
  std::optional<double> c = std::nullopt;
};

struct EmptyStruct {};

struct FlattenedStruct {
  rfl::Flatten<PlainStruct> inner;
  int extra = 0;
};

struct ExtraFieldsStruct {
  int id = 0;
  rfl::ExtraFields<rfl::Generic> extras;
};

struct ExplicitFieldStruct {
  rfl::Field<"x", int> x = 1;
};

struct CustomProcessor {
  template <class T, class NT>
  static auto process(NT&& _nt) {
    return std::forward<NT>(_nt);
  }
};

// Eligibility static_asserts
static_assert(rfl::parsing::can_use_direct_struct_parser_v<
              PlainStruct, rfl::Processors<>>);
static_assert(rfl::parsing::can_use_direct_struct_parser_v<
              PlainStruct,
              rfl::Processors<rfl::DefaultIfMissing, rfl::NoExtraFields,
                              rfl::FailFast, rfl::UnderlyingEnums>>);
static_assert(rfl::parsing::can_use_direct_struct_parser_v<
              PlainStruct,
              rfl::Processors<rfl::NoFieldNames, rfl::NoOptionals>>);
static_assert(!rfl::parsing::can_use_direct_struct_parser_v<
              EmptyStruct, rfl::Processors<>>);
static_assert(!rfl::parsing::can_use_direct_struct_parser_v<
              FlattenedStruct, rfl::Processors<>>);
static_assert(!rfl::parsing::can_use_direct_struct_parser_v<
              ExtraFieldsStruct, rfl::Processors<>>);
static_assert(!rfl::parsing::can_use_direct_struct_parser_v<
              ExplicitFieldStruct, rfl::Processors<>>);
static_assert(!rfl::parsing::can_use_direct_struct_parser_v<
              PlainStruct, rfl::Processors<rfl::SnakeCaseToCamelCase>>);
static_assert(!rfl::parsing::can_use_direct_struct_parser_v<
              PlainStruct, rfl::Processors<rfl::AddStructName<"type">>>);
static_assert(!rfl::parsing::can_use_direct_struct_parser_v<
              PlainStruct, rfl::Processors<CustomProcessor>>);

// Verify DirectFieldNames null-termination and binary-search (>8 fields)
struct WideStruct {
  int f01 = 1;
  int f02 = 2;
  int f03 = 3;
  int f04 = 4;
  int f05 = 5;
  int f06 = 6;
  int f07 = 7;
  int f08 = 8;
  rfl::Rename<"custom_f09", int> f09 = 9;
  int f10 = 10;
  std::string f11 = "eleven";
  bool f12 = true;
};

static_assert(rfl::parsing::can_use_direct_struct_parser_v<
              WideStruct, rfl::Processors<>>);
static_assert(rfl::internal::DirectFieldNames<WideStruct>::names_[8] ==
              "custom_f09");
static_assert(
    rfl::internal::DirectFieldNames<WideStruct>::names_[8].data()
        [rfl::internal::DirectFieldNames<WideStruct>::names_[8].size()] == '\0');
static_assert(
    rfl::internal::DirectFieldNames<WideStruct>::find("custom_f09") == 8);
static_assert(rfl::internal::DirectFieldNames<WideStruct>::find("f09") == -1);
static_assert(rfl::internal::DirectFieldNames<WideStruct>::find("f12") == 11);

// Lifetime-tracked move-only non-default-constructible type
struct LiveCounter {
  static inline std::atomic<int> live{0};
  static inline std::atomic<int> constructed{0};
  static inline std::atomic<int> destroyed{0};

  static void reset() {
    live.store(0);
    constructed.store(0);
    destroyed.store(0);
  }
};

struct Tracked {
  using ReflectionType = std::string;

  std::string value;
  bool alive = true;

  Tracked() = delete;
  Tracked(const Tracked&) = delete;
  Tracked& operator=(const Tracked&) = delete;

  explicit Tracked(std::string _v) : value(std::move(_v)), alive(true) {
    if (value == "__THROW__") {
      throw std::runtime_error("Tracked intentional throw");
    }
    LiveCounter::live.fetch_add(1);
    LiveCounter::constructed.fetch_add(1);
  }

  Tracked(Tracked&& _other) noexcept
      : value(std::move(_other.value)), alive(true) {
    LiveCounter::live.fetch_add(1);
    LiveCounter::constructed.fetch_add(1);
  }

  Tracked& operator=(Tracked&& _other) noexcept {
    if (this != &_other) {
      value = std::move(_other.value);
    }
    return *this;
  }

  ~Tracked() {
    EXPECT_TRUE(alive) << "Double destruction detected!";
    alive = false;
    LiveCounter::live.fetch_sub(1);
    LiveCounter::destroyed.fetch_add(1);
  }

  const std::string& reflection() const { return value; }
};

struct TenFieldTracked {
  Tracked t0;
  int n0;
  Tracked t1;
  std::string s1;
  Tracked t2;
  int n2;
  Tracked t3;
  std::optional<std::string> opt;
  Tracked t4;
  Tracked t5;
};

static_assert(rfl::parsing::can_use_direct_struct_parser_v<
              TenFieldTracked, rfl::Processors<>>);

struct MoveOnlyConstAndArrays {
  const int id = 7;
  const std::string tag = "default_tag";
  std::unique_ptr<int> ptr;
  std::array<int, 3> nums = {1, 2, 3};
  std::array<std::string, 2> words = {"a", "b"};
};

static_assert(rfl::parsing::can_use_direct_struct_parser_v<
              MoveOnlyConstAndArrays, rfl::Processors<>>);

struct RenamedSkipAndDefaults {
  rfl::Rename<"wire_name", std::string> cpp_name;
  rfl::DefaultVal<int> count = 42;
  rfl::Skip<int> skipped = 99;
  std::optional<std::string> note;
};

struct MetaDocStruct {
  int port = 8080;
  std::string host = "localhost";
  rfl::DefaultVal<bool> tls = true;
};

}  // namespace test_direct_struct_parser

template <>
struct rfl::Metadata<test_direct_struct_parser::MetaDocStruct> {
  static constexpr std::string_view description = "Server endpoint settings";
  static constexpr std::array<rfl::FieldMeta, 2> fields = {{
      {"port", "TCP listen port"},
      {"host", "Bind address", "Use bind_addr instead"},
  }};
};

namespace test_direct_struct_parser {

TEST(json, test_direct_struct_wide_binary_search_and_rename) {
  const std::string input =
      R"({"f12":false,"custom_f09":99,"f01":11,"f02":12,"f03":13,"f04":14,"f05":15,"f06":16,"f07":17,"f08":18,"f10":20,"f11":"hi"})";
  const auto res = rfl::json::read<WideStruct, rfl::NoExtraFields>(input);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->f01, 11);
  EXPECT_EQ(res->f09.value(), 99);
  EXPECT_EQ(res->f11, "hi");
  EXPECT_FALSE(res->f12);

  const auto written = rfl::json::write(*res);
  EXPECT_NE(written.find(R"("custom_f09":99)"), std::string::npos);

  const auto extra = rfl::json::read<WideStruct, rfl::NoExtraFields>(
      R"({"f01":1,"f02":2,"f03":3,"f04":4,"f05":5,"f06":6,"f07":7,"f08":8,"custom_f09":9,"f10":10,"f11":"x","f12":true,"bogus":1})");
  ASSERT_FALSE(extra.has_value());
  EXPECT_NE(extra.error().what().find("Value named 'bogus' not used."),
            std::string::npos);
}

TEST(json, test_direct_struct_tracked_lifetime_and_partial_destruction) {
  LiveCounter::reset();
  {
    const std::string ok =
        R"({"t0":"a","n0":1,"t1":"b","s1":"x","t2":"c","n2":2,"t3":"d","opt":"y","t4":"e","t5":"f"})";
    const auto res = rfl::json::read<TenFieldTracked>(ok);
    ASSERT_TRUE(res.has_value()) << res.error().what();
    EXPECT_EQ(res->t0.value, "a");
    EXPECT_EQ(res->t5.value, "f");
    const auto rt = rfl::json::read<TenFieldTracked>(rfl::json::write(*res));
    ASSERT_TRUE(rt.has_value()) << rt.error().what();
    EXPECT_EQ(rt->t3.value, "d");
  }
  EXPECT_EQ(LiveCounter::live.load(), 0);
  EXPECT_EQ(LiveCounter::constructed.load(), LiveCounter::destroyed.load());

  const std::vector<std::string> bad_inputs = {
      R"({"t0":123,"n0":1,"t1":"b","s1":"x","t2":"c","n2":2,"t3":"d","t4":"e","t5":"f"})",
      R"({"t0":"a","n0":1,"t1":"b","s1":"x","t2":"c","n2":"bad","t3":"d","t4":"e","t5":"f"})",
      R"({"t5":"f","t4":"e","t3":"d","t2":"c","t1":"b","t0":"a","n2":2,"s1":"x","n0":"bad"})",
      R"({"t0":"a","n0":1,"t1":"b","s1":"x","t2":"__THROW__","n2":2,"t3":"d","t4":"e","t5":"f"})",
      R"({"t0":"a","n0":1,"t1":"b","s1":"x","t2":"c","n2":2,"t3":"d","t4":"e"})",
      R"([1,2,3])",
  };
  for (const auto& input : bad_inputs) {
    LiveCounter::reset();
    {
      const auto r1 = rfl::json::read<TenFieldTracked>(input);
      EXPECT_FALSE(r1.has_value());
    }
    EXPECT_EQ(LiveCounter::live.load(), 0);
    EXPECT_EQ(LiveCounter::constructed.load(), LiveCounter::destroyed.load());

    LiveCounter::reset();
    {
      const auto r2 = rfl::json::read<TenFieldTracked, rfl::FailFast>(input);
      EXPECT_FALSE(r2.has_value());
    }
    EXPECT_EQ(LiveCounter::live.load(), 0);
    EXPECT_EQ(LiveCounter::constructed.load(), LiveCounter::destroyed.load());
  }
}

TEST(json, test_direct_struct_const_move_only_and_arrays) {
  const std::string json =
      R"({"id":99,"tag":"custom","ptr":123,"nums":[10,20,30],"words":["hi","yo"]})";
  const auto res = rfl::json::read<MoveOnlyConstAndArrays>(json);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->id, 99);
  EXPECT_EQ(res->tag, "custom");
  ASSERT_NE(res->ptr, nullptr);
  EXPECT_EQ(*res->ptr, 123);
  EXPECT_EQ(res->nums[0], 10);
  EXPECT_EQ(res->nums[2], 30);
  EXPECT_EQ(res->words[0], "hi");
  EXPECT_EQ(res->words[1], "yo");
  EXPECT_EQ(rfl::json::write(*res), json);

  const auto res_def =
      rfl::json::read<MoveOnlyConstAndArrays, rfl::DefaultIfMissing>(
          R"({"id":55,"words":["x","y"]})");
  ASSERT_TRUE(res_def.has_value()) << res_def.error().what();
  EXPECT_EQ(res_def->id, 55);
  EXPECT_EQ(res_def->tag, "default_tag");
  EXPECT_EQ(res_def->ptr, nullptr);
  EXPECT_EQ(res_def->nums[1], 2);
  EXPECT_EQ(res_def->words[1], "y");
}

TEST(json, test_direct_struct_defaults_skip_no_field_names_and_metadata) {
  const auto res =
      rfl::json::read<RenamedSkipAndDefaults>(R"({"wire_name":"ok"})");
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->cpp_name.value(), "ok");
  EXPECT_EQ(res->count.value(), 42);
  EXPECT_EQ(res->skipped.value(), 99);

  const auto missing_req = rfl::json::read<RenamedSkipAndDefaults>(R"({})");
  ASSERT_FALSE(missing_req.has_value());

  // NoFieldNames array read/write & excess-field error
  const auto arr_json = rfl::json::write<rfl::NoFieldNames>(PlainStruct{3, "z", 1.5});
  EXPECT_EQ(arr_json, R"([3,"z",1.5])");
  const auto arr_rt = rfl::json::read<PlainStruct, rfl::NoFieldNames>(arr_json);
  ASSERT_TRUE(arr_rt.has_value()) << arr_rt.error().what();
  EXPECT_EQ(arr_rt->a, 3);
  EXPECT_EQ(arr_rt->b, "z");
  EXPECT_EQ(arr_rt->c, std::optional<double>(1.5));

  const auto arr_excess =
      rfl::json::read<PlainStruct, rfl::NoFieldNames>(R"([1,"a",2.0,"extra"])");
  ASSERT_FALSE(arr_excess.has_value());
  EXPECT_NE(arr_excess.error().what().find(
                "Expected a maximum of 3 fields, but got at least one more."),
            std::string::npos);

  // Metadata + DefaultVal + NoExtraFields schema
  const auto schema = rfl::json::to_schema<MetaDocStruct, rfl::NoExtraFields>();
  EXPECT_NE(schema.find("Server endpoint settings"), std::string::npos);
  EXPECT_NE(schema.find("TCP listen port"), std::string::npos);
  EXPECT_NE(schema.find("Use bind_addr instead"), std::string::npos);
  EXPECT_NE(schema.find(R"("default":true)"), std::string::npos);
  EXPECT_NE(schema.find(R"("additionalProperties":false)"), std::string::npos);
}

struct ThrowingDefaultCtorField {
  using ReflectionType = std::string;
  std::string value;
  ThrowingDefaultCtorField() {
    throw std::runtime_error("intentional default-ctor throw");
  }
  explicit ThrowingDefaultCtorField(std::string _v) : value(std::move(_v)) {}
  const std::string& reflection() const { return value; }
};

struct ThrowingDefaultStruct {
  int a = 1;
  ThrowingDefaultCtorField boom;
};

struct ThrowingMidParseReader : rfl::json::Reader {
  template <class ObjectReader>
  std::optional<rfl::Error> read_object(const ObjectReader& _reader,
                                        const InputObjectType& _obj) const {
    rfl::json::Reader::read_object(_reader, _obj);
    throw std::runtime_error("mid-parse reader throw");
  }
};

TEST(json, test_direct_struct_exception_safety_guard_and_default_catch) {
  // 1. Throwing default constructor during DefaultIfMissing must be caught and
  //    returned as rfl::Error (not std::terminate).
  {
    const auto res =
        rfl::json::read<ThrowingDefaultStruct, rfl::DefaultIfMissing>(
            R"({"a":2,"boom":"ok"})");
    EXPECT_FALSE(res.has_value());
    EXPECT_NE(res.error().what().find("intentional default-ctor throw"),
              std::string::npos);
  }

  // 2. If a non-noexcept reader throws mid-parse after constructing fields in
  //    stack buf, DirectDestroyGuard must destroy every constructed field with
  //    zero leaks.
  using DirectParserT = rfl::parsing::DirectStructParser<
      ThrowingMidParseReader, rfl::json::Writer, TenFieldTracked,
      rfl::Processors<>>;
  LiveCounter::reset();
  {
    const std::string payload = R"({"t0":"first","n0":7,"t1":"second"})";
    yyjson_doc* doc = yyjson_read(payload.data(), payload.size(), 0);
    ASSERT_NE(doc, nullptr);
    const ThrowingMidParseReader reader{};
    EXPECT_THROW((void)DirectParserT::read(
                     reader, rfl::json::Reader::InputVarType(
                                 yyjson_doc_get_root(doc))),
                 std::runtime_error);
    yyjson_doc_free(doc);
  }
  EXPECT_EQ(LiveCounter::live.load(), 0);
  EXPECT_GT(LiveCounter::constructed.load(), 0);
  EXPECT_EQ(LiveCounter::constructed.load(), LiveCounter::destroyed.load());
}

}  // namespace test_direct_struct_parser
