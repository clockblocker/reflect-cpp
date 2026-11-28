#ifndef RFL_INT64ASSTRING_HPP_
#define RFL_INT64ASSTRING_HPP_

namespace rfl {

/// This is a 'fake' processor - it doesn't do much by itself, but its
/// inclusion instructs parsers to serialize and deserialize 64-bit (and wider)
/// integer types as base-10 strings instead of format numbers.
/// Useful when sharing one pure C++ struct (plain int64_t / uint64_t) across
/// binary/columnar formats (Arrow, CBOR, MsgPack, Parquet) and JS/Protobuf-JSON
/// APIs that lose integer precision above 2^53 - 1.
struct Int64AsString {
 public:
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

}  // namespace rfl

#endif
