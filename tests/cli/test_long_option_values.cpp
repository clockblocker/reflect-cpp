#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <variant>
#include <vector>

namespace test_long_option_values {

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "program");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

enum class Color { red, green, blue };

struct Database {
  std::string host;
  int port;
  std::optional<bool> ssl;
};

struct Config {
  std::string sql;
  int offset;
  double rate;
  std::vector<std::string> tags;
  Color color;
  std::optional<std::string> label;
  Database database;
};

TEST(cli, test_long_space_basic_types) {
  const auto result = read<Config>(
      {"--sql", "SELECT 1", "--offset", "-5", "--rate", "-.5", "--tags", "a,b",
       "--color", "green", "--label", "x", "--database.host", "localhost",
       "--database.port", "5432", "--database.ssl"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& c = result.value();
  EXPECT_EQ(c.sql, "SELECT 1");
  EXPECT_EQ(c.offset, -5);
  EXPECT_DOUBLE_EQ(c.rate, -0.5);
  EXPECT_EQ(c.tags, (std::vector<std::string>{"a", "b"}));
  EXPECT_EQ(c.color, Color::green);
  EXPECT_EQ(c.label, "x");
  EXPECT_EQ(c.database.host, "localhost");
  EXPECT_EQ(c.database.port, 5432);
  EXPECT_EQ(c.database.ssl, true);
}

TEST(cli, test_long_space_mixed_with_equals) {
  const auto result = read<Config>(
      {"--sql=SELECT 1", "--offset", "3", "--rate=2.5", "--tags=x",
       "--color", "red", "--database.host=h", "--database.port", "1",
       "--database.ssl=false"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& c = result.value();
  EXPECT_EQ(c.sql, "SELECT 1");
  EXPECT_EQ(c.offset, 3);
  EXPECT_DOUBLE_EQ(c.rate, 2.5);
  EXPECT_EQ(c.tags, (std::vector<std::string>{"x"}));
  EXPECT_FALSE(c.label);
  EXPECT_EQ(c.database.ssl, false);
}

TEST(cli, test_long_space_value_may_look_like_a_key) {
  // The value is taken verbatim, even if it contains '=' or matches a
  // field name.
  const auto result = read<Config>(
      {"--sql", "a=b", "--label", "sql", "--offset", "0", "--rate", "0",
       "--tags", "", "--color", "blue", "--database.host", "h",
       "--database.port", "1"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().sql, "a=b");
  EXPECT_EQ(result.value().label, "sql");
  EXPECT_TRUE(result.value().tags.empty());
}

struct Wrapped {
  rfl::Description<"The query to run", std::string> sql;
  rfl::Validator<int, rfl::Minimum<1>> limit;
  rfl::Rename<"outFile", std::string> out_file;
  rfl::Short<"p", int> port;
  rfl::Positional<std::string> input_file;
  std::optional<rfl::Validator<int, rfl::Minimum<0>>> retries;
  std::unique_ptr<std::string> owner;
};

TEST(cli, test_long_space_wrappers) {
  const auto result = read<Wrapped>(
      {"--sql", "SELECT 1", "--limit", "10", "--outFile", "out.csv",
       "--port", "8080", "--input-file", "in.csv", "--retries", "3", "--owner", "me"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& w = result.value();
  EXPECT_EQ(w.sql(), "SELECT 1");
  EXPECT_EQ(w.limit(), 10);
  EXPECT_EQ(w.out_file(), "out.csv");
  EXPECT_EQ(w.port(), 8080);
  EXPECT_EQ(w.input_file(), "in.csv");
  ASSERT_TRUE(w.retries);
  EXPECT_EQ((*w.retries)(), 3);
  ASSERT_TRUE(w.owner);
  EXPECT_EQ(*w.owner, "me");
}

struct WithDefaults {
  rfl::DefaultVal<std::string> mode;
  rfl::DefaultVal<int> level = 3;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_long_space_default_val) {
  const auto result = read<WithDefaults>({"--mode", "fast", "in.csv"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().mode(), "fast");
  EXPECT_EQ(result.value().level(), 3);
  EXPECT_EQ(result.value().input_file(), "in.csv");
}

TEST(cli, test_long_space_validator_error_reported) {
  const auto result = read<Wrapped>(
      {"--sql", "x", "--limit", "0", "--outFile", "o", "-p", "1", "in"});
  ASSERT_FALSE(result);
  EXPECT_NE(std::string(result.error().what()).find("limit"),
            std::string::npos)
      << result.error().what();
}

struct Address {
  std::string city;
  rfl::Short<"z", int> zip;
  bool verified;
};

struct Person {
  std::string name;
  rfl::Flatten<Address> address;
  rfl::Positional<std::string> note;
};

TEST(cli, test_long_space_flatten) {
  const auto result = read<Person>({"--name", "Alice", "--city", "Springfield",
                                    "--zip", "12345", "--verified", "hello"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& p = result.value();
  EXPECT_EQ(p.name, "Alice");
  EXPECT_EQ(p.address().city, "Springfield");
  EXPECT_EQ(p.address().zip(), 12345);
  EXPECT_TRUE(p.address().verified);
  EXPECT_EQ(p.note(), "hello");
}

struct Flags {
  bool verbose;
  std::optional<bool> dry_run;
  rfl::Short<"q", std::optional<bool>> quiet;
  rfl::Description<"Colorize output", std::optional<bool>> color;
  Database database;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_long_bool_does_not_consume_positional) {
  for (const char* flag :
       {"--verbose", "--dry-run", "--quiet", "--color", "--database.ssl"}) {
    std::vector<const char*> args = {"--database.host=h", "--database.port=1",
                                     flag, "input.txt"};
    if (std::string(flag) != "--verbose") {
      args.push_back("--verbose=false");
    }
    const auto result = read<Flags>(args);
    ASSERT_TRUE(result) << flag << ": " << result.error().what();
    EXPECT_EQ(result.value().input_file(), "input.txt") << flag;
  }
}

TEST(cli, test_long_bool_values) {
  const auto result = read<Flags>(
      {"--verbose", "--dry-run", "input.txt", "--quiet", "--color",
       "--database.host", "h", "--database.port", "1", "--database.ssl"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& f = result.value();
  EXPECT_TRUE(f.verbose);
  EXPECT_EQ(f.dry_run, true);
  EXPECT_EQ(f.quiet(), true);
  EXPECT_EQ(f.color(), true);
  EXPECT_EQ(f.database.ssl, true);
  EXPECT_EQ(f.input_file(), "input.txt");
}

struct Force {
  bool force;
  rfl::Positional<int> level;
};

TEST(cli, test_long_bool_does_not_take_bool_literal) {
  // Unlike `-f 0`, a long flag never takes the next token, even if it is a
  // boolean literal.
  for (const char* literal : {"0", "1"}) {
    const auto result = read<Force>({"--force", literal});
    ASSERT_TRUE(result) << literal << ": " << result.error().what();
    EXPECT_TRUE(result.value().force) << literal;
    EXPECT_EQ(result.value().level(), std::stoi(literal));
  }

  const auto with_equals = read<Force>({"--force=false", "0"});
  ASSERT_TRUE(with_equals) << with_equals.error().what();
  EXPECT_FALSE(with_equals.value().force);
  EXPECT_EQ(with_equals.value().level(), 0);

  const auto literal = read<Flags>(
      {"--database.host=h", "--database.port=1", "--verbose", "false"});
  ASSERT_TRUE(literal) << literal.error().what();
  EXPECT_TRUE(literal.value().verbose);
  EXPECT_EQ(literal.value().input_file(), "false");
}

TEST(cli, test_long_bool_literal_positional_after_separator) {
  const auto result = read<Flags>(
      {"--database.host=h", "--database.port=1", "--verbose", "--", "false"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().verbose);
  EXPECT_EQ(result.value().input_file(), "false");
}

struct Query {
  std::string sql;
  std::optional<bool> verbose;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_long_missing_value_at_end) {
  const auto result = read<Query>({"in.txt", "--sql"});
  ASSERT_FALSE(result);
  EXPECT_EQ(std::string(result.error().what()),
            "Missing value for option '--sql'.");
}

TEST(cli, test_long_missing_value_before_option) {
  const auto result = read<Query>({"--sql", "--verbose", "in.txt"});
  ASSERT_FALSE(result);
  EXPECT_EQ(std::string(result.error().what()),
            "Missing value for option '--sql'.");
}

TEST(cli, test_long_missing_value_before_separator) {
  const auto result = read<Query>({"--sql", "--", "in.txt"});
  ASSERT_FALSE(result);
  EXPECT_EQ(std::string(result.error().what()),
            "Missing value for option '--sql'.");
}

TEST(cli, test_long_empty_value_with_equals) {
  const auto result = read<Query>({"--sql=", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().sql, "");
  EXPECT_EQ(result.value().input_file(), "in.txt");
}

TEST(cli, test_long_space_separator) {
  const auto result =
      read<Query>({"--sql", "SELECT 1", "--", "--not-a-flag.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().sql, "SELECT 1");
  EXPECT_FALSE(result.value().verbose);
  EXPECT_EQ(result.value().input_file(), "--not-a-flag.txt");
}

TEST(cli, test_long_space_duplicate) {
  const auto result = read<Query>({"--sql", "a", "--sql", "b", "in.txt"});
  ASSERT_FALSE(result);
  EXPECT_EQ(std::string(result.error().what()), "Duplicate argument: --sql");
}

TEST(cli, test_long_space_conflicts_with_positional) {
  const auto result = read<Query>({"--sql=x", "--input-file", "a", "b"});
  ASSERT_FALSE(result);
  EXPECT_EQ(std::string(result.error().what()),
            "Conflict: positional argument and '--input-file' both provided.");
}

TEST(cli, test_long_unknown_option_lenient) {
  // Without rfl::NoExtraFields, unknown options are ignored and do not
  // consume the next token.
  const auto result =
      read<Query>({"--nope", "in.txt", "--sql", "x", "--other=1"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().sql, "x");
  EXPECT_EQ(result.value().input_file(), "in.txt");
}

TEST(cli, test_long_unknown_option_lenient_too_many_positional) {
  const auto result = read<Config>({"--nope", "1"});
  ASSERT_FALSE(result);
  EXPECT_EQ(std::string(result.error().what()),
            "Too many positional arguments: expected at most 0, got 1.");
}

TEST(cli, test_long_unknown_option_no_extra_fields) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"--sql=x", "--nope", "1", "in.txt"},
           {"--sql=x", "--nope=1", "in.txt"},
           {"--sql=x", "in.txt", "--nope"},
           {"--nope", "--sql=x", "in.txt"}}) {
    const auto result = read<Query, rfl::NoExtraFields>(args);
    ASSERT_FALSE(result);
    EXPECT_EQ(std::string(result.error().what()), "Unknown option '--nope'.");
  }
}

TEST(cli, test_long_unknown_nested_option_no_extra_fields) {
  const auto result = read<Config, rfl::NoExtraFields>(
      {"--sql=x", "--offset=1", "--rate=1", "--tags=a", "--color=red",
       "--database.host=h", "--database.port=1", "--database.nope", "1"});
  ASSERT_FALSE(result);
  EXPECT_EQ(std::string(result.error().what()),
            "Unknown option '--database.nope'.");
}

TEST(cli, test_long_known_options_no_extra_fields) {
  const auto result = read<Flags, rfl::NoExtraFields>(
      {"--verbose", "in.txt", "--database.host", "h", "--database.port", "1",
       "-q"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().verbose);
  EXPECT_EQ(result.value().quiet(), true);
  EXPECT_EQ(result.value().database.host, "h");
  EXPECT_EQ(result.value().input_file(), "in.txt");
}

struct WithMap {
  std::map<std::string, std::string> labels;
  std::map<std::string, bool> features;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_long_space_map) {
  const auto result = read<WithMap, rfl::NoExtraFields>(
      {"--labels.env", "prod", "--labels.team=data", "--features.fast",
       "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& m = result.value();
  EXPECT_EQ(m.labels, (std::map<std::string, std::string>{
                          {"env", "prod"}, {"team", "data"}}));
  EXPECT_EQ(m.features, (std::map<std::string, bool>{{"fast", true}}));
  EXPECT_EQ(m.input_file(), "in.txt");
}

struct Server {
  std::string host;
  int port;
};

struct WithMapOfStructs {
  std::map<std::string, Server> servers;
  std::map<std::string, std::map<std::string, int>> limits;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_long_space_map_of_structs) {
  const auto result = read<WithMapOfStructs, rfl::NoExtraFields>(
      {"--servers.eu.host", "h", "--servers.eu.port", "1",
       "--servers.us.host=u", "--servers.us.port", "2", "--limits.a.b", "3",
       "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& m = result.value();
  EXPECT_EQ(m.servers.at("eu").host, "h");
  EXPECT_EQ(m.servers.at("eu").port, 1);
  EXPECT_EQ(m.servers.at("us").host, "u");
  EXPECT_EQ(m.servers.at("us").port, 2);
  EXPECT_EQ(m.limits.at("a").at("b"), 3);
  EXPECT_EQ(m.input_file(), "in.txt");

  const auto unknown = read<WithMapOfStructs, rfl::NoExtraFields>(
      {"--servers.eu.nope", "1", "in.txt"});
  ASSERT_FALSE(unknown);
  EXPECT_EQ(std::string(unknown.error().what()),
            "Unknown option '--servers.eu.nope'.");
}

using DatabaseTuple = rfl::NamedTuple<rfl::Field<"host", std::string>,
                                      rfl::Field<"port", int>,
                                      rfl::Field<"ssl", std::optional<bool>>>;

struct WithNamedTuple {
  DatabaseTuple database;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_long_space_named_tuple) {
  const auto result = read<WithNamedTuple, rfl::NoExtraFields>(
      {"--database.host", "h", "--database.port", "1", "--database.ssl",
       "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& db = result.value().database;
  EXPECT_EQ(db.get<"host">(), "h");
  EXPECT_EQ(db.get<"port">(), 1);
  EXPECT_EQ(db.get<"ssl">(), true);
  EXPECT_EQ(result.value().input_file(), "in.txt");

  const auto root = read<DatabaseTuple>({"--host", "h", "--port", "1"});
  ASSERT_TRUE(root) << root.error().what();
  EXPECT_EQ(root.value().get<"host">(), "h");
  EXPECT_EQ(root.value().get<"port">(), 1);
}

struct WithVariants {
  std::optional<std::variant<bool, std::string>> mode;
  rfl::Generic extra;
  rfl::Positional<std::optional<std::string>> input_file;
};

TEST(cli, test_long_variant_and_generic_without_value) {
  // A variant or rfl::Generic may hold a bool, so `--mode` alone is valid
  // and the next token is never its value.
  const auto bare = read<WithVariants>({"--mode", "--extra", "in.txt"});
  ASSERT_TRUE(bare) << bare.error().what();
  EXPECT_EQ(bare.value().mode, (std::variant<bool, std::string>(true)));
  EXPECT_EQ(bare.value().input_file(), "in.txt");

  const auto at_end = read<WithVariants>({"--extra=1", "--mode"});
  ASSERT_TRUE(at_end) << at_end.error().what();
  EXPECT_EQ(at_end.value().mode, (std::variant<bool, std::string>(true)));

  const auto with_equals = read<WithVariants>({"--mode=fast", "--extra=x"});
  ASSERT_TRUE(with_equals) << with_equals.error().what();
  EXPECT_EQ(with_equals.value().mode,
            (std::variant<bool, std::string>(std::string("fast"))));
}

struct ShortVariants {
  rfl::Short<"m", std::variant<int, std::string>> mode;
  rfl::Short<"g", std::optional<rfl::Generic>> extra;
  rfl::Positional<std::optional<std::string>> input_file;
};

TEST(cli, test_short_variant_and_generic_take_next_token) {
  // A short alias of a variant or rfl::Generic takes the next token unless
  // it looks like an option.
  const auto word = read<ShortVariants>({"-m", "fast", "in.txt"});
  ASSERT_TRUE(word) << word.error().what();
  EXPECT_EQ(word.value().mode(),
            (std::variant<int, std::string>(std::string("fast"))));
  EXPECT_EQ(word.value().input_file(), "in.txt");

  const auto number = read<ShortVariants>({"-m", "5", "-g", "x"});
  ASSERT_TRUE(number) << number.error().what();
  EXPECT_EQ(number.value().mode(), (std::variant<int, std::string>(5)));
  ASSERT_TRUE(number.value().extra().has_value());
  EXPECT_EQ(number.value().extra()->to_string().value(), "x");
  EXPECT_FALSE(number.value().input_file().has_value());
}

TEST(cli, test_long_space_range_overload) {
  // The range overload of rfl::cli::read uses the option table too.
  const std::vector<std::string> args = {"--sql", "SELECT 1", "--verbose",
                                         "in.txt"};
  const auto result = rfl::cli::read<Query>(args);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().sql, "SELECT 1");
  EXPECT_EQ(result.value().verbose, true);
  EXPECT_EQ(result.value().input_file(), "in.txt");
}

// The `Config` of docs/content/supported_formats/cli.md.
struct DocsConfig {
  std::string host_name;
  int port;
  bool verbose;
  std::optional<double> rate;
  std::vector<std::string> tags;
};

TEST(cli, test_long_space_docs_examples) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"--host-name=localhost", "--port=8080", "--verbose",
            "--tags=a,b,c"},
           {"--host-name", "localhost", "--port", "8080", "--verbose",
            "--tags", "a,b,c", "--rate", "-0.5"}}) {
    const auto result = read<DocsConfig>(args);
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().host_name, "localhost");
    EXPECT_EQ(result.value().port, 8080);
    EXPECT_TRUE(result.value().verbose);
    EXPECT_EQ(result.value().tags,
              (std::vector<std::string>{"a", "b", "c"}));
  }
}

struct Node {
  int value;
  std::unique_ptr<Node> next;
};

TEST(cli, test_long_space_recursive_struct) {
  const auto result = read<Node>({"--value", "1", "--next.value=2"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().value, 1);
  ASSERT_TRUE(result.value().next);
  EXPECT_EQ(result.value().next->value, 2);
}

struct KebabConfig {
  std::string host_name;
  bool dry_run;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_long_space_kebab_case) {
  const auto result =
      read<KebabConfig>({"--host-name", "localhost", "--dry-run", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().host_name, "localhost");
  EXPECT_TRUE(result.value().dry_run);
  EXPECT_EQ(result.value().input_file(), "in.txt");
}

TEST(cli, test_long_space_wide_char) {
  const wchar_t* args[] = {L"program", L"--sql", L"SELECT 1", L"--verbose",
                           L"in.txt"};
  const auto result =
      rfl::cli::read<Query>(5, const_cast<wchar_t**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().sql, "SELECT 1");
  EXPECT_EQ(result.value().verbose, true);
  EXPECT_EQ(result.value().input_file(), "in.txt");
}

TEST(cli, test_parse_argv_without_long_options) {
  // Without the target struct's long options, parse_argv cannot tell
  // whether --sql takes a value, so the next token stays positional.
  const char* args[] = {"program", "--sql", "SELECT 1"};
  const auto parsed = rfl::cli::parse_argv(3, const_cast<char**>(args));
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed.value().named.at("sql"), "");
  EXPECT_EQ(parsed.value().positional, (std::vector<std::string>{"SELECT 1"}));

  const wchar_t* wargs[] = {L"program", L"--sql", L"SELECT 1"};
  const auto wparsed = rfl::cli::parse_argv(3, const_cast<wchar_t**>(wargs));
  ASSERT_TRUE(wparsed) << wparsed.error().what();
  EXPECT_EQ(wparsed.value().named.at("sql"), "");
  EXPECT_EQ(wparsed.value().positional,
            (std::vector<std::string>{"SELECT 1"}));
}

TEST(cli, test_parse_argv_with_long_options) {
  const auto options =
      rfl::cli::internal::make_options<Query, rfl::Processors<>>();
  EXPECT_EQ(options.find_long("sql").kind, rfl::cli::internal::OptionKind::value);
  EXPECT_EQ(options.find_long("verbose").kind, rfl::cli::internal::OptionKind::flag);
  EXPECT_EQ(options.find_long("input_file").kind, rfl::cli::internal::OptionKind::value);
  EXPECT_EQ(options.find_long("nope").kind, rfl::cli::internal::OptionKind::unknown);

  const wchar_t* args[] = {L"program", L"--sql", L"SELECT 1"};
  const auto parsed =
      rfl::cli::internal::parse_argv(3, const_cast<wchar_t**>(args), options);
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed.value().named.at("sql"), "SELECT 1");
  EXPECT_TRUE(parsed.value().positional.empty());
}

TEST(cli, test_short_options) {
  const auto result = read<Wrapped>(
      {"--sql=x", "--limit=1", "--outFile=o", "-p", "-1", "in.csv"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().port(), -1);
  EXPECT_EQ(result.value().input_file(), "in.csv");
}

}  // namespace test_long_option_values
