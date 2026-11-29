#ifndef RFL_PARSING_VIEWREADER_HPP_
#define RFL_PARSING_VIEWREADER_HPP_

#include <array>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Tuple.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/cold.hpp"
#include "../internal/is_array.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "FieldErrors.hpp"
#include "Parser_base.hpp"
#include "find_field_index.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "wrap_error.hpp"

namespace rfl::internal::parsing {

/// Placement-moves a parsed field value into uninitialized struct storage.
/// Free function parameterized only by (Target, Source)—never ViewType—so
/// instantiations do not carry the enclosing NamedTuple in mangled symbol
/// names or DWARF debug info.
template <class Target, class Source>
void placement_move_field(Target* _t, Source* _s) {
  if constexpr (std::is_const_v<Target>) {
    placement_move_field(const_cast<std::remove_const_t<Target>*>(_t), _s);
  } else if constexpr (!is_array_v<Source> && !std::is_array_v<Target>) {
    ::new (_t) Target(std::move(*_s));
  } else if constexpr (is_array_v<Source>) {
    static_assert(std::is_array_v<Target>, "Expected target to be a c-array.");
    for (size_t i = 0; i < _s->arr_.size(); ++i) {
      placement_move_field(&((*_t)[i]), &(_s->arr_[i]));
    }
  } else {
    for (size_t i = 0; i < _s->size(); ++i) {
      placement_move_field(&((*_t)[i]), &((*_s)[i]));
    }
  }
}

template <class FieldErrorsType>
RFL_COLD void record_uninitialized_field_error(const std::string_view _name,
                                               Error _err,
                                               FieldErrorsType* _errors,
                                               const size_t _reserve_errors) {
  // Reserve lazily to avoid error-vector reallocations while keeping
  // successful reads allocation-free.
  _errors->reserve(_reserve_errors);
  _errors->emplace_back(rfl::parsing::wrap_field_error(_name, std::move(_err)));
}

/// Reads one field into an uninitialized view slot. Free function parameterized
/// only by (R, W, OriginalType, ProcessorsType)—never the enclosing ViewType—so
/// instantiations do not repeat the full NamedTuple in mangled symbol names or
/// DWARF debug info.
template <class R, class W, class OriginalType, class ProcessorsType>
RFL_ALWAYS_INLINE void assign_uninitialized_view_field_inline(
    const R& _r, const typename R::InputVarType& _var,
    const std::string_view _name, OriginalType& _slot, bool& _is_set,
    rfl::parsing::FieldErrors<ProcessorsType>* _errors,
    const size_t _reserve_errors) {
  using T = std::remove_cvref_t<std::remove_pointer_t<OriginalType>>;
  auto res = rfl::parsing::Parser<R, W, T, ProcessorsType>::read(_r, _var);
  if (!res) [[unlikely]] {
    record_uninitialized_field_error(_name, std::move(res).error(), _errors,
                                     _reserve_errors);
    return;
  }
  if constexpr (std::is_pointer_v<OriginalType>) {
    placement_move_field(_slot, &(*res));
  } else {
    _slot = std::move(*res);
  }
  _is_set = true;
}

template <class R, class W, class OriginalType, class ProcessorsType>
inline void assign_uninitialized_view_field(
    const R& _r, const typename R::InputVarType& _var,
    const std::string_view _name, OriginalType& _slot, bool& _is_set,
    rfl::parsing::FieldErrors<ProcessorsType>* _errors,
    const size_t _reserve_errors) {
  assign_uninitialized_view_field_inline<R, W, OriginalType, ProcessorsType>(
      _r, _var, _name, _slot, _is_set, _errors, _reserve_errors);
}

/// Reads one extra field into an uninitialized ExtraFields slot.
template <class R, class W, class ExtraFieldsType, class ProcessorsType>
void assign_extra_uninitialized_field(
    const R& _r, const std::string_view _current_name,
    const typename R::InputVarType& _var, ExtraFieldsType* _extra_fields,
    bool& _is_found, bool& _is_set,
    rfl::parsing::FieldErrors<ProcessorsType>* _errors) {
  using T = std::remove_cvref_t<
      std::remove_pointer_t<typename ExtraFieldsType::Type>>;
  if (!_is_set) {
    ::new (_extra_fields) ExtraFieldsType();
    _is_set = true;
    _is_found = true;
  }
  auto res = rfl::parsing::Parser<R, W, T, ProcessorsType>::read(_r, _var);
  if (!res) [[unlikely]] {
    _errors->emplace_back(
        rfl::parsing::wrap_field_error(_current_name, std::move(res).error()));
    return;
  }
  _extra_fields->emplace(std::string(_current_name), std::move(*res));
}

}  // namespace rfl::internal::parsing

namespace rfl::parsing {

template <class R, class W, class _ViewType, class ProcessorsType>
class ViewReader {
 public:
  using InputVarType = typename R::InputVarType;
  using ViewType = _ViewType;

  static constexpr size_t size_ = ViewType::size();

  /**
   * @brief Constructor.
   *
   * @param _r The reader to use.
   * @param _view The view to read into.
   * @param _found A boolean array indicating which fields have been found.
   * @param _set A boolean array indicating which fields have been successfully
   * set.
   * @param _errors The field error collector.
   */
  ViewReader(const R* _r, ViewType* _view, std::array<bool, size_>* _found,
             std::array<bool, size_>* _set,
             FieldErrors<ProcessorsType>* _errors)
      : r_(_r), view_(_view), found_(_found), set_(_set), errors_(_errors) {}

  ~ViewReader() = default;

  /**
   * @brief Reads a single field into the view.
   *
   * @param _name The name of the field.
   * @param _var The input variable to read from.
   */
  RFL_ALWAYS_INLINE void read(const std::string_view& _name,
                              const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    assign_to_matching_name(*r_, _name, _var, view_, errors_, found_, set_,
                            next_idx_,
                            std::make_integer_sequence<int, size_>());
  }

  /**
   * @brief Reads a single field into the view.
   *
   * @param _index The index of the field.
   * @param _var The input variable to read from.
   */
  void read(const int _index, const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    assign_to_matching_index(*r_, _index, _var, view_, errors_, found_, set_,
                             std::make_integer_sequence<int, size_>());
  }

  // Positional readers already know the field at compile time.
  template <int i>
  void read(std::integral_constant<int, i>, const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    bool already_assigned = false;
    using FieldType = tuple_element_t<i, typename ViewType::Fields>;
    if (!std::get<i>(*found_)) {
      std::get<i>(*found_) = true;
      already_assigned = true;
      internal::parsing::assign_uninitialized_view_field<
          R, W, typename FieldType::Type, ProcessorsType>(
          *r_, _var, FieldType::name(), rfl::get<i>(*view_), std::get<i>(*set_),
          errors_, size_);
    }

    if constexpr (ViewType::pos_extra_fields() != -1) {
      static_assert(!schemaful::IsSchemafulReader<R>,
                    "rfl::ExtraFields are not supported for schemaful formats, "
                    "because schemaful formats cannot have extra fields.");
      static_assert(ViewType::pos_extra_fields() == -1,
                    "rfl::ExtraFields are not supported for positional reads.");
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      static_assert(
          !schemaful::IsSchemafulReader<R>,
          "Passing rfl::NoExtraFields to a schemaful format does not make "
          "sense, because schemaful formats cannot have extra fields.");
      if (!already_assigned) {
        std::stringstream stream;
        stream << "Value named '" << i << "' not used.";
        errors_->emplace_back(
            make_nested_error(static_cast<size_t>(i), stream.str()));
      }
    }
  }

  /**
   * @brief Returns the size of the view.
   *
   * @return The size of the view.
   */
  static constexpr size_t size() { return size_; }

 private:
  template <int... is>
  static RFL_ALWAYS_INLINE void assign_by_index(
      const R& _r, const int _idx, const InputVarType& _var, ViewType* _view,
      FieldErrors<ProcessorsType>* _errors, std::array<bool, size_>* _set,
      std::integer_sequence<int, is...>) {
    using Fields = typename ViewType::Fields;
    if constexpr (size_ <= 4) {
      static_cast<void>(
          ((_idx == is
                ? (internal::parsing::assign_uninitialized_view_field_inline<
                       R, W, typename tuple_element_t<is, Fields>::Type,
                       ProcessorsType>(_r, _var,
                                       tuple_element_t<is, Fields>::name(),
                                       rfl::get<is>(*_view),
                                       std::get<is>(*_set), _errors, size_),
                   true)
                : false) ||
           ...));
    } else {
      static_cast<void>(
          ((_idx == is ? (internal::parsing::assign_uninitialized_view_field<
                              R, W, typename tuple_element_t<is, Fields>::Type,
                              ProcessorsType>(
                              _r, _var, tuple_element_t<is, Fields>::name(),
                              rfl::get<is>(*_view), std::get<is>(*_set),
                              _errors, size_),
                          true)
                       : false) ||
           ...));
    }
  }

  template <int... is>
  static RFL_ALWAYS_INLINE void assign_to_matching_name(
      const R& _r, const std::string_view _current_name,
      const InputVarType& _var, ViewType* _view,
      FieldErrors<ProcessorsType>* _errors, std::array<bool, size_>* _found,
      std::array<bool, size_>* _set, size_t& _next_idx,
      std::integer_sequence<int, is...> _seq) {
    bool already_assigned = false;
    if constexpr (size_ > 0) {
      if (const int idx = internal::parsing::FieldNameLookup<
              typename ViewType::Names>::find_with_hint(_current_name,
                                                        _next_idx);
          idx >= 0) {
        if (!(*_found)[static_cast<size_t>(idx)]) {
          (*_found)[static_cast<size_t>(idx)] = true;
          already_assigned = true;
          assign_by_index(_r, idx, _var, _view, _errors, _set, _seq);
        }
      }
    }

    if constexpr (ViewType::pos_extra_fields() != -1) {
      static_assert(!schemaful::IsSchemafulReader<R>,
                    "rfl::ExtraFields are not supported for schemaful formats, "
                    "because schemaful formats cannot have extra fields.");
      constexpr int pos = ViewType::pos_extra_fields();
      if (!already_assigned) {
        auto* extra_fields = _view->template get<pos>();
        using ExtraFieldsType =
            std::remove_cvref_t<std::remove_pointer_t<decltype(extra_fields)>>;
        internal::parsing::assign_extra_uninitialized_field<
            R, W, ExtraFieldsType, ProcessorsType>(
            _r, _current_name, _var, extra_fields, std::get<pos>(*_found),
            std::get<pos>(*_set), _errors);
      }
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      static_assert(
          !schemaful::IsSchemafulReader<R>,
          "Passing rfl::NoExtraFields to a schemaful format does not make "
          "sense, because schemaful formats cannot have extra fields.");
      if (!already_assigned) {
        std::stringstream stream;
        stream << "Value named '" << _current_name << "' not used.";
        _errors->emplace_back(
            make_nested_error(std::string(_current_name), stream.str()));
      }
    }
  }

  template <int... is>
  static void assign_to_matching_index(
      const R& _r, const int _index, const InputVarType& _var, ViewType* _view,
      FieldErrors<ProcessorsType>* _errors, std::array<bool, size_>* _found,
      std::array<bool, size_>* _set, std::integer_sequence<int, is...> _seq) {
    bool already_assigned = false;
    if constexpr (size_ > 0) {
      if (_index >= 0 && static_cast<size_t>(_index) < size_ &&
          !(*_found)[static_cast<size_t>(_index)]) {
        (*_found)[static_cast<size_t>(_index)] = true;
        already_assigned = true;
        assign_by_index(_r, _index, _var, _view, _errors, _set, _seq);
      }
    }

    if constexpr (ViewType::pos_extra_fields() != -1) {
      static_assert(!schemaful::IsSchemafulReader<R>,
                    "rfl::ExtraFields are not supported for schemaful formats, "
                    "because schemaful formats cannot have extra fields.");
      static_assert(ViewType::pos_extra_fields() == -1,
                    "rfl::ExtraFields are not supported for positional reads.");
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      static_assert(
          !schemaful::IsSchemafulReader<R>,
          "Passing rfl::NoExtraFields to a schemaful format does not make "
          "sense, because schemaful formats cannot have extra fields.");
      if (!already_assigned) {
        std::stringstream stream;
        stream << "Value named '" << _index << "' not used.";
        _errors->emplace_back(
            make_nested_error(static_cast<size_t>(_index), stream.str()));
      }
    }
  }

 private:
  /// The underlying reader.
  const R* r_;

  /// The underlying view.
  ViewType* view_;

  /// Indicates that a certain field has been found.
  std::array<bool, size_>* found_;

  /// Indicates that a certain field has been successfully set - necessary,
  /// because we have to trigger the destructors manually.
  std::array<bool, size_>* set_;

  /// Stores field errors according to the selected processor policy.
  FieldErrors<ProcessorsType>* errors_;

  /// Sequential-key cursor hint for O(1) lookup when keys arrive in
  /// declaration order.
  mutable size_t next_idx_ = 0;
};

}  // namespace rfl::parsing

#endif
