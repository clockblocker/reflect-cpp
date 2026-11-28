#ifndef RFL_DEFAULTTAG_HPP_
#define RFL_DEFAULTTAG_HPP_

#include <utility>

#include "internal/StringLiteral.hpp"

namespace rfl {

/// Opt-in processor that supplies a default discriminator tag value when a
/// `rfl::TaggedUnion<_discriminator, ...>` is deserialized from an object
/// whose `_discriminator` field is absent.
///
/// Behavior:
/// - Only applies when the discriminator field is missing from the input object
///   (present non-string or unknown discriminator values still fail as usual).
/// - Writing always emits the explicit discriminator field.
/// - Validated at compile time: `_default_tag` must match one of the union's
///   allowed tags, and at most one `DefaultTag` may be specified per
///   `_discriminator`.
/// - Multiple `DefaultTag` processors with distinct discriminators may be
///   combined in a single `rfl::Processors<...>` bundle.
///
/// Example:
///   using Schema = rfl::TaggedUnion<"version", ConfigV1, ConfigV2>;
///   auto cfg = rfl::yaml::read<Schema, rfl::DefaultTag<"version", "1">>(yaml);
template <internal::StringLiteral _discriminator,
          internal::StringLiteral _default_tag>
struct DefaultTag {
  static constexpr internal::StringLiteral discriminator_ = _discriminator;
  static constexpr internal::StringLiteral default_tag_ = _default_tag;

  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return std::forward<decltype(_named_tuple)>(_named_tuple);
  }
};

}  // namespace rfl

#endif
