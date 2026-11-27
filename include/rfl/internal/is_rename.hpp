#ifndef RFL_INTERNAL_ISRENAME_HPP_
#define RFL_INTERNAL_ISRENAME_HPP_

#include <type_traits>

#include "../Rename.hpp"
#include "StringLiteral.hpp"

namespace rfl {

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
template <class T>
struct Positional;
template <internal::StringLiteral _name, class T>
struct Short;

namespace internal {

template <class T, bool _skip_serialization, bool _skip_deserialization>
class Skip;

template <class T>
class is_rename;

template <class T>
class is_rename : public std::false_type {};

template <StringLiteral _name, class Type>
class is_rename<Rename<_name, Type>> : public std::true_type {};

template <class T>
constexpr bool is_rename_v =
    is_rename<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

template <class T>
struct rename_traits {
  static constexpr bool has_rename = false;
};

template <class T>
struct rename_traits<const T> : rename_traits<T> {};

template <class T>
struct rename_traits<volatile T> : rename_traits<T> {};

template <class T>
struct rename_traits<const volatile T> : rename_traits<T> {};

template <class T>
struct rename_traits<T&> : rename_traits<T> {};

template <class T>
struct rename_traits<T&&> : rename_traits<T> {};

template <class T>
struct rename_traits<T*> : rename_traits<T> {};

template <StringLiteral _name, class T>
struct rename_traits<Rename<_name, T>> {
  static constexpr bool has_rename = true;
  using Name = typename Rename<_name, T>::Name;
};

template <class T, bool _skip_serialization, bool _skip_deserialization>
struct rename_traits<Skip<T, _skip_serialization, _skip_deserialization>>
    : rename_traits<T> {};

template <StringLiteral _description, class T>
struct rename_traits<Description<_description, T>> : rename_traits<T> {};

template <StringLiteral _deprecation_message, StringLiteral _description,
          class T>
struct rename_traits<Deprecated<_deprecation_message, _description, T>>
    : rename_traits<T> {};

template <class T>
struct rename_traits<DefaultVal<T>> : rename_traits<T> {};

template <StringLiteral _name, class T>
struct rename_traits<Short<_name, T>> : rename_traits<T> {};

template <class T>
struct rename_traits<Positional<T>> : rename_traits<T> {};

template <class T>
struct rename_traits<Commented<T>> : rename_traits<T> {};

template <class T>
struct rename_traits<Attribute<T>> : rename_traits<T> {};

template <class T>
constexpr bool has_rename_v = rename_traits<T>::has_rename;

}  // namespace internal
}  // namespace rfl

#endif
