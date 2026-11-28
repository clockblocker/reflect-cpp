#ifndef RFL_PARSING_PARSER_BASIC_TYPE_HPP_
#define RFL_PARSING_PARSER_BASIC_TYPE_HPP_

#include <array>
#include <charconv>
#include <cstddef>
#include <map>
#include <string>
#include <system_error>
#include <type_traits>

#include "../Result.hpp"
#include "../always_false.hpp"
#include "../enums.hpp"
#include "../from_named_tuple.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/has_default_val_v.hpp"
#include "../internal/has_reflection_method_v.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/has_reflector.hpp"
#include "../internal/int64_as_string_v.hpp"
#include "../internal/is_basic_type.hpp"
#include "../internal/is_description.hpp"
#include "../internal/is_literal.hpp"
#include "../internal/is_validator.hpp"
#include "../internal/processed_t.hpp"
#include "../internal/ptr_cast.hpp"
#include "../internal/to_ptr_named_tuple.hpp"
#include "../to_view.hpp"
#include "AreReaderAndWriter.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "call_destructors_where_necessary.hpp"
#include "is_tagged_union_wrapper.hpp"
#include "make_type_name.hpp"
#include "schema/Type.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "schemaful/IsSchemafulWriter.hpp"

namespace rfl::parsing {

/// Default case - anything that cannot be explicitly matched.
template <class R, class W, class T, class ProcessorsType>
struct ParserBasicType {
 public:
  using InputVarType = typename R::InputVarType;

  using ParentType = Parent<W>;

  /**
   * @brief Reads a basic type from the input.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @return A Result containing the parsed value or an error.
   */
  static RFL_ALWAYS_INLINE Result<T> read(const R& _r,
                                          const InputVarType& _var)
      noexcept(!internal::int64_as_string_v<ProcessorsType>) {
    using U = std::remove_cvref_t<T>;
    if constexpr (internal::int64_as_string_v<ProcessorsType> &&
                  std::is_integral_v<U> && !std::is_same_v<U, bool> &&
                  sizeof(U) >= 8) {
      return _r.template to_basic_type<std::string>(_var).and_then(
          [](const std::string& _str) -> Result<T> {
            if (_str.empty()) {
              return error("Could not parse empty string as an integer.");
            }
            U parsed = 0;
            const char* const begin = _str.data();
            const char* const end = _str.data() + _str.size();
            const auto [ptr, ec] = std::from_chars(begin, end, parsed);
            if (ec == std::errc::result_out_of_range) {
              return error("Integer string '" + _str +
                           "' is out of range for the target type.");
            }
            if (ec != std::errc{} || ptr != end) {
              return error("Could not parse '" + _str + "' as an integer.");
            }
            return parsed;
          });
    } else {
      return _r.template to_basic_type<U>(_var);
    }
  }

  /**
   * @brief Writes a basic type to the output.
   *
   * @tparam P The type of the parent.
   * @param _w The writer to use.
   * @param _var The value to write.
   * @param _parent The parent object.
   */
  template <class P>
  static void write(const W& _w, const T& _var, const P& _parent) {
    using U = std::remove_cvref_t<T>;
    if constexpr (internal::int64_as_string_v<ProcessorsType> &&
                  std::is_integral_v<U> && !std::is_same_v<U, bool> &&
                  sizeof(U) >= 8) {
      std::array<char, 64> buf{};
      const auto [ptr, ec] =
          std::to_chars(buf.data(), buf.data() + buf.size(), _var);
      const auto str =
          ec == std::errc{}
              ? std::string(buf.data(),
                            static_cast<std::size_t>(ptr - buf.data()))
              : std::to_string(_var);
      ParentType::add_value(_w, str, _parent);
    } else {
      ParentType::add_value(_w, _var, _parent);
    }
  }

  /**
   * @brief Generates the schema for the basic type.
   *
   * @param _definitions The map of definitions to add to.
   * @return The schema type.
   */
  static schema::Type to_schema(std::map<std::string, schema::Type>*) {
    using U = std::remove_cvref_t<T>;
    using Type = schema::Type;
    if constexpr (internal::int64_as_string_v<ProcessorsType> &&
                  std::is_integral_v<U> && !std::is_same_v<U, bool> &&
                  sizeof(U) >= 8) {
      return Type{Type::String{
          .format_ = std::is_unsigned_v<U> ? "uint64" : "int64"}};

    } else if constexpr (std::is_same<U, bool>()) {
      return Type{Type::Boolean{}};

    } else if constexpr (std::is_same<U, std::int32_t>()) {
      return Type{Type::Int32{}};

    } else if constexpr (std::is_same<U, std::int64_t>()) {
      return Type{Type::Int64{}};

    } else if constexpr (std::is_same<U, std::uint32_t>()) {
      return Type{Type::UInt32{}};

    } else if constexpr (std::is_same<U, std::uint64_t>()) {
      return Type{Type::UInt64{}};

    } else if constexpr (std::is_integral<U>()) {
      return Type{Type::Integer{}};

    } else if constexpr (std::is_same<U, float>()) {
      return Type{Type::Float{}};

    } else if constexpr (std::is_floating_point_v<U>) {
      return Type{Type::Double{}};

    } else if constexpr (std::is_same<U, std::string>()) {
      return Type{Type::String{}};

    } else {
      static_assert(rfl::always_false_v<U>, "Unsupported type.");
    }
  }
};

}  // namespace rfl::parsing

#endif
