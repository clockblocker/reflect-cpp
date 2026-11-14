#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_usage {

enum class LogLevel { debug, info, warn };

struct Database {
  std::string host;
  int port;
};

struct Config {
  rfl::Positional<std::string> input_file;
  std::string host_name;
  bool verbose;
  std::vector<std::string> tags;
  rfl::Short<"p", int> port;
  Database database;
  std::optional<LogLevel> log_level;
};

TEST(cli, test_usage_synopsis) {
  // The exact string proposed in
  // https://github.com/munich-data-engineering/reflect-cpp/issues/692.
  EXPECT_EQ(rfl::cli::usage<Config>("my_binary"),
            "Usage: my_binary <input-file> --host-name=<value> "
            "--verbose[=<bool>] --tags=<v1,v2,...> -p|--port=<value> "
            "--database.host=<value> --database.port=<value> "
            "[--log-level=<debug|info|warn>]\n");
}

struct Described {
  rfl::Positional<rfl::Description<"Path to the input file", std::string>>
      input_file;
  rfl::Description<"Host to connect to", std::string> host_name;
  bool verbose;
  rfl::Short<"p", rfl::Description<"Port to connect on", int>> port;
  rfl::Description<"Minimum level to log", std::optional<LogLevel>> log_level;
};

TEST(cli, test_usage_detailed) {
  EXPECT_EQ(
      rfl::cli::usage<Described>("my_binary", true),
      "Usage: my_binary <input-file> --host-name=<value> --verbose[=<bool>] "
      "-p|--port=<value> [--log-level=<debug|info|warn>]\n"
      "\n"
      "Arguments:\n"
      "  <input-file>                   Path to the input file. Required.\n"
      "  --host-name=<value>            Host to connect to. Required.\n"
      "  --verbose[=<bool>]             Required.\n"
      "  -p, --port=<value>             Port to connect on. Required.\n"
      "  --log-level=<debug|info|warn>  Minimum level to log\n");

  // What the help advertises is what the parser accepts.
  const char* args[] = {"my_binary",   "in.txt", "--host-name=localhost",
                        "--verbose",   "-p",     "8080",
                        "--log-level=warn"};
  const auto result =
      rfl::cli::read<Described>(7, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input_file().get(), "in.txt");
  EXPECT_EQ(result.value().port().get(), 8080);
  EXPECT_EQ(result.value().log_level.get(), LogLevel::warn);

  // A plain bool is required, as the help says.
  const char* no_verbose[] = {"my_binary", "in.txt", "--host-name=localhost",
                              "-p", "8080"};
  EXPECT_FALSE(rfl::cli::read<Described>(5, const_cast<char**>(no_verbose)));
}

struct DescriptionOutside {
  rfl::Description<"Path to the input file", rfl::Positional<std::string>>
      input_file;
};

TEST(cli, test_usage_description_outside_positional) {
  // The parser finds rfl::Positional inside rfl::Description, and so does
  // the help.
  EXPECT_EQ(rfl::cli::usage<DescriptionOutside>("app", true),
            "Usage: app <input-file>\n"
            "\n"
            "Arguments:\n"
            "  <input-file>  Path to the input file. Required.\n");

  const char* positional[] = {"app", "in.txt"};
  const auto result =
      rfl::cli::read<DescriptionOutside>(2, const_cast<char**>(positional));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input_file.value().value(), "in.txt");
}

struct Limits {
  rfl::Description<"Connections per host", rfl::DefaultVal<int>> per_host = 4;
  int total;
};

/// rfl::DefaultVal inside other wrappers, in different orders.
struct WrappedDefaultVal {
  rfl::Short<"l", rfl::Description<"Minimum level to log",
                                   rfl::DefaultVal<LogLevel>>>
      log_level = LogLevel::info;
  rfl::Description<"Seconds to wait", rfl::DefaultVal<int>> timeout = 30;
  rfl::Deprecated<"Use --timeout", "Old timeout", rfl::DefaultVal<int>> wait =
      10;
  rfl::Positional<rfl::DefaultVal<std::string>> output = "out.txt";
  rfl::Short<"n", rfl::Description<"Number of runs", int>> runs;
  Limits limits;
};

TEST(cli, test_usage_wrapped_default_val) {
  EXPECT_EQ(rfl::cli::usage<WrappedDefaultVal>("app", true),
            "Usage: app [-l|--log-level=<debug|info|warn>] "
            "[--timeout=<value>] [--wait=<value>] [<output>] "
            "-n|--runs=<value> [--limits.per-host=<value>] "
            "--limits.total=<value>\n"
            "\n"
            "Arguments:\n"
            "  -l, --log-level=<debug|info|warn>  Minimum level to log\n"
            "  --timeout=<value>                  Seconds to wait\n"
            "  --wait=<value>                     Old timeout\n"
            "  <output>\n"
            "  -n, --runs=<value>                 Number of runs. Required.\n"
            "  --limits.per-host=<value>          Connections per host\n"
            "  --limits.total=<value>             Required.\n");

  // Everything the help leaves optional can be left out.
  const char* minimal[] = {"app", "-n", "3", "--limits.total=8"};
  const auto defaulted =
      rfl::cli::read<WrappedDefaultVal>(4, const_cast<char**>(minimal));
  ASSERT_TRUE(defaulted) << defaulted.error().what();
  EXPECT_EQ(defaulted.value().log_level.value().value().get(),
            LogLevel::info);
  EXPECT_EQ(defaulted.value().timeout.value().get(), 30);
  EXPECT_EQ(defaulted.value().wait.value().get(), 10);
  EXPECT_EQ(defaulted.value().output.value().get(), "out.txt");
  EXPECT_EQ(defaulted.value().runs.value().value(), 3);
  EXPECT_EQ(defaulted.value().limits.per_host.value().get(), 4);
  EXPECT_EQ(defaulted.value().limits.total, 8);

  // The arguments are spelled as the help shows them.
  const char* all[] = {"app",          "-l",          "warn",
                       "--timeout=5",  "--wait=1",    "result.txt",
                       "--runs=2",     "--limits.per-host=2",
                       "--limits.total=4"};
  const auto given =
      rfl::cli::read<WrappedDefaultVal>(9, const_cast<char**>(all));
  ASSERT_TRUE(given) << given.error().what();
  EXPECT_EQ(given.value().log_level.value().value().get(), LogLevel::warn);
  EXPECT_EQ(given.value().timeout.value().get(), 5);
  EXPECT_EQ(given.value().wait.value().get(), 1);
  EXPECT_EQ(given.value().output.value().get(), "result.txt");
  EXPECT_EQ(given.value().limits.per_host.value().get(), 2);

  // What the help calls required is required.
  const char* no_runs[] = {"app", "--limits.total=8"};
  EXPECT_FALSE(
      rfl::cli::read<WrappedDefaultVal>(2, const_cast<char**>(no_runs)));
  const char* no_total[] = {"app", "-n", "3"};
  EXPECT_FALSE(
      rfl::cli::read<WrappedDefaultVal>(3, const_cast<char**>(no_total)));
}

/// The default-constructed `port` does not satisfy its validator.
struct WithInvalidDefault {
  rfl::Validator<int, rfl::Minimum<1>> port;
  rfl::DefaultVal<int> threads = 4;
};

/// The same with the DefaultVal inside rfl::Short.
struct WithInvalidWrappedDefault {
  rfl::Validator<int, rfl::Minimum<1>> port;
  rfl::Short<"t", rfl::DefaultVal<int>> threads = 4;
};

struct WithInvalidWrappedDefaultInside {
  WithInvalidWrappedDefault server;
};

/// Has no default constructor, so neither has a struct that contains it.
class Port {
 public:
  using ReflectionType = int;
  Port(int _value) : value_(_value) {}
  int reflection() const { return value_; }

 private:
  int value_;
};

struct WithPort {
  Port port;
  int retries = 3;
};

struct WithPortAndDefaultVal {
  Port port;
  rfl::DefaultVal<int> retries = 3;
};

TEST(cli, test_usage_defaults_without_default_construction) {
  // The parser fills in a missing DefaultVal field only after it
  // default-constructed the struct. If that throws (`port`'s zero value
  // fails its validator) or the struct has no default constructor, the
  // field is required, and the help says so.
  static_assert(!std::is_default_constructible_v<WithPortAndDefaultVal>);
  EXPECT_EQ(rfl::cli::usage<WithInvalidDefault>("app", true),
            "Usage: app --port=<value> --threads=<value>\n"
            "\n"
            "Arguments:\n"
            "  --port=<value>     Required.\n"
            "  --threads=<value>  Required.\n");
  EXPECT_EQ(rfl::cli::usage<WithInvalidWrappedDefault>("app"),
            "Usage: app --port=<value> -t|--threads=<value>\n");
  EXPECT_EQ(rfl::cli::usage<WithInvalidWrappedDefaultInside>("app"),
            "Usage: app --server.port=<value> --server.threads=<value>\n");
  EXPECT_EQ(rfl::cli::usage<WithPortAndDefaultVal>("app"),
            "Usage: app --port=<value> --retries=<value>\n");

  // A bare DefaultVal makes every read fail; a wrapped one is required.
  const char* both[] = {"app", "--port=1", "--threads=2"};
  EXPECT_FALSE(rfl::cli::read<WithInvalidDefault>(3, const_cast<char**>(both)));
  const auto wrapped = rfl::cli::read<WithInvalidWrappedDefault>(
      3, const_cast<char**>(both));
  ASSERT_TRUE(wrapped) << wrapped.error().what();
  EXPECT_EQ(wrapped.value().threads.value().get(), 2);
  const char* port_only[] = {"app", "--port=1"};
  EXPECT_FALSE(rfl::cli::read<WithInvalidWrappedDefault>(
      2, const_cast<char**>(port_only)));
  EXPECT_FALSE(rfl::cli::read<WithPortAndDefaultVal>(
      2, const_cast<char**>(port_only)));
  const char* nested[] = {"app", "--server.port=1"};
  EXPECT_FALSE(rfl::cli::read<WithInvalidWrappedDefaultInside>(
      2, const_cast<char**>(nested)));

  // With rfl::DefaultIfMissing nothing is required, even without T{}.
  static_assert(!std::is_default_constructible_v<WithPort>);
  EXPECT_EQ((rfl::cli::usage<WithPort, rfl::DefaultIfMissing>("app", true)),
            "Usage: app [--port=<value>] [--retries=<value>]\n"
            "\n"
            "Arguments:\n"
            "  --port=<value>\n"
            "  --retries=<value>\n");
}

struct Inner {
  std::string host = "db";
  int port = 5432;
};

struct WithInitializers {
  int count = 3;
  std::string name;
  bool verbose = false;
  std::optional<int> limit;
  Inner database;
};

TEST(cli, test_usage_without_default_if_missing) {
  EXPECT_EQ(rfl::cli::usage<WithInitializers>("app"),
            "Usage: app --count=<value> --name=<value> --verbose[=<bool>] "
            "[--limit=<value>] --database.host=<value> "
            "--database.port=<value>\n");

  const char* args[] = {"app"};
  EXPECT_FALSE(rfl::cli::read<WithInitializers>(1, const_cast<char**>(args)));
}

struct Address {
  std::string city;
  int zip;
};

struct Names {
  rfl::Rename<"output_dir", std::string> output_dir;
  rfl::Flatten<Address> address;
  std::optional<Database> backup;
  std::map<std::string, std::string> labels;
};

TEST(cli, test_usage_names) {
  EXPECT_EQ(rfl::cli::usage<Names>("app", true),
            "Usage: app --output_dir=<value> --city=<value> --zip=<value> "
            "[--backup.host=<value>] [--backup.port=<value>] "
            "--labels.<key>=<value>\n"
            "\n"
            "Arguments:\n"
            "  --output_dir=<value>    Required.\n"
            "  --city=<value>          Required.\n"
            "  --zip=<value>           Required.\n"
            "  --backup.host=<value>\n"
            "  --backup.port=<value>\n"
            "  --labels.<key>=<value>  Required.\n");

  const char* args[] = {"app", "--output_dir=/tmp", "--city=Springfield",
                        "--zip=12345", "--labels.env=prod"};
  const auto result = rfl::cli::read<Names>(5, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().labels.at("env"), "prod");
}

struct Optionals {
  rfl::Positional<std::optional<LogLevel>> level;
  rfl::Short<"v", std::optional<bool>> verbose;
  rfl::Deprecated<"Use --port instead", "Old port", std::optional<int>>
      old_port;
  rfl::Description<"First line\nSecond line", std::string> notes;
};

TEST(cli, test_usage_optionals) {
  EXPECT_EQ(rfl::cli::usage<Optionals>("app", true),
            "Usage: app [<level>] [-v|--verbose[=<bool>]] "
            "[--old-port=<value>] --notes=<value>\n"
            "\n"
            "Arguments:\n"
            "  <level>                 Choices: debug|info|warn.\n"
            "  -v, --verbose[=<bool>]\n"
            "  --old-port=<value>      Old port\n"
            "  --notes=<value>         First line\n"
            "                          Second line. Required.\n");
}

TEST(cli, test_usage_no_optionals) {
  // Processors are applied exactly as in read: with rfl::NoOptionals,
  // optional fields are required.
  EXPECT_EQ((rfl::cli::usage<Optionals, rfl::NoOptionals>("app")),
            "Usage: app <level> -v|--verbose[=<bool>] --old-port=<value> "
            "--notes=<value>\n");
}

struct Empty {};

TEST(cli, test_usage_empty) {
  EXPECT_EQ(rfl::cli::usage<Empty>("app"), "Usage: app\n");
  EXPECT_EQ(rfl::cli::usage<Empty>("app", true), "Usage: app\n");
}

}  // namespace test_usage
