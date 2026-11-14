#ifndef RFL_PARSING_PARSER_IMPLICITVALUE_HPP_
#define RFL_PARSING_PARSER_IMPLICITVALUE_HPP_

#include <map>
#include <type_traits>

#include "../ImplicitValue.hpp"
#include "../Result.hpp"
#include "../internal/StringLiteral.hpp"
#include "Parser_base.hpp"
#include "schema/Type.hpp"

namespace rfl::parsing {

template <class R, class W, class T, class ProcessorsType>
struct ParserImplicitValue;

template <class R, class W, class T, internal::StringLiteral _implicit,
          class ProcessorsType>
struct ParserImplicitValue<R, W, ImplicitValue<T, _implicit>, ProcessorsType> {
  using InputVarType = typename R::InputVarType;

  /**
   * @brief Reads an ImplicitValue from the input.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @return A Result containing the parsed value or an error.
   */
  static Result<ImplicitValue<T, _implicit>> read(
      const R& _r, const InputVarType& _var) noexcept {
    const auto to_implicit_value = [](auto&& _t) {
      return ImplicitValue<T, _implicit>(std::move(_t));
    };
    return Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::read(_r, _var)
        .transform(to_implicit_value);
  }

  /**
   * @brief Writes an ImplicitValue to the output.
   *
   * @tparam P The type of the parent.
   * @param _w The writer to use.
   * @param _implicit_value The ImplicitValue to write.
   * @param _parent The parent object.
   */
  template <class P>
  static void write(const W& _w,
                    const ImplicitValue<T, _implicit>& _implicit_value,
                    const P& _parent) {
    Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::write(
        _w, _implicit_value.value(), _parent);
  }

  /**
   * @brief Generates the schema for the ImplicitValue, which is the schema of
   * the underlying type.
   *
   * @param _definitions The map of definitions to add to.
   * @return The schema type.
   */
  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    return Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::to_schema(
        _definitions);
  }
};

}  // namespace rfl::parsing

#endif
