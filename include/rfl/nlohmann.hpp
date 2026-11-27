#ifndef RFL_NLOHMANN_HPP_
#define RFL_NLOHMANN_HPP_

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "../rfl.hpp"
#include "nlohmann/Parser.hpp"
#include "nlohmann/Reader.hpp"
#include "nlohmann/Writer.hpp"
#include "nlohmann/read.hpp"
#include "nlohmann/write.hpp"

namespace rfl::nlohmann {

/// Opt-in trait enabling nlohmann::adl_serializer for a reflected type T.
/// Specialize to std::true_type for types that should support j.get<T>() and
/// nlohmann::json j = obj; without affecting any other types.
template <class T>
struct enable_adl_serializer : std::false_type {};

template <class T>
inline constexpr bool enable_adl_serializer_v =
    enable_adl_serializer<std::remove_cvref_t<T>>::value;

}  // namespace rfl::nlohmann

NLOHMANN_JSON_NAMESPACE_BEGIN

template <typename T>
struct adl_serializer<
    T, std::enable_if_t<::rfl::nlohmann::enable_adl_serializer_v<T>>> {
  template <typename BasicJsonType>
  static void to_json(BasicJsonType& _j, const T& _val) {
    _j = ::rfl::nlohmann::write<BasicJsonType>(_val);
  }

  template <typename BasicJsonType>
  static T from_json(const BasicJsonType& _j) {
    auto res = ::rfl::nlohmann::read<T>(_j);
    if (!res) {
      throw std::runtime_error(res.error().what());
    }
    return std::move(*res);
  }

  template <typename BasicJsonType>
  static void from_json(const BasicJsonType& _j, T& _val) {
    _val = from_json(_j);
  }
};

NLOHMANN_JSON_NAMESPACE_END

#endif
