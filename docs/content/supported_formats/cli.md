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

## Long options

A long option takes its value either after `=` or as the next token:

```
./app --host-name=localhost --port=8080 --verbose --tags=a,b,c
./app --host-name localhost --port 8080 --verbose --tags a,b,c --rate -0.5
```

This works for every field that takes a value: nested fields
(`--database.host db.local`, also inside an `rfl::NamedTuple`), vectors,
enums, numbers, optionals, entries of maps with string keys
(`--labels.env prod`, or `--servers.eu.port 8080` for a map of structs;
other maps need `=`), fields inlined by
`rfl::Flatten`, and fields wrapped in `rfl::Short`, `rfl::Positional`,
`rfl::Rename`, `rfl::Description`, `rfl::Validator` or `rfl::DefaultVal`.
A `std::variant`, `rfl::Variant`, `rfl::TaggedUnion` or `rfl::Generic`
may hold a bool, so its long option never takes the next token: pass its
value with `=`. Alone, `--key` passes an empty value (`true` for a bool).
Its short alias takes the next token unless it looks like an option:
`-m fast`.

The next token is the value unless it looks like an option (`-` followed by
anything except a digit or `.`), so negative numbers such as `--offset -5`
work. If the value is
missing, parsing fails with `Missing value for option '--sql'.` Use `=` to
pass an empty value or one that starts with `-`: `--sql=`, `--pattern=-x`.
Short aliases follow the same rules: `-o out.csv`, `-o=out.csv`, and
`Missing value for option '-o'.` For an option whose value may be left out,
use `rfl::ImplicitValue` (below).

Boolean fields (`bool`, `std::optional<bool>` and wrapped bools) are flags
and never consume the next token: `--verbose input.txt` sets `verbose` and
passes `input.txt` as a positional argument, and so does `--verbose 0` with
`0`. To set a flag to `false`, write `--verbose=false`.

## Implicit values

Wrap a field with `rfl::ImplicitValue<T, "text">` to let its option be given
without a value, like Boost's `implicit_value`:

```cpp
struct Config {
  rfl::Short<"h", rfl::ImplicitValue<std::optional<std::string>>> help;
  rfl::ImplicitValue<std::optional<std::string>, "all"> help_reports;
  rfl::ImplicitValue<rfl::DefaultVal<int>, "2"> verbosity = 0;
  rfl::Positional<std::optional<std::string>> input;
};

// ./app --help-reports            ->  help_reports = "all"
// ./app --help-reports=memory     ->  help_reports = "memory"
// ./app --help-reports in.txt     ->  help_reports = "all", input = "in.txt"
// ./app -h  /  -h=tag  /  -htag   ->  help = ""  /  "tag"  /  "tag"
// ./app --verbosity               ->  verbosity = 2
```

A bare option stands for the text, which is parsed like any other value
(the default text is empty). A value can only be given after `=` or attached
to a short option (`-htag`): the next token is never taken. If the option is
absent, the field is read as usual: `std::optional` gives `std::nullopt`,
`rfl::DefaultVal` its default, and any other field is required. Other formats
read and write `rfl::ImplicitValue` as its inner type, as they do
`rfl::Short`.

## Unknown options

By default, long options that match no field are ignored, and they do not
consume the next token. Pass `rfl::NoExtraFields` to reject them:

```cpp
const auto result = rfl::cli::read<Config, rfl::NoExtraFields>(argc, argv);
// ./app --nope 1   ->  error: Unknown option '--nope'.
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

Short bool flags (`bool`, `std::optional<bool>` and wrapped bools) do not
consume the next token as a value — `-v somefile` treats `somefile` as a
positional argument, not as the value of `-v`. Unlike long flags, they take
the next token if it is `true`, `false`, `1` or `0`: `-v false` sets the
flag to `false`. Write `-v -- 1` to pass such a token as a positional
argument. To explicitly set a bool short flag, use `=` syntax: `-v=true`,
`-v=false`.

### Grouped short options

As in getopt, short options can share one token, and a value can be
attached to its option:

```cpp
struct Config {
  rfl::Short<"q", std::optional<bool>> quiet;
  rfl::Short<"v", std::optional<bool>> verbose;
  rfl::Short<"f", std::optional<std::string>> format;
  rfl::Short<"h", rfl::ImplicitValue<std::optional<std::string>>> help;
};

// ./app -qv -fcsv     ->  quiet, verbose, format = "csv"
// ./app -qvfcsv       ->  the same
// ./app -qf csv       ->  quiet, format = "csv"
// ./app -qh  /  -htag ->  quiet, help = ""  /  help = "tag"
```

Flags are set up to the first option that takes a value. That option takes
the rest of the token, or the next token if nothing is left. An
`rfl::ImplicitValue` option takes the rest of the token if there is any and
its implicit value otherwise, never the next token. The last option in a
token treats the next token as it would alone (`-qv false` sets `verbose`
to `false`).

- `-f=csv` and `-qf=csv` give `csv`, where getopt gives `=csv`.
- A flag inside a group takes no value: `-vq=false` is an error.
- A letter that is not a short option is an error:
  `Unknown short argument: -x (in '-qxf').`
- A token like `-42`, `-1.5` or `-.5` is a negative number, so `-p -42`,
  `-p-42` and a positional `-42` work.
- As in argparse, that changes if a short option's name is a digit, such as
  `rfl::Short<"1", bool>` for `ls -1`. Then such a token is a group of
  short options: `-15` is `-1 -5`, and `-10` and `-1.5` fail with
  `Unknown short argument: -0 (in '-10').` and `-. (in '-1.5')`. Pass
  negative positionals after `--` (`./app -1 -- -1.5 -10`). An option still
  takes a negative number as its value (`-n -5`).

## Combining wrappers

`Short`, `Positional` and `ImplicitValue` can be nested with each other and
with `DefaultVal`, `std::optional`, `Description`, `Rename` and `Deprecated`
in any order: `rfl::Description<"...", rfl::Short<"o", T>>` is `-o`, and
`rfl::DefaultVal<rfl::Positional<T>>` is positional. Put `rfl::Rename`
outermost, as in `rfl::Rename<"out", rfl::Short<"o", T>>`; inside other
wrappers it is ignored, as elsewhere in reflect-cpp.

If a field has several `Short` or `Positional` wrappers, the outermost one
counts: `rfl::Short<"o", rfl::Description<"...", rfl::Positional<T>>>` is
`-o`. A `Short` found through other wrappers counts only if no other field
has its name, so another field `rfl::Short<"o", U>` keeps `-o`. Two fields
whose outermost wrapper is the same `Short` make every read fail with
`Duplicate short name '-o' in struct definition.`

Bare arguments go first to the fields whose outermost wrapper is
`Positional`, then to those where it is found through other wrappers, each in
declaration order. `Short` directly around `Positional` (or the reverse), and
a `Positional` field with an `ImplicitValue`, are compile-time errors:

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
| `rfl::ImplicitValue<T, "x">` | `--key` or `--key=value` | Bare means `x` |

## Architecture

Parsing proceeds in three stages:

1. **`parse_argv`** — categorizes raw tokens into `named`, `short_args`,
   and `positional` buckets (`ParsedArgs` struct). `rfl::cli::read` gives
   it an internal option table derived from the target struct, which says
   which long and short options are flags, take a value or have an implicit
   value, so that `--key value` and grouped short options work, and whether
   a short option's name looks like a number. The public `parse_argv`, which
   has no table, reads `-5` as a short option.
2. **`resolve_args`** — uses the same table to map short aliases to long
   names and to merge positional arguments. Produces a flat
   `map<string, string>`.
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
- `include/rfl/cli.hpp` — aggregator header
- `include/rfl/SnakeCaseToKebabCase.hpp` — processor
- `include/rfl/Positional.hpp` — `Positional<T>` wrapper
- `include/rfl/Short.hpp` — `Short<"x", T>` wrapper
- `include/rfl/ImplicitValue.hpp` — `ImplicitValue<T, "x">` wrapper
