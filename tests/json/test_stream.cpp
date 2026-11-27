#include <gtest/gtest.h>

#include <chrono>
#include <deque>
#include <iterator>
#include <limits>
#include <memory>
#include <ranges>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <sstream>
#include <streambuf>
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

struct IdOnly {
  int id;
  bool operator==(const IdOnly&) const = default;
};

// Unbuffered streambuf: yields 1 byte per underflow() with in_avail() == 0,
// simulating unbuffered streams such as synced std::cin.
class UnbufferedBuf : public std::streambuf {
 public:
  explicit UnbufferedBuf(std::string _data) : data_(std::move(_data)) {}

 protected:
  int_type underflow() override {
    if (idx_ >= data_.size()) {
      return traits_type::eof();
    }
    return traits_type::to_int_type(data_[idx_]);
  }

  int_type uflow() override {
    if (idx_ >= data_.size()) {
      return traits_type::eof();
    }
    return traits_type::to_int_type(data_[idx_++]);
  }

 private:
  std::string data_;
  size_t idx_ = 0;
};

static_assert(std::input_iterator<rfl::json::StreamReader<Row>::Iterator>);
static_assert(std::ranges::input_range<rfl::json::StreamReader<Row>>);
static_assert(
    std::same_as<decltype(std::declval<rfl::json::StreamReader<Row>&>().end()),
                 std::default_sentinel_t>);
static_assert(
    std::input_iterator<rfl::json::StreamReader<MoveOnlyRow>::Iterator>);
static_assert(std::ranges::input_range<rfl::json::StreamReader<MoveOnlyRow>>);

// Simulated live pipe streambuf: yields chunks one at a time and tracks how
// many underflow() calls have occurred so tests can assert non-blocking reads.
class ChunkedPipeBuf : public std::streambuf {
 public:
  void push_chunk(std::string _chunk) { chunks_.push_back(std::move(_chunk)); }

  size_t underflow_count() const noexcept { return underflows_; }

 protected:
  int_type underflow() override {
    if (gptr() < egptr()) {
      return traits_type::to_int_type(*gptr());
    }
    if (chunks_.empty()) {
      return traits_type::eof();
    }
    ++underflows_;
    current_ = std::move(chunks_.front());
    chunks_.pop_front();
    setg(current_.data(), current_.data(), current_.data() + current_.size());
    return traits_type::to_int_type(*gptr());
  }

 private:
  std::deque<std::string> chunks_;
  std::string current_;
  size_t underflows_ = 0;
};

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
    for (const auto& row : expected) {
      ASSERT_TRUE(writer.write(row).has_value());
    }
    ASSERT_TRUE(writer.flush().has_value());
    ASSERT_TRUE(writer.finish().has_value());
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
    for (const auto& row : expected) {
      ASSERT_TRUE(writer.write(row).has_value());
    }
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
    for (const auto& row : expected) {
      ASSERT_TRUE(writer.write(row).has_value());
    }
    ASSERT_TRUE(writer.finish().has_value());
    ASSERT_TRUE(writer.finish().has_value());  // idempotent
    const auto after_finish = writer.write(expected.front());
    EXPECT_FALSE(after_finish.has_value());
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
  {
    rfl::json::StreamWriter writer(empty_ss,
                                   rfl::json::StreamFormat::json_array);
  }
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

  // StreamWriter allows pretty printing for json_array
  {
    std::stringstream ss;
    {
      rfl::json::StreamWriter writer(ss, rfl::json::StreamFormat::json_array,
                                     rfl::json::pretty);
      for (const auto& row : expected) {
        ASSERT_TRUE(writer.write(row).has_value());
      }
    }
    std::istringstream in(ss.str());
    rfl::json::StreamReader<Row> reader(in, rfl::json::StreamFormat::json_array,
                                        0, 13);
    EXPECT_EQ(collect_ok(reader), expected);
  }

  // StreamReader supports multi-line / pretty-printed json_seq records (both
  // istream and string_view); ndjson enforces strict single-line framing so
  // unclosed records on one line cannot swallow subsequent lines.
  {
    std::string seq_pretty;
    for (const auto& row : expected) {
      const auto pretty_rec = rfl::json::write(row, rfl::json::pretty);
      seq_pretty += '\x1e';
      seq_pretty += pretty_rec + "\n";
    }

    std::istringstream in_seq(seq_pretty);
    rfl::json::StreamReader<Row> r_seq_in(
        in_seq, rfl::json::StreamFormat::json_seq, 0, 13);
    EXPECT_EQ(collect_ok(r_seq_in), expected);

    rfl::json::StreamReader<Row> r_seq_sv(std::string_view{seq_pretty},
                                          rfl::json::StreamFormat::json_seq);
    EXPECT_EQ(collect_ok(r_seq_sv), expected);
  }
}

TEST(json, test_stream_writer_errors_and_flags) {
  const Row sample{1, "Alice", 9.5};

  // Pretty / newline-at-end flags rejected for ndjson and json_seq
  for (const auto fmt :
       {rfl::json::StreamFormat::ndjson, rfl::json::StreamFormat::json_seq}) {
    for (const yyjson_write_flag bad_flag :
         {YYJSON_WRITE_PRETTY, YYJSON_WRITE_PRETTY_TWO_SPACES,
          YYJSON_WRITE_NEWLINE_AT_END}) {
      std::stringstream ss;
      rfl::json::StreamWriter writer(ss, fmt, bad_flag);
      const auto res = writer.write(sample);
      EXPECT_FALSE(res.has_value());
      EXPECT_TRUE(ss.str().empty());
    }
  }

  // Bad / failbit ostream returns rfl::Error from write(), flush(), finish(),
  // and a failed finish() continues returning Error on retry.
  {
    std::stringstream ss;
    ss.setstate(std::ios::badbit);
    rfl::json::StreamWriter writer(ss, rfl::json::StreamFormat::ndjson);
    EXPECT_FALSE(writer.write(sample).has_value());
    EXPECT_FALSE(writer.flush().has_value());
    EXPECT_FALSE(writer.finish().has_value());
    EXPECT_FALSE(writer.finish().has_value());  // retry still returns error
  }
  {
    std::stringstream ss;
    ss.setstate(std::ios::failbit);
    rfl::json::StreamWriter writer(ss, rfl::json::StreamFormat::json_array);
    EXPECT_FALSE(writer.write(sample).has_value());
    EXPECT_FALSE(writer.finish().has_value());
    EXPECT_FALSE(writer.finish().has_value());  // retry still returns error
  }

  // eofbit alone (without failbit/badbit) does not block writing
  {
    std::stringstream ss;
    ss.setstate(std::ios::eofbit);
    rfl::json::StreamWriter writer(ss, rfl::json::StreamFormat::ndjson);
    EXPECT_TRUE(writer.write(sample).has_value());
    EXPECT_TRUE(writer.finish().has_value());
    EXPECT_TRUE(writer.flush().has_value());
    EXPECT_NE(ss.str().find("\"Alice\""), std::string::npos);
  }

  // Serialization failure (e.g. NaN without ALLOW_INF_AND_NAN) returns Error
  {
    std::stringstream ss;
    rfl::json::StreamWriter writer(ss, rfl::json::StreamFormat::ndjson);
    const Row nan_row{1, "NaN", std::numeric_limits<double>::quiet_NaN()};
    const auto res = writer.write(nan_row);
    EXPECT_FALSE(res.has_value());
    EXPECT_TRUE(ss.str().empty());
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
    for (const auto& s : expected_strs) {
      ASSERT_TRUE(writer.write(s).has_value());
    }
  }

  const std::vector<Row> expected_rows = {
      Row{100, std::string(256, 'a'), 1.25},
      Row{200, "h\xc3\xa9llo \xf0\x9f\x9a\x80", -99.5},
  };
  std::stringstream obj_ss;
  {
    rfl::json::StreamWriter writer(obj_ss, rfl::json::StreamFormat::json_array);
    for (const auto& r : expected_rows) {
      ASSERT_TRUE(writer.write(r).has_value());
    }
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

TEST(json, test_stream_live_pipe_non_blocking) {
  // NDJSON yields record 1 immediately without pulling chunk 2
  {
    ChunkedPipeBuf pipe;
    pipe.push_chunk(R"({"id":1,"name":"live1","score":1.0})" "\n");
    pipe.push_chunk(R"({"id":2,"name":"live2","score":2.0})" "\n");
    std::istream in(&pipe);
    rfl::json::StreamReader<Row> reader(in, rfl::json::StreamFormat::ndjson);

    const auto r1 = reader.next();
    ASSERT_TRUE(r1.has_value() && r1->has_value());
    EXPECT_EQ(**r1, (Row{1, "live1", 1.0}));
    EXPECT_EQ(pipe.underflow_count(), 1u);

    const auto r2 = reader.next();
    ASSERT_TRUE(r2.has_value() && r2->has_value());
    EXPECT_EQ(**r2, (Row{2, "live2", 2.0}));
    EXPECT_EQ(pipe.underflow_count(), 2u);
  }

  // json_seq yields record 1 immediately without pulling chunk 2
  {
    ChunkedPipeBuf pipe;
    pipe.push_chunk("\x1e" R"({"id":1,"name":"seq1","score":1.0})" "\n");
    pipe.push_chunk("\x1e" R"({"id":2,"name":"seq2","score":2.0})" "\n");
    std::istream in(&pipe);
    rfl::json::StreamReader<Row> reader(in, rfl::json::StreamFormat::json_seq);

    const auto r1 = reader.next();
    ASSERT_TRUE(r1.has_value() && r1->has_value());
    EXPECT_EQ(**r1, (Row{1, "seq1", 1.0}));
    EXPECT_EQ(pipe.underflow_count(), 1u);

    const auto r2 = reader.next();
    ASSERT_TRUE(r2.has_value() && r2->has_value());
    EXPECT_EQ(**r2, (Row{2, "seq2", 2.0}));
    EXPECT_EQ(pipe.underflow_count(), 2u);
  }

  // json_array yields element 1 immediately without pulling chunk 2
  {
    ChunkedPipeBuf pipe;
    pipe.push_chunk(R"([{"id":1,"name":"arr1","score":1.0},)");
    pipe.push_chunk(R"({"id":2,"name":"arr2","score":2.0}])");
    std::istream in(&pipe);
    rfl::json::StreamReader<Row> reader(in,
                                        rfl::json::StreamFormat::json_array);

    const auto r1 = reader.next();
    ASSERT_TRUE(r1.has_value() && r1->has_value());
    EXPECT_EQ(**r1, (Row{1, "arr1", 1.0}));
    EXPECT_EQ(pipe.underflow_count(), 1u);

    const auto r2 = reader.next();
    ASSERT_TRUE(r2.has_value() && r2->has_value());
    EXPECT_EQ(**r2, (Row{2, "arr2", 2.0}));
    EXPECT_EQ(pipe.underflow_count(), 2u);
    EXPECT_FALSE(reader.next().has_value());
  }
}

TEST(json, test_stream_processors_and_move_only) {
  const std::vector<SnakeRow> expected = {{1, "Ada"}, {2, "Grace"}};
  std::stringstream ss;
  {
    rfl::json::StreamWriter<rfl::SnakeCaseToCamelCase> writer(ss);
    for (const auto& r : expected) {
      ASSERT_TRUE(writer.write(r).has_value());
    }
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

TEST(json, test_stream_syntax_error_recovery) {
  // NDJSON recovers after malformed line and reads subsequent valid records
  const std::string bad_ndjson =
      R"({"id":1,"name":"first","score":1.0})" "\n"
      "{not valid json}\n"
      R"({"id":2,"name":"second","score":2.0})" "\n"
      R"({"id":3,"name":"trunc)" "\n"
      R"({"id":4,"name":"fourth","score":4.0})" "\n";

  for (const bool use_stream : {false, true}) {
    std::istringstream in(bad_ndjson);
    auto reader =
        use_stream
            ? rfl::json::StreamReader<Row>(in, rfl::json::StreamFormat::ndjson,
                                           0, 11)
            : rfl::json::StreamReader<Row>(std::string_view{bad_ndjson},
                                           rfl::json::StreamFormat::ndjson);

    const auto r1 = reader.next();
    ASSERT_TRUE(r1.has_value() && r1->has_value());
    EXPECT_EQ(**r1, (Row{1, "first", 1.0}));

    const auto r2 = reader.next();
    ASSERT_TRUE(r2.has_value());
    EXPECT_FALSE(r2->has_value());

    const auto r3 = reader.next();
    ASSERT_TRUE(r3.has_value() && r3->has_value());
    EXPECT_EQ(**r3, (Row{2, "second", 2.0}));

    const auto r4 = reader.next();
    ASSERT_TRUE(r4.has_value());
    EXPECT_FALSE(r4->has_value());

    const auto r5 = reader.next();
    ASSERT_TRUE(r5.has_value() && r5->has_value());
    EXPECT_EQ(**r5, (Row{4, "fourth", 4.0}));

    EXPECT_FALSE(reader.next().has_value());
  }

  // RFC 7464 json_seq resyncs to next \x1e across malformed/multi-line garbage
  const std::string bad_seq =
      "\x1e" R"({"id":1,"name":"ok1","score":1.0})" "\n"
      "\x1e{bad\nstill bad}\n"
      "\x1e" R"({"id":2,"name":"trunc)"
      "\x1e" R"({"id":3,"name":"ok3","score":3.0})" "\n";

  for (const bool use_stream : {false, true}) {
    std::istringstream in(bad_seq);
    auto reader =
        use_stream
            ? rfl::json::StreamReader<Row>(
                  in, rfl::json::StreamFormat::json_seq, 0, 11)
            : rfl::json::StreamReader<Row>(std::string_view{bad_seq},
                                           rfl::json::StreamFormat::json_seq);

    const auto r1 = reader.next();
    ASSERT_TRUE(r1.has_value() && r1->has_value());
    EXPECT_EQ(**r1, (Row{1, "ok1", 1.0}));

    const auto r2 = reader.next();
    ASSERT_TRUE(r2.has_value());
    EXPECT_FALSE(r2->has_value());

    const auto r3 = reader.next();
    ASSERT_TRUE(r3.has_value());
    EXPECT_FALSE(r3->has_value());

    const auto r4 = reader.next();
    ASSERT_TRUE(r4.has_value() && r4->has_value());
    EXPECT_EQ(**r4, (Row{3, "ok3", 3.0}));

    EXPECT_FALSE(reader.next().has_value());
  }
}

TEST(json, test_stream_max_record_bytes) {
  const std::string big_ndjson =
      R"({"id":1,"name":"ok","score":1.0})" "\n"
      "{\"id\":2,\"name\":\"" +
      std::string(200, 'x') + "\",\"score\":2.0}\n";

  // Small cap rejects oversized record in ndjson (both string_view & istream)
  {
    rfl::json::StreamReader<Row> reader(std::string_view{big_ndjson},
                                        rfl::json::StreamFormat::ndjson, 0, 64);
    const auto r1 = reader.next();
    ASSERT_TRUE(r1.has_value() && r1->has_value());
    EXPECT_EQ(**r1, (Row{1, "ok", 1.0}));

    const auto r2 = reader.next();
    ASSERT_TRUE(r2.has_value());
    EXPECT_FALSE(r2->has_value());
    EXPECT_FALSE(reader.next().has_value());
  }
  {
    std::istringstream in(big_ndjson);
    rfl::json::StreamReader<Row> reader(in, rfl::json::StreamFormat::ndjson, 0,
                                        16, 64);
    const auto r1 = reader.next();
    ASSERT_TRUE(r1.has_value() && r1->has_value());

    const auto r2 = reader.next();
    ASSERT_TRUE(r2.has_value());
    EXPECT_FALSE(r2->has_value());
    EXPECT_FALSE(reader.next().has_value());
  }

  // Cap enforced in json_array as well
  const std::string big_array =
      R"([{"id":1,"name":"ok","score":1.0},{"id":2,"name":")" +
      std::string(200, 'y') + R"(","score":2.0}])";
  {
    std::istringstream in(big_array);
    rfl::json::StreamReader<Row> reader(in, rfl::json::StreamFormat::json_array,
                                        0, 16, 64);
    const auto r1 = reader.next();
    ASSERT_TRUE(r1.has_value() && r1->has_value());
    const auto r2 = reader.next();
    ASSERT_TRUE(r2.has_value());
    EXPECT_FALSE(r2->has_value());
    EXPECT_FALSE(reader.next().has_value());
  }

  // 0 means unlimited
  {
    std::istringstream in(big_ndjson);
    rfl::json::StreamReader<Row> reader(in, rfl::json::StreamFormat::ndjson, 0,
                                        16, 0);
    const auto rows = collect_ok(reader);
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[1].name.size(), 200u);
  }
}

TEST(json, test_stream_bom_and_comments) {
  const std::string bom_and_comments_array =
      "\xEF\xBB\xBF"
      "/* leading comment */\n"
      "[ // after open\n"
      "  {\"id\":1,\"name\":\"A\",\"score\":1.0} /* mid */,\n"
      "  // before second\n"
      "  {\"id\":2,\"name\":\"B\",\"score\":2.0}\n"
      "] /* trailing */";

  const std::vector<Row> expected{{1, "A", 1.0}, {2, "B", 2.0}};

  // Works with ALLOW_BOM | ALLOW_COMMENTS (including tiny chunk_size = 1)
  {
    rfl::json::StreamReader<Row> sv_reader(
        std::string_view{bom_and_comments_array},
        rfl::json::StreamFormat::json_array,
        YYJSON_READ_ALLOW_BOM | YYJSON_READ_ALLOW_COMMENTS);
    EXPECT_EQ(collect_ok(sv_reader), expected);

    std::istringstream in(bom_and_comments_array);
    rfl::json::StreamReader<Row> in_reader(
        in, rfl::json::StreamFormat::json_array,
        YYJSON_READ_ALLOW_BOM | YYJSON_READ_ALLOW_COMMENTS, 1);
    EXPECT_EQ(collect_ok(in_reader), expected);
  }

  // Rejected without ALLOW_BOM
  {
    rfl::json::StreamReader<Row> reader(
        std::string_view{bom_and_comments_array},
        rfl::json::StreamFormat::json_array, YYJSON_READ_ALLOW_COMMENTS);
    const auto res = reader.next();
    ASSERT_TRUE(res.has_value());
    EXPECT_FALSE(res->has_value());
  }

  // Unclosed multi-line comment returns error
  {
    const std::string unclosed = "[1 /* unclosed";
    rfl::json::StreamReader<int> reader(std::string_view{unclosed},
                                        rfl::json::StreamFormat::json_array,
                                        YYJSON_READ_ALLOW_COMMENTS);
    const auto r1 = reader.next();
    ASSERT_TRUE(r1.has_value() && r1->has_value());
    EXPECT_EQ(**r1, 1);
    const auto r2 = reader.next();
    ASSERT_TRUE(r2.has_value());
    EXPECT_FALSE(r2->has_value());
  }

  // BOM + comments also work in ndjson
  {
    const std::string bom_ndjson =
        "\xEF\xBB\xBF"
        "// header\n"
        "{\"id\":1,\"name\":\"A\",\"score\":1.0}\n"
        "/* between */\n"
        "{\"id\":2,\"name\":\"B\",\"score\":2.0}\n";
    std::istringstream in(bom_ndjson);
    rfl::json::StreamReader<Row> reader(
        in, rfl::json::StreamFormat::ndjson,
        YYJSON_READ_ALLOW_BOM | YYJSON_READ_ALLOW_COMMENTS, 1);
    EXPECT_EQ(collect_ok(reader), expected);
  }
}

TEST(json, test_stream_linear_performance_sanity) {
  constexpr int kCount = 10000;
  std::stringstream nd_ss;
  std::stringstream seq_ss;
  std::stringstream arr_ss;
  {
    rfl::json::StreamWriter nd_w(nd_ss, rfl::json::StreamFormat::ndjson);
    rfl::json::StreamWriter seq_w(seq_ss, rfl::json::StreamFormat::json_seq);
    rfl::json::StreamWriter arr_w(arr_ss, rfl::json::StreamFormat::json_array);
    for (int i = 0; i < kCount; ++i) {
      const Row row{i, "record_payload", static_cast<double>(i) * 0.5};
      ASSERT_TRUE(nd_w.write(row).has_value());
      ASSERT_TRUE(seq_w.write(row).has_value());
      ASSERT_TRUE(arr_w.write(row).has_value());
    }
  }

  const std::string nd_payload = nd_ss.str();
  const std::string seq_payload = seq_ss.str();
  const std::string arr_payload = arr_ss.str();

  auto measure_ms = [](auto&& _fn) {
    const auto t0 = std::chrono::steady_clock::now();
    _fn();
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0)
        .count();
  };

  // O(N^2) yyjson copy would take many seconds on 10,000 records (~500 KB);
  // windowed O(N) parsing completes well within budget.
  constexpr long long kBudgetMs = 2000;

  {
    int count = 0;
    const auto ms = measure_ms([&] {
      rfl::json::StreamReader<Row> reader(std::string_view{nd_payload},
                                          rfl::json::StreamFormat::ndjson);
      for (auto&& res : reader) {
        ASSERT_TRUE(res.has_value());
        ++count;
      }
    });
    EXPECT_EQ(count, kCount);
    EXPECT_LT(ms, kBudgetMs) << "ndjson string_view took " << ms << "ms";
  }

  {
    int count = 0;
    const auto ms = measure_ms([&] {
      std::istringstream in(nd_payload);
      rfl::json::StreamReader<Row> reader(in, rfl::json::StreamFormat::ndjson);
      for (auto&& res : reader) {
        ASSERT_TRUE(res.has_value());
        ++count;
      }
    });
    EXPECT_EQ(count, kCount);
    EXPECT_LT(ms, kBudgetMs) << "ndjson istream took " << ms << "ms";
  }

  {
    int count = 0;
    const auto ms = measure_ms([&] {
      rfl::json::StreamReader<Row> reader(std::string_view{seq_payload},
                                          rfl::json::StreamFormat::json_seq);
      for (auto&& res : reader) {
        ASSERT_TRUE(res.has_value());
        ++count;
      }
    });
    EXPECT_EQ(count, kCount);
    EXPECT_LT(ms, kBudgetMs) << "json_seq string_view took " << ms << "ms";
  }

  {
    int count = 0;
    const auto ms = measure_ms([&] {
      rfl::json::StreamReader<Row> reader(std::string_view{arr_payload},
                                          rfl::json::StreamFormat::json_array);
      for (auto&& res : reader) {
        ASSERT_TRUE(res.has_value());
        ++count;
      }
    });
    EXPECT_EQ(count, kCount);
    EXPECT_LT(ms, kBudgetMs) << "json_array string_view took " << ms << "ms";
  }

  {
    int count = 0;
    const auto ms = measure_ms([&] {
      std::istringstream in(arr_payload);
      rfl::json::StreamReader<Row> reader(in,
                                          rfl::json::StreamFormat::json_array);
      for (auto&& res : reader) {
        ASSERT_TRUE(res.has_value());
        ++count;
      }
    });
    EXPECT_EQ(count, kCount);
    EXPECT_LT(ms, kBudgetMs) << "json_array istream took " << ms << "ms";
  }

  // B2: Large single record (256 KiB) with tiny initial chunk_size (16 bytes)
  // doubles refill step so parsing is O(R), not O(R^2 / chunk_size).
  {
    const Row big_row{42, std::string(256 * 1024, 'z'), 3.14};
    std::stringstream big_ss;
    {
      rfl::json::StreamWriter w(big_ss, rfl::json::StreamFormat::json_array);
      ASSERT_TRUE(w.write(big_row).has_value());
    }
    const std::string big_payload = big_ss.str();
    const auto ms = measure_ms([&] {
      std::istringstream in(big_payload);
      rfl::json::StreamReader<Row> reader(in,
                                          rfl::json::StreamFormat::json_array,
                                          0, 16);
      const auto res = reader.next();
      ASSERT_TRUE(res.has_value() && res->has_value());
      EXPECT_EQ(**res, big_row);
      EXPECT_FALSE(reader.next().has_value());
    });
    EXPECT_LT(ms, kBudgetMs) << "large record doubling took " << ms << "ms";
  }
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

TEST(json, test_stream_eof_after_compaction_regression) {
  // Blocker #1 repro: default chunk_size, no trailing '\n' on last record.
  {
    std::istringstream in("{\"id\":1}\n{\"id\":22}");
    rfl::json::StreamReader<IdOnly> reader(in, rfl::json::StreamFormat::ndjson);
    EXPECT_EQ(collect_ok(reader), (std::vector<IdOnly>{{1}, {22}}));
  }

  // Primitive at EOF after compaction (forces refill_raw -> compact_buffer -> EOF).
  {
    std::istringstream in("11111111\n22");
    rfl::json::StreamReader<int> reader(in, rfl::json::StreamFormat::ndjson);
    EXPECT_EQ(collect_ok(reader), (std::vector<int>{11111111, 22}));
  }

  // Truncated record at EOF after compaction.
  {
    std::istringstream in("{\"id\":1}\n{\"id\":22");
    rfl::json::StreamReader<IdOnly> reader(in, rfl::json::StreamFormat::ndjson);
    const auto r1 = reader.next();
    ASSERT_TRUE(r1.has_value() && r1->has_value());
    EXPECT_EQ(**r1, (IdOnly{1}));
    const auto r2 = reader.next();
    ASSERT_TRUE(r2.has_value());
    EXPECT_FALSE(r2->has_value());
    EXPECT_FALSE(reader.next().has_value());
  }
}

TEST(json, test_stream_exact_max_record_bytes_crlf) {
  // Blocker #3 repro: exact-size record (8 bytes) followed by \r\n.
  const std::string crlf_objs = "{\"id\":1}\r\n{\"id\":2}\r\n";
  const std::string crlf_seq = "\x1e{\"id\":1}\r\n\x1e{\"id\":2}\r\n";
  const std::string crlf_arr = "[{\"id\":1}\r\n,{\"id\":2}\r\n]";
  const std::vector<IdOnly> expected_ids{{1}, {2}};

  for (const size_t chunk :
       {size_t{1}, size_t{3}, size_t{8}, size_t{9}, size_t{65536}}) {
    {
      std::istringstream in(crlf_objs);
      rfl::json::StreamReader<IdOnly> reader(
          in, rfl::json::StreamFormat::ndjson, 0, chunk, 8);
      EXPECT_EQ(collect_ok(reader), expected_ids) << "ndjson chunk=" << chunk;
    }
    {
      std::istringstream in(crlf_seq);
      rfl::json::StreamReader<IdOnly> reader(
          in, rfl::json::StreamFormat::json_seq, 0, chunk, 8);
      EXPECT_EQ(collect_ok(reader), expected_ids) << "json_seq chunk=" << chunk;
    }
    {
      std::istringstream in(crlf_arr);
      rfl::json::StreamReader<IdOnly> reader(
          in, rfl::json::StreamFormat::json_array, 0, chunk, 8);
      EXPECT_EQ(collect_ok(reader), expected_ids)
          << "json_array chunk=" << chunk;
    }
  }

  // Exact-size string ("abcdef" = 8 bytes) and primitive (12345678 = 8 bytes) + \r\n
  for (const size_t chunk : {size_t{1}, size_t{8}, size_t{65536}}) {
    {
      std::istringstream in("\"abcdef\"\r\n\"ghijkl\"\r\n");
      rfl::json::StreamReader<std::string> reader(
          in, rfl::json::StreamFormat::ndjson, 0, chunk, 8);
      EXPECT_EQ(collect_ok(reader),
                (std::vector<std::string>{"abcdef", "ghijkl"}));
    }
    {
      std::istringstream in("12345678\r\n87654321\r\n");
      rfl::json::StreamReader<int> reader(
          in, rfl::json::StreamFormat::ndjson, 0, chunk, 8);
      EXPECT_EQ(collect_ok(reader), (std::vector<int>{12345678, 87654321}));
    }
  }

  // string_view overload with exact-size record + \r\n
  {
    rfl::json::StreamReader<IdOnly> reader(
        std::string_view{crlf_objs}, rfl::json::StreamFormat::ndjson, 0, 8);
    EXPECT_EQ(collect_ok(reader), expected_ids);
  }

  // 1 byte over cap (max_record_bytes = 7 on 8-byte record) must fail
  {
    std::istringstream in(crlf_objs);
    rfl::json::StreamReader<IdOnly> reader(
        in, rfl::json::StreamFormat::ndjson, 0, 65536, 7);
    const auto r = reader.next();
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(r->has_value());
    EXPECT_FALSE(reader.next().has_value());
  }
  {
    std::istringstream in("12345678\r\n");
    rfl::json::StreamReader<int> reader(
        in, rfl::json::StreamFormat::ndjson, 0, 65536, 7);
    const auto r = reader.next();
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(r->has_value());
    EXPECT_FALSE(reader.next().has_value());
  }
}

TEST(json, test_stream_4kib_chunks_and_unbuffered_linear) {
  auto feed_in_chunks = [](ChunkedPipeBuf& _pipe, const std::string& _payload,
                           const size_t _chunk_bytes) {
    for (size_t off = 0; off < _payload.size(); off += _chunk_bytes) {
      _pipe.push_chunk(_payload.substr(off, _chunk_bytes));
    }
  };

  auto measure_ms = [](auto&& _fn) {
    const auto t0 = std::chrono::steady_clock::now();
    _fn();
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0)
        .count();
  };

  // 512 KiB single record fed in 4 KiB chunks (simulates std::ifstream / pipe).
  // True O(R) incremental scanner invokes yyjson_read_opts once when record closes.
  const Row big_row{99, std::string(512 * 1024, 'q'), 2.75};
  constexpr long long kBudgetMs = 2000;

  for (const auto fmt :
       {rfl::json::StreamFormat::ndjson, rfl::json::StreamFormat::json_seq,
        rfl::json::StreamFormat::json_array}) {
    std::stringstream ss;
    {
      rfl::json::StreamWriter w(ss, fmt);
      ASSERT_TRUE(w.write(big_row).has_value());
    }
    const std::string payload = ss.str();

    ChunkedPipeBuf pipe;
    feed_in_chunks(pipe, payload, 4096);
    std::istream in(&pipe);

    const auto ms = measure_ms([&] {
      rfl::json::StreamReader<Row> reader(in, fmt);
      const auto res = reader.next();
      ASSERT_TRUE(res.has_value() && res->has_value())
          << (res.has_value() ? res->error().what() : "unexpected eof");
      EXPECT_EQ(**res, big_row);
      EXPECT_FALSE(reader.next().has_value());
    });
    EXPECT_LT(ms, kBudgetMs) << "4KiB chunked 512KiB record took " << ms << "ms";
  }

  // Multi-line pretty-printed 256 KiB record in 4 KiB chunks (json_seq)
  {
    const Row pretty_big{7, std::string(256 * 1024, 'p'), 1.5};
    const std::string payload =
        std::string("\x1e") + rfl::json::write(pretty_big, rfl::json::pretty) +
        "\n";
    ChunkedPipeBuf pipe;
    feed_in_chunks(pipe, payload, 4096);
    std::istream in(&pipe);

    const auto ms = measure_ms([&] {
      rfl::json::StreamReader<Row> reader(in,
                                          rfl::json::StreamFormat::json_seq);
      const auto res = reader.next();
      ASSERT_TRUE(res.has_value() && res->has_value());
      EXPECT_EQ(**res, pretty_big);
      EXPECT_FALSE(reader.next().has_value());
    });
    EXPECT_LT(ms, kBudgetMs) << "4KiB chunked pretty record took " << ms << "ms";
  }

  // Unbuffered 1-byte-at-a-time stream (in_avail() == 0) in json_array & ndjson
  {
    const Row med_row{11, std::string(64 * 1024, 'u'), -4.5};
    std::stringstream arr_ss;
    {
      rfl::json::StreamWriter w(arr_ss, rfl::json::StreamFormat::json_array);
      ASSERT_TRUE(w.write(med_row).has_value());
    }
    UnbufferedBuf ubuf(arr_ss.str());
    std::istream in(&ubuf);
    const auto ms = measure_ms([&] {
      rfl::json::StreamReader<Row> reader(in,
                                          rfl::json::StreamFormat::json_array);
      const auto res = reader.next();
      ASSERT_TRUE(res.has_value() && res->has_value());
      EXPECT_EQ(**res, med_row);
      EXPECT_FALSE(reader.next().has_value());
    });
    EXPECT_LT(ms, kBudgetMs) << "unbuffered 64KiB json_array took " << ms << "ms";
  }

  // Non-blocking primitives on live pipe
  {
    ChunkedPipeBuf pipe;
    pipe.push_chunk("123\n");
    pipe.push_chunk("456\n");
    std::istream in(&pipe);
    rfl::json::StreamReader<int> reader(in, rfl::json::StreamFormat::ndjson);
    const auto r1 = reader.next();
    ASSERT_TRUE(r1.has_value() && r1->has_value());
    EXPECT_EQ(**r1, 123);
    EXPECT_EQ(pipe.underflow_count(), 1u);
    const auto r2 = reader.next();
    ASSERT_TRUE(r2.has_value() && r2->has_value());
    EXPECT_EQ(**r2, 456);
    EXPECT_EQ(pipe.underflow_count(), 2u);
  }

  // Comments containing braces/quotes inside containers (json_seq) + strict
  // single-line NDJSON recovery & linear-time DoS resistance
  {
    const std::string with_inner_comments =
        "\x1e{\n"
        "  /* } ] \"tricky\" */ \"id\": 1,\n"
        "  // another } brace and \" quote\n"
        "  \"name\": \"ok\", \"score\": 1.5\n"
        "}\n";
    std::istringstream in(with_inner_comments);
    rfl::json::StreamReader<Row> reader(in, rfl::json::StreamFormat::json_seq,
                                        YYJSON_READ_ALLOW_COMMENTS, 7);
    EXPECT_EQ(collect_ok(reader), (std::vector<Row>{{1, "ok", 1.5}}));
  }
  {
    const std::string unclosed_bad_line =
        "{not valid\n"
        "{\"id\":2,\"name\":\"recovered\",\"score\":2.5}\n";
    std::istringstream in(unclosed_bad_line);
    rfl::json::StreamReader<Row> reader(in, rfl::json::StreamFormat::ndjson);
    const auto r1 = reader.next();
    ASSERT_TRUE(r1.has_value());
    EXPECT_FALSE(r1->has_value());
    const auto r2 = reader.next();
    ASSERT_TRUE(r2.has_value() && r2->has_value());
    EXPECT_EQ(**r2, (Row{2, "recovered", 2.5}));
    EXPECT_FALSE(reader.next().has_value());
  }
  // Regression: unclosed NDJSON line followed by > max_record_bytes valid lines
  // recovers all valid records without hitting fatal max_record_bytes.
  {
    std::string payload = "{\"id\":0,\n";
    for (int i = 1; i <= 1000; ++i) {
      payload += "{\"id\":" + std::to_string(i) +
                 ",\"name\":\"ok\",\"score\":1.0}\n";
    }
    std::istringstream in(payload);
    rfl::json::StreamReader<Row> reader(in, rfl::json::StreamFormat::ndjson, 0,
                                        65536, 4096);
    int errs = 0;
    int ok = 0;
    while (const auto res = reader.next()) {
      if (res->has_value()) {
        ++ok;
      } else {
        ++errs;
      }
    }
    EXPECT_EQ(errs, 1);
    EXPECT_EQ(ok, 1000);
  }
  // Regression: 4,000 unclosed "{\n" lines + 4,000 valid records finishes in
  // linear time (no O(N^2) rescanning DoS).
  {
    std::string payload;
    payload.reserve(160000);
    for (int i = 0; i < 4000; ++i) {
      payload += "{\n";
    }
    for (int i = 0; i < 4000; ++i) {
      payload += "{\"id\":" + std::to_string(i) +
                 ",\"name\":\"r\",\"score\":1.0}\n";
    }
    int errs = 0;
    int ok = 0;
    const auto ms = measure_ms([&] {
      std::istringstream in(payload);
      rfl::json::StreamReader<Row> reader(in, rfl::json::StreamFormat::ndjson);
      while (const auto res = reader.next()) {
        if (res->has_value()) {
          ++ok;
        } else {
          ++errs;
        }
      }
    });
    EXPECT_EQ(errs, 4000);
    EXPECT_EQ(ok, 4000);
    EXPECT_LT(ms, kBudgetMs) << "4000 unclosed NDJSON lines took " << ms << "ms";
  }
}

}  // namespace test_stream
