#ifndef RFL_INTERNAL_DEFERREDVALIDATION_HPP_
#define RFL_INTERNAL_DEFERREDVALIDATION_HPP_

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../Result.hpp"

namespace rfl::internal {

/// Thrown by the default constructor of rfl::Validator when the default value
/// is invalid. It derives from the type that Result::value() throws, so code
/// outside of the parser sees no difference. The parser catches exactly this
/// type to tell an invalid default value apart from other exceptions.
#ifdef REFLECTCPP_USE_STD_EXPECTED
class InvalidDefaultValue : public std::bad_expected_access<Error> {
 public:
  explicit InvalidDefaultValue(Error _err)
      : std::bad_expected_access<Error>(std::move(_err)) {}
};
#else
class InvalidDefaultValue : public std::runtime_error {
 public:
  explicit InvalidDefaultValue(const Error& _err)
      : std::runtime_error(_err.what()) {}
};
#endif

/// Lets the parser default-construct a struct even though a default-constructed
/// rfl::Validator inside it rejects its own default value.
///
/// Parser::read_struct_with_default default-constructs the struct it reads
/// into, only to obtain the default values of the fields that the input does
/// not contain. If that throws an InvalidDefaultValue, it constructs the
/// struct again while a DeferredValidation covering the struct's storage is
/// active on this thread. A Validator that is default-constructed inside that
/// storage and whose default value is invalid then keeps it instead of
/// throwing, and is recorded here with its error. After reading, the parser
/// checks the fields that the input did not contain.
///
/// Outside of that one construction nothing is active, and Validators behave
/// exactly as they always have.
class DeferredValidation {
 public:
  struct Record {
    /// The address of the Validator.
    const void* address;

    /// Why its default value is invalid.
    Error error;
  };

  DeferredValidation(const void* _begin, std::size_t _size) noexcept
      : begin_(static_cast<const unsigned char*>(_begin)),
        end_(begin_ + _size) {}

  DeferredValidation(const DeferredValidation&) = delete;
  DeferredValidation& operator=(const DeferredValidation&) = delete;

  /// Makes a DeferredValidation the innermost active one on this thread for
  /// as long as the Activation lives.
  class Activation {
   public:
    explicit Activation(DeferredValidation* _d) noexcept : d_(_d) {
      auto& h = head();
      d_->prev_ = h;
      h = d_;
    }

    ~Activation() {
      head() = d_->prev_;
      d_->prev_ = nullptr;
    }

    Activation(const Activation&) = delete;
    Activation& operator=(const Activation&) = delete;

   private:
    DeferredValidation* d_;
  };

  /// The innermost active DeferredValidation on this thread, or nullptr.
  static DeferredValidation*& head() noexcept {
    static thread_local DeferredValidation* h = nullptr;
    return h;
  }

  /// Returns the innermost active DeferredValidation on this thread whose
  /// storage contains _address, or nullptr.
  static DeferredValidation* find(const void* _address) noexcept {
    for (auto* d = head(); d; d = d->prev_) {
      if (d->contains(_address)) {
        return d;
      }
    }
    return nullptr;
  }

  /// Records a Validator whose default value is invalid.
  void defer(const void* _address, Error&& _error) {
    records_.push_back(Record{_address, std::move(_error)});
  }

  const std::vector<Record>& records() const noexcept { return records_; }

  bool contains(const void* _address) const noexcept {
    const auto p = static_cast<const unsigned char*>(_address);
    return !std::less<>{}(p, begin_) && std::less<>{}(p, end_);
  }

 private:
  /// The storage of the struct being default-constructed.
  const unsigned char* begin_;
  const unsigned char* end_;

  /// The next outer active DeferredValidation, if any.
  DeferredValidation* prev_ = nullptr;

  std::vector<Record> records_;
};

}  // namespace rfl::internal

#endif
