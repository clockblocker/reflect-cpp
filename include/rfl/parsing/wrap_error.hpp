#ifndef RFL_PARSING_WRAPERROR_HPP_
#define RFL_PARSING_WRAPERROR_HPP_

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../Result.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/cold.hpp"

namespace rfl::parsing {

/**
 * @brief Wraps an error that occurred while reading a nested value, such as a
 * field or an array element.
 *
 * @param _prefix The text to put in front of the error message.
 * @param _segment The location of the nested value relative to its parent,
 * which is prepended to the path of every detail.
 * @param _err The error that occurred while reading the nested value.
 * @return The error as seen from the parent.
 */
RFL_COLD inline Error wrap_error(const std::string_view _prefix,
                                 const ErrorDetail::PathSegment& _segment,
                                 Error&& _err) {
  auto what = std::string(_prefix) + _err.what();
  auto details = std::move(_err).details();
  for (auto& detail : details) {
    detail.path.insert(detail.path.begin(), _segment);
  }
  return Error(std::move(what), std::move(details));
}

/**
 * @brief Wraps an error that occurred while reading a field.
 *
 * @param _name The name of the field.
 * @param _err The error that occurred while reading the field.
 * @return The error as seen from the object containing the field.
 */
RFL_COLD inline Error wrap_field_error(const std::string_view _name,
                                       Error&& _err) {
  auto name = std::string(_name);
  return wrap_error("Failed to parse field '" + name + "': ", name,
                    std::move(_err));
}

/**
 * @brief Adds context to an error without changing the location at which it
 * occurred.
 *
 * @param _prefix The text to put in front of the error message. It is also put
 * in front of the messages of all details at the root, because these describe
 * the same failure as the error message.
 * @param _err The error to add context to.
 * @return The error with the added context.
 */
RFL_COLD inline Error wrap_error(const std::string_view _prefix, Error&& _err) {
  auto what = std::string(_prefix) + _err.what();
  if (!internal::has_allocated_details(_err)) {
    return Error(std::move(what), _err.kind());
  }
  auto details = std::move(_err).details();
  for (auto& detail : details) {
    if (detail.path.empty()) {
      detail.message = std::string(_prefix) + detail.message;
    }
  }
  return Error(std::move(what), std::move(details));
}

/**
 * @brief Creates an error for a nested value that could not be read at all,
 * for instance because it is missing.
 *
 * @param _segment The location of the nested value relative to its parent.
 * @param _msg The error message.
 * @param _kind Coarse classification of the error.
 * @param _location Optional source location for the error detail.
 * @return The error as seen from the parent.
 */
RFL_COLD inline Error make_nested_error(
    const ErrorDetail::PathSegment& _segment, std::string _msg,
    const ErrorKind _kind = ErrorKind::other,
    std::optional<SourceLocation> _location = std::nullopt) {
  auto details = std::vector<ErrorDetail>(
      {ErrorDetail{.path = {_segment},
                   .message = _msg,
                   .location = _location,
                   .kind = _kind}});
  return Error(std::move(_msg), std::move(details));
}

RFL_COLD inline Error make_nested_error(
    const ErrorDetail::PathSegment& _segment, std::string _msg,
    std::optional<SourceLocation> _location) {
  return make_nested_error(_segment, std::move(_msg), ErrorKind::other,
                           _location);
}

/**
 * @brief Creates a root error carrying an optional source location.
 *
 * @param _msg The error message.
 * @param _location Optional source location for the error detail.
 * @param _kind Coarse classification of the error.
 * @return The error.
 */
RFL_COLD inline Error make_error_with_location(
    std::string _msg, std::optional<SourceLocation> _location,
    const ErrorKind _kind = ErrorKind::other) {
  if (!_location) {
    return Error(std::move(_msg), _kind);
  }
  auto details = std::vector<ErrorDetail>({ErrorDetail{
      .path = {}, .message = _msg, .location = _location, .kind = _kind}});
  return Error(std::move(_msg), std::move(details));
}

/**
 * @brief Queries a reader for the source location of an input node, if the
 * reader supports source locations.
 */
template <class R, class InputType>
std::optional<SourceLocation> get_source_location(
    const R& _r, const InputType& _input, const bool _exact = true) noexcept {
  if constexpr (requires { _r.source_location(_input); }) {
    auto loc = _r.source_location(_input);
    if (loc) {
      loc->exact = _exact;
    }
    return loc;
  } else {
    return std::nullopt;
  }
}

/**
 * @brief Fills in a source location on any error details that do not already
 * have one, preserving existing leaf locations.
 */
RFL_COLD inline Error attach_source_location(
    const std::optional<SourceLocation>& _location, Error&& _err) {
  if (!_location || _err.has_all_source_locations()) {
    return std::move(_err);
  }
  auto what = _err.what();
  auto details = std::move(_err).details();
  for (auto& detail : details) {
    if (!detail.location.has_value()) {
      detail.location = _location;
    }
  }
  return Error(std::move(what), std::move(details));
}

/**
 * @brief Fills in a source location from a reader on any error details that do
 * not already have one, preserving existing leaf locations.
 */
template <class R, class InputType>
RFL_ALWAYS_INLINE Error attach_source_location(const R& _r,
                                               const InputType& _input,
                                               Error&& _err,
                                               const bool _exact = true) {
  if constexpr (requires { _r.source_location(_input); }) {
    if (_err.has_all_source_locations()) {
      return std::move(_err);
    }
    return attach_source_location(get_source_location(_r, _input, _exact),
                                  std::move(_err));
  } else {
    return std::move(_err);
  }
}

}  // namespace rfl::parsing

#endif
