#ifndef RFL_NLOHMANN_PARSER_HPP_
#define RFL_NLOHMANN_PARSER_HPP_

#include <concepts>
#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Generic.hpp"
#include "../Result.hpp"
#include "../parsing/AreReaderAndWriter.hpp"
#include "../parsing/Parent.hpp"
#include "../parsing/Parser.hpp"
#include "../parsing/schema/Type.hpp"
#include "Reader.hpp"
#include "Writer.hpp"

namespace rfl::nlohmann {

template <class T, class ProcessorsType, class BasicJson = ::nlohmann::json>
using Parser =
    parsing::Parser<Reader<BasicJson>, Writer<BasicJson>, T, ProcessorsType>;

namespace detail {

// Detects rfl::json::Reader via dependent ADL so <rfl/nlohmann.hpp> compiles
// without <rfl/json.hpp> or yyjson.h.
template <class R>
concept IsYYJSONReader = requires(const typename R::InputVarType& v) {
  { yyjson_is_null(v.val_) } -> std::convertible_to<bool>;
  { yyjson_is_uint(v.val_) } -> std::convertible_to<bool>;
  { yyjson_get_uint(v.val_) } -> std::convertible_to<uint64_t>;
  { yyjson_get_sint(v.val_) } -> std::convertible_to<int64_t>;
};

template <class BasicJson, class ValPtr>
Result<BasicJson> read_from_yyjson(ValPtr _val) {
  if (!_val || yyjson_is_null(_val)) return BasicJson(nullptr);
  if (yyjson_is_bool(_val))
    return BasicJson(
        static_cast<typename BasicJson::boolean_t>(yyjson_get_bool(_val)));
  if (yyjson_is_uint(_val))
    return BasicJson(static_cast<typename BasicJson::number_unsigned_t>(
        yyjson_get_uint(_val)));
  if (yyjson_is_sint(_val))
    return BasicJson(static_cast<typename BasicJson::number_integer_t>(
        yyjson_get_sint(_val)));
  if (yyjson_is_real(_val))
    return BasicJson(static_cast<typename BasicJson::number_float_t>(
        yyjson_get_real(_val)));
  if (yyjson_is_str(_val))
    return BasicJson(typename BasicJson::string_t(yyjson_get_str(_val),
                                                  yyjson_get_len(_val)));
  if (yyjson_is_arr(_val)) {
    auto arr = BasicJson::array();
    auto iter = yyjson_arr_iter_with(_val);
    while (auto* elem = yyjson_arr_iter_next(&iter)) {
      auto res = read_from_yyjson<BasicJson>(elem);
      if (!res) return error(std::move(res.error()));
      arr.push_back(std::move(*res));
    }
    return arr;
  }
  if (yyjson_is_obj(_val)) {
    auto obj = BasicJson::object();
    auto iter = yyjson_obj_iter_with(_val);
    while (auto* key = yyjson_obj_iter_next(&iter)) {
      auto res = read_from_yyjson<BasicJson>(yyjson_obj_iter_get_val(key));
      if (!res) return error(std::move(res.error()));
      obj[typename BasicJson::string_t(yyjson_get_str(key),
                                       yyjson_get_len(key))] = std::move(*res);
    }
    return obj;
  }
  return error("Unsupported yyjson value type.");
}

template <class BasicJson, class R>
Result<BasicJson> read_from_generic_reader(
    const R& _r, const typename R::InputVarType& _var) {
  if (_r.is_empty(_var)) return BasicJson(nullptr);
  if (auto obj_res = _r.to_object(_var)) {
    auto obj = BasicJson::object();
    std::optional<Error> first_err;
    struct ObjReader {
      const R& r_;
      BasicJson& obj_;
      std::optional<Error>& err_;
      void read(const std::string_view _key,
                const typename R::InputVarType& _v) const {
        if (err_) return;
        auto val_res = read_from_generic_reader<BasicJson>(r_, _v);
        if (!val_res) {
          err_ = std::move(val_res.error());
          return;
        }
        obj_[typename BasicJson::string_t(_key)] = std::move(*val_res);
      }
    };
    if (auto err = _r.read_object(ObjReader{_r, obj, first_err}, *obj_res))
      return error(std::move(*err));
    if (first_err) return error(std::move(*first_err));
    return obj;
  }
  if (auto arr_res = _r.to_array(_var)) {
    auto arr = BasicJson::array();
    struct ArrReader {
      const R& r_;
      BasicJson& arr_;
      std::optional<Error> read(const typename R::InputVarType& _v) const {
        auto val_res = read_from_generic_reader<BasicJson>(r_, _v);
        if (!val_res) return std::move(val_res.error());
        arr_.push_back(std::move(*val_res));
        return std::nullopt;
      }
    };
    if (auto err = _r.read_array(ArrReader{_r, arr}, *arr_res))
      return error(std::move(*err));
    return arr;
  }
  if (auto b = _r.template to_basic_type<bool>(_var))
    return BasicJson(static_cast<typename BasicJson::boolean_t>(*b));
  if (auto i = _r.template to_basic_type<int64_t>(_var))
    return BasicJson(static_cast<typename BasicJson::number_integer_t>(*i));
  if (auto u = _r.template to_basic_type<uint64_t>(_var))
    return BasicJson(static_cast<typename BasicJson::number_unsigned_t>(*u));
  if (auto d = _r.template to_basic_type<double>(_var))
    return BasicJson(static_cast<typename BasicJson::number_float_t>(*d));
  if (auto s = _r.template to_basic_type<std::string>(_var))
    return BasicJson(typename BasicJson::string_t(std::move(*s)));
  return error("Could not parse value into nlohmann::basic_json.");
}

template <class BasicJson, class W, class P>
void write_to_writer(const W& _w, const BasicJson& _val, const P& _parent) {
  using ParentType = parsing::Parent<W>;
  if (_val.is_null()) {
    ParentType::add_null(_w, _parent);
  } else if (const auto* b = _val.template get_ptr<
                 const typename BasicJson::boolean_t*>()) {
    ParentType::add_value(_w, static_cast<bool>(*b), _parent);
  } else if (const auto* u = _val.template get_ptr<
                 const typename BasicJson::number_unsigned_t*>()) {
    ParentType::add_value(_w, static_cast<uint64_t>(*u), _parent);
  } else if (const auto* i = _val.template get_ptr<
                 const typename BasicJson::number_integer_t*>()) {
    ParentType::add_value(_w, static_cast<int64_t>(*i), _parent);
  } else if (const auto* f = _val.template get_ptr<
                 const typename BasicJson::number_float_t*>()) {
    ParentType::add_value(_w, static_cast<double>(*f), _parent);
  } else if (const auto* s =
                 _val.template get_ptr<const typename BasicJson::string_t*>()) {
    if constexpr (std::is_same_v<typename BasicJson::string_t, std::string>)
      ParentType::add_value(_w, *s, _parent);
    else
      ParentType::add_value(_w, std::string(*s), _parent);
  } else if (_val.is_array()) {
    auto arr = ParentType::add_array(_w, _val.size(), _parent);
    const auto elem_parent = typename ParentType::Array{&arr};
    for (const auto& elem : _val) write_to_writer(_w, elem, elem_parent);
    _w.end_array(&arr);
  } else if (_val.is_object()) {
    auto obj = ParentType::add_object(_w, _val.size(), _parent);
    for (auto it = _val.begin(); it != _val.end(); ++it) {
      const auto field_parent =
          typename ParentType::Object{std::string_view(it.key()), &obj};
      write_to_writer(_w, it.value(), field_parent);
    }
    _w.end_object(&obj);
  } else {
    ParentType::add_null(_w, _parent);
  }
}

}  // namespace detail
}  // namespace rfl::nlohmann

namespace rfl::parsing {

/// Parser specialization enabling nlohmann::basic_json as a field type inside
/// reflected structs across rfl::nlohmann, rfl::json, rfl::generic, etc.
template <class R, class W,
          template <typename U, typename V, typename... Args> class ObjectType,
          template <typename U, typename... Args> class ArrayType,
          class StringType, class BooleanType, class NumberIntegerType,
          class NumberUnsignedType, class NumberFloatType,
          template <typename U> class AllocatorType,
          template <typename T, typename SFINAE> class JSONSerializer,
          class BinaryType, class CustomBaseClass, class ProcessorsType>
  requires AreReaderAndWriter<
      R, W,
      ::nlohmann::basic_json<ObjectType, ArrayType, StringType, BooleanType,
                             NumberIntegerType, NumberUnsignedType,
                             NumberFloatType, AllocatorType, JSONSerializer,
                             BinaryType, CustomBaseClass>>
struct Parser<
    R, W,
    ::nlohmann::basic_json<ObjectType, ArrayType, StringType, BooleanType,
                           NumberIntegerType, NumberUnsignedType,
                           NumberFloatType, AllocatorType, JSONSerializer,
                           BinaryType, CustomBaseClass>,
    ProcessorsType> {
  using BasicJson =
      ::nlohmann::basic_json<ObjectType, ArrayType, StringType, BooleanType,
                             NumberIntegerType, NumberUnsignedType,
                             NumberFloatType, AllocatorType, JSONSerializer,
                             BinaryType, CustomBaseClass>;
  using InputVarType = typename R::InputVarType;
  using ParentType = Parent<W>;

  static Result<BasicJson> read(const R& _r, const InputVarType& _var) {
    if constexpr (std::is_same_v<std::remove_cvref_t<R>,
                                 rfl::nlohmann::Reader<BasicJson>>) {
      return _var.val_ ? *_var.val_ : BasicJson(nullptr);
    } else if constexpr (rfl::nlohmann::detail::IsYYJSONReader<R>) {
      return rfl::nlohmann::detail::read_from_yyjson<BasicJson>(_var.val_);
    } else {
      return rfl::nlohmann::detail::read_from_generic_reader<BasicJson>(_r,
                                                                        _var);
    }
  }

  template <class P>
  static void write(const W& _w, const BasicJson& _val, const P& _parent) {
    using PType = std::remove_cvref_t<P>;
    if constexpr (std::is_same_v<std::remove_cvref_t<W>,
                                 rfl::nlohmann::Writer<BasicJson>>) {
      if constexpr (std::is_same_v<PType, typename ParentType::Root>)
        _w.raw_as_root(_val);
      else if constexpr (std::is_same_v<PType, typename ParentType::Array>)
        _w.add_raw_to_array(_val, _parent.arr_);
      else if constexpr (std::is_same_v<PType, typename ParentType::Object>)
        _w.add_raw_to_object(_parent.name_, _val, _parent.obj_);
      else
        rfl::nlohmann::detail::write_to_writer(_w, _val, _parent);
    } else {
      rfl::nlohmann::detail::write_to_writer(_w, _val, _parent);
    }
  }

  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    return Parser<R, W, rfl::Generic, ProcessorsType>::to_schema(_definitions);
  }
};

}  // namespace rfl::parsing

#endif
