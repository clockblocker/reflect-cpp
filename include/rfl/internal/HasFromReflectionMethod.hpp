#ifndef RFL_INTERNAL_HASFROMREFLECTIONMETHOD_HPP_
#define RFL_INTERNAL_HASFROMREFLECTIONMETHOD_HPP_

#include <concepts>
#include <utility>

#include "../Result.hpp"

namespace rfl::internal {

/// Satisfied by a type `T` providing `static U from_reflection(ReflectionType&&)`
/// where `U` is convertible to `T` or `Result<T>`.
template <class T>
concept HasFromReflectionMethod =
    requires(typename T::ReflectionType&& _r) {
      { T::from_reflection(std::move(_r)) } -> std::convertible_to<T>;
    } || requires(typename T::ReflectionType&& _r) {
      { T::from_reflection(std::move(_r)) } -> std::convertible_to<Result<T>>;
    };

}  // namespace rfl::internal

#endif
