#include <gtest/gtest.h>
#include <array>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <rfl/json.hpp>
#include <set>
#include <string>
#include <vector>

namespace test_blank_list_items {

using Blanks = rfl::cli::BlankItemsAsMissing;
using Path = std::vector<rfl::ErrorDetail::PathSegment>;

struct Config {
  std::vector<std::optional<std::string>> tags;
};

TEST(cli, test_blank_items_skipped_by_default) {
  const char* args[] = {"program", "--tags=,a,,b,"};
  const auto result = rfl::cli::read<Config>(2, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().tags,
            (std::vector<std::optional<std::string>>{"a", "b"}));
}

TEST(cli, test_blank_items_optional) {
  const char* args[] = {"program", "--tags=a,,b"};
  const auto result =
      rfl::cli::read<Config, Blanks>(2, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().tags,
            (std::vector<std::optional<std::string>>{"a", std::nullopt, "b"}));
}

TEST(cli, test_blank_items_leading_and_trailing) {
  const char* args[] = {"program", "--tags=,a,"};
  const auto result =
      rfl::cli::read<Config, Blanks>(2, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().tags,
            (std::vector<std::optional<std::string>>{std::nullopt, "a",
                                                     std::nullopt}));
}

TEST(cli, test_blank_items_empty_value) {
  const char* args[] = {"program", "--tags="};
  const auto result =
      rfl::cli::read<Config, Blanks>(2, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().tags.empty());
}

struct DefaultValConfig {
  std::vector<rfl::DefaultVal<std::string>> names;
  std::vector<rfl::DefaultVal<int>> counts;
};

TEST(cli, test_blank_items_default_val) {
  const char* args[] = {"program", "--names=a,,b", "--counts=,2"};
  const auto result =
      rfl::cli::read<DefaultValConfig, Blanks>(3, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  const auto& names = result.value().names;
  ASSERT_EQ(names.size(), 3u);
  EXPECT_EQ(names[0].value(), "a");
  EXPECT_EQ(names[1].value(), "");
  EXPECT_EQ(names[2].value(), "b");
  const auto& counts = result.value().counts;
  ASSERT_EQ(counts.size(), 2u);
  EXPECT_EQ(counts[0].value(), 0);
  EXPECT_EQ(counts[1].value(), 2);
}

struct RequiredConfig {
  std::vector<std::string> tags;
};

TEST(cli, test_blank_items_required_error) {
  const char* args[] = {"program", "--tags=a,,b"};
  const auto result =
      rfl::cli::read<RequiredConfig, Blanks>(2, const_cast<char**>(args));
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().what(),
            "Failed to parse field 'tags': Blank item at index 1 of '--tags'.");
  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 1u);
  EXPECT_EQ(details[0].path, (Path{"tags", size_t{1}}));
  EXPECT_EQ(details[0].message, "Blank item at index 1 of '--tags'.");
}

struct BoolConfig {
  std::vector<bool> flags;
};

TEST(cli, test_blank_items_required_bool_error) {
  // A blank bool is not read as `true`, unlike `--flag` or `--flag=`.
  const char* args[] = {"program", "--flags=true,"};
  const auto result =
      rfl::cli::read<BoolConfig, Blanks>(2, const_cast<char**>(args));
  ASSERT_FALSE(result);
  EXPECT_EQ(
      result.error().what(),
      "Failed to parse field 'flags': Blank item at index 1 of '--flags'.");
}

struct Inner {
  std::vector<std::optional<std::string>> tags;
  std::vector<std::string> names;
};

struct Outer {
  Inner a;
};

TEST(cli, test_blank_items_nested) {
  const char* args[] = {"program", "--a.tags=x,,y", "--a.names=n"};
  const auto result =
      rfl::cli::read<Outer, Blanks>(3, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().a.tags,
            (std::vector<std::optional<std::string>>{"x", std::nullopt, "y"}));
  EXPECT_EQ(result.value().a.names, (std::vector<std::string>{"n"}));
}

TEST(cli, test_blank_items_nested_error) {
  const char* args[] = {"program", "--a.tags=x", "--a.names=,n"};
  const auto result =
      rfl::cli::read<Outer, Blanks>(3, const_cast<char**>(args));
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().what(),
            "Failed to parse field 'a': Failed to parse field 'names': Blank "
            "item at index 0 of '--a.names'.");
  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 1u);
  EXPECT_EQ(details[0].path, (Path{"a", "names", size_t{0}}));
}

struct Containers {
  std::array<std::optional<int>, 3> numbers;
  std::set<std::string> names;
};

TEST(cli, test_blank_items_other_containers) {
  const char* ok_args[] = {"program", "--numbers=1,,3", "--names=b,a"};
  const auto ok =
      rfl::cli::read<Containers, Blanks>(3, const_cast<char**>(ok_args));
  ASSERT_TRUE(ok) << ok.error().what();
  EXPECT_EQ(ok.value().numbers,
            (std::array<std::optional<int>, 3>{1, std::nullopt, 3}));
  EXPECT_EQ(ok.value().names, (std::set<std::string>{"a", "b"}));

  const char* err_args[] = {"program", "--numbers=1,2,3", "--names=b,,a"};
  const auto err =
      rfl::cli::read<Containers, Blanks>(3, const_cast<char**>(err_args));
  ASSERT_FALSE(err);
  EXPECT_EQ(
      err.error().what(),
      "Failed to parse field 'names': Blank item at index 1 of '--names'.");
}

struct WideConfig {
  std::vector<std::wstring> names;
  std::vector<std::optional<std::wstring>> tags;
};

TEST(cli, test_blank_items_wstring) {
  // A blank std::wstring is an error, as a blank std::string is, not "".
  const char* ok_args[] = {"program", "--names=a,b", "--tags=x,,y"};
  const auto ok =
      rfl::cli::read<WideConfig, Blanks>(3, const_cast<char**>(ok_args));
  ASSERT_TRUE(ok) << ok.error().what();
  EXPECT_EQ(ok.value().names, (std::vector<std::wstring>{L"a", L"b"}));
  EXPECT_EQ(ok.value().tags, (std::vector<std::optional<std::wstring>>{
                                 L"x", std::nullopt, L"y"}));

  const char* err_args[] = {"program", "--names=a,,b", "--tags=x"};
  const auto err =
      rfl::cli::read<WideConfig, Blanks>(3, const_cast<char**>(err_args));
  ASSERT_FALSE(err);
  EXPECT_EQ(
      err.error().what(),
      "Failed to parse field 'names': Blank item at index 1 of '--names'.");
  const auto details = err.error().details();
  ASSERT_EQ(details.size(), 1u);
  EXPECT_EQ(details[0].path, (Path{"names", size_t{1}}));
}

struct PointerConfig {
  std::vector<std::shared_ptr<std::string>> shared;
  std::vector<std::unique_ptr<int>> unique;
};

TEST(cli, test_blank_items_smart_pointers) {
  // A smart pointer is never required, so a blank one is null, as a
  // missing field of that type is.
  const char* args[] = {"program", "--shared=a,", "--unique=,2"};
  const auto result =
      rfl::cli::read<PointerConfig, Blanks>(3, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  const auto& shared = result.value().shared;
  ASSERT_EQ(shared.size(), 2u);
  ASSERT_TRUE(shared[0]);
  EXPECT_EQ(*shared[0], "a");
  EXPECT_FALSE(shared[1]);
  const auto& unique = result.value().unique;
  ASSERT_EQ(unique.size(), 2u);
  EXPECT_FALSE(unique[0]);
  ASSERT_TRUE(unique[1]);
  EXPECT_EQ(*unique[1], 2);
}

TEST(cli, test_blank_items_setting_ignored_by_json) {
  const auto config = Config{.tags = {"a", std::nullopt}};
  EXPECT_EQ(rfl::json::write<Blanks>(config), rfl::json::write(config));
  EXPECT_EQ((rfl::json::to_schema<Config, Blanks>()),
            rfl::json::to_schema<Config>());
  const auto result =
      rfl::json::read<Config, Blanks>(rfl::json::write(config));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().tags, config.tags);
}

}  // namespace test_blank_list_items
