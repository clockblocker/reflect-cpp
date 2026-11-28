#include <gtest/gtest.h>

#include <rfl/nlohmann.hpp>

#ifdef YYJSON_VERSION_MAJOR
#error "<rfl/nlohmann.hpp> must not include yyjson"
#endif

namespace test_nlohmann_standalone {

struct StandaloneItem {
  std::string name;
  uint64_t count;
  nlohmann::json extra;
};

struct StandaloneOptIn {
  int x;
  std::string label;
};

}  // namespace test_nlohmann_standalone

template <>
struct rfl::nlohmann::enable_adl_serializer<
    test_nlohmann_standalone::StandaloneOptIn> : std::true_type {};

namespace test_nlohmann_standalone {

TEST(nlohmann_standalone, zero_yyjson_coupling) {
  const StandaloneItem item{
      .name = "widget",
      .count = 18446744073709551615ULL,
      .extra = nlohmann::json{{"nested", {1, true, "ok", nullptr}}},
  };
  const nlohmann::json j = rfl::nlohmann::write(item);
  const auto parsed = rfl::nlohmann::read<StandaloneItem>(j);
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(parsed->name, item.name);
  EXPECT_EQ(parsed->count, item.count);
  EXPECT_EQ(parsed->extra, item.extra);

  const StandaloneOptIn opt{.x = 7, .label = "solo"};
  const nlohmann::json oj = opt;
  EXPECT_EQ(oj.get<StandaloneOptIn>().x, 7);
  EXPECT_EQ(oj.get<StandaloneOptIn>().label, "solo");
}

}  // namespace test_nlohmann_standalone
