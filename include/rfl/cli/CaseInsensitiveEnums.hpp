#ifndef RFL_CLI_CASEINSENSITIVEENUMS_HPP_
#define RFL_CLI_CASEINSENSITIVEENUMS_HPP_

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

#include "../CaseInsensitiveEnums.hpp"
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
template <class EnumType, bool _enum_names_only = false,
          class ProcessorsType = rfl::Processors<>>
rfl::Result<EnumType> parse_enum_icase(const std::string_view _str) {
  return rfl::parse_enum_icase<EnumType, _enum_names_only, ProcessorsType>(
      _str);
}

}  // namespace rfl::cli

#endif
