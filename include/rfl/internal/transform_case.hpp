#ifndef RFL_INTERNAL_TRANSFORMSNAKECASE_HPP_
#define RFL_INTERNAL_TRANSFORMSNAKECASE_HPP_

#include <array>
#include <cstddef>
#include <string_view>

#include "StringLiteral.hpp"

namespace rfl::internal {

constexpr bool is_upper(char c) { return c >= 'A' && c <= 'Z'; }

constexpr char to_upper(char c) {
  return c >= 'a' && c <= 'z' ? c + ('A' - 'a') : c;
}

constexpr char to_lower(char c) { return is_upper(c) ? c - ('A' - 'a') : c; }

enum class CaseTransformOp : unsigned char {
  snake_to_camel,
  snake_to_pascal,
  camel_to_snake,
  snake_to_kebab,
  to_all_caps,
};

/// Computes the exact output length of applying `_op` to `_src[0.._src_len)`.
consteval std::size_t case_transform_output_len(const CaseTransformOp _op,
                                                const char* _src,
                                                const std::size_t _src_len) {
  switch (_op) {
    case CaseTransformOp::to_all_caps:
    case CaseTransformOp::snake_to_kebab:
      return _src_len;
    case CaseTransformOp::snake_to_camel:
    case CaseTransformOp::snake_to_pascal: {
      std::size_t non_underscores = 0;
      for (std::size_t i = 0; i < _src_len; ++i) {
        if (_src[i] != '_') {
          ++non_underscores;
        }
      }
      if (non_underscores == 0) {
        return 0;
      }
      // `_src[0]` is always emitted (including a leading `_`), followed by all
      // non-underscore characters in `_src[1.._src_len)`.
      return (_src[0] == '_') ? (non_underscores + 1) : non_underscores;
    }
    case CaseTransformOp::camel_to_snake: {
      std::size_t out_len = 0;
      for (std::size_t i = 0; i < _src_len; ++i) {
        if (is_upper(_src[i]) && i > 0) {
          ++out_len;
        }
        ++out_len;
      }
      return out_len;
    }
  }
  return _src_len;
}

/// Applies a single case-transform op from `_src[0.._src_len)` into `_dst`,
/// returning the exact number of characters written.
consteval std::size_t apply_case_transform_op(const CaseTransformOp _op,
                                              const char* _src,
                                              const std::size_t _src_len,
                                              char* _dst) {
  switch (_op) {
    case CaseTransformOp::to_all_caps: {
      for (std::size_t i = 0; i < _src_len; ++i) {
        _dst[i] = to_upper(_src[i]);
      }
      return _src_len;
    }
    case CaseTransformOp::snake_to_kebab: {
      for (std::size_t i = 0; i < _src_len; ++i) {
        _dst[i] = _src[i] == '_' ? '-' : _src[i];
      }
      return _src_len;
    }
    case CaseTransformOp::snake_to_camel:
    case CaseTransformOp::snake_to_pascal: {
      const bool capitalize = (_op == CaseTransformOp::snake_to_pascal);
      std::size_t non_underscores = 0;
      for (std::size_t i = 0; i < _src_len; ++i) {
        if (_src[i] != '_') {
          ++non_underscores;
        }
      }
      if (non_underscores == 0) {
        return 0;
      }
      _dst[0] = capitalize ? to_upper(_src[0]) : to_lower(_src[0]);
      std::size_t shift = 1;
      for (std::size_t i = 1; i < _src_len; ++i) {
        if (_src[i] == '_') {
          continue;
        }
        _dst[shift++] = _src[i - 1] == '_' ? to_upper(_src[i]) : _src[i];
      }
      return shift;
    }
    case CaseTransformOp::camel_to_snake: {
      std::size_t out_len = 0;
      for (std::size_t i = 0; i < _src_len; ++i) {
        if (is_upper(_src[i])) {
          if (i > 0) {
            _dst[out_len++] = '_';
          }
          _dst[out_len++] = to_lower(_src[i]);
        } else {
          _dst[out_len++] = _src[i];
        }
      }
      return out_len;
    }
  }
  return _src_len;
}

template <internal::StringLiteral _name, CaseTransformOp _op>
consteval auto apply_case_transform_literal() {
  constexpr auto src = _name.string_view();
  constexpr auto len = case_transform_output_len(_op, src.data(), src.size());
  auto result = std::array<char, len + 1>{};
  if constexpr (len > 0) {
    apply_case_transform_op(_op, src.data(), src.size(), result.data());
  }
  return StringLiteral<len + 1>(result);
}

/// Transforms the field name to all uppercase letters.
template <internal::StringLiteral _name>
consteval auto to_all_caps() {
  return apply_case_transform_literal<_name, CaseTransformOp::to_all_caps>();
}

/// Transforms the field name from snake_case to camelCase or PascalCase.
template <internal::StringLiteral _name, bool _capitalize>
consteval auto transform_snake_case() {
  return apply_case_transform_literal<
      _name, _capitalize ? CaseTransformOp::snake_to_pascal
                         : CaseTransformOp::snake_to_camel>();
}

/// Transforms the field name from camel case to snake case.
template <internal::StringLiteral _name>
consteval auto transform_camel_case() {
  return apply_case_transform_literal<_name, CaseTransformOp::camel_to_snake>();
}

/// Transforms the field name from snake_case to kebab-case.
template <internal::StringLiteral _name>
consteval auto transform_snake_to_kebab() {
  return apply_case_transform_literal<_name, CaseTransformOp::snake_to_kebab>();
}

}  // namespace rfl::internal

#endif
