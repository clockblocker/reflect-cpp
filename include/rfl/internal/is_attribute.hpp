#ifndef RFL_INTERNAL_ISATTRIBUTE_HPP_
#define RFL_INTERNAL_ISATTRIBUTE_HPP_

#include <memory>
#include <optional>
#include <type_traits>

#include "../Alias.hpp"
#include "../Attribute.hpp"
#include "../Box.hpp"
#include "../DefaultVal.hpp"
#include "../Deprecated.hpp"
#include "../Description.hpp"
#include "../Positional.hpp"
#include "../Ref.hpp"
#include "../Rename.hpp"
#include "../Short.hpp"
#include "Skip.hpp"
#include "StringLiteral.hpp"
#include "TransparentWrapper.hpp"

namespace rfl::internal {

template <class T>
struct is_direct_attribute : std::false_type {};

template <class Type>
struct is_direct_attribute<Attribute<Type>> : std::true_type {};

template <class T>
constexpr bool is_direct_attribute_v =
    is_direct_attribute<std::remove_cvref_t<T>>::value;

template <class T>
class is_attribute;

template <class T>
class is_attribute : public std::false_type {};

template <class Type>
class is_attribute<Attribute<Type>> : public std::true_type {};

template <StringLiteral _new_name, class Type>
class is_attribute<Rename<_new_name, Type>>
    : public is_attribute<std::remove_cvref_t<Type>> {};

template <class Type>
class is_attribute<std::optional<Type>>
    : public is_attribute<std::remove_cvref_t<Type>> {};

template <class Type>
class is_attribute<std::unique_ptr<Type>>
    : public is_attribute<std::remove_cvref_t<Type>> {};

template <class Type>
class is_attribute<std::shared_ptr<Type>>
    : public is_attribute<std::remove_cvref_t<Type>> {};

template <class Type, Copyability _copyability>
class is_attribute<Box<Type, _copyability>>
    : public is_attribute<std::remove_cvref_t<Type>> {};

template <class Type>
class is_attribute<Ref<Type>>
    : public is_attribute<std::remove_cvref_t<Type>> {};

template <class Type>
class is_attribute<DefaultVal<Type>>
    : public is_attribute<std::remove_cvref_t<Type>> {};

template <StringLiteral _description, class Type>
class is_attribute<Description<_description, Type>>
    : public is_attribute<std::remove_cvref_t<Type>> {};

template <StringLiteral _deprecation_message, StringLiteral _description,
          class Type>
class is_attribute<Deprecated<_deprecation_message, _description, Type>>
    : public is_attribute<std::remove_cvref_t<Type>> {};

template <StringLiteral _short_name, class Type>
class is_attribute<Short<_short_name, Type>>
    : public is_attribute<std::remove_cvref_t<Type>> {};

template <class Type>
class is_attribute<Positional<Type>>
    : public is_attribute<std::remove_cvref_t<Type>> {};

template <class Type, bool _skip_serialization, bool _skip_deserialization>
class is_attribute<Skip<Type, _skip_serialization, _skip_deserialization>>
    : public is_attribute<std::remove_cvref_t<Type>> {};

template <StringLiteral _alias, class Type>
class is_attribute<Alias<_alias, Type>>
    : public is_attribute<std::remove_cvref_t<Type>> {};

template <class T>
  requires(TransparentWrapperType<T> && !is_direct_attribute_v<T>)
class is_attribute<T>
    : public is_attribute<std::remove_cvref_t<typename T::Type>> {};

template <class T>
constexpr bool is_attribute_v =
    is_attribute<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace rfl::internal

#endif
