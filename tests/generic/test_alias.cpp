#include <gtest/gtest.h>

#include <rfl.hpp>
#include <string>

namespace test_generic_alias {

struct Settings {
  rfl::Alias<"download-dir", rfl::Alias<"downloadDir", std::string>>
      download_dir;
  rfl::Alias<"speedLimitDown", int> speed_limit_down;
};

TEST(generic, test_alias_from_and_to_generic) {
  rfl::Generic::Object obj;
  obj["download-dir"] = std::string("/downloads");
  obj["speedLimitDown"] = 300;

  const auto res =
      rfl::from_generic<Settings, rfl::NoExtraFields>(rfl::Generic(obj));
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->download_dir, "/downloads");
  EXPECT_EQ(res->speed_limit_down, 300);

  // to_generic emits canonical primary names only.
  const auto gen = rfl::to_generic(*res);
  const auto out_obj = gen.to_object().value();
  EXPECT_EQ(out_obj.get("download_dir").value().to_string().value(),
            "/downloads");
  EXPECT_EQ(out_obj.get("speed_limit_down").value().to_int().value(), 300);
  EXPECT_FALSE(out_obj.get("download-dir"));
  EXPECT_FALSE(out_obj.get("speedLimitDown"));

  // Duplicate / conflicting keys in Generic::Object are rejected.
  rfl::Generic::Object dup;
  dup["download_dir"] = std::string("/a");
  dup["downloadDir"] = std::string("/b");
  dup["speed_limit_down"] = 10;
  const auto bad = rfl::from_generic<Settings>(rfl::Generic(dup));
  EXPECT_FALSE(bad);
}

}  // namespace test_generic_alias
