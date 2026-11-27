---
breadcrumbs: true
---

# nlohmann/json

For `nlohmann::json` interoperability, include `<rfl/nlohmann.hpp>` and enable `-DREFLECTCPP_NLOHMANN=ON` when building reflect-cpp (or link `nlohmann_json::nlohmann_json` directly — the adapter is header-only and requires `nlohmann/json >= 3.11.3`). It has no dependency on `yyjson` and works with `-DREFLECTCPP_JSON=OFF`.

## Reading and writing (`nlohmann::json` and `nlohmann::ordered_json`)

Convert reflected structs directly to and from `nlohmann::basic_json` without string round-trips:

```cpp
#include <rfl/nlohmann.hpp>

struct Person {
    std::string first_name;
    rfl::Validator<int, rfl::Minimum<0>> age;
    std::optional<std::string> nickname;
};

const Person src{.first_name = "Ada", .age = 36, .nickname = "countess"};

// Serialize to nlohmann::json (supports reflect-cpp processors)
const nlohmann::json j = rfl::nlohmann::write<rfl::SnakeCaseToCamelCase>(src);

// Deserialize without throwing (errors are returned via rfl::Result)
const rfl::Result<Person> res =
    rfl::nlohmann::read<Person, rfl::SnakeCaseToCamelCase, rfl::NoExtraFields>(j);

// Preserve struct field order with nlohmann::ordered_json
const nlohmann::ordered_json oj = rfl::nlohmann::write<nlohmann::ordered_json>(src);
```

## Opt-in `nlohmann::adl_serializer`

When C++ exceptions are enabled, `<rfl/nlohmann.hpp>` includes `<rfl/nlohmann/adl_serializer.hpp>`. Specialize `rfl::nlohmann::enable_adl_serializer<T>` to `std::true_type` to opt specific types into `nlohmann::json` conversions without affecting any other types:

```cpp
struct User {
    const std::string name;
    rfl::Validator<int, rfl::Minimum<0>> age;
};

template <>
struct rfl::nlohmann::enable_adl_serializer<User> : std::true_type {};

nlohmann::json j = User{.name = "Grace", .age = 42};
User u = j.get<User>();  // throws nlohmann::json::other_error (a nlohmann::json::exception) on error
```

Because `adl_serializer` uses the returning 1-argument `from_json` overload, `j.get<T>()` works cleanly with `rfl::Validator`, `const` fields, and non-default-constructible structs.

## Dynamic `nlohmann::json` fields in reflected structs

`nlohmann::json` and `nlohmann::ordered_json` (and other `nlohmann::basic_json` specializations) can be embedded directly as fields inside reflected structs and serialized across `rfl::nlohmann`, `rfl::json`, `rfl::generic`, and other formats:

```cpp
struct Event {
    std::string kind;
    nlohmann::json payload;
    nlohmann::ordered_json metadata;
};
```

- `rfl::Bytestring` and `rfl::Vectorstring` serialize as RFC 4648 Base64 strings and accept either Base64 strings or native `nlohmann::json::binary_t` values on read.
- Cross-`basic_json` conversions (`nlohmann::json` ↔ `nlohmann::ordered_json`) preserve exact integer signedness (`uint64_t`/`int64_t`), booleans, strings, binary payloads, and key order.
