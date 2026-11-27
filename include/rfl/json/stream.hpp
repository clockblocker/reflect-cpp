#ifndef RFL_JSON_STREAM_HPP_
#define RFL_JSON_STREAM_HPP_

#if __has_include(<yyjson.h>)
#include <yyjson.h>
#else
#include "../thirdparty/yyjson.h"
#endif

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <istream>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Processors.hpp"
#include "../Result.hpp"
#include "../common.hpp"
#include "../internal/wrap_in_rfl_array_t.hpp"
#include "../parsing/Parent.hpp"
#include "Parser.hpp"
#include "Reader.hpp"
#include "Writer.hpp"

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
        finished_(_other.finished_),
        finish_error_(std::move(_other.finish_error_)) {
    _other.stream_ = nullptr;
    _other.finished_ = true;
    _other.finish_error_.reset();
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
    if (!stream_ || stream_->fail() || stream_->bad()) {
      return error("Output stream is not in a good state.",
                   ErrorKind::io_error);
    }
    if (stream_->eof()) {
      stream_->clear(stream_->rdstate() & ~std::ios_base::eofbit);
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
        return error("I/O error while writing JSON record to stream.",
                     ErrorKind::io_error);
      }
    } catch (const std::exception& _e) {
      if (stream_->fail() || stream_->bad()) {
        return error(std::string("An error occurred while writing to JSON: ") +
                         _e.what(),
                     ErrorKind::io_error);
      }
      return error(std::string("An error occurred while writing to JSON: ") +
                   _e.what());
    }
    return Nothing{};
  }

  /// Flushes the underlying output stream.
  Result<Nothing> flush() {
    if (!stream_) {
      return error("StreamWriter has no output stream.", ErrorKind::io_error);
    }
    if (stream_->fail() || stream_->bad()) {
      return error("Output stream is not in a good state.",
                   ErrorKind::io_error);
    }
    if (stream_->eof()) {
      stream_->clear(stream_->rdstate() & ~std::ios_base::eofbit);
    }
    try {
      stream_->flush();
      if (stream_->fail() || stream_->bad()) {
        return error("I/O error while flushing JSON stream.",
                     ErrorKind::io_error);
      }
    } catch (const std::exception& _e) {
      return error(std::string("I/O error while flushing JSON stream: ") +
                       _e.what(),
                   ErrorKind::io_error);
    }
    return Nothing{};
  }

  /// Finishes the stream (emits `[]` or `]` for `json_array`). Idempotent;
  /// if finishing failed, subsequent retries return the error.
  Result<Nothing> finish() {
    if (finished_) {
      if (finish_error_.has_value()) {
        return error(*finish_error_);
      }
      return Nothing{};
    }
    finished_ = true;
    if (!stream_) {
      finish_error_ =
          Error("StreamWriter has no output stream.", ErrorKind::io_error);
      return error(*finish_error_);
    }
    if (stream_->fail() || stream_->bad()) {
      finish_error_ = Error("I/O error while finishing JSON stream.",
                            ErrorKind::io_error);
      return error(*finish_error_);
    }
    if (stream_->eof()) {
      stream_->clear(stream_->rdstate() & ~std::ios_base::eofbit);
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
        finish_error_ = Error("I/O error while finishing JSON stream.",
                              ErrorKind::io_error);
        return error(*finish_error_);
      }
    } catch (const std::exception& _e) {
      finish_error_ =
          Error(std::string("I/O error while finishing JSON stream: ") +
                    _e.what(),
                ErrorKind::io_error);
      return error(*finish_error_);
    }
    return Nothing{};
  }

 private:
  std::ostream* stream_;
  StreamFormat format_;
  yyjson_write_flag flag_;
  bool started_ = false;
  bool finished_ = false;
  std::optional<Error> finish_error_;
};

}  // namespace json

namespace internal::json {

using rfl::json::StreamFormat;

struct YYJsonDocDeleter {
  void operator()(yyjson_doc* _doc) const noexcept { yyjson_doc_free(_doc); }
};

using YYJsonDocPtr = std::unique_ptr<yyjson_doc, YYJsonDocDeleter>;

/// Non-template framing, buffering, and incremental JSON value scanning engine
/// shared by all StreamReader<T, Ps...> instantiations.
class RFL_API StreamReaderEngine {
 public:
  StreamReaderEngine(std::istream& _stream, StreamFormat _format,
                     yyjson_read_flag _flag, size_t _chunk_size,
                     size_t _max_record_bytes);

  StreamReaderEngine(std::string_view _input, StreamFormat _format,
                     yyjson_read_flag _flag, size_t _max_record_bytes);

  StreamReaderEngine(const StreamReaderEngine&) = delete;
  StreamReaderEngine& operator=(const StreamReaderEngine&) = delete;
  StreamReaderEngine(StreamReaderEngine&&) = default;
  StreamReaderEngine& operator=(StreamReaderEngine&&) = default;
  ~StreamReaderEngine() = default;

  std::optional<Result<YYJsonDocPtr>> next_doc();

 private:
  enum class ArrayState { before_open, expect_first, expect_sep_end, closed };
  enum class SkipResult { found, eof, io_error, unclosed_comment };
  enum class CommentMode { none, slash, line, block, block_star };
  enum class ScanOutcome {
    complete,
    hard_boundary,
    syntax_break,
    too_large,
    io_error,
    eof_truncated,
  };

  struct ScanResult {
    ScanOutcome outcome;
    size_t slice_len = 0;
  };

  const char* data_ptr() const noexcept;
  size_t data_size() const noexcept;
  size_t avail() const noexcept;

  std::optional<Result<YYJsonDocPtr>> fail(const std::string& _msg,
                                           ErrorKind _kind = ErrorKind::other);
  std::optional<Result<YYJsonDocPtr>> eof_or_io_error(const char* _eof_msg);

  void compact_buffer();
  size_t refill_raw(size_t _requested, bool _line_mode, bool _stop_at_rs);
  size_t refill_for_record(size_t _requested, bool _line_mode,
                           bool _stop_at_rs);
  bool skip_bom_if_needed();
  SkipResult skip_ws(bool _skip_rs);
  void advance_past_newline();
  void resync_to_rs();

  ScanResult scan_value(bool _line_mode, bool _is_seq);

  std::optional<Result<YYJsonDocPtr>> parse_delimited_value(bool _is_seq);
  std::optional<Result<YYJsonDocPtr>> parse_array_value();
  std::optional<Result<YYJsonDocPtr>> next_delimited(bool _skip_rs);
  std::optional<Result<YYJsonDocPtr>> finish_array();
  std::optional<Result<YYJsonDocPtr>> next_array_element();

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
  bool bom_checked_ = false;
  bool done_ = false;
  bool invalid_flag_ = false;
  ArrayState array_state_ = ArrayState::before_open;
};

}  // namespace internal::json

namespace json {

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
      : engine_(_stream, _format, _flag, _chunk_size, _max_record_bytes) {}

  explicit StreamReader(
      const std::string_view _input,
      const StreamFormat _format = StreamFormat::ndjson,
      const yyjson_read_flag _flag = 0,
      const size_t _max_record_bytes = kDefaultMaxRecordBytes)
      : engine_(_input, _format, _flag, _max_record_bytes) {}

  StreamReader(const StreamReader&) = delete;
  StreamReader& operator=(const StreamReader&) = delete;
  StreamReader(StreamReader&&) = default;
  StreamReader& operator=(StreamReader&&) = default;

  /// Reads the next record, or returns `std::nullopt` at clean EOF.
  std::optional<ValueType> next() {
    auto doc_opt = engine_.next_doc();
    if (!doc_opt.has_value()) {
      return std::nullopt;
    }
    if (!doc_opt->has_value()) {
      return error(std::move(doc_opt->error()));
    }
    yyjson_val* root = yyjson_doc_get_root(doc_opt->value().get());
    const auto r = Reader();
    return Parser<T, Processors<Ps...>>::read(r, Reader::InputVarType(root));
  }

  Iterator begin() {
    current_ = next();
    return current_.has_value() ? Iterator(this) : Iterator();
  }

  std::default_sentinel_t end() const noexcept { return std::default_sentinel; }

 private:
  rfl::internal::json::StreamReaderEngine engine_;
  std::optional<ValueType> current_;
};

}  // namespace json
}  // namespace rfl

#endif
