#ifndef RFL_PARSING_PEEL_TRANSPARENT_WRAPPERS_HPP_
#define RFL_PARSING_PEEL_TRANSPARENT_WRAPPERS_HPP_

#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../DefaultVal.hpp"
#include "../internal/TransparentWrapper.hpp"
#include "../internal/always_inline.hpp"
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

  static RFL_ALWAYS_INLINE T wrap(T&& _v) { return std::move(_v); }
};

template <class T, class R>
  requires(is_peelable_read_wrapper_v<T> &&
           !R::template has_custom_constructor<T>)
struct peel_transparent<T, R> {
  using Inner = std::remove_cvref_t<typename T::Type>;
  using Next = peel_transparent<Inner, R>;
  using CoreType = typename Next::CoreType;
  static constexpr bool peeled = true;

  static RFL_ALWAYS_INLINE T wrap(CoreType&& _v) {
    return T(Next::wrap(std::move(_v)));
  }
};

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
