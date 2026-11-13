#ifndef RFL_VARIANTSASONEOF_HPP_
#define RFL_VARIANTSASONEOF_HPP_

namespace rfl {

/// A processor that makes the JSON schema describe rfl::Variant and
/// std::variant as oneOf instead of anyOf.
/// This is a marker type (doesn't modify data) that changes the generated
/// schema. It has no effect on reading or writing.
/// By default, untagged variants are described as anyOf, because their
/// alternatives can overlap: 1 matches both alternatives of
/// rfl::Variant<int, double>, and oneOf would reject it. rfl::TaggedUnion is
/// always described as oneOf, and std::optional and rfl::Generic always as
/// anyOf.
/// Usage: rfl::json::to_schema<MyStruct, VariantsAsOneOf>()
struct VariantsAsOneOf {
 public:
  /// Identity process function - returns the named tuple unchanged.
  /// The actual change happens in the schema generation, not here.
  /// @tparam StructType The struct type being processed
  /// @param _named_tuple The named tuple representation of the struct
  /// @return The same named tuple (unchanged)
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

}  // namespace rfl

#endif
