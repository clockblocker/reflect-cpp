#ifndef RFL_NLOHMANN_READ_HPP_
#define RFL_NLOHMANN_READ_HPP_

#include "../Processors.hpp"
#include "../Result.hpp"
#include "../internal/wrap_in_rfl_array_t.hpp"
#include "Parser.hpp"
#include "Reader.hpp"

namespace rfl::nlohmann {

/// Parses a C++ object from a nlohmann::basic_json value using reflection.
template <class T, class... Ps, class BasicJson>
  requires is_basic_json_v<BasicJson>
Result<::rfl::internal::wrap_in_rfl_array_t<T>> read(const BasicJson& _j) {
  const auto r = Reader<BasicJson>();
  return Parser<T, Processors<Ps...>, BasicJson>::read(
      r, typename Reader<BasicJson>::InputVarType(&_j));
}

}  // namespace rfl::nlohmann

#endif
