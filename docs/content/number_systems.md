# `rfl::Binary`, `rfl::Hex` and `rfl::Oct` 

`rfl::Binary`, `rfl::Hex` and `rfl::Oct` can be used to express numbers
in binary, hex and octal format.

```cpp
struct ExampleStruct {
  rfl::Binary<uint8_t> binary;
  rfl::Hex<int> hex;
  rfl::Oct<int> oct;
};

const auto example = ExampleStruct{
    .binary = 30,
    .hex = 30,
    .oct = 30};
```

This results in the following JSON:

```json
{"binary":"00011110","hex":"1e","oct":"36"}
```

Note that the contained type must be integral for `rfl::Hex` and `rfl::Oct`. For `rfl::Binary`, it must be unsigned. Moreover, 
the number of digits for `rfl::Binary` will be determined by the bitsize of the type.

You can access the contained value using `.value()`, `.get()`, `operator()()`, `operator*()`, or `operator->()` (const and non-const overloads).

You can produce the string representation using `.str()`.

## `rfl::Stringified`

`rfl::Stringified<T>` serializes and deserializes integral values as base-10 strings. This is useful when interoperating with JavaScript/TypeScript frontends, Protobuf JSON APIs, or Snowflake-style IDs where 64-bit integers exceed IEEE-754 double precision (`2^53 - 1`).

```cpp
struct Record {
  rfl::Stringified<int64_t> id;
  rfl::Stringified<uint64_t> sequence;
};

const auto record = Record{
    .id = 9007199254740993LL,
    .sequence = 18446744073709551615ULL};
```

This results in the following JSON:

```json
{"id":"9007199254740993","sequence":"18446744073709551615"}
```

Parsing uses `std::from_chars` and returns an `rfl::Error` on empty strings, whitespace, non-numeric characters, negative values for unsigned types, or out-of-range integers.

Like `rfl::Binary`, `rfl::Hex`, and `rfl::Oct`, you can access the underlying value with `.value()`, `.get()`, `operator*()`, `operator()()`, or `operator->()`, and format it with `.str()`.

