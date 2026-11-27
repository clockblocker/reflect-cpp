#ifndef RFL_INTERNAL_ISEMPTY_HPP_
#define RFL_INTERNAL_ISEMPTY_HPP_

#include <type_traits>

#include "../Tuple.hpp"
#include "is_named_tuple.hpp"
#include "is_tagged_union.hpp"
#include "ptr_tuple_t.hpp"

namespace rfl::internal {

template <class T>
constexpr bool is_empty() {
  using U = remove_all_ptrs_t<T>;
  if constexpr (is_tagged_union_based<U>) {
    return false;
  } else if constexpr (is_named_tuple_v<U>) {
    return U::size() == 0;
  } else {
    using TupleType = ptr_tuple_t<U>;
    return rfl::tuple_size_v<TupleType> == 0;
  }
}

}  // namespace rfl::internal

#endif
