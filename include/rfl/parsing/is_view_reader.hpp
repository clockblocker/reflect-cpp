#ifndef RFL_PARSING_ISVIEWREADER_HPP_
#define RFL_PARSING_ISVIEWREADER_HPP_

#include <cstddef>
#include <type_traits>

#include "ViewReader.hpp"

namespace rfl::parsing {

template <class R, class W, class ProcessorsType, bool _with_default,
          bool _use_bitmask>
class TableViewReader;

template <class R, class W, class ProcessorsType, std::size_t _size,
          bool _with_default, bool _use_bitmask>
class SizedTableViewReader;

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

template <class R, class W, class ProcessorsType, bool _use_bitmask>
class is_view_reader<
    TableViewReader<R, W, ProcessorsType, false, _use_bitmask>>
    : public std::true_type {};

template <class R, class W, class ProcessorsType, std::size_t _size,
          bool _use_bitmask>
class is_view_reader<
    SizedTableViewReader<R, W, ProcessorsType, _size, false, _use_bitmask>>
    : public std::true_type {};

template <class T>
constexpr bool is_view_reader_v =
    is_view_reader<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace rfl::parsing

#endif
