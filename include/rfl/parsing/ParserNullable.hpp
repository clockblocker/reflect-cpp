#ifndef RFL_PARSING_PARSER_NULLABLE_HPP_
#define RFL_PARSING_PARSER_NULLABLE_HPP_

#include <map>
#include <optional>
#include <type_traits>

#include "../Nullable.hpp"
#include "../Ref.hpp"
#include "../Result.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "schema/Type.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "schemaful/IsSchemafulWriter.hpp"
#include "schemaful/OptionalReader.hpp"

namespace rfl::parsing {

template <class T>
constexpr bool is_nullable_wrapper_v = internal::is_nullable_wrapper_v<T>;

template <class R, class W, class T, class ProcessorsType>
struct ParserNullable {
  using InputVarType = typename R::InputVarType;
  using ParentType = Parent<W>;

  static Result<Nullable<T>> read(const R& _r,
                                  const InputVarType& _var) noexcept {
    if constexpr (schemaful::IsSchemafulReader<R>) {
      using O = schemaful::OptionalReader<R, W, std::remove_cvref_t<T>,
                                          ProcessorsType>;
      const auto to_nullable = [&](const auto& _u) -> Result<Nullable<T>> {
        return _r.template read_union<std::optional<T>, O>(_u).transform(
            [](std::optional<T>&& _opt) { return Nullable<T>(std::move(_opt)); });
      };
      return _r.to_union(_var).and_then(to_nullable);
    } else {
      if (_r.is_empty(_var)) {
        return Nullable<T>(std::nullopt);
      }
      const auto to_nullable = [](auto&& _t) {
        return Nullable<T>(std::forward<decltype(_t)>(_t));
      };
      return Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::read(_r,
                                                                        _var)
          .transform(to_nullable);
    }
  }

  template <class P>
  static void write(const W& _w, const Nullable<T>& _n, const P& _parent) {
    if constexpr (schemaful::IsSchemafulWriter<W>) {
      auto u = ParentType::add_union(_w, _parent);
      using UnionType = typename ParentType::template Union<decltype(u)>;
      auto p =
          UnionType{.index_ = static_cast<size_t>(_n.has_value() ? 0 : 1),
                    .union_ = &u};
      if (_n.has_value()) {
        Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::write(_w, *_n, p);
      } else {
        ParentType::add_null(_w, p);
      }
    } else {
      if (_n.has_value()) {
        Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::write(_w, *_n,
                                                                    _parent);
      } else {
        ParentType::add_null(_w, _parent);
      }
    }
  }

  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    using U = std::remove_cvref_t<T>;
    return schema::Type{schema::Type::Optional{Ref<schema::Type>::make(
        Parser<R, W, U, ProcessorsType>::to_schema(_definitions))}};
  }
};

}  // namespace rfl::parsing

#endif
