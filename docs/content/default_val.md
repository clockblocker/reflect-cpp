# Default values (rfl::DefaultVal)

The `rfl::DefaultVal<T>` wrapper allows a struct field to have a predefined default value when serializing and deserializing. When a field is declared as `rfl::DefaultVal<T>`, the library will accept input that omits that field and will populate it with the provided default (or with a default-constructed T when no explicit default is given).

## Declaration and initialization

You can declare a default-valued field like this:

```cpp
struct Person {
  std::string first_name;                             // required
  rfl::DefaultVal<std::string> last_name = "Simpson"; // has explicit default
  rfl::DefaultVal<std::string> town;                  // default-constructed (empty string)
};
```

DefaultVal behaves like a thin wrapper around the underlying type. You can construct and assign it from the underlying type, from other DefaultVal instances (if convertible), or assign the special token `rfl::Default` to reset it to the default-constructed value (if the type is default-constructible):

```cpp
Person p;
p.last_name = "Smith";            // assign underlying value
p.town = rfl::Default{};            // reset to default (empty string)
std::string s = p.last_name.value();
```

API convenience:

- .get(), .value(), operator()(), operator*(), operator->() — access the underlying value (const and non-const overloads).
- set(...) — assign underlying value.

## JSON behaviour

When writing JSON, fields that are DefaultVal are written like normal fields using their current underlying value. When reading JSON, omitted DefaultVal fields are filled with the default value (the value assigned in the declaration, or the type's default-constructed value).

Example (object fields):

```cpp
// Person from above
const auto homer = rfl::json::read<Person>(R"({"first_name":"Homer"})").value();
// homer.last_name == "Simpson" (declared default)
// homer.town == "" (default-constructed)
```

Example (no field names / positional arrays):

DefaultVal also works when using rfl::NoFieldNames (positional JSON arrays). Omitted positions that correspond to DefaultVal fields get their default values:

```cpp
const auto homer = rfl::json::read<Person, rfl::NoFieldNames>(R"(["Homer"])").value();
// homer.first_name == "Homer"
// homer.last_name == "Simpson"
// homer.town == ""
```

## Validators without a valid default value

A struct with default values can also contain an `rfl::Validator` whose default-constructed value is invalid, such as `rfl::Validator<int, rfl::Minimum<1>>`. Such a Validator is only checked if its field is missing from the input. Then `read` returns an error:

```cpp
using Foods = rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>>;
using Positive = rfl::Validator<int, rfl::Minimum<1>>;

struct Person {
  rfl::DefaultVal<std::string> name = "Homer Simpson";
  Foods favorite_foods;
  rfl::DefaultVal<Positive> age;
};

rfl::json::read<Person>(R"({"favorite_foods":["donuts"],"age":39})");  // OK
rfl::json::read<Person>(R"({"favorite_foods":["donuts"]})");
// Error: Field named 'age' not found, and its default value is invalid:
// Value expected to be greater than or equal to 1, but got 0.
```

The same applies to all fields when you pass `rfl::DefaultIfMissing`.

This only concerns a Validator that is default-constructed in place, as in `Foods favorite_foods;`, `rfl::DefaultVal<Positive> age{};` or `= rfl::Default{}`. An explicit default, such as `= 0`, or a temporary passed to a DefaultVal, such as `rfl::DefaultVal<Positive> age = Positive();`, is checked when it is constructed, so it must be valid, or every read fails. Outside of `read`, nothing changes: `Person{}` still throws.

The check also covers copies of such a default value that another default member initializer makes, as long as `read` can look into the type they are copied into: structs, standard containers, `std::optional`, `std::unique_ptr`, `rfl::Box`, variants, `rfl::Object` and wrappers like `rfl::DefaultVal` or `rfl::Rename`. It does not look into a class with a custom reflection type, a Reflector or its own `Parser`, so an invalid default copied into one of its members is not detected. Neither does it follow `std::shared_ptr` or `rfl::Ref`, because the object they point to may be shared with other code, so an invalid default copied into such an object is not detected either.

While `read` constructs such a struct, an invalid default value stays in its Validator until the struct is complete. So a default member initializer must not depend on the value of a Validator whose default is invalid, as in `std::string first = favorite_foods.value().front();`: it would see the invalid value. For the same reason, a function that catches the exception of a Validator's default constructor to fall back to another value, as in `Host host = default_host();`, does not get that exception during `read` if the struct also needs the check described here.

## When to use

Use `rfl::DefaultVal` when you want a field to be optional at the input side but still available as a value on the resulting object (no std::optional or pointer indirection). It is particularly useful for fields with sensible defaults (for example, a common last name, a default configuration value, or empty containers/strings).

## Notes

- The underlying type must be default-constructible to allow resetting via `rfl::Default` or when no explicit default is supplied.
- DefaultVal preserves normal read/write semantics; other fields that are not DefaultVal remain required unless expressed as optionals or handled by processors (e.g., `rfl::DefaultIfMissing`).

For more advanced control over when fields are considered missing and how defaults are applied, see the processors documentation (e.g., `rfl::DefaultIfMissing`).
