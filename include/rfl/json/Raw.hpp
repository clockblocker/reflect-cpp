#ifndef RFL_JSON_RAW_HPP_
#define RFL_JSON_RAW_HPP_

#if __has_include(<yyjson.h>)
#include <yyjson.h>
#else
#include "../thirdparty/yyjson.h"
#endif

#include <cstddef>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Generic.hpp"
#include "../Result.hpp"
#include "../always_false.hpp"
#include "../parsing/AreReaderAndWriter.hpp"
#include "../parsing/Parent.hpp"
#include "../parsing/Parser.hpp"
#include "../parsing/schema/Type.hpp"
#include "Reader.hpp"
#include "Writer.hpp"

namespace rfl::json {

/// Holds a raw, unparsed JSON value for passthrough serialization and
/// deserialization without converting into intermediate C++ structures.
class Raw {
 public:
  Raw() = default;

  Raw(std::string _value) : value_(std::move(_value)) {}

  Raw(std::string_view _value) : value_(_value) {}

  Raw(const char* _value) : value_(_value ? _value : "null") {}

  const std::string& str() const noexcept { return value_; }

  const std::string& value() const noexcept { return value_; }

  const char* c_str() const noexcept { return value_.c_str(); }

  bool empty() const noexcept { return value_.empty(); }

  bool operator==(const Raw&) const = default;

  static Result<Raw> from_json_obj(const Reader::YYJSONInputVar& _var) {
    if (!_var.val_) {
      return error("Cannot construct rfl::json::Raw from null JSON value.");
    }
    struct FreeDeleter {
      void operator()(char* _p) const noexcept { std::free(_p); }
    };
    size_t len = 0;
    std::unique_ptr<char, FreeDeleter> buf(
        yyjson_val_write(_var.val_, 0, &len));
    if (!buf) {
      return error("Could not serialize JSON value into rfl::json::Raw.");
    }
    return Raw(std::string(buf.get(), len));
  }

 private:
  std::string value_{"null"};
};

}  // namespace rfl::json

namespace rfl::parsing {

template <class ProcessorsType>
  requires AreReaderAndWriter<json::Reader, json::Writer, json::Raw>
struct Parser<json::Reader, json::Writer, json::Raw, ProcessorsType> {
 public:
  using InputVarType = typename json::Reader::InputVarType;
  using ParentType = Parent<json::Writer>;

  static Result<json::Raw> read(const json::Reader& _r,
                                const InputVarType& _var) noexcept {
    return _r.template use_custom_constructor<json::Raw>(_var);
  }

  template <class P>
  static void write(const json::Writer& _w, const json::Raw& _raw,
                    const P& _parent) {
    using Type = std::remove_cvref_t<P>;
    if constexpr (std::is_same_v<Type, typename ParentType::Array>) {
      _w.add_raw_to_array(_raw.str(), _parent.arr_);
    } else if constexpr (std::is_same_v<Type, typename ParentType::Object>) {
      _w.add_raw_to_object(_parent.name_, _raw.str(), _parent.obj_);
    } else if constexpr (std::is_same_v<Type, typename ParentType::Root>) {
      _w.raw_as_root(_raw.str());
    } else {
      static_assert(always_false_v<Type>, "Unsupported parent type.");
    }
  }

  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    return Parser<json::Reader, json::Writer, Generic,
                  ProcessorsType>::to_schema(_definitions);
  }
};

}  // namespace rfl::parsing

#endif
