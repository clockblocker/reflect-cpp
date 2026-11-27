---
breadcrumbs: true
---

# JSON

For JSON support, you must also include the header `<rfl/json.hpp>`.

## Reading and writing

Suppose you have a struct like this:

```cpp
struct Person {
    rfl::Rename<"firstName", std::string> first_name;
    rfl::Rename<"lastName", std::string> last_name;
    rfl::Timestamp<"%Y-%m-%d"> birthday;
    std::vector<Person> children;
};
```

You can parse JSON strings like this:

```cpp
const rfl::Result<Person> result = rfl::json::read<Person>(json_string);
```

A `Person` struct can be serialized like this:

```cpp
const auto person = Person{...};
const std::string json_string = rfl::json::write(person);
```

If you want a "pretty" JSON representation, you can do this:

```cpp
const std::string json_string = rfl::json::write(person, rfl::json::pretty);
```

## Loading and saving

You can also load and save to disc using a very similar syntax:

```cpp
const rfl::Result<Person> result = rfl::json::load<Person>("/path/to/file.json");

const auto person = Person{...};
rfl::json::save("/path/to/file.json", person);
```

`rfl::json::pretty` will work here as well:

```cpp
rfl::json::save("/path/to/file.json", person, rfl::json::pretty);
```

## Reading from and writing into streams

You can also read from and write into any `std::istream` and `std::ostream` respectively.

```cpp
const rfl::Result<Person> result = rfl::json::read<Person>(my_istream);

const auto person = Person{...};
rfl::json::write(person, my_ostream);
```

`rfl::json::pretty` will work here as well:

```cpp
rfl::json::write(person, my_ostream, rfl::json::pretty);
```

Note that `std::cout` is also an ostream, so this works as well:

```cpp
rfl::json::write(person, std::cout) << std::endl;
```

## Record-level streaming

For large datasets or live streams, `rfl::json::StreamReader` and `rfl::json::StreamWriter` read and write one record at a time without loading the entire document into memory. Three framing formats are supported via `rfl::json::StreamFormat`:

- `StreamFormat::ndjson` — Newline-delimited JSON (`{...}\n{...}\n`, default)
- `StreamFormat::json_seq` — RFC 7464 JSON Text Sequences (`\x1e{...}\n`)
- `StreamFormat::json_array` — Top-level JSON array (`[{...},{...}]`)

```cpp
// Writing records incrementally
rfl::json::StreamWriter writer(my_ostream, rfl::json::StreamFormat::ndjson);
for (const auto& person : people) {
    writer.write(person).value();
}
writer.finish().value();
writer.flush().value();

// Reading records incrementally (C++20 input_range or next())
rfl::json::StreamReader<Person> reader(my_istream, rfl::json::StreamFormat::ndjson);
for (auto&& res : reader) {
    if (res) {
        process(*res);
    } else {
        log_error(res.error().what());
    }
}
```

`StreamReader` also accepts a `std::string_view` directly (zero-copy), supports processors such as `rfl::SnakeCaseToCamelCase` and `rfl::NoExtraFields`, enforces a configurable `max_record_bytes` cap (64 MiB default, `0` for unlimited), and recovers from per-record syntax errors in `ndjson` (advancing to the next line) and `json_seq` (resyncing to the next `\x1e`). `StreamWriter::write()`, `flush()`, and `finish()` return `rfl::Result<rfl::Nothing>`; pretty-printing flags are rejected for `ndjson` and `json_seq`.

## Read and write flags

`rfl::json` provides `inline constexpr` flag constants (combinable with `|`) so you do not need to reference underlying `YYJSON_*` macros directly.

### Read flags (`rfl::json::read` and `rfl::json::load`)

| Flag | Description |
| --- | --- |
| `rfl::json::allow_comments` | Allow C-style single-line (`//`) and multi-line (`/* */`) comments |
| `rfl::json::allow_trailing_commas` | Allow a single trailing comma at the end of an object or array |
| `rfl::json::jsonc` | Shorthand for `allow_comments \| allow_trailing_commas` (JSONC) |
| `rfl::json::allow_bom` | Allow and skip a leading UTF-8 BOM |
| `rfl::json::stop_when_done` | Stop after the first JSON value instead of failing on trailing content (e.g. NDJSON) |
| `rfl::json::read_allow_inf_and_nan` | Allow `Inf`, `Infinity`, and `NaN` numbers and literals |
| `rfl::json::read_allow_invalid_unicode` | Allow invalid UTF-8 byte sequences in string values |
| `rfl::json::allow_ext_number` | Allow hex numbers, leading/trailing decimal points, and leading `+` |
| `rfl::json::allow_ext_escape` | Allow extended string escape sequences |
| `rfl::json::allow_ext_whitespace` | Allow extended whitespace characters |
| `rfl::json::allow_single_quoted_str` | Allow single-quoted strings |
| `rfl::json::allow_unquoted_key` | Allow unquoted object keys |
| `rfl::json::json5` | Allow JSON5 syntax |

Example reading JSONC:

```cpp
const auto result = rfl::json::read<Person>(
    jsonc_string, rfl::json::allow_comments | rfl::json::allow_trailing_commas);

// Or using the convenience alias:
const auto result2 = rfl::json::read<Person>(jsonc_string, rfl::json::jsonc);
```

### Write flags (`rfl::json::write`, `rfl::json::save`, and `rfl::json::to_schema`)

| Flag | Description |
| --- | --- |
| `rfl::json::pretty` | Pretty-print with 4-space indentation |
| `rfl::json::pretty_two_spaces` | Pretty-print with 2-space indentation (overrides `pretty`) |
| `rfl::json::newline_at_end` | Append a trailing `\n` to the output |
| `rfl::json::escape_unicode` | Escape non-ASCII characters as `\uXXXX` for ASCII-only output |
| `rfl::json::escape_slashes` | Escape `/` as `\/` |
| `rfl::json::write_allow_inf_and_nan` | Write `Inf`/`NaN` as `Infinity`/`NaN` literals (non-standard) |
| `rfl::json::inf_and_nan_as_null` | Write `Inf`/`NaN` as `null` (overrides `write_allow_inf_and_nan`) |
| `rfl::json::write_allow_invalid_unicode` | Allow invalid UTF-8 byte sequences when encoding strings |

Example combining write flags:

```cpp
const std::string json_string = rfl::json::write(
    person, rfl::json::pretty_two_spaces | rfl::json::newline_at_end);
```

> Note: `read_allow_inf_and_nan` / `write_allow_inf_and_nan` and `read_allow_invalid_unicode` / `write_allow_invalid_unicode` are prefixed with `read_` / `write_` because the underlying read and write flag bit values differ.

## Custom constructors

One of the great things about C++ is that it gives you control over
when and how you code is compiled.

For large and complex systems of structs, it is often a good idea to split up
your code into smaller compilation units. You can do so using custom constructors.

For the JSON format, these must be a static function on your struct or class called
`from_json_obj` that take a `rfl::json::Reader::InputVarType` as input and return
the class or the class wrapped in `rfl::Result`.

In your header file you can write something like this:

```cpp
struct Person {
    rfl::Rename<"firstName", std::string> first_name;
    rfl::Rename<"lastName", std::string> last_name;
    rfl::Timestamp<"%Y-%m-%d"> birthday;

    using JSONVar = typename rfl::json::Reader::InputVarType;
    static rfl::Result<Person> from_json_obj(const JSONVar& _obj);
};
```

And in your source file, you implement `from_json_obj` as follows:

```cpp
rfl::Result<Person> Person::from_json_obj(const JSONVar& _obj) {
    const auto from_nt = [](auto&& _nt) {
        return rfl::from_named_tuple<Person>(std::move(_nt));
    };
    return rfl::json::read<rfl::named_tuple_t<Person>>(_obj)
        .transform(from_nt);
}
```

This will force the compiler to only compile the JSON parsing when the
source file is compiled.

## Raw JSON Passthrough (`rfl::json::Raw`)

Use `rfl::json::Raw` when a struct contains a JSON subtree that you want to
pass through without converting into intermediate C++ types:

```cpp
struct Envelope {
    int id;
    rfl::json::Raw payload;
};

const auto env = rfl::json::read<Envelope>(
    R"({"id":1,"payload":{"nested":[1,2,3]}})").value();
const auto out = rfl::json::write(env);
```

When constructing `rfl::json::Raw` from untrusted input, prefer the validating
factory `rfl::json::Raw::from_string(...)` (which returns `rfl::Result<rfl::json::Raw>`)
rather than the unchecked `explicit` constructors, because writing emits the
stored JSON text verbatim via `yyjson_mut_rawncpy`.

On read, `rfl::json::Raw` captures the parsed `yyjson` subtree via
`yyjson_val_write`, which canonicalizes whitespace. Pass `YYJSON_READ_BIGNUM_AS_RAW`
to `rfl::json::read` if integers wider than 64 bits must be preserved verbatim.
`rfl::json::Raw` is supported with `rfl::json` only.

