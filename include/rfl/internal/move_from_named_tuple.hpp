#ifndef RFL_MOVE_FROM_NAMED_TUPLE_HPP_
#define RFL_MOVE_FROM_NAMED_TUPLE_HPP_

#include <array>
#include <type_traits>
#include <utility>

#include "../Tuple.hpp"
#include "../named_tuple_t.hpp"
#include "is_field.hpp"
#include "is_named_tuple.hpp"
#include "nt_to_ptr_named_tuple.hpp"
#include "ptr_field_tuple_t.hpp"

namespace rfl {
namespace internal {

template <class Field>
consteval int ptr_field_flattened_size() {
  using CleanField = std::remove_cvref_t<Field>;
  using T =
      std::remove_cvref_t<std::remove_pointer_t<typename CleanField::Type>>;
  if constexpr (is_flatten_field<CleanField>::value) {
    if constexpr (is_named_tuple_v<T>) {
      return static_cast<int>(T::size());
    } else {
      using SubPtrFieldTupleType = std::remove_cvref_t<ptr_field_tuple_t<T>>;
      constexpr int sub_size =
          static_cast<int>(rfl::tuple_size_v<SubPtrFieldTupleType>);
      if constexpr (sub_size == 0) {
        return 0;
      } else {
        return []<int... _is>(std::integer_sequence<int, _is...>) {
          return (0 + ... +
                  ptr_field_flattened_size<
                      tuple_element_t<_is, SubPtrFieldTupleType>>());
        }(std::make_integer_sequence<int, sub_size>());
      }
    }
  } else {
    return 1;
  }
}

template <class PtrFieldTupleType, int _offset>
consteval auto calc_ptr_field_offsets() {
  constexpr int size = static_cast<int>(
      rfl::tuple_size_v<std::remove_cvref_t<PtrFieldTupleType>>);
  std::array<int, size> offsets{};
  if constexpr (size > 0) {
    constexpr auto sizes = []<int... _is>(std::integer_sequence<int, _is...>) {
      return std::array<int, size>{ptr_field_flattened_size<
          tuple_element_t<_is, PtrFieldTupleType>>()...};
    }(std::make_integer_sequence<int, size>());
    int cur = _offset;
    for (int i = 0; i < size; ++i) {
      offsets[i] = cur;
      cur += sizes[i];
    }
  }
  return offsets;
}

template <class PtrFieldTupleType, class PtrNamedTupleType, int _offset = 0>
auto make_ptr_fields(PtrNamedTupleType& _n) {
  constexpr auto size =
      rfl::tuple_size_v<std::remove_cvref_t<PtrFieldTupleType>>;

  if constexpr (size == 0) {
    return rfl::Tuple<>();
  } else {
    constexpr auto offsets =
        calc_ptr_field_offsets<PtrFieldTupleType, _offset>();

    const auto get_one = [&]<int _i>(std::integral_constant<int, _i>) {
      using Field = std::remove_cvref_t<tuple_element_t<_i, PtrFieldTupleType>>;
      using T =
          std::remove_cvref_t<std::remove_pointer_t<typename Field::Type>>;
      constexpr int start = offsets[_i];
      if constexpr (is_flatten_field<Field>::value && is_named_tuple_v<T>) {
        return [&]<int... _ks>(std::integer_sequence<int, _ks...>) {
          return rfl::make_tuple(
              rfl::make_field<
                  tuple_element_t<start + _ks,
                                  typename PtrNamedTupleType::Fields>::name_>(
                  rfl::get<start + _ks>(_n))...);
        }(std::make_integer_sequence<int, static_cast<int>(T::size())>());
      } else if constexpr (is_flatten_field<Field>::value) {
        using SubPtrFieldTupleType = std::remove_cvref_t<ptr_field_tuple_t<T>>;
        return make_ptr_fields<SubPtrFieldTupleType, PtrNamedTupleType, start>(
            _n);
      } else {
        return rfl::make_field<Field::name_>(rfl::get<start>(_n));
      }
    };

    return [&]<int... _is>(std::integer_sequence<int, _is...>) {
      return rfl::make_tuple(get_one(std::integral_constant<int, _is>{})...);
    }(std::make_integer_sequence<int, size>());
  }
}

template <class T, class Pointers>
auto move_from_ptr_fields(Pointers& _ptrs) {
  const auto get_one = [&]<int _i>(std::integral_constant<int, _i>) {
    using FieldType = tuple_element_t<_i, std::remove_cvref_t<Pointers>>;
    if constexpr (is_field_v<FieldType>) {
      return rfl::make_field<FieldType::name_>(
          std::move(*rfl::get<_i>(_ptrs).value()));
    } else {
      using PtrFieldTupleType = std::remove_cvref_t<ptr_field_tuple_t<T>>;
      using U = std::remove_cvref_t<std::remove_pointer_t<
          typename tuple_element_t<_i, PtrFieldTupleType>::Type>>;
      return move_from_ptr_fields<U>(rfl::get<_i>(_ptrs));
    }
  };

  constexpr auto size = rfl::tuple_size_v<std::remove_cvref_t<Pointers>>;

  return [&]<int... _is>(std::integer_sequence<int, _is...>) {
    return T{std::move(get_one(std::integral_constant<int, _is>{}))...};
  }
  (std::make_integer_sequence<int, size>());
}

/// Creates a struct of type T from a named tuple by moving the underlying
/// fields.
template <class T, class NamedTupleType>
T move_from_named_tuple(NamedTupleType&& _n) {
  using RequiredType = std::remove_cvref_t<named_tuple_t<T>>;

  if constexpr (is_named_tuple_v<std::remove_cvref_t<T>>) {
    return std::move(_n);

  } else if constexpr (std::is_same<std::remove_cvref_t<NamedTupleType>,
                                    RequiredType>()) {
    auto ptr_named_tuple = nt_to_ptr_named_tuple(_n);

    using PtrFieldTupleType = std::remove_cvref_t<ptr_field_tuple_t<T>>;

    auto pointers = make_ptr_fields<PtrFieldTupleType>(ptr_named_tuple);

    return move_from_ptr_fields<T>(pointers);

  } else {
    return move_from_named_tuple<T, RequiredType>(RequiredType(std::move(_n)));
  }
}

}  // namespace internal
}  // namespace rfl

#endif
