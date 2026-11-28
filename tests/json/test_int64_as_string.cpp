#include <cstdint>
#include <limits>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <vector>

#include "write_and_read.hpp"

namespace test_int64_as_string {

struct Trade {
  int64_t trade_id;
  uint64_t sequence;
  int32_t quantity;
  double price;
};

TEST(json, test_int64_as_string_roundtrip) {
  const auto trade = Trade{
      .trade_id = 9007199254740993LL,
      .sequence = std::numeric_limits<uint64_t>::max(),
      .quantity = 42,
      .price = 12.5,
  };

  // With rfl::Int64AsString: 64-bit ints become strings, 32-bit/double stay numbers.
  write_and_read<rfl::Int64AsString>(
      trade,
      R"({"trade_id":"9007199254740993","sequence":"18446744073709551615","quantity":42,"price":12.5})");

  // Default behavior (without processor) is completely unchanged.
  write_and_read(
      trade,
      R"({"trade_id":9007199254740993,"sequence":18446744073709551615,"quantity":42,"price":12.5})");
}

TEST(json, test_int64_as_string_nested_optional_vector_validator) {
  struct Envelope {
    std::optional<int64_t> min_id;
    std::vector<uint64_t> ids;
    rfl::Validator<int64_t, rfl::Minimum<1>> positive_id;
  };

  const auto env = Envelope{
      .min_id = std::numeric_limits<int64_t>::min(),
      .ids = {0ULL, 9007199254740993ULL},
      .positive_id = 9007199254740993LL,
  };

  write_and_read<rfl::Int64AsString>(
      env,
      R"({"min_id":"-9223372036854775808","ids":["0","9007199254740993"],"positive_id":"9007199254740993"})");

  // Validator still rejects values failing the numeric constraint.
  EXPECT_FALSE((rfl::json::read<Envelope, rfl::Int64AsString>(
      R"({"ids":[],"positive_id":"0"})")));

  // Works inside rfl::Processors<...> alongside other processors.
  using Procs = rfl::Processors<rfl::SnakeCaseToCamelCase, rfl::Int64AsString>;
  const auto trade = Trade{
      .trade_id = -9223372036854775807LL,
      .sequence = 1ULL,
      .quantity = 3,
      .price = 1.5,
  };
  write_and_read<Procs>(
      trade,
      R"({"tradeId":"-9223372036854775807","sequence":"1","quantity":3,"price":1.5})");
}

TEST(json, test_int64_as_string_errors_and_schema) {
  struct SingleSigned {
    int64_t v;
  };
  struct SingleUnsigned {
    uint64_t v;
  };

  // Bare JSON number is rejected when Int64AsString is active.
  EXPECT_FALSE((rfl::json::read<SingleSigned, rfl::Int64AsString>(R"({"v":42})")));
  // Syntax and overflow errors.
  EXPECT_FALSE((rfl::json::read<SingleSigned, rfl::Int64AsString>(R"({"v":""})")));
  EXPECT_FALSE((rfl::json::read<SingleSigned, rfl::Int64AsString>(R"({"v":" 42"})")));
  EXPECT_FALSE((rfl::json::read<SingleSigned, rfl::Int64AsString>(R"({"v":"42abc"})")));
  EXPECT_FALSE((rfl::json::read<SingleSigned, rfl::Int64AsString>(
      R"({"v":"9223372036854775808"})")));
  EXPECT_FALSE((rfl::json::read<SingleSigned, rfl::Int64AsString>(
      R"({"v":"-9223372036854775809"})")));
  EXPECT_FALSE((rfl::json::read<SingleUnsigned, rfl::Int64AsString>(
      R"({"v":"18446744073709551616"})")));
  EXPECT_FALSE((rfl::json::read<SingleUnsigned, rfl::Int64AsString>(
      R"({"v":"-1"})")));

  // JSON schema emits OpenAPI/Protobuf-JSON string+format hints for 64-bit
  // integers and leaves narrower integers as plain "integer".
  const auto schema = rfl::json::to_schema<Trade, rfl::Int64AsString>();
  EXPECT_NE(schema.find(R"("trade_id":{"type":"string","format":"int64"})"),
            std::string::npos);
  EXPECT_NE(schema.find(R"("sequence":{"type":"string","format":"uint64"})"),
            std::string::npos);
  EXPECT_NE(schema.find(R"("quantity":{"type":"integer"})"), std::string::npos);
}

}  // namespace test_int64_as_string
