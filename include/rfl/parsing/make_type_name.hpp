#ifndef RFL_PARSING_MAKETYPENAME_HPP_
#define RFL_PARSING_MAKETYPENAME_HPP_

#include <cctype>

#include "../SchemaExt.hpp"
#include "../type_name_t.hpp"
#include "is_tagged_union_wrapper.hpp"

namespace rfl::parsing {

/**
 * @brief Replaces all non-alphanumeric characters with underscores.
 *
 * @param _str The string to process.
 * @return The processed string.
 */
inline std::string replace_non_alphanumeric(std::string _str) {
  for (auto& ch : _str) {
    ch = std::isalnum(static_cast<unsigned char>(ch)) ? ch : '_';
  }
  return _str;
}

/**
 * @brief Generates a type name for the type U.
 *
 * Respects `rfl::SchemaName<U>` when specialized.
 *
 * @tparam U The type to generate the name for.
 * @return The type name.
 */
template <class U>
static std::string make_type_name() {
  if constexpr (is_tagged_union_wrapper_v<U>) {
    using Inner = typename U::Type;
    if constexpr (internal::has_schema_name_v<Inner>) {
      auto custom = internal::get_schema_name<Inner>();
      if (!custom.empty()) {
        return custom + "__tagged";
      }
    }
    return replace_non_alphanumeric(type_name_t<Inner>().str() + "__tagged");
  } else {
    if constexpr (internal::has_schema_name_v<U>) {
      auto custom = internal::get_schema_name<U>();
      if (!custom.empty()) {
        return custom;
      }
    }
    return replace_non_alphanumeric(type_name_t<U>().str());
  }
}

}  // namespace rfl::parsing

#endif
