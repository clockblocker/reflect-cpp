#ifndef RFL_PARSING_VIEWREADER_HPP_
#define RFL_PARSING_VIEWREADER_HPP_

#include <array>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Tuple.hpp"
#include "../internal/StringLiteral.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/field_aliases.hpp"
#include "../internal/is_array.hpp"
#include "../internal/is_skip.hpp"
#include "../internal/no_duplicate_keys_v.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "FieldErrors.hpp"
#include "Parser_base.hpp"
#include "peel_transparent_wrappers.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "wrap_error.hpp"

namespace rfl::parsing {

/// Reads one field into an uninitialized view slot. Free function parameterized
/// only by (R, W, OriginalType, ProcessorsType)—never the enclosing ViewType—so
/// instantiations do not repeat the full NamedTuple in mangled symbol names or
/// DWARF debug info.
template <class R, class W, class OriginalType, class ProcessorsType>
RFL_ALWAYS_INLINE void assign_uninitialized_view_field(
    const R& _r, const typename R::InputVarType& _var,
    const std::string_view _name, OriginalType& _slot, bool& _is_set,
    FieldErrors<ProcessorsType>* _errors, const size_t _reserve_errors) {
  using T = std::remove_cvref_t<std::remove_pointer_t<OriginalType>>;
  using Peel = peel_transparent<T, R>;
  auto res = read_field_core<R, W, typename Peel::CoreType, ProcessorsType>(
      _r, _var, _name, _errors, _reserve_errors);
  if (!res) [[unlikely]] {
    return;
  }
  if constexpr (std::is_pointer_v<OriginalType>) {
    if constexpr (Peel::peeled) {
      auto wrapped = Peel::wrap(std::move(*res));
      placement_move_field(_slot, &wrapped);
    } else {
      placement_move_field(_slot, &(*res));
    }
  } else {
    _slot = Peel::wrap(std::move(*res));
  }
  _is_set = true;
}

template <class R, class W, class _ViewType, class ProcessorsType>
class ViewReader {
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
   * @param _found A boolean array indicating which fields have been found.
   * @param _set A boolean array indicating which fields have been successfully
   * set.
   * @param _errors The field error collector.
   */
  ViewReader(const R* _r, ViewType* _view, std::array<bool, size_>* _found,
             std::array<bool, size_>* _set, FieldErrors<ProcessorsType>* _errors)
      : r_(_r), view_(_view), found_(_found), set_(_set), errors_(_errors) {}

  ~ViewReader() = default;

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
    assign_to_matching_name(*r_, _name, _var, _key_loc, view_, errors_, found_,
                            set_, std::make_integer_sequence<int, size_>());
  }

  /**
   * @brief Records a field-level error reported directly by the reader (for
   * example, a duplicate key).
   */
  void add_error(Error&& _err) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    errors_->reserve(size_);
    errors_->emplace_back(std::move(_err));
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
    using FieldType = tuple_element_t<i, typename ViewType::Fields>;
    if (!std::get<i>(*found_)) {
      std::get<i>(*found_) = true;
      assign_uninitialized_view_field<R, W, typename FieldType::Type,
                                      ProcessorsType>(
          *r_, _var, FieldType::name(), rfl::get<i>(*view_), std::get<i>(*set_),
          errors_, size_);
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      static_assert(
          !schemaful::IsSchemafulReader<R>,
          "Passing rfl::NoExtraFields to a schemaful format does not make "
          "sense, because schemaful formats cannot have extra fields.");
      std::stringstream stream;
      stream << "Value named '" << i << "' not used.";
      errors_->emplace_back(make_nested_error(
          static_cast<size_t>(i), stream.str(), ErrorKind::unknown_field,
          get_source_location(*r_, _var)));
    }
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
                                     auto* _errors, auto* _found, auto* _set) {
    auto* extra_fields = _view->template get<_pos>();
    using ExtraFieldsType =
        std::remove_cvref_t<std::remove_pointer_t<decltype(extra_fields)>>;
    using T = std::remove_cvref_t<
        std::remove_pointer_t<typename ExtraFieldsType::Type>>;
    if (!std::get<_pos>(*_set)) {
      ::new (extra_fields) ExtraFieldsType();
      std::get<_pos>(*_set) = true;
      std::get<_pos>(*_found) = true;
    }
    auto res = Parser<R, W, T, ProcessorsType>::read(_r, _var);
    if (!res) [[unlikely]] {
      _errors->emplace_back(
          wrap_field_error(_current_name, std::move(res).error()));
      return;
    }
    extra_fields->emplace(std::string(_current_name), std::move(*res));
  }

  template <int... is>
  static void assign_by_index(const R& _r, const int _idx,
                              const InputVarType& _var, ViewType* _view,
                              FieldErrors<ProcessorsType>* _errors,
                              std::array<bool, size_>* _set,
                              std::integer_sequence<int, is...>) {
    using Fields = typename ViewType::Fields;
    static_cast<void>(
        ((_idx == is
              ? (assign_uninitialized_view_field<
                     R, W, typename tuple_element_t<is, Fields>::Type,
                     ProcessorsType>(_r, _var,
                                     tuple_element_t<is, Fields>::name(),
                                     rfl::get<is>(*_view), std::get<is>(*_set),
                                     _errors, size_),
                 true)
              : false) ||
         ...));
  }

  template <int... is>
  static void assign_to_matching_name(
      const R& _r, const std::string_view& _current_name, const auto& _var,
      const std::optional<SourceLocation>& _key_loc, ViewType* _view,
      FieldErrors<ProcessorsType>* _errors, std::array<bool, size_>* _found,
      std::array<bool, size_>* _set, std::integer_sequence<int, is...> _seq) {
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
          _errors->reserve(size_);
          _errors->emplace_back(
              make_nested_error(std::string(_current_name), stream.str()));
          return;
        }
      } else {
        (*_found)[static_cast<size_t>(idx)] = true;
        already_assigned = true;
        assign_by_index(_r, idx, _var, _view, _errors, _set, _seq);
      }
    }

    if constexpr (ViewType::pos_extra_fields() != -1) {
      static_assert(!schemaful::IsSchemafulReader<R>,
                    "rfl::ExtraFields are not supported for schemaful formats, "
                    "because schemaful formats cannot have extra fields.");
      constexpr int pos = ViewType::pos_extra_fields();
      if (!already_assigned) {
        assign_to_extra_fields<pos>(_r, _current_name, _var, _view, _errors,
                                    _found, _set);
      }
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      static_assert(
          !schemaful::IsSchemafulReader<R>,
          "Passing rfl::NoExtraFields to a schemaful format does not make "
          "sense, because schemaful formats cannot have extra fields.");
      if (!already_assigned) {
        std::stringstream stream;
        stream << "Value named '" << _current_name << "' not used.";
        _errors->emplace_back(make_nested_error(
            std::string(_current_name), stream.str(), ErrorKind::unknown_field,
            _key_loc ? _key_loc : get_source_location(_r, _var)));
      }
    }
  }

  template <int... is>
  static void assign_to_matching_index(
      const R& _r, const int _index, const auto& _var, ViewType* _view,
      FieldErrors<ProcessorsType>* _errors, std::array<bool, size_>* _found,
      std::array<bool, size_>* _set, std::integer_sequence<int, is...> _seq) {
    bool already_assigned = false;
    if (_index >= 0 && static_cast<size_t>(_index) < size_ &&
        !(*_found)[static_cast<size_t>(_index)]) {
      (*_found)[static_cast<size_t>(_index)] = true;
      already_assigned = true;
      assign_by_index(_r, _index, _var, _view, _errors, _set, _seq);
    }

    if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      static_assert(
          !schemaful::IsSchemafulReader<R>,
          "Passing rfl::NoExtraFields to a schemaful format does not make "
          "sense, because schemaful formats cannot have extra fields.");
      if (!already_assigned) {
        std::stringstream stream;
        stream << "Value named '" << _index << "' not used.";
        _errors->emplace_back(make_nested_error(
            static_cast<size_t>(_index), stream.str(), ErrorKind::unknown_field,
            get_source_location(_r, _var)));
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
};

}  // namespace rfl::parsing

#endif
