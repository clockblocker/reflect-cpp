#ifndef RFL_INTERNAL_ISSKIP_HPP_
#define RFL_INTERNAL_ISSKIP_HPP_

#include <type_traits>

#include "Skip.hpp"
#include "StringLiteral.hpp"

namespace rfl {

template <internal::StringLiteral _name, class T>
struct Alias;
template <class T>
struct Attribute;
template <class T>
struct Commented;
template <class T>
struct DefaultVal;
template <internal::StringLiteral _deprecation_message,
          internal::StringLiteral _description, class T>
struct Deprecated;
template <internal::StringLiteral _description, class T>
struct Description;
template <internal::StringLiteral _name, class T>
struct Field;
template <class T>
struct Positional;
template <internal::StringLiteral _name, class T>
struct Rename;
template <internal::StringLiteral _name, class T>
struct Short;

namespace internal {

template <class T>
class is_skip;

template <class T>
class is_skip : public std::false_type {};

template <class T, bool _skip_serialization, bool _skip_deserialization>
class is_skip<Skip<T, _skip_serialization, _skip_deserialization>>
    : public std::true_type {};

template <StringLiteral _name, class T>
class is_skip<Alias<_name, T>> : public is_skip<std::remove_cvref_t<T>> {};

template <class T>
constexpr bool is_skip_v =
    is_skip<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

template <class T>
struct skip_traits {
  static constexpr bool is_skip = false;
  static constexpr bool skip_serialization = false;
  static constexpr bool skip_deserialization = false;
};

template <class T>
struct skip_traits<const T> : skip_traits<T> {};

template <class T>
struct skip_traits<volatile T> : skip_traits<T> {};

template <class T>
struct skip_traits<const volatile T> : skip_traits<T> {};

template <class T>
struct skip_traits<T&> : skip_traits<T> {};

template <class T>
struct skip_traits<T&&> : skip_traits<T> {};

template <class T>
struct skip_traits<T*> : skip_traits<T> {};

template <class T, bool _skip_serialization, bool _skip_deserialization>
struct skip_traits<Skip<T, _skip_serialization, _skip_deserialization>> {
  static constexpr bool is_skip = true;
  static constexpr bool skip_serialization =
      _skip_serialization || skip_traits<T>::skip_serialization;
  static constexpr bool skip_deserialization =
      _skip_deserialization || skip_traits<T>::skip_deserialization;
};

template <StringLiteral _name, class T>
struct skip_traits<Alias<_name, T>> : skip_traits<T> {};

template <StringLiteral _name, class T>
struct skip_traits<Rename<_name, T>> : skip_traits<T> {};

template <StringLiteral _name, class T>
struct skip_traits<Field<_name, T>> : skip_traits<T> {};

template <StringLiteral _description, class T>
struct skip_traits<Description<_description, T>> : skip_traits<T> {};

template <StringLiteral _deprecation_message, StringLiteral _description,
          class T>
struct skip_traits<Deprecated<_deprecation_message, _description, T>>
    : skip_traits<T> {};

template <class T>
struct skip_traits<DefaultVal<T>> : skip_traits<T> {};

template <StringLiteral _name, class T>
struct skip_traits<Short<_name, T>> : skip_traits<T> {};

template <class T>
struct skip_traits<Positional<T>> : skip_traits<T> {};

template <class T>
struct skip_traits<Commented<T>> : skip_traits<T> {};

template <class T>
struct skip_traits<Attribute<T>> : skip_traits<T> {};

template <class T>
constexpr bool skip_serialization_v = skip_traits<T>::skip_serialization;

template <class T>
constexpr bool skip_deserialization_v = skip_traits<T>::skip_deserialization;

template <class T>
constexpr bool is_fully_skipped_v =
    skip_serialization_v<T> && skip_deserialization_v<T>;
}  // namespace internal
}  // namespace rfl

#endif
