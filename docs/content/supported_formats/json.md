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

