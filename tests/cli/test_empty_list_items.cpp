#include <gtest/gtest.h>
#include <array>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <set>
#include <string>
#include <vector>

namespace test_empty_list_items {

using KeepEmpty = rfl::cli::KeepEmptyItems;
using Path = std::vector<rfl::ErrorDetail::PathSegment>;

struct Config {
  std::vector<std::optional<std::string>> tags;
};

TEST(cli, test_empty_items_skipped_by_default) {
  const char* args[] = {"program", "--tags=,a,,b,"};
  const auto result = rfl::cli::read<Config>(2, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().tags,
            (std::vector<std::optional<std::string>>{"a", "b"}));
}

TEST(cli, test_empty_items_optional) {
  const char* args[] = {"program", "--tags=a,,b"};
  const auto result =
      rfl::cli::read<Config, KeepEmpty>(2, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().tags,
            (std::vector<std::optional<std::string>>{"a", std::nullopt, "b"}));
}

TEST(cli, test_empty_items_leading_and_trailing) {
  const char* args[] = {"program", "--tags=,a,"};
  const auto result =
      rfl::cli::read<Config, KeepEmpty>(2, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().tags,
            (std::vector<std::optional<std::string>>{std::nullopt, "a",
                                                     std::nullopt}));
}

TEST(cli, test_empty_items_empty_value) {
  const char* args[] = {"program", "--tags="};
  const auto result =
      rfl::cli::read<Config, KeepEmpty>(2, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().tags.empty());
}

struct DefaultValConfig {
  std::vector<rfl::DefaultVal<std::string>> names;
  std::vector<rfl::DefaultVal<int>> counts;
};

TEST(cli, test_empty_items_default_val) {
  const char* args[] = {"program", "--names=a,,b", "--counts=,2"};
  const auto result =
      rfl::cli::read<DefaultValConfig, KeepEmpty>(3, const_cast<char**>(args));
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

TEST(cli, test_empty_items_required_error) {
  const char* args[] = {"program", "--tags=a,,b"};
  const auto result =
      rfl::cli::read<RequiredConfig, KeepEmpty>(2, const_cast<char**>(args));
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().what(),
            "Failed to parse field 'tags': Empty item at index 1 of '--tags'.");
  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 1u);
  EXPECT_EQ(details[0].path, (Path{"tags", size_t{1}}));
  EXPECT_EQ(details[0].message, "Empty item at index 1 of '--tags'.");
}

struct BoolConfig {
  std::vector<bool> flags;
};

TEST(cli, test_empty_items_required_bool_error) {
  // An empty bool item is not read as `true`, unlike `--flag` or `--flag=`.
  const char* args[] = {"program", "--flags=true,"};
  const auto result =
      rfl::cli::read<BoolConfig, KeepEmpty>(2, const_cast<char**>(args));
  ASSERT_FALSE(result);
  EXPECT_EQ(
      result.error().what(),
      "Failed to parse field 'flags': Empty item at index 1 of '--flags'.");
}

struct Inner {
  std::vector<std::optional<std::string>> tags;
  std::vector<std::string> names;
};

struct Outer {
  Inner a;
};

TEST(cli, test_empty_items_nested) {
  const char* args[] = {"program", "--a.tags=x,,y", "--a.names=n"};
  const auto result =
      rfl::cli::read<Outer, KeepEmpty>(3, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().a.tags,
            (std::vector<std::optional<std::string>>{"x", std::nullopt, "y"}));
  EXPECT_EQ(result.value().a.names, (std::vector<std::string>{"n"}));
}

TEST(cli, test_empty_items_nested_error) {
  const char* args[] = {"program", "--a.tags=x", "--a.names=,n"};
  const auto result =
      rfl::cli::read<Outer, KeepEmpty>(3, const_cast<char**>(args));
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().what(),
            "Failed to parse field 'a': Failed to parse field 'names': Empty "
            "item at index 0 of '--a.names'.");
  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 1u);
  EXPECT_EQ(details[0].path, (Path{"a", "names", size_t{0}}));
}

struct Containers {
  std::array<std::optional<int>, 3> numbers;
  std::set<std::string> names;
  std::vector<std::shared_ptr<int>> pointers;
};

TEST(cli, test_empty_items_other_containers) {
  const char* ok_args[] = {"program", "--numbers=1,,3", "--names=b,a",
                           "--pointers=,2"};
  const auto ok =
      rfl::cli::read<Containers, KeepEmpty>(4, const_cast<char**>(ok_args));
  ASSERT_TRUE(ok) << ok.error().what();
  EXPECT_EQ(ok.value().numbers,
            (std::array<std::optional<int>, 3>{1, std::nullopt, 3}));
  EXPECT_EQ(ok.value().names, (std::set<std::string>{"a", "b"}));
  // A smart pointer is never required, so an empty one is null.
  ASSERT_EQ(ok.value().pointers.size(), 2u);
  EXPECT_FALSE(ok.value().pointers[0]);

  const char* err_args[] = {"program", "--numbers=1,2,3", "--names=b,,a",
                            "--pointers=1"};
  const auto err =
      rfl::cli::read<Containers, KeepEmpty>(4, const_cast<char**>(err_args));
  ASSERT_FALSE(err);
  EXPECT_EQ(
      err.error().what(),
      "Failed to parse field 'names': Empty item at index 1 of '--names'.");
}

struct WideConfig {
  std::vector<std::wstring> names;
  std::vector<std::optional<std::wstring>> tags;
};

TEST(cli, test_empty_items_wstring) {
  // An empty std::wstring item is an error, as an empty std::string item is,
  // not "".
  const char* ok_args[] = {"program", "--names=a,b", "--tags=x,,y"};
  const auto ok =
      rfl::cli::read<WideConfig, KeepEmpty>(3, const_cast<char**>(ok_args));
  ASSERT_TRUE(ok) << ok.error().what();
  EXPECT_EQ(ok.value().names, (std::vector<std::wstring>{L"a", L"b"}));
  EXPECT_EQ(ok.value().tags, (std::vector<std::optional<std::wstring>>{
                                 L"x", std::nullopt, L"y"}));

  const char* err_args[] = {"program", "--names=a,,b", "--tags=x"};
  const auto err =
      rfl::cli::read<WideConfig, KeepEmpty>(3, const_cast<char**>(err_args));
  ASSERT_FALSE(err);
  EXPECT_EQ(
      err.error().what(),
      "Failed to parse field 'names': Empty item at index 1 of '--names'.");
  const auto details = err.error().details();
  ASSERT_EQ(details.size(), 1u);
  EXPECT_EQ(details[0].path, (Path{"names", size_t{1}}));
}

// --- With repeated options, ListDelimiter<""> and list positionals ---

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "program");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

using OptStrings = std::vector<std::optional<std::string>>;
using NoSplit = rfl::cli::ListDelimiter<"">;

struct RepeatedConfig {
  rfl::Short<"t", OptStrings> tags;
};

TEST(cli, test_empty_items_repeated_option) {
  // The empty items of every occurrence are kept, at their index in the
  // combined list.
  const auto result =
      read<RepeatedConfig, KeepEmpty>({"--tags", "a,", "-t", ",b", "--tags=c"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().tags(),
            (OptStrings{"a", std::nullopt, std::nullopt, "b", "c"}));
}

TEST(cli, test_empty_items_no_delimiter) {
  // With ListDelimiter<"">, lists are not split, so the setting changes
  // nothing: an empty occurrence is the item "".
  const auto result =
      read<RepeatedConfig, NoSplit, KeepEmpty>({"--tags", "a,,b", "-t", ""});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().tags(), (OptStrings{"a,,b", ""}));

  const auto required = read<RequiredConfig, NoSplit, KeepEmpty>({"--tags="});
  ASSERT_TRUE(required) << required.error().what();
  EXPECT_EQ(required.value().tags, (std::vector<std::string>{""}));
}

struct RequiredPositionalConfig {
  rfl::Positional<std::string> command;
  rfl::Positional<std::vector<std::string>> files;
};

TEST(cli, test_empty_items_list_positional_error) {
  const auto result =
      read<RequiredPositionalConfig, KeepEmpty>({"run", "a", "b,,c"});
  ASSERT_FALSE(result);
  EXPECT_EQ(
      result.error().what(),
      "Failed to parse field 'files': Empty item at index 2 of '--files'.");
  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 1u);
  EXPECT_EQ(details[0].path, (Path{"files", size_t{2}}));
}

}  // namespace test_empty_list_items
