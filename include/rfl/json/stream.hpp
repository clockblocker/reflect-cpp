#ifndef RFL_JSON_STREAM_HPP_
#define RFL_JSON_STREAM_HPP_

#if __has_include(<yyjson.h>)
#include <yyjson.h>
#else
#include "../thirdparty/yyjson.h"
#endif

#include <algorithm>
#include <cstddef>
#include <istream>
#include <iterator>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "../Processors.hpp"
#include "../Result.hpp"
#include "../internal/wrap_in_rfl_array_t.hpp"
#include "Parser.hpp"
#include "Reader.hpp"
#include "write.hpp"

namespace rfl {
namespace json {

/// Framing format for record-level JSON streaming.
enum class StreamFormat {
  ndjson,      ///< Newline-delimited JSON (`{...}\n{...}\n`).
  json_seq,    ///< RFC 7464 JSON Text Sequences (`\x1e{...}\n`).
  json_array,  ///< Top-level JSON array (`[{...},{...}]`).
};

/// Record-level streaming JSON writer.
template <class... Ps>
class StreamWriter {
 public:
  explicit StreamWriter(std::ostream& _stream,
                        const StreamFormat _format = StreamFormat::ndjson,
                        const yyjson_write_flag _flag = 0)
      : stream_(&_stream), format_(_format), flag_(_flag) {}

  StreamWriter(const StreamWriter&) = delete;
  StreamWriter& operator=(const StreamWriter&) = delete;

  StreamWriter(StreamWriter&& _other) noexcept
      : stream_(_other.stream_),
        format_(_other.format_),
        flag_(_other.flag_),
        started_(_other.started_),
        finished_(_other.finished_) {
    _other.finished_ = true;
  }

  StreamWriter& operator=(StreamWriter&&) = delete;

  ~StreamWriter() noexcept {
    try {
      finish();
    } catch (...) {
    }
  }

  /// Serializes one record and writes it immediately with framing.
  void write(const auto& _obj) {
    if (finished_) {
      throw std::runtime_error("Cannot write to a finished StreamWriter.");
    }
    const auto encoded = rfl::json::write<Ps...>(_obj, flag_);
    switch (format_) {
      case StreamFormat::ndjson:
        (*stream_) << encoded << '\n';
        break;
      case StreamFormat::json_seq:
        (*stream_) << '\x1e' << encoded << '\n';
        break;
      case StreamFormat::json_array:
        (*stream_) << (started_ ? ',' : '[');
        started_ = true;
        (*stream_) << encoded;
        break;
    }
  }

  /// Finishes the stream (emits `[]` or `]` for `json_array`). Idempotent.
  void finish() {
    if (finished_) {
      return;
    }
    finished_ = true;
    if (format_ == StreamFormat::json_array) {
      (*stream_) << (started_ ? "]" : "[]");
    }
  }

 private:
  std::ostream* stream_;
  StreamFormat format_;
  yyjson_write_flag flag_;
  bool started_ = false;
  bool finished_ = false;
};

/// Record-level streaming JSON reader.
template <class T, class... Ps>
class StreamReader {
 public:
  using ValueType = Result<internal::wrap_in_rfl_array_t<T>>;

  /// C++20 single-pass input iterator over streamed records.
  class Iterator {
   public:
    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using value_type = ValueType;
    using difference_type = std::ptrdiff_t;

    Iterator() noexcept = default;
    explicit Iterator(StreamReader* _reader) noexcept : reader_(_reader) {}

    ValueType& operator*() const noexcept { return reader_->current_.value(); }
    ValueType* operator->() const noexcept {
      return &reader_->current_.value();
    }

    Iterator& operator++() {
      if (reader_) {
        reader_->current_ = reader_->next();
        if (!reader_->current_.has_value()) {
          reader_ = nullptr;
        }
      }
      return *this;
    }

    void operator++(int) { ++(*this); }

    bool operator==(const Iterator&) const noexcept = default;
    bool operator==(std::default_sentinel_t) const noexcept {
      return reader_ == nullptr;
    }

   private:
    StreamReader* reader_ = nullptr;
  };

  explicit StreamReader(std::istream& _stream,
                        const StreamFormat _format = StreamFormat::ndjson,
                        const yyjson_read_flag _flag = 0,
                        const size_t _chunk_size = 65536)
      : stream_(&_stream),
        format_(_format),
        flag_(_flag),
        chunk_size_(_chunk_size > 0 ? _chunk_size : 1),
        invalid_flag_((_flag & YYJSON_READ_INSITU) != 0) {}

  explicit StreamReader(const std::string_view _input,
                        const StreamFormat _format = StreamFormat::ndjson,
                        const yyjson_read_flag _flag = 0)
      : stream_(nullptr),
        format_(_format),
        flag_(_flag),
        chunk_size_(0),
        buf_(_input),
        eof_(true),
        invalid_flag_((_flag & YYJSON_READ_INSITU) != 0) {}

  StreamReader(const StreamReader&) = delete;
  StreamReader& operator=(const StreamReader&) = delete;
  StreamReader(StreamReader&&) = default;
  StreamReader& operator=(StreamReader&&) = default;

  /// Reads the next record, or returns `std::nullopt` at clean EOF.
  std::optional<ValueType> next() {
    if (done_) {
      return std::nullopt;
    }
    if (invalid_flag_) {
      return fail("YYJSON_READ_INSITU is not supported");
    }
    switch (format_) {
      case StreamFormat::ndjson:
        return next_delimited(false);
      case StreamFormat::json_seq:
        return next_delimited(true);
      case StreamFormat::json_array:
        return next_array_element();
    }
    done_ = true;
    return std::nullopt;
  }

  Iterator begin() {
    current_ = next();
    return current_.has_value() ? Iterator(this) : Iterator();
  }

  Iterator end() const noexcept { return Iterator(); }

 private:
  enum class ArrayState { before_open, expect_first, expect_sep_end, closed };

  static bool is_json_ws(const char _c) noexcept {
    return _c == ' ' || _c == '\t' || _c == '\n' || _c == '\r';
  }

  std::optional<ValueType> fail(const std::string& _msg) {
    done_ = true;
    return error(_msg);
  }

  std::optional<ValueType> eof_or_io_error(const char* _eof_msg) {
    done_ = true;
    if (io_error_) {
      return error("I/O error while reading JSON stream.");
    }
    return error(_eof_msg);
  }

  void compact_buffer() {
    if (pos_ == 0) {
      return;
    }
    if (pos_ >= buf_.size()) {
      buf_.clear();
      pos_ = 0;
    } else if (stream_ != nullptr &&
               (pos_ >= chunk_size_ || pos_ * 2 >= buf_.size())) {
      buf_.erase(0, pos_);
      pos_ = 0;
    }
  }

  bool refill() {
    if (eof_ || !stream_) {
      eof_ = true;
      return false;
    }
    compact_buffer();
    const size_t old_size = buf_.size();
    const size_t needed = old_size + chunk_size_;
    if (buf_.capacity() < needed) {
      buf_.reserve(std::max(needed, buf_.capacity() * 2));
    }
    buf_.resize(needed);
    stream_->read(buf_.data() + old_size,
                  static_cast<std::streamsize>(chunk_size_));
    const auto gcount = stream_->gcount();
    const size_t n = gcount > 0 ? static_cast<size_t>(gcount) : size_t{0};
    buf_.resize(old_size + n);
    if (stream_->bad()) {
      io_error_ = true;
      eof_ = true;
      return false;
    }
    if (n == 0) {
      eof_ = true;
      return false;
    }
    if (stream_->eof()) {
      eof_ = true;
    }
    return true;
  }

  bool skip_ws(const bool _skip_rs = false) {
    while (true) {
      while (pos_ < buf_.size()) {
        const char c = buf_[pos_];
        if (is_json_ws(c) || (_skip_rs && c == '\x1e')) {
          ++pos_;
        } else {
          return true;
        }
      }
      if (!refill()) {
        return false;
      }
    }
  }

  std::optional<ValueType> parse_next_value(const bool _allow_empty_eof) {
    const yyjson_read_flag effective_flag = flag_ | YYJSON_READ_STOP_WHEN_DONE;
    while (true) {
      const size_t avail = buf_.size() - pos_;
      if (avail == 0) {
        if (!refill()) {
          if (!io_error_ && _allow_empty_eof) {
            done_ = true;
            return std::nullopt;
          }
          return eof_or_io_error(
              "Could not parse document: unexpected end of data");
        }
        continue;
      }

      yyjson_read_err err{};
      yyjson_doc* doc = yyjson_read_opts(buf_.data() + pos_, avail,
                                         effective_flag, nullptr, &err);
      if (doc) {
        const size_t read_size = yyjson_doc_get_read_size(doc);
        yyjson_val* root = yyjson_doc_get_root(doc);
        const bool is_self_delimiting =
            yyjson_is_ctn(root) || yyjson_is_str(root);
        // Refill before committing bare primitives (e.g. `12` + `3`) at the
        // exact buffer boundary when more bytes may still arrive.
        if (!is_self_delimiting && pos_ + read_size == buf_.size() && !eof_) {
          yyjson_doc_free(doc);
          if (!refill() && io_error_) {
            return fail("I/O error while reading JSON stream.");
          }
          continue;
        }

        pos_ += read_size;
        const auto r = Reader();
        auto res =
            Parser<T, Processors<Ps...>>::read(r, Reader::InputVarType(root));
        yyjson_doc_free(doc);
        compact_buffer();
        return res;
      }

      const bool maybe_truncated =
          (err.code == YYJSON_READ_ERROR_UNEXPECTED_END ||
           err.code == YYJSON_READ_ERROR_EMPTY_CONTENT);
      if (maybe_truncated && !eof_) {
        if (refill()) {
          continue;
        }
        if (io_error_) {
          return fail("I/O error while reading JSON stream.");
        }
      }

      done_ = true;
      if (_allow_empty_eof && err.code == YYJSON_READ_ERROR_EMPTY_CONTENT) {
        return std::nullopt;
      }
      return error("Could not parse document: " +
                   std::string(err.msg ? err.msg : "unknown error"));
    }
  }

  std::optional<ValueType> next_delimited(const bool _skip_rs) {
    if (!skip_ws(_skip_rs)) {
      done_ = true;
      if (io_error_) {
        return error("I/O error while reading JSON stream.");
      }
      return std::nullopt;
    }
    return parse_next_value(true);
  }

  std::optional<ValueType> finish_array() {
    ++pos_;  // consume ']'
    array_state_ = ArrayState::closed;
    done_ = true;
    if (skip_ws(false)) {
      return error(
          "Unexpected trailing content after closing ']' of JSON array.");
    }
    if (io_error_) {
      return error("I/O error while reading JSON stream.");
    }
    return std::nullopt;
  }

  std::optional<ValueType> next_array_element() {
    if (array_state_ == ArrayState::before_open) {
      if (!skip_ws(false)) {
        return eof_or_io_error(
            "Expected '[' at start of JSON array, got end of input.");
      }
      if (buf_[pos_] != '[') {
        return fail("Expected '[' at start of JSON array.");
      }
      ++pos_;
      array_state_ = ArrayState::expect_first;
    }

    if (array_state_ == ArrayState::expect_first) {
      if (!skip_ws(false)) {
        return eof_or_io_error(
            "Unexpected end of input in JSON array: expected ']' or value.");
      }
      if (buf_[pos_] == ']') {
        return finish_array();
      }
      auto res = parse_next_value(false);
      if (!done_) {
        array_state_ = ArrayState::expect_sep_end;
      }
      return res;
    }

    if (array_state_ == ArrayState::expect_sep_end) {
      if (!skip_ws(false)) {
        return eof_or_io_error(
            "Unexpected end of input in JSON array: expected ',' or ']'.");
      }
      if (buf_[pos_] == ']') {
        return finish_array();
      }
      if (buf_[pos_] != ',') {
        return fail("Expected ',' or ']' in JSON array.");
      }
      ++pos_;  // consume ','
      if (!skip_ws(false)) {
        return eof_or_io_error(
            "Unexpected end of input in JSON array after ','.");
      }
      if (buf_[pos_] == ']') {
        if (flag_ & YYJSON_READ_ALLOW_TRAILING_COMMAS) {
          return finish_array();
        }
        return fail("Trailing comma is not allowed in JSON array.");
      }
      return parse_next_value(false);
    }

    done_ = true;
    return std::nullopt;
  }

  std::istream* stream_;
  StreamFormat format_;
  yyjson_read_flag flag_;
  size_t chunk_size_;
  std::string buf_;
  size_t pos_ = 0;
  bool eof_ = false;
  bool io_error_ = false;
  bool done_ = false;
  bool invalid_flag_ = false;
  ArrayState array_state_ = ArrayState::before_open;
  std::optional<ValueType> current_;
};

}  // namespace json
}  // namespace rfl

#endif
