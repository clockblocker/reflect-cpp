#ifndef RFL_INTERNAL_ERROR_HPP_
#define RFL_INTERNAL_ERROR_HPP_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "cold.hpp"

namespace rfl::internal {

/// Coarse, format-agnostic classification of an ErrorDetail failure.
/// Classifies the input (or I/O environment), never caller/programmer bugs.
/// Open enum: new enumerators may be added in minor releases; switch
/// statements over ErrorKind should include a `default:` branch.
enum class ErrorKind : std::uint8_t {
  /// Unclassified: custom parsers/reflectors, caller/programmer bugs,
  /// and formats not yet tagged.
  other = 0,

  /// Input could not be obtained or output could not be emitted
  /// (file open/read/write/close, stream badbit/failbit, directory/size I/O).
  io_error,

  /// Bytes or tokens cannot form the format's data model regardless of target
  /// type (malformed/truncated JSON/YAML/TOML, invalid UTF-8/UTF-16, depth
  /// limit, recursive YAML alias, malformed CLI token such as `--=val`).
  syntax_error,

  /// A declared, individually-required slot has no entry
  /// (required struct field, tuple element, required positional, or
  /// tagged-union discriminator key).
  missing_field,

  /// A key, option, or argv token resolves to no declared slot—or ambiguously
  /// to more than one (under NoExtraFields, extra CLI positionals, or
  /// ambiguous CLI option abbreviations).
  unknown_field,

  /// A key or non-repeatable option appeared more than once where repeats
  /// are forbidden (NoDuplicateKeys, duplicate scalar CLI option).
  duplicate_field,

  /// Element or value count violates a count fixed by the C++ type or option
  /// arity (std::array<T, N> size mismatch, tuple overflow, inplace_vector
  /// capacity, CLI option arity mismatch).
  invalid_length,

  /// The node's native wire class is incompatible with the target type
  /// (null/bool/integer/float/string/bytes/array/object mismatch).
  invalid_type,

  /// Right wire class (or string-only format token), but domain/lexical
  /// conversion to the C++ type failed (numeric range/sign overflow including
  /// negative int for unsigned, unknown enum/tag/FieldVariant key, bad
  /// timestamp/base64/bitset, no matching variant).
  invalid_value,

  /// Value successfully converted to the target C++ type, then a declared
  /// check rejected it (rfl::Validator<...>, AllOf/AnyOf/OneOf, Size, Pattern,
  /// Minimum, CLI constraints).
  constraint_violation,
};

/// Describes a single failure contained in an Error: where it occurred and
/// what went wrong.
struct ErrorDetail {
  /// A field name or an array index.
  using PathSegment = std::variant<std::string, size_t>;

  /// The location of the failure, starting from the root of the value being
  /// read. Field names are spelled as they appear in the input, that is, after
  /// processors such as rfl::SnakeCaseToCamelCase have been applied. An empty
  /// path refers to the root value itself.
  std::vector<PathSegment> path;

  /// The message describing the failure, without the "Failed to parse field
  /// '...': " prefixes that Error::what() contains for every level of nesting.
  std::string message;

  /// The coarse category of this failure.
  ErrorKind kind = ErrorKind::other;

  bool operator==(const ErrorDetail&) const = default;
};

/// Defines the error class to be returned when something went wrong.
/// This is used throughout reflect-cpp for error handling.
class Error {
 public:
  /// Default constructor - creates an empty error.
  Error() = default;

  /// Constructs an error with a message (copy).
  /// @param _what The error message describing what went wrong
  Error(const std::string& _what) : what_(_what) {}

  /// Constructs an error with a message (move).
  /// @param _what The error message describing what went wrong (will be moved)
  Error(std::string&& _what) : what_(std::move(_what)) {}

  /// Constructs a classified leaf error with a message (copy), without
  /// allocating an explicit detail vector.
  /// @param _what The error message describing what went wrong
  /// @param _kind Coarse classification of the failure
  Error(const std::string& _what, const ErrorKind _kind)
      : what_(_what), kind_(_kind) {}

  /// Constructs a classified leaf error with a message (move), without
  /// allocating an explicit detail vector.
  /// @param _what The error message describing what went wrong (will be moved)
  /// @param _kind Coarse classification of the failure
  Error(std::string&& _what, const ErrorKind _kind)
      : what_(std::move(_what)), kind_(_kind) {}

  /// Constructs an error with a message and the individual failures it
  /// summarizes.
  /// @param _what The error message describing what went wrong
  /// @param _details The individual failures, see details()
  Error(std::string _what, std::vector<ErrorDetail> _details)
      : what_(std::move(_what)),
        details_(_details.empty() ? OptionalDetails()
                                  : OptionalDetails(std::move(_details))),
        kind_(details_.has_value() ? (*details_).front().kind
                                   : ErrorKind::other) {}

  /// Destructor.
  ~Error() = default;

  /// Copy constructor.
  /// @param e The error to copy from
  Error(const Error& e) = default;

  /// Move constructor.
  /// @param e The error to move from
  Error(Error&& e) noexcept = default;

  /// Copy assignment operator.
  /// @param _other The error to copy from
  /// @return Reference to this error
  Error& operator=(const Error& _other) = default;

  /// Move assignment operator.
  /// @param _other The error to move from
  /// @return Reference to this error
  Error& operator=(Error&& _other) noexcept = default;

  /// Returns the error message, equivalent to .what() in std::exception.
  const std::string& what() const& { return what_; }
  /// Moves the error message out of Error object and leaves what_ in a moved
  /// from state
  std::string what() && { return std::move(what_); }

  /// Returns the ErrorKind of this error. For multi-error results, returns the
  /// kind of the first detail.
  ErrorKind kind() const noexcept {
    return details_.has_value() && !(*details_).empty()
               ? (*details_).front().kind
               : kind_;
  }

  /// Returns the individual failures summarized by what(), each with the
  /// location at which it occurred. An error that was constructed from a
  /// message only is reported as a single failure at the root.
  std::vector<ErrorDetail> details() const& {
    if (!details_.has_value()) {
      return {ErrorDetail{.path = {}, .message = what_, .kind = kind_}};
    }
    return *details_;
  }

  /// Moves the individual failures out of the Error object and leaves it in a
  /// moved from state.
  std::vector<ErrorDetail> details() && {
    if (!details_.has_value()) {
      return {
          ErrorDetail{.path = {}, .message = std::move(what_), .kind = kind_}};
    }
    return std::move(*details_);
  }

 private:
  friend bool has_allocated_details(const Error& _err) noexcept {
    return _err.details_.has_value();
  }

  friend void set_error_kind_if_other(Error& _err,
                                      const ErrorKind _kind) noexcept {
    if (_kind == ErrorKind::other) {
      return;
    }
    if (!_err.details_.has_value()) {
      if (_err.kind_ == ErrorKind::other) {
        _err.kind_ = _kind;
      }
      return;
    }
    for (auto& detail : *_err.details_) {
      if (detail.kind == ErrorKind::other) {
        detail.kind = _kind;
      }
    }
  }

  /// Either nothing or a heap-allocated vector of details, deep-copied along
  /// with the Error. Holding nothing is the common case: it means that the
  /// Error is just its message, and it does not allocate.
  ///
  /// Errors are created and destroyed in many places, mostly as part of
  /// rfl::Result. Deleting and copying the details out of line keeps that code
  /// small, which matters for the performance of successful reads.
  class OptionalDetails {
   public:
    /// Holds nothing.
    OptionalDetails() = default;

    /// Holds a heap-allocated vector containing _details.
    explicit OptionalDetails(std::vector<ErrorDetail>&& _details)
        : ptr_(new std::vector<ErrorDetail>(std::move(_details))) {}

    /// Holds a copy of the details _other holds, if any.
    OptionalDetails(const OptionalDetails& _other) : ptr_(copy(_other.ptr_)) {}

    OptionalDetails(OptionalDetails&&) noexcept = default;

    /// Holds a copy of the details _other holds, if any.
    OptionalDetails& operator=(const OptionalDetails& _other) {
      ptr_ = copy(_other.ptr_);
      return *this;
    }

    OptionalDetails& operator=(OptionalDetails&&) noexcept = default;

    bool has_value() const { return ptr_ != nullptr; }

    /// Must only be called if has_value() is true.
    const std::vector<ErrorDetail>& operator*() const { return *ptr_; }

    /// Must only be called if has_value() is true.
    std::vector<ErrorDetail>& operator*() { return *ptr_; }

   private:
    struct Delete {
      RFL_COLD void operator()(std::vector<ErrorDetail>* _ptr) const {
        delete _ptr;
      }
    };

    using Ptr = std::unique_ptr<std::vector<ErrorDetail>, Delete>;

    RFL_COLD static Ptr copy(const Ptr& _ptr) {
      return _ptr ? Ptr(new std::vector<ErrorDetail>(*_ptr)) : nullptr;
    }

    Ptr ptr_;
  };

 private:
  /// Documents what went wrong
  std::string what_;

  /// The individual failures summarized by what_. Holds nothing, unless what_
  /// was assembled from failures at nested locations.
  OptionalDetails details_;

  /// Coarse category for a leaf error when details_ holds nothing.
  ErrorKind kind_ = ErrorKind::other;
};

/// Returns whether _err has a heap-allocated detail vector.
bool has_allocated_details(const Error& _err) noexcept;

/// Replaces ErrorKind::other with _kind in place without copying what_ or
/// allocating details on leaf errors.
void set_error_kind_if_other(Error& _err, ErrorKind _kind) noexcept;

}  // namespace rfl::internal

#endif
