#include <gtest/gtest.h>

#include <cstdlib>
#include <rfl.hpp>
#include <rfl/env.hpp>
#include <string>

namespace test_env_alias {

static void set_env_var(const char* _k, const char* _v) {
#ifdef _WIN32
  _putenv_s(_k, _v);
#else
  setenv(_k, _v, 1);
#endif
}

static void unset_env_var(const char* _k) {
#ifdef _WIN32
  _putenv_s(_k, "");
#else
  unsetenv(_k);
#endif
}

struct EnvGuard {
  const char* key;
  ~EnvGuard() { unset_env_var(key); }
};

struct Config {
  rfl::Alias<"OLD_HOST", rfl::Alias<"LEGACY_HOST", std::string>> host;
  rfl::Alias<"OLD_PORT", rfl::Alias<"OLD_PORT", int>> port;
};

TEST(env, test_alias_read_and_conflicts) {
  EnvGuard g1{"HOST"}, g2{"OLD_HOST"}, g3{"LEGACY_HOST"};
  EnvGuard g4{"PORT"}, g5{"OLD_PORT"};
  unset_env_var("HOST");
  unset_env_var("OLD_HOST");
  unset_env_var("LEGACY_HOST");
  unset_env_var("PORT");
  unset_env_var("OLD_PORT");

  // 1. Read via aliases (including duplicate alias declaration on `port`).
  set_env_var("OLD_HOST", "alias-host");
  set_env_var("OLD_PORT", "9090");
  const auto res1 = rfl::env::read<Config>();
  ASSERT_TRUE(res1) << res1.error().what();
  EXPECT_EQ(res1->host, "alias-host");
  EXPECT_EQ(res1->port, 9090);

  // 2. Second chained alias works.
  unset_env_var("OLD_HOST");
  set_env_var("LEGACY_HOST", "legacy-host");
  const auto res2 = rfl::env::read<Config>();
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_EQ(res2->host, "legacy-host");

  // 3. Conflicting primary + alias is rejected.
  set_env_var("HOST", "primary-host");
  const auto conflict = rfl::env::read<Config>();
  EXPECT_FALSE(conflict.has_value());
}

struct NestedConfig {
  Config myapp;
};

TEST(env, test_alias_with_nested_prefix) {
  EnvGuard g1{"MYAPP_HOST"}, g2{"MYAPP_OLD_HOST"}, g3{"MYAPP_OLD_PORT"};
  unset_env_var("MYAPP_HOST");
  unset_env_var("MYAPP_OLD_HOST");
  unset_env_var("MYAPP_OLD_PORT");

  set_env_var("MYAPP_OLD_HOST", "prefixed-host");
  set_env_var("MYAPP_OLD_PORT", "7000");

  const auto res = rfl::env::read<NestedConfig>();
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->myapp.host, "prefixed-host");
  EXPECT_EQ(res->myapp.port, 7000);
}

}  // namespace test_env_alias
