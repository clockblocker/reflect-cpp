#ifndef RFL_CLI_CASEINSENSITIVEENUMS_HPP_
#define RFL_CLI_CASEINSENSITIVEENUMS_HPP_

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

#include "../Result.hpp"
#include "../enums.hpp"

namespace rfl::cli {

/// Processor enabling case-insensitive enum parsing in `rfl::cli::read` and
/// `rfl::cli::read_app` (e.g. `--log-level=DEBUG` matches `LogLevel::debug`).
struct CaseInsensitiveEnums {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

/// Case-insensitive ASCII enum lookup helper (usable both internally by
/// `CaseInsensitiveEnums` and directly with `DynamicOptions`).
template <class EnumType, bool _enum_names_only = false>
rfl::Result<EnumType> parse_enum_icase(const std::string_view _str) noexcept {
  const auto eq_icase = [](std::string_view _a, std::string_view _b) noexcept {
    if (_a.size() != _b.size()) {
      return false;
    }
    for (size_t i = 0; i < _a.size(); ++i) {
      if (std::tolower(static_cast<unsigned char>(_a[i]))
          != std::tolower(static_cast<unsigned char>(_b[i]))) {
        return false;
      }
    }
    return true;
  };
  constexpr auto enumerators = rfl::get_enumerator_array<EnumType>();
  for (const auto& [name, val] : enumerators) {
    if (eq_icase(name, _str)) {
      return val;
    }
  }
  return rfl::string_to_enum<EnumType, _enum_names_only>(std::string(_str));
}

}  // namespace rfl::cli

#endif
