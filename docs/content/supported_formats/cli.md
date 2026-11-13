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

When the arguments do not come straight from `main`, pass them as a range
instead, such as `std::vector<std::string>`, `std::vector<std::string_view>`,
`std::span<char*>` or `const char* const[]`, or as a braced list of
strings. Ranges of wide strings (`std::vector<std::wstring>`,
`const wchar_t*[]`) are converted to UTF-8. The number of arguments is
taken from the range. A null pointer in a range or array of pointers is
rejected with an error. A braced list holds `std::string_view`s, so a null
pointer in it is undefined behaviour at the call site; pass a
`std::vector` or an array of pointers instead.

The parser keeps a `std::string_view` to each narrow argument, so a range
of narrow strings must be multi-pass (a `std::forward_range`) and its
elements must be lvalue strings, pointers or `std::string_view`s. A
single-pass range such as `std::views::istream<std::string>`, or a view
that yields temporary `std::string`s, does not compile; copy the arguments
into a `std::vector<std::string>` first.

**Every element is parsed: unlike `read(argc, argv)`, the first one is not
skipped as the program name.**

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

// Or a braced list:
const auto result2 = rfl::cli::read<Config>(
    {"--host-name=localhost", "--port=8080", "--verbose", "--tags=a,b"});
```

To parse all of main's arguments, use `read(argc, argv)`. Passing
`std::span(argv, argc)` instead would parse the program name as an
argument; for a struct with a `rfl::Positional` field, it becomes the
first positional value.

`rfl::cli::parse_argv` has the same overloads.

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

As in argparse, this changes if the struct has a short option whose name
looks like a number, such as `rfl::Short<"1", bool>` for `ls -1`. Then such a
token is a short option, and negative positional arguments need `--`:
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

Short bool flags do not consume the next token as a value — `-v somefile`
treats `somefile` as a positional argument, not as the value of `-v`.
To explicitly set a bool short flag, use `=` syntax: `-v=true`, `-v=false`.

## Combining Positional and Short

`Positional` and `Short` can be used together in the same struct, but
**cannot be nested** (`Positional<Short<...>>` is a compile-time error):

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

`rfl::cli::usage<Config>("app")` returns the usage line, where optional
arguments are in brackets:

```
Usage: app <input-file> -p|--port=<value> [--log-level=<debug|info|warn>] --tags=<v1,v2,...> --database.host=<value> --database.port=<value>
```

`rfl::cli::usage<Config>("app", true)` adds one line per argument, with its
description, the constraints of its validators, its default and whether it is
required:

```
Usage: app <input-file> -p|--port=<value> [--log-level=<debug|info|warn>] --tags=<v1,v2,...> --database.host=<value> --database.port=<value>

Arguments:
  <input-file>                   Path to the input file. Required.
  -p, --port=<value>             Port to listen on. Range: 1..65535. Required.
  --log-level=<debug|info|warn>  Minimum level to log
  --tags=<v1,v2,...>             Tags to attach. Size: >= 1. Required.
  --database.host=<value>        Required.
  --database.port=<value>        Required.
```

Defaults are shown for fields wrapped in `rfl::DefaultVal`, and for all fields
if you pass `rfl::DefaultIfMissing` (to both `read` and `usage`), as long as
the struct can be default-constructed:

```cpp
struct Server {
  rfl::DefaultVal<std::string> host = "localhost";
  rfl::DefaultVal<int> threads = 4;
  rfl::DefaultVal<bool> dry_run = false;
};
```

```
Arguments:
  --host=<value>      Default: localhost.
  --threads=<value>   Default: 4.
  --dry-run[=<bool>]  Default: false.
```

The parser takes the defaults of `rfl::DefaultVal` fields from a
default-constructed struct. If the struct cannot be default-constructed, for
instance because a validator rejects another field's default value, those
fields are required, and the help says so.

An `rfl::DefaultVal` inside `rfl::Short`, `rfl::Positional`, `rfl::Description`
or `rfl::Deprecated`, in any order, makes the field optional for the parser,
and the help shows it in brackets with its default:

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
  -l, --log-level=<debug|info|warn>  Minimum level to log. Default: info.
```

`rfl::Positional` and `rfl::Short` only take effect as the outermost wrapper
of a field, so put `rfl::Description` and validators inside them. The help
shows what the parser does: a field declared as
`rfl::Description<"...", rfl::Positional<std::string>>` is listed as a named
argument, because that is how it is parsed.

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
| `rfl::Short<"x", T>` | `-x value` or `-x=value` | Single-character alias |

## Architecture

Parsing proceeds in three stages:

1. **`parse_argv`** — categorizes raw tokens into `named`, `short_args`,
   and `positional` buckets (`ParsedArgs` struct). No type information
   needed, except that `rfl::cli::read` tells it whether a short option's
   name looks like a number. On its own, it reads `-5` as a short option.
2. **`resolve_args`** — uses compile-time metadata from the target struct to
   map short aliases to long names, reclaim values from bool short flags,
   and merge positional arguments. Produces a flat `map<string, string>`.
3. **`Reader`** — implements reflect-cpp's `IsReader` concept by presenting
   virtual tree nodes over the flat map. Each node is a `{map*, path}` pair —
   no data copying, just prefix-based lookup via `lower_bound`.

## Files

- `include/rfl/cli/read.hpp` — public API
- `include/rfl/cli/Reader.hpp` — Reader + `parse_value` overloads
- `include/rfl/cli/Parser.hpp` — Parser type alias
- `include/rfl/cli/parse_argv.hpp` — `argv` -> `ParsedArgs`
- `include/rfl/cli/resolve_args.hpp` — `ParsedArgs` -> `map<string, string>`
- `include/rfl/cli/usage.hpp` — `usage()` help text, built on `rfl::parsing::schema`
- `include/rfl/cli.hpp` — aggregator header
- `include/rfl/SnakeCaseToKebabCase.hpp` — processor
- `include/rfl/Positional.hpp` — `Positional<T>` wrapper
- `include/rfl/Short.hpp` — `Short<"x", T>` wrapper
