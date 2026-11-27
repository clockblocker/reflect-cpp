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
 * @brief Overlays input into an existing value when the parser supports
 *        `read_into`, otherwise parses a fresh replacement via `read`.
 */
template <class R, class W, class T, class ProcessorsType>
auto read_into_or_read(const R& _r, const typename R::InputVarType& _var,
                       T&& _base) {
  using U = std::remove_cvref_t<T>;
  if constexpr (requires {
                  Parser<R, W, U, ProcessorsType>::read_into(
                      _r, _var, std::forward<T>(_base));
                }) {
    return Parser<R, W, U, ProcessorsType>::read_into(_r, _var,
                                                      std::forward<T>(_base));
  } else {
    return Parser<R, W, U, ProcessorsType>::read(_r, _var);
  }
}

}  // namespace rfl::parsing

#endif
