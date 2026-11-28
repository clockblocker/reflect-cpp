#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/yaml.hpp>
#include <string>

namespace test_yaml_alias {

struct Config {
  rfl::Alias<"download-dir", rfl::Alias<"downloadDir", std::string>>
      download_dir;
  rfl::Alias<"max-peers", rfl::DefaultVal<int>> max_peers = 50;
};

TEST(yaml, test_alias_read_write_and_conflicts) {
  const std::string yaml_alias =
      "download-dir: /data/downloads\nmax-peers: 120\n";
  const auto res = rfl::yaml::read<Config, rfl::NoExtraFields>(yaml_alias);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->download_dir, "/data/downloads");
  EXPECT_EQ(res->max_peers.value().value(), 120);

  // Writes canonical field names.
  const auto written = rfl::yaml::write(*res);
  EXPECT_NE(written.find("download_dir:"), std::string::npos) << written;
  EXPECT_NE(written.find("max_peers:"), std::string::npos) << written;
  EXPECT_EQ(written.find("download-dir:"), std::string::npos) << written;
  EXPECT_EQ(written.find("max-peers:"), std::string::npos) << written;

  // Chained alias with default preserved.
  const auto res2 =
      rfl::yaml::read<Config>(std::string("downloadDir: /tmp\n"));
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_EQ(res2->download_dir, "/tmp");
  EXPECT_EQ(res2->max_peers.value().value(), 50);

  // Conflicting primary + alias rejected.
  const auto conflict = rfl::yaml::read<Config>(
      std::string("download_dir: /a\ndownload-dir: /b\n"));
  EXPECT_FALSE(conflict);
}

}  // namespace test_yaml_alias
