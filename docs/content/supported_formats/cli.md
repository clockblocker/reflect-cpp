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

`Short` and `Positional` can be nested with `DefaultVal`, `std::optional`,
`Description`, `Rename` and `Deprecated` in any order:
`rfl::Description<"...", rfl::Short<"o", T>>` is `-o`, and
`rfl::DefaultVal<rfl::Positional<T>>` is positional. Put `rfl::Rename`
outermost, as in `rfl::Rename<"out", rfl::Short<"o", T>>`; inside other
wrappers it is ignored.

Bare arguments go to the `Positional` fields in declaration order. Two fields
with the same `Short` name make every read fail with
`Duplicate short name '-o' in struct definition.` A field with more than one
`Short` or `Positional`, such as `rfl::Positional<rfl::Short<"o", T>>` or
`rfl::Short<"o", rfl::Description<"...", rfl::Positional<T>>>`, is a
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

## Usage / help

`rfl::cli::usage<T, Ps...>(program_name, detailed = false)` generates the
help text from the same struct, and with the same processors, as
`rfl::cli::read<T, Ps...>()`. So the names always match what the parser
accepts, including kebab-case, `rfl::Rename`, `rfl::Flatten` and nested paths.
It returns an `rfl::Result<std::string>`.

```cpp
enum class LogLevel { debug, info, warn };

struct Database {
  std::string host;
  int port;
};

struct Config {
  rfl::Positional<rfl::Description<"Path to the input file", std::string>>
      input_file;
  rfl::Short<"p", rfl::Description<"Port to listen on", int>> port;
  rfl::Description<"Minimum level to log", std::optional<LogLevel>> log_level;
  rfl::Description<"Tags to attach", std::vector<std::string>> tags;
  Database database;
};
```

`rfl::cli::usage<Config>("app", true).value()` is the usage line, where
optional arguments are in brackets, and one line per argument, with its
description and whether it is required. `rfl::cli::usage<Config>("app")`
gives only the usage line.

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

Fields wrapped in `rfl::DefaultVal` are optional, and so are all fields if you
pass `rfl::DefaultIfMissing` (to both `read` and `usage`). The parser takes
the defaults of `rfl::DefaultVal` fields from a default-constructed struct. If
a struct with `rfl::DefaultVal` cannot be default-constructed (for instance
because a validator rejects another field's default value), it cannot be read,
and `usage` returns the error, as it does for two fields with the same short
name.

Each argument is shown as the option table that `read` parses with (see
[Architecture](#architecture)) reads it: a `bool`, also inside a nested
struct, is a flag whose value can be left out (`--verbose[=<bool>]`), and
every other option takes a value (`--port=<value>`).

`rfl::Positional` and `rfl::Short` are found through the other wrappers, as
the parser does (see [Combining wrappers](#combining-wrappers)), so
`rfl::Description<"...", rfl::Positional<std::string>>` is listed as a
positional argument.

## Help and version

`--help` and `--version` are off by default: `rfl::cli::read` treats them
as it treats any option that matches no field. To turn them on, pass the
setting `rfl::cli::App<"name", "version", "description">` (each part is
optional) and call `rfl::cli::read_app` instead of `read`. It returns either
the struct or an `rfl::cli::HelpOrVersion` with the text to print:

```cpp
using CopyApp = rfl::cli::App<"copy", "1.2.0", "Copies a file.">;

struct Copy {
  rfl::Positional<rfl::Description<"File to copy", std::string>> source;
  rfl::Short<"o", rfl::Description<"Where to copy it", std::string>> output;
  rfl::Short<"v", std::optional<bool>> verbose;
  rfl::Short<"j", rfl::DefaultVal<int>> jobs = 1;
};

int main(int argc, char* argv[]) {
  const auto result = rfl::cli::read_app<Copy, CopyApp>(argc, argv);
  if (!result) {
    std::cerr << result.error().what() << "\n\n"
              << rfl::cli::usage<Copy, CopyApp>(argv[0]);
    return 2;
  }
  if (const auto* info = std::get_if<rfl::cli::HelpOrVersion>(&*result)) {
    std::cout << info->text;  // info->kind is help or version
    return 0;
  }
  const Copy& copy = std::get<Copy>(*result);
  // ...
}
```

`./copy --help` prints the detailed usage, with a header from the setting
and rows for the two options:

```
copy 1.2.0
Copies a file.

Usage: ./copy [-h|--help] [--version] <source> -o|--output=<value> [-v|--verbose[=<bool>]] [-j|--jobs=<value>]

Arguments:
  -h, --help              Show this help
  --version               Show the version
  <source>                File to copy. Required.
  -o, --output=<value>    Where to copy it. Required.
  -v, --verbose[=<bool>]
  -j, --jobs=<value>
```

and `./copy --version` prints `copy 1.2.0`.

- The help is returned before the struct is read, so it works when required
  fields are missing or other values are invalid. A token before it that
  cannot be split into options at all, such as `-o` without its value, is
  still an error. If both are given, the help wins.
- `-h`, `--help` and `--version` are flags in the same option table as the
  fields, so `-vh` works, `--` ends them, and `--help=false` does not ask for
  the help.
- **A field that takes their names is a compile-time error**, as in
  argparse (`conflicting option string: -h`): a field with
  `rfl::Short<"h", ...>`, wherever `rfl::Short` sits among its wrappers, a
  field named `help`, and, if there is a version, a field named `version`.
  To use those names, opt out of the built-in option with the fourth
  parameter, `rfl::cli::BuiltinHelp`:

  ```cpp
  // -h is the field's, the help is --help only.
  using Client = rfl::cli::App<"client", "1.0", "", rfl::cli::BuiltinHelp::long_only>;
  // No help at all, like argparse's add_help=False: a field can be `help`.
  using Tool = rfl::cli::App<"tool", "1.0", "", rfl::cli::BuiltinHelp::none>;
  ```

- An empty version leaves out `--version`, so a field can be `version`. An
  empty name is replaced by the program name (`argv[0]`).
- Pass the same settings to `usage` as to `read_app`: `usage` shows the
  header (in the detailed form only) and the rows of the options that
  `read_app` handles.
- `read_app` is in `rfl/cli/read_app.hpp`, which `rfl/cli.hpp` includes.
  It takes `argc` and `argv` (narrow or wide, const or not), since
  the help and the version show the program name. `rfl::cli::read<T,
  rfl::cli::App<...>>` does not compile, in any of its forms: its result has
  no room for a help request. Other formats ignore the setting.

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

- `include/rfl/cli/read.hpp` — public API: `read`
- `include/rfl/cli/read_app.hpp` — public API: `read_app`, with `--help` and `--version`
- `include/rfl/cli/App.hpp` — `App<"name", "version", "description", BuiltinHelp>` setting for `--help` and `--version`
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
