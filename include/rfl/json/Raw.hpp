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

/// Holds a raw JSON value for passthrough serialization and deserialization
/// without converting into intermediate C++ structures.
///
/// Note on reading vs. writing:
/// - Writing emits the stored JSON text verbatim via `yyjson_mut_rawncpy`
///   without re-parsing. Use `Raw::from_string()` when constructing from
///   untrusted input so malformed JSON is rejected up front.
/// - Reading captures the parsed yyjson subtree via `yyjson_val_write`, which
///   canonicalizes whitespace. Pass `YYJSON_READ_BIGNUM_AS_RAW` to
///   `rfl::json::read` if you need integers wider than 64 bits preserved
///   verbatim.
class Raw {
 public:
  Raw() = default;

  explicit Raw(std::string _value)
      : value_(_value.empty() ? std::string("null") : std::move(_value)) {}

  explicit Raw(const std::string_view _value)
      : value_(_value.empty() ? std::string("null") : std::string(_value)) {}

  explicit Raw(const char* _value)
      : value_((_value && _value[0] != '\0') ? std::string(_value)
                                             : std::string("null")) {}

  /// Validates that `_str` contains a single well-formed JSON value and
  /// constructs a `Raw` holding `_str` verbatim.
  static Result<Raw> from_string(const std::string_view _str,
                                 const yyjson_read_flag _flag = 0) {
    if (_flag & YYJSON_READ_INSITU) {
      return error("YYJSON_READ_INSITU is not supported");
    }
    yyjson_read_err err{};
    yyjson_doc* doc = yyjson_read_opts(const_cast<char*>(_str.data()),
                                       _str.size(), _flag, nullptr, &err);
    if (!doc) {
      return error("Invalid raw JSON: " +
                       std::string(err.msg ? err.msg : "parse error"),
                   ErrorKind::syntax_error);
    }
    yyjson_doc_free(doc);
    return Raw(_str);
  }

  const std::string& str() const noexcept { return value_; }

  const std::string& value() const noexcept { return value_; }

  const char* c_str() const noexcept { return value_.c_str(); }

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

template <class R, class W, class ProcessorsType>
  requires AreReaderAndWriter<R, W, json::Raw>
struct Parser<R, W, json::Raw, ProcessorsType> {
  static_assert(always_false_v<R>,
                "rfl::json::Raw is only supported with rfl::json.");
};

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
