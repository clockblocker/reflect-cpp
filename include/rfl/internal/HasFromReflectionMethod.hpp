#ifndef RFL_INTERNAL_HASFROMREFLECTIONMETHOD_HPP_
#define RFL_INTERNAL_HASFROMREFLECTIONMETHOD_HPP_

#include <concepts>
#include <utility>

#include "../Result.hpp"

namespace rfl::internal {

/// Satisfied by a type `T` providing `static T from_reflection(ReflectionType&&)`
/// or `static Result<T> from_reflection(ReflectionType&&)`.
template <class T>
concept HasFromReflectionMethod = requires(typename T::ReflectionType&& _r) {
  { T::from_reflection(std::move(_r)) } -> std::convertible_to<Result<T>>;
};

}  // namespace rfl::internal

#endif
