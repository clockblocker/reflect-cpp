#ifndef RFL_INTERNAL_MAX_DEPTH_HPP_
#define RFL_INTERNAL_MAX_DEPTH_HPP_

#include <cstddef>
#include <string>
#include <type_traits>

#include "../MaxDepth.hpp"
#include "../Processors.hpp"
#include "../Result.hpp"

namespace rfl::internal {

template <class T>
struct max_depth_info {
  static constexpr std::size_t count = 0;
  static constexpr std::size_t limit = 0;
};

template <std::size_t _limit>
struct max_depth_info<MaxDepth<_limit>> {
  static constexpr std::size_t count = 1;
  static constexpr std::size_t limit = _limit;
};

template <>
struct max_depth_info<Processors<>> {
  static constexpr std::size_t count = 0;
  static constexpr std::size_t limit = 0;
};

template <class Head, class... Tail>
struct max_depth_info<Processors<Head, Tail...>> {
 private:
  using HeadInfo = max_depth_info<std::remove_cvref_t<Head>>;
  using TailInfo = max_depth_info<Processors<Tail...>>;

 public:
  static constexpr std::size_t count = HeadInfo::count + TailInfo::count;
  static_assert(count <= 1,
                "Multiple rfl::MaxDepth processors are not allowed.");
  static constexpr std::size_t limit =
      HeadInfo::count > 0 ? HeadInfo::limit : TailInfo::limit;
};

template <class ProcessorsType>
inline constexpr std::size_t max_depth_v =
    max_depth_info<std::remove_cvref_t<ProcessorsType>>::limit;

inline std::size_t& current_parse_depth() noexcept {
  thread_local std::size_t depth = 0;
  return depth;
}

template <std::size_t _limit>
class ParseDepthGuard {
 public:
  ParseDepthGuard() noexcept : exceeded_(++current_parse_depth() > _limit) {}

  ~ParseDepthGuard() noexcept { --current_parse_depth(); }

  ParseDepthGuard(const ParseDepthGuard&) = delete;
  ParseDepthGuard& operator=(const ParseDepthGuard&) = delete;

  bool exceeded() const noexcept { return exceeded_; }

  static Error make_error() {
    return Error("Maximum nesting depth of " + std::to_string(_limit) +
                 " exceeded.");
  }

 private:
  bool exceeded_;
};

}  // namespace rfl::internal

#endif
