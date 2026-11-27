#ifndef RFL_NLOHMANN_ADL_SERIALIZER_HPP_
#define RFL_NLOHMANN_ADL_SERIALIZER_HPP_

#include <nlohmann/json.hpp>
#include <type_traits>
#include <utility>

#include "read.hpp"
#include "write.hpp"

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

#if defined(__cpp_exceptions)

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
      throw BasicJsonType::other_error::create(501, res.error().what(), &_j);
    }
    return std::move(*res);
  }
};

NLOHMANN_JSON_NAMESPACE_END

#endif

#endif
