#ifndef RFL_INTERNAL_DIRECTFIELDNAMES_HPP_
#define RFL_INTERNAL_DIRECTFIELDNAMES_HPP_

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../CamelCaseToSnakeCase.hpp"
#include "../Processors.hpp"
#include "../SnakeCaseToCamelCase.hpp"
#include "../SnakeCaseToKebabCase.hpp"
#include "../SnakeCaseToPascalCase.hpp"
#include "../ToAllCaps.hpp"
#include "../num_fields.hpp"
#include "../parsing/find_field_index.hpp"
#include "bind_to_tuple.hpp"
#include "get_field_names.hpp"
#include "is_field.hpp"
#include "is_flatten_field.hpp"
#include "nth_element_t.hpp"
#include "transform_case.hpp"
#ifndef REFLECTCPP_USE_CPP26_REFLECTION
#include "cpp20/get_ith_field_from_fake_object.hpp"
#endif

namespace rfl::internal {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-var-template"
#pragma clang diagnostic ignored "-Wundefined-inline"
#pragma clang diagnostic ignored "-Wundefined-internal"
#endif

enum class DirectCaseOp : unsigned char {
  snake_to_camel,
  snake_to_pascal,
  camel_to_snake,
  snake_to_kebab,
  to_all_caps,
};

template <DirectCaseOp... Ops>
struct DirectCasePipeline {};

template <class A, class B>
struct concat_case_pipelines;

template <DirectCaseOp... A, DirectCaseOp... B>
struct concat_case_pipelines<DirectCasePipeline<A...>,
                             DirectCasePipeline<B...>> {
  using type = DirectCasePipeline<A..., B...>;
};

template <class P>
struct extract_case_pipeline {
  using type = DirectCasePipeline<>;
};

template <>
struct extract_case_pipeline<SnakeCaseToCamelCase> {
  using type = DirectCasePipeline<DirectCaseOp::snake_to_camel>;
};

template <>
struct extract_case_pipeline<SnakeCaseToPascalCase> {
  using type = DirectCasePipeline<DirectCaseOp::snake_to_pascal>;
};

template <>
struct extract_case_pipeline<CamelCaseToSnakeCase> {
  using type = DirectCasePipeline<DirectCaseOp::camel_to_snake>;
};

template <>
struct extract_case_pipeline<SnakeCaseToKebabCase> {
  using type = DirectCasePipeline<DirectCaseOp::snake_to_kebab>;
};

template <>
struct extract_case_pipeline<ToAllCaps> {
  using type = DirectCasePipeline<DirectCaseOp::to_all_caps>;
};

template <>
struct extract_case_pipeline<Processors<>> {
  using type = DirectCasePipeline<>;
};

template <class Head, class... Tail>
struct extract_case_pipeline<Processors<Head, Tail...>> {
  using type = typename concat_case_pipelines<
      typename extract_case_pipeline<Head>::type,
      typename extract_case_pipeline<Processors<Tail...>>::type>::type;
};

template <class ProcessorsType>
using direct_case_pipeline_t =
    typename extract_case_pipeline<ProcessorsType>::type;

/// Total flattened leaf count for an aggregate struct `T` (recursing into
/// plain aggregate `rfl::Flatten<Sub>` members).
template <class T>
consteval std::size_t direct_flattened_field_count();

template <class FieldT>
consteval std::size_t direct_single_field_count() {
  using U = std::remove_cvref_t<FieldT>;
  if constexpr (is_flatten_field_v<U>) {
    using Sub = std::remove_cvref_t<typename U::Type>;
    return direct_flattened_field_count<Sub>();
  } else {
    return 1;
  }
}

template <class T>
consteval std::size_t direct_flattened_field_count() {
  using Type = std::remove_cvref_t<T>;
  using Count = decltype(apply_to_fields(
      std::declval<Type&>(), []<class... Fields>(Fields&...) {
        return std::integral_constant<
            std::size_t, (std::size_t{0} + ... +
                          direct_single_field_count<Fields>())>{};
      }));
  return Count::value;
}

template <class T>
inline constexpr std::size_t direct_flattened_field_count_v =
    direct_flattened_field_count<T>();

namespace detail {

#ifndef REFLECTCPP_USE_CPP26_REFLECTION
using DirectNameInfo = cpp20::DirectFieldNameInfo;
#else
using DirectNameInfo = cpp26::DirectFieldNameInfo;
#endif

template <class T, std::size_t I>
struct direct_member_type {
  using Type = std::remove_cvref_t<T>;
  using ptr_type = decltype(apply_to_fields(
      std::declval<Type&>(), []<class... Fields>(Fields&...) {
        return static_cast<nth_element_t<I, Fields...>*>(nullptr);
      }));
  using raw_type = std::remove_pointer_t<ptr_type>;
};

template <class T>
consteval std::array<DirectNameInfo, direct_flattened_field_count_v<T>>
extract_flattened_raw_names();

template <class T, std::size_t I>
#if defined(__GNUC__) && !defined(__clang__)
[[gnu::no_sanitize_undefined]]
#endif
consteval void append_member_names(DirectNameInfo* _out, std::size_t& _pos) {
  using Type = std::remove_cvref_t<T>;
  using FieldT =
      std::remove_cvref_t<typename direct_member_type<Type, I>::raw_type>;
  if constexpr (is_flatten_field_v<FieldT>) {
    using Sub = std::remove_cvref_t<typename FieldT::Type>;
    const auto sub_names = extract_flattened_raw_names<Sub>();
    for (const auto& info : sub_names) {
      _out[_pos++] = info;
    }
  } else {
#ifndef REFLECTCPP_USE_CPP26_REFLECTION
#ifdef __clang__
    _out[_pos++] = cpp20::get_direct_field_name_info<
        Type, cpp20::wrap(cpp20::get_ith_field_from_fake_object<Type, I>())>();
#else
    _out[_pos++] = cpp20::get_direct_field_name_info<
        Type, cpp20::get_ith_field_from_fake_object<Type, I>()>();
#endif
#else
    _out[_pos++] = cpp26::get_direct_field_name_info<Type, I>();
#endif
  }
}

template <class T, std::size_t... Is>
consteval void append_all_member_names(DirectNameInfo* _out, std::size_t& _pos,
                                       std::index_sequence<Is...>) {
  (void)_out;
  (void)_pos;
  (append_member_names<T, Is>(_out, _pos), ...);
}

template <class T>
consteval std::array<DirectNameInfo, direct_flattened_field_count_v<T>>
extract_flattened_raw_names() {
  using Type = std::remove_cvref_t<T>;
  std::array<DirectNameInfo, direct_flattened_field_count_v<Type>> out{};
  std::size_t pos = 0;
  append_all_member_names<Type>(out.data(), pos,
                                std::make_index_sequence<num_fields<Type>>{});
  return out;
}

/// Applies a single case-transform op from `src[0..src_len)` into `dst`,
/// returning the resulting length. Mirrors `include/rfl/internal/transform_case.hpp`.
consteval std::size_t apply_one_case_op(const DirectCaseOp _op,
                                        const char* _src,
                                        const std::size_t _src_len,
                                        char* _dst) {
  switch (_op) {
    case DirectCaseOp::to_all_caps: {
      for (std::size_t i = 0; i < _src_len; ++i) {
        _dst[i] = to_upper(_src[i]);
      }
      return _src_len;
    }
    case DirectCaseOp::snake_to_kebab: {
      for (std::size_t i = 0; i < _src_len; ++i) {
        _dst[i] = _src[i] == '_' ? '-' : _src[i];
      }
      return _src_len;
    }
    case DirectCaseOp::snake_to_camel:
    case DirectCaseOp::snake_to_pascal: {
      const bool capitalize = (_op == DirectCaseOp::snake_to_pascal);
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
    case DirectCaseOp::camel_to_snake: {
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

template <DirectCaseOp... Ops>
consteval std::size_t worst_case_scratch_for_len(const std::size_t _raw_len,
                                                 DirectCasePipeline<Ops...>) {
  std::size_t cap = _raw_len > 0 ? _raw_len : 1;
  if constexpr (sizeof...(Ops) > 0) {
    constexpr std::array<DirectCaseOp, sizeof...(Ops)> ops = {Ops...};
    for (const auto op : ops) {
      if (op == DirectCaseOp::camel_to_snake) {
        // Every char after index 0 could be uppercase -> at most 2x growth.
        cap *= 2;
      }
    }
  }
  return cap;
}

template <std::size_t _scratch_cap, DirectCaseOp... Ops>
consteval std::size_t transform_name_into(const DirectNameInfo _info,
                                          char* _out_buf,
                                          DirectCasePipeline<Ops...>) {
  constexpr std::size_t kNumOps = sizeof...(Ops);
  if constexpr (kNumOps == 0) {
    for (std::size_t i = 0; i < _info.name.size(); ++i) {
      _out_buf[i] = _info.name[i];
    }
    return _info.name.size();
  } else {
    if (_info.is_renamed) {
      for (std::size_t i = 0; i < _info.name.size(); ++i) {
        _out_buf[i] = _info.name[i];
      }
      return _info.name.size();
    }
    constexpr std::array<DirectCaseOp, kNumOps> ops = {Ops...};
    std::array<char, _scratch_cap> a{};
    std::array<char, _scratch_cap> b{};
    std::size_t len = _info.name.size();
    for (std::size_t i = 0; i < len; ++i) {
      a[i] = _info.name[i];
    }
    bool in_a = true;
    for (const auto op : ops) {
      const char* cur = in_a ? a.data() : b.data();
      // Every case-transform processor checks `name != "xml_content"` before
      // transforming, so an intermediate "xml_content" stops further ops.
      if (std::string_view(cur, len) == "xml_content") {
        break;
      }
      if (in_a) {
        len = apply_one_case_op(op, a.data(), len, b.data());
      } else {
        len = apply_one_case_op(op, b.data(), len, a.data());
      }
      in_a = !in_a;
    }
    const char* final_src = in_a ? a.data() : b.data();
    for (std::size_t i = 0; i < len; ++i) {
      _out_buf[i] = final_src[i];
    }
    return len;
  }
}

}  // namespace detail

/// Value-level flattened field name storage and lookup table keyed solely by
/// `(std::remove_cvref_t<T>, CasePipeline)` — never by `StringLiteral` non-type
/// template parameters and shared across `DefaultIfMissing`, `NoExtraFields`,
/// `FailFast`, `read`, `write`, and `to_schema`. All names are copied into a
/// compact null-terminated char buffer so `__PRETTY_FUNCTION__` strings never
/// leak into `.rodata`.
template <class T, class CasePipeline = DirectCasePipeline<>>
struct DirectFieldNames {
  using CleanT = std::remove_cvref_t<T>;
  static constexpr std::size_t size_ = direct_flattened_field_count_v<CleanT>;

 private:
  static consteval std::size_t compute_scratch_cap() {
    const auto raw = detail::extract_flattened_raw_names<CleanT>();
    std::size_t max_len = 1;
    for (const auto& info : raw) {
      if (info.name.size() > max_len) {
        max_len = info.name.size();
      }
    }
    return detail::worst_case_scratch_for_len(max_len, CasePipeline{});
  }

  static constexpr std::size_t scratch_cap_ = compute_scratch_cap();

  static consteval auto compute_lengths() {
    const auto raw = detail::extract_flattened_raw_names<CleanT>();
    std::array<std::size_t, size_> lens{};
    std::array<char, scratch_cap_> scratch{};
    for (std::size_t i = 0; i < size_; ++i) {
      lens[i] = detail::transform_name_into<scratch_cap_>(
          raw[i], scratch.data(), CasePipeline{});
    }
    return lens;
  }

  static constexpr auto lengths_ = compute_lengths();

  static consteval std::size_t buffer_size() {
    std::size_t total = size_;
    for (const auto len : lengths_) {
      total += len;
    }
    return total;
  }

  static consteval auto make_buffer() {
    const auto raw = detail::extract_flattened_raw_names<CleanT>();
    std::array<char, buffer_size()> buffer{};
    std::size_t offset = 0;
    for (std::size_t i = 0; i < size_; ++i) {
      const auto written = detail::transform_name_into<scratch_cap_>(
          raw[i], buffer.data() + offset, CasePipeline{});
      offset += written;
      buffer[offset++] = '\0';
    }
    return buffer;
  }

  static constexpr auto buffer_ = make_buffer();

  static consteval auto make_names() {
    std::array<std::string_view, size_> names{};
    std::size_t offset = 0;
    for (std::size_t i = 0; i < size_; ++i) {
      names[i] = std::string_view(buffer_.data() + offset, lengths_[i]);
      offset += lengths_[i] + 1;
    }
    return names;
  }

  static consteval auto make_lookup() {
    std::array<parsing::detail::FieldNameEntry, size_> table{};
    for (std::size_t i = 0; i < size_; ++i) {
      table[i] = {names_[i], static_cast<int>(i)};
    }
    if constexpr (size_ > 8) {
      parsing::detail::sort_field_name_table(table);
    }
    return table;
  }

  static consteval bool has_unique_names() {
    if constexpr (size_ <= 8) {
      for (std::size_t i = 0; i < size_; ++i) {
        for (std::size_t j = i + 1; j < size_; ++j) {
          if (names_[i] == names_[j]) {
            return false;
          }
        }
      }
      return true;
    } else {
      for (std::size_t i = 1; i < size_; ++i) {
        if (lookup_[i - 1].name == lookup_[i].name) {
          return false;
        }
      }
      return true;
    }
  }

 public:
  static constexpr auto names_ = make_names();
  static constexpr auto lookup_ = make_lookup();
  static_assert(has_unique_names(), "Duplicate field names are not allowed.");
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif

}  // namespace rfl::internal

#endif
