#ifndef RFL_PARSING_FLATTENED_TAGGED_UNION_PARSER_HPP_
#define RFL_PARSING_FLATTENED_TAGGED_UNION_PARSER_HPP_

#include <array>
#include <cstddef>
#include <exception>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "../Literal.hpp"
#include "../NamedTuple.hpp"
#include "../Result.hpp"
#include "../TaggedUnion.hpp"
#include "../Tuple.hpp"
#include "../get.hpp"
#include "../internal/concat_literals.hpp"
#include "../internal/default_if_missing_v.hpp"
#include "../internal/has_default_val_v.hpp"
#include "../internal/has_fields.hpp"
#include "../internal/has_flattened_tagged_union.hpp"
#include "../internal/has_reflection_method_v.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/has_reflector.hpp"
#include "../internal/is_named_tuple.hpp"
#include "../internal/is_tagged_union.hpp"
#include "../internal/make_tag.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "../internal/no_field_names_v.hpp"
#include "../internal/processed_t.hpp"
#include "../internal/ptr_cast.hpp"
#include "../internal/strings/strings.hpp"
#include "../internal/tag_t.hpp"
#include "../internal/to_ptr_named_tuple.hpp"
#include "../to_view.hpp"
#include "../type_name_t.hpp"
#include "../visit.hpp"
#include "AreReaderAndWriter.hpp"
#include "NamedTupleParser.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "TaggedUnionWrapper.hpp"
#include "call_destructors_where_necessary.hpp"
#include "is_tagged_union_wrapper.hpp"
#include "schema/Type.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "schemaful/IsSchemafulWriter.hpp"
#include "tagged_union_wrapper_no_ptr.hpp"
#include "wrap_error.hpp"

namespace rfl::parsing {

namespace flattened_tu_detail {

template <internal::StringLiteral _discriminator, class Alt,
          bool _as_const_pointer>
struct WrappedAltType {
  using CleanAlt = std::remove_cvref_t<Alt>;
  using Type = std::conditional_t<
      internal::contains_discriminator_v<CleanAlt, _discriminator>, CleanAlt,
      std::conditional_t<
          internal::has_fields<CleanAlt>(),
          TaggedUnionWrapperWithFields<
              CleanAlt, internal::tag_t<_discriminator, CleanAlt>,
              _discriminator, _as_const_pointer>,
          TaggedUnionWrapperNoFields<CleanAlt,
                                     internal::tag_t<_discriminator, CleanAlt>,
                                     _discriminator, _as_const_pointer>>>;
};

template <internal::StringLiteral _discriminator, class Alt,
          bool _as_const_pointer>
using wrapped_alt_t =
    typename WrappedAltType<_discriminator, Alt, _as_const_pointer>::Type;

template <internal::StringLiteral _discriminator, class Alt>
auto wrap_alt_for_write(const Alt& _alt) {
  using CleanAlt = std::remove_cvref_t<Alt>;
  if constexpr (internal::contains_discriminator_v<CleanAlt, _discriminator>) {
    return &_alt;
  } else {
    const auto tag = internal::make_tag<_discriminator, CleanAlt>(_alt);
    using TagType = std::remove_cvref_t<decltype(tag)>;
    if constexpr (internal::has_fields<CleanAlt>()) {
      using WrapperType =
          TaggedUnionWrapperWithFields<CleanAlt, TagType, _discriminator, true>;
      return WrapperType{.tag = tag, .fields = &_alt};
    } else {
      using WrapperType =
          TaggedUnionWrapperNoFields<CleanAlt, TagType, _discriminator, true>;
      return WrapperType{.tag = tag, .fields = &_alt};
    }
  }
}

template <class... Ts>
struct TypeList {};

template <class List1, class List2>
struct ConcatTypeLists;

template <class... Ts1, class... Ts2>
struct ConcatTypeLists<TypeList<Ts1...>, TypeList<Ts2...>> {
  using type = TypeList<Ts1..., Ts2...>;
};

template <class TupleType>
struct TuplePtrsToTypeList;

template <class... PtrTypes>
struct TuplePtrsToTypeList<rfl::Tuple<PtrTypes...>> {
  using type = TypeList<internal::remove_all_ptrs_t<PtrTypes>...>;
};

template <class ProcessorsType, class OuterNames, class TUList>
constexpr void validate_tu_list();

template <class S, class ProcessorsType, class OuterNames, class RemainingTUs>
constexpr void validate_struct_with_remaining() {
  using U = internal::remove_all_ptrs_t<S>;
  if constexpr (internal::is_tagged_union_based<U>) {
    using NextList =
        typename ConcatTypeLists<TypeList<U>, RemainingTUs>::type;
    validate_tu_list<ProcessorsType, OuterNames, NextList>();
  } else if constexpr (internal::has_reflection_type_v<U>) {
    validate_struct_with_remaining<
        std::remove_cvref_t<typename U::ReflectionType>, ProcessorsType,
        OuterNames, RemainingTUs>();
  } else if constexpr (internal::has_read_reflector<U> ||
                       internal::has_write_reflector<U>) {
    validate_struct_with_remaining<
        std::remove_cvref_t<typename Reflector<U>::ReflType>, ProcessorsType,
        OuterNames, RemainingTUs>();
  } else if constexpr (internal::is_named_tuple_v<U> ||
                       (std::is_class_v<U> && std::is_aggregate_v<U>)) {
    using RegNT = internal::processed_t<U, ProcessorsType>;
    using RegNames = typename RegNT::Names;
    static_assert(
        !OuterNames::template contains_any<RegNames>(),
        "Flattening rfl::TaggedUnion created duplicate field names.");
    using NewOuter =
        decltype(internal::concat_literals(OuterNames{}, RegNames{}));
    if constexpr (internal::has_flattened_tagged_union_v<U>) {
      using OwnTUs =
          typename TuplePtrsToTypeList<internal::flattened_tu_ptrs_t<U>>::type;
      using NextList = typename ConcatTypeLists<OwnTUs, RemainingTUs>::type;
      validate_tu_list<ProcessorsType, NewOuter, NextList>();
    } else {
      validate_tu_list<ProcessorsType, NewOuter, RemainingTUs>();
    }
  } else {
    validate_tu_list<ProcessorsType, OuterNames, RemainingTUs>();
  }
}

template <class ProcessorsType, class OuterNames,
          internal::StringLiteral _discriminator, class... Alts,
          class TailTUList>
constexpr void validate_one_tu_alts_impl(
    const TaggedUnion<_discriminator, Alts...>*, TailTUList) {
  static_assert(
      !OuterNames::template contains<_discriminator>(),
      "Duplicate discriminator or field name when flattening "
      "rfl::TaggedUnion: inner discriminator collides with an outer field or "
      "discriminator.");
  (validate_struct_with_remaining<
       wrapped_alt_t<_discriminator, Alts, false>, ProcessorsType, OuterNames,
       TailTUList>(),
   ...);
}

template <class ProcessorsType, class OuterNames, class TUList>
constexpr void validate_tu_list() {
  if constexpr (!std::is_same_v<TUList, TypeList<>>) {
    []<class HeadTU, class... TailTUs>(const TypeList<HeadTU, TailTUs...>*) {
      using BaseTU = internal::tagged_union_base_t<HeadTU>;
      validate_one_tu_alts_impl<ProcessorsType, OuterNames>(
          static_cast<const BaseTU*>(nullptr), TypeList<TailTUs...>{});
    }(static_cast<const TUList*>(nullptr));
  }
}

template <class NT1, class NT2>
auto concat_named_tuples(NT1&& _nt1, NT2&& _nt2) {
  if constexpr (std::remove_cvref_t<NT1>::size() == 0) {
    return std::forward<NT2>(_nt2);
  } else if constexpr (std::remove_cvref_t<NT2>::size() == 0) {
    return std::forward<NT1>(_nt1);
  } else {
    return std::forward<NT1>(_nt1).add(std::forward<NT2>(_nt2));
  }
}

template <int _i, class NT>
auto get_indexed_field(const NT& _nt) {
  using FieldType =
      tuple_element_t<_i, typename std::remove_cvref_t<NT>::Fields>;
  return FieldType(rfl::get<_i>(_nt));
}

template <int _i, int _end, class RegView, class AccView>
auto append_prefix_fields(const RegView& _reg_view, AccView&& _acc_view) {
  if constexpr (_i >= _end) {
    return std::forward<AccView>(_acc_view);
  } else {
    auto next =
        std::forward<AccView>(_acc_view).add(get_indexed_field<_i>(_reg_view));
    return append_prefix_fields<_i + 1, _end>(_reg_view, std::move(next));
  }
}

}  // namespace flattened_tu_detail

template <class S, class ProcessorsType>
constexpr void validate_flattened_struct_no_collisions() {
  flattened_tu_detail::validate_struct_with_remaining<
      S, ProcessorsType, rfl::Literal<>, flattened_tu_detail::TypeList<>>();
}

template <class TU, class ProcessorsType>
constexpr void validate_tagged_union_no_collisions() {
  flattened_tu_detail::validate_tu_list<
      ProcessorsType, rfl::Literal<>,
      flattened_tu_detail::TypeList<internal::remove_all_ptrs_t<TU>>>();
}

template <class R, class W, class T, class ProcessorsType>
  requires AreReaderAndWriter<R, W, T>
struct FlattenedTaggedUnionParser {
  using InputVarType = typename R::InputVarType;
  using ParentType = Parent<W>;

  constexpr static bool no_field_names_ =
      internal::no_field_names_v<ProcessorsType>;

  using InputObjectOrArrayType =
      std::conditional_t<no_field_names_, typename R::InputArrayType,
                         typename R::InputObjectType>;
  using OutputObjectOrArrayType =
      std::conditional_t<no_field_names_, typename W::OutputArrayType,
                         typename W::OutputObjectType>;

  static Result<T> read(const R& _r, const InputVarType& _var) noexcept {
    static_assert(
        !schemaful::IsSchemafulReader<R>,
        "Flattening rfl::TaggedUnion is not supported for schemaful formats.");
    validate_flattened_struct_no_collisions<T, ProcessorsType>();

    auto obj_or_arr = [&]() {
      if constexpr (no_field_names_) {
        return _r.to_array(_var);
      } else {
        return _r.to_object(_var);
      }
    }();
    if (!obj_or_arr) [[unlikely]] {
      return error(obj_or_arr.error());
    }

    constexpr bool use_default =
        internal::default_if_missing_v<ProcessorsType> ||
        internal::has_default_val_v<T>;

    const auto leaf_cont = [&]<class CombinedView>(
                               CombinedView& _combined_view,
                               size_t) -> std::optional<Error> {
      if constexpr (use_default) {
        return Parser<R, W, CombinedView, ProcessorsType>::
            read_view_with_default(_r, _var, &_combined_view);
      } else {
        auto [set, err] =
            Parser<R, W, CombinedView, ProcessorsType>::read_view(
                _r, _var, &_combined_view);
        if (err) [[unlikely]] {
          call_destructors_where_necessary(set, &_combined_view);
          return std::move(err);
        }
        return std::nullopt;
      }
    };

    if constexpr (use_default) {
      static_assert(
          !internal::has_default_val_v<T> || requires { T{}; },
          "A struct with rfl::DefaultVal fields must be default-constructible, "
          "because the default values come from a default-constructed "
          "instance.");
      try {
        auto t = T{};
        try {
          auto err = resolve_struct_and_read<true>(
              _r, _var, *obj_or_arr, 0, t, rfl::NamedTuple<>(), leaf_cont);
          if (err) [[unlikely]] {
            return error(std::move(*err));
          }
          return t;
        } catch (std::exception& e) {
          return error(e.what());
        }
      } catch (std::exception& e) {
        return error(e.what());
      }
    } else {
      alignas(T) unsigned char buf[sizeof(T)]{};
      auto* ptr = internal::ptr_cast<T*>(&buf);
      auto err = resolve_struct_and_read<false>(
          _r, _var, *obj_or_arr, 0, *ptr, rfl::NamedTuple<>(), leaf_cont);
      if (err) [[unlikely]] {
        return error(std::move(*err));
      }
      auto res = Result<T>(std::move(*ptr));
      ptr->~T();
      return res;
    }
  }

  template <class P>
  static void write(const W& _w, const T& _val, const P& _parent) {
    static_assert(
        !schemaful::IsSchemafulWriter<W>,
        "Flattening rfl::TaggedUnion is not supported for schemaful formats.");
    validate_flattened_struct_no_collisions<T, ProcessorsType>();

    const auto num_fields = count_fields(_val);
    if constexpr (no_field_names_) {
      auto arr = ParentType::add_array(_w, num_fields, _parent);
      write_fields_into_object(_w, _val, &arr);
      _w.end_array(&arr);
    } else {
      auto obj = ParentType::add_object(_w, num_fields, _parent);
      write_fields_into_object(_w, _val, &obj);
      _w.end_object(&obj);
    }
  }

  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    validate_flattened_struct_no_collisions<T, ProcessorsType>();
    auto branches = collect_flattened_schemas<T>(_definitions);
    if (branches.size() == 1) {
      return std::move(branches[0]);
    }
    return schema::Type{schema::Type::AnyOf{.types_ = std::move(branches)}};
  }

 private:
  // =========================================================================
  // Reading helpers (CPS combined-view resolution)
  // =========================================================================

  template <internal::StringLiteral _discriminator>
  static Result<std::string> get_discriminator(
      const R& _r, const InputObjectOrArrayType& _obj_or_arr,
      const size_t _array_offset) noexcept {
    const auto to_type = [&_r](auto _v) {
      return _r.template to_basic_type<std::string>(_v);
    };
    const auto embellish_error = [](const auto&) {
      std::stringstream stream;
      stream << "Could not parse tagged union: Could not find field '"
             << _discriminator.str() << "' or type of field was not a string.";
      return Error(stream.str());
    };
    if constexpr (no_field_names_) {
      return _r.get_field_from_array(_array_offset, _obj_or_arr)
          .and_then(to_type)
          .transform_error(embellish_error);
    } else {
      return _r.get_field_from_object(_discriminator.str(), _obj_or_arr)
          .and_then(to_type)
          .transform_error(embellish_error);
    }
  }

  template <bool _use_default, class S, class AccView, class Cont>
  static std::optional<Error> resolve_struct_and_read(
      const R& _r, const InputVarType& _var,
      const InputObjectOrArrayType& _obj_or_arr, const size_t _array_offset,
      S& _target, AccView&& _acc_view, const Cont& _cont) noexcept {
    using CleanS = std::remove_cvref_t<S>;
    if constexpr (internal::is_tagged_union_based<CleanS>) {
      return resolve_one_tu<_use_default>(_r, _var, _obj_or_arr, _array_offset,
                                          &_target,
                                          std::forward<AccView>(_acc_view),
                                          _cont);
    } else if constexpr (internal::has_flattened_tagged_union_v<CleanS>) {
      auto reg_view = ProcessorsType::template process<CleanS>(
          internal::to_ptr_named_tuple(_target));
      auto tu_ptrs = internal::to_flattened_tu_ptrs(_target);
      constexpr size_t raw_reg = internal::raw_regular_field_count<CleanS>();
      constexpr size_t delta =
          std::remove_cvref_t<decltype(reg_view)>::size() - raw_reg;
      auto acc_with_prefix = flattened_tu_detail::append_prefix_fields<0, delta>(
          reg_view, std::forward<AccView>(_acc_view));
      return resolve_slots<_use_default, CleanS, 0, delta>(
          _r, _var, _obj_or_arr, _array_offset + delta, reg_view, tu_ptrs,
          std::move(acc_with_prefix), _cont);
    } else {
      auto reg_view = ProcessorsType::template process<CleanS>(
          internal::to_ptr_named_tuple(_target));
      constexpr size_t reg_size =
          std::remove_cvref_t<decltype(reg_view)>::size();
      auto combined = flattened_tu_detail::concat_named_tuples(
          std::forward<AccView>(_acc_view), std::move(reg_view));
      return _cont(combined, _array_offset + reg_size);
    }
  }

  template <bool _use_default, class CleanS, size_t _slot_i, size_t _delta,
            class RegView, class TUPtrs, class AccView, class Cont>
  static std::optional<Error> resolve_slots(
      const R& _r, const InputVarType& _var,
      const InputObjectOrArrayType& _obj_or_arr, const size_t _array_offset,
      const RegView& _reg_view, TUPtrs& _tu_ptrs, AccView&& _acc_view,
      const Cont& _cont) noexcept {
    constexpr auto slots = internal::flattened_tu_slots<CleanS>();
    if constexpr (_slot_i == slots.size()) {
      auto final_view = std::forward<AccView>(_acc_view);
      return _cont(final_view, _array_offset);
    } else {
      constexpr auto slot = slots[_slot_i];
      if constexpr (!slot.is_tu) {
        constexpr int reg_i = static_cast<int>(_delta + slot.index);
        auto next_view = std::forward<AccView>(_acc_view)
                             .add(flattened_tu_detail::get_indexed_field<reg_i>(
                                 _reg_view));
        return resolve_slots<_use_default, CleanS, _slot_i + 1, _delta>(
            _r, _var, _obj_or_arr, _array_offset + 1, _reg_view, _tu_ptrs,
            std::move(next_view), _cont);
      } else {
        auto* tu_ptr = rfl::get<static_cast<int>(slot.index)>(_tu_ptrs);
        const auto next_cont = [&](auto& _view_after_tu,
                                   const size_t _offset_after_tu) {
          return resolve_slots<_use_default, CleanS, _slot_i + 1, _delta>(
              _r, _var, _obj_or_arr, _offset_after_tu, _reg_view, _tu_ptrs,
              std::move(_view_after_tu), _cont);
        };
        return resolve_one_tu<_use_default>(_r, _var, _obj_or_arr,
                                            _array_offset, tu_ptr,
                                            std::forward<AccView>(_acc_view),
                                            next_cont);
      }
    }
  }

  template <bool _use_default, class TUType, class AccView, class Cont>
  static std::optional<Error> resolve_one_tu(
      const R& _r, const InputVarType& _var,
      const InputObjectOrArrayType& _obj_or_arr, const size_t _array_offset,
      TUType* _tu_ptr, const AccView& _acc_view, const Cont& _cont) noexcept {
    using BaseTU = internal::tagged_union_base_t<TUType>;
    const auto disc_res =
        get_discriminator<BaseTU::discriminator_>(_r, _obj_or_arr,
                                                  _array_offset);
    if (!disc_res) [[unlikely]] {
      return disc_res.error();
    }
    return dispatch_tu_alternatives<_use_default>(
        _r, _var, _obj_or_arr, _array_offset, _tu_ptr, *disc_res, _acc_view,
        _cont, static_cast<const BaseTU*>(nullptr));
  }

  template <bool _use_default, class TUType, class AccView, class Cont,
            internal::StringLiteral _discriminator, class... Alts>
  static std::optional<Error> dispatch_tu_alternatives(
      const R& _r, const InputVarType& _var,
      const InputObjectOrArrayType& _obj_or_arr, const size_t _array_offset,
      TUType* _tu_ptr, const std::string& _disc_value, const AccView& _acc_view,
      const Cont& _cont,
      const TaggedUnion<_discriminator, Alts...>*) noexcept {
    using BaseTU = TaggedUnion<_discriminator, Alts...>;
    using PossibleTags = possible_tags_t<BaseTU>;
    static_assert(!PossibleTags::has_duplicates(),
                  "Duplicate tags are not allowed inside tagged unions.");

    std::optional<Error> res;
    bool match_found = false;
    [&]<int... _is>(std::integer_sequence<int, _is...>) {
      (try_one_tu_alt<_use_default, _is, BaseTU>(
           _r, _var, _obj_or_arr, _array_offset, _tu_ptr, _disc_value,
           _acc_view, _cont, &res, &match_found),
       ...);
    }(std::make_integer_sequence<int, sizeof...(Alts)>());

    if (match_found) [[likely]] {
      return res;
    }
    const auto names = PossibleTags::names();
    std::stringstream stream;
    stream << "Could not parse tagged union, could not match "
           << _discriminator.str() << " '" << _disc_value
           << "'. The following tags are allowed: "
           << internal::strings::join(", ", names);
    return Error(stream.str());
  }

  template <bool _use_default, int _i, class BaseTU, class TUType,
            class AccView, class Cont>
  static void try_one_tu_alt(const R& _r, const InputVarType& _var,
                             const InputObjectOrArrayType& _obj_or_arr,
                             const size_t _array_offset, TUType* _tu_ptr,
                             const std::string& _disc_value,
                             const AccView& _acc_view, const Cont& _cont,
                             std::optional<Error>* _result,
                             bool* _match_found) noexcept {
    using AltType = std::remove_cvref_t<
        rfl::variant_alternative_t<_i, typename BaseTU::VariantType>>;
    constexpr auto disc = BaseTU::discriminator_;

    if (!*_match_found && internal::tag_t<disc, AltType>::contains(_disc_value)) {
      *_match_found = true;

      using WrappedAlt = flattened_tu_detail::wrapped_alt_t<disc, AltType, false>;

      const auto extract_alt = [](WrappedAlt& _w) -> AltType {
        if constexpr (is_tagged_union_wrapper_v<WrappedAlt>) {
          return std::move(_w.fields());
        } else {
          return std::move(_w);
        }
      };

      const auto embellish = [&](Error _e) {
        std::stringstream stream;
        stream << "Could not parse tagged union with discriminator "
               << disc.str() << " '" << _disc_value << "': ";
        return wrap_error(stream.str(), std::move(_e));
      };

      auto acc_copy = _acc_view;
      if constexpr (_use_default) {
        WrappedAlt alt{};
        auto err = resolve_struct_and_read<true>(
            _r, _var, _obj_or_arr, _array_offset, alt, std::move(acc_copy),
            _cont);
        if (err) [[unlikely]] {
          *_result = embellish(std::move(*err));
          return;
        }
        *_tu_ptr = internal::from_tagged_union_base<TUType>(
            BaseTU(extract_alt(alt)));
      } else {
        alignas(WrappedAlt) unsigned char alt_buf[sizeof(WrappedAlt)]{};
        auto* alt_ptr = internal::ptr_cast<WrappedAlt*>(&alt_buf);
        auto err = resolve_struct_and_read<false>(
            _r, _var, _obj_or_arr, _array_offset, *alt_ptr, std::move(acc_copy),
            _cont);
        if (err) [[unlikely]] {
          *_result = embellish(std::move(*err));
          return;
        }
        ::new (_tu_ptr) TUType(internal::from_tagged_union_base<TUType>(
            BaseTU(extract_alt(*alt_ptr))));
        alt_ptr->~WrappedAlt();
      }
    }
  }

  // =========================================================================
  // Writing helpers (single flat object/array in declaration order)
  // =========================================================================

  template <class X>
  static size_t count_fields(const X& _x) {
    using CleanX = std::remove_cvref_t<X>;
    if constexpr (std::is_pointer_v<CleanX>) {
      return count_fields(*_x);
    } else if constexpr (internal::is_named_tuple_v<CleanX>) {
      return _x.num_fields();
    } else if constexpr (internal::is_tagged_union_based<CleanX>) {
      using BaseTU = internal::tagged_union_base_t<CleanX>;
      return rfl::visit(
          [](const auto& _alt) -> size_t {
            const auto wrapped = flattened_tu_detail::wrap_alt_for_write<
                BaseTU::discriminator_>(_alt);
            return count_fields(wrapped);
          },
          static_cast<const BaseTU&>(_x).variant());
    } else if constexpr (internal::has_flattened_tagged_union_v<CleanX>) {
      const auto reg_nt = ProcessorsType::template process<CleanX>(
          internal::to_ptr_named_tuple(_x));
      const auto tu_ptrs = internal::to_flattened_tu_ptrs(_x);
      size_t total = reg_nt.num_fields();
      constexpr size_t num_tus =
          rfl::tuple_size_v<std::remove_cvref_t<decltype(tu_ptrs)>>;
      [&]<int... _is>(std::integer_sequence<int, _is...>) {
        ((total += count_fields(*rfl::get<_is>(tu_ptrs))), ...);
      }(std::make_integer_sequence<int, num_tus>());
      return total;
    } else if constexpr (internal::has_write_reflector<CleanX>) {
      return count_fields(Reflector<CleanX>::from(_x));
    } else if constexpr (internal::has_reflection_type_v<CleanX>) {
      if constexpr (internal::has_reflection_method_v<CleanX>) {
        return count_fields(_x.reflection());
      } else {
        const auto& [r] = _x;
        return count_fields(r);
      }
    } else {
      const auto ptr_nt = ProcessorsType::template process<CleanX>(
          internal::to_ptr_named_tuple(_x));
      return ptr_nt.num_fields();
    }
  }

  template <class X>
  static void write_fields_into_object(const W& _w, const X& _x,
                                       OutputObjectOrArrayType* _obj) {
    using CleanX = std::remove_cvref_t<X>;
    if constexpr (std::is_pointer_v<CleanX>) {
      write_fields_into_object(_w, *_x, _obj);
    } else if constexpr (internal::is_named_tuple_v<CleanX>) {
      Parser<R, W, CleanX, ProcessorsType>::write_fields(_w, _x, _obj);
    } else if constexpr (internal::is_tagged_union_based<CleanX>) {
      using BaseTU = internal::tagged_union_base_t<CleanX>;
      rfl::visit(
          [&](const auto& _alt) {
            const auto wrapped = flattened_tu_detail::wrap_alt_for_write<
                BaseTU::discriminator_>(_alt);
            write_fields_into_object(_w, wrapped, _obj);
          },
          static_cast<const BaseTU&>(_x).variant());
    } else if constexpr (internal::has_flattened_tagged_union_v<CleanX>) {
      const auto reg_nt = ProcessorsType::template process<CleanX>(
          internal::to_ptr_named_tuple(_x));
      using RegNT = std::remove_cvref_t<decltype(reg_nt)>;
      const auto tu_ptrs = internal::to_flattened_tu_ptrs(_x);
      constexpr auto slots = internal::flattened_tu_slots<CleanX>();
      constexpr size_t raw_reg = internal::raw_regular_field_count<CleanX>();
      constexpr size_t delta = RegNT::size() - raw_reg;

      [&]<int... _is>(std::integer_sequence<int, _is...>) {
        (Parser<R, W, RegNT, ProcessorsType>::template write_single_field<_is>(
             _w, reg_nt, _obj),
         ...);
      }(std::make_integer_sequence<int, delta>());

      [&]<size_t... _si>(std::index_sequence<_si...>) {
        (write_one_slot<slots[_si], delta>(_w, reg_nt, tu_ptrs, _obj), ...);
      }(std::make_index_sequence<slots.size()>());
    } else if constexpr (internal::has_write_reflector<CleanX>) {
      write_fields_into_object(_w, Reflector<CleanX>::from(_x), _obj);
    } else if constexpr (internal::has_reflection_type_v<CleanX>) {
      if constexpr (internal::has_reflection_method_v<CleanX>) {
        write_fields_into_object(_w, _x.reflection(), _obj);
      } else {
        const auto& [r] = _x;
        write_fields_into_object(_w, r, _obj);
      }
    } else {
      const auto ptr_nt = ProcessorsType::template process<CleanX>(
          internal::to_ptr_named_tuple(_x));
      using PtrNT = std::remove_cvref_t<decltype(ptr_nt)>;
      Parser<R, W, PtrNT, ProcessorsType>::write_fields(_w, ptr_nt, _obj);
    }
  }

  template <internal::FlattenedSlot _slot, size_t _delta, class RegNT,
            class TUPtrs>
  static void write_one_slot(const W& _w, const RegNT& _reg_nt,
                             const TUPtrs& _tu_ptrs,
                             OutputObjectOrArrayType* _obj) {
    if constexpr (!_slot.is_tu) {
      constexpr int idx = static_cast<int>(_delta + _slot.index);
      Parser<R, W, RegNT, ProcessorsType>::template write_single_field<idx>(
          _w, _reg_nt, _obj);
    } else {
      write_fields_into_object(
          _w, *rfl::get<static_cast<int>(_slot.index)>(_tu_ptrs), _obj);
    }
  }

  // =========================================================================
  // Schema generation helpers (flat merged Object/Tuple branches)
  // =========================================================================

  using SchemaObjOrTuple = std::conditional_t<no_field_names_,
                                              schema::Type::Tuple,
                                              schema::Type::Object>;

  static schema::Type merge_schema_pair(const schema::Type& _a,
                                        const schema::Type& _b) {
    if constexpr (no_field_names_) {
      auto merged = rfl::get<schema::Type::Tuple>(_a.variant_);
      const auto& b_tup = rfl::get<schema::Type::Tuple>(_b.variant_);
      for (const auto& item : b_tup.types_) {
        merged.types_.push_back(item);
      }
      return schema::Type{std::move(merged)};
    } else {
      auto merged = rfl::get<schema::Type::Object>(_a.variant_);
      const auto& b_obj = rfl::get<schema::Type::Object>(_b.variant_);
      for (const auto& [k, v] : b_obj.types_) {
        merged.types_[k] = v;
      }
      if (const auto* b_ptr =
              rfl::get_if<std::shared_ptr<schema::Type>>(
                  &b_obj.additional_properties_);
          b_ptr && *b_ptr) {
        merged.additional_properties_ = *b_ptr;
      } else if (const auto* a_ptr =
                     rfl::get_if<std::shared_ptr<schema::Type>>(
                         &merged.additional_properties_);
                 a_ptr && *a_ptr) {
        // Keep _a's ExtraFields schema.
      } else if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
        merged.additional_properties_ = false;
      }
      return schema::Type{std::move(merged)};
    }
  }

  template <class X>
  static std::vector<schema::Type> collect_flattened_schemas(
      std::map<std::string, schema::Type>* _definitions) {
    using CleanX = internal::remove_all_ptrs_t<X>;
    if constexpr (internal::is_tagged_union_based<CleanX>) {
      using BaseTU = internal::tagged_union_base_t<CleanX>;
      return collect_tu_schemas(_definitions,
                                static_cast<const BaseTU*>(nullptr));
    } else if constexpr (internal::has_flattened_tagged_union_v<CleanX>) {
      using XVal = tagged_union_wrapper_no_ptr_t<CleanX>;
      using RegNT = internal::processed_t<XVal, ProcessorsType>;
      using RegParser = Parser<R, W, RegNT, ProcessorsType>;
      constexpr auto slots = internal::flattened_tu_slots<XVal>();
      constexpr size_t raw_reg = internal::raw_regular_field_count<XVal>();
      constexpr size_t delta = RegNT::size() - raw_reg;

      const auto run_with_view = [&](auto* _view_ptr) {
        using ViewT = std::remove_pointer_t<decltype(_view_ptr)>;
        std::vector<schema::Type> current;
        current.emplace_back(schema::Type{SchemaObjOrTuple{}});

        const auto add_reg_field = [&]<size_t _idx>() {
          for (auto& branch : current) {
            auto& target = rfl::get<SchemaObjOrTuple>(branch.variant_);
            RegParser::template add_single_field_to_schema<ViewT, _idx>(
                _definitions, &target, _view_ptr);
          }
        };

        [&]<size_t... _is>(std::index_sequence<_is...>) {
          (add_reg_field.template operator()<_is>(), ...);
        }(std::make_index_sequence<delta>());

        using TUPtrsTuple = internal::flattened_tu_ptrs_t<XVal>;
        [&]<size_t... _si>(std::index_sequence<_si...>) {
          ([&]<size_t _k>() {
            constexpr auto slot = slots[_k];
            if constexpr (!slot.is_tu) {
              add_reg_field.template operator()<delta + slot.index>();
            } else {
              using TU = internal::remove_all_ptrs_t<
                  tuple_element_t<slot.index, TUPtrsTuple>>;
              auto sub_branches = collect_flattened_schemas<TU>(_definitions);
              std::vector<schema::Type> next;
              next.reserve(current.size() * sub_branches.size());
              for (const auto& lhs : current) {
                for (const auto& rhs : sub_branches) {
                  next.emplace_back(merge_schema_pair(lhs, rhs));
                }
              }
              current = std::move(next);
            }
          }.template operator()<_si>(), ...);
        }(std::make_index_sequence<slots.size()>());

        if constexpr (!no_field_names_) {
          // Apply ExtraFields or NoExtraFields from RegNT onto every branch.
          apply_extra_properties_from_nt<RegNT>(_definitions, &current);
        }
        return current;
      };

      if constexpr (internal::has_default_val_v<RegNT>) {
        static_assert(
            requires { XVal{}; },
            "A struct with rfl::DefaultVal fields must be "
            "default-constructible, because the default values come from a "
            "default-constructed instance.");
        auto defaults = XVal{};
        auto view =
            ProcessorsType::template process<XVal>(to_view(defaults));
        return run_with_view(&view);
      } else {
        return run_with_view(static_cast<void*>(nullptr));
      }
    } else if constexpr (internal::has_reflection_type_v<CleanX>) {
      return collect_flattened_schemas<
          std::remove_cvref_t<typename CleanX::ReflectionType>>(_definitions);
    } else if constexpr (internal::has_read_reflector<CleanX> ||
                         internal::has_write_reflector<CleanX>) {
      return collect_flattened_schemas<
          std::remove_cvref_t<typename Reflector<CleanX>::ReflType>>(
          _definitions);
    } else {
      using XVal = tagged_union_wrapper_no_ptr_t<CleanX>;
      using NT = internal::processed_t<XVal, ProcessorsType>;
      if constexpr (internal::has_default_val_v<XVal>) {
        static_assert(
            requires { XVal{}; },
            "A struct with rfl::DefaultVal fields must be "
            "default-constructible, because the default values come from a "
            "default-constructed instance.");
        auto defaults = XVal{};
        auto view =
            ProcessorsType::template process<XVal>(to_view(defaults));
        return {Parser<R, W, NT, ProcessorsType>::to_schema(_definitions,
                                                            &view)};
      } else {
        return {Parser<R, W, NT, ProcessorsType>::to_schema(_definitions)};
      }
    }
  }

  template <class RegNT>
  static void apply_extra_properties_from_nt(
      std::map<std::string, schema::Type>* _definitions,
      std::vector<schema::Type>* _branches) {
    if constexpr (RegNT::pos_extra_fields() != -1) {
      using F = tuple_element_t<RegNT::pos_extra_fields(),
                                typename RegNT::Fields>;
      using ExtraFieldsType = std::remove_cvref_t<typename F::Type>;
      using U = std::remove_cvref_t<typename ExtraFieldsType::Type>;
      const auto extra_schema = std::make_shared<schema::Type>(
          Parser<R, W, U, ProcessorsType>::to_schema(_definitions));
      for (auto& branch : *_branches) {
        auto& obj = rfl::get<schema::Type::Object>(branch.variant_);
        obj.additional_properties_ = extra_schema;
      }
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      for (auto& branch : *_branches) {
        auto& obj = rfl::get<schema::Type::Object>(branch.variant_);
        const auto* ptr = rfl::get_if<std::shared_ptr<schema::Type>>(
            &obj.additional_properties_);
        if (!ptr || !*ptr) {
          obj.additional_properties_ = false;
        }
      }
    }
  }

  template <internal::StringLiteral _discriminator, class... Alts>
  static std::vector<schema::Type> collect_tu_schemas(
      std::map<std::string, schema::Type>* _definitions,
      const TaggedUnion<_discriminator, Alts...>*) {
    std::vector<schema::Type> result;
    const auto append_one = [&]<class Alt>() {
      using Wrapped = flattened_tu_detail::wrapped_alt_t<_discriminator, Alt, false>;
      auto sub = collect_flattened_schemas<Wrapped>(_definitions);
      for (auto& item : sub) {
        result.emplace_back(std::move(item));
      }
    };
    (append_one.template operator()<Alts>(), ...);
    return result;
  }
};

}  // namespace rfl::parsing

#endif
