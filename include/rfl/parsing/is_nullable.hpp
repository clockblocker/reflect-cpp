#ifndef RFL_PARSING_IS_NULLABLE_HPP_
#define RFL_PARSING_IS_NULLABLE_HPP_

#include <memory>
#include <optional>
#include <type_traits>

#include "../Box.hpp"
#include "../Commented.hpp"
#include "../DefaultVal.hpp"
#include "../Ref.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/has_reflector.hpp"
#include "../internal/is_positional.hpp"
#include "../internal/is_rename.hpp"
#include "../internal/is_short.hpp"
#include "../internal/is_skip.hpp"

namespace rfl::parsing {

namespace internal_nullable {

template <class T>
struct is_optional : std::false_type {};

template <class T>
struct is_optional<std::optional<T>> : std::true_type {};

template <class T>
struct is_unique_ptr : std::false_type {};

template <class T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};

template <class T>
struct is_shared_ptr : std::false_type {};

template <class T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

template <class T>
struct is_box : std::false_type {};

template <class T, Copyability C>
struct is_box<Box<T, C>> : std::true_type {
  using element_type = T;
};

template <class T>
struct is_ref : std::false_type {};

template <class T>
struct is_ref<Ref<T>> : std::true_type {
  using element_type = T;
};

template <class T>
struct is_commented : std::false_type {};

template <class T>
struct is_commented<Commented<T>> : std::true_type {
  using element_type = T;
};

template <class T>
struct is_default_val : std::false_type {};

template <class T>
struct is_default_val<DefaultVal<T>> : std::true_type {
  using element_type = T;
};

}  // namespace internal_nullable

/// Checks whether a type natively represents null as a valid value
/// (such as std::optional, std::unique_ptr, std::shared_ptr, raw pointers,
/// rfl::Generic, or transparent wrappers around them).
template <class T>
consteval bool is_nullable() {
  using U = std::remove_cvref_t<T>;
  if constexpr (internal_nullable::is_optional<U>::value ||
                internal_nullable::is_unique_ptr<U>::value ||
                internal_nullable::is_shared_ptr<U>::value ||
                std::is_pointer_v<U>) {
    return true;
  } else if constexpr (internal::is_rename_v<U> || internal::is_short_v<U> ||
                       internal::is_positional_v<U>) {
    return is_nullable<typename U::Type>();
  } else if constexpr (internal_nullable::is_default_val<U>::value) {
    return is_nullable<
        typename internal_nullable::is_default_val<U>::element_type>();
  } else if constexpr (internal_nullable::is_commented<U>::value) {
    return is_nullable<
        typename internal_nullable::is_commented<U>::element_type>();
  } else if constexpr (internal_nullable::is_box<U>::value) {
    return is_nullable<typename internal_nullable::is_box<U>::element_type>();
  } else if constexpr (internal_nullable::is_ref<U>::value) {
    return is_nullable<typename internal_nullable::is_ref<U>::element_type>();
  } else if constexpr (internal::is_skip_v<U>) {
    if constexpr (U::skip_deserialization_) {
      return false;
    } else {
      return is_nullable<typename U::Type>();
    }
  } else if constexpr (internal::has_reflection_type_v<U>) {
    return is_nullable<typename U::ReflectionType>();
  } else if constexpr (internal::has_read_reflector<U>) {
    return is_nullable<typename Reflector<U>::ReflType>();
  } else {
    return false;
  }
}

template <class T>
constexpr bool is_nullable_v = is_nullable<T>();

}  // namespace rfl::parsing

#endif
