#ifndef RFL_JSON_STREAM_HPP_
#define RFL_JSON_STREAM_HPP_

#if __has_include(<yyjson.h>)
#include <yyjson.h>
#else
#include "../thirdparty/yyjson.h"
#endif

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <istream>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Processors.hpp"
#include "../Result.hpp"
#include "../internal/wrap_in_rfl_array_t.hpp"
#include "../parsing/Parent.hpp"
#include "Parser.hpp"
#include "Reader.hpp"
#include "Writer.hpp"
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
    _other.stream_ = nullptr;
    _other.finished_ = true;
  }

  StreamWriter& operator=(StreamWriter&&) = delete;

  ~StreamWriter() noexcept {
    try {
      (void)finish();
    } catch (...) {
    }
  }

  /// Serializes one record and writes it immediately with framing.
  Result<Nothing> write(const auto& _obj) {
    if (finished_) {
      return error("Cannot write to a finished StreamWriter.");
    }
    if ((format_ == StreamFormat::ndjson ||
         format_ == StreamFormat::json_seq) &&
        (flag_ & (YYJSON_WRITE_PRETTY | YYJSON_WRITE_PRETTY_TWO_SPACES |
                  YYJSON_WRITE_NEWLINE_AT_END)) != 0) {
      return error(
          "Pretty-printing and trailing newline flags are not allowed for "
          "NDJSON or JSON Text Sequences.");
    }
    if (!stream_ || !stream_->good()) {
      return error("Output stream is not in a good state.");
    }
    try {
      using T = std::remove_cvref_t<decltype(_obj)>;
      using ParentType = parsing::Parent<Writer>;
      auto w = Writer();
      Parser<T, Processors<Ps...>>::write(w, _obj, typename ParentType::Root{});
      yyjson_write_err err{};
      size_t len = 0;
      const std::unique_ptr<char, decltype(&std::free)> json_c_str(
          yyjson_mut_write_opts(w.doc(), flag_, nullptr, &len, &err),
          &std::free);
      if (!json_c_str) {
        return error("An error occurred while writing to JSON: " +
                     std::string(err.msg ? err.msg : "unknown error"));
      }
      switch (format_) {
        case StreamFormat::ndjson:
          stream_->write(json_c_str.get(), static_cast<std::streamsize>(len));
          stream_->put('\n');
          break;
        case StreamFormat::json_seq:
          stream_->put('\x1e');
          stream_->write(json_c_str.get(), static_cast<std::streamsize>(len));
          stream_->put('\n');
          break;
        case StreamFormat::json_array:
          stream_->put(started_ ? ',' : '[');
          started_ = true;
          stream_->write(json_c_str.get(), static_cast<std::streamsize>(len));
          break;
      }
      if (stream_->fail() || stream_->bad()) {
        return error("I/O error while writing JSON record to stream.");
      }
    } catch (const std::exception& _e) {
      return error(std::string("An error occurred while writing to JSON: ") +
                   _e.what());
    }
    return Nothing{};
  }

  /// Flushes the underlying output stream.
  Result<Nothing> flush() {
    if (!stream_) {
      return error("StreamWriter has no output stream.");
    }
    if (stream_->fail() || stream_->bad()) {
      return error("Output stream is not in a good state.");
    }
    try {
      stream_->flush();
      if (stream_->fail() || stream_->bad()) {
        return error("I/O error while flushing JSON stream.");
      }
    } catch (const std::exception& _e) {
      return error(std::string("I/O error while flushing JSON stream: ") +
                   _e.what());
    }
    return Nothing{};
  }

  /// Finishes the stream (emits `[]` or `]` for `json_array`). Idempotent.
  Result<Nothing> finish() {
    if (finished_) {
      return Nothing{};
    }
    finished_ = true;
    if (!stream_) {
      return error("StreamWriter has no output stream.");
    }
    try {
      if (format_ == StreamFormat::json_array) {
        if (started_) {
          stream_->put(']');
        } else {
          stream_->write("[]", 2);
        }
      }
      if (stream_->fail() || stream_->bad()) {
        return error("I/O error while finishing JSON stream.");
      }
    } catch (const std::exception& _e) {
      return error(std::string("I/O error while finishing JSON stream: ") +
                   _e.what());
    }
    return Nothing{};
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

  /// Default per-record memory cap (64 MiB). Pass `0` for unlimited.
  static constexpr size_t kDefaultMaxRecordBytes = 64 * 1024 * 1024;

  /// C++20 single-pass input iterator over streamed records.
  class Iterator {
   public:
    using iterator_concept = std::input_iterator_tag;
    using value_type = ValueType;
    using difference_type = std::ptrdiff_t;

    Iterator() noexcept = default;
    explicit Iterator(StreamReader* _reader) noexcept : reader_(_reader) {}

    ValueType& operator*() const noexcept { return *reader_->current_; }
    ValueType* operator->() const noexcept { return &(*reader_->current_); }

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

  explicit StreamReader(
      std::istream& _stream, const StreamFormat _format = StreamFormat::ndjson,
      const yyjson_read_flag _flag = 0, const size_t _chunk_size = 65536,
      const size_t _max_record_bytes = kDefaultMaxRecordBytes)
      : stream_(&_stream),
        format_(_format),
        flag_(_flag),
        chunk_size_(_chunk_size > 0 ? _chunk_size : 1),
        max_record_bytes_(_max_record_bytes),
        invalid_flag_((_flag & YYJSON_READ_INSITU) != 0) {}

  explicit StreamReader(
      const std::string_view _input,
      const StreamFormat _format = StreamFormat::ndjson,
      const yyjson_read_flag _flag = 0,
      const size_t _max_record_bytes = kDefaultMaxRecordBytes)
      : stream_(nullptr),
        format_(_format),
        flag_(_flag),
        chunk_size_(0),
        max_record_bytes_(_max_record_bytes),
        input_(_input),
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

  std::default_sentinel_t end() const noexcept { return std::default_sentinel; }

 private:
  static constexpr size_t kInitialWindow = 4096;

  enum class ArrayState { before_open, expect_first, expect_sep_end, closed };
  enum class SkipResult { found, eof, io_error, unclosed_comment };

  static bool is_json_ws(const char _c) noexcept {
    return _c == ' ' || _c == '\t' || _c == '\n' || _c == '\r';
  }

  static size_t safe_double(const size_t _n) noexcept {
    constexpr size_t kMax = std::numeric_limits<size_t>::max();
    return _n > kMax / 2 ? kMax : _n * 2;
  }

  static size_t safe_inc(const size_t _n) noexcept {
    constexpr size_t kMax = std::numeric_limits<size_t>::max();
    return _n < kMax ? _n + 1 : kMax;
  }

  const char* data_ptr() const noexcept {
    return stream_ ? buf_.data() : input_.data();
  }

  size_t data_size() const noexcept {
    return stream_ ? buf_.size() : input_.size();
  }

  size_t avail() const noexcept { return data_size() - pos_; }

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
    if (!stream_ || pos_ == 0) {
      return;
    }
    if (pos_ >= buf_.size()) {
      buf_.clear();
      pos_ = 0;
    } else if (pos_ >= chunk_size_ || pos_ * 2 >= buf_.size()) {
      buf_.erase(0, pos_);
      pos_ = 0;
    }
  }

  /// Reads up to `_requested` bytes from `*stream_` without blocking past the
  /// first available OS read (or first line terminator on unbuffered streams).
  size_t refill_raw(const size_t _requested, const bool _line_mode,
                    const bool _stop_at_rs) {
    if (eof_ || !stream_) {
      eof_ = true;
      return 0;
    }
    if (_requested == 0) {
      return 0;
    }
    if (stream_->bad() || (stream_->fail() && !stream_->eof())) {
      io_error_ = true;
      eof_ = true;
      return 0;
    }
    std::streambuf* sb = stream_->rdbuf();
    if (!sb) {
      eof_ = true;
      return 0;
    }

    compact_buffer();
    const size_t old_size = buf_.size();
    const size_t max_string_add = buf_.max_size() - old_size;
    const size_t max_stream_add =
        static_cast<size_t>(std::numeric_limits<std::streamsize>::max());
    const size_t to_read =
        std::min({_requested, max_string_add, max_stream_add});
    if (to_read == 0) {
      return 0;
    }

    using Traits = std::istream::traits_type;
    auto drain_available = [&](const size_t _limit) -> size_t {
      if (_limit == 0) {
        return 0;
      }
      const std::streamsize avail_now = sb->in_avail();
      if (avail_now < 0) {
        eof_ = true;
        return 0;
      }
      if (avail_now == 0) {
        return 0;
      }
      const size_t n_want = std::min(_limit, static_cast<size_t>(avail_now));
      const size_t cur = buf_.size();
      const size_t needed = cur + n_want;
      if (buf_.capacity() < needed) {
        const size_t doubled = safe_double(buf_.capacity());
        buf_.reserve(std::min(buf_.max_size(), std::max(needed, doubled)));
      }
      buf_.resize(needed);
      const std::streamsize got =
          sb->sgetn(buf_.data() + cur, static_cast<std::streamsize>(n_want));
      const size_t n_got = got > 0 ? static_cast<size_t>(got) : size_t{0};
      buf_.resize(cur + n_got);
      return n_got;
    };

    size_t total_got = 0;
    try {
      const std::streamsize initial_avail = sb->in_avail();
      if (initial_avail < 0) {
        eof_ = true;
        return 0;
      }
      if (initial_avail > 0) {
        total_got = drain_available(to_read);
      } else {
        // Nothing buffered: block once for the first byte.
        const auto ch = sb->sbumpc();
        if (Traits::eq_int_type(ch, Traits::eof())) {
          eof_ = true;
          if (stream_->bad()) {
            io_error_ = true;
          }
          return 0;
        }
        const char first = Traits::to_char_type(ch);
        buf_.push_back(first);
        total_got = 1;

        if (!(_line_mode && first == '\n') && to_read > 1) {
          const std::streamsize after_avail = sb->in_avail();
          if (after_avail > 0) {
            total_got += drain_available(to_read - 1);
          } else if (after_avail == 0 && _line_mode) {
            // Unbuffered stream in line mode: read up to '\n' (or '\x1e').
            while (total_got < to_read) {
              const auto next_ch = sb->sbumpc();
              if (Traits::eq_int_type(next_ch, Traits::eof())) {
                eof_ = true;
                break;
              }
              const char c = Traits::to_char_type(next_ch);
              buf_.push_back(c);
              ++total_got;
              if (c == '\n' || (_stop_at_rs && c == '\x1e')) {
                break;
              }
            }
          }
        }
      }
    } catch (...) {
      io_error_ = true;
      eof_ = true;
      return 0;
    }

    if (stream_->bad()) {
      io_error_ = true;
      eof_ = true;
      return 0;
    }
    return total_got;
  }

  /// Refills while enforcing `max_record_bytes_` (allowing 1 lookahead byte
  /// for delimiters after a record of exact size `max_record_bytes_`).
  size_t refill_for_record(const size_t _requested, const bool _line_mode,
                           const bool _stop_at_rs) {
    size_t allowed = _requested;
    if (max_record_bytes_ > 0) {
      const size_t cap = safe_inc(max_record_bytes_);
      const size_t cur_avail = avail();
      if (cur_avail >= cap) {
        record_too_large_ = true;
        return 0;
      }
      allowed = std::min(allowed, cap - cur_avail);
    }
    return refill_raw(allowed, _line_mode, _stop_at_rs);
  }

  bool skip_bom_if_needed() {
    if (bom_checked_) {
      return true;
    }
    bom_checked_ = true;
    if ((flag_ & YYJSON_READ_ALLOW_BOM) == 0) {
      return true;
    }
    const bool line_mode = (format_ != StreamFormat::json_array);
    const bool stop_at_rs = (format_ == StreamFormat::json_seq);
    if (avail() == 0 && !eof_) {
      refill_raw(chunk_size_, line_mode, stop_at_rs);
      if (io_error_) {
        return false;
      }
    }
    if (avail() == 0 ||
        static_cast<unsigned char>(data_ptr()[pos_]) != 0xEF) {
      return true;
    }
    while (avail() < 3 && !eof_) {
      if (refill_raw(chunk_size_, line_mode, stop_at_rs) == 0) {
        break;
      }
    }
    if (io_error_) {
      return false;
    }
    if (avail() >= 3 &&
        static_cast<unsigned char>(data_ptr()[pos_]) == 0xEF &&
        static_cast<unsigned char>(data_ptr()[pos_ + 1]) == 0xBB &&
        static_cast<unsigned char>(data_ptr()[pos_ + 2]) == 0xBF) {
      pos_ += 3;
      compact_buffer();
    }
    return true;
  }

  SkipResult skip_ws(const bool _skip_rs = false) {
    if (!skip_bom_if_needed()) {
      return SkipResult::io_error;
    }
    const bool allow_comments = (flag_ & YYJSON_READ_ALLOW_COMMENTS) != 0;
    const bool line_mode = (format_ != StreamFormat::json_array);
    const bool stop_at_rs = (format_ == StreamFormat::json_seq);

    while (true) {
      while (pos_ < data_size()) {
        const char c = data_ptr()[pos_];
        if (is_json_ws(c) || (_skip_rs && c == '\x1e')) {
          ++pos_;
          continue;
        }
        if (allow_comments && c == '/') {
          while (avail() < 2 && !eof_) {
            if (refill_raw(chunk_size_, line_mode, stop_at_rs) == 0) {
              break;
            }
          }
          if (io_error_) {
            return SkipResult::io_error;
          }
          if (avail() < 2) {
            return SkipResult::found;
          }
          const char next_c = data_ptr()[pos_ + 1];
          if (next_c == '/') {
            pos_ += 2;
            while (true) {
              while (pos_ < data_size()) {
                const char cc = data_ptr()[pos_];
                if (cc == '\n' || cc == '\r') {
                  break;
                }
                ++pos_;
              }
              if (pos_ < data_size()) {
                break;
              }
              if (refill_raw(chunk_size_, line_mode, stop_at_rs) == 0) {
                return io_error_ ? SkipResult::io_error : SkipResult::eof;
              }
            }
            continue;
          }
          if (next_c == '*') {
            pos_ += 2;
            bool closed = false;
            while (!closed) {
              while (pos_ < data_size()) {
                if (data_ptr()[pos_] == '*') {
                  if (pos_ + 1 >= data_size()) {
                    if (eof_) {
                      return SkipResult::unclosed_comment;
                    }
                    if (refill_raw(chunk_size_, line_mode, stop_at_rs) == 0) {
                      return io_error_ ? SkipResult::io_error
                                       : SkipResult::unclosed_comment;
                    }
                    continue;
                  }
                  if (data_ptr()[pos_ + 1] == '/') {
                    pos_ += 2;
                    closed = true;
                    break;
                  }
                }
                ++pos_;
              }
              if (!closed) {
                if (refill_raw(chunk_size_, line_mode, stop_at_rs) == 0) {
                  return io_error_ ? SkipResult::io_error
                                   : SkipResult::unclosed_comment;
                }
              }
            }
            continue;
          }
        }
        return SkipResult::found;
      }
      if (refill_raw(chunk_size_, line_mode, stop_at_rs) == 0) {
        return io_error_ ? SkipResult::io_error : SkipResult::eof;
      }
    }
  }

  void advance_past_newline() {
    const char* ptr = data_ptr() + pos_;
    const size_t cur_avail = avail();
    if (const char* nl =
            static_cast<const char*>(std::memchr(ptr, '\n', cur_avail))) {
      pos_ += static_cast<size_t>(nl - ptr) + 1;
      compact_buffer();
      return;
    }
    pos_ = data_size();
    compact_buffer();
    while (!eof_) {
      if (refill_raw(chunk_size_, true, false) == 0) {
        break;
      }
      const char* p = data_ptr() + pos_;
      const size_t a = avail();
      if (const char* nl = static_cast<const char*>(std::memchr(p, '\n', a))) {
        pos_ += static_cast<size_t>(nl - p) + 1;
        compact_buffer();
        return;
      }
      pos_ = data_size();
      compact_buffer();
    }
  }

  void resync_to_rs() {
    const char* ptr = data_ptr() + pos_;
    const size_t cur_avail = avail();
    if (cur_avail > 1) {
      if (const char* rs = static_cast<const char*>(
              std::memchr(ptr + 1, '\x1e', cur_avail - 1))) {
        pos_ += static_cast<size_t>(rs - ptr);
        compact_buffer();
        return;
      }
    }
    pos_ = data_size();
    compact_buffer();
    while (!eof_) {
      if (refill_raw(chunk_size_, true, true) == 0) {
        break;
      }
      const char* p = data_ptr() + pos_;
      const size_t a = avail();
      if (const char* rs =
              static_cast<const char*>(std::memchr(p, '\x1e', a))) {
        pos_ += static_cast<size_t>(rs - p);
        compact_buffer();
        return;
      }
      pos_ = data_size();
      compact_buffer();
    }
  }

  std::optional<ValueType> parse_delimited_value(const bool _is_seq) {
    const yyjson_read_flag effective_flag = flag_ | YYJSON_READ_STOP_WHEN_DONE;
    record_too_large_ = false;
    size_t window = kInitialWindow;
    size_t refill_step = chunk_size_;
    bool expanded = false;

    while (true) {
      if (avail() == 0) {
        if (refill_for_record(refill_step, true, _is_seq) == 0) {
          if (record_too_large_) {
            return fail("JSON record exceeds maximum allowed size.");
          }
          if (io_error_) {
            return fail("I/O error while reading JSON stream.");
          }
          done_ = true;
          return std::nullopt;
        }
        continue;
      }

      const char* ptr = data_ptr() + pos_;
      const size_t cur_avail = avail();

      const size_t probe_len = std::min(cur_avail, window);
      const char* rs_ptr =
          _is_seq
              ? static_cast<const char*>(std::memchr(ptr, '\x1e', probe_len))
              : nullptr;
      const size_t nl_search_len =
          rs_ptr ? static_cast<size_t>(rs_ptr - ptr) : probe_len;
      const char* nl_ptr =
          !expanded
              ? static_cast<const char*>(std::memchr(ptr, '\n', nl_search_len))
              : nullptr;

      size_t slice_len = 0;
      bool hard_boundary = false;

      if (nl_ptr) {
        slice_len = static_cast<size_t>(nl_ptr - ptr) + 1;
        hard_boundary = false;
      } else if (rs_ptr) {
        slice_len = static_cast<size_t>(rs_ptr - ptr);
        hard_boundary = true;
      } else if (!expanded && !eof_ && cur_avail < window) {
        if (refill_for_record(refill_step, true, _is_seq) > 0) {
          continue;
        }
        if (record_too_large_) {
          return fail("JSON record exceeds maximum allowed size.");
        }
        if (io_error_) {
          return fail("I/O error while reading JSON stream.");
        }
        slice_len = avail();
        hard_boundary = true;
      } else {
        slice_len = probe_len;
        hard_boundary = (slice_len == cur_avail && eof_);
      }

      bool hit_size_cap = false;
      if (max_record_bytes_ > 0) {
        const size_t allowed_slice = safe_inc(max_record_bytes_);
        if (slice_len > allowed_slice) {
          slice_len = allowed_slice;
          hit_size_cap = true;
          hard_boundary = false;
        } else if (slice_len == allowed_slice && !nl_ptr && !rs_ptr &&
                   !(slice_len == cur_avail && eof_)) {
          hit_size_cap = true;
        }
      }

      if (slice_len == 0) {
        if (hit_size_cap) {
          return fail("JSON record exceeds maximum allowed size.");
        }
        done_ = true;
        return std::nullopt;
      }

      yyjson_read_err err{};
      yyjson_doc* doc = yyjson_read_opts(const_cast<char*>(ptr), slice_len,
                                         effective_flag, nullptr, &err);
      if (doc) {
        const size_t read_size = yyjson_doc_get_read_size(doc);
        yyjson_val* root = yyjson_doc_get_root(doc);
        const bool is_self_delimiting =
            yyjson_is_ctn(root) || yyjson_is_str(root);

        if (!is_self_delimiting && read_size == slice_len && !hard_boundary) {
          yyjson_doc_free(doc);
          if (hit_size_cap ||
              (max_record_bytes_ > 0 && read_size > max_record_bytes_)) {
            return fail("JSON record exceeds maximum allowed size.");
          }
          expanded = true;
          if (slice_len < cur_avail) {
            window = std::max(safe_double(slice_len), safe_double(window));
          } else if (!eof_) {
            refill_step = safe_double(refill_step);
            window = std::max(safe_double(slice_len), window);
            if (refill_for_record(refill_step, true, _is_seq) == 0) {
              if (record_too_large_) {
                return fail("JSON record exceeds maximum allowed size.");
              }
              if (io_error_) {
                return fail("I/O error while reading JSON stream.");
              }
            }
          }
          continue;
        }

        if (max_record_bytes_ > 0 && read_size > max_record_bytes_) {
          yyjson_doc_free(doc);
          return fail("JSON record exceeds maximum allowed size.");
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

      if (maybe_truncated) {
        if (hit_size_cap ||
            (max_record_bytes_ > 0 && slice_len >= safe_inc(max_record_bytes_))) {
          return fail("JSON record exceeds maximum allowed size.");
        }
        if (!hard_boundary) {
          expanded = true;
          if (slice_len < cur_avail) {
            window = std::max(safe_double(slice_len), safe_double(window));
            continue;
          }
          if (!eof_) {
            refill_step = safe_double(refill_step);
            window = std::max(safe_double(slice_len), safe_double(window));
            if (refill_for_record(refill_step, true, _is_seq) > 0) {
              continue;
            }
            if (record_too_large_) {
              return fail("JSON record exceeds maximum allowed size.");
            }
            if (io_error_) {
              return fail("I/O error while reading JSON stream.");
            }
          }
        }
      }

      const std::string err_msg =
          "Could not parse document: " +
          std::string(err.msg ? err.msg : "unknown error");
      if (_is_seq) {
        resync_to_rs();
      } else {
        advance_past_newline();
      }
      if (io_error_) {
        return fail("I/O error while reading JSON stream.");
      }
      return error(err_msg);
    }
  }

  std::optional<ValueType> parse_array_value() {
    const yyjson_read_flag effective_flag = flag_ | YYJSON_READ_STOP_WHEN_DONE;
    record_too_large_ = false;
    size_t window = kInitialWindow;
    size_t refill_step = chunk_size_;

    while (true) {
      if (avail() == 0) {
        if (refill_for_record(refill_step, false, false) == 0) {
          if (record_too_large_) {
            return fail("JSON record exceeds maximum allowed size.");
          }
          return eof_or_io_error(
              "Could not parse document: unexpected end of data");
        }
        continue;
      }

      const char* ptr = data_ptr() + pos_;
      const size_t cur_avail = avail();
      size_t slice_len = std::min(cur_avail, window);
      bool hit_size_cap = false;

      if (max_record_bytes_ > 0) {
        const size_t allowed_slice = safe_inc(max_record_bytes_);
        if (slice_len > allowed_slice) {
          slice_len = allowed_slice;
          hit_size_cap = true;
        } else if (slice_len == allowed_slice &&
                   !(slice_len == cur_avail && eof_)) {
          hit_size_cap = true;
        }
      }

      yyjson_read_err err{};
      yyjson_doc* doc = yyjson_read_opts(const_cast<char*>(ptr), slice_len,
                                         effective_flag, nullptr, &err);
      if (doc) {
        const size_t read_size = yyjson_doc_get_read_size(doc);
        yyjson_val* root = yyjson_doc_get_root(doc);
        const bool is_self_delimiting =
            yyjson_is_ctn(root) || yyjson_is_str(root);

        const bool more_bytes_possible = (slice_len < cur_avail) || !eof_;
        if (!is_self_delimiting && read_size == slice_len &&
            more_bytes_possible) {
          yyjson_doc_free(doc);
          if (hit_size_cap ||
              (max_record_bytes_ > 0 && read_size > max_record_bytes_)) {
            return fail("JSON record exceeds maximum allowed size.");
          }
          if (slice_len < cur_avail) {
            window = safe_double(window);
          } else {
            refill_step = safe_double(refill_step);
            window = std::max(safe_double(slice_len), window);
            if (refill_for_record(refill_step, false, false) == 0) {
              if (record_too_large_) {
                return fail("JSON record exceeds maximum allowed size.");
              }
              if (io_error_) {
                return fail("I/O error while reading JSON stream.");
              }
            }
          }
          continue;
        }

        if (max_record_bytes_ > 0 && read_size > max_record_bytes_) {
          yyjson_doc_free(doc);
          return fail("JSON record exceeds maximum allowed size.");
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
      if (maybe_truncated) {
        if (hit_size_cap ||
            (max_record_bytes_ > 0 && slice_len >= safe_inc(max_record_bytes_))) {
          return fail("JSON record exceeds maximum allowed size.");
        }
        if (slice_len < cur_avail) {
          window = safe_double(window);
          continue;
        }
        if (!eof_) {
          refill_step = safe_double(refill_step);
          window = std::max(safe_double(slice_len), window);
          if (refill_for_record(refill_step, false, false) > 0) {
            continue;
          }
          if (record_too_large_) {
            return fail("JSON record exceeds maximum allowed size.");
          }
          if (io_error_) {
            return fail("I/O error while reading JSON stream.");
          }
        }
      }

      done_ = true;
      return error("Could not parse document: " +
                   std::string(err.msg ? err.msg : "unknown error"));
    }
  }

  std::optional<ValueType> next_delimited(const bool _skip_rs) {
    const auto skipped = skip_ws(_skip_rs);
    if (skipped == SkipResult::eof) {
      done_ = true;
      return std::nullopt;
    }
    if (skipped == SkipResult::io_error) {
      return fail("I/O error while reading JSON stream.");
    }
    if (skipped == SkipResult::unclosed_comment) {
      return fail("Unclosed multi-line comment in JSON stream.");
    }
    return parse_delimited_value(_skip_rs);
  }

  std::optional<ValueType> finish_array() {
    ++pos_;  // consume ']'
    array_state_ = ArrayState::closed;
    done_ = true;
    const auto skipped = skip_ws(false);
    if (skipped == SkipResult::found) {
      return error(
          "Unexpected trailing content after closing ']' of JSON array.");
    }
    if (skipped == SkipResult::io_error) {
      return error("I/O error while reading JSON stream.");
    }
    if (skipped == SkipResult::unclosed_comment) {
      return error("Unclosed multi-line comment in JSON stream.");
    }
    return std::nullopt;
  }

  std::optional<ValueType> next_array_element() {
    auto handle_skip_error = [&](const SkipResult _sr,
                                 const char* _eof_msg) -> std::optional<ValueType> {
      if (_sr == SkipResult::unclosed_comment) {
        return fail("Unclosed multi-line comment in JSON stream.");
      }
      return eof_or_io_error(_eof_msg);
    };

    if (array_state_ == ArrayState::before_open) {
      const auto skipped = skip_ws(false);
      if (skipped != SkipResult::found) {
        return handle_skip_error(
            skipped, "Expected '[' at start of JSON array, got end of input.");
      }
      if (data_ptr()[pos_] != '[') {
        return fail("Expected '[' at start of JSON array.");
      }
      ++pos_;
      array_state_ = ArrayState::expect_first;
    }

    if (array_state_ == ArrayState::expect_first) {
      const auto skipped = skip_ws(false);
      if (skipped != SkipResult::found) {
        return handle_skip_error(
            skipped,
            "Unexpected end of input in JSON array: expected ']' or value.");
      }
      if (data_ptr()[pos_] == ']') {
        return finish_array();
      }
      auto res = parse_array_value();
      if (!done_) {
        array_state_ = ArrayState::expect_sep_end;
      }
      return res;
    }

    if (array_state_ == ArrayState::expect_sep_end) {
      const auto skipped = skip_ws(false);
      if (skipped != SkipResult::found) {
        return handle_skip_error(
            skipped,
            "Unexpected end of input in JSON array: expected ',' or ']'.");
      }
      if (data_ptr()[pos_] == ']') {
        return finish_array();
      }
      if (data_ptr()[pos_] != ',') {
        return fail("Expected ',' or ']' in JSON array.");
      }
      ++pos_;  // consume ','
      const auto after_comma = skip_ws(false);
      if (after_comma != SkipResult::found) {
        return handle_skip_error(
            after_comma, "Unexpected end of input in JSON array after ','.");
      }
      if (data_ptr()[pos_] == ']') {
        if (flag_ & YYJSON_READ_ALLOW_TRAILING_COMMAS) {
          return finish_array();
        }
        return fail("Trailing comma is not allowed in JSON array.");
      }
      return parse_array_value();
    }

    done_ = true;
    return std::nullopt;
  }

  std::istream* stream_;
  StreamFormat format_;
  yyjson_read_flag flag_;
  size_t chunk_size_;
  size_t max_record_bytes_;
  std::string_view input_;
  std::string buf_;
  size_t pos_ = 0;
  bool eof_ = false;
  bool io_error_ = false;
  bool record_too_large_ = false;
  bool bom_checked_ = false;
  bool done_ = false;
  bool invalid_flag_ = false;
  ArrayState array_state_ = ArrayState::before_open;
  std::optional<ValueType> current_;
};

}  // namespace json
}  // namespace rfl

#endif
