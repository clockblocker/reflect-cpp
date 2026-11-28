#ifndef RFL_NODUPLICATEKEYS_HPP_
#define RFL_NODUPLICATEKEYS_HPP_

#include <utility>

namespace rfl {

/// Reject duplicate keys in input objects/mappings for readers that support
/// duplicate-key detection (JSON and YAML).
///
/// By default, duplicate keys keep existing reader behavior for backwards
/// compatibility. Pass rfl::NoDuplicateKeys to report each repeated key as an
/// error:
///   rfl::json::read<MyStruct, rfl::NoExtraFields, rfl::NoDuplicateKeys>(json)
///   rfl::yaml::read<MyStruct, rfl::NoExtraFields, rfl::NoDuplicateKeys>(yaml)
struct NoDuplicateKeys {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return std::forward<decltype(_named_tuple)>(_named_tuple);
  }
};

}  // namespace rfl

#endif
