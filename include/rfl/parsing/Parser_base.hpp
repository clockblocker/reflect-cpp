#ifndef RFL_PARSING_PARSER_BASE_HPP_
#define RFL_PARSING_PARSER_BASE_HPP_

#include <type_traits>
#include <utility>

#include "AreReaderAndWriter.hpp"

namespace rfl::parsing {

/**
 * @brief The main parser struct, which must be specialized for each type.
 *
 * @tparam R The reader type.
 * @tparam W The writer type.
 * @tparam T The type to be parsed.
 * @tparam ProcessorsType The processors to use.
 */
template <class R, class W, class T, class ProcessorsType>
  requires AreReaderAndWriter<R, W, T>
struct Parser;

/**
 * @brief Constructs a Result without calling the noexcept rvalue constructor
 *        when moving V may throw (e.g., structs containing const members with
 *        a throwing copy constructor), so callers inside try/catch can convert
 *        exceptions into rfl::Error instead of calling std::terminate().
 */
template <class ResultType, class U>
ResultType make_result_safe(U&& _u) {
  using V = std::remove_cvref_t<U>;
  if constexpr (!std::is_nothrow_move_constructible_v<V> &&
                std::is_copy_constructible_v<V>) {
    return ResultType(static_cast<const V&>(_u));
  } else {
    return ResultType(std::forward<U>(_u));
  }
}

/**
 * @brief Checks whether replacing an existing live object of type U requires
 *        in-place destroy + placement-move rather than assignment. Unwraps
 *        wrapper types that expose `::Type` before testing
 *        `std::is_move_assignable_v<U>`, avoiding hard errors in unconstrained
 *        template `operator=` overloads on wrappers like Rename / Description.
 */
template <class U>
consteval bool needs_in_place_reconstruct() {
  if constexpr (std::is_const_v<U>) {
    return true;
  } else if constexpr (requires { typename std::remove_cvref_t<U>::Type; }) {
    using Inner = typename std::remove_cvref_t<U>::Type;
    if constexpr (!std::is_same_v<std::remove_cvref_t<Inner>,
                                  std::remove_cvref_t<U>>) {
      if constexpr (needs_in_place_reconstruct<Inner>()) {
        return true;
      } else {
        return !std::is_move_assignable_v<U>;
      }
    } else {
      return !std::is_move_assignable_v<U>;
    }
  } else {
    return !std::is_move_assignable_v<U>;
  }
}

/**
 * @brief Overlays input into an existing value when the parser supports
 *        `read_into`, otherwise parses a fresh replacement via `read`.
 */
template <class R, class W, class T, class ProcessorsType, class Base>
auto read_into_or_read(const R& _r, const typename R::InputVarType& _var,
                       Base&& _base) {
  using U = std::remove_cvref_t<T>;
  if constexpr (requires {
                  Parser<R, W, U, ProcessorsType>::read_into(
                      _r, _var, U(std::forward<Base>(_base)));
                }) {
    return Parser<R, W, U, ProcessorsType>::read_into(
        _r, _var, U(std::forward<Base>(_base)));
  } else {
    return Parser<R, W, U, ProcessorsType>::read(_r, _var);
  }
}

}  // namespace rfl::parsing

#endif
