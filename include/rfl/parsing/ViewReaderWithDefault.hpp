#ifndef RFL_PARSING_VIEWREADERWITHDEFAULT_HPP_
#define RFL_PARSING_VIEWREADERWITHDEFAULT_HPP_

#include <array>
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Ref.hpp"
#include "../Tuple.hpp"
#include "../internal/StringLiteral.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/default_if_null_v.hpp"
#include "../internal/field_aliases.hpp"
#include "../internal/is_array.hpp"
#include "../internal/is_skip.hpp"
#include "../internal/no_duplicate_keys_v.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "FieldErrors.hpp"
#include "Parser_base.hpp"
#include "is_nullable.hpp"
#include "peel_transparent_wrappers.hpp"
#include "wrap_error.hpp"

namespace rfl::parsing {

/// Reads one field into an already-constructed view slot. Free function
/// parameterized only by (R, W, OriginalType, ProcessorsType, _overlay)—never
/// the enclosing ViewType—so instantiations do not repeat the full NamedTuple
/// in mangled symbol names or DWARF debug info.
template <class R, class W, class OriginalType, class ProcessorsType,
          bool _overlay>
RFL_ALWAYS_INLINE void assign_default_view_field(
    const R& _r, const typename R::InputVarType& _var,
    const std::string_view _name, OriginalType& _slot,
    FieldErrors<ProcessorsType>* _errors) {
  using T = std::remove_cvref_t<std::remove_pointer_t<OriginalType>>;
  if constexpr (internal::default_if_null_v<ProcessorsType> &&
                !is_nullable_v<T>) {
    if (_r.is_empty(_var)) {
      return;
    }
  }
  if constexpr (_overlay) {
    if constexpr (std::is_pointer_v<OriginalType>) {
      using Target = std::remove_pointer_t<OriginalType>;
      if constexpr (std::is_array_v<Target>) {
        auto res = Parser<R, W, T, ProcessorsType>::read(_r, _var);
        if (!res) [[unlikely]] {
          _errors->emplace_back(
              wrap_field_error(_name, std::move(res).error()));
          return;
        }
        move_assign_field(_slot, &(*res));
      } else if constexpr (std::is_const_v<Target>) {
        auto res = [&]() {
          if constexpr (std::is_copy_constructible_v<T>) {
            return read_into_or_read<R, W, T, ProcessorsType>(_r, _var,
                                                              T(*_slot));
          } else {
            return Parser<R, W, T, ProcessorsType>::read(_r, _var);
          }
        }();
        if (!res) [[unlikely]] {
          _errors->emplace_back(
              wrap_field_error(_name, std::move(res).error()));
          return;
        }
        move_assign_field(_slot, &(*res));
      } else {
        auto res = read_into_or_read<R, W, T, ProcessorsType>(
            _r, _var, std::move(*_slot));
        if (!res) [[unlikely]] {
          _errors->emplace_back(
              wrap_field_error(_name, std::move(res).error()));
          return;
        }
        move_assign_field(_slot, &(*res));
      }
    } else {
      auto res = read_into_or_read<R, W, T, ProcessorsType>(_r, _var,
                                                            std::move(_slot));
      if (!res) [[unlikely]] {
        _errors->emplace_back(wrap_field_error(_name, std::move(res).error()));
        return;
      }
      _slot = std::move(*res);
    }
  } else {
    using Peel = peel_transparent<T, R>;
    auto res = read_field_core<R, W, typename Peel::CoreType, ProcessorsType>(
        _r, _var, _name, _errors);
    if (!res) [[unlikely]] {
      return;
    }
    // Parser::check_deferred_defaults relies on DefaultVal fields being
    // replaced as a whole. For pure non-DefaultVal transparent wrappers on
    // mutable fields, assign directly to the inner CoreType without
    // instantiating per-wrapper move helpers.
    if constexpr (std::is_pointer_v<OriginalType>) {
      using TargetType = std::remove_pointer_t<OriginalType>;
      if constexpr (Peel::peeled && !Peel::has_default_val &&
                    !std::is_const_v<TargetType> &&
                    !std::is_array_v<TargetType> &&
                    !rfl::internal::is_array_v<typename Peel::CoreType>) {
        Peel::unwrap_ref(*_slot) = std::move(*res);
      } else if constexpr (Peel::peeled) {
        auto wrapped = Peel::wrap(std::move(*res));
        move_assign_field(_slot, &wrapped);
      } else {
        move_assign_field(_slot, &(*res));
      }
    } else {
      _slot = Peel::wrap(std::move(*res));
    }
  }
}

template <class R, class W, class _ViewType, class ProcessorsType,
          bool _overlay = false>
class ViewReaderWithDefault {
 public:
  using InputVarType = typename R::InputVarType;
  using ViewType = _ViewType;

  static constexpr bool no_duplicate_keys =
      internal::no_duplicate_keys_v<ProcessorsType>;
  static_assert(!no_duplicate_keys ||
                    internal::reader_supports_no_duplicate_keys_v<R>,
                "rfl::NoDuplicateKeys is not supported by this reader.");
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
    read(_name, _var, std::nullopt);
  }

  /**
   * @brief Reads a single field into the view, with an optional source
   * location for the key.
   *
   * @param _name The name of the field.
   * @param _var The input variable to read from.
   * @param _key_loc The source location of the key, if available.
   */
  void read(const std::string_view& _name, const InputVarType& _var,
            const std::optional<SourceLocation>& _key_loc) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    assign_to_matching_field(*r_, _name, _var, _key_loc, view_, errors_,
                             found_.get(),
                             std::make_integer_sequence<int, size_>());
  }

  /**
   * @brief Records a field-level error reported directly by the reader (for
   * example, a duplicate key).
   */
  void add_error(Error&& _err) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    errors_->emplace_back(std::move(_err));
  }

  /**
   * @brief Returns the size of the view.
   *
   * @return The size of the view.
   */
  static constexpr size_t size() { return size_; }

 private:
  template <internal::StringLiteral _name>
  static constexpr bool has_deserialized_field_with_name() {
    using Fields = typename ViewType::Fields;
    return []<int... _is>(std::integer_sequence<int, _is...>) {
      return (false || ... ||
              (tuple_element_t<_is, Fields>::name_ == _name &&
               !internal::skip_deserialization_v<
                   typename tuple_element_t<_is, Fields>::Type>));
    }(std::make_integer_sequence<int, size_>());
  }

  template <int _i>
  static constexpr bool is_field_shadowed_by_deserialized() {
    using FieldType = tuple_element_t<_i, typename ViewType::Fields>;
    using T =
        std::remove_cvref_t<std::remove_pointer_t<typename FieldType::Type>>;
    return internal::skip_deserialization_v<T> &&
           has_deserialized_field_with_name<FieldType::name_>();
  }

  template <int _i>
  static constexpr size_t field_name_count() {
    if constexpr (is_field_shadowed_by_deserialized<_i>()) {
      return 0;
    } else {
      using FieldType = tuple_element_t<_i, typename ViewType::Fields>;
      return 1 + internal::field_aliases_t<typename FieldType::Type>::size;
    }
  }

  static constexpr size_t total_name_count() {
    return []<int... _is>(std::integer_sequence<int, _is...>) {
      return (size_t{0} + ... + field_name_count<_is>());
    }(std::make_integer_sequence<int, size_>());
  }

  struct NameEntry {
    std::string_view name;
    std::string_view primary_name;
    int field_idx;
    bool has_aliases;
  };

  static constexpr auto make_sorted_name_table() {
    return []<int... _is>(std::integer_sequence<int, _is...>) {
      std::array<NameEntry, total_name_count()> table{};
      size_t pos = 0;
      auto add_field = [&]<int _idx>() {
        if constexpr (!is_field_shadowed_by_deserialized<_idx>()) {
          using FieldType = tuple_element_t<_idx, typename ViewType::Fields>;
          using Aliases = internal::field_aliases_t<typename FieldType::Type>;
          constexpr auto primary = FieldType::name();
          constexpr bool has_aliases = Aliases::size > 0;
          table[pos++] = NameEntry{primary, primary, _idx, has_aliases};
          if constexpr (has_aliases) {
            for (const auto alias : Aliases::to_array()) {
              table[pos++] = NameEntry{alias, primary, _idx, true};
            }
          }
        }
      };
      (add_field.template operator()<_is>(), ...);
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

  static const NameEntry* find_field_entry(
      const std::string_view _name) noexcept {
    if constexpr (total_name_count() == 0) {
      return nullptr;
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
        return &table[lo];
      }
      return nullptr;
    }
  }

  template <int _pos>
  static void assign_to_extra_fields(const R& _r,
                                     const std::string_view& _current_name,
                                     const auto& _var, auto* _view,
                                     auto* _errors) {
    auto* extra_fields = _view->template get<_pos>();
    using ExtraFieldsType =
        std::remove_cvref_t<std::remove_pointer_t<decltype(extra_fields)>>;
    using T = std::remove_cvref_t<
        std::remove_pointer_t<typename ExtraFieldsType::Type>>;
    if constexpr (_overlay) {
      for (auto& [k, v] : *extra_fields) {
        if (k == _current_name) {
          auto res = read_into_or_read<R, W, T, ProcessorsType>(_r, _var,
                                                                std::move(v));
          if (!res) {
            _errors->emplace_back(
                wrap_field_error(_current_name, std::move(res).error()));
            return;
          }
          v = std::move(*res);
          return;
        }
      }
    }
    auto res = Parser<R, W, T, ProcessorsType>::read(_r, _var);
    if (!res) {
      _errors->emplace_back(
          wrap_field_error(_current_name, std::move(res).error()));
      return;
    }
    extra_fields->emplace(std::string(_current_name), std::move(*res));
  }

  template <int... is>
  static void assign_to_matching_field(
      const R& _r, const std::string_view& _current_name, const auto& _var,
      const std::optional<SourceLocation>& _key_loc, auto* _view, auto* _errors,
      auto* _found, std::integer_sequence<int, is...>) {
    using Fields = typename ViewType::Fields;
    bool already_assigned = false;
    if (const auto* entry = find_field_entry(_current_name)) {
      const int idx = entry->field_idx;
      if ((*_found)[static_cast<size_t>(idx)]) {
        if (entry->has_aliases) [[unlikely]] {
          already_assigned = true;
          std::stringstream stream;
          stream << "Field named '" << entry->primary_name
                 << "' was specified more than once (conflicting key '"
                 << _current_name << "').";
          _errors->emplace_back(
              make_nested_error(std::string(_current_name), stream.str()));
          return;
        }
      } else {
        (*_found)[static_cast<size_t>(idx)] = true;
        already_assigned = true;
        static_cast<void>(
            ((idx == is
                  ? (assign_default_view_field<
                         R, W, typename tuple_element_t<is, Fields>::Type,
                         ProcessorsType, _overlay>(
                         _r, _var, tuple_element_t<is, Fields>::name(),
                         rfl::get<is>(*_view), _errors),
                     true)
                  : false) ||
             ...));
      }
    }

    if constexpr (ViewType::pos_extra_fields() != -1) {
      constexpr int pos = ViewType::pos_extra_fields();
      if (!already_assigned) {
        assign_to_extra_fields<pos>(_r, _current_name, _var, _view, _errors);
      }
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      if (!already_assigned) {
        std::stringstream stream;
        stream << "Value named '" << std::string(_current_name)
               << "' not used. Remove the rfl::NoExtraFields processor or add "
                  "rfl::ExtraFields to avoid this error message.";
        _errors->emplace_back(make_nested_error(
            std::string(_current_name), stream.str(), ErrorKind::unknown_field,
            _key_loc ? _key_loc : get_source_location(_r, _var)));
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
