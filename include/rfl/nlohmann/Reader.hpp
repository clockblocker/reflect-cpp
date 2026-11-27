#ifndef RFL_NLOHMANN_READER_HPP_
#define RFL_NLOHMANN_READER_HPP_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "../Bytestring.hpp"
#include "../Result.hpp"
#include "../Vectorstring.hpp"
#include "../always_false.hpp"
#include "../internal/base64.hpp"

static_assert(
    (NLOHMANN_JSON_VERSION_MAJOR > 3) ||
        (NLOHMANN_JSON_VERSION_MAJOR == 3 &&
         NLOHMANN_JSON_VERSION_MINOR > 11) ||
        (NLOHMANN_JSON_VERSION_MAJOR == 3 &&
         NLOHMANN_JSON_VERSION_MINOR == 11 &&
         NLOHMANN_JSON_VERSION_PATCH >= 3),
    "reflect-cpp requires nlohmann/json >= 3.11.3 (11-parameter "
    "nlohmann::basic_json template with CustomBaseClass).");

namespace rfl::nlohmann {

template <class T>
struct is_basic_json : std::false_type {};

template <template <typename U, typename V, typename... Args> class ObjectType,
          template <typename U, typename... Args> class ArrayType,
          class StringType, class BooleanType, class NumberIntegerType,
          class NumberUnsignedType, class NumberFloatType,
          template <typename U> class AllocatorType,
          template <typename T, typename SFINAE> class JSONSerializer,
          class BinaryType, class CustomBaseClass>
struct is_basic_json<::nlohmann::basic_json<
    ObjectType, ArrayType, StringType, BooleanType, NumberIntegerType,
    NumberUnsignedType, NumberFloatType, AllocatorType, JSONSerializer,
    BinaryType, CustomBaseClass>> : std::true_type {};

template <class T>
inline constexpr bool is_basic_json_v =
    is_basic_json<std::remove_cvref_t<T>>::value;

/// Reader for nlohmann::basic_json (nlohmann::json, nlohmann::ordered_json).
/// Guards every type check via get_ptr/predicates so it never throws.
template <class BasicJson = ::nlohmann::json>
struct Reader {
  using BasicJsonType = BasicJson;

  struct InputArray { const BasicJson* val_ = nullptr; };
  struct InputObject { const BasicJson* val_ = nullptr; };
  struct InputVar { const BasicJson* val_ = nullptr; };

  using InputArrayType = InputArray;
  using InputObjectType = InputObject;
  using InputVarType = InputVar;

  template <class T>
  static constexpr bool has_custom_constructor = false;

  rfl::Result<InputVarType> get_field_from_array(
      const size_t _idx, const InputArrayType& _arr) const noexcept {
    if (!_arr.val_ || !_arr.val_->is_array() || _idx >= _arr.val_->size()) {
      return error("Index " + std::to_string(_idx) + " out of bounds.");
    }
    return InputVarType{&(*_arr.val_)[_idx]};
  }

  rfl::Result<InputVarType> get_field_from_object(
      const std::string& _name, const InputObjectType& _obj) const noexcept {
    if (!_obj.val_ || !_obj.val_->is_object()) {
      return error("Could not cast to object.");
    }
    const auto it = _obj.val_->find(typename BasicJson::string_t(_name));
    if (it == _obj.val_->end()) {
      return error("Object contains no field named '" + _name + "'.");
    }
    return InputVarType{&it.value()};
  }

  bool is_empty(const InputVarType& _var) const noexcept {
    return !_var.val_ || _var.val_->is_null();
  }

  template <class ArrayReader>
  std::optional<Error> read_array(const ArrayReader& _array_reader,
                                  const InputArrayType& _arr) const noexcept {
    if (!_arr.val_ || !_arr.val_->is_array()) {
      return Error("Could not cast to array.");
    }
    if constexpr (requires { _array_reader.reserve(_arr.val_->size()); }) {
      if (auto err = _array_reader.reserve(_arr.val_->size())) return err;
    }
    for (const auto& elem : *_arr.val_) {
      auto var = InputVarType{&elem};
      if (auto err = _array_reader.read(var)) return err;
    }
    return std::nullopt;
  }

  template <class ObjectReader>
  std::optional<Error> read_object(const ObjectReader& _object_reader,
                                   const InputObjectType& _obj) const noexcept {
    if (!_obj.val_ || !_obj.val_->is_object()) {
      return Error("Could not cast to object.");
    }
    for (auto it = _obj.val_->begin(); it != _obj.val_->end(); ++it) {
      auto var = InputVarType{&it.value()};
      _object_reader.read(std::string_view(it.key()), var);
    }
    return std::nullopt;
  }

  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _var) const noexcept {
    using U = std::remove_cvref_t<T>;
    using StrT = typename BasicJson::string_t;
    using BoolT = typename BasicJson::boolean_t;
    using IntT = typename BasicJson::number_integer_t;
    using UIntT = typename BasicJson::number_unsigned_t;
    using FloatT = typename BasicJson::number_float_t;
    if (!_var.val_) return error("Could not cast null value.");

    if constexpr (std::is_same_v<U, std::string>) {
      const auto* p = _var.val_->template get_ptr<const StrT*>();
      return p ? rfl::Result<T>(std::string(*p))
               : error("Could not cast to string.");
    } else if constexpr (std::is_same_v<U, Bytestring>) {
      if (_var.val_->is_binary()) {
        const auto& bin = _var.val_->get_binary();
        Bytestring out;
        out.reserve(bin.size());
        for (const auto b : bin) {
          out.push_back(static_cast<std::byte>(b));
        }
        return out;
      }
      const auto* p = _var.val_->template get_ptr<const StrT*>();
      return p ? ::rfl::internal::decode_base64(std::string_view(*p))
               : error("Could not cast to a Base64 string.");
    } else if constexpr (std::is_same_v<U, Vectorstring>) {
      if (_var.val_->is_binary()) {
        const auto& bin = _var.val_->get_binary();
        Vectorstring out;
        out.reserve(bin.size());
        for (const auto b : bin) {
          out.push_back(static_cast<char>(b));
        }
        return out;
      }
      const auto* p = _var.val_->template get_ptr<const StrT*>();
      if (!p) {
        return error("Could not cast to a Base64 string.");
      }
      return ::rfl::internal::decode_base64(std::string_view(*p))
          .transform([](const Bytestring& _bytes) {
            Vectorstring out;
            out.reserve(_bytes.size());
            for (const auto b : _bytes) {
              out.push_back(
                  static_cast<char>(std::to_integer<unsigned char>(b)));
            }
            return out;
          });
    } else if constexpr (std::is_same_v<U, bool>) {
      const auto* p = _var.val_->template get_ptr<const BoolT*>();
      return p ? rfl::Result<T>(static_cast<bool>(*p))
               : error("Could not cast to boolean.");
    } else if constexpr (std::is_floating_point_v<U>) {
      if (const auto* p = _var.val_->template get_ptr<const FloatT*>())
        return static_cast<T>(*p);
      if (const auto* p = _var.val_->template get_ptr<const UIntT*>())
        return static_cast<T>(*p);
      if (const auto* p = _var.val_->template get_ptr<const IntT*>())
        return static_cast<T>(*p);
      return error("Could not cast to floating point number.");
    } else if constexpr (std::is_unsigned_v<U>) {
      if (const auto* p = _var.val_->template get_ptr<const UIntT*>()) {
        if (*p > static_cast<uint64_t>(std::numeric_limits<U>::max()))
          return error("Integer is out of range for the target type.");
        return static_cast<T>(*p);
      }
      if (const auto* p = _var.val_->template get_ptr<const IntT*>()) {
        if (*p < 0)
          return error("Cannot cast negative integer to unsigned type.");
        if (static_cast<uint64_t>(*p) >
            static_cast<uint64_t>(std::numeric_limits<U>::max()))
          return error("Integer is out of range for the target type.");
        return static_cast<T>(*p);
      }
      return error("Could not cast to unsigned integer.");
    } else if constexpr (std::is_integral_v<U>) {
      if (const auto* p = _var.val_->template get_ptr<const IntT*>()) {
        if (*p < static_cast<int64_t>(std::numeric_limits<U>::min()) ||
            *p > static_cast<int64_t>(std::numeric_limits<U>::max()))
          return error("Integer is out of range for the target type.");
        return static_cast<T>(*p);
      }
      if (const auto* p = _var.val_->template get_ptr<const UIntT*>()) {
        if (*p > static_cast<uint64_t>(std::numeric_limits<U>::max()))
          return error("Integer is out of range for the target type.");
        return static_cast<T>(*p);
      }
      return error("Could not cast to integer.");
    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
  }

  rfl::Result<InputArrayType> to_array(
      const InputVarType& _var) const noexcept {
    if (!_var.val_ || !_var.val_->is_array())
      return error("Could not cast to array.");
    return InputArrayType{_var.val_};
  }

  rfl::Result<InputObjectType> to_object(
      const InputVarType& _var) const noexcept {
    if (!_var.val_ || !_var.val_->is_object())
      return error("Could not cast to object.");
    return InputObjectType{_var.val_};
  }

  template <class T>
  rfl::Result<T> use_custom_constructor(const InputVarType&) const noexcept {
    return error("Custom constructors are not supported for nlohmann::json.");
  }
};

template <class T>
struct is_nlohmann_reader : std::false_type {};

template <class BasicJson>
struct is_nlohmann_reader<Reader<BasicJson>> : std::true_type {};

template <class T>
inline constexpr bool is_nlohmann_reader_v =
    is_nlohmann_reader<std::remove_cvref_t<T>>::value;

}  // namespace rfl::nlohmann

#endif
