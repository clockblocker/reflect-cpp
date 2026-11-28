#ifndef RFL_STRINGIFIED_HPP_
#define RFL_STRINGIFIED_HPP_

#include <array>
#include <charconv>
#include <compare>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

#include "Result.hpp"

namespace rfl {

/// Wraps an integral value for base-10 string serialization and deserialization.
/// Useful for wire formats and APIs that encode 64-bit integers as JSON strings
/// to avoid IEEE-754 double precision loss (e.g. JavaScript/TypeScript clients,
/// Protobuf JSON mapping, Snowflake IDs).
/// Uses std::to_chars / std::from_chars for exact, locale-independent
/// conversion with full overflow and syntax checking.
/// @tparam T The integral type to wrap (excluding bool)
template <class T>
  requires(std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>)
struct Stringified {
  /// The underlying integral type.
  using Type = T;

  /// Wire representation type used by reflect-cpp.
  using ReflectionType = std::string;

  /// Default constructor - initializes to zero.
  constexpr Stringified() noexcept : value_(0) {}

  /// Constructs from the underlying integer value.
  constexpr Stringified(const Type& _value) noexcept : value_(_value) {}

  /// Move constructor.
  constexpr Stringified(Stringified&& _other) noexcept = default;

  /// Copy constructor.
  constexpr Stringified(const Stringified& _other) noexcept = default;

  /// Constructs from a compatible Stringified integer type.
  template <class U>
    requires(std::is_integral_v<U> &&
             !std::is_same_v<std::remove_cv_t<U>, bool> &&
             std::is_convertible_v<U, Type>)
  constexpr Stringified(const Stringified<U>& _other) noexcept
      : value_(static_cast<Type>(_other.get())) {}

  /// Constructs from any integral type convertible to Type.
  template <class U>
    requires(std::is_integral_v<U> &&
             !std::is_same_v<std::remove_cv_t<U>, bool> &&
             !std::is_same_v<std::remove_cv_t<U>, Type> &&
             std::is_convertible_v<U, Type>)
  constexpr Stringified(const U& _value) noexcept
      : value_(static_cast<Type>(_value)) {}

  /// Destructor.
  ~Stringified() = default;

  /// Parses a base-10 integer string into a Stringified<T>.
  /// Rejects empty strings, leading/trailing whitespace, non-digit characters,
  /// leading '+', negative numbers for unsigned types, and out-of-range values.
  static Result<Stringified<T>> from_string(std::string_view _str) {
    if (_str.empty()) {
      return error("Could not parse empty string as an integer.");
    }
    Type parsed = 0;
    const char* const begin = _str.data();
    const char* const end = _str.data() + _str.size();
    const auto [ptr, ec] = std::from_chars(begin, end, parsed);
    if (ec == std::errc::result_out_of_range) {
      return error("Integer string '" + std::string(_str) +
                   "' is out of range for the target type.");
    }
    if (ec != std::errc{} || ptr != end) {
      return error("Could not parse '" + std::string(_str) +
                   "' as an integer.");
    }
    return Stringified<T>(parsed);
  }

  /// Factory used by reflect-cpp during deserialization.
  static Result<Stringified<T>> from_reflection(std::string_view _str) {
    return from_string(_str);
  }

  /// Returns the underlying integer value (const).
  constexpr const Type& get() const noexcept { return value_; }

  /// Returns the underlying integer value.
  constexpr Type& get() noexcept { return value_; }

  /// Dereference operator - returns the underlying value.
  constexpr Type& operator*() noexcept { return value_; }

  /// Dereference operator (const) - returns the underlying value.
  constexpr const Type& operator*() const noexcept { return value_; }

  /// Function call operator - returns the underlying value.
  constexpr Type& operator()() noexcept { return value_; }

  /// Function call operator (const) - returns the underlying value.
  constexpr const Type& operator()() const noexcept { return value_; }

  /// Pointer to the underlying value.
  constexpr Type* operator->() noexcept { return &value_; }

  /// Pointer to the underlying value (const).
  constexpr const Type* operator->() const noexcept { return &value_; }

  /// Copy assignment operator.
  constexpr Stringified& operator=(const Stringified& _other) noexcept =
      default;

  /// Move assignment operator.
  constexpr Stringified& operator=(Stringified&& _other) noexcept = default;

  /// Assigns a new integer value.
  constexpr Stringified& operator=(const Type& _value) noexcept {
    value_ = _value;
    return *this;
  }

  /// Assigns from a compatible integral type.
  template <class U>
    requires(std::is_integral_v<U> &&
             !std::is_same_v<std::remove_cv_t<U>, bool> &&
             !std::is_same_v<std::remove_cv_t<U>, Type> &&
             std::is_convertible_v<U, Type>)
  constexpr Stringified& operator=(const U& _value) noexcept {
    value_ = static_cast<Type>(_value);
    return *this;
  }

  /// Assigns from a compatible Stringified integer type.
  template <class U>
    requires(std::is_integral_v<U> &&
             !std::is_same_v<std::remove_cv_t<U>, bool> &&
             std::is_convertible_v<U, Type>)
  constexpr Stringified& operator=(const Stringified<U>& _other) noexcept {
    value_ = static_cast<Type>(_other.get());
    return *this;
  }

  /// Spaceship comparison between Stringified<T> values.
  constexpr auto operator<=>(const Stringified& _other) const noexcept =
      default;

  /// Equality comparison with the underlying integer type.
  constexpr bool operator==(const Type& _other) const noexcept {
    return value_ == _other;
  }

  /// Returns the value formatted as a base-10 string for serialization.
  std::string reflection() const {
    std::array<char, 64> buf{};
    const auto [ptr, ec] =
        std::to_chars(buf.data(), buf.data() + buf.size(), value_);
    if (ec != std::errc{}) {
      return std::to_string(value_);
    }
    return std::string(buf.data(), static_cast<std::size_t>(ptr - buf.data()));
  }

  /// Sets the integer value.
  constexpr void set(const Type& _value) noexcept { value_ = _value; }

  /// Returns the value as a base-10 string, alias for .reflection().
  std::string str() const { return reflection(); }

  /// Returns the underlying integer value.
  constexpr Type& value() noexcept { return value_; }

  /// Returns the underlying integer value (const).
  constexpr const Type& value() const noexcept { return value_; }

  /// The underlying integer value.
  Type value_;
};

}  // namespace rfl

#endif
