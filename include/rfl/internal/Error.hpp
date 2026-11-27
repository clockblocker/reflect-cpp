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
  /// are forbidden (e.g. duplicate mapping key or scalar option).
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

/// Internal compact tags for built-in validator rule codes so leaf validator
/// errors carry a machine-readable code inline in Error without increasing
/// sizeof(Error) or allocating OptionalDetails.
enum class BuiltinErrorCode : std::uint8_t {
  none = 0,
  equal_to,
  not_equal_to,
  minimum,
  exclusive_minimum,
  maximum,
  exclusive_maximum,
  pattern,
  any_of,
  one_of,
  size,
  size_equal_to,
  size_not_equal_to,
  size_minimum,
  size_exclusive_minimum,
  size_maximum,
  size_exclusive_maximum,
  size_any_of,
  size_one_of,
  code_points,
  code_points_equal_to,
  code_points_not_equal_to,
  code_points_minimum,
  code_points_exclusive_minimum,
  code_points_maximum,
  code_points_exclusive_maximum,
  code_points_any_of,
  code_points_one_of,
};

/// Returns the canonical wire string for a BuiltinErrorCode, or empty if none.
constexpr std::string_view builtin_error_code_str(
    const BuiltinErrorCode _code) noexcept {
  switch (_code) {
    case BuiltinErrorCode::none:
      return {};
    case BuiltinErrorCode::equal_to:
      return "equal_to";
    case BuiltinErrorCode::not_equal_to:
      return "not_equal_to";
    case BuiltinErrorCode::minimum:
      return "minimum";
    case BuiltinErrorCode::exclusive_minimum:
      return "exclusive_minimum";
    case BuiltinErrorCode::maximum:
      return "maximum";
    case BuiltinErrorCode::exclusive_maximum:
      return "exclusive_maximum";
    case BuiltinErrorCode::pattern:
      return "pattern";
    case BuiltinErrorCode::any_of:
      return "any_of";
    case BuiltinErrorCode::one_of:
      return "one_of";
    case BuiltinErrorCode::size:
      return "size";
    case BuiltinErrorCode::size_equal_to:
      return "size.equal_to";
    case BuiltinErrorCode::size_not_equal_to:
      return "size.not_equal_to";
    case BuiltinErrorCode::size_minimum:
      return "size.minimum";
    case BuiltinErrorCode::size_exclusive_minimum:
      return "size.exclusive_minimum";
    case BuiltinErrorCode::size_maximum:
      return "size.maximum";
    case BuiltinErrorCode::size_exclusive_maximum:
      return "size.exclusive_maximum";
    case BuiltinErrorCode::size_any_of:
      return "size.any_of";
    case BuiltinErrorCode::size_one_of:
      return "size.one_of";
    case BuiltinErrorCode::code_points:
      return "code_points";
    case BuiltinErrorCode::code_points_equal_to:
      return "code_points.equal_to";
    case BuiltinErrorCode::code_points_not_equal_to:
      return "code_points.not_equal_to";
    case BuiltinErrorCode::code_points_minimum:
      return "code_points.minimum";
    case BuiltinErrorCode::code_points_exclusive_minimum:
      return "code_points.exclusive_minimum";
    case BuiltinErrorCode::code_points_maximum:
      return "code_points.maximum";
    case BuiltinErrorCode::code_points_exclusive_maximum:
      return "code_points.exclusive_maximum";
    case BuiltinErrorCode::code_points_any_of:
      return "code_points.any_of";
    case BuiltinErrorCode::code_points_one_of:
      return "code_points.one_of";
  }
  return {};
}

/// Maps a canonical wire string back to its compact BuiltinErrorCode if it is
/// one of the built-in validator codes, or BuiltinErrorCode::none otherwise.
constexpr BuiltinErrorCode parse_builtin_error_code(
    const std::string_view _code) noexcept {
  if (_code.empty()) {
    return BuiltinErrorCode::none;
  }
  if (_code == "equal_to") return BuiltinErrorCode::equal_to;
  if (_code == "not_equal_to") return BuiltinErrorCode::not_equal_to;
  if (_code == "minimum") return BuiltinErrorCode::minimum;
  if (_code == "exclusive_minimum") return BuiltinErrorCode::exclusive_minimum;
  if (_code == "maximum") return BuiltinErrorCode::maximum;
  if (_code == "exclusive_maximum") return BuiltinErrorCode::exclusive_maximum;
  if (_code == "pattern") return BuiltinErrorCode::pattern;
  if (_code == "any_of") return BuiltinErrorCode::any_of;
  if (_code == "one_of") return BuiltinErrorCode::one_of;
  if (_code == "size") return BuiltinErrorCode::size;
  if (_code == "size.equal_to") return BuiltinErrorCode::size_equal_to;
  if (_code == "size.not_equal_to") return BuiltinErrorCode::size_not_equal_to;
  if (_code == "size.minimum") return BuiltinErrorCode::size_minimum;
  if (_code == "size.exclusive_minimum")
    return BuiltinErrorCode::size_exclusive_minimum;
  if (_code == "size.maximum") return BuiltinErrorCode::size_maximum;
  if (_code == "size.exclusive_maximum")
    return BuiltinErrorCode::size_exclusive_maximum;
  if (_code == "size.any_of") return BuiltinErrorCode::size_any_of;
  if (_code == "size.one_of") return BuiltinErrorCode::size_one_of;
  if (_code == "code_points") return BuiltinErrorCode::code_points;
  if (_code == "code_points.equal_to")
    return BuiltinErrorCode::code_points_equal_to;
  if (_code == "code_points.not_equal_to")
    return BuiltinErrorCode::code_points_not_equal_to;
  if (_code == "code_points.minimum")
    return BuiltinErrorCode::code_points_minimum;
  if (_code == "code_points.exclusive_minimum")
    return BuiltinErrorCode::code_points_exclusive_minimum;
  if (_code == "code_points.maximum")
    return BuiltinErrorCode::code_points_maximum;
  if (_code == "code_points.exclusive_maximum")
    return BuiltinErrorCode::code_points_exclusive_maximum;
  if (_code == "code_points.any_of")
    return BuiltinErrorCode::code_points_any_of;
  if (_code == "code_points.one_of")
    return BuiltinErrorCode::code_points_one_of;
  return BuiltinErrorCode::none;
}

/// Composes a BuiltinErrorCode under the `size.*` namespace when possible.
constexpr BuiltinErrorCode compose_size_code(
    const BuiltinErrorCode _inner) noexcept {
  switch (_inner) {
    case BuiltinErrorCode::none:
      return BuiltinErrorCode::size;
    case BuiltinErrorCode::equal_to:
      return BuiltinErrorCode::size_equal_to;
    case BuiltinErrorCode::not_equal_to:
      return BuiltinErrorCode::size_not_equal_to;
    case BuiltinErrorCode::minimum:
      return BuiltinErrorCode::size_minimum;
    case BuiltinErrorCode::exclusive_minimum:
      return BuiltinErrorCode::size_exclusive_minimum;
    case BuiltinErrorCode::maximum:
      return BuiltinErrorCode::size_maximum;
    case BuiltinErrorCode::exclusive_maximum:
      return BuiltinErrorCode::size_exclusive_maximum;
    case BuiltinErrorCode::any_of:
      return BuiltinErrorCode::size_any_of;
    case BuiltinErrorCode::one_of:
      return BuiltinErrorCode::size_one_of;
    default:
      return BuiltinErrorCode::none;
  }
}

/// Composes a BuiltinErrorCode under the `code_points.*` namespace when
/// possible.
constexpr BuiltinErrorCode compose_code_points_code(
    const BuiltinErrorCode _inner) noexcept {
  switch (_inner) {
    case BuiltinErrorCode::none:
      return BuiltinErrorCode::code_points;
    case BuiltinErrorCode::equal_to:
      return BuiltinErrorCode::code_points_equal_to;
    case BuiltinErrorCode::not_equal_to:
      return BuiltinErrorCode::code_points_not_equal_to;
    case BuiltinErrorCode::minimum:
      return BuiltinErrorCode::code_points_minimum;
    case BuiltinErrorCode::exclusive_minimum:
      return BuiltinErrorCode::code_points_exclusive_minimum;
    case BuiltinErrorCode::maximum:
      return BuiltinErrorCode::code_points_maximum;
    case BuiltinErrorCode::exclusive_maximum:
      return BuiltinErrorCode::code_points_exclusive_maximum;
    case BuiltinErrorCode::any_of:
      return BuiltinErrorCode::code_points_any_of;
    case BuiltinErrorCode::one_of:
      return BuiltinErrorCode::code_points_one_of;
    default:
      return BuiltinErrorCode::none;
  }
}

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

  /// Optional machine-readable rule or error code (for example, `"minimum"`,
  /// `"size.minimum"`, `"pattern"`, or a custom validator code). Empty when
  /// no finer-grained code applies.
  std::string code = {};

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

  /// Constructs a classified leaf error with a built-in rule code, without
  /// allocating an explicit detail vector.
  Error(std::string _what, const ErrorKind _kind,
        const BuiltinErrorCode _builtin_code)
      : what_(std::move(_what)), kind_(_kind), builtin_code_(_builtin_code) {}

  /// Constructs a classified error with a machine-readable code.
  /// Built-in codes are stored inline without allocating details; custom codes
  /// are stored in details_.
  Error(std::string _what, const ErrorKind _kind, const std::string_view _code)
      : what_(std::move(_what)), kind_(_kind) {
    if (_code.empty()) {
      return;
    }
    const auto builtin = parse_builtin_error_code(_code);
    if (builtin != BuiltinErrorCode::none) {
      builtin_code_ = builtin;
      return;
    }
    std::vector<ErrorDetail> details;
    details.push_back(ErrorDetail{
        .path = {},
        .message = what_,
        .kind = _kind,
        .code = std::string(_code),
    });
    details_ = OptionalDetails(std::move(details));
  }

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

  /// Returns the machine-readable error code of this error, or an empty view
  /// when none is set. For multi-error results, returns the code of the first
  /// detail.
  std::string_view code() const noexcept {
    if (details_.has_value() && !(*details_).empty()) {
      return (*details_).front().code;
    }
    return builtin_error_code_str(builtin_code_);
  }

  /// Returns the individual failures summarized by what(), each with the
  /// location at which it occurred. An error that was constructed from a
  /// message only is reported as a single failure at the root.
  std::vector<ErrorDetail> details() const& {
    if (!details_.has_value()) {
      return {ErrorDetail{
          .path = {},
          .message = what_,
          .kind = kind_,
          .code = std::string(builtin_error_code_str(builtin_code_))}};
    }
    return *details_;
  }

  /// Moves the individual failures out of the Error object and leaves it in a
  /// moved from state.
  std::vector<ErrorDetail> details() && {
    if (!details_.has_value()) {
      return {ErrorDetail{
          .path = {},
          .message = std::move(what_),
          .kind = kind_,
          .code = std::string(builtin_error_code_str(builtin_code_))}};
    }
    return std::move(*details_);
  }

 private:
  friend bool has_allocated_details(const Error& _err) noexcept {
    return _err.details_.has_value();
  }

  friend BuiltinErrorCode builtin_error_code_of(const Error& _err) noexcept {
    return _err.details_.has_value() ? BuiltinErrorCode::none
                                     : _err.builtin_code_;
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

  friend void set_error_code_if_empty(Error& _err,
                                      const std::string_view _code) {
    if (_code.empty()) {
      return;
    }
    if (!_err.details_.has_value()) {
      if (_err.builtin_code_ != BuiltinErrorCode::none) {
        return;
      }
      const auto builtin = parse_builtin_error_code(_code);
      if (builtin != BuiltinErrorCode::none) {
        _err.builtin_code_ = builtin;
        return;
      }
      std::vector<ErrorDetail> details;
      details.push_back(ErrorDetail{
          .path = {},
          .message = _err.what_,
          .kind = _err.kind_,
          .code = std::string(_code),
      });
      _err.details_ = OptionalDetails(std::move(details));
      return;
    }
    for (auto& detail : *_err.details_) {
      if (detail.code.empty()) {
        detail.code = std::string(_code);
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

  /// Compact built-in rule code for a leaf error when details_ holds nothing.
  /// Packed into ErrorKind's tail padding so sizeof(Error) does not grow.
  BuiltinErrorCode builtin_code_ = BuiltinErrorCode::none;
};

/// Returns whether _err has a heap-allocated detail vector.
bool has_allocated_details(const Error& _err) noexcept;

/// Returns the inline BuiltinErrorCode for a leaf error (none if detailed).
BuiltinErrorCode builtin_error_code_of(const Error& _err) noexcept;

/// Replaces ErrorKind::other with _kind in place without copying what_ or
/// allocating details on leaf errors.
void set_error_kind_if_other(Error& _err, ErrorKind _kind) noexcept;

/// Attaches _code to _err (or to each uncoded detail) when no code is already
/// present. Preserves zero-allocation for built-in codes on leaf errors.
void set_error_code_if_empty(Error& _err, std::string_view _code);

}  // namespace rfl::internal

#endif
