#ifndef RFL_SIZE_HPP_
#define RFL_SIZE_HPP_

#include <map>
#include <utility>

#include "Ref.hpp"
#include "Result.hpp"
#include "internal/validator_code.hpp"
#include "parsing/schema/ValidationType.hpp"

namespace rfl {

/// Validates the size of a container or string using a validator V.
/// The validator V is applied to the size() method of the container.
template <class V>
struct Size {
  /// Validates that the size of the container satisfies the validator V.
  /// @tparam T The type of the container to validate
  /// @param _t The container whose size will be validated
  /// @return The container if validation passes, otherwise an error
  template <class T>
  static rfl::Result<T> validate(T _t) {
    auto res = V::validate(_t.size());
    if (!res) {
      auto inner_err = std::move(res).error();
      internal::set_error_code_if_empty(inner_err,
                                        internal::validator_code<V>());
      const auto msg = "Size validation failed: " + inner_err.what();
      const auto inner_builtin = internal::builtin_error_code_of(inner_err);
      if (inner_builtin != internal::BuiltinErrorCode::none) {
        const auto composed = internal::compose_size_code(inner_builtin);
        if (composed != internal::BuiltinErrorCode::none) {
          return error(Error(msg, ErrorKind::constraint_violation, composed));
        }
      }
      const auto inner_code = inner_err.code();
      if (inner_code.empty()) {
        return error(Error(msg, ErrorKind::constraint_violation,
                           internal::BuiltinErrorCode::size));
      }
      return error(Error(msg, ErrorKind::constraint_violation,
                         "size." + std::string(inner_code)));
    }
    return std::move(_t);
  }

  /// Converts the size validator to a JSON schema type.
  /// @tparam T The type of the container being validated
  /// @return A ValidationType representing size constraints
  template <class T>
  static parsing::schema::ValidationType to_schema() {
    using ValidationType = parsing::schema::ValidationType;
    return ValidationType{ValidationType::Size{
        .size_limit_ =
            rfl::Ref<ValidationType>::make(V::template to_schema<size_t>())}};
  }
};

}  // namespace rfl

#endif
