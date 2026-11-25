#ifndef RFL_NODUPLICATEKEYS_HPP_
#define RFL_NODUPLICATEKEYS_HPP_

#include <utility>

namespace rfl {

/// Reject duplicate keys in input mappings/objects for readers that support
/// duplicate-key detection (such as YAML).
///
/// By default, duplicate mapping keys keep existing reader behavior for
/// backwards compatibility. Pass rfl::NoDuplicateKeys to report each repeated
/// key as an error:
///   rfl::yaml::read<MyStruct, rfl::NoExtraFields, rfl::NoDuplicateKeys>(yaml)
struct NoDuplicateKeys {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return std::forward<decltype(_named_tuple)>(_named_tuple);
  }
};

}  // namespace rfl

#endif
