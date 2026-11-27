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
Result<EnumType> parse_enum_icase(const std::string_view _str) {
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
  const auto parse_one = [&](const std::string_view _tok) -> Result<EnumType> {
    if constexpr (internal::has_custom_enum_names_v<EnumType, ProcessorsType>) {
      for (const auto& [wire, val] :
           get_enum_wire_names<EnumType, ProcessorsType>()) {
        if (eq_icase(wire, _tok)) {
          return val;
        }
      }
    } else {
      constexpr auto enumerators = get_enumerator_array<EnumType>();
      for (const auto& [name, val] : enumerators) {
        if (eq_icase(name, _tok)) {
          return val;
        }
      }
    }
    return string_to_enum<EnumType, _enum_names_only, ProcessorsType>(_tok);
  };
  if constexpr (internal::enums::is_flag_enum<EnumType>) {
    using T = std::underlying_type_t<EnumType>;
    auto res = static_cast<T>(0);
    std::string_view remaining = _str;
    while (true) {
      const auto pos = remaining.find('|');
      const auto token = remaining.substr(0, pos);
      const auto r = parse_one(token);
      if (!r) {
        return r;
      }
      res |= static_cast<T>(*r);
      if (pos == std::string_view::npos) {
        break;
      }
      remaining.remove_prefix(pos + 1);
    }
    return static_cast<EnumType>(res);
  } else {
    return parse_one(_str);
  }
}

}  // namespace rfl

#endif
