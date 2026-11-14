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

## Long options

A long option takes its value either after `=` or as the next token:

```
./app --host-name=localhost --port=8080 --verbose --tags=a,b,c
./app --host-name localhost --port 8080 --verbose --tags a,b,c --rate -0.5
```

This works for every field that takes a value, including nested fields
(`--database.host db.local`), entries of maps with string keys
(`--labels.env prod`; other maps need `=`) and fields inside wrappers such
as `rfl::Short`, `rfl::Description` or `rfl::DefaultVal`. The next token is
the value unless it looks like an option (`-` followed by anything except a
digit or `.`), so `--offset -5` works. If the value is missing, parsing
fails with `Missing value for option '--sql'.` Use `=` to pass an empty
value or one that starts with `-`: `--sql=`, `--pattern=-x`. For an option
whose value may be left out, use `rfl::ImplicitValue` (below).

Boolean fields (`bool`, `std::optional<bool>` and wrapped bools) are flags
and never consume the next token: `--verbose input.txt` sets `verbose` and
passes `input.txt` as a positional argument, and so does `--verbose 0` with
`0`. To set a flag to `false`, write `--verbose=false`. A `std::variant`,
`rfl::Variant`, `rfl::TaggedUnion` or `rfl::Generic` may hold a bool, so
its long option never takes the next token either: pass its value with `=`.

The public `rfl::cli::parse_argv` does not know the struct, so it reads
`--sql SELECT` as `--sql` followed by a positional argument.

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
// ./app --nope=1   ->  error: Unknown option '--nope'.
```

Unknown short options are always an error.

## Abbreviated long options

A long option can be shortened to a prefix that matches only one option:
with fields `report` and `format`, `--rep=a`, `--rep a` and `--form` stand
for `--report=a`, `--report a` and `--format`. This works for flags,
`rfl::ImplicitValue` options, positional fields given by name and nested
fields, where only the last part of the dotted name is shortened:
`--database.h` stands for `--database.host`, but `--data` and `--database.`
stand for nothing.

- An exact name always wins: `--format` is `--format` even when
  `--format-version` exists.
- A prefix of several options is an error, even without `rfl::NoExtraFields`:
  `Option '--for' is ambiguous; possibilities: '--force-export', '--format'.`
  A field has only one long name, so the matches are always different
  fields.
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

## Lists and repeated options

A field that is read as a list (`std::vector`, `std::set`, `std::deque` and
the like, `std::array`, `rfl::Array`, tuples and pairs, inside any of the
wrappers above) takes comma-separated items. Its option can also be
repeated: every occurrence is split at the commas, and the items are added
in argv order.

```cpp
struct Config {
  rfl::Short<"r", std::vector<std::string>> report;
};

// ./app --report a,b --report c        ->  report = {"a", "b", "c"}
// ./app -r a --report b --report=c     ->  report = {"a", "b", "c"}
```

- Long and short spellings of a field all add to it.
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

- The delimiter is a single ASCII character, such as `;`, `:` or a space.
  A longer one would make values such as `a;;;b` ambiguous; repeat the
  option instead.
- It applies to every list option and list positional argument of the
  struct. Other fields are never split.
- Without a delimiter, each occurrence, and each positional argument, is
  exactly one item, even if it is empty.
- Other formats ignore it.

### Empty list items

By default, empty items in a split list are skipped: `--tags=a,,b` gives
`[a, b]`. Pass `rfl::cli::KeepEmptyItems` to keep each empty item as a
missing value at its index. The element type then decides, as it does for a
missing field:

```cpp
const auto result =
    rfl::cli::read<Config, rfl::cli::KeepEmptyItems>(argc, argv);
```

| Element type | `--tags=a,,b` |
|---|---|
| `std::vector<std::optional<T>>` | `[a, nullopt, b]` |
| `std::vector<rfl::DefaultVal<T>>` | `[a, T{}, b]` |
| `std::vector<T>` | Error: `Empty item at index 1 of '--tags'.` |

`T` is any type that needs a value, such as `std::string`, `std::wstring`,
a number or a `bool`. Smart pointers (`std::shared_ptr<T>`,
`std::unique_ptr<T>`) are never required, so an empty one is null, as a
missing field of that type is.

Leading and trailing empty items (`,b`, `a,`) count, too. An empty value
(`--tags=`) is still an empty list. Other list types, such as `std::set`,
`std::deque` and `std::array`, behave the same way. A
[repeated option](#lists-and-repeated-options) keeps the empty items of
every occurrence, at their index in the combined list: `--tags a,b --tags
c,,d` has an empty item at index 3. With `rfl::cli::ListDelimiter<"">`,
lists are not split, so the setting changes nothing.

### A list as the last positional argument

If the last positional field is a list, it takes all remaining positional
arguments (last in the order of [Combining wrappers](#combining-wrappers),
which is declaration order unless some `Positional`s are wrapped), and each
argument is split at the commas:

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
- Options can still come anywhere (`./app run a --verbose b`).
- The long form works as for any list: `--files=a,b`, or
  `--files a --files b`. It cannot be mixed with bare arguments:
  `Conflict: positional argument and '--files' both provided.`
- Only the last positional field takes the remaining arguments. A list
  before other positional fields, such as `std::vector`, `std::set`, a
  pair or a `std::array`, takes one argument, split at the commas.

## Combining wrappers

`Short`, `Positional` and `ImplicitValue` can be nested with each other and
with `DefaultVal`, `std::optional`, `Description`, `Rename` and `Deprecated`
in any order: `rfl::Description<"...", rfl::Short<"o", T>>` is `-o`, and
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

`rfl::cli::usage<Config>("app", true)` returns the usage line, where
optional arguments are in brackets, and one line per argument, with its
description, the constraints of its validators, its default and whether it
is required. `rfl::cli::usage<Config>("app")` returns only the usage line.

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
  -j, --jobs=<value>      Default: 1.
```

and `./copy --version` prints `copy 1.2.0`.

- The help is returned before the struct is read, so it works when required
  fields are missing or other values are invalid. A token before it that
  cannot be split into options at all, such as `-o` without its value, is
  still an error. If both are given, the help wins.
- `-h`, `--help` and `--version` are flags in the same option table as the
  fields, so `--hel` and `-vh` work, `--ver` is ambiguous if there is also a
  `--verbose`, and `--` ends them. `--help=false` does not ask for the help.
- **A field takes precedence.** If a field has `rfl::Short<"h", ...>`, `-h`
  is that field's and the help is `--help` only. That includes an
  `rfl::Short` inside other wrappers, unless another field has the same
  short name, in which case the parser gives `-h` to neither field (see
  [Combining wrappers](#combining-wrappers)) and it stays the help. A field
  named `help` turns the built-in help off and is read like any other
  field, so you can define your own, for instance `--help[=<topic>]` with
  `rfl::ImplicitValue<std::optional<std::string>, "all"> help;`. A field
  named `version` turns `--version` off.
- An empty version leaves out `--version`. An empty name is replaced by the
  program name (`argv[0]`).
- Pass the same settings to `usage` as to `read_app`: `usage` shows the
  header (in the detailed form only) and the rows of the options that
  `read_app` handles.
- `read_app` takes `argc` and `argv` (narrow or wide, const or not), since
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
| `std::vector<T>` | `--key=a,b,c` or `--key a --key b,c` | Comma-separated; empty items skipped unless [kept](#empty-list-items); [repeatable](#lists-and-repeated-options) |
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

- `include/rfl/cli/read.hpp` — public API: `read` and `read_app`
- `include/rfl/cli/App.hpp` — `App<"name", "version", "description">` setting for `--help` and `--version`
- `include/rfl/cli/Reader.hpp` — Reader + `parse_value` overloads
- `include/rfl/cli/Parser.hpp` — Parser type alias + empty `DefaultVal` and `std::wstring` items
- `include/rfl/cli/KeepEmptyItems.hpp` — `KeepEmptyItems` processor
- `include/rfl/cli/parse_argv.hpp` — `argv` -> `ParsedArgs`
- `include/rfl/cli/options.hpp` — the internal option table: the long, short and positional options of a struct
- `include/rfl/cli/NoAbbreviations.hpp` — `NoAbbreviations` processor
- `include/rfl/cli/ListDelimiter.hpp` — `ListDelimiter<"x">` processor
- `include/rfl/cli/resolve_args.hpp` — `ParsedArgs` -> `map<string, string>`
- `include/rfl/cli/usage.hpp` — `usage()` help text, built on `rfl::parsing::schema` and the internal option table
- `include/rfl/cli.hpp` — aggregator header
- `include/rfl/SnakeCaseToKebabCase.hpp` — processor
- `include/rfl/Positional.hpp` — `Positional<T>` wrapper
- `include/rfl/Short.hpp` — `Short<"x", T>` wrapper
- `include/rfl/ImplicitValue.hpp` — `ImplicitValue<T, "x">` wrapper
