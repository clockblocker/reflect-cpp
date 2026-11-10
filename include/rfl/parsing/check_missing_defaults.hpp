#ifndef RFL_PARSING_CHECKMISSINGDEFAULTS_HPP_
#define RFL_PARSING_CHECKMISSINGDEFAULTS_HPP_

#include <array>
#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "../Binary.hpp"
#include "../Generic.hpp"
#include "../Hex.hpp"
#include "../Oct.hpp"
#include "../Result.hpp"
#include "../TaggedUnion.hpp"
#include "../Timestamp.hpp"
#include "../Tuple.hpp"
#include "../Variant.hpp"
#include "../apply.hpp"
#include "../internal/DeferredValidation.hpp"
#include "../internal/cold.hpp"
#include "../internal/fail_fast_v.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/has_reflector.hpp"
#include "../internal/is_attribute.hpp"
#include "../internal/is_basic_type.hpp"
#include "../internal/is_bitset.hpp"
#include "../internal/is_deprecated.hpp"
#include "../internal/is_description.hpp"
#include "../internal/is_flatten_field.hpp"
#include "../internal/is_literal.hpp"
#include "../internal/is_validator.hpp"
#include "../internal/processed_t.hpp"
#include "FieldErrors.hpp"
#include "ParserArray.hpp"
#include "ParserAtomic.hpp"
#include "ParserBox.hpp"
#include "ParserBytestring.hpp"
#include "ParserCArray.hpp"
#include "ParserCommented.hpp"
#include "ParserDefaultVal.hpp"
#include "ParserDuration.hpp"
#include "ParserExpected.hpp"
#include "ParserFilepath.hpp"
#include "ParserOptional.hpp"
#include "ParserPair.hpp"
#include "ParserPositional.hpp"
#include "ParserRef.hpp"
#include "ParserReferenceWrapper.hpp"
#include "ParserRename.hpp"
#include "ParserResult.hpp"
#include "ParserRflArray.hpp"
#include "ParserRflTuple.hpp"
#include "ParserRflVariant.hpp"
#include "ParserSharedPtr.hpp"
#include "ParserShort.hpp"
#include "ParserSkip.hpp"
#include "ParserSpan.hpp"
#include "ParserStringView.hpp"
#include "ParserTaggedUnion.hpp"
#include "ParserTimePoint.hpp"
#include "ParserTuple.hpp"
#include "ParserUniquePtr.hpp"
#include "ParserVariant.hpp"
#include "ParserVectorstring.hpp"
#include "ParserWString.hpp"
#include "Parser_base.hpp"
#include "is_map_like.hpp"
#include "is_string_map.hpp"
#include "is_vector_like.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "to_single_error_message.hpp"

namespace rfl::parsing {

/// Checks the default values of the fields that the input did not contain,
/// after their struct was default-constructed with the check of invalid
/// default values deferred (see internal::DeferredValidation).
namespace missing_defaults {

/// How the check treats a type. It follows Parser::read: only a type that
/// the default Parser reads as a struct is decomposed into its fields.
enum class Kind {
  /// Cannot hold a Validator.
  leaf,
  validator,
  /// Standard containers, std::array, C arrays.
  range,
  /// Maps that are read as objects, rfl::Object, rfl::ExtraFields.
  string_map,
  /// std::optional, std::unique_ptr, rfl::Box.
  owned,
  /// std::variant, rfl::Variant, rfl::TaggedUnion.
  variant,
  /// std::tuple, std::pair, rfl::Tuple.
  tuple,
  /// The library's wrappers, such as rfl::DefaultVal or rfl::Rename.
  wrapper,
  /// rfl::Result, std::expected.
  result,
  /// A struct that the default Parser reads field by field.
  structure,
  /// A class that is read some other way, such as through a custom
  /// reflection type, a Reflector or a Parser specialization. The check
  /// cannot look into it.
  opaque
};

template <class T>
struct is_timestamp : std::false_type {};

template <internal::StringLiteral _format>
struct is_timestamp<Timestamp<_format>> : std::true_type {};

/// Binary, Hex and Oct hold an integral value.
template <class T>
struct is_integral_wrapper : std::false_type {};

template <class T>
struct is_integral_wrapper<Binary<T>> : std::true_type {};

template <class T>
struct is_integral_wrapper<Hex<T>> : std::true_type {};

template <class T>
struct is_integral_wrapper<Oct<T>> : std::true_type {};

template <class... Ts>
struct TypeList {};

template <class T>
struct alternatives;

template <class... Ts>
struct alternatives<std::variant<Ts...>> {
  using type = TypeList<Ts...>;
};

template <class... Ts>
struct alternatives<rfl::Variant<Ts...>> {
  using type = TypeList<Ts...>;
};

template <internal::StringLiteral _discriminator, class... Ts>
struct alternatives<TaggedUnion<_discriminator, Ts...>> {
  using type = TypeList<Ts...>;
};

template <class... Ts>
struct alternatives<std::tuple<Ts...>> {
  using type = TypeList<Ts...>;
};

template <class A, class B>
struct alternatives<std::pair<A, B>> {
  using type = TypeList<A, B>;
};

template <class... Ts>
struct alternatives<rfl::Tuple<Ts...>> {
  using type = TypeList<Ts...>;
};

/// The marker that only the primary Parser template defines.
template <class R, class W, class T, class P>
concept read_by_default_parser = requires {
  typename Parser<R, W, T, P>::DefaultParserFor;
} && std::is_same_v<typename Parser<R, W, T, P>::DefaultParserFor, T>;

template <class R, class W, class P, class T>
consteval Kind kind_of() {
  if constexpr (internal::is_validator_v<T>) {
    return Kind::validator;
  } else if constexpr (internal::is_basic_type_v<T> || is_bytestring_v<T> ||
                       is_vectorstring_v<T> || std::is_enum_v<T> ||
                       std::is_pointer_v<T> || is_duration_v<T> ||
                       is_filepath_v<T> || is_span_v<T> || is_time_point_v<T> ||
                       is_wstring_v<T> || is_string_view_v<T> ||
                       is_reference_wrapper_v<T> || internal::is_literal_v<T> ||
                       std::is_same_v<T, Generic> || is_timestamp<T>::value ||
                       is_integral_wrapper<T>::value ||
                       internal::is_bitset_v<T> || is_atomic_v<T> ||
                       std::is_empty_v<T>) {
    return Kind::leaf;
  } else if constexpr (is_shared_ptr_v<T> || is_ref_v<T>) {
    // The object may be shared with other code, which the check must not
    // touch. That also rules out cycles.
    return Kind::leaf;
  } else if constexpr (is_string_map_v<T>) {
    return Kind::string_map;
  } else if constexpr (is_vector_like_v<T> || is_array_v<T> ||
                       std::is_array_v<T> || is_rfl_array_v<T>) {
    return Kind::range;
  } else if constexpr (is_optional_v<T> || is_unique_ptr_v<T> || is_box_v<T>) {
    return Kind::owned;
  } else if constexpr (is_variant_v<T> || is_rfl_variant_v<T> ||
                       is_tagged_union_v<T>) {
    return Kind::variant;
  } else if constexpr (is_tuple_v<T> || is_pair_v<T> || is_rfl_tuple_v<T>) {
    return Kind::tuple;
  } else if constexpr (is_default_val_v<T> || is_rename_v<T> || is_skip_v<T> ||
                       is_short_v<T> || is_positional_v<T> ||
                       is_commented_v<T> || internal::is_description_v<T> ||
                       internal::is_attribute_v<T> ||
                       internal::is_deprecated_v<T> ||
                       internal::is_flatten_field_v<T>) {
    return Kind::wrapper;
  } else if constexpr (is_result_v<T> || is_expected_v<T>) {
    return Kind::result;
  } else if constexpr (internal::has_read_reflector<T> ||
                       R::template has_custom_constructor<T> ||
                       internal::has_reflection_type_v<T>) {
    return Kind::opaque;
  } else if constexpr (std::is_class_v<T> &&
                       read_by_default_parser<R, W, T, P>) {
    return Kind::structure;
  } else {
    return Kind::opaque;
  }
}

/// The value inside a wrapper.
template <class U>
decltype(auto) wrapped(U& _value) {
  if constexpr (requires { _value.value(); }) {
    return _value.value();
  } else {
    return _value.get();
  }
}

/// The elements of a range.
template <class U>
auto& elements(U& _range) {
  if constexpr (is_rfl_array_v<std::remove_cv_t<U>>) {
    return _range.arr_;
  } else {
    return _range;
  }
}

/// The element type of a range. Its value_type, because dereferencing an
/// iterator can give a proxy, as for std::vector<bool>.
template <class T>
struct element {
  using type = std::remove_cvref_t<decltype(*std::begin(std::declval<T&>()))>;
};

template <class T>
  requires requires { typename T::value_type; }
struct element<T> {
  using type = typename T::value_type;
};

template <class T>
using element_t = typename element<
    std::remove_cvref_t<decltype(elements(std::declval<T&>()))>>::type;

template <class T>
using mapped_t =
    std::remove_cvref_t<decltype(std::begin(std::declval<T&>())->second)>;

template <class T>
using pointee_t = std::remove_cvref_t<decltype(*std::declval<T&>())>;

template <class T, class... Visited>
constexpr bool visited_v = (std::is_same_v<T, Visited> || ...);

/// Whether a T may hold an rfl::Validator, as far as the check can tell. A
/// type that the check cannot look into may. Only types reached through a
/// container or pointer are tracked in Visited, because only they can recurse.
template <class R, class W, class P, class T, class... Visited>
consteval bool may_contain_validator() {
  if constexpr (visited_v<T, Visited...>) {
    return false;
  } else {
    constexpr Kind kind = kind_of<R, W, P, T>();
    if constexpr (kind == Kind::leaf) {
      return false;
    } else if constexpr (kind == Kind::validator || kind == Kind::opaque) {
      return true;
    } else if constexpr (kind == Kind::range) {
      return may_contain_validator<R, W, P, element_t<T>, T, Visited...>();
    } else if constexpr (kind == Kind::string_map) {
      return may_contain_validator<R, W, P, mapped_t<T>, T, Visited...>();
    } else if constexpr (kind == Kind::owned) {
      return may_contain_validator<R, W, P, pointee_t<T>, T, Visited...>();
    } else if constexpr (kind == Kind::result) {
      return may_contain_validator<R, W, P, pointee_t<T>, Visited...>();
    } else if constexpr (kind == Kind::wrapper) {
      using Inner = std::remove_cvref_t<decltype(wrapped(std::declval<T&>()))>;
      return may_contain_validator<R, W, P, Inner, Visited...>();
    } else if constexpr (kind == Kind::variant || kind == Kind::tuple) {
      return []<class... Ts>(TypeList<Ts...>) {
        return (may_contain_validator<R, W, P, std::remove_cvref_t<Ts>,
                                      Visited...>() ||
                ...);
      }(typename alternatives<T>::type{});
    } else {
      using Fields = typename internal::processed_t<T, P>::Fields;
      return []<int... _is>(std::integer_sequence<int, _is...>) {
        return (
            may_contain_validator<R, W, P,
                                  std::remove_cvref_t<typename tuple_element_t<
                                      _is, Fields>::Type>,
                                  Visited...>() ||
            ...);
      }(std::make_integer_sequence<int, rfl::tuple_size_v<Fields>>());
    }
  }
}

template <class R, class W, class P, class T>
constexpr bool may_contain_validator_v =
    may_contain_validator<R, W, P, std::remove_cvref_t<T>>();

/// Whether a T can be copied. Standard containers claim to be copyable even
/// if their elements are not, so they are looked into.
template <class T>
consteval bool copyable() {
  if constexpr (!std::is_copy_constructible_v<T>) {
    return false;
  } else if constexpr (is_pair_v<T> || is_tuple_v<T>) {
    return []<class... Ts>(TypeList<Ts...>) {
      return (copyable<std::remove_cvref_t<Ts>>() && ...);
    }(typename alternatives<T>::type{});
  } else if constexpr (is_vector_like_v<T> || is_string_map_v<T>) {
    return copyable<std::remove_cvref_t<typename T::value_type>>();
  } else {
    return true;
  }
}

/// Walks the default value of one missing field and validates every
/// Validator it reaches (see validate).
template <class R, class W, class P>
class Walk {
 public:
  using Path = std::vector<ErrorDetail::PathSegment>;

  /// Returns true if _value holds an invalid Validator.
  template <class U>
  bool check(U& _value) {
    walk(_value);
    return error_.has_value();
  }

  /// Whether the walk met a type it cannot look into.
  bool opaque() const noexcept { return opaque_; }

  /// Records an error that was found by other means.
  void fail(const Error& _err) {
    error_.emplace(_err);
    error_path_ = path_;
  }

  /// Builds the error for the missing field _name.
  Error to_error(const std::string& _name) && {
    auto path = Path({_name});
    auto where = _name;
    for (const auto& seg : error_path_) {
      if (const auto* s = std::get_if<std::string>(&seg)) {
        where += "." + *s;
      } else {
        where += "[" + std::to_string(std::get<std::size_t>(seg)) + "]";
      }
      path.push_back(seg);
    }
    auto msg = "Field named '" + _name +
               "' not found, and its default value is invalid" +
               (error_path_.empty() ? std::string()
                                    : std::string(" at '") + where + "'") +
               ": " + error_->what();
    auto details = std::vector<ErrorDetail>(
        {ErrorDetail{.path = std::move(path), .message = msg}});
    return Error(std::move(msg), std::move(details));
  }

 private:
  template <class U>
  void walk(U& _value) {
    using T = std::remove_cv_t<U>;
    if constexpr (may_contain_validator_v<R, W, P, T>) {
      if (error_) {
        return;
      }
      constexpr Kind kind = kind_of<R, W, P, T>();
      if constexpr (kind == Kind::validator) {
        walk(_value.get());
        validate(_value);
      } else if constexpr (kind == Kind::range) {
        walk_range(elements(_value));
      } else if constexpr (kind == Kind::string_map) {
        for (auto& [key, val] : _value) {
          walk_keyed(key, val);
        }
      } else if constexpr (kind == Kind::owned || kind == Kind::result) {
        if constexpr (is_box_v<T>) {
          if (_value.get()) {
            walk(*_value);
          }
        } else if (_value) {
          walk(*_value);
        }
      } else if constexpr (kind == Kind::variant) {
        if constexpr (is_variant_v<T>) {
          std::visit([&](auto& _v) { walk(_v); }, _value);
        } else if constexpr (is_rfl_variant_v<T>) {
          _value.visit([&](auto& _v) { walk(_v); });
        } else {
          _value.variant().visit([&](auto& _v) { walk(_v); });
        }
      } else if constexpr (kind == Kind::tuple) {
        if constexpr (is_rfl_tuple_v<T>) {
          rfl::apply([&](auto&... _vs) { (walk(_vs), ...); }, _value);
        } else {
          std::apply([&](auto&... _vs) { (walk(_vs), ...); }, _value);
        }
      } else if constexpr (kind == Kind::wrapper) {
        walk(wrapped(_value));
      } else if constexpr (kind == Kind::structure) {
        walk_struct(_value);
      } else {
        opaque_ = true;
      }
    }
  }

  template <class U>
  void walk_range(U& _range) {
    using T = std::remove_cv_t<U>;
    std::size_t i = 0;
    for (auto& elem : _range) {
      if constexpr (is_map_like<T>::value) {
        // Maps with integral or floating point keys are read as objects.
        using Key = std::remove_cvref_t<decltype(elem.first)>;
        if constexpr (std::is_integral_v<Key> ||
                      std::is_floating_point_v<Key>) {
          walk_keyed(std::to_string(elem.first), elem.second);
          continue;
        }
      }
      path_.push_back(i++);
      walk(elem);
      path_.pop_back();
      if (error_) {
        return;
      }
    }
  }

  template <class K, class V>
  void walk_keyed(const K& _key, V& _value) {
    if (error_) {
      return;
    }
    path_.push_back(std::string(_key));
    walk(_value);
    path_.pop_back();
  }

  template <class U>
  void walk_struct(U& _struct) {
    using T = std::remove_cv_t<U>;
    // The view gives non-const access, but nothing is changed through it.
    auto view = P::template process<T>(to_view(const_cast<T&>(_struct)));
    using ViewType = decltype(view);
    const bool was_const = in_const_;
    in_const_ = in_const_ || std::is_const_v<U>;
    [&]<int... _is>(std::integer_sequence<int, _is...>) {
      (walk_field<_is>(view), ...);
    }(std::make_integer_sequence<int, static_cast<int>(ViewType::size())>());
    in_const_ = was_const;
  }

  template <int _i, class ViewType>
  void walk_field(ViewType& _view) {
    using FieldType = tuple_element_t<_i, typename ViewType::Fields>;
    if constexpr (std::is_pointer_v<typename FieldType::Type>) {
      if (error_) {
        return;
      }
      path_.push_back(std::string(FieldType::name()));
      walk(*rfl::get<_i>(_view));
      path_.pop_back();
    }
  }

  /// Validates a copy of the value. Only a value that cannot be copied is
  /// moved out and back: it is owned by the struct being read, because the
  /// walk does not follow shared ownership.
  template <class V>
  void validate(V& _validator) {
    using Validator = std::remove_cv_t<V>;
    using T = typename Validator::ReflectionType;
    using ValidationType = typename Validator::ValidationType;
    if constexpr (copyable<T>()) {
      auto res = ValidationType::validate(T(_validator.value()));
      if (!res) {
        fail(res.error());
      }
    } else if constexpr (!std::is_const_v<V>) {
      if (in_const_) {
        opaque_ = true;
        return;
      }
      auto res = ValidationType::validate(std::move(_validator.get()));
      if (!res) {
        fail(res.error());
        return;
      }
      _validator.get() = std::move(*res);
    } else {
      opaque_ = true;
    }
  }

  Path path_;
  bool in_const_ = false;
  Path error_path_;
  std::optional<Error> error_;
  bool opaque_ = false;
};

/// Whether field _i of the view, which points into the struct being read,
/// contains the object at _address. Empty members are skipped: under
/// [[no_unique_address]] they share their address with other members.
template <int _i, class ViewType>
bool field_contains(const ViewType& _view, const void* _address) {
  using FieldType = tuple_element_t<_i, typename ViewType::Fields>;
  using PtrType = typename FieldType::Type;
  if constexpr (std::is_pointer_v<PtrType>) {
    using Pointee = std::remove_cv_t<std::remove_pointer_t<PtrType>>;
    if constexpr (std::is_empty_v<Pointee>) {
      return false;
    } else {
      const auto begin =
          reinterpret_cast<const unsigned char*>(rfl::get<_i>(_view));
      const auto end = begin + sizeof(Pointee);
      const auto p = static_cast<const unsigned char*>(_address);
      return !std::less<>{}(p, begin) && std::less<>{}(p, end);
    }
  } else {
    return false;
  }
}

/// See check_missing_defaults.
template <class R, class W, class P, class ViewType>
RFL_COLD std::optional<Error> check(
    const internal::DeferredValidation* _deferred, ViewType& _view,
    const std::array<bool, ViewType::size()>& _found) {
  constexpr int size = static_cast<int>(ViewType::size());
  FieldErrors<P> errors;

  const auto check_field = [&]<int _i>(std::integral_constant<int, _i>) {
    using FieldType = tuple_element_t<_i, typename ViewType::Fields>;
    if constexpr (std::is_pointer_v<typename FieldType::Type>) {
      if (_found[_i] || should_stop_on_field_error<P>(errors)) {
        return;
      }
      auto walk = Walk<R, W, P>();
      bool failed = walk.check(*rfl::get<_i>(_view));
      if (!failed && walk.opaque() && _deferred) {
        // A type the walk cannot look into may hold Validators whose default
        // value was invalid when they were constructed. The records say
        // where they were. Their objects may have been replaced since, so
        // they are not looked at: the field is reported with the error.
        for (const auto& record : _deferred->records()) {
          if (field_contains<_i>(_view, record.address)) {
            walk.fail(record.error);
            failed = true;
            break;
          }
        }
      }
      if (failed) {
        errors.emplace_back(
            std::move(walk).to_error(std::string(FieldType::name())));
      }
    }
  };
  [&]<int... _is>(std::integer_sequence<int, _is...>) {
    (check_field(std::integral_constant<int, _is>{}), ...);
  }(std::make_integer_sequence<int, size>());

  // A recorded Validator outside every field, such as in a base class that is
  // not read, stays in the result as it is.
  if (_deferred && !should_stop_on_field_error<P>(errors)) {
    for (const auto& record : _deferred->records()) {
      bool in_field = false;
      [&]<int... _is>(std::integer_sequence<int, _is...>) {
        ((in_field = in_field || field_contains<_is>(_view, record.address)),
         ...);
      }(std::make_integer_sequence<int, size>());
      if (!in_field) {
        errors.emplace_back(Error(
            "The default value of a member that is not read is invalid: " +
            record.error.what()));
        break;
      }
    }
  }

  if constexpr (internal::fail_fast_v<P>) {
    return std::move(errors).error();
  } else {
    if (errors.empty()) {
      return std::nullopt;
    }
    return to_single_error(std::move(errors));
  }
}

}  // namespace missing_defaults

/// Called after reading into a struct whose default values may hold invalid
/// Validators: either it was default-constructed with the check deferred
/// (then _deferred is that DeferredValidation), or it was default-constructed
/// while another struct was, and may hold copies of that struct's defaults.
/// Returns an error if a field that the input did not contain holds an
/// invalid Validator. _found says which fields the input contained. Fields
/// that it contained were overwritten with values that were read.
template <class R, class W, class ProcessorsType, class ViewType>
std::optional<Error> check_missing_defaults(
    const internal::DeferredValidation* _deferred, ViewType& _view,
    const std::array<bool, ViewType::size()>& _found) {
  bool all_found = true;
  for (const bool f : _found) {
    all_found = all_found && f;
  }
  if (all_found && (!_deferred || _deferred->records().empty())) {
    return std::nullopt;
  }
  return missing_defaults::check<R, W, ProcessorsType>(_deferred, _view,
                                                       _found);
}

}  // namespace rfl::parsing

#endif
