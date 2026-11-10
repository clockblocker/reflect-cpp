#ifndef RFL_CLI_PARSER_HPP_
#define RFL_CLI_PARSER_HPP_

#include <string>
#include <type_traits>

#include "../DefaultVal.hpp"
#include "../Result.hpp"
#include "../generic/Writer.hpp"
#include "../parsing/AreReaderAndWriter.hpp"
#include "../parsing/Parser.hpp"
#include "../parsing/ParserDefaultVal.hpp"
#include "../parsing/ParserWString.hpp"
#include "Reader.hpp"

namespace rfl::parsing {

/// With rfl::cli::BlankItemsAsMissing, a blank item in a comma-separated list
/// is a missing value. A DefaultVal element then takes its default value,
/// T{}, much like a missing DefaultVal field keeps its default. Everything
/// else is handled by ParserDefaultVal.
template <class W, class T, class ProcessorsType>
  requires AreReaderAndWriter<cli::Reader, W, DefaultVal<T>>
struct Parser<cli::Reader, W, DefaultVal<T>, ProcessorsType>
    : public ParserDefaultVal<cli::Reader, W, DefaultVal<T>, ProcessorsType> {
  static Result<DefaultVal<T>> read(const cli::Reader& _r,
                                    const cli::CliVarType& _var) noexcept {
    if constexpr (std::is_default_constructible_v<
                      typename DefaultVal<T>::Type>) {
      if (cli::internal::is_blank_item(_var)) {
        return DefaultVal<T>();
      }
    }
    return ParserDefaultVal<cli::Reader, W, DefaultVal<T>,
                            ProcessorsType>::read(_r, _var);
  }
};

/// With rfl::cli::BlankItemsAsMissing, a blank std::wstring item is an
/// error, as a blank std::string item is. ParserWString would read it as an
/// empty string, because the reader reports it as empty. Everything else is
/// handled by ParserWString.
template <class W, class ProcessorsType>
  requires AreReaderAndWriter<cli::Reader, W, std::wstring>
struct Parser<cli::Reader, W, std::wstring, ProcessorsType>
    : public ParserWString<cli::Reader, W, ProcessorsType> {
  static Result<std::wstring> read(const cli::Reader& _r,
                                   const cli::CliVarType& _var) noexcept {
    if (cli::internal::is_blank_item(_var)) {
      return error(_r.template to_basic_type<std::string>(_var).error());
    }
    return ParserWString<cli::Reader, W, ProcessorsType>::read(_r, _var);
  }
};

}  // namespace rfl::parsing

namespace rfl::cli {

template <class T, class ProcessorsType>
using Parser = parsing::Parser<Reader, generic::Writer, T, ProcessorsType>;

}  // namespace rfl::cli

#endif
