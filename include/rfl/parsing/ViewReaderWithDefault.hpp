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
#include "../internal/always_inline.hpp"
#include "../internal/is_array.hpp"
#include "../internal/no_duplicate_keys_v.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "FieldErrors.hpp"
#include "Parser_base.hpp"
#include "peel_transparent_wrappers.hpp"
#include "wrap_error.hpp"

namespace rfl::parsing {

template <class R, class W, class _ViewType, class ProcessorsType>
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
  static constexpr auto make_sorted_name_table() {
    return []<int... _is>(std::integer_sequence<int, _is...>) {
      std::array<std::pair<std::string_view, int>, sizeof...(_is)> table{
          std::pair<std::string_view, int>{
              tuple_element_t<_is, typename ViewType::Fields>::name(), _is}...};
      for (size_t i = 1; i < table.size(); ++i) {
        const auto key = table[i];
        size_t j = i;
        while (j > 0 && key.first < table[j - 1].first) {
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
        if (table[mid].first < _name) {
          lo = mid + 1;
        } else {
          hi = mid;
        }
      }
      if (lo < table.size() && table[lo].first == _name) {
        return table[lo].second;
      }
      return -1;
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
    auto res = Parser<R, W, T, ProcessorsType>::read(_r, _var);
    if (!res) {
      _errors->emplace_back(
          wrap_field_error(_current_name, std::move(res).error()));
      return;
    }
    extra_fields->emplace(std::string(_current_name), std::move(*res));
  }

  template <int _i>
  static RFL_ALWAYS_INLINE void assign_one_field(
      const R& _r, const auto& _var, ViewType* _view,
      FieldErrors<ProcessorsType>* _errors) {
    using FieldType = tuple_element_t<_i, typename ViewType::Fields>;
    using OriginalType = typename FieldType::Type;
    using T = std::remove_cvref_t<std::remove_pointer_t<OriginalType>>;
    using Peel = peel_transparent<T, R>;
    auto res = read_field_core<R, W, typename Peel::CoreType, ProcessorsType>(
        _r, _var, FieldType::name(), _errors);
    if (!res) [[unlikely]] {
      return;
    }
    // Parser::check_deferred_defaults relies on the field being replaced as
    // a whole.
    if constexpr (std::is_pointer_v<OriginalType>) {
      if constexpr (Peel::peeled) {
        auto wrapped = Peel::wrap(std::move(*res));
        move_to(rfl::get<_i>(*_view), &wrapped);
      } else {
        move_to(rfl::get<_i>(*_view), &(*res));
      }
    } else {
      rfl::get<_i>(*_view) = Peel::wrap(std::move(*res));
    }
  }

  template <int... is>
  static void assign_to_matching_field(
      const R& _r, const std::string_view& _current_name, const auto& _var,
      const std::optional<SourceLocation>& _key_loc, auto* _view, auto* _errors,
      auto* _found, std::integer_sequence<int, is...>) {
    bool already_assigned = false;
    const int idx = find_field_index(_current_name);
    if (idx >= 0 && !(*_found)[static_cast<size_t>(idx)]) {
      (*_found)[static_cast<size_t>(idx)] = true;
      already_assigned = true;
      static_cast<void>(
          ((idx == is ? (assign_one_field<is>(_r, _var, _view, _errors), true)
                      : false) ||
           ...));
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
            std::string(_current_name), stream.str(),
            _key_loc ? _key_loc : get_source_location(_r, _var)));
      }
    }
  }

  template <class Target, class Source>
  static void move_to(Target* _t, Source* _s) {
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
        move_to(&((*_t)[i]), &(_s->arr_[i]));
      }
    } else {
      for (size_t i = 0; i < _s->size(); ++i) {
        move_to(&((*_t)[i]), &((*_s)[i]));
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
