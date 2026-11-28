#ifndef RFL_INTERNAL_DEFAULT_TAG_HPP_
#define RFL_INTERNAL_DEFAULT_TAG_HPP_

#include <cstddef>
#include <type_traits>

#include "../DefaultTag.hpp"
#include "../Processors.hpp"
#include "StringLiteral.hpp"

namespace rfl::internal {

template <class T, StringLiteral _discriminator>
struct default_tag_info {
  static constexpr std::size_t count = 0;
};

template <StringLiteral _discriminator, StringLiteral _default_tag>
struct default_tag_info<DefaultTag<_discriminator, _default_tag>,
                        _discriminator> {
  static constexpr std::size_t count = 1;

  static consteval auto get_value() noexcept { return _default_tag; }
};

template <StringLiteral _discriminator>
struct default_tag_info<Processors<>, _discriminator> {
  static constexpr std::size_t count = 0;
};

template <class Head, class... Tail, StringLiteral _discriminator>
struct default_tag_info<Processors<Head, Tail...>, _discriminator> {
 private:
  using HeadInfo = default_tag_info<std::remove_cvref_t<Head>, _discriminator>;
  using TailInfo = default_tag_info<Processors<Tail...>, _discriminator>;

 public:
  static constexpr std::size_t count = HeadInfo::count + TailInfo::count;

  static consteval auto get_value() noexcept {
    if constexpr (HeadInfo::count > 0) {
      return HeadInfo::get_value();
    } else {
      return TailInfo::get_value();
    }
  }
};

template <class ProcessorsType, StringLiteral _discriminator>
inline constexpr std::size_t default_tag_count_v =
    default_tag_info<std::remove_cvref_t<ProcessorsType>,
                     _discriminator>::count;

template <class ProcessorsType, StringLiteral _discriminator>
inline constexpr bool has_default_tag_v =
    default_tag_count_v<ProcessorsType, _discriminator> > 0;

template <class ProcessorsType, StringLiteral _discriminator>
inline constexpr auto default_tag_literal_v =
    default_tag_info<std::remove_cvref_t<ProcessorsType>,
                     _discriminator>::get_value();

}  // namespace rfl::internal

#endif
