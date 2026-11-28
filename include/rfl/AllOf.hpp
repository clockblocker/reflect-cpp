#ifndef RFL_ALLOF_HPP_
#define RFL_ALLOF_HPP_

#include <utility>
#include <vector>

#include "Result.hpp"
#include "internal/validator_code.hpp"
#include "parsing/schema/ValidationType.hpp"

namespace rfl {

/// Requires that all of the constraints C and Cs be true.
template <class C, class... Cs>
struct AllOf {
  /// Validates that all constraints are satisfied.
  /// @param _value The value to validate
  /// @return The value if all constraints pass, otherwise an error
  template <class T>
  static rfl::Result<T> validate(T _value) noexcept {
    return validate_impl<T, C, Cs...>(std::move(_value));
  }

  /// Converts the validator to a JSON schema type.
  /// @return A ValidationType representing all-of schema
  template <class T>
  static parsing::schema::ValidationType to_schema() {
    using ValidationType = parsing::schema::ValidationType;
    const auto types = std::vector<ValidationType>(
        {C::template to_schema<T>(), Cs::template to_schema<T>()...});
    return ValidationType{ValidationType::AllOf{.types_ = types}};
  }

 private:
  template <class T, class Head, class... Tail>
  static rfl::Result<T> validate_impl(T _value) noexcept {
    auto res = Head::validate(std::move(_value));
    if (!res) {
      auto err = std::move(res).error();
      internal::set_error_kind_if_other(err, ErrorKind::constraint_violation);
      internal::set_error_code_if_empty(err, internal::validator_code<Head>());
      return error(std::move(err));
    }
    if constexpr (sizeof...(Tail) == 0) {
      return res;
    } else {
      return validate_impl<T, Tail...>(std::move(*res));
    }
  }
};

}  // namespace rfl

#endif
