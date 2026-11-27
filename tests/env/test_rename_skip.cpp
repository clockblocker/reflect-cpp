#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/env.hpp>
#include <string>

#include "write_and_read.hpp"

namespace test_env_rename_skip {

// Note: rfl::env automatically applies rfl::ToAllCaps to non-renamed fields, so
// `items` becomes wire name "ITEMS" and shares that wire name with
// `rfl::Rename<"ITEMS", ...>`.
struct SkipAndRename {
  rfl::Skip<std::string> items;
  rfl::Rename<"ITEMS", std::string> serialized_items;
};

struct SkipDiffTypeAndRename {
  rfl::Skip<int> items;
  rfl::Rename<"ITEMS", std::string> serialized_items;
};

struct SkipAndRenameWithExtraFields {
  rfl::Skip<std::string> items;
  rfl::Rename<"ITEMS", std::string> serialized_items;
  rfl::ExtraFields<rfl::Generic> extra_fields;
};

struct SplitSkipSerFirst {
  rfl::SkipSerialization<std::string> items;
  rfl::Rename<"ITEMS", rfl::SkipDeserialization<std::string>> serialized_items;
};

struct SplitSkipDeserFirst {
  rfl::SkipDeserialization<std::string> items;
  rfl::Rename<"ITEMS", rfl::SkipSerialization<std::string>> deserialized_items;
};

TEST(env, test_rename_skip_no_extra_fields) {
  const auto scoped_environment = test_env::ScopedEnvironment();

  const auto orig =
      SkipAndRename{.items = "internal", .serialized_items = "public"};
  rfl::env::write(orig);

  const auto parsed = rfl::env::read<SkipAndRename, rfl::NoExtraFields>();
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_TRUE(parsed->items.value().empty());
  EXPECT_EQ(parsed->serialized_items.value(), "public");

  const auto parsed_default =
      rfl::env::read<SkipAndRename, rfl::DefaultIfMissing,
                     rfl::NoExtraFields>();
  ASSERT_TRUE(parsed_default) << parsed_default.error().what();
  EXPECT_TRUE(parsed_default->items.value().empty());
  EXPECT_EQ(parsed_default->serialized_items.value(), "public");
}

TEST(env, test_rename_skip_diff_type_and_extra_fields) {
  const auto scoped_environment = test_env::ScopedEnvironment();

  const auto orig =
      SkipDiffTypeAndRename{.items = 99, .serialized_items = "hello"};
  rfl::env::write(orig);

  const auto parsed =
      rfl::env::read<SkipDiffTypeAndRename, rfl::NoExtraFields>();
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed->items.value(), 0);
  EXPECT_EQ(parsed->serialized_items.value(), "hello");

  // With ExtraFields, the shared wire name must not be double-read into
  // extra_fields.
  const auto parsed_extra = rfl::env::read<SkipAndRenameWithExtraFields>();
  ASSERT_TRUE(parsed_extra) << parsed_extra.error().what();
  EXPECT_TRUE(parsed_extra->items.value().empty());
  EXPECT_EQ(parsed_extra->serialized_items.value(), "hello");
  EXPECT_EQ(parsed_extra->extra_fields.size(), 0u);
}

TEST(env, test_rename_skip_split_serialization_deserialization) {
  {
    const auto scoped_environment = test_env::ScopedEnvironment();

    const auto orig = SplitSkipSerFirst{.items = "ignored_on_write",
                                        .serialized_items = "from_writer"};
    rfl::env::write(orig);

    const auto parsed =
        rfl::env::read<SplitSkipSerFirst, rfl::NoExtraFields>();
    ASSERT_TRUE(parsed) << parsed.error().what();
    EXPECT_EQ(parsed->items.value(), "from_writer");
    EXPECT_TRUE(parsed->serialized_items.value().value().empty());
  }

  {
    const auto scoped_environment = test_env::ScopedEnvironment();

    const auto orig = SplitSkipDeserFirst{
        .items = "from_writer_2", .deserialized_items = "ignored_on_write"};
    rfl::env::write(orig);

    const auto parsed =
        rfl::env::read<SplitSkipDeserFirst, rfl::NoExtraFields>();
    ASSERT_TRUE(parsed) << parsed.error().what();
    EXPECT_TRUE(parsed->items.value().empty());
    EXPECT_EQ(parsed->deserialized_items.value().value(), "from_writer_2");
  }
}

}  // namespace test_env_rename_skip
