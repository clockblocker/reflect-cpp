#ifndef RFL_PARSING_VIEWREADERWITHDEFAULT_HPP_
#define RFL_PARSING_VIEWREADERWITHDEFAULT_HPP_

#include <array>
#include <memory>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Ref.hpp"
#include "../Tuple.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/is_array.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "FieldErrors.hpp"
#include "Parser_base.hpp"
#include "wrap_error.hpp"

namespace rfl::parsing {

/// Move-assigns a parsed field value into an already-constructed struct field.
/// Free function parameterized only by (Target, Source)—never ViewType—so
/// instantiations do not carry the enclosing NamedTuple in mangled symbol
/// names or DWARF debug info.
template <class Target, class Source>
void move_assign_field(Target* _t, Source* _s) {
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
    static_assert(std::is_array_v<Target>, "Expected target to be a c-array.");
    for (size_t i = 0; i < _s->arr_.size(); ++i) {
      move_assign_field(&((*_t)[i]), &(_s->arr_[i]));
    }
  } else {
    for (size_t i = 0; i < _s->size(); ++i) {
      move_assign_field(&((*_t)[i]), &((*_s)[i]));
    }
  }
}

/// Reads one field into an already-constructed view slot. Free function
/// parameterized only by (R, W, OriginalType, ProcessorsType)—never the
/// enclosing ViewType—so instantiations do not repeat the full NamedTuple in
/// mangled symbol names or DWARF debug info.
template <class R, class W, class OriginalType, class ProcessorsType>
RFL_ALWAYS_INLINE void assign_default_view_field(
    const R& _r, const typename R::InputVarType& _var,
    const std::string_view _name, OriginalType& _slot,
    FieldErrors<ProcessorsType>* _errors) {
  using T = std::remove_cvref_t<std::remove_pointer_t<OriginalType>>;
  auto res = Parser<R, W, T, ProcessorsType>::read(_r, _var);
  if (!res) [[unlikely]] {
    _errors->emplace_back(wrap_field_error(_name, std::move(res).error()));
    return;
  }
  if constexpr (std::is_pointer_v<OriginalType>) {
    move_assign_field(_slot, &(*res));
  } else {
    _slot = std::move(*res);
  }
}

/// Reads one extra field into an already-constructed ExtraFields slot.
template <class R, class W, class ExtraFieldsType, class ProcessorsType>
void assign_extra_default_field(const R& _r,
                                const std::string_view _current_name,
                                const typename R::InputVarType& _var,
                                ExtraFieldsType* _extra_fields,
                                FieldErrors<ProcessorsType>* _errors) {
  using T = std::remove_cvref_t<
      std::remove_pointer_t<typename ExtraFieldsType::Type>>;
  auto res = Parser<R, W, T, ProcessorsType>::read(_r, _var);
  if (!res) [[unlikely]] {
    _errors->emplace_back(
        wrap_field_error(_current_name, std::move(res).error()));
    return;
  }
  _extra_fields->emplace(std::string(_current_name), std::move(*res));
}

template <class R, class W, class _ViewType, class ProcessorsType>
class ViewReaderWithDefault {
 public:
  using InputVarType = typename R::InputVarType;
  using ViewType = _ViewType;

  static constexpr size_t size_ = ViewType::size();

  /**
   * @brief Constructor.
   *
   * @param _r The reader to use.
   * @param _view The view to read into.
   * @param _errors The field error collector.
   */
  ViewReaderWithDefault(const R* _r, ViewType* _view,
                        FieldErrors<ProcessorsType>* _errors)
      : r_(_r), view_(_view), errors_(_errors) {
    found_->fill(false);
  }

  ~ViewReaderWithDefault() = default;

  /**
   * @brief Returns a boolean array indicating which fields were found.
   *
   * @return The boolean array.
   */
  const std::array<bool, size_>& found() const { return *found_; }

  /**
   * @brief Reads a single field into the view.
   *
   * @param _name The name of the field.
   * @param _var The input variable to read from.
   */
  void read(const std::string_view& _name, const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    assign_to_matching_field(*r_, _name, _var, view_, errors_, found_.get(),
                             std::make_integer_sequence<int, size_>());
  }

  /**
   * @brief Returns the size of the view.
   *
   * @return The size of the view.
   */
  static constexpr size_t size() { return size_; }

 private:
  struct NameEntry {
    std::string_view name;
    int field_idx;
  };

  static constexpr auto make_sorted_name_table() {
    return []<int... _is>(std::integer_sequence<int, _is...>) {
      using Fields = typename ViewType::Fields;
      std::array<NameEntry, size_> table{
          NameEntry{tuple_element_t<_is, Fields>::name(), _is}...};
      for (size_t i = 1; i < table.size(); ++i) {
        const auto key = table[i];
        size_t j = i;
        while (j > 0 && key.name < table[j - 1].name) {
          table[j] = table[j - 1];
          --j;
        }
        table[j] = key;
      }
      return table;
    }(std::make_integer_sequence<int, size_>());
  }

  static int find_field_index(const std::string_view _name) noexcept {
    if constexpr (size_ == 0) {
      return -1;
    } else {
      static constexpr auto table = make_sorted_name_table();
      size_t lo = 0;
      size_t hi = table.size();
      while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        if (table[mid].name < _name) {
          lo = mid + 1;
        } else {
          hi = mid;
        }
      }
      if (lo < table.size() && table[lo].name == _name) {
        return table[lo].field_idx;
      }
      return -1;
    }
  }

  template <int... is>
  static void assign_to_matching_field(const R& _r,
                                       const std::string_view& _current_name,
                                       const InputVarType& _var,
                                       ViewType* _view,
                                       FieldErrors<ProcessorsType>* _errors,
                                       std::array<bool, size_>* _found,
                                       std::integer_sequence<int, is...>) {
    using Fields = typename ViewType::Fields;
    bool already_assigned = false;
    if (const int idx = find_field_index(_current_name); idx >= 0) {
      if (!(*_found)[static_cast<size_t>(idx)]) {
        (*_found)[static_cast<size_t>(idx)] = true;
        already_assigned = true;
        static_cast<void>(
            ((idx == is
                  ? (assign_default_view_field<
                         R, W, typename tuple_element_t<is, Fields>::Type,
                         ProcessorsType>(_r, _var,
                                         tuple_element_t<is, Fields>::name(),
                                         rfl::get<is>(*_view), _errors),
                     true)
                  : false) ||
             ...));
      }
    }

    if constexpr (ViewType::pos_extra_fields() != -1) {
      constexpr int pos = ViewType::pos_extra_fields();
      if (!already_assigned) {
        auto* extra_fields = _view->template get<pos>();
        using ExtraFieldsType =
            std::remove_cvref_t<std::remove_pointer_t<decltype(extra_fields)>>;
        assign_extra_default_field<R, W, ExtraFieldsType, ProcessorsType>(
            _r, _current_name, _var, extra_fields, _errors);
      }
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      if (!already_assigned) {
        std::stringstream stream;
        stream << "Value named '" << std::string(_current_name)
               << "' not used. Remove the rfl::NoExtraFields processor or add "
                  "rfl::ExtraFields to avoid this error message.";
        _errors->emplace_back(
            make_nested_error(std::string(_current_name), stream.str()));
      }
    }
  }

 private:
  /// The underlying reader.
  const R* r_;

  /// The underlying view.
  ViewType* view_;

  /// Indicates that a certain field has been found.
  rfl::Ref<std::array<bool, size_>> found_;

  /// Stores field errors according to the selected processor policy.
  FieldErrors<ProcessorsType>* errors_;
};

}  // namespace rfl::parsing

#endif
