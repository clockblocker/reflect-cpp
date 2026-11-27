#ifndef RFL_CODEPOINTS_HPP_
#define RFL_CODEPOINTS_HPP_

#include <cstddef>
#include <string_view>
#include <utility>

#include "Result.hpp"
#include "Size.hpp"
#include "internal/validator_code.hpp"
#include "parsing/schema/ValidationType.hpp"

namespace rfl {

/// Validates the length of a UTF-8 encoded string, measured in Unicode code
/// points, using a validator V.
///
/// Unlike rfl::Size, which applies V to the number of bytes returned by
/// size(), this counts code points. This matches the semantics of the
/// minLength/maxLength keywords in JSON Schema, which is what the generated
/// schema emits.
///
/// Note that this counts code points, not grapheme clusters (user-perceived
/// characters), so e.g. an emoji with a skin-tone modifier counts as two.
/// It also does not check that the input is well-formed UTF-8: it simply
/// counts every byte that is not a UTF-8 continuation byte (10xxxxxx).
template <class V>
struct CodePoints {
  /// Validates that the number of code points in the string satisfies the
  /// validator V.
  /// @tparam T The type of the string to validate
  /// @param _t The string whose code point count will be validated
  /// @return The string if validation passes, otherwise an error
  template <class T>
  static rfl::Result<T> validate(T _t) {
    const auto view = std::string_view(_t);
    std::size_t n = 0;
    for (const char c : view) {
      if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) {
        ++n;
      }
    }
    auto res = V::validate(n);
    if (!res) {
      auto inner_err = std::move(res).error();
      internal::set_error_code_if_empty(inner_err,
                                        internal::validator_code<V>());
      const auto msg =
          "Code point count validation failed: " + inner_err.what();
      const auto inner_builtin = internal::builtin_error_code_of(inner_err);
      if (inner_builtin != internal::BuiltinErrorCode::none) {
        const auto composed = internal::compose_code_points_code(inner_builtin);
        if (composed != internal::BuiltinErrorCode::none) {
          return error(Error(msg, ErrorKind::constraint_violation, composed));
        }
      }
      const auto inner_code = inner_err.code();
      if (inner_code.empty()) {
        return error(Error(msg, ErrorKind::constraint_violation,
                           internal::BuiltinErrorCode::code_points));
      }
      return error(Error(msg, ErrorKind::constraint_violation,
                         "code_points." + std::string(inner_code)));
    }
    return std::move(_t);
  }

  /// Converts the code point validator to a JSON schema type. The output is
  /// identical to that of rfl::Size, i.e. minLength/maxLength for strings.
  /// @tparam T The type of the string being validated
  /// @return A ValidationType representing length constraints
  template <class T>
  static parsing::schema::ValidationType to_schema() {
    return Size<V>::template to_schema<T>();
  }
};

}  // namespace rfl

#endif
