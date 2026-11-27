#ifndef RFL_PARSING_PEEL_TRANSPARENT_WRAPPERS_HPP_
#define RFL_PARSING_PEEL_TRANSPARENT_WRAPPERS_HPP_

#include <cstddef>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../DefaultVal.hpp"
#include "../internal/TransparentWrapper.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/is_array.hpp"
#include "../internal/is_deprecated.hpp"
#include "../internal/is_description.hpp"
#include "../internal/is_positional.hpp"
#include "../internal/is_rename.hpp"
#include "../internal/is_short.hpp"
#include "FieldErrors.hpp"
#include "Parser_base.hpp"
#include "wrap_error.hpp"

namespace rfl::parsing {

template <class T>
struct is_default_val_wrapper : std::false_type {};

template <class T>
struct is_default_val_wrapper<DefaultVal<T>> : std::true_type {};

template <class T>
constexpr bool is_default_val_wrapper_v =
    is_default_val_wrapper<std::remove_cvref_t<T>>::value;

/// Whether T is a pure metadata/annotation wrapper whose Parser::read does
/// nothing except read `typename T::Type` and construct `T` from it.
template <class T>
constexpr bool is_peelable_read_wrapper_v =
    rfl::internal::is_transparent_wrapper_v<T> ||
    rfl::internal::is_description_v<T> || rfl::internal::is_deprecated_v<T> ||
    rfl::internal::is_short_v<T> || rfl::internal::is_positional_v<T> ||
    rfl::internal::is_rename_v<T> || is_default_val_wrapper_v<T>;

/// Peels all pure metadata/annotation wrappers off T at compile time so that
/// fields with different help/group/short/value-name annotations share a single
/// `Parser<R, W, CoreType, ProcessorsType>::read` instantiation.
template <class T, class R>
struct peel_transparent {
  using CoreType = T;
  static constexpr bool peeled = false;
  static constexpr bool has_default_val = is_default_val_wrapper_v<T>;

  template <class U>
  static RFL_ALWAYS_INLINE U&& wrap(U&& _v) noexcept {
    return std::forward<U>(_v);
  }
  static RFL_ALWAYS_INLINE CoreType& unwrap_ref(T& _t) noexcept { return _t; }
};

template <class T, class R>
  requires(is_peelable_read_wrapper_v<T> &&
           !R::template has_custom_constructor<T>)
struct peel_transparent<T, R> {
  using Inner = std::remove_cvref_t<typename T::Type>;
  using Next = peel_transparent<Inner, R>;
  using CoreType = typename Next::CoreType;
  static constexpr bool peeled = true;
  static constexpr bool has_default_val =
      is_default_val_wrapper_v<T> || Next::has_default_val;

  static RFL_ALWAYS_INLINE T wrap(CoreType&& _v) {
    return T(Next::wrap(std::move(_v)));
  }

  static RFL_ALWAYS_INLINE CoreType& unwrap_ref(T& _t) noexcept {
    return Next::unwrap_ref(_t.value());
  }
};

/// Move-assigns a parsed value into an already-constructed target field.
/// Free function (outside ViewReaderWithDefault) so instantiations never carry
/// the enclosing NamedTuple in their mangled symbol name or DWARF debug info.
template <class Target, class Source>
RFL_ALWAYS_INLINE void move_assign_field(Target* _t, Source* _s) {
  if constexpr (!rfl::internal::is_array_v<Source> &&
                !std::is_array_v<Target>) {
    if constexpr (std::is_const_v<Target>) {
      // The struct was default-constructed, so *_t is a live const object
      // and must not be assigned to. Re-create it in place instead. If the
      // constructor throws, we are inside a noexcept function, so it cannot
      // be destroyed twice.
      using NonConstTarget = std::remove_const_t<Target>;
      auto* const ptr = const_cast<NonConstTarget*>(_t);
      std::destroy_at(ptr);
      ::new (ptr) NonConstTarget(std::move(*_s));
    } else {
      *_t = Target(std::move(*_s));
    }
  } else if constexpr (rfl::internal::is_array_v<Source>) {
    static_assert(std::is_array_v<Target>,
                  "Expected target to be a c-array.");
    for (size_t i = 0; i < _s->arr_.size(); ++i) {
      move_assign_field(&((*_t)[i]), &(_s->arr_[i]));
    }
  } else {
    for (size_t i = 0; i < _s->size(); ++i) {
      move_assign_field(&((*_t)[i]), &((*_s)[i]));
    }
  }
}

/// Placement-moves a parsed value into uninitialized target storage.
/// Free function (outside ViewReader) so instantiations never carry the
/// enclosing NamedTuple in their mangled symbol name or DWARF debug info.
template <class Target, class Source>
RFL_ALWAYS_INLINE void placement_move_field(Target* _t, Source* _s) {
  if constexpr (std::is_const_v<Target>) {
    placement_move_field(const_cast<std::remove_const_t<Target>*>(_t), _s);
  } else if constexpr (!rfl::internal::is_array_v<Source> &&
                       !std::is_array_v<Target>) {
    ::new (_t) Target(std::move(*_s));
  } else if constexpr (rfl::internal::is_array_v<Source>) {
    static_assert(std::is_array_v<Target>,
                  "Expected target to be a c-array.");
    for (size_t i = 0; i < _s->arr_.size(); ++i) {
      placement_move_field(&((*_t)[i]), &(_s->arr_[i]));
    }
  } else {
    for (size_t i = 0; i < _s->size(); ++i) {
      placement_move_field(&((*_t)[i]), &((*_s)[i]));
    }
  }
}

/// Reads one field using only (R, W, CoreType, ProcessorsType) as template
/// arguments—never the enclosing ViewType—so all fields with the same unwrapped
/// CoreType share a single compiled Parser::read instantiation.
template <class R, class W, class CoreType, class ProcessorsType>
auto read_field_core(const R& _r, const typename R::InputVarType& _var,
                     const std::string_view _name,
                     FieldErrors<ProcessorsType>* _errors,
                     const size_t _reserve_errors = 0) {
  auto res = Parser<R, W, CoreType, ProcessorsType>::read(_r, _var);
  if (!res) [[unlikely]] {
    if (_reserve_errors != 0) {
      _errors->reserve(_reserve_errors);
    }
    _errors->emplace_back(wrap_field_error(_name, std::move(res).error()));
  }
  return res;
}

}  // namespace rfl::parsing

#endif
