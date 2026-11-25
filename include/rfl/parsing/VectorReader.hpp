#ifndef RFL_PARSING_VECTORREADER_HPP_
#define RFL_PARSING_VECTORREADER_HPP_

#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <type_traits>

#include "../Result.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/fail_fast_v.hpp"
#include "Parser_base.hpp"
#include "is_inplace_vector.hpp"
#include "is_map_like.hpp"
#include "is_set_like.hpp"
#include "wrap_error.hpp"

namespace rfl::parsing {

template <class R, class W, class VecType, class ProcessorsType>
class VectorReader {
 private:
  using InputVarType = typename R::InputVarType;
  using T = typename VecType::value_type;

  static constexpr auto parsed_type() {
    if constexpr (is_map_like_v<VecType>) {
      using K = std::remove_cvref_t<typename T::first_type>;
      using V = std::remove_cvref_t<typename T::second_type>;
      return std::type_identity<std::pair<K, V>>{};
    } else {
      return std::type_identity<std::remove_cvref_t<T>>{};
    }
  }

  using ParsedType = typename decltype(parsed_type())::type;

 public:
  static constexpr bool fail_fast = internal::fail_fast_v<ProcessorsType>;

  /**
   * @brief Constructor.
   *
   * @param _r The reader to use.
   * @param _vec The vector to read into.
   */
  VectorReader(const R* _r, VecType* _vec)
      : num_inserted_(0), num_failed_(0), r_(_r), vec_(_vec) {}

  ~VectorReader() = default;

  std::optional<Error> reserve(const std::size_t _size) const noexcept {
    if constexpr (requires { vec_->reserve(_size); } &&
                  !is_inplace_vector_v<VecType> && !is_set_like_v<VecType> &&
                  !is_map_like_v<VecType>) {
      try {
        vec_->reserve(_size);
      } catch (const std::exception& e) {
        return Error(e.what());
      }
    }
    return std::nullopt;
  }

  /**
   * @brief Reads a single variable from the input.
   *
   * @param _var The input variable to read from.
   * @return An optional error.
   */
  RFL_ALWAYS_INLINE std::optional<Error> read(const InputVarType& _var) const {
    if constexpr (is_inplace_vector_v<VecType>) {
      const auto idx = index();
      if (idx >= vec_->capacity()) [[unlikely]] {
        ++num_failed_;
        if (idx == vec_->capacity()) {
          return attach_source_location(
              *r_, _var,
              Error("Too many elements, capacity is " +
                    std::to_string(vec_->capacity()) + "."));
        }
        return std::nullopt;
      }
    }
    auto res = Parser<R, W, ParsedType, ProcessorsType>::read(*r_, _var);
    if (!res) [[unlikely]] {
      // The message stays as it is, but the index of the element becomes
      // part of the path.
      const auto idx = index();
      ++num_failed_;
      return wrap_error("", idx, std::move(res).error());
    }
    if constexpr (is_map_like_v<VecType> || is_set_like_v<VecType>) {
      vec_->insert(std::move(*res));
      ++num_inserted_;
    } else if constexpr (is_inplace_vector_v<VecType>) {
      if (!vec_->try_emplace_back(std::move(*res))) {
        ++num_failed_;
        return attach_source_location(
            *r_, _var,
            Error("Too many elements, capacity is " +
                  std::to_string(vec_->capacity()) + "."));
      }
    } else {
      vec_->emplace_back(std::move(*res));
    }
    return std::nullopt;
  }

 private:
  /// Returns the index of the element that is currently being read.
  size_t index() const noexcept {
    if constexpr (is_map_like_v<VecType> || is_set_like_v<VecType>) {
      // Inserting a duplicate does not change the size of a map or set.
      return num_inserted_ + num_failed_;
    } else {
      return vec_->size() + num_failed_;
    }
  }

 private:
  /// The number of elements inserted into a map or set so far.
  mutable size_t num_inserted_;

  /// The number of elements that failed to parse so far (only touched on the
  /// error path to avoid adding overhead to successful std::vector reads).
  mutable size_t num_failed_;

  /// The underlying reader.
  const R* r_;

  /// The underlying vector.
  VecType* vec_;
};

}  // namespace rfl::parsing

#endif
