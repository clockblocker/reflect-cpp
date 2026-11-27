#ifndef RFL_DEFAULTIFNULL_HPP_
#define RFL_DEFAULTIFNULL_HPP_

namespace rfl {

/// A processor that instructs parsers to use default values when encountering
/// null or empty input values.
/// This is a marker type (doesn't modify data) that changes parser behavior.
/// When an input value is null/empty (for example, `null`, `~`, or an empty
/// scalar in YAML, or `null` in JSON) and the target type is
/// default-constructible, the default value is used instead of treating it as
/// an error. On struct fields, in-class member initializers are preserved.
/// Nullable types such as `std::optional`, `std::unique_ptr`,
/// `std::shared_ptr`, and raw pointers continue to decode `null` as empty
/// (`std::nullopt` / `nullptr`).
/// Usage: rfl::yaml::read<MyStruct, rfl::DefaultIfNull>(yaml_str)
struct DefaultIfNull {
 public:
  /// Identity process function - returns the named tuple unchanged.
  /// The actual default value application happens in the parser, not here.
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
