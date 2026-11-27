#ifndef RFL_PARSING_PARSER_OPTIONAL_HPP_
#define RFL_PARSING_PARSER_OPTIONAL_HPP_

#include <map>
#include <optional>
#include <type_traits>

#include "../Ref.hpp"
#include "../Result.hpp"
#include "../always_false.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "schema/Type.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "schemaful/IsSchemafulWriter.hpp"
#include "schemaful/OptionalReader.hpp"

namespace rfl::parsing {

template <class T>
struct is_optional : std::false_type {};

template <class T>
struct is_optional<std::optional<T>> : std::true_type {};

template <class T>
constexpr bool is_optional_v = is_optional<std::remove_cvref_t<T>>::value;

template <class R, class W, class T, class ProcessorsType>
struct ParserOptional {
  using InputVarType = typename R::InputVarType;

  using ParentType = Parent<W>;

  /**
   * @brief Reads an optional value from the input.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @return A Result containing the parsed optional value or an error.
   */
  static Result<std::optional<T>> read(const R& _r,
                                       const InputVarType& _var) noexcept {
    if constexpr (schemaful::IsSchemafulReader<R>) {
      using O = schemaful::OptionalReader<R, W, std::remove_cvref_t<T>,
                                          ProcessorsType>;
      const auto to_optional = [&](const auto& _u) -> Result<std::optional<T>> {
        return _r.template read_union<std::optional<T>, O>(_u);
      };
      return _r.to_union(_var).and_then(to_optional);
    } else {
      if (_r.is_empty(_var)) {
        return std::optional<T>();
      }
      const auto to_opt = [](auto&& _t) {
        return std::make_optional<T>(std::forward<decltype(_t)>(_t));
      };
      return Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::read(_r,
                                                                        _var)
          .transform(to_opt);
    }
  }

  /**
   * @brief Overlays an optional value from the input onto an existing optional.
   *
   * If the input is null/empty, resets to `std::nullopt`. If the base holds a
   * value, recursively overlays onto it; otherwise parses a fresh value.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @param _base The existing optional value.
   * @return A Result containing the overlaid optional value or an error.
   */
  static Result<std::optional<T>> read_into(
      const R& _r, const InputVarType& _var,
      std::optional<T> _base) noexcept {
    if constexpr (schemaful::IsSchemafulReader<R>) {
      return read(_r, _var);
    } else {
      if (_r.is_empty(_var)) {
        return std::optional<T>();
      }
      try {
        auto res =
            _base.has_value()
                ? read_into_or_read<R, W, std::remove_cvref_t<T>,
                                    ProcessorsType>(_r, _var, std::move(*_base))
                : Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::read(
                      _r, _var);
        if (!res) {
          return error(std::move(res).error());
        }
        return make_result_safe<Result<std::optional<T>>>(
            std::make_optional<T>(std::move(*res)));
      } catch (std::exception& e) {
        return error(e.what());
      }
    }
  }

  static std::optional<Error> read_into_in_place(
      const R& _r, const InputVarType& _var, std::optional<T>* _base) noexcept {
    using CleanT = std::remove_cvref_t<T>;
    if constexpr (schemaful::IsSchemafulReader<R>) {
      auto res = read(_r, _var);
      if (!res) {
        return std::move(res).error();
      }
      try {
        *_base = std::move(*res);
        return std::nullopt;
      } catch (std::exception& e) {
        _base->reset();
        return Error(e.what());
      }
    } else {
      if (_r.is_empty(_var)) {
        _base->reset();
        return std::nullopt;
      }
      try {
        if (_base->has_value()) {
          if constexpr (requires(CleanT* _p) {
                          Parser<R, W, CleanT, ProcessorsType>::
                              read_into_in_place(_r, _var, _p);
                        }) {
            return Parser<R, W, CleanT, ProcessorsType>::read_into_in_place(
                _r, _var, &(**_base));
          } else {
            auto res = read_into_or_read<R, W, CleanT, ProcessorsType>(
                _r, _var, std::move(**_base));
            if (!res) {
              return std::move(res).error();
            }
            _base->emplace(std::move(*res));
            return std::nullopt;
          }
        }
        auto res = Parser<R, W, CleanT, ProcessorsType>::read(_r, _var);
        if (!res) {
          return std::move(res).error();
        }
        _base->emplace(std::move(*res));
        return std::nullopt;
      } catch (std::exception& e) {
        _base->reset();
        return Error(e.what());
      }
    }
  }

  /**
   * @brief Writes an optional value to the output.
   *
   * @tparam P The type of the parent.
   * @param _w The writer to use.
   * @param _o The optional value to write.
   * @param _parent The parent object.
   */
  template <class P>
  static void write(const W& _w, const std::optional<T>& _o, const P& _parent) {
    if constexpr (schemaful::IsSchemafulWriter<W>) {
      auto u = ParentType::add_union(_w, _parent);
      using UnionType = typename ParentType::template Union<decltype(u)>;
      auto p =
          UnionType{.index_ = static_cast<size_t>(_o ? 0 : 1), .union_ = &u};
      if (_o) {
        Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::write(_w, *_o, p);
      } else {
        ParentType::add_null(_w, p);
      }
    } else {
      if (_o) {
        Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::write(_w, *_o,
                                                                    _parent);
      } else {
        ParentType::add_null(_w, _parent);
      }
    }
  }

  /**
   * @brief Generates the schema for the optional value.
   *
   * @param _definitions The map of definitions to add to.
   * @return The schema type.
   */
  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    using U = std::remove_cvref_t<T>;
    return schema::Type{schema::Type::Optional{Ref<schema::Type>::make(
        Parser<R, W, U, ProcessorsType>::to_schema(_definitions))}};
  }
};

}  // namespace rfl::parsing

#endif
