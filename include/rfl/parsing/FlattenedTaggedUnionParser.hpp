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
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../Literal.hpp"
#include "../NamedTuple.hpp"
#include "../Result.hpp"
#include "../TaggedUnion.hpp"
#include "../Tuple.hpp"
#include "../get.hpp"
#include "../internal/HasFromReflectionMethod.hpp"
#include "../internal/concat_literals.hpp"
#include "../internal/default_if_missing_v.hpp"
#include "../internal/fail_fast_v.hpp"
#include "../internal/field_aliases.hpp"
#include "../internal/has_default_val_v.hpp"
#include "../internal/has_fields.hpp"
#include "../internal/has_flattened_tagged_union.hpp"
#include "../internal/has_reflection_method_v.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/has_reflector.hpp"
#include "../internal/is_array.hpp"
#include "../internal/is_default_val_v.hpp"
#include "../internal/is_extra_fields.hpp"
#include "../internal/is_named_tuple.hpp"
#include "../internal/is_tagged_union.hpp"
#include "../internal/is_validator.hpp"
#include "../internal/make_tag.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "../internal/no_field_names_v.hpp"
#include "../internal/no_optionals_v.hpp"
#include "../internal/nth_element_t.hpp"
#include "../internal/processed_t.hpp"
#include "../internal/ptr_cast.hpp"
#include "../internal/strings/strings.hpp"
#include "../internal/tag_t.hpp"
#include "../internal/to_ptr_named_tuple.hpp"
#include "../to_view.hpp"
#include "../type_name_t.hpp"
#include "../visit.hpp"
#include "AreReaderAndWriter.hpp"
#include "FieldErrors.hpp"
#include "NamedTupleParser.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "TaggedUnionWrapper.hpp"
#include "call_destructors_where_necessary.hpp"
#include "is_required.hpp"
#include "is_tagged_union_wrapper.hpp"
#include "schema/Type.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "schemaful/IsSchemafulWriter.hpp"
#include "tagged_union_wrapper_no_ptr.hpp"
#include "to_single_error_message.hpp"
#include "wrap_error.hpp"

namespace rfl::internal {

template <StringLiteral _discriminator, class Alt, bool _as_const_pointer>
struct FlattenedTUWrappedAltType {
  using CleanAlt = std::remove_cvref_t<Alt>;
  using Type = std::conditional_t<
      contains_discriminator_v<CleanAlt, _discriminator>, CleanAlt,
      std::conditional_t<
          has_fields<CleanAlt>(),
          parsing::TaggedUnionWrapperWithFields<
              CleanAlt, tag_t<_discriminator, CleanAlt>, _discriminator,
              _as_const_pointer>,
          parsing::TaggedUnionWrapperNoFields<
              CleanAlt, tag_t<_discriminator, CleanAlt>, _discriminator,
              _as_const_pointer>>>;
};

template <StringLiteral _discriminator, class Alt, bool _as_const_pointer>
using flattened_tu_wrapped_alt_t =
    typename FlattenedTUWrappedAltType<_discriminator, Alt,
                                       _as_const_pointer>::Type;

template <StringLiteral _discriminator, class Alt>
auto flattened_tu_wrap_alt_for_write(const Alt& _alt) {
  using CleanAlt = std::remove_cvref_t<Alt>;
  if constexpr (contains_discriminator_v<CleanAlt, _discriminator>) {
    return &_alt;
  } else {
    const auto tag = make_tag<_discriminator, CleanAlt>(_alt);
    using TagType = std::remove_cvref_t<decltype(tag)>;
    if constexpr (has_fields<CleanAlt>()) {
      using WrapperType = parsing::TaggedUnionWrapperWithFields<
          CleanAlt, TagType, _discriminator, true>;
      return WrapperType{.tag = tag, .fields = &_alt};
    } else {
      using WrapperType = parsing::TaggedUnionWrapperNoFields<
          CleanAlt, TagType, _discriminator, true>;
      return WrapperType{.tag = tag, .fields = &_alt};
    }
  }
}

template <class... Ts>
struct FlattenedTUTypeList {};

template <class List1, class List2>
struct FlattenedTUConcatTypeLists;

template <class... Ts1, class... Ts2>
struct FlattenedTUConcatTypeLists<FlattenedTUTypeList<Ts1...>,
                                  FlattenedTUTypeList<Ts2...>> {
  using type = FlattenedTUTypeList<Ts1..., Ts2...>;
};

template <class TupleType>
struct FlattenedTUTuplePtrsToTypeList;

template <class... PtrTypes>
struct FlattenedTUTuplePtrsToTypeList<rfl::Tuple<PtrTypes...>> {
  using type = FlattenedTUTypeList<remove_all_ptrs_t<PtrTypes>...>;
};

template <class W>
struct FlattenedTUWrapperTraits;

template <class T, class TagType, StringLiteral _discriminator,
          bool _as_const_pointer>
struct FlattenedTUWrapperTraits<parsing::TaggedUnionWrapperNoFields<
    T, TagType, _discriminator, _as_const_pointer>> {
  using Inner = T;
  using TagFieldType = rfl::Rename<_discriminator, TagType>;
  static constexpr auto discriminator_ = _discriminator;
};

template <class T, class TagType, StringLiteral _discriminator,
          bool _as_const_pointer>
struct FlattenedTUWrapperTraits<parsing::TaggedUnionWrapperWithFields<
    T, TagType, _discriminator, _as_const_pointer>> {
  using Inner = T;
  using TagFieldType = rfl::Field<_discriminator, TagType>;
  static constexpr auto discriminator_ = _discriminator;
};

template <class T>
constexpr bool is_wrapper_over_custom_reflection_v = []() consteval {
  using U = std::remove_cvref_t<T>;
  if constexpr (is_tagged_union_wrapper_v<U>) {
    using Inner = typename FlattenedTUWrapperTraits<U>::Inner;
    return has_reflection_type_v<Inner> || has_read_reflector<Inner> ||
           has_write_reflector<Inner>;
  } else {
    return false;
  }
}();

template <class ProcessorsType, class OuterNames, class TUList>
constexpr void flattened_tu_validate_tu_list();

template <class S, class ProcessorsType, class OuterNames, class RemainingTUs>
constexpr void flattened_tu_validate_struct_with_remaining() {
  using U = remove_all_ptrs_t<S>;
  if constexpr (is_tagged_union_based<U>) {
    using NextList =
        typename FlattenedTUConcatTypeLists<FlattenedTUTypeList<U>,
                                            RemainingTUs>::type;
    flattened_tu_validate_tu_list<ProcessorsType, OuterNames, NextList>();
  } else if constexpr (is_wrapper_over_custom_reflection_v<U>) {
    using Traits = FlattenedTUWrapperTraits<U>;
    constexpr auto disc = Traits::discriminator_;
    static_assert(
        !OuterNames::template contains<disc>(),
        "Flattening rfl::TaggedUnion created duplicate field names.");
    using NewOuter =
        decltype(concat_literals(OuterNames{}, rfl::Literal<disc>{}));
    flattened_tu_validate_struct_with_remaining<
        typename Traits::Inner, ProcessorsType, NewOuter, RemainingTUs>();
  } else if constexpr (has_reflection_type_v<U>) {
    flattened_tu_validate_struct_with_remaining<
        std::remove_cvref_t<typename U::ReflectionType>, ProcessorsType,
        OuterNames, RemainingTUs>();
  } else if constexpr (has_read_reflector<U> || has_write_reflector<U>) {
    flattened_tu_validate_struct_with_remaining<
        std::remove_cvref_t<typename Reflector<U>::ReflType>, ProcessorsType,
        OuterNames, RemainingTUs>();
  } else if constexpr (is_named_tuple_v<U> ||
                       (std::is_class_v<U> && std::is_aggregate_v<U>)) {
    using RegNT = processed_t<U, ProcessorsType>;
    using RegNames = typename RegNT::Names;
    static_assert(
        !OuterNames::template contains_any<RegNames>(),
        "Flattening rfl::TaggedUnion created duplicate field names.");
    using NewOuter = decltype(concat_literals(OuterNames{}, RegNames{}));
    if constexpr (has_flattened_tagged_union_v<U>) {
      using OwnTUs =
          typename FlattenedTUTuplePtrsToTypeList<flattened_tu_ptrs_t<U>>::type;
      using NextList =
          typename FlattenedTUConcatTypeLists<OwnTUs, RemainingTUs>::type;
      flattened_tu_validate_tu_list<ProcessorsType, NewOuter, NextList>();
    } else {
      flattened_tu_validate_tu_list<ProcessorsType, NewOuter, RemainingTUs>();
    }
  } else {
    flattened_tu_validate_tu_list<ProcessorsType, OuterNames, RemainingTUs>();
  }
}

template <class ProcessorsType, class OuterNames, StringLiteral _discriminator,
          class... Alts, class TailTUList>
constexpr void flattened_tu_validate_one_tu_alts_impl(
    const TaggedUnion<_discriminator, Alts...>*, TailTUList) {
  static_assert(
      !OuterNames::template contains<_discriminator>(),
      "Duplicate discriminator or field name when flattening "
      "rfl::TaggedUnion: inner discriminator collides with an outer field or "
      "discriminator.");
  (flattened_tu_validate_struct_with_remaining<
       flattened_tu_wrapped_alt_t<_discriminator, Alts, false>, ProcessorsType,
       OuterNames, TailTUList>(),
   ...);
}

template <class ProcessorsType, class OuterNames, class TUList>
constexpr void flattened_tu_validate_tu_list() {
  if constexpr (!std::is_same_v<TUList, FlattenedTUTypeList<>>) {
    []<class HeadTU, class... TailTUs>(
        const FlattenedTUTypeList<HeadTU, TailTUs...>*) {
      using BaseTU = tagged_union_base_t<HeadTU>;
      flattened_tu_validate_one_tu_alts_impl<ProcessorsType, OuterNames>(
          static_cast<const BaseTU*>(nullptr),
          FlattenedTUTypeList<TailTUs...>{});
    }(static_cast<const TUList*>(nullptr));
  }
}

template <class NT1, class NT2>
auto flattened_tu_concat_named_tuples(NT1&& _nt1, NT2&& _nt2) {
  if constexpr (std::remove_cvref_t<NT1>::size() == 0) {
    return std::forward<NT2>(_nt2);
  } else if constexpr (std::remove_cvref_t<NT2>::size() == 0) {
    return std::forward<NT1>(_nt1);
  } else {
    return std::forward<NT1>(_nt1).add(std::forward<NT2>(_nt2));
  }
}

template <int _i, class NT>
auto flattened_tu_get_indexed_field(const NT& _nt) {
  using FieldType =
      tuple_element_t<_i, typename std::remove_cvref_t<NT>::Fields>;
  return FieldType(rfl::get<_i>(_nt));
}

template <int _i, int _end, class RegView, class AccView>
auto flattened_tu_append_prefix_fields(const RegView& _reg_view,
                                       AccView&& _acc_view) {
  if constexpr (_i >= _end) {
    return std::forward<AccView>(_acc_view);
  } else {
    auto next = std::forward<AccView>(_acc_view)
                    .add(flattened_tu_get_indexed_field<_i>(_reg_view));
    return flattened_tu_append_prefix_fields<_i + 1, _end>(_reg_view,
                                                           std::move(next));
  }
}

template <bool _val, size_t _count, bool... _bs>
consteval auto flattened_tu_append_live_bits(
    std::integer_sequence<bool, _bs...>) {
  if constexpr (_count == 0) {
    return std::integer_sequence<bool, _bs...>{};
  } else {
    return flattened_tu_append_live_bits<_val, _count - 1>(
        std::integer_sequence<bool, _bs..., _val>{});
  }
}

}  // namespace rfl::internal

namespace rfl::parsing {

template <class S, class ProcessorsType>
constexpr void validate_flattened_struct_no_collisions() {
  internal::flattened_tu_validate_struct_with_remaining<
      S, ProcessorsType, rfl::Literal<>, internal::FlattenedTUTypeList<>>();
}

template <class TU, class ProcessorsType>
constexpr void validate_tagged_union_no_collisions() {
  internal::flattened_tu_validate_tu_list<
      ProcessorsType, rfl::Literal<>,
      internal::FlattenedTUTypeList<internal::remove_all_ptrs_t<TU>>>();
}

/// View reader for flattened TaggedUnion structs where some fields belong to
/// default-constructed structs (`_live_mask[i] == true`) and others live in
/// uninitialized raw storage (`_live_mask[i] == false`).
template <class R, class W, class ViewType, class ProcessorsType,
          bool... _live_bits>
class FlattenedMixedViewReader {
 public:
  using InputVarType = typename R::InputVarType;
  static constexpr size_t size_ = ViewType::size();
  static constexpr std::array<bool, size_> live_mask_{_live_bits...};

  FlattenedMixedViewReader(const R* _r, ViewType* _view,
                           std::array<bool, size_>* _found,
                           std::array<bool, size_>* _set,
                           FieldErrors<ProcessorsType>* _errors)
      : r_(_r), view_(_view), found_(_found), set_(_set), errors_(_errors) {}

  void read(const std::string_view& _name, const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    bool already_assigned = false;
    [&]<int... _is>(std::integer_sequence<int, _is...>) {
      (assign_if_matches<_is>(_name, _var, &already_assigned), ...);
    }(std::make_integer_sequence<int, size_>());

    if constexpr (ViewType::pos_extra_fields() != -1) {
      constexpr int pos = ViewType::pos_extra_fields();
      if (!already_assigned) {
        assign_to_extra_fields<pos>(_name, _var);
      }
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      if (!already_assigned) {
        std::stringstream stream;
        stream << "Value named '" << std::string(_name)
               << "' not used. Remove the rfl::NoExtraFields processor or add "
                  "rfl::ExtraFields to avoid this error message.";
        errors_->emplace_back(
            make_nested_error(std::string(_name), stream.str()));
      }
    }
  }

  std::optional<Error> read(const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) {
      return std::nullopt;
    }
    if (pos_idx_ == static_cast<int>(size_)) {
      std::stringstream stream;
      stream << "Expected a maximum of " << std::to_string(size_)
             << " fields, but got at least one more.";
      return Error(stream.str());
    }
    [&]<int... _is>(std::integer_sequence<int, _is...>) {
      (assign_if_index<_is>(_var), ...);
    }(std::make_integer_sequence<int, size_>());
    ++pos_idx_;
    return std::nullopt;
  }

 private:
  template <class FieldType>
  static bool is_matching(const std::string_view& _current_name) {
    if (_current_name == FieldType::name()) {
      return true;
    }
    using Aliases = internal::field_aliases_t<typename FieldType::Type>;
    if constexpr (Aliases::size > 0) {
      return Aliases::contains(_current_name);
    }
    return false;
  }

  template <bool _is_live, class Target, class Source>
  static void move_to(Target* _t, Source* _s) {
    if constexpr (!internal::is_array_v<Source> && !std::is_array_v<Target>) {
      if constexpr (_is_live) {
        if constexpr (std::is_const_v<Target> ||
                      !std::is_move_assignable_v<Target>) {
          using NonConstTarget = std::remove_const_t<Target>;
          auto* const ptr = const_cast<NonConstTarget*>(_t);
          std::destroy_at(ptr);
          ::new (ptr) NonConstTarget(std::move(*_s));
        } else {
          *_t = Target(std::move(*_s));
        }
      } else {
        if constexpr (std::is_const_v<Target>) {
          move_to<false>(const_cast<std::remove_const_t<Target>*>(_t), _s);
        } else {
          ::new (_t) Target(std::move(*_s));
        }
      }
    } else if constexpr (internal::is_array_v<Source>) {
      static_assert(std::is_array_v<Target>,
                    "Expected target to be a c-array.");
      for (size_t i = 0; i < _s->arr_.size(); ++i) {
        move_to<_is_live>(&((*_t)[i]), &(_s->arr_[i]));
      }
    } else {
      for (size_t i = 0; i < _s->size(); ++i) {
        move_to<_is_live>(&((*_t)[i]), &((*_s)[i]));
      }
    }
  }

  template <int _i>
  void parse_and_store(const InputVarType& _var) const {
    using FieldType = tuple_element_t<_i, typename ViewType::Fields>;
    using OriginalType = typename FieldType::Type;
    using T = std::remove_cvref_t<std::remove_pointer_t<OriginalType>>;
    constexpr auto name = FieldType::name();
    constexpr bool is_live = live_mask_[_i];

    auto res = Parser<R, W, T, ProcessorsType>::read(*r_, _var);
    if (!res) [[unlikely]] {
      errors_->emplace_back(wrap_field_error(name, std::move(res).error()));
      return;
    }
    if constexpr (std::is_pointer_v<OriginalType>) {
      move_to<is_live>(rfl::get<_i>(*view_), &(*res));
    } else {
      rfl::get<_i>(*view_) = std::move(*res);
    }
    if constexpr (!is_live) {
      std::get<_i>(*set_) = true;
    }
  }

  template <int _i>
  void assign_if_matches(const std::string_view& _current_name,
                         const InputVarType& _var,
                         bool* _already_assigned) const {
    using FieldType = tuple_element_t<_i, typename ViewType::Fields>;
    using OriginalType = typename FieldType::Type;
    using Aliases = internal::field_aliases_t<OriginalType>;
    constexpr auto name = FieldType::name();
    if constexpr (Aliases::size == 0) {
      if (!(*_already_assigned) && !std::get<_i>(*found_) &&
          _current_name == name) {
        std::get<_i>(*found_) = true;
        *_already_assigned = true;
        parse_and_store<_i>(_var);
      }
    } else {
      if (!(*_already_assigned) && is_matching<FieldType>(_current_name)) {
        if (std::get<_i>(*found_)) [[unlikely]] {
          *_already_assigned = true;
          std::stringstream stream;
          stream << "Field named '" << name
                 << "' was specified more than once (conflicting key '"
                 << _current_name << "').";
          errors_->emplace_back(
              make_nested_error(std::string(_current_name), stream.str()));
          return;
        }
        std::get<_i>(*found_) = true;
        *_already_assigned = true;
        parse_and_store<_i>(_var);
      }
    }
  }

  template <int _i>
  void assign_if_index(const InputVarType& _var) const {
    if (pos_idx_ == _i) {
      std::get<_i>(*found_) = true;
      parse_and_store<_i>(_var);
    }
  }

  template <int _pos>
  void assign_to_extra_fields(const std::string_view& _current_name,
                              const InputVarType& _var) const {
    constexpr bool is_live = live_mask_[_pos];
    auto* extra_fields = view_->template get<_pos>();
    using ExtraFieldsType =
        std::remove_cvref_t<std::remove_pointer_t<decltype(extra_fields)>>;
    using T = std::remove_cvref_t<
        std::remove_pointer_t<typename ExtraFieldsType::Type>>;
    if constexpr (!is_live) {
      if (!std::get<_pos>(*set_)) {
        ::new (extra_fields) ExtraFieldsType();
        std::get<_pos>(*set_) = true;
        std::get<_pos>(*found_) = true;
      }
    }
    auto res = Parser<R, W, T, ProcessorsType>::read(*r_, _var);
    if (!res) [[unlikely]] {
      errors_->emplace_back(
          wrap_field_error(_current_name, std::move(res).error()));
      return;
    }
    extra_fields->emplace(std::string(_current_name), std::move(*res));
  }

  const R* r_;
  ViewType* view_;
  std::array<bool, size_>* found_;
  std::array<bool, size_>* set_;
  FieldErrors<ProcessorsType>* errors_;
  mutable int pos_idx_ = 0;
};

template <class R, class W, class T, class ProcessorsType>
  requires AreReaderAndWriter<R, W, T>
struct FlattenedTaggedUnionParser {
  using InputVarType = typename R::InputVarType;
  using ParentType = Parent<W>;

  constexpr static bool no_field_names_ =
      internal::no_field_names_v<ProcessorsType>;
  constexpr static bool all_required_ =
      internal::no_optionals_v<ProcessorsType>;
  constexpr static bool ignore_empty_containers_ = false;

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

    bool leaf_succeeded = false;
    const auto leaf_cont =
        [&]<class CombinedView, bool... LiveBits>(
            CombinedView& _combined_view,
            std::integer_sequence<bool, LiveBits...> _live_seq,
            size_t) -> std::optional<Error> {
      auto err = read_combined_view(_r, _var, *obj_or_arr, &_combined_view,
                                    _live_seq);
      if (!err) {
        leaf_succeeded = true;
      }
      return err;
    };

    constexpr bool outer_has_dval = internal::has_default_val_v<T>;
    static_assert(
        !outer_has_dval || requires { T{}; },
        "A struct with rfl::DefaultVal fields must be default-constructible, "
        "because the default values come from a default-constructed instance.");
    constexpr bool outer_is_live =
        outer_has_dval ||
        (internal::default_if_missing_v<ProcessorsType> && requires { T{}; });

    if constexpr (outer_is_live) {
      try {
        auto t = T{};
        try {
          auto err = resolve_struct_and_read<true>(
              _r, _var, *obj_or_arr, 0, t, rfl::NamedTuple<>(),
              std::integer_sequence<bool>{}, leaf_cont, &leaf_succeeded);
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
          _r, _var, *obj_or_arr, 0, *ptr, rfl::NamedTuple<>(),
          std::integer_sequence<bool>{}, leaf_cont, &leaf_succeeded);
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
    static_assert(
        !schemaful::IsSchemafulReader<R> && !schemaful::IsSchemafulWriter<W>,
        "Flattening rfl::TaggedUnion is not supported for schemaful formats.");
    validate_flattened_struct_no_collisions<T, ProcessorsType>();
    auto branches = collect_flattened_schemas<T>(_definitions);
    if (branches.size() == 1) {
      return std::move(branches[0]);
    }
    return schema::Type{schema::Type::AnyOf{.types_ = std::move(branches)}};
  }

 private:
  // =========================================================================
  // Combined-view reader with per-field liveness (supports mixed live / raw)
  // =========================================================================

  template <class CombinedView, bool... LiveBits>
  static std::optional<Error> read_combined_view(
      const R& _r, const InputVarType& _var,
      const InputObjectOrArrayType& _obj_or_arr, CombinedView* _combined_view,
      std::integer_sequence<bool, LiveBits...>) noexcept {
    constexpr bool all_live = (true && ... && LiveBits);
    constexpr bool any_live = (false || ... || LiveBits);
    if constexpr (all_live) {
      return Parser<R, W, CombinedView, ProcessorsType>::read_view_with_default(
          _r, _var, _combined_view);
    } else if constexpr (!any_live) {
      auto [set, err] =
          Parser<R, W, CombinedView, ProcessorsType>::read_view(
              _r, _var, _combined_view);
      if (err) [[unlikely]] {
        call_destructors_where_necessary(set, _combined_view);
        return std::move(err);
      }
      return std::nullopt;
    } else {
      constexpr size_t N = CombinedView::size();
      constexpr std::array<bool, N> live_mask{LiveBits...};
      static_assert(
          internal::no_alias_collisions<typename CombinedView::Fields>(),
          "Field alias collides with another field's name or alias.");

      std::array<bool, N> found{};
      found.fill(false);
      std::array<bool, N> set{};
      set.fill(false);
      FieldErrors<ProcessorsType> errors;

      using ReaderType =
          FlattenedMixedViewReader<R, W, CombinedView, ProcessorsType,
                                   LiveBits...>;
      const auto reader = ReaderType(&_r, _combined_view, &found, &set, &errors);
      std::optional<Error> io_err;
      if constexpr (no_field_names_) {
        io_err = _r.read_array(reader, _obj_or_arr);
      } else {
        io_err = _r.read_object(reader, _obj_or_arr);
      }
      if (io_err) [[unlikely]] {
        call_destructors_where_necessary(set, _combined_view);
        return io_err;
      }

      if (!should_stop_on_field_error<ProcessorsType>(errors)) {
        [&]<int... _is>(std::integer_sequence<int, _is...>) {
          (handle_mixed_missing_field<_is, live_mask[_is]>(
               found, *_combined_view, &set, &errors),
           ...);
        }(std::make_integer_sequence<int, N>());
      }

      if constexpr (internal::fail_fast_v<ProcessorsType>) {
        if (auto err = std::move(errors).error()) [[unlikely]] {
          call_destructors_where_necessary(set, _combined_view);
          return err;
        }
        return std::nullopt;
      } else {
        if (errors.size() != 0) [[unlikely]] {
          call_destructors_where_necessary(set, _combined_view);
          return to_single_error(std::move(errors));
        }
        return std::nullopt;
      }
    }
  }

  template <int _i, bool _is_live, class CombinedView>
  static void handle_mixed_missing_field(
      const std::array<bool, CombinedView::size()>& _found,
      const CombinedView& _view, std::array<bool, CombinedView::size()>* _set,
      FieldErrors<ProcessorsType>* _errors) noexcept {
    if (should_stop_on_field_error<ProcessorsType>(*_errors)) return;
    if (std::get<_i>(_found)) return;

    using FieldType = tuple_element_t<_i, typename CombinedView::Fields>;
    using ValueType = std::remove_reference_t<
        std::remove_pointer_t<typename FieldType::Type>>;
    constexpr bool is_default_val_field = internal::is_default_val_v<ValueType>;
    constexpr bool is_required_field =
        !is_default_val_field && !internal::is_extra_fields_v<ValueType> &&
        (all_required_ || is_required<ValueType, ignore_empty_containers_>());

    if constexpr (_is_live) {
      // The enclosing struct was default-constructed: its fields already hold
      // their defaults (needed for DefaultVal DMIs and DefaultIfMissing).
      if constexpr (!internal::default_if_missing_v<ProcessorsType> &&
                    is_required_field) {
        constexpr auto current_name = FieldType::name();
        std::stringstream stream;
        stream << "Field named '" << std::string(current_name)
               << "' not found.";
        _errors->emplace_back(
            make_nested_error(std::string(current_name), stream.str()));
      }
    } else {
      if constexpr (is_required_field || is_default_val_field) {
        constexpr auto current_name = FieldType::name();
        std::stringstream stream;
        stream << "Field named '" << std::string(current_name)
               << "' not found.";
        _errors->emplace_back(
            make_nested_error(std::string(current_name), stream.str()));
      } else {
        using NonConstT = std::remove_const_t<ValueType>;
        auto* const ptr = const_cast<NonConstT*>(rfl::get<_i>(_view));
        if constexpr (std::is_nothrow_default_constructible_v<NonConstT>) {
          ::new (ptr) NonConstT();
        } else {
          try {
            ::new (ptr) NonConstT();
          } catch (std::exception& e) {
            constexpr auto current_name = FieldType::name();
            _errors->emplace_back(make_nested_error(
                std::string(current_name),
                "Field named '" + std::string(current_name) +
                    "' not found, and its default value could not be "
                    "constructed: " +
                    e.what()));
            return;
          }
        }
        std::get<_i>(*_set) = true;
      }
    }
  }

  template <class RegView>
  static void destroy_all_in_view(RegView* _reg_view) noexcept {
    constexpr size_t N = RegView::size();
    std::array<bool, N> all_set{};
    all_set.fill(true);
    call_destructors_where_necessary(all_set, _reg_view);
  }

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

  template <bool _target_is_live, class TargetT, class ValueT>
  static void store_into_target(TargetT& _target, ValueT&& _val) {
    if constexpr (_target_is_live) {
      if constexpr (std::is_const_v<TargetT> ||
                    !std::is_move_assignable_v<TargetT>) {
        using NonConstT = std::remove_const_t<TargetT>;
        auto* const ptr = const_cast<NonConstT*>(&_target);
        std::destroy_at(ptr);
        ::new (ptr) NonConstT(std::forward<ValueT>(_val));
      } else {
        _target = std::forward<ValueT>(_val);
      }
    } else {
      ::new (&_target) std::remove_const_t<TargetT>(std::forward<ValueT>(_val));
    }
  }

  template <bool _target_is_live, class S, class AccView, bool... LiveBits,
            class Cont>
  static std::optional<Error> resolve_struct_and_read(
      const R& _r, const InputVarType& _var,
      const InputObjectOrArrayType& _obj_or_arr, const size_t _array_offset,
      S& _target, AccView&& _acc_view,
      std::integer_sequence<bool, LiveBits...> _live_seq, const Cont& _cont,
      bool* _leaf_succeeded) noexcept {
    using CleanS = std::remove_cvref_t<S>;
    if constexpr (internal::is_tagged_union_based<CleanS>) {
      return resolve_one_tu<_target_is_live>(
          _r, _var, _obj_or_arr, _array_offset, &_target,
          std::forward<AccView>(_acc_view), _live_seq, _cont, _leaf_succeeded);
    } else if constexpr (internal::is_wrapper_over_custom_reflection_v<
                             CleanS>) {
      using Traits = internal::FlattenedTUWrapperTraits<CleanS>;
      using TagField =
          rfl::Field<Traits::discriminator_, decltype(&_target.tag)>;
      auto tag_view = rfl::NamedTuple<TagField>(TagField(&_target.tag));
      auto next_acc = internal::flattened_tu_concat_named_tuples(
          std::forward<AccView>(_acc_view), std::move(tag_view));
      constexpr auto next_live =
          internal::flattened_tu_append_live_bits<_target_is_live, 1>(
              _live_seq);
      auto err = resolve_struct_and_read<_target_is_live>(
          _r, _var, _obj_or_arr, _array_offset + 1, _target.fields.get(),
          std::move(next_acc), next_live, _cont, _leaf_succeeded);
      if (err && *_leaf_succeeded && !_target_is_live) {
        auto own_tag_view = rfl::NamedTuple<TagField>(TagField(&_target.tag));
        destroy_all_in_view(&own_tag_view);
      }
      return err;
    } else if constexpr (internal::has_flattened_tagged_union_v<CleanS>) {
      auto reg_view = ProcessorsType::template process<CleanS>(
          internal::to_ptr_named_tuple(_target));
      auto tu_ptrs = internal::to_flattened_tu_ptrs(_target);
      constexpr size_t raw_reg = internal::raw_regular_field_count<CleanS>();
      constexpr size_t delta =
          std::remove_cvref_t<decltype(reg_view)>::size() - raw_reg;
      auto acc_with_prefix =
          internal::flattened_tu_append_prefix_fields<0, delta>(
              reg_view, std::forward<AccView>(_acc_view));
      constexpr auto live_with_prefix =
          internal::flattened_tu_append_live_bits<_target_is_live, delta>(
              _live_seq);
      auto err = resolve_slots<_target_is_live, CleanS, 0, delta>(
          _r, _var, _obj_or_arr, _array_offset + delta, reg_view, tu_ptrs,
          std::move(acc_with_prefix), live_with_prefix, _cont, _leaf_succeeded);
      if (err && *_leaf_succeeded && !_target_is_live) {
        destroy_all_in_view(&reg_view);
      }
      return err;
    } else if constexpr (internal::has_read_reflector<CleanS>) {
      using ReflType = std::remove_cvref_t<typename Reflector<CleanS>::ReflType>;
      constexpr bool refl_has_dval = internal::has_default_val_v<ReflType>;
      constexpr bool refl_is_live =
          refl_has_dval || (internal::default_if_missing_v<ProcessorsType> &&
                            requires { ReflType{}; });
      try {
        if constexpr (refl_is_live) {
          ReflType refl_val{};
          auto err = resolve_struct_and_read<true>(
              _r, _var, _obj_or_arr, _array_offset, refl_val,
              std::forward<AccView>(_acc_view), _live_seq, _cont,
              _leaf_succeeded);
          if (err) [[unlikely]] {
            return err;
          }
          store_into_target<_target_is_live>(
              _target, Reflector<CleanS>::to(std::move(refl_val)));
          return std::nullopt;
        } else {
          alignas(ReflType) unsigned char refl_buf[sizeof(ReflType)]{};
          auto* refl_ptr = internal::ptr_cast<ReflType*>(&refl_buf);
          auto err = resolve_struct_and_read<false>(
              _r, _var, _obj_or_arr, _array_offset, *refl_ptr,
              std::forward<AccView>(_acc_view), _live_seq, _cont,
              _leaf_succeeded);
          if (err) [[unlikely]] {
            return err;
          }
          struct ReflDestroyer {
            ReflType* p;
            ~ReflDestroyer() { p->~ReflType(); }
          } guard{refl_ptr};
          store_into_target<_target_is_live>(
              _target, Reflector<CleanS>::to(std::move(*refl_ptr)));
          return std::nullopt;
        }
      } catch (std::exception& e) {
        return Error(e.what());
      }
    } else if constexpr (internal::has_reflection_type_v<CleanS>) {
      using ReflectionType = std::remove_cvref_t<typename CleanS::ReflectionType>;
      constexpr bool refl_has_dval =
          internal::has_default_val_v<ReflectionType>;
      constexpr bool refl_is_live =
          refl_has_dval || (internal::default_if_missing_v<ProcessorsType> &&
                            requires { ReflectionType{}; });
      const auto wrap_in_clean = [](ReflectionType&& _rv) -> Result<CleanS> {
        try {
          if constexpr (internal::HasFromReflectionMethod<CleanS>) {
            return CleanS::from_reflection(std::move(_rv));
          } else if constexpr (internal::is_validator_v<CleanS>) {
            return CleanS::from_value(std::move(_rv));
          } else {
            return CleanS{std::move(_rv)};
          }
        } catch (std::exception& e) {
          return error(e.what());
        }
      };
      try {
        if constexpr (refl_is_live) {
          ReflectionType refl_val{};
          auto err = resolve_struct_and_read<true>(
              _r, _var, _obj_or_arr, _array_offset, refl_val,
              std::forward<AccView>(_acc_view), _live_seq, _cont,
              _leaf_succeeded);
          if (err) [[unlikely]] {
            return err;
          }
          auto wrapped = wrap_in_clean(std::move(refl_val));
          if (!wrapped) [[unlikely]] {
            return wrapped.error();
          }
          store_into_target<_target_is_live>(_target, std::move(*wrapped));
          return std::nullopt;
        } else {
          alignas(ReflectionType) unsigned char refl_buf[sizeof(ReflectionType)]{};
          auto* refl_ptr = internal::ptr_cast<ReflectionType*>(&refl_buf);
          auto err = resolve_struct_and_read<false>(
              _r, _var, _obj_or_arr, _array_offset, *refl_ptr,
              std::forward<AccView>(_acc_view), _live_seq, _cont,
              _leaf_succeeded);
          if (err) [[unlikely]] {
            return err;
          }
          struct ReflDestroyer {
            ReflectionType* p;
            ~ReflDestroyer() { p->~ReflectionType(); }
          } guard{refl_ptr};
          auto wrapped = wrap_in_clean(std::move(*refl_ptr));
          if (!wrapped) [[unlikely]] {
            return wrapped.error();
          }
          store_into_target<_target_is_live>(_target, std::move(*wrapped));
          return std::nullopt;
        }
      } catch (std::exception& e) {
        return Error(e.what());
      }
    } else {
      auto reg_view = ProcessorsType::template process<CleanS>(
          internal::to_ptr_named_tuple(_target));
      constexpr size_t reg_size =
          std::remove_cvref_t<decltype(reg_view)>::size();
      auto combined = internal::flattened_tu_concat_named_tuples(
          std::forward<AccView>(_acc_view), std::move(reg_view));
      constexpr auto next_live =
          internal::flattened_tu_append_live_bits<_target_is_live, reg_size>(
              _live_seq);
      auto err = _cont(combined, next_live, _array_offset + reg_size);
      if (err && *_leaf_succeeded && !_target_is_live) {
        auto own_view = ProcessorsType::template process<CleanS>(
            internal::to_ptr_named_tuple(_target));
        destroy_all_in_view(&own_view);
      }
      return err;
    }
  }

  template <bool _target_is_live, class CleanS, size_t _slot_i, size_t _delta,
            class RegView, class TUPtrs, class AccView, bool... LiveBits,
            class Cont>
  static std::optional<Error> resolve_slots(
      const R& _r, const InputVarType& _var,
      const InputObjectOrArrayType& _obj_or_arr, const size_t _array_offset,
      const RegView& _reg_view, TUPtrs& _tu_ptrs, AccView&& _acc_view,
      std::integer_sequence<bool, LiveBits...> _live_seq, const Cont& _cont,
      bool* _leaf_succeeded) noexcept {
    constexpr auto slots = internal::flattened_tu_slots<CleanS>();
    if constexpr (_slot_i == slots.size()) {
      auto final_view = std::forward<AccView>(_acc_view);
      return _cont(final_view, _live_seq, _array_offset);
    } else {
      constexpr auto slot = slots[_slot_i];
      if constexpr (!slot.is_tu) {
        constexpr int reg_i = static_cast<int>(_delta + slot.index);
        auto next_view =
            std::forward<AccView>(_acc_view)
                .add(internal::flattened_tu_get_indexed_field<reg_i>(
                    _reg_view));
        constexpr auto next_live =
            internal::flattened_tu_append_live_bits<_target_is_live, 1>(
                _live_seq);
        return resolve_slots<_target_is_live, CleanS, _slot_i + 1, _delta>(
            _r, _var, _obj_or_arr, _array_offset + 1, _reg_view, _tu_ptrs,
            std::move(next_view), next_live, _cont, _leaf_succeeded);
      } else {
        auto* tu_ptr = rfl::get<static_cast<int>(slot.index)>(_tu_ptrs);
        const auto next_cont = [&](auto& _view_after_tu,
                                   auto _live_after_tu,
                                   const size_t _offset_after_tu) {
          return resolve_slots<_target_is_live, CleanS, _slot_i + 1, _delta>(
              _r, _var, _obj_or_arr, _offset_after_tu, _reg_view, _tu_ptrs,
              std::move(_view_after_tu), _live_after_tu, _cont,
              _leaf_succeeded);
        };
        return resolve_one_tu<_target_is_live>(
            _r, _var, _obj_or_arr, _array_offset, tu_ptr,
            std::forward<AccView>(_acc_view), _live_seq, next_cont,
            _leaf_succeeded);
      }
    }
  }

  template <bool _tu_slot_is_live, class TUType, class AccView,
            bool... LiveBits, class Cont>
  static std::optional<Error> resolve_one_tu(
      const R& _r, const InputVarType& _var,
      const InputObjectOrArrayType& _obj_or_arr, const size_t _array_offset,
      TUType* _tu_ptr, const AccView& _acc_view,
      std::integer_sequence<bool, LiveBits...> _live_seq, const Cont& _cont,
      bool* _leaf_succeeded) noexcept {
    using BaseTU = internal::tagged_union_base_t<TUType>;
    const auto disc_res = get_discriminator<BaseTU::discriminator_>(
        _r, _obj_or_arr, _array_offset);
    if (!disc_res) [[unlikely]] {
      return disc_res.error();
    }
    return dispatch_tu_alternatives<_tu_slot_is_live>(
        _r, _var, _obj_or_arr, _array_offset, _tu_ptr, *disc_res, _acc_view,
        _live_seq, _cont, _leaf_succeeded, static_cast<const BaseTU*>(nullptr));
  }

  template <bool _tu_slot_is_live, class TUType, class AccView,
            bool... LiveBits, class Cont,
            internal::StringLiteral _discriminator, class... Alts>
  static std::optional<Error> dispatch_tu_alternatives(
      const R& _r, const InputVarType& _var,
      const InputObjectOrArrayType& _obj_or_arr, const size_t _array_offset,
      TUType* _tu_ptr, const std::string& _disc_value, const AccView& _acc_view,
      std::integer_sequence<bool, LiveBits...> _live_seq, const Cont& _cont,
      bool* _leaf_succeeded,
      const TaggedUnion<_discriminator, Alts...>*) noexcept {
    using BaseTU = TaggedUnion<_discriminator, Alts...>;
    using PossibleTags = possible_tags_t<BaseTU>;
    static_assert(!PossibleTags::has_duplicates(),
                  "Duplicate tags are not allowed inside tagged unions.");

    std::optional<Error> res;
    bool match_found = false;
    [&]<int... _is>(std::integer_sequence<int, _is...>) {
      (try_one_tu_alt<_tu_slot_is_live, _is, BaseTU>(
           _r, _var, _obj_or_arr, _array_offset, _tu_ptr, _disc_value,
           _acc_view, _live_seq, _cont, _leaf_succeeded, &res, &match_found),
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

  template <bool _tu_slot_is_live, int _i, class BaseTU, class TUType,
            class AccView, bool... LiveBits, class Cont>
  static void try_one_tu_alt(
      const R& _r, const InputVarType& _var,
      const InputObjectOrArrayType& _obj_or_arr, const size_t _array_offset,
      TUType* _tu_ptr, const std::string& _disc_value, const AccView& _acc_view,
      std::integer_sequence<bool, LiveBits...> _live_seq, const Cont& _cont,
      bool* _leaf_succeeded, std::optional<Error>* _result,
      bool* _match_found) noexcept {
    using AltType = std::remove_cvref_t<
        rfl::variant_alternative_t<_i, typename BaseTU::VariantType>>;
    constexpr auto disc = BaseTU::discriminator_;

    if (!*_match_found && internal::tag_t<disc, AltType>::contains(_disc_value)) {
      *_match_found = true;

      using WrappedAlt =
          internal::flattened_tu_wrapped_alt_t<disc, AltType, false>;

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

      // Decide default-construction strictly for THIS matched alternative so a
      // sibling alternative with DefaultVal (or DefaultIfMissing) never forces
      // default construction on a non-default-constructible or throwing alt.
      constexpr bool alt_uses_custom_refl =
          internal::has_read_reflector<AltType> ||
          internal::has_reflection_type_v<AltType>;
      constexpr bool alt_has_dval = []() consteval {
        if constexpr (alt_uses_custom_refl) {
          return false;
        } else {
          return internal::has_default_val_v<WrappedAlt>;
        }
      }();
      static_assert(
          !alt_has_dval || requires { WrappedAlt{}; },
          "A tagged union alternative with rfl::DefaultVal fields must be "
          "default-constructible, because the default values come from a "
          "default-constructed instance.");
      constexpr bool alt_is_live = []() consteval {
        if constexpr (alt_uses_custom_refl) {
          return false;
        } else {
          return alt_has_dval || (internal::default_if_missing_v<ProcessorsType> &&
                                  requires { WrappedAlt{}; });
        }
      }();

      try {
        auto acc_copy = _acc_view;
        if constexpr (alt_is_live) {
          WrappedAlt alt{};
          auto err = resolve_struct_and_read<true>(
              _r, _var, _obj_or_arr, _array_offset, alt, std::move(acc_copy),
              _live_seq, _cont, _leaf_succeeded);
          if (err) [[unlikely]] {
            *_result = embellish(std::move(*err));
            return;
          }
          store_into_target<_tu_slot_is_live>(
              *_tu_ptr,
              internal::from_tagged_union_base<TUType>(
                  BaseTU(extract_alt(alt))));
        } else {
          alignas(WrappedAlt) unsigned char alt_buf[sizeof(WrappedAlt)]{};
          auto* alt_ptr = internal::ptr_cast<WrappedAlt*>(&alt_buf);
          auto err = resolve_struct_and_read<false>(
              _r, _var, _obj_or_arr, _array_offset, *alt_ptr,
              std::move(acc_copy), _live_seq, _cont, _leaf_succeeded);
          if (err) [[unlikely]] {
            *_result = embellish(std::move(*err));
            return;
          }
          struct AltDestroyer {
            WrappedAlt* p;
            ~AltDestroyer() { p->~WrappedAlt(); }
          } guard{alt_ptr};
          store_into_target<_tu_slot_is_live>(
              *_tu_ptr,
              internal::from_tagged_union_base<TUType>(
                  BaseTU(extract_alt(*alt_ptr))));
        }
      } catch (std::exception& e) {
        *_result = embellish(Error(e.what()));
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
            const auto wrapped = internal::flattened_tu_wrap_alt_for_write<
                BaseTU::discriminator_>(_alt);
            return count_fields(wrapped);
          },
          static_cast<const BaseTU&>(_x).variant());
    } else if constexpr (internal::is_wrapper_over_custom_reflection_v<
                             CleanX>) {
      return 1 + count_fields(_x.fields.get());
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
            const auto wrapped = internal::flattened_tu_wrap_alt_for_write<
                BaseTU::discriminator_>(_alt);
            write_fields_into_object(_w, wrapped, _obj);
          },
          static_cast<const BaseTU&>(_x).variant());
    } else if constexpr (internal::is_wrapper_over_custom_reflection_v<
                             CleanX>) {
      using Traits = internal::FlattenedTUWrapperTraits<CleanX>;
      using TagField = rfl::Field<Traits::discriminator_, decltype(&_x.tag)>;
      const auto tag_nt = rfl::NamedTuple<TagField>(TagField(&_x.tag));
      Parser<R, W, rfl::NamedTuple<TagField>, ProcessorsType>::write_fields(
          _w, tag_nt, _obj);
      write_fields_into_object(_w, _x.fields.get(), _obj);
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
      if (const auto* b_ptr = rfl::get_if<std::shared_ptr<schema::Type>>(
              &b_obj.additional_properties_);
          b_ptr && *b_ptr) {
        merged.additional_properties_ = *b_ptr;
      } else if (const auto* a_ptr = rfl::get_if<std::shared_ptr<schema::Type>>(
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
    } else if constexpr (internal::is_wrapper_over_custom_reflection_v<
                             CleanX>) {
      using Traits = internal::FlattenedTUWrapperTraits<CleanX>;
      using TagField =
          rfl::Field<Traits::discriminator_, typename Traits::TagFieldType>;
      const auto tag_schema =
          Parser<R, W, rfl::NamedTuple<TagField>, ProcessorsType>::to_schema(
              _definitions);
      auto inner_branches =
          collect_flattened_schemas<typename Traits::Inner>(_definitions);
      std::vector<schema::Type> merged;
      merged.reserve(inner_branches.size());
      for (const auto& rhs : inner_branches) {
        merged.emplace_back(merge_schema_pair(tag_schema, rhs));
      }
      return merged;
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
          (
              [&]<size_t _k>() {
                constexpr auto slot = slots[_k];
                if constexpr (!slot.is_tu) {
                  add_reg_field.template operator()<delta + slot.index>();
                } else {
                  using TU = internal::remove_all_ptrs_t<
                      tuple_element_t<slot.index, TUPtrsTuple>>;
                  auto sub_branches =
                      collect_flattened_schemas<TU>(_definitions);
                  std::vector<schema::Type> next;
                  next.reserve(current.size() * sub_branches.size());
                  for (const auto& lhs : current) {
                    for (const auto& rhs : sub_branches) {
                      next.emplace_back(merge_schema_pair(lhs, rhs));
                    }
                  }
                  current = std::move(next);
                }
              }.template operator()<_si>(),
              ...);
        }(std::make_index_sequence<slots.size()>());

        if constexpr (!no_field_names_) {
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
        auto view = ProcessorsType::template process<XVal>(to_view(defaults));
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
        auto view = ProcessorsType::template process<XVal>(to_view(defaults));
        return {
            Parser<R, W, NT, ProcessorsType>::to_schema(_definitions, &view)};
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
      using Wrapped =
          internal::flattened_tu_wrapped_alt_t<_discriminator, Alt, false>;
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
