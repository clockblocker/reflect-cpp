#ifndef RFL_PARSING_ISVIEWREADER_HPP_
#define RFL_PARSING_ISVIEWREADER_HPP_

#include <cstddef>
#include <type_traits>

#include "ViewReader.hpp"

namespace rfl::parsing {

template <class R, class W, class ProcessorsType, bool _with_default>
class DirectStructViewReader;

template <class Base, std::size_t _size>
class SizedDirectStructViewReader;

/**
 * @brief Trait to check if a type is a view reader.
 *
 * @tparam T The type to check.
 */
template <class T>
class is_view_reader;

template <class T>
class is_view_reader : public std::false_type {};

template <class R, class W, class NamedTupleType, class ProcessorsType>
class is_view_reader<ViewReader<R, W, NamedTupleType, ProcessorsType>>
    : public std::true_type {};

template <class R, class W, class ProcessorsType>
class is_view_reader<DirectStructViewReader<R, W, ProcessorsType, false>>
    : public std::true_type {};

template <class Base, std::size_t _size>
class is_view_reader<SizedDirectStructViewReader<Base, _size>>
    : public is_view_reader<Base> {};

template <class T>
constexpr bool is_view_reader_v =
    is_view_reader<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace rfl::parsing

#endif
