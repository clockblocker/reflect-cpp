# Read-only field aliases (`rfl::Alias`)

The `rfl::Alias<"old-name", T>` wrapper declares a read-only alias for a struct field. During deserialization, `reflect-cpp` accepts **either** the canonical primary wire name **or** any declared alias. During serialization and JSON Schema generation, `reflect-cpp` **always** emits the canonical primary wire name.

This is especially useful when evolving an API or configuration format (for example, migrating legacy `kebab-case` or `camelCase` RPC keys to `snake_case`, or supporting deprecated CLI flag names) without maintaining manual key-translation tables.

## Basic usage

```cpp
#include <rfl.hpp>
#include <rfl/json.hpp>

struct Settings {
  rfl::Alias<"download-dir", std::string> download_dir;
  rfl::Alias<"speedLimitDown", int> speed_limit_down;
};

// Accepted on read (alias or canonical name):
const auto s = rfl::json::read<Settings>(
    R"({"download-dir":"/downloads","speedLimitDown":250})").value();

// Always serialized using the canonical primary names:
const auto out = rfl::json::write(s);
// {"download_dir":"/downloads","speed_limit_down":250}
```

## Multiple aliases

Nest `rfl::Alias` wrappers to accept multiple legacy names for the same field:

```cpp
struct Settings {
  rfl::Alias<"download-dir", rfl::Alias<"downloadDir", std::string>>
      download_dir;
};
```

Reading `{"download_dir": "/dl"}`, `{"download-dir": "/dl"}`, or `{"downloadDir": "/dl"}` all populate `download_dir`, and writing always emits `{"download_dir":"/dl"}`.

## Semantics

- **Primary wire name:** Unchanged. It comes from the C++ field name (transformed by naming processors such as `rfl::SnakeCaseToCamelCase` or `rfl::SnakeCaseToKebabCase`, or overridden by `rfl::Rename<"canonical", ...>`).
- **Serialization (write):** Always writes the canonical primary wire name.
- **JSON Schema (`to_schema`):** Emits only the canonical primary property name (preserving inner `rfl::Description`, `rfl::Deprecated`, `rfl::DefaultVal`, and `rfl::Validator` metadata).
- **`rfl::NoExtraFields`:** Aliases are recognized as valid field names and are not rejected as unknown extra fields.
- **Conflicting / duplicate input:** If an input object specifies both the primary name and an alias (or two different aliases for the same field), deserialization fails with an error instead of silently overwriting.
- **Compile-time collision check:** If an alias on one field equals the primary wire name or an alias of a different field, compilation fails with a `static_assert`.

## Composing with other wrappers and processors

`rfl::Alias` is transparent and composes cleanly with `rfl::Rename`, `rfl::Description`, `rfl::Deprecated`, `rfl::DefaultVal`, `rfl::DefaultIfMissing`, `std::optional`, `rfl::Validator`, `rfl::Short`, `rfl::Positional`, and case-transforming processors:

```cpp
struct Config {
  rfl::Rename<"download_dir", rfl::Alias<"download-dir", std::string>> dir;
  rfl::Alias<"old-port", rfl::DefaultVal<int>> port = 8080;
  rfl::Description<"Timeout in seconds", rfl::Alias<"timeoutSec", std::optional<int>>>
      timeout_sec;
};
```

## CLI support

`rfl::cli::read` also recognizes `rfl::Alias` names as long options (including under `rfl::NoExtraFields`) and detects conflicts between aliases, canonical names, `rfl::Short`, and `rfl::Positional`:

```cpp
struct CliConfig {
  rfl::Alias<"download_dir", rfl::Alias<"downloadDir", std::string>>
      download_dir;
  rfl::Alias<"debug", rfl::DefaultVal<rfl::Short<"v", bool>>> verbose = false;
};

// Accepts --download-dir=/dl, --download_dir=/dl, or --downloadDir=/dl
// Accepts --verbose, -v, or --debug
```

## Wrapper ergonomics

`rfl::Alias<Name, T>` provides `.value()`, `.get()`, `operator*`, `operator->`, `operator()`, implicit construction from convertible values, implicit conversion to `T&` / `const T&`, `==`, `<=>`, and `std::hash` specialization. Its default constructor is constrained on `std::is_default_constructible_v<T>`.
