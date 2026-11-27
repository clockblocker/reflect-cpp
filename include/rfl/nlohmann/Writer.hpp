#ifndef RFL_NLOHMANN_WRITER_HPP_
#define RFL_NLOHMANN_WRITER_HPP_

#include <cstddef>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Bytestring.hpp"
#include "../Vectorstring.hpp"
#include "../always_false.hpp"
#include "../internal/base64.hpp"

namespace rfl::nlohmann {

/// Writer constructing nlohmann::basic_json directly without string round-trip.
/// Preserves insertion order for nlohmann::ordered_json.
template <class BasicJson = ::nlohmann::json>
struct Writer {
  using BasicJsonType = BasicJson;

  struct OutputArray { BasicJson* val_ = nullptr; };
  struct OutputObject { BasicJson* val_ = nullptr; };
  struct OutputVar {
    BasicJson* val_ = nullptr;
    OutputVar() = default;
    explicit OutputVar(BasicJson* _val) : val_(_val) {}
    OutputVar(OutputArray _arr) : val_(_arr.val_) {}
    OutputVar(OutputObject _obj) : val_(_obj.val_) {}
  };

  using OutputArrayType = OutputArray;
  using OutputObjectType = OutputObject;
  using OutputVarType = OutputVar;

  OutputArrayType array_as_root(const size_t _size) const {
    root_ = BasicJson::array();
    reserve_array(root_, _size);
    return OutputArrayType{&root_};
  }

  OutputObjectType object_as_root(const size_t _size) const {
    root_ = BasicJson::object();
    reserve_object(root_, _size);
    return OutputObjectType{&root_};
  }

  OutputVarType null_as_root() const {
    root_ = nullptr;
    return OutputVarType(&root_);
  }

  template <class T>
  OutputVarType value_as_root(const T& _var) const {
    root_ = to_basic_json(_var);
    return OutputVarType(&root_);
  }

  OutputArrayType add_array_to_array(const size_t _size,
                                     OutputArrayType* _parent) const {
    _parent->val_->push_back(BasicJson::array());
    auto& child = _parent->val_->back();
    reserve_array(child, _size);
    return OutputArrayType{&child};
  }

  OutputArrayType add_array_to_object(const std::string_view& _name,
                                      const size_t _size,
                                      OutputObjectType* _parent) const {
    auto& child = emplace_in_object(
        *_parent->val_, _name, [] { return BasicJson::array(); });
    reserve_array(child, _size);
    return OutputArrayType{&child};
  }

  OutputObjectType add_object_to_array(const size_t _size,
                                       OutputArrayType* _parent) const {
    _parent->val_->push_back(BasicJson::object());
    auto& child = _parent->val_->back();
    reserve_object(child, _size);
    return OutputObjectType{&child};
  }

  OutputObjectType add_object_to_object(const std::string_view& _name,
                                        const size_t _size,
                                        OutputObjectType* _parent) const {
    auto& child = emplace_in_object(
        *_parent->val_, _name, [] { return BasicJson::object(); });
    reserve_object(child, _size);
    return OutputObjectType{&child};
  }

  template <class T>
  OutputVarType add_value_to_array(const T& _var,
                                   OutputArrayType* _parent) const {
    _parent->val_->push_back(to_basic_json(_var));
    return OutputVarType(&_parent->val_->back());
  }

  template <class T>
  OutputVarType add_value_to_object(const std::string_view& _name,
                                    const T& _var,
                                    OutputObjectType* _parent) const {
    auto& slot = emplace_in_object(
        *_parent->val_, _name, [&_var] { return to_basic_json(_var); });
    return OutputVarType(&slot);
  }

  OutputVarType add_null_to_array(OutputArrayType* _parent) const {
    _parent->val_->push_back(nullptr);
    return OutputVarType(&_parent->val_->back());
  }

  OutputVarType add_null_to_object(const std::string_view& _name,
                                   OutputObjectType* _parent) const {
    auto& slot =
        emplace_in_object(*_parent->val_, _name, [] { return BasicJson(nullptr); });
    return OutputVarType(&slot);
  }

  OutputVarType raw_as_root(const BasicJson& _val) const {
    root_ = _val;
    return OutputVarType(&root_);
  }

  OutputVarType add_raw_to_array(const BasicJson& _val,
                                 OutputArrayType* _parent) const {
    _parent->val_->push_back(_val);
    return OutputVarType(&_parent->val_->back());
  }

  OutputVarType add_raw_to_object(const std::string_view& _name,
                                  const BasicJson& _val,
                                  OutputObjectType* _parent) const {
    auto& slot = emplace_in_object(
        *_parent->val_, _name, [&_val] { return BasicJson(_val); });
    return OutputVarType(&slot);
  }

  void end_array(OutputArrayType*) const noexcept {}
  void end_object(OutputObjectType*) const noexcept {}
  BasicJson& root() noexcept { return root_; }
  const BasicJson& root() const noexcept { return root_; }

 private:
  template <class MakeValue>
  static BasicJson& emplace_in_object(BasicJson& _obj,
                                      const std::string_view _name,
                                      MakeValue&& _make_value) {
    auto& slot = _obj[typename BasicJson::string_t(_name)];
    slot = std::forward<MakeValue>(_make_value)();
    return slot;
  }

  static void reserve_array(BasicJson& _arr, const size_t _size) {
    if (_size == 0) return;
    if constexpr (requires(typename BasicJson::array_t& a, size_t n) {
                    a.reserve(n);
                  }) {
      if (auto* vec = _arr.template get_ptr<typename BasicJson::array_t*>())
        vec->reserve(_size);
    }
  }

  static void reserve_object(BasicJson& _obj, const size_t _size) {
    if (_size == 0) return;
    if constexpr (requires(typename BasicJson::object_t& o, size_t n) {
                    o.reserve(n);
                  }) {
      if (auto* map = _obj.template get_ptr<typename BasicJson::object_t*>())
        map->reserve(_size);
    }
  }

  template <class T>
  static BasicJson to_basic_json(const T& _var) {
    using U = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<U, std::string>) {
      return BasicJson(typename BasicJson::string_t(_var));
    } else if constexpr (std::is_same_v<U, Bytestring>) {
      return BasicJson(
          typename BasicJson::string_t(::rfl::internal::encode_base64(_var)));
    } else if constexpr (std::is_same_v<U, Vectorstring>) {
      Bytestring bytes;
      bytes.reserve(_var.size());
      for (const char c : _var) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
      }
      return BasicJson(
          typename BasicJson::string_t(::rfl::internal::encode_base64(bytes)));
    } else if constexpr (std::is_same_v<U, bool>) {
      return BasicJson(static_cast<typename BasicJson::boolean_t>(_var));
    } else if constexpr (std::is_floating_point_v<U>) {
      return BasicJson(static_cast<typename BasicJson::number_float_t>(_var));
    } else if constexpr (std::is_unsigned_v<U>) {
      return BasicJson(
          static_cast<typename BasicJson::number_unsigned_t>(_var));
    } else if constexpr (std::is_integral_v<U>) {
      return BasicJson(static_cast<typename BasicJson::number_integer_t>(_var));
    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
  }

  mutable BasicJson root_;
};

template <class T>
struct is_nlohmann_writer : std::false_type {};

template <class BasicJson>
struct is_nlohmann_writer<Writer<BasicJson>> : std::true_type {};

template <class T>
inline constexpr bool is_nlohmann_writer_v =
    is_nlohmann_writer<std::remove_cvref_t<T>>::value;

}  // namespace rfl::nlohmann

#endif
