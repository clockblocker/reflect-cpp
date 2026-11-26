#ifndef RFL_UNWRAP_HPP_
#define RFL_UNWRAP_HPP_

#include <type_traits>
#include <utility>

#include "internal/is_default_val_v.hpp"
#include "internal/is_deprecated.hpp"
#include "internal/is_description.hpp"
#include "internal/is_group.hpp"
#include "internal/is_hidden.hpp"
#include "internal/is_implicit_value.hpp"
#include "internal/is_multi_token.hpp"
#include "internal/is_positional.hpp"
#include "internal/is_rename.hpp"
#include "internal/is_short.hpp"
#include "internal/is_value_name.hpp"

namespace rfl {

namespace internal {

template <class T>
constexpr bool is_metadata_wrapper_v =
    is_default_val_v<T> || is_description_v<T> || is_deprecated_v<T> ||
    is_rename_v<T> || is_short_v<T> || is_positional_v<T> ||
    is_implicit_value_v<T> || is_hidden_v<T> || is_group_v<T> ||
    is_value_name_v<T> || is_multi_token_v<T>;

}  // namespace internal

/// Recursively unwraps reflect-cpp metadata wrappers (DefaultVal, Description,
/// Deprecated, Rename, Short, Positional, ImplicitValue, Hidden, Group) and
/// returns a reference to the underlying value. Does not unwrap std::optional,
/// pointers, or Validator.
template <class T>
constexpr decltype(auto) unwrap(T&& _val) noexcept {
  using U = std::remove_cvref_t<T>;
  if constexpr (internal::is_metadata_wrapper_v<U>) {
    if constexpr (std::is_lvalue_reference_v<T>) {
      return unwrap(_val.value());
    } else {
      return unwrap(std::move(_val.value()));
    }
  } else {
    return std::forward<T>(_val);
  }
}

}  // namespace rfl

#endif
