#ifndef RFL_PARSING_TAGGEDUNIONWRAPPERNOPTR_HPP_
#define RFL_PARSING_TAGGEDUNIONWRAPPERNOPTR_HPP_

#include <type_traits>

#include "../internal/StringLiteral.hpp"
#include "TaggedUnionWrapper.hpp"

namespace rfl {
namespace parsing {

template <class T>
struct tagged_union_wrapper_no_ptr;

template <class T>
struct tagged_union_wrapper_no_ptr {
  using Type = T;
};

template <class T, class TagType, internal::StringLiteral _discriminator,
          bool _as_const_pointer>
struct tagged_union_wrapper_no_ptr<
    TaggedUnionWrapperNoFields<T, TagType, _discriminator, _as_const_pointer>> {
  using Type = TaggedUnionWrapperNoFields<T, TagType, _discriminator, false>;
};

template <class T, class TagType, internal::StringLiteral _discriminator,
          bool _as_const_pointer>
struct tagged_union_wrapper_no_ptr<TaggedUnionWrapperWithFields<
    T, TagType, _discriminator, _as_const_pointer>> {
  using Type = TaggedUnionWrapperWithFields<T, TagType, _discriminator, false>;
};

template <class T>
using tagged_union_wrapper_no_ptr_t =
    typename tagged_union_wrapper_no_ptr<std::remove_cvref_t<T>>::Type;

template <class T>
struct unwrap_tagged_union_wrapper {
  using Type = std::remove_cvref_t<T>;
};

template <class T, class TagType, internal::StringLiteral _discriminator,
          bool _as_const_pointer>
struct unwrap_tagged_union_wrapper<
    TaggedUnionWrapperNoFields<T, TagType, _discriminator, _as_const_pointer>> {
  using Type = std::remove_cvref_t<T>;
};

template <class T, class TagType, internal::StringLiteral _discriminator,
          bool _as_const_pointer>
struct unwrap_tagged_union_wrapper<TaggedUnionWrapperWithFields<
    T, TagType, _discriminator, _as_const_pointer>> {
  using Type = std::remove_cvref_t<T>;
};

template <class T>
using unwrap_tagged_union_wrapper_t =
    typename unwrap_tagged_union_wrapper<std::remove_cvref_t<T>>::Type;

}  // namespace parsing
}  // namespace rfl

#endif
