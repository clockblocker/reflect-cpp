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
  // The exact string proposed in #692.
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
  // The parser only treats a field as positional if rfl::Positional is the
  // outermost wrapper, so the help shows a named argument.
  EXPECT_EQ(rfl::cli::usage<DescriptionOutside>("app", true),
            "Usage: app --input-file=<value>\n"
            "\n"
            "Arguments:\n"
            "  --input-file=<value>  Path to the input file. Required.\n");

  const char* positional[] = {"app", "in.txt"};
  EXPECT_FALSE(
      rfl::cli::read<DescriptionOutside>(2, const_cast<char**>(positional)));
  const char* named[] = {"app", "--input-file=in.txt"};
  EXPECT_TRUE(rfl::cli::read<DescriptionOutside>(2, const_cast<char**>(named)));
}

struct Validated {
  rfl::Description<"Port to listen on",
                   rfl::Validator<int, rfl::Minimum<1>, rfl::Maximum<65535>>>
      port;
  rfl::Validator<double, rfl::ExclusiveMinimum<0>, rfl::Maximum<1>> ratio;
  rfl::Validator<int, rfl::ExclusiveMaximum<100>> percent;
  rfl::Validator<int, rfl::AnyOf<rfl::Maximum<0>, rfl::Minimum<10>>> outside;
  rfl::Validator<int, rfl::OneOf<rfl::EqualTo<1>, rfl::EqualTo<2>>> one_or_two;
  rfl::Validator<int, rfl::NotEqualTo<0>> nonzero;
  rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>> tags;
  rfl::Validator<std::vector<int>,
                 rfl::Size<rfl::AllOf<rfl::Minimum<1>, rfl::Maximum<3>>>>
      ids;
  rfl::Validator<std::string, rfl::Size<rfl::Minimum<1>>,
                 rfl::PatternValidator<"^[a-z]+$", "Lowercase">>
      user;
  rfl::Literal<"fast", "slow"> mode;
};

TEST(cli, test_usage_validators) {
  EXPECT_EQ(rfl::cli::usage<Validated>("app", true),
            "Usage: app --port=<value> --ratio=<value> --percent=<value> "
            "--outside=<value> --one-or-two=<value> --nonzero=<value> "
            "--tags=<v1,v2,...> --ids=<v1,v2,...> --user=<value> "
            "--mode=<fast|slow>\n"
            "\n"
            "Arguments:\n"
            "  --port=<value>        Port to listen on. Range: 1..65535. "
            "Required.\n"
            "  --ratio=<value>       Range: > 0 and <= 1. Required.\n"
            "  --percent=<value>     Range: < 100. Required.\n"
            "  --outside=<value>     Range: <= 0 or >= 10. Required.\n"
            "  --one-or-two=<value>  Range: 1 or 2. Required.\n"
            "  --nonzero=<value>     Range: != 0. Required.\n"
            "  --tags=<v1,v2,...>    Size: >= 1. Required.\n"
            "  --ids=<v1,v2,...>     Size: 1..3. Required.\n"
            "  --user=<value>        Size: >= 1. Pattern: ^[a-z]+$. "
            "Required.\n"
            "  --mode=<fast|slow>    Required.\n");
}

struct WithDefaultVal {
  rfl::DefaultVal<int> port = 8080;
  rfl::DefaultVal<std::string> host = "localhost";
  rfl::DefaultVal<std::vector<std::string>> tags =
      std::vector<std::string>{"a", "b"};
  rfl::DefaultVal<LogLevel> level = LogLevel::info;
  rfl::DefaultVal<double> rate = 0.25;
  rfl::DefaultVal<bool> dry_run = false;
  int count = 3;
  rfl::Short<"r", rfl::DefaultVal<int>> retries = 5;
};

TEST(cli, test_usage_default_val) {
  // Without rfl::DefaultIfMissing, `count` is required, so its member
  // initializer is never used and not shown. A DefaultVal inside rfl::Short
  // makes `retries` optional, as for the parser.
  EXPECT_EQ(rfl::cli::usage<WithDefaultVal>("app", true),
            "Usage: app [--port=<value>] [--host=<value>] "
            "[--tags=<v1,v2,...>] [--level=<debug|info|warn>] "
            "[--rate=<value>] [--dry-run[=<bool>]] --count=<value> "
            "[-r|--retries=<value>]\n"
            "\n"
            "Arguments:\n"
            "  --port=<value>             Default: 8080.\n"
            "  --host=<value>             Default: localhost.\n"
            "  --tags=<v1,v2,...>         Default: a,b.\n"
            "  --level=<debug|info|warn>  Default: info.\n"
            "  --rate=<value>             Default: 0.25.\n"
            "  --dry-run[=<bool>]         Default: false.\n"
            "  --count=<value>            Required.\n"
            "  -r, --retries=<value>      Default: 5.\n");

  const char* args[] = {"app", "--count=1", "-r", "2"};
  const auto result =
      rfl::cli::read<WithDefaultVal>(4, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().port.get(), 8080);
  EXPECT_EQ(result.value().host.get(), "localhost");
  EXPECT_EQ(result.value().retries.value().get(), 2);

  const char* no_retries[] = {"app", "--count=1"};
  const auto defaulted =
      rfl::cli::read<WithDefaultVal>(2, const_cast<char**>(no_retries));
  ASSERT_TRUE(defaulted) << defaulted.error().what();
  EXPECT_EQ(defaulted.value().retries.value().get(), 5);
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
            "  -l, --log-level=<debug|info|warn>  Minimum level to log. "
            "Default: info.\n"
            "  --timeout=<value>                  Seconds to wait. "
            "Default: 30.\n"
            "  --wait=<value>                     Old timeout. Deprecated: "
            "Use --timeout. Default: 10.\n"
            "  <output>                           Default: out.txt.\n"
            "  -n, --runs=<value>                 Number of runs. Required.\n"
            "  --limits.per-host=<value>          Connections per host. "
            "Default: 4.\n"
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

TEST(cli, test_usage_default_val_struct_not_default_constructible) {
  // The defaults are read from a default-constructed struct, which throws
  // here. The help leaves them out instead of aborting. The parser fills in
  // a missing DefaultVal field only after it default-constructed the
  // struct, so `threads` is required.
  EXPECT_EQ(rfl::cli::usage<WithInvalidDefault>("app", true),
            "Usage: app --port=<value> --threads=<value>\n"
            "\n"
            "Arguments:\n"
            "  --port=<value>     Range: >= 1. Required.\n"
            "  --threads=<value>  Required.\n");

  // A bare DefaultVal makes every read fail, since the struct must be
  // default-constructed for it.
  const char* both[] = {"app", "--port=1", "--threads=2"};
  EXPECT_FALSE(rfl::cli::read<WithInvalidDefault>(3, const_cast<char**>(both)));

  // One that is wrapped in rfl::Short is read without the defaults, and is
  // required.
  EXPECT_EQ(rfl::cli::usage<WithInvalidWrappedDefault>("app"),
            "Usage: app --port=<value> -t|--threads=<value>\n");
  const auto wrapped = rfl::cli::read<WithInvalidWrappedDefault>(
      3, const_cast<char**>(both));
  ASSERT_TRUE(wrapped) << wrapped.error().what();
  EXPECT_EQ(wrapped.value().threads.value().get(), 2);
  const char* no_threads[] = {"app", "--port=1"};
  EXPECT_FALSE(rfl::cli::read<WithInvalidWrappedDefault>(
      2, const_cast<char**>(no_threads)));

  // The same in a nested struct.
  EXPECT_EQ(rfl::cli::usage<WithInvalidWrappedDefaultInside>("app"),
            "Usage: app --server.port=<value> --server.threads=<value>\n");
  const char* nested_no_threads[] = {"app", "--server.port=1"};
  EXPECT_FALSE(rfl::cli::read<WithInvalidWrappedDefaultInside>(
      2, const_cast<char**>(nested_no_threads)));
  const char* nested_both[] = {"app", "--server.port=1", "--server.threads=2"};
  EXPECT_TRUE(rfl::cli::read<WithInvalidWrappedDefaultInside>(
      3, const_cast<char**>(nested_both)));
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

TEST(cli, test_usage_default_if_missing) {
  EXPECT_EQ(
      (rfl::cli::usage<WithInitializers, rfl::DefaultIfMissing>("app", true)),
      "Usage: app [--count=<value>] [--name=<value>] [--verbose[=<bool>]] "
      "[--limit=<value>] [--database.host=<value>] [--database.port=<value>]\n"
      "\n"
      "Arguments:\n"
      "  --count=<value>          Default: 3.\n"
      "  --name=<value>           Default: \"\".\n"
      "  --verbose[=<bool>]       Default: false.\n"
      "  --limit=<value>\n"
      "  --database.host=<value>  Default: db.\n"
      "  --database.port=<value>  Default: 5432.\n");

  const char* args[] = {"app"};
  const auto result = rfl::cli::read<WithInitializers, rfl::DefaultIfMissing>(
      1, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().count, 3);
  EXPECT_EQ(result.value().database.port, 5432);
}

TEST(cli, test_usage_without_default_if_missing) {
  EXPECT_EQ(rfl::cli::usage<WithInitializers>("app"),
            "Usage: app --count=<value> --name=<value> --verbose[=<bool>] "
            "[--limit=<value>] --database.host=<value> "
            "--database.port=<value>\n");

  const char* args[] = {"app"};
  EXPECT_FALSE(rfl::cli::read<WithInitializers>(1, const_cast<char**>(args)));
}

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

TEST(cli, test_usage_default_val_without_default_constructor) {
  static_assert(!std::is_default_constructible_v<WithPortAndDefaultVal>);
  // The struct cannot be default-constructed, so the parser cannot fill in
  // `retries`, and it is required.
  EXPECT_EQ(rfl::cli::usage<WithPortAndDefaultVal>("app"),
            "Usage: app --port=<value> --retries=<value>\n");
  const char* no_retries[] = {"app", "--port=1"};
  EXPECT_FALSE(rfl::cli::read<WithPortAndDefaultVal>(
      2, const_cast<char**>(no_retries)));
  const char* both[] = {"app", "--port=1", "--retries=2"};
  const auto result =
      rfl::cli::read<WithPortAndDefaultVal>(3, const_cast<char**>(both));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().retries.get(), 2);
}

TEST(cli, test_usage_default_if_missing_not_default_constructible) {
  static_assert(!std::is_default_constructible_v<WithPort>);
  // Nothing is required, but the defaults cannot be known without T{}, so
  // usage() still compiles and leaves them out.
  EXPECT_EQ((rfl::cli::usage<WithPort, rfl::DefaultIfMissing>("app", true)),
            "Usage: app [--port=<value>] [--retries=<value>]\n"
            "\n"
            "Arguments:\n"
            "  --port=<value>\n"
            "  --retries=<value>\n");
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
            "  --old-port=<value>      Old port. Deprecated: Use --port "
            "instead.\n"
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
