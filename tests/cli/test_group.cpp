#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <optional>
#include <string>
#include <vector>

namespace test_group {

struct InputMixin {
  rfl::Group<"INPUT OPTIONS",
             rfl::Short<"F", rfl::Description<"Format of input file",
                                              std::optional<std::string>>>>
      input_format;
};

struct OutputMixin {
  rfl::Group<"OUTPUT OPTIONS",
             rfl::Short<"o", rfl::Description<"Output file",
                                              std::optional<std::string>>>>
      output;
  rfl::Short<"O", rfl::Group<"OUTPUT OPTIONS",
                             rfl::Description<"Allow overwrite",
                                              rfl::DefaultVal<bool>>>>
      overwrite = false;
};

struct CatConfig {
  rfl::Group<"COMMAND OPTIONS",
             rfl::Description<"Buffer data in memory", rfl::DefaultVal<bool>>>
      buffer_data = false;
  rfl::Flatten<InputMixin> input;
  rfl::Flatten<OutputMixin> output;
};

TEST(cli, test_group_sections_and_flatten) {
  const std::vector<std::string> args = {"--buffer-data", "-F", "osm", "-o",
                                         "out.pbf", "-O"};
  const auto res =
      rfl::cli::read<CatConfig, rfl::NoExtraFields,
                     rfl::cli::SeparateOptionValues>(args);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_TRUE(res->buffer_data.value().value().get());
  EXPECT_EQ(res->input().input_format.value().value().value(), "osm");
  EXPECT_EQ(res->output().output.value().value().value(), "out.pbf");
  EXPECT_TRUE(res->output().overwrite.value().value().value().get());

  const auto detailed = rfl::cli::usage<CatConfig>("osmium cat", true);
  ASSERT_TRUE(detailed) << detailed.error().what();

  const auto cmd_pos = detailed->find("\nCOMMAND OPTIONS:\n");
  const auto in_pos = detailed->find("\nINPUT OPTIONS:\n");
  const auto out_pos = detailed->find("\nOUTPUT OPTIONS:\n");
  ASSERT_NE(cmd_pos, std::string::npos) << *detailed;
  ASSERT_NE(in_pos, std::string::npos) << *detailed;
  ASSERT_NE(out_pos, std::string::npos) << *detailed;
  EXPECT_LT(cmd_pos, in_pos);
  EXPECT_LT(in_pos, out_pos);
  EXPECT_EQ(detailed->find("\nArguments:\n"), std::string::npos);
}

}  // namespace test_group
