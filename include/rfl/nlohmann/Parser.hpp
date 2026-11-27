#ifndef RFL_NLOHMANN_PARSER_HPP_
#define RFL_NLOHMANN_PARSER_HPP_

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Bytestring.hpp"
#include "../Generic.hpp"
#include "../Result.hpp"
#include "../internal/base64.hpp"
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

namespace internal {

template <class BasicJson>
void reserve_array(BasicJson& _arr, const size_t _size) {
  if (_size == 0) return;
  if constexpr (requires(typename BasicJson::array_t& a, size_t n) {
                  a.reserve(n);
                }) {
    if (auto* vec = _arr.template get_ptr<typename BasicJson::array_t*>())
      vec->reserve(_size);
  }
}

template <class BasicJson>
void reserve_object(BasicJson& _obj, const size_t _size) {
  if (_size == 0) return;
  if constexpr (requires(typename BasicJson::object_t& o, size_t n) {
                  o.reserve(n);
                }) {
    if (auto* map = _obj.template get_ptr<typename BasicJson::object_t*>())
      map->reserve(_size);
  }
}

template <class BasicJson>
void emplace_field(BasicJson& _obj, typename BasicJson::string_t _key,
                   BasicJson&& _val) {
  if constexpr (requires(typename BasicJson::object_t& o,
                         typename BasicJson::string_t k, BasicJson v) {
                  o.insert_or_assign(std::move(k), std::move(v));
                }) {
    if (auto* map = _obj.template get_ptr<typename BasicJson::object_t*>()) {
      map->insert_or_assign(std::move(_key), std::move(_val));
      return;
    }
  }
  auto [it, inserted] = _obj.emplace(std::move(_key), std::move(_val));
  if (!inserted) {
    it.value() = std::move(_val);
  }
}

/// Lossless conversion between distinct nlohmann::basic_json instantiations
/// (e.g., nlohmann::json <-> nlohmann::ordered_json).
template <class TargetBasicJson, class SourceBasicJson>
TargetBasicJson convert_basic_json(const SourceBasicJson& _src) {
  if (_src.is_null()) {
    return TargetBasicJson(nullptr);
  }
  if (const auto* b = _src.template get_ptr<
                      const typename SourceBasicJson::boolean_t*>()) {
    return TargetBasicJson(
        static_cast<typename TargetBasicJson::boolean_t>(*b));
  }
  if (const auto* u = _src.template get_ptr<
                      const typename SourceBasicJson::number_unsigned_t*>()) {
    return TargetBasicJson(
        static_cast<typename TargetBasicJson::number_unsigned_t>(*u));
  }
  if (const auto* i = _src.template get_ptr<
                      const typename SourceBasicJson::number_integer_t*>()) {
    return TargetBasicJson(
        static_cast<typename TargetBasicJson::number_integer_t>(*i));
  }
  if (const auto* f = _src.template get_ptr<
                      const typename SourceBasicJson::number_float_t*>()) {
    return TargetBasicJson(
        static_cast<typename TargetBasicJson::number_float_t>(*f));
  }
  if (const auto* s = _src.template get_ptr<
                      const typename SourceBasicJson::string_t*>()) {
    return TargetBasicJson(
        typename TargetBasicJson::string_t(s->data(), s->size()));
  }
  if (_src.is_binary()) {
    const auto& src_bin = _src.get_binary();
    typename TargetBasicJson::binary_t::container_type dst_bytes;
    if constexpr (requires { dst_bytes.reserve(src_bin.size()); }) {
      dst_bytes.reserve(src_bin.size());
    }
    using DstByte = typename TargetBasicJson::binary_t::value_type;
    for (const auto b : src_bin) {
      dst_bytes.push_back(static_cast<DstByte>(b));
    }
    if (src_bin.has_subtype()) {
      return TargetBasicJson::binary(std::move(dst_bytes), src_bin.subtype());
    }
    return TargetBasicJson::binary(std::move(dst_bytes));
  }
  if (_src.is_array()) {
    auto arr = TargetBasicJson::array();
    reserve_array(arr, _src.size());
    for (const auto& elem : _src) {
      arr.push_back(convert_basic_json<TargetBasicJson>(elem));
    }
    return arr;
  }
  if (_src.is_object()) {
    auto obj = TargetBasicJson::object();
    reserve_object(obj, _src.size());
    for (auto it = _src.begin(); it != _src.end(); ++it) {
      emplace_field(obj,
                    typename TargetBasicJson::string_t(it.key().data(),
                                                       it.key().size()),
                    convert_basic_json<TargetBasicJson>(it.value()));
    }
    return obj;
  }
  return TargetBasicJson(nullptr);
}

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
    reserve_array(arr, yyjson_arr_size(_val));
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
    reserve_object(obj, yyjson_obj_size(_val));
    auto iter = yyjson_obj_iter_with(_val);
    while (auto* key = yyjson_obj_iter_next(&iter)) {
      auto res = read_from_yyjson<BasicJson>(yyjson_obj_iter_get_val(key));
      if (!res) return error(std::move(res.error()));
      emplace_field(
          obj,
          typename BasicJson::string_t(yyjson_get_str(key), yyjson_get_len(key)),
          std::move(*res));
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
        emplace_field(obj_, typename BasicJson::string_t(_key),
                      std::move(*val_res));
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
  // Preserve explicitly quoted YAML strings (Tag() == "!") without coercing
  // them into booleans or numbers.
  if constexpr (requires(const typename R::InputVarType& v) {
                  { v.node_.Tag() == "!" } -> std::convertible_to<bool>;
                }) {
    if (_var.node_.Tag() == "!") {
      if (auto s = _r.template to_basic_type<std::string>(_var))
        return BasicJson(typename BasicJson::string_t(std::move(*s)));
    }
  }
  if (auto b = _r.template to_basic_type<bool>(_var))
    return BasicJson(static_cast<typename BasicJson::boolean_t>(*b));
  // Preserve native unsigned 64-bit integers in readers that distinguish them
  // (e.g., jsoncons-based CBOR/UBJSON) so values > INT64_MAX do not wrap.
  if constexpr (requires(const typename R::InputVarType& v) {
                  { v.val_->is_uint64() } -> std::convertible_to<bool>;
                }) {
    if (_var.val_ && _var.val_->is_uint64()) {
      if (auto u = _r.template to_basic_type<uint64_t>(_var))
        return BasicJson(
            static_cast<typename BasicJson::number_unsigned_t>(*u));
    }
  }
  if (auto i = _r.template to_basic_type<int64_t>(_var)) {
    return BasicJson(static_cast<typename BasicJson::number_integer_t>(*i));
  }
  if (auto u = _r.template to_basic_type<uint64_t>(_var)) {
    return BasicJson(static_cast<typename BasicJson::number_unsigned_t>(*u));
  }
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
  } else if (_val.is_binary()) {
    const auto& bin = _val.get_binary();
    Bytestring bytes;
    bytes.reserve(bin.size());
    for (const auto b : bin) {
      bytes.push_back(static_cast<std::byte>(b));
    }
    ParentType::add_value(_w, ::rfl::internal::encode_base64(bytes), _parent);
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

}  // namespace internal
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
    } else if constexpr (rfl::nlohmann::is_nlohmann_reader_v<R>) {
      return _var.val_
                 ? rfl::nlohmann::internal::convert_basic_json<BasicJson>(
                       *_var.val_)
                 : BasicJson(nullptr);
    } else if constexpr (rfl::nlohmann::internal::IsYYJSONReader<R>) {
      return rfl::nlohmann::internal::read_from_yyjson<BasicJson>(_var.val_);
    } else {
      return rfl::nlohmann::internal::read_from_generic_reader<BasicJson>(_r,
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
        rfl::nlohmann::internal::write_to_writer(_w, _val, _parent);
    } else if constexpr (rfl::nlohmann::is_nlohmann_writer_v<W>) {
      using TargetBasicJson = typename std::remove_cvref_t<W>::BasicJsonType;
      const auto converted =
          rfl::nlohmann::internal::convert_basic_json<TargetBasicJson>(_val);
      if constexpr (std::is_same_v<PType, typename ParentType::Root>)
        _w.raw_as_root(converted);
      else if constexpr (std::is_same_v<PType, typename ParentType::Array>)
        _w.add_raw_to_array(converted, _parent.arr_);
      else if constexpr (std::is_same_v<PType, typename ParentType::Object>)
        _w.add_raw_to_object(_parent.name_, converted, _parent.obj_);
      else
        rfl::nlohmann::internal::write_to_writer(_w, _val, _parent);
    } else {
      rfl::nlohmann::internal::write_to_writer(_w, _val, _parent);
    }
  }

  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    return Parser<R, W, rfl::Generic, ProcessorsType>::to_schema(_definitions);
  }
};

}  // namespace rfl::parsing

#endif
