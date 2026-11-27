#ifndef RFL_PARSING_PARSER_ALIAS_HPP_
#define RFL_PARSING_PARSER_ALIAS_HPP_

#include <map>
#include <string>
#include <type_traits>
#include <utility>

#include "../Alias.hpp"
#include "../Result.hpp"
#include "../internal/StringLiteral.hpp"
#include "Parser_base.hpp"
#include "schema/Type.hpp"

namespace rfl::parsing {

template <class T>
struct is_alias : std::false_type {};

template <internal::StringLiteral _name, class T>
struct is_alias<Alias<_name, T>> : std::true_type {
  using element_type = T;
  static constexpr auto name = _name;
};

template <class T>
constexpr bool is_alias_v = is_alias<std::remove_cvref_t<T>>::value;

template <class R, class W, class T, class ProcessorsType>
struct ParserAlias;

template <class R, class W, internal::StringLiteral _name, class T,
          class ProcessorsType>
struct ParserAlias<R, W, Alias<_name, T>, ProcessorsType> {
  using InputVarType = typename R::InputVarType;

  /**
   * @brief Reads an aliased value from the input.
   */
  static Result<Alias<_name, T>> read(const R& _r,
                                      const InputVarType& _var) noexcept {
    const auto to_alias = [](auto&& _t) {
      return Alias<_name, T>(std::forward<decltype(_t)>(_t));
    };
    return Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::read(_r, _var)
        .transform(to_alias);
  }

  /**
   * @brief Writes an aliased value to the output.
   */
  template <class P>
  static void write(const W& _w, const Alias<_name, T>& _alias,
                    const P& _parent) {
    Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::write(
        _w, _alias.value(), _parent);
  }

  /**
   * @brief Generates the schema for the aliased value.
   */
  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    return Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::to_schema(
        _definitions);
  }
};

}  // namespace rfl::parsing

#endif
