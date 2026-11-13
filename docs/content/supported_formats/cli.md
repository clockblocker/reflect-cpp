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

By default, long options that match no field, not even as an abbreviation
(below), are ignored, and they do not consume the next token. Pass
`rfl::NoExtraFields` to reject them:

```cpp
const auto result = rfl::cli::read<Config, rfl::NoExtraFields>(argc, argv);
// ./app --nope 1   ->  error: Unknown option '--nope'.
```

Unknown short options are always an error.

## Abbreviated long options

As in getopt_long and argparse, a long option can be shortened to a prefix
that matches only one option: with fields `report` and `format`, `--rep=a`,
`--rep a` and `--form` stand for `--report=a`, `--report a` and `--format`.
This works for flags, `rfl::ImplicitValue` options, positional fields given
by name and nested fields, where only the last part of the dotted name is
shortened: `--database.h` stands for `--database.host`, but `--data` and
`--database.` stand for nothing.

- An exact name always wins: `--format` is `--format` even when
  `--format-version` exists.
- A prefix of several options is an error, even without `rfl::NoExtraFields`:
  `Option '--for' is ambiguous; possibilities: '--force-export', '--format'.`
  getopt_long accepts several matches if they all name the same option and
  argparse rejects them, but in rfl a field has only one long name, so the
  matches are always different fields.
- A prefix of no option is an unknown option, as above.
- Short options, map keys and `rfl::ExtraFields` keys are never
  abbreviated. Under `rfl::ExtraFields`, an unknown name is an extra key.

Abbreviations are on by default, so an option that matches no field is
not always unknown: `--rep` sets `--report` if that is its only match. With
a field `verbose_logging` and no `verbose`, `--verbose` sets
`verbose_logging`. `rfl::NoExtraFields` does not stop this: with a field
`port_range` and no `port`, `--port=8080` fills `port_range` instead of
failing. Adding an option can make an abbreviation ambiguous, so scripts
should spell options out. `rfl::cli::NoAbbreviations` accepts exact names
only, and other formats ignore it:

```cpp
rfl::cli::read<Config, rfl::cli::NoAbbreviations>(argc, argv);
// ./app --rep=a   ->  --rep is an unknown option
```

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

## Lists and repeated options

A field that is read as a list (`std::vector`, `std::set`, `std::deque` and
the like, `std::array`, `rfl::Array`, tuples and pairs, inside any of the
wrappers above) takes comma-separated items. Its option can also be
repeated, like Boost's composing options and argparse's `extend`: every
occurrence is split at the commas, and the items are added in argv order.

```cpp
struct Config {
  rfl::Short<"r", std::vector<std::string>> report;
};

// ./app --report a,b --report c        ->  report = {"a", "b", "c"}
// ./app -r a --report b --rep=c        ->  report = {"a", "b", "c"}
```

- Long, abbreviated and short spellings of a field all add to it.
- Repeating is the same as joining with a comma, so the size of a
  `std::array` and validators such as `rfl::Size` see all items. An empty
  occurrence (`--report=`) adds none.
- A bare occurrence of an `rfl::ImplicitValue` list adds its implicit value:
  with `rfl::ImplicitValue<std::optional<std::vector<std::string>>, "all">`,
  `--help-reports --help-reports=cpu` gives `{"all", "cpu"}`.
- Nested lists (`--database.replicas a --database.replicas b`) and map
  entries whose values are lists (`--groups.x a --groups.x b`) work the
  same way.

Any other option can be given only once. That includes nested
fields and map entries: `Duplicate argument: --output`,
`Duplicate short argument: -o`, `Conflict: '-o' and '--output' both provided.`

### List delimiter

`rfl::cli::ListDelimiter<"x">` splits lists at another character, and
`rfl::cli::ListDelimiter<"">` does not split them at all:

```cpp
rfl::cli::read<Config, rfl::cli::ListDelimiter<";">>(argc, argv);
// ./app --report 'a;b' --report c,d    ->  report = {"a", "b", "c,d"}

rfl::cli::read<Config, rfl::cli::ListDelimiter<"">>(argc, argv);
// ./app --report a,b --report ''       ->  report = {"a,b", ""}
```

- The delimiter is a single ASCII character, such as `;`, `:` or a space,
  as in clap and CLI11. A longer one would make values such as `a;;;b`
  ambiguous; repeat the option instead.
- It applies to every list option and list positional argument of the
  struct. Other fields are never split.
- Without a delimiter, each occurrence, and each positional argument, is
  exactly one item, even if it is empty.
- Other formats ignore it: `rfl::json::write`, `rfl::json::read` and
  `rfl::json::to_schema` give the same results with it.

### A list as the last positional argument

If the last positional field is a list, it takes all remaining positional
arguments (last in the order of [Combining wrappers](#combining-wrappers),
which is declaration order unless some `Positional`s are wrapped), like
argparse's `nargs='+'` (or `'*'` if it is optional), and each argument is
split at the commas:

```cpp
struct Config {
  rfl::Positional<std::string> command;
  rfl::Positional<std::vector<std::string>> files;
};

// ./app run a,b        ->  files = {"a", "b"}
// ./app run a b        ->  files = {"a", "b"}
// ./app run a,b c      ->  files = {"a", "b", "c"}
```

- A plain list needs at least one argument. With `std::optional` or
  `rfl::DefaultVal`, no arguments give `std::nullopt` or the default.
- Options can still come anywhere (`./app run a --verbose b`), unless the
  struct is read with `rfl::cli::OptionsFirst` (below).
- The long form works as for any list: `--files=a,b`, or
  `--files a --files b`. It cannot be mixed with bare arguments:
  `Conflict: positional argument and '--files' both provided.`
- Only the last positional field takes the remaining arguments. A list
  before other positional fields, such as `std::vector`, `std::set`, a
  pair or a `std::array`, takes one argument, split at the commas.
- `resolve_args<T, ProcessorsType>`, for arguments parsed without the
  option table, cannot repeat list options, so there a list as the last
  positional field takes one argument.

### Options first

`rfl::cli::OptionsFirst` ends the options at the first positional argument,
like getopt's `+` mode (`POSIXLY_CORRECT`), argparse's `REMAINDER` and
clap's `trailing_var_arg`. Every later token is a positional argument, taken
as it is, even `--x`, `-x`, `--` and unknown options. With
`rfl::cli::ListDelimiter<"">`, a list as the last positional field gets the
rest of the command line unchanged, for example to pass it on to another
program:

```cpp
struct Recipe {
  rfl::Short<"v", std::optional<bool>> verbose;
  rfl::Positional<std::string> name;
  rfl::Positional<rfl::DefaultVal<std::vector<std::string>>> args;
};

const auto result =
    rfl::cli::read<Recipe, rfl::cli::OptionsFirst, rfl::cli::ListDelimiter<"">>(
        argc, argv);

// ./recipe -v build --jobs 4 -v -- a,b
//   ->  verbose = true, name = "build",
//       args = {"--jobs", "4", "-v", "--", "a,b"}
```

**Without `ListDelimiter<"">`, the passed-through arguments are lists like
any other:** each one is split at the commas and empty items are dropped.
`rfl::cli::read<Recipe, rfl::cli::OptionsFirst>` reads
`./recipe build --jobs 4,5 "" x` as
`args = {"--jobs", "4", "5", "x"}`. Pass `ListDelimiter<"">` to get the
arguments unchanged.

- Before the first positional argument, options work as usual, and `--`
  ends them and is dropped (`./recipe -- -v` makes `-v` the name).
- After it, `--` is an argument like any other, as in getopt's `+` mode.
  argparse drops a `--` that is the first token of a `REMAINDER`
  (`recipe build -- x` gives `["x"]`); rfl keeps it.
- A token such as `-5` that is read as a positional argument (see
  [grouped short options](#grouped-short-options)) also ends the options.
- `rfl::NoExtraFields` rejects unknown options before the first
  positional argument.
- Without `OptionsFirst`, options and positional arguments can be mixed.
  Other formats ignore the setting.

## Blank list items

By default, blank items in a list are skipped: `--tags=a,,b` gives `[a, b]`.
Pass `rfl::cli::BlankItemsAsMissing` to keep each blank item as a missing
value at its index. The element type then decides, as it does for a missing
field:

```cpp
const auto result =
    rfl::cli::read<Config, rfl::cli::BlankItemsAsMissing>(argc, argv);
```

| Element type | `--tags=a,,b` |
|---|---|
| `std::vector<std::optional<T>>` | `[a, nullopt, b]` |
| `std::vector<rfl::DefaultVal<T>>` | `[a, T{}, b]` |
| `std::vector<T>` | Error: `Blank item at index 1 of '--tags'.` |

`T` is any type that needs a value, such as `std::string`, `std::wstring`,
a number or a `bool`. Smart pointers (`std::shared_ptr<T>`,
`std::unique_ptr<T>`) are never required, so a blank one is null, as a
missing field of that type is.

Leading and trailing blanks (`,b`, `a,`) are blank items, too. An empty
value (`--tags=`) is still an empty list. Other list types, such as
`std::set`, `std::deque` and `std::array`, behave the same way.

The setting works with the other list features:

- [Repeated options](#lists-and-repeated-options) keep the blank items of
  every occurrence, and the index is the one in the combined list:
  `--tags a,b --tags c,,d` has a blank item at index 3. An empty occurrence
  (`--tags=`) still adds no items.
- [`rfl::cli::ListDelimiter<";">`](#list-delimiter) keeps the blank items
  between two `;`, so `--tags='a;;b,,c'` gives `[a, nullopt, "b,,c"]`.
- With `rfl::cli::ListDelimiter<"">`, every occurrence is one item, so an
  empty one is a blank item: `--tags a --tags ''` gives `[a, nullopt]`, and
  `--tags=` alone gives `[nullopt]`. This includes the rest of the command
  line with [`rfl::cli::OptionsFirst`](#options-first): the pass-through
  `Recipe` above, whose `args` are `std::string`s, fails on an empty
  argument (`./recipe build ''` gives `Blank item at index 0 of '--args'.`).
  Use `std::optional<std::string>` items to keep it as `nullopt`, or leave
  the setting out to keep it as `""`.
- A [list as the last positional field](#a-list-as-the-last-positional-argument)
  splits each argument as an occurrence: `./app run a,,b c` gives
  `[a, nullopt, b, c]`. An error names the field by its long option, as in
  `Blank item at index 1 of '--files'.`

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

The help is built from the same internal option table that `read` parses
with (see [Architecture](#architecture)), so it shows how each argument can
be given:

| Field | Synopsis | Detail |
|---|---|---|
| `bool` | `--verbose[=<bool>]` | |
| `rfl::ImplicitValue<std::optional<Color>, "always">` | `--color[=<never\|always>]` | `Without a value: always.` |
| list (vector, set, array, ...) | `--tags=<v1,v2,...>` | `Can be repeated.` |
| list, with `rfl::cli::ListDelimiter<";">` | `--tags=<v1;v2;...>` | defaults joined with `;` |
| list, with `rfl::cli::ListDelimiter<"">` | `--tags=<value>` | `Can be repeated.` |
| list as the last `rfl::Positional` | `<files>...` | |

`rfl::Positional` and `rfl::Short` are found through the other wrappers, as
the parser does (see [Combining wrappers](#combining-wrappers)), so
`rfl::Description<"...", rfl::Positional<std::string>>` is listed as a
positional argument. The usage line lists the positional arguments in the
order in which the parser fills them, and leaves out a short alias that the
parser does not give a field because another field has it.

With `rfl::cli::OptionsFirst`, the options end at the first positional
argument, so the usage line lists every option before the positional
arguments. The rows of the detailed form keep the declaration order:

```cpp
struct Recipe {
  rfl::Short<"v", std::optional<bool>> verbose;
  rfl::Positional<std::string> name;
  rfl::Positional<rfl::DefaultVal<std::vector<std::string>>> args;
  rfl::Short<"j", std::optional<int>> jobs;
};

rfl::cli::usage<Recipe, rfl::cli::OptionsFirst>("recipe");
// Usage: recipe [-v|--verbose[=<bool>]] [-j|--jobs=<value>] <name> [<args>...]
```

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
| `std::vector<T>` | `--key=a,b,c` or `--key a --key b,c` | Comma-separated; blank items skipped unless [kept](#blank-list-items); [repeatable](#lists-and-repeated-options) |
| Nested struct | `--parent.child=val` | Dot-separated path |
| `rfl::Flatten<T>` | fields inlined | No prefix needed |
| `rfl::Rename<"x", T>` | `--x=val` | Bypasses kebab conversion |
| `rfl::Positional<T>` | bare token | Matched in declaration order; a list last takes the rest |
| `rfl::Short<"x", T>` | `-x value`, `-x=value` or `-xvalue` | Single-character alias, can be grouped (`-qv`) |
| `rfl::ImplicitValue<T, "x">` | `--key` or `--key=value` | Bare means `x` |

## Architecture

Parsing proceeds in three stages:

1. **`parse_argv`** — categorizes raw tokens into `named`, `short_args`,
   and `positional` buckets (`ParsedArgs` struct). `rfl::cli::read` gives
   it an internal option table derived from the target struct, which says
   which long and short options are flags, take a value, have an implicit
   value or are lists, so that `--key value`, grouped short options,
   abbreviated long options and repeated list options work, and whether a
   short option's name looks like a number. The public `parse_argv`, which
   has no table, does none of these: `-abc` is the one short option `abc`,
   and `-5` the short option `5`.
2. **`resolve_args`** — uses the same table to map short aliases to long
   names and to merge positional arguments. Produces a flat
   `map<string, string>`. A list option given more than once keeps its
   first occurrence there and the others, in argv order, beside it. The
   public `resolve_args<T, ProcessorsType>(parsed)`, for arguments parsed
   without the table, returns the flat map alone.
3. **`Reader`** — implements reflect-cpp's `IsReader` concept by presenting
   virtual tree nodes over the flat map. Each node is a `{map*, path}` pair —
   no data copying, just prefix-based lookup via `lower_bound`.

## Files

- `include/rfl/cli/read.hpp` — public API
- `include/rfl/cli/BlankItemsAsMissing.hpp` — setting for blank list items
- `include/rfl/cli/Reader.hpp` — Reader + `parse_value` overloads
- `include/rfl/cli/Parser.hpp` — Parser type alias + blank `DefaultVal` and `std::wstring` items
- `include/rfl/cli/parse_argv.hpp` — `argv` -> `ParsedArgs`
- `include/rfl/cli/options.hpp` — the internal option table: the long, short and positional options of a struct
- `include/rfl/cli/NoAbbreviations.hpp` — `NoAbbreviations` processor
- `include/rfl/cli/ListDelimiter.hpp` — `ListDelimiter<"x">` processor
- `include/rfl/cli/OptionsFirst.hpp` — `OptionsFirst` processor
- `include/rfl/cli/resolve_args.hpp` — `ParsedArgs` -> `map<string, string>`
- `include/rfl/cli/usage.hpp` — `usage()` help text, built on `rfl::parsing::schema` and the internal option table
- `include/rfl/cli.hpp` — aggregator header
- `include/rfl/SnakeCaseToKebabCase.hpp` — processor
- `include/rfl/Positional.hpp` — `Positional<T>` wrapper
- `include/rfl/Short.hpp` — `Short<"x", T>` wrapper
- `include/rfl/ImplicitValue.hpp` — `ImplicitValue<T, "x">` wrapper
