#ifndef RFL_PARSING_CALL_DESTRUCTORS_WHERE_NECESSARY_HPP_
#define RFL_PARSING_CALL_DESTRUCTORS_WHERE_NECESSARY_HPP_

#include <array>
#include <type_traits>

#include "../get.hpp"
#include "../internal/always_inline.hpp"

namespace rfl::parsing {

/**
 * @brief Calls the destructors on an array.
 *
 * @tparam T The type of the elements in the array.
 * @param _size The size of the array.
 * @param _ptr A pointer to the array.
 */
template <class T>
void call_destructor_on_array(const size_t _size, T* _ptr) {
  if constexpr (!std::is_trivially_destructible_v<std::remove_all_extents_t<T>>) {
    for (size_t i = 0; i < _size; ++i) {
      if constexpr (std::is_array_v<T>) {
        call_destructor_on_array(sizeof(*_ptr) / sizeof(**_ptr), *(_ptr + i));
      } else if constexpr (std::is_destructible_v<std::remove_cv_t<T>>) {
        (_ptr + i)->~T();
      }
    }
  }
}

/**
 * @brief Calls the destructor on a single field pointer if it was set.
 * Free function parameterized only by the field's pointer type (never
 * ViewType) so instantiations do not carry the enclosing NamedTuple in their
 * mangled symbol name or DWARF debug info.
 */
template <class OriginalType>
RFL_ALWAYS_INLINE void destroy_field_if_set(
    const bool _is_set, const OriginalType& _slot) noexcept {
  using CleanOriginal = std::remove_cvref_t<OriginalType>;
  using ValueType =
      std::remove_cvref_t<std::remove_pointer_t<CleanOriginal>>;
  if constexpr (std::is_pointer_v<CleanOriginal> &&
                std::is_array_v<ValueType>) {
    if constexpr (!std::is_trivially_destructible_v<
                      std::remove_all_extents_t<ValueType>>) {
      if (_is_set) {
        call_destructor_on_array(sizeof(*_slot) / sizeof(**_slot), *_slot);
      }
    }
  } else if constexpr (std::is_pointer_v<CleanOriginal> &&
                       std::is_destructible_v<ValueType> &&
                       !std::is_trivially_destructible_v<ValueType>) {
    if (_is_set) {
      _slot->~ValueType();
    }
  }
}

/**
 * @brief Calls the destructors on the elements of a view, if necessary.
 *
 * @tparam ViewType The type of the view.
 * @tparam _size The size of the set array.
 * @param _set A boolean array indicating which elements have been set.
 * @param _view The view to call the destructors on.
 */
template <class ViewType, auto _size>
void call_destructors_where_necessary(const std::array<bool, _size>& _set,
                                      ViewType* _view) {
  [&]<int... is>(std::integer_sequence<int, is...>) {
    (destroy_field_if_set(std::get<is>(_set), rfl::get<is>(*_view)), ...);
  }(std::make_integer_sequence<int, _size>());
}

}  // namespace rfl::parsing

#endif
