#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace test_descriptor_table_stress {

// ----------------------------------------------------------------------------
// 1. Tracked non-default-constructible move-only type for strict lifetime /
//    partial-destruction verification under ASan + UBSan.
// ----------------------------------------------------------------------------

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
  bool destroyed_ = false;

  Tracked() = delete;
  Tracked(const Tracked&) = delete;
  Tracked& operator=(const Tracked&) = delete;

  explicit Tracked(std::string v) : value(std::move(v)), destroyed_(false) {
    if (value == "__THROW__") {
      throw std::runtime_error("Tracked intentional throw");
    }
    LiveCounter::live.fetch_add(1);
    LiveCounter::constructed.fetch_add(1);
  }

  Tracked(Tracked&& other) noexcept
      : value(std::move(other.value)), destroyed_(false) {
    LiveCounter::live.fetch_add(1);
    LiveCounter::constructed.fetch_add(1);
  }

  Tracked& operator=(Tracked&& other) noexcept {
    if (this != &other) {
      value = std::move(other.value);
    }
    return *this;
  }

  ~Tracked() {
    EXPECT_FALSE(destroyed_) << "Double destruction detected!";
    destroyed_ = true;
    LiveCounter::live.fetch_sub(1);
    LiveCounter::destroyed.fetch_add(1);
  }

  const std::string& reflection() const { return value; }
};

struct TwelveTracked {
  Tracked f0;
  int n0;
  Tracked f1;
  std::string s1;
  Tracked f2;
  int n2;
  Tracked f3;
  std::optional<std::string> opt;
  Tracked f4;
  int n4;
  Tracked f5;
  Tracked f6;
};

TEST(descriptor_table_stress, tracked_full_roundtrip) {
  LiveCounter::reset();
  {
    const std::string json =
        R"({"f0":"a","n0":1,"f1":"b","s1":"x","f2":"c","n2":2,"f3":"d","opt":"y","f4":"e","n4":4,"f5":"f","f6":"g"})";
    const auto res = rfl::json::read<TwelveTracked>(json);
    ASSERT_TRUE(res.has_value()) << res.error().what();
    EXPECT_EQ(res->f0.value, "a");
    EXPECT_EQ(res->f6.value, "g");
    EXPECT_EQ(res->n4, 4);
    const auto out = rfl::json::write(*res);
    const auto rt = rfl::json::read<TwelveTracked>(out);
    ASSERT_TRUE(rt.has_value()) << rt.error().what();
    EXPECT_EQ(rt->f3.value, "d");
    EXPECT_EQ(rt->opt, std::optional<std::string>("y"));
  }
  EXPECT_EQ(LiveCounter::live.load(), 0);
  EXPECT_GT(LiveCounter::constructed.load(), 0);
  EXPECT_EQ(LiveCounter::constructed.load(), LiveCounter::destroyed.load());
}

TEST(descriptor_table_stress, tracked_mid_parse_failures_and_out_of_order) {
  // Fails at various positions, out-of-order keys, missing fields, and throwing
  // constructors — both default error collection and rfl::FailFast.
  const std::vector<std::string> bad_inputs = {
      // Fail on first field (type mismatch)
      R"({"f0":123,"n0":1,"f1":"b","s1":"x","f2":"c","n2":2,"f3":"d","f4":"e","n4":4,"f5":"f","f6":"g"})",
      // Fail in middle (n2 wrong type) after f0, f1, f2 constructed
      R"({"f0":"a","n0":1,"f1":"b","s1":"x","f2":"c","n2":"bad","f3":"d","f4":"e","n4":4,"f5":"f","f6":"g"})",
      // Out-of-order keys failing near the end
      R"({"f6":"g","f5":"f","f4":"e","f3":"d","f2":"c","f1":"b","f0":"a","n4":4,"n2":2,"s1":"x","n0":"bad"})",
      // Missing required field at start
      R"({"n0":1,"f1":"b","s1":"x","f2":"c","n2":2,"f3":"d","f4":"e","n4":4,"f5":"f","f6":"g"})",
      // Missing required field at end
      R"({"f0":"a","n0":1,"f1":"b","s1":"x","f2":"c","n2":2,"f3":"d","f4":"e","n4":4,"f5":"f"})",
      // Constructor throws mid-parse
      R"({"f0":"a","n0":1,"f1":"b","s1":"x","f2":"__THROW__","n2":2,"f3":"d","f4":"e","n4":4,"f5":"f","f6":"g"})",
      // Wrong root type
      R"([1, 2, 3])",
  };

  for (const auto& input : bad_inputs) {
    LiveCounter::reset();
    {
      const auto r1 = rfl::json::read<TwelveTracked>(input);
      EXPECT_FALSE(r1.has_value());
    }
    EXPECT_EQ(LiveCounter::live.load(), 0) << "Leaked on input: " << input;
    EXPECT_EQ(LiveCounter::constructed.load(), LiveCounter::destroyed.load());

    LiveCounter::reset();
    {
      const auto r2 = rfl::json::read<TwelveTracked, rfl::FailFast>(input);
      EXPECT_FALSE(r2.has_value());
    }
    EXPECT_EQ(LiveCounter::live.load(), 0)
        << "Leaked (FailFast) on input: " << input;
    EXPECT_EQ(LiveCounter::constructed.load(), LiveCounter::destroyed.load());
  }
}

// ----------------------------------------------------------------------------
// 2. Non-standard-layout aggregates: inheritance, empty base, const fields,
//    move-only fields, C-arrays, empty struct.
// ----------------------------------------------------------------------------

struct NonStandardLayoutMember {
  int a = 1;
 private:
  int b_ = 2;
 public:
  using ReflectionType = int;
  NonStandardLayoutMember() = default;
  explicit NonStandardLayoutMember(int v) : a(v), b_(v * 2) {}
  int reflection() const { return a; }
  int doubled() const { return b_; }
};

struct NonStandardLayoutAggregate {
  int base_x = 10;
  std::string base_name = "base";
  NonStandardLayoutMember nsl;
  double derived_val = 2.5;
  std::optional<std::string> derived_note;
};

TEST(descriptor_table_stress, non_standard_layout_aggregate) {
  static_assert(!std::is_standard_layout_v<NonStandardLayoutAggregate>);
  const std::string json =
      R"({"base_x":42,"base_name":"hello","nsl":7,"derived_val":3.5,"derived_note":"ok"})";
  const auto res = rfl::json::read<NonStandardLayoutAggregate>(json);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->base_x, 42);
  EXPECT_EQ(res->base_name, "hello");
  EXPECT_EQ(res->nsl.a, 7);
  EXPECT_DOUBLE_EQ(res->derived_val, 3.5);
  EXPECT_EQ(res->derived_note, std::optional<std::string>("ok"));

  const auto written = rfl::json::write(*res);
  EXPECT_EQ(written, json);

  // Also DefaultIfMissing path
  const auto res_def =
      rfl::json::read<NonStandardLayoutAggregate, rfl::DefaultIfMissing>(
          R"({"derived_val":9.0})");
  ASSERT_TRUE(res_def.has_value()) << res_def.error().what();
  EXPECT_EQ(res_def->base_x, 10);
  EXPECT_EQ(res_def->base_name, "base");
  EXPECT_EQ(res_def->nsl.a, 1);
  EXPECT_DOUBLE_EQ(res_def->derived_val, 9.0);
}

struct MoveOnlyAndConst {
  const int id = 7;
  const std::string tag = "default_tag";
  std::unique_ptr<int> ptr;
  rfl::Box<std::string> boxed = rfl::make_box<std::string>("box_default");
};

TEST(descriptor_table_stress, move_only_and_const_members) {
  const std::string json =
      R"({"id":99,"tag":"custom","ptr":123,"boxed":"hello"})";
  const auto res = rfl::json::read<MoveOnlyAndConst>(json);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->id, 99);
  EXPECT_EQ(res->tag, "custom");
  ASSERT_NE(res->ptr, nullptr);
  EXPECT_EQ(*res->ptr, 123);
  EXPECT_EQ(*res->boxed, "hello");

  const auto out = rfl::json::write(*res);
  EXPECT_EQ(out, json);

  const auto res_def =
      rfl::json::read<MoveOnlyAndConst, rfl::DefaultIfMissing>(
          R"({"id":55,"ptr":null})");
  ASSERT_TRUE(res_def.has_value()) << res_def.error().what();
  EXPECT_EQ(res_def->id, 55);
  EXPECT_EQ(res_def->tag, "default_tag");
  EXPECT_EQ(res_def->ptr, nullptr);
  EXPECT_EQ(*res_def->boxed, "box_default");
}

struct EmptyStruct {};

TEST(descriptor_table_stress, empty_struct) {
  const auto res = rfl::json::read<EmptyStruct>("{}");
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(rfl::json::write(*res), "{}");
  const auto schema = rfl::json::to_schema<EmptyStruct>();
  EXPECT_NE(schema.find("EmptyStruct"), std::string::npos);
}

// ----------------------------------------------------------------------------
// 3. Flatten + ExtraFields + Tracked partial-destruction on failure.
// ----------------------------------------------------------------------------

struct InnerFlat {
  Tracked inner_a;
  int inner_b;
};

struct OuterFlat {
  Tracked outer_first;
  rfl::Flatten<InnerFlat> flat;
  Tracked outer_last;
};

TEST(descriptor_table_stress, flatten_tracked_partial_destruction) {
  LiveCounter::reset();
  {
    const std::string ok_json =
        R"({"outer_first":"x","inner_a":"y","inner_b":7,"outer_last":"z"})";
    const auto ok = rfl::json::read<OuterFlat>(ok_json);
    ASSERT_TRUE(ok.has_value()) << ok.error().what();
    EXPECT_EQ(ok->outer_first.value, "x");
    EXPECT_EQ(ok->flat.value_.inner_a.value, "y");
    EXPECT_EQ(ok->flat.value_.inner_b, 7);
    EXPECT_EQ(ok->outer_last.value, "z");
    EXPECT_EQ(rfl::json::write(*ok), ok_json);
  }
  EXPECT_EQ(LiveCounter::live.load(), 0);

  LiveCounter::reset();
  {
    const std::string bad_json =
        R"({"outer_first":"x","inner_a":"y","inner_b":"not_an_int","outer_last":"z"})";
    const auto bad = rfl::json::read<OuterFlat>(bad_json);
    EXPECT_FALSE(bad.has_value());
  }
  EXPECT_EQ(LiveCounter::live.load(), 0);
  EXPECT_EQ(LiveCounter::constructed.load(), LiveCounter::destroyed.load());
}

struct WithExtraAndTracked {
  Tracked required_a;
  int count;
  rfl::ExtraFields<rfl::Generic> extras;
};

TEST(descriptor_table_stress, extra_fields_and_tracked_cleanup) {
  LiveCounter::reset();
  {
    const std::string ok_json =
        R"({"required_a":"hi","count":3,"dyn1":"v1","dyn2":42})";
    const auto ok = rfl::json::read<WithExtraAndTracked>(ok_json);
    ASSERT_TRUE(ok.has_value()) << ok.error().what();
    EXPECT_EQ(ok->required_a.value, "hi");
    EXPECT_EQ(ok->extras.size(), 2u);
    const auto out = rfl::json::write(*ok);
    EXPECT_NE(out.find("\"dyn1\":\"v1\""), std::string::npos);
    EXPECT_NE(out.find("\"dyn2\":42"), std::string::npos);
  }
  EXPECT_EQ(LiveCounter::live.load(), 0);

  // Failure after extras and required_a have been constructed
  LiveCounter::reset();
  {
    const std::string bad_json =
        R"({"required_a":"hi","dyn1":"v1","count":"oops"})";
    const auto bad = rfl::json::read<WithExtraAndTracked>(bad_json);
    EXPECT_FALSE(bad.has_value());
  }
  EXPECT_EQ(LiveCounter::live.load(), 0);
  EXPECT_EQ(LiveCounter::constructed.load(), LiveCounter::destroyed.load());
}

// ----------------------------------------------------------------------------
// 4. Case-transform processors + Rename + Skip + DefaultVal + NoExtraFields
//    + NoFieldNames + Duplicate JSON keys + Wide struct (> 8 fields).
// ----------------------------------------------------------------------------

struct WideCaseStruct {
  int first_field = 1;
  std::string second_field = "two";
  rfl::Rename<"EXACT_WIRE_NAME", int> renamed_field = 3;
  rfl::DefaultVal<int> default_fourth = 4;
  rfl::Skip<int> skipped_fifth = 5;
  bool sixth_flag = true;
  double seventh_ratio = 1.5;
  std::optional<std::string> eighth_opt;
  int ninth_value = 9;
  std::string tenth_name = "ten";
};

TEST(descriptor_table_stress, case_transforms_respect_rename_and_skip) {
  const std::string camel_json =
      R"({"firstField":11,"secondField":"two2","EXACT_WIRE_NAME":33,"defaultFourth":44,"sixthFlag":false,"seventhRatio":2.5,"eighthOpt":"e","ninthValue":99,"tenthName":"ten10"})";
  const auto res =
      rfl::json::read<WideCaseStruct, rfl::SnakeCaseToCamelCase,
                      rfl::NoExtraFields>(camel_json);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->first_field, 11);
  EXPECT_EQ(res->renamed_field.value(), 33);
  EXPECT_EQ(res->default_fourth.value(), 44);
  EXPECT_EQ(res->skipped_fifth.value(), 5);
  EXPECT_EQ(res->tenth_name, "ten10");

  const auto written =
      rfl::json::write<rfl::SnakeCaseToCamelCase>(*res);
  EXPECT_EQ(written, camel_json);

  // Unknown key rejected under NoExtraFields + SnakeCaseToCamelCase
  const auto bad =
      rfl::json::read<WideCaseStruct, rfl::SnakeCaseToCamelCase,
                      rfl::NoExtraFields>(
          R"({"firstField":1,"secondField":"s","EXACT_WIRE_NAME":3,"sixthFlag":true,"seventhRatio":1.0,"ninthValue":9,"tenthName":"t","bogus":1})");
  EXPECT_FALSE(bad.has_value());

  // Schema respects transformed names + EXACT_WIRE_NAME + DefaultVal default
  const auto schema =
      rfl::json::to_schema<WideCaseStruct, rfl::SnakeCaseToCamelCase,
                           rfl::NoExtraFields>();
  EXPECT_NE(schema.find("\"firstField\""), std::string::npos);
  EXPECT_NE(schema.find("\"EXACT_WIRE_NAME\""), std::string::npos);
  EXPECT_EQ(schema.find("\"skippedFifth\""), std::string::npos);
  EXPECT_NE(schema.find("\"additionalProperties\":false"), std::string::npos);

  // Kebab-case + DefaultIfMissing
  const auto kebab =
      rfl::json::read<WideCaseStruct, rfl::SnakeCaseToKebabCase,
                      rfl::DefaultIfMissing>(
          R"({"first-field":77,"EXACT_WIRE_NAME":88,"tenth-name":"k"})");
  ASSERT_TRUE(kebab.has_value()) << kebab.error().what();
  EXPECT_EQ(kebab->first_field, 77);
  EXPECT_EQ(kebab->renamed_field.value(), 88);
  EXPECT_EQ(kebab->default_fourth.value(), 4);
  EXPECT_EQ(kebab->tenth_name, "k");

  // PascalCase
  const auto pascal_out =
      rfl::json::write<rfl::SnakeCaseToPascalCase>(*kebab);
  EXPECT_NE(pascal_out.find("\"FirstField\":77"), std::string::npos);
  EXPECT_NE(pascal_out.find("\"EXACT_WIRE_NAME\":88"), std::string::npos);

  // ToAllCaps
  const auto caps_out = rfl::json::write<rfl::ToAllCaps>(*kebab);
  EXPECT_NE(caps_out.find("\"FIRST_FIELD\":77"), std::string::npos);
  EXPECT_NE(caps_out.find("\"EXACT_WIRE_NAME\":88"), std::string::npos);
}

struct CamelStruct {
  int myFieldOne = 1;
  std::string anotherFieldTwo = "x";
};

TEST(descriptor_table_stress, camel_to_snake_and_no_field_names) {
  const auto res = rfl::json::read<CamelStruct, rfl::CamelCaseToSnakeCase>(
      R"({"my_field_one":5,"another_field_two":"ok"})");
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->myFieldOne, 5);
  EXPECT_EQ(rfl::json::write<rfl::CamelCaseToSnakeCase>(*res),
            R"({"my_field_one":5,"another_field_two":"ok"})");

  // NoFieldNames (array representation)
  const auto arr_res =
      rfl::json::read<CamelStruct, rfl::NoFieldNames>(R"([42,"pos"])");
  ASSERT_TRUE(arr_res.has_value()) << arr_res.error().what();
  EXPECT_EQ(arr_res->myFieldOne, 42);
  EXPECT_EQ(arr_res->anotherFieldTwo, "pos");
  EXPECT_EQ(rfl::json::write<rfl::NoFieldNames>(*arr_res), R"([42,"pos"])");

  // Too many array elements rejected
  const auto arr_bad =
      rfl::json::read<CamelStruct, rfl::NoFieldNames>(R"([1,"a","extra"])");
  EXPECT_FALSE(arr_bad.has_value());

  // NoFieldNames + DefaultIfMissing
  const auto arr_def =
      rfl::json::read<CamelStruct, rfl::NoFieldNames, rfl::DefaultIfMissing>(
          R"([99])");
  ASSERT_TRUE(arr_def.has_value()) << arr_def.error().what();
  EXPECT_EQ(arr_def->myFieldOne, 99);
  EXPECT_EQ(arr_def->anotherFieldTwo, "x");
}

struct DuplicateKeyStruct {
  int a;
  int b;
  int c;
  int d;
  int e;
  int f;
  int g;
  int h;
  int i;
};

TEST(descriptor_table_stress, duplicate_json_key_first_wins_and_no_extra) {
  const std::string dup_json =
      R"({"a":1,"a":999,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9})";
  const auto res = rfl::json::read<DuplicateKeyStruct>(dup_json);
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->a, 1);

  // Under NoExtraFields, duplicate key is treated as unused (matches ViewReader)
  const auto res_strict =
      rfl::json::read<DuplicateKeyStruct, rfl::NoExtraFields>(dup_json);
  EXPECT_FALSE(res_strict.has_value());
}

struct TaggedItem {
  int x = 1;
  std::string label = "lbl";
};

TEST(descriptor_table_stress, add_struct_name_and_validators) {
  const auto out = rfl::json::write<rfl::AddStructName<"kind">>(TaggedItem{});
  EXPECT_EQ(out, R"({"kind":"TaggedItem","x":1,"label":"lbl"})");
  const auto rt = rfl::json::read<TaggedItem, rfl::AddStructName<"kind">>(out);
  ASSERT_TRUE(rt.has_value()) << rt.error().what();
  EXPECT_EQ(rt->x, 1);
  EXPECT_EQ(rt->label, "lbl");

  struct ValidatedStruct {
    rfl::Validator<int, rfl::Minimum<1>> positive = 1;
    rfl::DefaultVal<rfl::Description<"A described field", std::string>> desc =
        "hi";
  };
  const auto v_ok = rfl::json::read<ValidatedStruct>(R"({"positive":5})");
  ASSERT_TRUE(v_ok.has_value()) << v_ok.error().what();
  EXPECT_EQ(v_ok->positive.value(), 5);
  EXPECT_EQ(v_ok->desc.value().value(), "hi");

  const auto v_bad = rfl::json::read<ValidatedStruct>(R"({"positive":0})");
  EXPECT_FALSE(v_bad.has_value());
}

// ----------------------------------------------------------------------------
// 5. Review Round 1 regression tests:
//    - ExtraFields key matching C++ member name (first key, uninit + default)
//    - Throwing move constructor during Result<T>(std::move(*ptr))
//    - Throwing move-assign inside read_with_default (must return Error, not terminate)
//    - Flatten<const Sub> mutability + multi-op xml_content preservation
// ----------------------------------------------------------------------------

struct ExtraStringStruct {
  std::string name = "default";
  rfl::ExtraFields<std::string> extras;
};

TEST(descriptor_table_stress, extra_fields_key_matching_member_name) {
  // Key literally named "extras" arrives FIRST, before any other extra key.
  const std::string input = R"({"extras":"first_val","name":"Alice","k2":"v2"})";
  const auto r1 = rfl::json::read<ExtraStringStruct>(input);
  ASSERT_TRUE(r1.has_value()) << r1.error().what();
  EXPECT_EQ(r1->name, "Alice");
  ASSERT_EQ(r1->extras.size(), 2u);
  EXPECT_EQ(r1->extras.get("extras").value(), "first_val");
  EXPECT_EQ(r1->extras.get("k2").value(), "v2");

  const auto r2 =
      rfl::json::read<ExtraStringStruct, rfl::DefaultIfMissing>(input);
  ASSERT_TRUE(r2.has_value()) << r2.error().what();
  EXPECT_EQ(r2->extras.get("extras").value(), "first_val");
  EXPECT_EQ(r2->extras.get("k2").value(), "v2");
}

// Minimal non-noexcept custom named Reader (intentionally WITHOUT
// `using UnsizedObjectReader = void;`) that both:
// 1. Verifies custom/external named readers work with rfl::ExtraFields, and
// 2. Throws mid-parse on a sentinel key to exercise UninitFieldsGuard and
//    read_struct_with_default exception catching.
struct CustomThrowingReader {
  struct InputVarType {
    std::string str;
    bool is_obj = false;
    std::vector<std::pair<std::string, std::string>> entries;
  };
  using InputObjectType = InputVarType;
  using InputArrayType = InputVarType;

  template <class T>
  static constexpr bool has_custom_constructor = false;

  rfl::Result<InputVarType> get_field_from_array(const std::size_t,
                                                 const InputArrayType&) const {
    return rfl::error("unused");
  }

  rfl::Result<InputVarType> get_field_from_object(const std::string&,
                                                  const InputObjectType&) const {
    return rfl::error("unused");
  }

  bool is_empty(const InputVarType& _var) const {
    return !_var.is_obj && _var.str.empty();
  }

  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _var) const {
    if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::string>) {
      return _var.str;
    } else if constexpr (std::is_same_v<std::remove_cvref_t<T>, int>) {
      return std::stoi(_var.str);
    } else {
      return rfl::error("unsupported");
    }
  }

  rfl::Result<InputArrayType> to_array(const InputVarType&) const {
    return rfl::error("not array");
  }

  rfl::Result<InputObjectType> to_object(const InputVarType& _var) const {
    if (!_var.is_obj) return rfl::error("not object");
    return _var;
  }

  template <class ArrayReader>
  std::optional<rfl::Error> read_array(const ArrayReader&,
                                       const InputArrayType&) const {
    return rfl::Error("unused");
  }

  template <class ObjectReader>
  std::optional<rfl::Error> read_object(const ObjectReader& _reader,
                                        const InputObjectType& _obj) const {
    for (const auto& [k, v] : _obj.entries) {
      if (k == "__THROW_READER__") {
        throw std::runtime_error("intentional reader throw");
      }
      _reader.read(std::string_view(k), InputVarType{.str = v});
    }
    return std::nullopt;
  }

  template <class T>
  rfl::Result<T> use_custom_constructor(const InputVarType&) const {
    return rfl::error("unused");
  }
};

struct ThrowingReaderTarget {
  Tracked before;
  Tracked after;
};

struct ThrowingDefaultTarget {
  std::string label = "init";
  int count = 0;
};

TEST(descriptor_table_stress,
     throwing_reader_guard_and_custom_named_reader_extra_fields) {
  using PUninit = rfl::parsing::Parser<CustomThrowingReader, rfl::json::Writer,
                                       ThrowingReaderTarget, rfl::Processors<>>;
  using PDefault =
      rfl::parsing::Parser<CustomThrowingReader, rfl::json::Writer,
                           ThrowingDefaultTarget,
                           rfl::Processors<rfl::DefaultIfMissing>>;
  using PExtra = rfl::parsing::Parser<CustomThrowingReader, rfl::json::Writer,
                                      ExtraStringStruct, rfl::Processors<>>;

  const CustomThrowingReader reader{};

  // 1. UninitFieldsGuard must destroy `before` when read_object throws mid-parse
  LiveCounter::reset();
  bool caught = false;
  try {
    const CustomThrowingReader::InputVarType input{
        .is_obj = true,
        .entries = {{"before", "alive_val"}, {"__THROW_READER__", "boom"}}};
    (void)PUninit::read(reader, input);
  } catch (const std::runtime_error& e) {
    caught = (std::string_view(e.what()) == "intentional reader throw");
  }
  EXPECT_TRUE(caught);
  EXPECT_EQ(LiveCounter::live.load(), 0);
  EXPECT_GT(LiveCounter::constructed.load(), 0);
  EXPECT_EQ(LiveCounter::constructed.load(), LiveCounter::destroyed.load());

  // 2. read_with_default must NOT be noexcept so Parser_default's try/catch
  //    catches the exception and returns an rfl::Error cleanly.
  {
    const CustomThrowingReader::InputVarType input{
        .is_obj = true,
        .entries = {{"label", "ok"}, {"__THROW_READER__", "boom"}}};
    const auto res = PDefault::read(reader, input);
    ASSERT_FALSE(res.has_value());
    EXPECT_NE(res.error().what().find("intentional reader throw"),
              std::string::npos);
  }

  // 3. External/custom named Reader without UnsizedObjectReader tag must still
  //    compile and handle rfl::ExtraFields cleanly via SizedTableViewReader.
  {
    const CustomThrowingReader::InputVarType input{
        .is_obj = true,
        .entries = {{"extras", "first_val"}, {"name", "Bob"}, {"dyn", "v"}}};
    const auto res = PExtra::read(reader, input);
    ASSERT_TRUE(res.has_value()) << res.error().what();
    EXPECT_EQ(res->name, "Bob");
    EXPECT_EQ(res->extras.get("extras").value(), "first_val");
    EXPECT_EQ(res->extras.get("dyn").value(), "v");
  }
}

struct InnerPlain {
  int a = 1;
  std::string b = "init";
};

struct FlattenConstWrapper {
  rfl::Flatten<const InnerPlain> flat;
  int tail = 2;
};

struct XmlMultiOpStruct {
  int firstField = 1;
  std::string xmlContent = "body";
};

TEST(descriptor_table_stress, flatten_const_sub_and_multi_op_xml_content) {
  const auto r = rfl::json::read<FlattenConstWrapper, rfl::DefaultIfMissing>(
      R"({"a":10,"b":"updated","tail":20})");
  ASSERT_TRUE(r.has_value()) << r.error().what();
  EXPECT_EQ(r->flat.value_.a, 10);
  EXPECT_EQ(r->flat.value_.b, "updated");
  EXPECT_EQ(r->tail, 20);

  // CamelCaseToSnakeCase turns xmlContent -> xml_content, then ToAllCaps must
  // leave xml_content untouched (matching NamedTuple processor semantics).
  const auto out =
      rfl::json::write<rfl::CamelCaseToSnakeCase, rfl::ToAllCaps>(
          XmlMultiOpStruct{});
  EXPECT_EQ(out, R"({"FIRST_FIELD":1,"xml_content":"body"})");
  const auto rt =
      rfl::json::read<XmlMultiOpStruct, rfl::CamelCaseToSnakeCase,
                      rfl::ToAllCaps>(out);
  ASSERT_TRUE(rt.has_value()) << rt.error().what();
  EXPECT_EQ(rt->firstField, 1);
  EXPECT_EQ(rt->xmlContent, "body");
}

}  // namespace test_descriptor_table_stress
