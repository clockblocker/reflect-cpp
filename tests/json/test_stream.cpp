#include <gtest/gtest.h>

#include <iterator>
#include <memory>
#include <ranges>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace test_stream {

struct Row {
  int id;
  std::string name;
  double score;
  bool operator==(const Row&) const = default;
};

struct SnakeRow {
  int user_id;
  std::string first_name;
  bool operator==(const SnakeRow&) const = default;
};

struct MoveOnlyRow {
  int id;
  std::unique_ptr<std::string> label;
};

static_assert(std::input_iterator<rfl::json::StreamReader<Row>::Iterator>);
static_assert(std::ranges::input_range<rfl::json::StreamReader<Row>>);
static_assert(
    std::input_iterator<rfl::json::StreamReader<MoveOnlyRow>::Iterator>);

template <class T, class... Ps>
std::vector<T> collect_ok(rfl::json::StreamReader<T, Ps...>& _reader) {
  std::vector<T> out;
  for (auto&& res : _reader) {
    EXPECT_TRUE(res.has_value()) << (res.has_value() ? "" : res.error().what());
    if (res.has_value()) {
      out.push_back(std::move(*res));
    }
  }
  return out;
}

std::vector<Row> sample_rows() {
  return {
      Row{1, "Alice", 9.5},
      Row{2, "Bob", -3.25},
      Row{3, "Charlie\nLine2", 0.0},
  };
}

TEST(json, test_stream_ndjson_roundtrip) {
  const auto expected = sample_rows();
  std::stringstream ss;
  {
    rfl::json::StreamWriter writer(ss, rfl::json::StreamFormat::ndjson);
    for (const auto& row : expected) writer.write(row);
    writer.finish();
  }

  const std::string payload = ss.str();
  EXPECT_NE(payload.find('\n'), std::string::npos);

  // istream overload via next()
  std::istringstream in(payload);
  rfl::json::StreamReader<Row> reader(in, rfl::json::StreamFormat::ndjson);
  std::vector<Row> actual;
  while (auto res = reader.next()) {
    ASSERT_TRUE(res->has_value()) << res->error().what();
    actual.push_back(std::move(**res));
  }
  EXPECT_EQ(actual, expected);
  EXPECT_FALSE(reader.next().has_value());

  // string_view overload via range-for
  std::vector<Row> from_sv;
  for (auto&& res : rfl::json::StreamReader<Row>(
           std::string_view{payload}, rfl::json::StreamFormat::ndjson)) {
    ASSERT_TRUE(res.has_value()) << res.error().what();
    from_sv.push_back(std::move(*res));
  }
  EXPECT_EQ(from_sv, expected);
}

TEST(json, test_stream_json_seq_roundtrip_and_consecutive_rs) {
  const auto expected = sample_rows();
  std::stringstream ss;
  {
    rfl::json::StreamWriter writer(ss, rfl::json::StreamFormat::json_seq);
    for (const auto& row : expected) writer.write(row);
  }
  ASSERT_FALSE(ss.str().empty());
  EXPECT_EQ(ss.str().front(), '\x1e');

  std::istringstream in(ss.str());
  rfl::json::StreamReader<Row> reader(in, rfl::json::StreamFormat::json_seq);
  EXPECT_EQ(collect_ok(reader), expected);

  // Consecutive \x1e, blank lines, and trailing empty sequences
  const std::string messy =
      "\x1e\x1e  \n\x1e{\"id\":1,\"name\":\"A\",\"score\":1.5}\n"
      "\x1e\n\x1e\t{\"id\":2,\"name\":\"B\",\"score\":2.5}\n\x1e\x1e  ";
  rfl::json::StreamReader<Row> messy_reader(std::string_view{messy},
                                            rfl::json::StreamFormat::json_seq);
  EXPECT_EQ(collect_ok(messy_reader),
            (std::vector<Row>{{1, "A", 1.5}, {2, "B", 2.5}}));
}

TEST(json, test_stream_json_array_roundtrip_and_finish) {
  const auto expected = sample_rows();
  std::stringstream ss;
  {
    rfl::json::StreamWriter writer(ss, rfl::json::StreamFormat::json_array);
    for (const auto& row : expected) writer.write(row);
    writer.finish();
    writer.finish();  // idempotent
    EXPECT_THROW(writer.write(expected.front()), std::runtime_error);
  }

  const std::string payload = ss.str();
  EXPECT_EQ(payload.front(), '[');
  EXPECT_EQ(payload.back(), ']');

  const auto full = rfl::json::read<std::vector<Row>>(payload);
  ASSERT_TRUE(full.has_value()) << full.error().what();
  EXPECT_EQ(*full, expected);

  std::istringstream in(payload);
  rfl::json::StreamReader<Row> reader(in, rfl::json::StreamFormat::json_array);
  EXPECT_EQ(collect_ok(reader), expected);

  // 0 records + destructor auto-finish emits "[]"
  std::stringstream empty_ss;
  { rfl::json::StreamWriter writer(empty_ss, rfl::json::StreamFormat::json_array); }
  EXPECT_EQ(empty_ss.str(), "[]");
}

TEST(json, test_stream_empty_inputs) {
  for (const std::string_view input : {"", "   \n\t\r\n  "}) {
    std::istringstream in{std::string(input)};
    EXPECT_FALSE(
        rfl::json::StreamReader<Row>(in, rfl::json::StreamFormat::ndjson)
            .next()
            .has_value());
  }
  for (const std::string_view input : {"", "\x1e", "\x1e\x1e\n  \x1e\t"}) {
    EXPECT_FALSE(
        rfl::json::StreamReader<Row>(input, rfl::json::StreamFormat::json_seq)
            .next()
            .has_value());
  }
  for (const std::string_view input : {"[]", " [ \n\t ] "}) {
    std::istringstream in{std::string(input)};
    EXPECT_FALSE(
        rfl::json::StreamReader<Row>(in, rfl::json::StreamFormat::json_array)
            .next()
            .has_value());
  }

  // Empty string is not a valid json_array
  rfl::json::StreamReader<Row> bad_empty(std::string_view{""},
                                         rfl::json::StreamFormat::json_array);
  const auto res = bad_empty.next();
  ASSERT_TRUE(res.has_value());
  EXPECT_FALSE(res->has_value());
  EXPECT_FALSE(bad_empty.next().has_value());
}

TEST(json, test_stream_multiline_and_pretty_records) {
  const auto expected = sample_rows();
  for (const auto fmt :
       {rfl::json::StreamFormat::ndjson, rfl::json::StreamFormat::json_seq,
        rfl::json::StreamFormat::json_array}) {
    std::stringstream ss;
    {
      rfl::json::StreamWriter writer(ss, fmt, rfl::json::pretty);
      for (const auto& row : expected) writer.write(row);
    }
    std::istringstream in(ss.str());
    rfl::json::StreamReader<Row> reader(in, fmt, 0, 13);
    EXPECT_EQ(collect_ok(reader), expected);
  }
}

TEST(json, test_stream_tiny_chunks_primitives_and_objects) {
  const std::vector<double> expected_nums = {0.0, 123456.0, -987.625, 1.5e-4,
                                             42.0};
  const std::string ndjson_nums = "0\n123456\n-987.625\n1.5e-4\n42";
  const std::string array_nums = "[0, 123456, -987.625, 1.5e-4, 42]";

  const std::vector<std::string> expected_strs = {
      "hello", "line1\nline2\t\"quoted\"",
      "utf8: h\xc3\xa9llo \xf0\x9f\x8c\x8d", std::string(200, 'x')};
  std::stringstream str_ss;
  {
    rfl::json::StreamWriter writer(str_ss, rfl::json::StreamFormat::ndjson);
    for (const auto& s : expected_strs) writer.write(s);
  }

  const std::vector<Row> expected_rows = {
      Row{100, std::string(256, 'a'), 1.25},
      Row{200, "h\xc3\xa9llo \xf0\x9f\x9a\x80", -99.5},
  };
  std::stringstream obj_ss;
  {
    rfl::json::StreamWriter writer(obj_ss, rfl::json::StreamFormat::json_array);
    for (const auto& r : expected_rows) writer.write(r);
  }

  for (const size_t chunk : {size_t{1}, size_t{3}, size_t{7}, size_t{13}}) {
    {
      std::istringstream in(ndjson_nums);
      rfl::json::StreamReader<double> r(in, rfl::json::StreamFormat::ndjson, 0,
                                        chunk);
      EXPECT_EQ(collect_ok(r), expected_nums) << "chunk=" << chunk;
    }
    {
      std::istringstream in(array_nums);
      rfl::json::StreamReader<double> r(in, rfl::json::StreamFormat::json_array,
                                        0, chunk);
      EXPECT_EQ(collect_ok(r), expected_nums) << "chunk=" << chunk;
    }
    {
      std::istringstream in(str_ss.str());
      rfl::json::StreamReader<std::string> r(
          in, rfl::json::StreamFormat::ndjson, 0, chunk);
      EXPECT_EQ(collect_ok(r), expected_strs) << "chunk=" << chunk;
    }
    {
      std::istringstream in(obj_ss.str());
      rfl::json::StreamReader<Row> r(in, rfl::json::StreamFormat::json_array, 0,
                                     chunk);
      EXPECT_EQ(collect_ok(r), expected_rows) << "chunk=" << chunk;
    }
  }
}

TEST(json, test_stream_processors_and_move_only) {
  const std::vector<SnakeRow> expected = {{1, "Ada"}, {2, "Grace"}};
  std::stringstream ss;
  {
    rfl::json::StreamWriter<rfl::SnakeCaseToCamelCase> writer(ss);
    for (const auto& r : expected) writer.write(r);
  }
  EXPECT_NE(ss.str().find("\"userId\""), std::string::npos);
  EXPECT_NE(ss.str().find("\"firstName\""), std::string::npos);

  std::istringstream in(ss.str());
  rfl::json::StreamReader<SnakeRow, rfl::SnakeCaseToCamelCase> reader(in);
  EXPECT_EQ(collect_ok(reader), expected);

  // NoExtraFields rejects extra field on record 2 while keeping stream intact
  const std::string mixed =
      R"({"id":1,"name":"ok","score":1.0})" "\n"
      R"({"id":2,"name":"bad","score":2.0,"extra":true})" "\n"
      R"({"id":3,"name":"ok2","score":3.0})" "\n";
  rfl::json::StreamReader<Row, rfl::NoExtraFields> strict_reader{
      std::string_view{mixed}};
  const auto r1 = strict_reader.next();
  ASSERT_TRUE(r1.has_value() && r1->has_value());
  EXPECT_EQ(**r1, (Row{1, "ok", 1.0}));
  const auto r2 = strict_reader.next();
  ASSERT_TRUE(r2.has_value());
  EXPECT_FALSE(r2->has_value());
  const auto r3 = strict_reader.next();
  ASSERT_TRUE(r3.has_value() && r3->has_value());
  EXPECT_EQ(**r3, (Row{3, "ok2", 3.0}));
  EXPECT_FALSE(strict_reader.next().has_value());

  // Move-only record type with range-for
  const std::string move_json =
      R"({"id":7,"label":"first"})" "\n" R"({"id":8,"label":"second"})" "\n";
  std::vector<MoveOnlyRow> moved;
  for (auto&& res :
       rfl::json::StreamReader<MoveOnlyRow>(std::string_view{move_json})) {
    ASSERT_TRUE(res.has_value()) << res.error().what();
    moved.push_back(std::move(*res));
  }
  ASSERT_EQ(moved.size(), 2u);
  EXPECT_EQ(*moved[0].label, "first");
  EXPECT_EQ(*moved[1].label, "second");
}

TEST(json, test_stream_error_handling) {
  // YYJSON_READ_INSITU rejected
  {
    rfl::json::StreamReader<Row> reader(
        std::string_view{R"({"id":1,"name":"a","score":1.0})"},
        rfl::json::StreamFormat::ndjson, YYJSON_READ_INSITU);
    const auto res = reader.next();
    ASSERT_TRUE(res.has_value());
    EXPECT_FALSE(res->has_value());
    EXPECT_FALSE(reader.next().has_value());
  }

  // Malformed and truncated ndjson
  for (const std::string_view bad :
       {std::string_view{"{bad}"}, std::string_view{R"({"id":1,"name":"a")"}}) {
    std::istringstream in{std::string(bad)};
    rfl::json::StreamReader<Row> reader(in, rfl::json::StreamFormat::ndjson, 0,
                                        3);
    const auto res = reader.next();
    ASSERT_TRUE(res.has_value());
    EXPECT_FALSE(res->has_value());
    EXPECT_FALSE(reader.next().has_value());
  }

  // json_array framing errors
  for (const std::string_view bad :
       {"{}", "[", "[1, 2", "[1 2]", "[1,]", "[1,", "[] extra", "[1] extra"}) {
    std::istringstream in{std::string(bad)};
    rfl::json::StreamReader<int> reader(
        in, rfl::json::StreamFormat::json_array, 0, 3);
    bool saw_error = false;
    int steps = 0;
    while (auto res = reader.next()) {
      ASSERT_LE(++steps, 10) << "Infinite loop on input: " << bad;
      if (!res->has_value()) saw_error = true;
    }
    EXPECT_TRUE(saw_error) << "Expected error for input: " << bad;
  }

  // Trailing comma allowed when YYJSON_READ_ALLOW_TRAILING_COMMAS is passed
  rfl::json::StreamReader<int> trailing(std::string_view{"[1, 2,]"},
                                        rfl::json::StreamFormat::json_array,
                                        YYJSON_READ_ALLOW_TRAILING_COMMAS);
  EXPECT_EQ(collect_ok(trailing), (std::vector<int>{1, 2}));
}

}  // namespace test_stream
