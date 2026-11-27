#ifndef RFL_PARSING_PARSER_TRISTATE_HPP_
#define RFL_PARSING_PARSER_TRISTATE_HPP_

#include <map>
#include <optional>
#include <type_traits>

#include "../Ref.hpp"
#include "../Result.hpp"
#include "../TriState.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "schema/Type.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "schemaful/IsSchemafulWriter.hpp"
#include "schemaful/OptionalReader.hpp"

namespace rfl::parsing {

template <class T>
constexpr bool is_tri_state_v = internal::is_tri_state_v<T>;

template <class R, class W, class T, class ProcessorsType>
struct ParserTriState {
  using InputVarType = typename R::InputVarType;
  using ParentType = Parent<W>;

  static Result<TriState<T>> read(const R& _r,
                                  const InputVarType& _var) noexcept {
    if constexpr (schemaful::IsSchemafulReader<R>) {
      using O = schemaful::OptionalReader<R, W, std::remove_cvref_t<T>,
                                          ProcessorsType>;
      const auto to_ts = [&](const auto& _u) -> Result<TriState<T>> {
        return _r.template read_union<std::optional<T>, O>(_u).transform(
            [](std::optional<T>&& _opt) {
              if (_opt.has_value()) {
                return TriState<T>(std::move(*_opt));
              }
              return TriState<T>::null();
            });
      };
      return _r.to_union(_var).and_then(to_ts);
    } else {
      if (_r.is_empty(_var)) {
        return TriState<T>::null();
      }
      const auto to_ts = [](auto&& _t) {
        return TriState<T>(std::forward<decltype(_t)>(_t));
      };
      return Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::read(_r,
                                                                        _var)
          .transform(to_ts);
    }
  }

  static Result<TriState<T>> read_into(const R& _r, const InputVarType& _var,
                                       TriState<T> _base) noexcept {
    if constexpr (schemaful::IsSchemafulReader<R>) {
      return read(_r, _var);
    } else {
      if (_r.is_empty(_var)) {
        return TriState<T>::null();
      }
      const auto to_ts = [](auto&& _t) {
        return TriState<T>(std::forward<decltype(_t)>(_t));
      };
      if (_base.has_value()) {
        return read_into_or_read<R, W, std::remove_cvref_t<T>, ProcessorsType>(
                   _r, _var, std::move(*_base))
            .transform(to_ts);
      }
      return Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::read(_r,
                                                                        _var)
          .transform(to_ts);
    }
  }

  template <class P>
  static void write(const W& _w, const TriState<T>& _ts, const P& _parent) {
    if constexpr (schemaful::IsSchemafulWriter<W>) {
      auto u = ParentType::add_union(_w, _parent);
      using UnionType = typename ParentType::template Union<decltype(u)>;
      auto p =
          UnionType{.index_ = static_cast<size_t>(_ts.has_value() ? 0 : 1),
                    .union_ = &u};
      if (_ts.has_value()) {
        Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::write(_w, *_ts,
                                                                    p);
      } else {
        ParentType::add_null(_w, p);
      }
    } else {
      if (_ts.has_value()) {
        Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::write(_w, *_ts,
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
