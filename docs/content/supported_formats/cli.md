---
breadcrumbs: true
---

# rfl::cli — Command-Line Argument Parser

Parse `argc`/`argv` into any reflectable struct via `rfl::cli::read<T>(argc, argv)`,
or a sequence of arguments via `rfl::cli::read<T>(args)`.

## Usage

```cpp
#include <rfl/cli.hpp>

struct Config {
  std::string host_name;
  int port;
  bool verbose;
  std::optional<double> rate;
  std::vector<std::string> tags;
};

int main(int argc, char* argv[]) {
  const auto result = rfl::cli::read<Config>(argc, argv);
  // ./app --host-name=localhost --port=8080 --verbose --tags=a,b,c
}
```

Field names undergo automatic `snake_case` -> `kebab-case` conversion:
`host_name` matches `--host-name`.

`argv` may be `char*[]` or `const char*[]`. `argv[0]` is the program name
and is skipped.

## Passing a sequence of arguments

When the arguments do not come straight from `main`, pass them as a range,
such as `std::vector<std::string>`, `std::vector<std::string_view>`,
`std::span<char*>` or `const char* const[]`, or as a braced list of strings.
Ranges of wide strings (`std::vector<std::wstring>`, `const wchar_t*[]`) are
converted to UTF-8. A null pointer in a range or array of pointers is an
error. A braced list holds `std::string_view`s, so a null pointer in it is
undefined behaviour; pass a `std::vector` or an array of pointers instead.

The parser keeps a `std::string_view` to each narrow argument, so a range of
narrow strings must be a `std::forward_range` of lvalue strings, pointers or
`std::string_view`s. `std::views::istream<std::string>`, or a view that
yields temporary `std::string`s, does not compile; copy the arguments into a
`std::vector<std::string>` first.

**Every element is parsed: unlike `read(argc, argv)`, the first one is not
skipped as the program name.** So `std::span(argv, argc)` would read the
program name as an argument (for a struct with a `rfl::Positional` field, as
its first value).

```cpp
// ./app serve --host-name=localhost --port=8080 --verbose --tags=a,b
int main(int argc, char* argv[]) {
  // argv[0] is the program name, argv[1] a subcommand the program handles.
  const auto rest = std::span(argv, argc).subspan(std::min(argc, 2));
  const auto result = rfl::cli::read<Config>(rest);
}

// Arguments from somewhere other than main:
const std::vector<std::string> args = {"--host-name=localhost", "--port=8080",
                                       "--verbose", "--tags=a,b"};
const auto result = rfl::cli::read<Config>(args);
```

`rfl::cli::parse_argv` has the same overloads.

## Unknown options

By default, long options that match no field are ignored. Pass
`rfl::NoExtraFields` to reject them:

```cpp
const auto result = rfl::cli::read<Config, rfl::NoExtraFields>(argc, argv);
// ./app --nope=1   ->  error: Unknown option '--nope'.
```

Unknown short options are always an error.

## Positional arguments

Wrap a field with `rfl::Positional<T>` to accept it as a bare (non-flag) argument:

```cpp
struct Config {
  rfl::Positional<std::string> input_file;
  rfl::Positional<std::string> output_file;
  bool verbose;
};

// ./app input.txt output.txt --verbose
```

Positional arguments are matched in declaration order. They can also be
passed as named arguments: `--input-file=input.txt`.

The `--` separator forces all subsequent tokens into positional:

```
./app --verbose -- --not-a-flag.txt
```

A token that starts with `-` and a digit or `.`, such as `-5`, `-1.5` or
`-1e3`, is a negative number, so it is read as a positional argument (or as
the value of a short option: `-n -5`). `-inf` and `-nan` start with a letter
and are read as short options; pass them after `--` or as `--offset=-inf`.

This changes if the struct has a short option whose name looks like a
number, such as `rfl::Short<"1", bool>` for `ls -1`. Then such a token is a
short option, and negative positional arguments need `--`:
`./app -1 -- -5`. Right after a short option, such a token is still read as
that option's value, as any other token is: `-n -1` sets `n` to `-1`.

## Short aliases

Wrap a field with `rfl::Short<"x", T>` to add a single-character alias:

```cpp
struct Config {
  rfl::Short<"p", int> port;
  rfl::Short<"v", bool> verbose;
  std::string host;
};

// ./app -p 8080 -v --host=localhost
// ./app -p=8080 -v --host=localhost
// ./app --port=8080 --verbose --host=localhost   (long names still work)
```

Short bool flags (`bool`, `std::optional<bool>` and wrapped bools) do not
consume the next token as a value — `-v somefile` treats `somefile` as a
positional argument, not as the value of `-v`. Unlike long flags, they take
the next token if it is `true`, `false`, `1` or `0`: `-v false` sets the
flag to `false`. Write `-v -- 1` to pass such a token as a positional
argument. To explicitly set a bool short flag, use `=` syntax: `-v=true`,
`-v=false`.

Any other short option takes the next token as its value unless that token
looks like an option (`-` followed by anything except a digit or `.`):
`-o out.csv`, `-o=out.csv`. If the value is missing, parsing fails with
`Missing value for option '-o'.` Use `=` to pass an empty value or one that
starts with `-`: `-o=`, `-o=-x`.

### Grouped short options

As in getopt, short options can share one token, and a value can be
attached to its option:

```cpp
struct Config {
  rfl::Short<"q", std::optional<bool>> quiet;
  rfl::Short<"v", std::optional<bool>> verbose;
  rfl::Short<"f", std::optional<std::string>> format;
};

// ./app -qv -fcsv     ->  quiet, verbose, format = "csv"
// ./app -qvfcsv       ->  the same
// ./app -qf csv       ->  quiet, format = "csv"
```

Flags are set up to the first option that takes a value. That option takes
the rest of the token, or the next token if nothing is left. The last
option in a token treats the next token as it would alone (`-qv false` sets
`verbose` to `false`).

- `-f=csv` and `-qf=csv` give `csv`: the `=` is not part of the value.
- A flag inside a group takes no value: `-vq=false` is an error.
- A letter that is not a short option is an error:
  `Unknown short argument: -x (in '-qxf').`
- A token like `-42`, `-1.5` or `-.5` is a negative number, so `-p -42`,
  `-p-42` and a positional `-42` work. If a short option's name is a digit
  (see [Positional arguments](#positional-arguments)), such a token is a
  group of short options instead: `-15` is `-1 -5`, and `-10` and `-1.5`
  fail with `Unknown short argument: -0 (in '-10').` and
  `-. (in '-1.5')`.
- Grouping needs the struct: the public `rfl::cli::parse_argv` reads `-abc`
  as the one short option `abc`.

## Combining wrappers

`Short` and `Positional` can be nested with each other and with
`DefaultVal`, `std::optional`, `Description`, `Rename` and `Deprecated` in
any order: `rfl::Description<"...", rfl::Short<"o", T>>` is `-o`, and
`rfl::DefaultVal<rfl::Positional<T>>` is positional. Put `rfl::Rename`
outermost, as in `rfl::Rename<"out", rfl::Short<"o", T>>`; inside other
wrappers it is ignored.

If a field has several `Short` or `Positional` wrappers, the outermost one
counts: `rfl::Short<"o", rfl::Description<"...", rfl::Positional<T>>>` is
`-o`. A `Short` found through other wrappers counts only if no other field
has its name, so another field `rfl::Short<"o", U>` keeps `-o`. Two fields
whose outermost wrapper is the same `Short` make every read fail with
`Duplicate short name '-o' in struct definition.`

Bare arguments go first to the fields whose outermost wrapper is
`Positional`, then to those where it is found through other wrappers, each in
declaration order. `Short` directly around `Positional` (or the reverse) is a
compile-time error:

```cpp
struct Config {
  rfl::Positional<std::string> input_file;
  rfl::Short<"o", std::string> output_dir;
  rfl::Short<"v", bool> verbose;
  int count;
};

// ./app data.csv -o /tmp/out -v --count=10
```

The type inside `rfl::Short`, `rfl::Positional` or `rfl::Description` can be
a `std::optional` or an `rfl::DefaultVal`, so the argument may be omitted:
with `rfl::Short<"o", rfl::DefaultVal<std::string>> output_dir = "out";`,
leaving out `-o` gives `"out"`.

## Usage / help

`rfl::cli::usage<T, Ps...>(program_name, detailed = false)` generates the
help text from the same struct, and with the same processors, as
`rfl::cli::read<T, Ps...>()`. So the names always match what the parser
accepts, including kebab-case, `rfl::Rename`, `rfl::Flatten` and nested paths.

```cpp
enum class LogLevel { debug, info, warn };

struct Database {
  std::string host;
  int port;
};

struct Config {
  rfl::Positional<rfl::Description<"Path to the input file", std::string>>
      input_file;
  rfl::Short<"p", rfl::Description<"Port to listen on",
                                   rfl::Validator<int, rfl::Minimum<1>,
                                                  rfl::Maximum<65535>>>>
      port;
  rfl::Description<"Minimum level to log", std::optional<LogLevel>> log_level;
  rfl::Description<"Tags to attach",
                   rfl::Validator<std::vector<std::string>,
                                  rfl::Size<rfl::Minimum<1>>>>
      tags;
  Database database;
};
```

`rfl::cli::usage<Config>("app", true)` returns the usage line, where
optional arguments are in brackets, and one line per argument, with its
description and whether it is required. `rfl::cli::usage<Config>("app")`
returns only the usage line.

```
Usage: app <input-file> -p|--port=<value> [--log-level=<debug|info|warn>] --tags=<v1,v2,...> --database.host=<value> --database.port=<value>

Arguments:
  <input-file>                   Path to the input file. Required.
  -p, --port=<value>             Port to listen on. Required.
  --log-level=<debug|info|warn>  Minimum level to log
  --tags=<v1,v2,...>             Tags to attach. Required.
  --database.host=<value>        Required.
  --database.port=<value>        Required.
```

Fields wrapped in `rfl::DefaultVal` are optional, and so are all fields if
you pass `rfl::DefaultIfMissing` (to both `read` and `usage`).

The parser takes the defaults of `rfl::DefaultVal` fields from a
default-constructed struct. If the struct cannot be default-constructed, for
instance because a validator rejects another field's default value, those
fields are required, and the help says so.

An `rfl::DefaultVal` inside `rfl::Short`, `rfl::Positional`, `rfl::Description`
or `rfl::Deprecated`, in any order, makes the field optional for the parser,
and the help shows it in brackets:

```cpp
struct Logging {
  rfl::Short<"l", rfl::Description<"Minimum level to log",
                                   rfl::DefaultVal<LogLevel>>>
      log_level = LogLevel::info;
};
```

```
Usage: app [-l|--log-level=<debug|info|warn>]

Arguments:
  -l, --log-level=<debug|info|warn>  Minimum level to log
```

The help is built from the same internal option table that `read` parses
with (see [Architecture](#architecture)), so it shows how each argument can
be given:

| Field | Synopsis | Detail |
|---|---|---|
| `bool` | `--verbose[=<bool>]` | |

`rfl::Positional` and `rfl::Short` are found through the other wrappers, as
the parser does (see [Combining wrappers](#combining-wrappers)), so
`rfl::Description<"...", rfl::Positional<std::string>>` is listed as a
positional argument. The usage line lists the positional arguments in the
order in which the parser fills them, and leaves out a short alias that the
parser does not give a field because another field has it.

`read` does not handle `--help`. Check for it before calling `read`, and
print the usage line when parsing fails:

```cpp
int main(int argc, char* argv[]) {
  if (std::any_of(argv + 1, argv + argc, [](std::string_view _arg) {
        return _arg == "--help" || _arg == "-h";
      })) {
    std::cout << rfl::cli::usage<Config>(argv[0], true);
    return 0;
  }
  const auto config = rfl::cli::read<Config>(argc, argv);
  if (!config) {
    std::cerr << config.error().what() << "\n\n"
              << rfl::cli::usage<Config>(argv[0]);
    return 1;
  }
  // ...
}
```

(Leave out `-h` if one of your fields uses `rfl::Short<"h", ...>`.)

## Supported types

| Type | CLI format | Notes |
|------|-----------|-------|
| `std::string` | `--key=value` | |
| `int`, `long`, ... | `--key=42` | |
| `float`, `double` | `--key=1.5` | |
| `bool` | `--flag` or `--flag=true` | No `=` implies `true` |
| `enum` | `--key=value_name` | Via `rfl::string_to_enum` |
| `std::optional<T>` | omit for `nullopt` | |
| `std::vector<T>` | `--key=a,b,c` | Comma-separated; empty elements skipped |
| Nested struct | `--parent.child=val` | Dot-separated path |
| `rfl::Flatten<T>` | fields inlined | No prefix needed |
| `rfl::Rename<"x", T>` | `--x=val` | Bypasses kebab conversion |
| `rfl::Positional<T>` | bare token | Matched in declaration order |
| `rfl::Short<"x", T>` | `-x value`, `-x=value` or `-xvalue` | Single-character alias, can be grouped (`-qv`) |

## Architecture

Parsing proceeds in three stages:

1. **`parse_argv`** — categorizes raw tokens into `named`, `short_args`,
   and `positional` buckets (`ParsedArgs` struct). `rfl::cli::read` gives
   it an internal option table derived from the target struct, which says
   what each option does with the next token and whether a short option's
   name looks like a number. The public `parse_argv` has no table: it reads
   `-5` as a short option.
2. **`resolve_args`** — uses the same table to map short aliases to long
   names and to merge positional arguments. Produces a flat
   `map<string, string>`. The public `resolve_args<T, ProcessorsType>(parsed)`,
   for arguments parsed without the table, also returns the tokens that bool
   short flags took to the positional arguments.
3. **`Reader`** — implements reflect-cpp's `IsReader` concept by presenting
   virtual tree nodes over the flat map. Each node is a `{map*, path}` pair —
   no data copying, just prefix-based lookup via `lower_bound`.

## Files

- `include/rfl/cli/read.hpp` — public API
- `include/rfl/cli/Reader.hpp` — Reader + `parse_value` overloads
- `include/rfl/cli/Parser.hpp` — Parser type alias
- `include/rfl/cli/parse_argv.hpp` — `argv` -> `ParsedArgs`
- `include/rfl/cli/options.hpp` — the internal option table: the long, short and positional options of a struct
- `include/rfl/cli/resolve_args.hpp` — `ParsedArgs` -> `map<string, string>`
- `include/rfl/cli/usage.hpp` — `usage()` help text, built on `rfl::parsing::schema` and the internal option table
- `include/rfl/cli.hpp` — aggregator header
- `include/rfl/SnakeCaseToKebabCase.hpp` — processor
- `include/rfl/Positional.hpp` — `Positional<T>` wrapper
- `include/rfl/Short.hpp` — `Short<"x", T>` wrapper
