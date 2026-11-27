#ifndef RFL_NLOHMANN_WRITE_HPP_
#define RFL_NLOHMANN_WRITE_HPP_

#include <nlohmann/json.hpp>
#include <tuple>
#include <type_traits>
#include <utility>

#include "../Processors.hpp"
#include "../parsing/Parent.hpp"
#include "Parser.hpp"
#include "Reader.hpp"
#include "Writer.hpp"

namespace rfl::nlohmann {

/// Serializes a C++ object directly into a specified nlohmann::basic_json type
/// (e.g., nlohmann::ordered_json) with optional reflect-cpp processors.
template <class BasicJson, class... Ps>
  requires is_basic_json_v<BasicJson>
BasicJson write(const auto& _obj) {
  using T = std::remove_cvref_t<decltype(_obj)>;
  using ParentType = parsing::Parent<Writer<BasicJson>>;
  auto w = Writer<BasicJson>();
  Parser<T, Processors<Ps...>, BasicJson>::write(
      w, _obj, typename ParentType::Root{});
  return std::move(w.root());
}

/// Serializes a C++ object directly into nlohmann::json with optional
/// reflect-cpp processors.
template <class... Ps>
  requires(sizeof...(Ps) == 0 ||
           !is_basic_json_v<std::tuple_element_t<0, std::tuple<Ps..., void>>>)
::nlohmann::json write(const auto& _obj) {
  return write<::nlohmann::json, Ps...>(_obj);
}

}  // namespace rfl::nlohmann

#endif
