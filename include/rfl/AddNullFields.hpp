#ifndef RFL_ADDNULLFIELDS_HPP_
#define RFL_ADDNULLFIELDS_HPP_

namespace rfl {

/// A write-side processor that instructs serializers to emit `"field": null`
/// for empty optional/nullable fields (`std::optional`, `std::shared_ptr`,
/// `std::unique_ptr`, raw pointers) instead of omitting them, while keeping
/// them optional on read (unlike `rfl::NoOptionals`, which also makes them
/// mandatory on read).
///
/// `rfl::TriState<T>` fields in `State::omitted` remain omitted so tri-state
/// round-tripping is preserved.
struct AddNullFields {
 public:
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

}  // namespace rfl

#endif
