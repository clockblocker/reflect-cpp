#ifndef RFL_CASEINSENSITIVEENUMS_HPP_
#define RFL_CASEINSENSITIVEENUMS_HPP_

#include <cctype>
#include <string>
#include <string_view>

#include "Result.hpp"
#include "enums.hpp"

namespace rfl {

/// A processor that enables case-insensitive ASCII matching of declared enum
/// enumerator names when reading enums across any format (JSON, YAML, TOML,
/// CLI, etc.).
/// Usage: rfl::json::read<MyStruct, rfl::CaseInsensitiveEnums>(json_str)
struct CaseInsensitiveEnums {
 public:
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

/// Case-insensitive ASCII lookup for declared enumerators of `EnumType`.
/// Falls back to `rfl::string_to_enum<EnumType, _enum_names_only>` on no match
/// so numeric strings and error wording match standard enum parsing.
template <class EnumType, bool _enum_names_only = false,
          class ProcessorsType = Processors<>>
Result<EnumType> parse_enum_icase(const std::string_view _str) noexcept {
  const auto eq_icase = [](std::string_view _a, std::string_view _b) noexcept {
    if (_a.size() != _b.size()) {
      return false;
    }
    for (size_t i = 0; i < _a.size(); ++i) {
      if (std::tolower(static_cast<unsigned char>(_a[i])) !=
          std::tolower(static_cast<unsigned char>(_b[i]))) {
        return false;
      }
    }
    return true;
  };
  if constexpr (internal::has_custom_enum_names_v<EnumType, ProcessorsType>) {
    for (const auto& [wire, val] :
         get_enum_wire_names<EnumType, ProcessorsType>()) {
      if (eq_icase(wire, _str)) {
        return val;
      }
    }
    for (const auto& [val, wire] : internal::get_enum_overrides<EnumType>()) {
      if (eq_icase(wire, _str)) {
        return val;
      }
    }
  } else {
    constexpr auto enumerators = get_enumerator_array<EnumType>();
    for (const auto& [name, val] : enumerators) {
      if (eq_icase(name, _str)) {
        return val;
      }
    }
  }
  return string_to_enum<EnumType, _enum_names_only, ProcessorsType>(
      std::string(_str));
}

}  // namespace rfl

#endif
