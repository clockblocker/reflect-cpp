#ifndef RFL_CAPNPROTO_IS_NAMED_TYPE_HPP_
#define RFL_CAPNPROTO_IS_NAMED_TYPE_HPP_

#include <type_traits>

#include "../parsing/schema/Type.hpp"

namespace rfl::capnproto {

inline bool is_named_type(const parsing::schema::Type& _type) {
  return _type.variant_.visit([&](const auto& _v) -> bool {
    using T = std::remove_cvref_t<decltype(_v)>;
    if constexpr (std::is_same_v<T, parsing::schema::Type::Description> ||
                  std::is_same_v<T, parsing::schema::Type::Deprecated>) {
      return is_named_type(*_v.type_);
    } else {
      return std::is_same<T, parsing::schema::Type::Object>() ||
             std::is_same<T, parsing::schema::Type::AnyOf>() ||
             std::is_same<T, parsing::schema::Type::StringMap>() ||
             std::is_same<T, parsing::schema::Type::Literal>();
    }
  });
}

}  // namespace rfl::capnproto

#endif

