/*

MIT License

Copyright (c) 2023-2024 Code17 GmbH

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "rfl/json/stream.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <streambuf>

namespace rfl::json::detail {

namespace {

bool is_json_ws(const char _c, const bool _allow_ext_ws) noexcept {
  if (_c == ' ' || _c == '\t' || _c == '\n' || _c == '\r') {
    return true;
  }
  return _allow_ext_ws && (_c == '\v' || _c == '\f');
}

bool is_primitive_char(const char _c) noexcept {
  return (_c >= '0' && _c <= '9') || (_c >= 'a' && _c <= 'z') ||
         (_c >= 'A' && _c <= 'Z') || _c == '+' || _c == '-' || _c == '.';
}

size_t safe_double(const size_t _n) noexcept {
  constexpr size_t kMax = std::numeric_limits<size_t>::max();
  return _n > kMax / 2 ? kMax : _n * 2;
}

size_t safe_inc(const size_t _n) noexcept {
  constexpr size_t kMax = std::numeric_limits<size_t>::max();
  return _n < kMax ? _n + 1 : kMax;
}

}  // namespace

StreamReaderEngine::StreamReaderEngine(std::istream& _stream,
                                       const StreamFormat _format,
                                       const yyjson_read_flag _flag,
                                       const size_t _chunk_size,
                                       const size_t _max_record_bytes)
    : stream_(&_stream),
      format_(_format),
      flag_(_flag),
      chunk_size_(_chunk_size > 0 ? _chunk_size : 1),
      max_record_bytes_(_max_record_bytes),
      invalid_flag_((_flag & YYJSON_READ_INSITU) != 0) {}

StreamReaderEngine::StreamReaderEngine(const std::string_view _input,
                                       const StreamFormat _format,
                                       const yyjson_read_flag _flag,
                                       const size_t _max_record_bytes)
    : stream_(nullptr),
      format_(_format),
      flag_(_flag),
      chunk_size_(0),
      max_record_bytes_(_max_record_bytes),
      input_(_input),
      eof_(true),
      invalid_flag_((_flag & YYJSON_READ_INSITU) != 0) {}

std::optional<Result<YYJsonDocPtr>> StreamReaderEngine::next_doc() {
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

const char* StreamReaderEngine::data_ptr() const noexcept {
  return stream_ ? buf_.data() : input_.data();
}

size_t StreamReaderEngine::data_size() const noexcept {
  return stream_ ? buf_.size() : input_.size();
}

size_t StreamReaderEngine::avail() const noexcept { return data_size() - pos_; }

std::optional<Result<YYJsonDocPtr>> StreamReaderEngine::fail(
    const std::string& _msg, const ErrorKind _kind) {
  done_ = true;
  return error(_msg, _kind);
}

std::optional<Result<YYJsonDocPtr>> StreamReaderEngine::eof_or_io_error(
    const char* _eof_msg) {
  done_ = true;
  if (io_error_) {
    return error("I/O error while reading JSON stream.", ErrorKind::io_error);
  }
  return error(_eof_msg, ErrorKind::syntax_error);
}

void StreamReaderEngine::compact_buffer() {
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

size_t StreamReaderEngine::refill_raw(const size_t _requested,
                                      const bool _line_mode,
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

size_t StreamReaderEngine::refill_for_record(const size_t _requested,
                                             const bool _line_mode,
                                             const bool _stop_at_rs) {
  size_t allowed = _requested;
  if (max_record_bytes_ > 0) {
    const size_t cap = safe_inc(max_record_bytes_);
    const size_t cur_avail = avail();
    if (cur_avail >= cap) {
      return 0;
    }
    allowed = std::min(allowed, cap - cur_avail);
  }
  return refill_raw(allowed, _line_mode, _stop_at_rs);
}

bool StreamReaderEngine::skip_bom_if_needed() {
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

StreamReaderEngine::SkipResult StreamReaderEngine::skip_ws(
    const bool _skip_rs) {
  if (!skip_bom_if_needed()) {
    return SkipResult::io_error;
  }
  const bool allow_comments = (flag_ & YYJSON_READ_ALLOW_COMMENTS) != 0;
  const bool allow_ext_ws = (flag_ & YYJSON_READ_ALLOW_EXT_WHITESPACE) != 0;
  const bool line_mode = (format_ != StreamFormat::json_array);
  const bool stop_at_rs = (format_ == StreamFormat::json_seq);

  while (true) {
    while (pos_ < data_size()) {
      const char c = data_ptr()[pos_];
      if (is_json_ws(c, allow_ext_ws) || (_skip_rs && c == '\x1e')) {
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

void StreamReaderEngine::advance_past_newline() {
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

void StreamReaderEngine::resync_to_rs() {
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
    if (const char* rs = static_cast<const char*>(std::memchr(p, '\x1e', a))) {
      pos_ += static_cast<size_t>(rs - p);
      compact_buffer();
      return;
    }
    pos_ = data_size();
    compact_buffer();
  }
}

StreamReaderEngine::ScanResult StreamReaderEngine::scan_value(
    const bool _line_mode, const bool _is_seq, const bool _check_first_nl,
    std::string* _first_nl_err) {
  const bool allow_comments = (flag_ & YYJSON_READ_ALLOW_COMMENTS) != 0;
  const bool allow_single_quote =
      (flag_ & YYJSON_READ_ALLOW_SINGLE_QUOTED_STR) != 0;
  const bool allow_ext_escape = (flag_ & YYJSON_READ_ALLOW_EXT_ESCAPE) != 0;
  const char c0 = data_ptr()[pos_];

  // Case 1: JSON object or array container.
  if (c0 == '{' || c0 == '[') {
    size_t scan_offset = 0;
    size_t depth = 0;
    char quote = 0;
    bool escape = false;
    CommentMode comment = CommentMode::none;
    bool checked_first_nl = !_check_first_nl;
    size_t refill_step = chunk_size_;

    while (true) {
      const char* ptr = data_ptr() + pos_;
      const size_t cur_avail = avail();
      const size_t scan_limit = max_record_bytes_ > 0
                                    ? std::min(cur_avail, max_record_bytes_)
                                    : cur_avail;

      while (scan_offset < scan_limit) {
        const char c = ptr[scan_offset];

        if (_is_seq && c == '\x1e') {
          return ScanResult{ScanOutcome::hard_boundary, scan_offset};
        }

        if (comment != CommentMode::none) {
          if (comment == CommentMode::slash) {
            if (c == '/') {
              comment = CommentMode::line;
              ++scan_offset;
              continue;
            }
            if (c == '*') {
              comment = CommentMode::block;
              ++scan_offset;
              continue;
            }
            comment = CommentMode::none;
            // Not a comment; fall through to process `c` normally.
          } else if (comment == CommentMode::line) {
            ++scan_offset;
            if (c == '\n' || c == '\r') {
              comment = CommentMode::none;
              if (c == '\n' && !checked_first_nl) {
                checked_first_nl = true;
                yyjson_read_err err{};
                yyjson_doc* probe = yyjson_read_opts(
                    const_cast<char*>(ptr), scan_offset,
                    flag_ | YYJSON_READ_STOP_WHEN_DONE, nullptr, &err);
                if (probe) {
                  yyjson_doc_free(probe);
                } else if (err.code != YYJSON_READ_ERROR_UNEXPECTED_END &&
                           err.code != YYJSON_READ_ERROR_EMPTY_CONTENT) {
                  if (_first_nl_err) {
                    *_first_nl_err = err.msg ? err.msg : "unknown error";
                  }
                  return ScanResult{ScanOutcome::syntax_break, scan_offset};
                }
              }
            }
            continue;
          } else if (comment == CommentMode::block) {
            if (c == '*') {
              comment = CommentMode::block_star;
            }
            ++scan_offset;
            continue;
          } else {  // CommentMode::block_star
            if (c == '/') {
              comment = CommentMode::none;
            } else if (c != '*') {
              comment = CommentMode::block;
            }
            ++scan_offset;
            continue;
          }
        }

        if (quote != 0) {
          if (escape) {
            escape = false;
            ++scan_offset;
            continue;
          }
          if (c == '\\') {
            escape = true;
            ++scan_offset;
            continue;
          }
          if (c == quote) {
            quote = 0;
            ++scan_offset;
            continue;
          }
          if (!allow_ext_escape && (c == '\n' || c == '\r')) {
            ++scan_offset;
            return ScanResult{ScanOutcome::syntax_break, scan_offset};
          }
          ++scan_offset;
          continue;
        }

        if (c == '"' || (allow_single_quote && c == '\'')) {
          quote = c;
          ++scan_offset;
          continue;
        }

        if (allow_comments && c == '/') {
          comment = CommentMode::slash;
          ++scan_offset;
          continue;
        }

        if (c == '{' || c == '[') {
          ++depth;
          ++scan_offset;
          continue;
        }

        if (c == '}' || c == ']') {
          if (depth > 0) {
            --depth;
          }
          ++scan_offset;
          if (depth == 0) {
            return ScanResult{ScanOutcome::complete, scan_offset};
          }
          continue;
        }

        ++scan_offset;
        if (c == '\n' && !checked_first_nl) {
          checked_first_nl = true;
          yyjson_read_err err{};
          yyjson_doc* probe = yyjson_read_opts(
              const_cast<char*>(ptr), scan_offset,
              flag_ | YYJSON_READ_STOP_WHEN_DONE, nullptr, &err);
          if (probe) {
            yyjson_doc_free(probe);
          } else if (err.code != YYJSON_READ_ERROR_UNEXPECTED_END &&
                     err.code != YYJSON_READ_ERROR_EMPTY_CONTENT) {
            if (_first_nl_err) {
              *_first_nl_err = err.msg ? err.msg : "unknown error";
            }
            return ScanResult{ScanOutcome::syntax_break, scan_offset};
          }
        }
      }

      if (max_record_bytes_ > 0 && scan_offset >= max_record_bytes_) {
        if (avail() <= max_record_bytes_ && !eof_) {
          (void)refill_for_record(refill_step, _line_mode, _is_seq);
          if (io_error_) {
            return ScanResult{ScanOutcome::io_error, scan_offset};
          }
        }
        if (avail() > max_record_bytes_) {
          if (_is_seq && data_ptr()[pos_ + max_record_bytes_] == '\x1e') {
            return ScanResult{ScanOutcome::hard_boundary, scan_offset};
          }
          return ScanResult{ScanOutcome::too_large, scan_offset};
        }
        return ScanResult{ScanOutcome::eof_truncated, scan_offset};
      }

      if (eof_) {
        return ScanResult{ScanOutcome::eof_truncated, scan_offset};
      }

      const size_t got = refill_for_record(refill_step, _line_mode, _is_seq);
      if (io_error_) {
        return ScanResult{ScanOutcome::io_error, scan_offset};
      }
      if (got == 0) {
        return ScanResult{ScanOutcome::eof_truncated, scan_offset};
      }
      refill_step = safe_double(refill_step);
    }
  }

  // Case 2: JSON string literal.
  if (c0 == '"' || (allow_single_quote && c0 == '\'')) {
    const char quote = c0;
    size_t scan_offset = 1;
    bool escape = false;
    size_t refill_step = chunk_size_;

    while (true) {
      const char* ptr = data_ptr() + pos_;
      const size_t cur_avail = avail();
      const size_t scan_limit = max_record_bytes_ > 0
                                    ? std::min(cur_avail, max_record_bytes_)
                                    : cur_avail;

      while (scan_offset < scan_limit) {
        const char c = ptr[scan_offset];
        if (_is_seq && c == '\x1e') {
          return ScanResult{ScanOutcome::hard_boundary, scan_offset};
        }
        if (escape) {
          escape = false;
          ++scan_offset;
          continue;
        }
        if (c == '\\') {
          escape = true;
          ++scan_offset;
          continue;
        }
        if (c == quote) {
          ++scan_offset;
          return ScanResult{ScanOutcome::complete, scan_offset};
        }
        if (!allow_ext_escape && (c == '\n' || c == '\r')) {
          ++scan_offset;
          return ScanResult{ScanOutcome::syntax_break, scan_offset};
        }
        ++scan_offset;
      }

      if (max_record_bytes_ > 0 && scan_offset >= max_record_bytes_) {
        if (avail() <= max_record_bytes_ && !eof_) {
          (void)refill_for_record(refill_step, _line_mode, _is_seq);
          if (io_error_) {
            return ScanResult{ScanOutcome::io_error, scan_offset};
          }
        }
        if (avail() > max_record_bytes_) {
          if (_is_seq && data_ptr()[pos_ + max_record_bytes_] == '\x1e') {
            return ScanResult{ScanOutcome::hard_boundary, scan_offset};
          }
          return ScanResult{ScanOutcome::too_large, scan_offset};
        }
        return ScanResult{ScanOutcome::eof_truncated, scan_offset};
      }

      if (eof_) {
        return ScanResult{ScanOutcome::eof_truncated, scan_offset};
      }

      const size_t got = refill_for_record(refill_step, _line_mode, _is_seq);
      if (io_error_) {
        return ScanResult{ScanOutcome::io_error, scan_offset};
      }
      if (got == 0) {
        return ScanResult{ScanOutcome::eof_truncated, scan_offset};
      }
      refill_step = safe_double(refill_step);
    }
  }

  // Case 3: JSON primitive (number, boolean, null, NaN/Infinity) or bad token.
  if (!is_primitive_char(c0)) {
    return ScanResult{ScanOutcome::syntax_break, 1};
  }

  size_t scan_offset = 1;
  size_t refill_step = chunk_size_;
  while (true) {
    const char* ptr = data_ptr() + pos_;
    const size_t cur_avail = avail();
    const size_t scan_limit = max_record_bytes_ > 0
                                  ? std::min(cur_avail, max_record_bytes_)
                                  : cur_avail;

    while (scan_offset < scan_limit) {
      if (!is_primitive_char(ptr[scan_offset])) {
        return ScanResult{ScanOutcome::complete, scan_offset};
      }
      ++scan_offset;
    }

    if (max_record_bytes_ > 0 && scan_offset >= max_record_bytes_) {
      if (avail() <= max_record_bytes_ && !eof_) {
        (void)refill_for_record(refill_step, _line_mode, _is_seq);
        if (io_error_) {
          return ScanResult{ScanOutcome::io_error, scan_offset};
        }
      }
      if (avail() > max_record_bytes_ &&
          is_primitive_char(data_ptr()[pos_ + max_record_bytes_])) {
        return ScanResult{ScanOutcome::too_large, scan_offset};
      }
      return ScanResult{ScanOutcome::complete, scan_offset};
    }

    if (eof_) {
      return ScanResult{ScanOutcome::complete, scan_offset};
    }

    const size_t got = refill_for_record(refill_step, _line_mode, _is_seq);
    if (io_error_) {
      return ScanResult{ScanOutcome::io_error, scan_offset};
    }
    if (got == 0) {
      return ScanResult{ScanOutcome::complete, scan_offset};
    }
    refill_step = safe_double(refill_step);
  }
}

std::optional<Result<YYJsonDocPtr>> StreamReaderEngine::parse_delimited_value(
    const bool _is_seq) {
  if (avail() == 0) {
    done_ = true;
    return std::nullopt;
  }

  std::string first_nl_err;
  const ScanResult scan = scan_value(true, _is_seq, !_is_seq, &first_nl_err);

  if (scan.outcome == ScanOutcome::too_large) {
    return fail("JSON record exceeds maximum allowed size.",
                ErrorKind::syntax_error);
  }
  if (scan.outcome == ScanOutcome::io_error) {
    return fail("I/O error while reading JSON stream.", ErrorKind::io_error);
  }

  if (scan.outcome == ScanOutcome::syntax_break && !first_nl_err.empty()) {
    const std::string err_msg = "Could not parse document: " + first_nl_err;
    if (_is_seq) {
      resync_to_rs();
    } else {
      advance_past_newline();
    }
    if (io_error_) {
      return fail("I/O error while reading JSON stream.", ErrorKind::io_error);
    }
    return error(err_msg, ErrorKind::syntax_error);
  }

  if (scan.slice_len == 0) {
    done_ = true;
    return std::nullopt;
  }

  // Always compute ptr fresh after scan_value (which may refill/compact buf_).
  const char* ptr = data_ptr() + pos_;
  yyjson_read_err err{};
  YYJsonDocPtr doc(yyjson_read_opts(const_cast<char*>(ptr), scan.slice_len,
                                    flag_, nullptr, &err));
  if (doc) {
    const size_t read_size = yyjson_doc_get_read_size(doc.get());
    if (max_record_bytes_ > 0 && read_size > max_record_bytes_) {
      return fail("JSON record exceeds maximum allowed size.",
                  ErrorKind::syntax_error);
    }
    pos_ += read_size;
    compact_buffer();
    return Result<YYJsonDocPtr>(std::move(doc));
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
    return fail("I/O error while reading JSON stream.", ErrorKind::io_error);
  }
  return error(err_msg, ErrorKind::syntax_error);
}

std::optional<Result<YYJsonDocPtr>> StreamReaderEngine::parse_array_value() {
  if (avail() == 0) {
    return eof_or_io_error("Could not parse document: unexpected end of data");
  }

  const ScanResult scan = scan_value(false, false, false, nullptr);

  if (scan.outcome == ScanOutcome::too_large) {
    return fail("JSON record exceeds maximum allowed size.",
                ErrorKind::syntax_error);
  }
  if (scan.outcome == ScanOutcome::io_error) {
    return fail("I/O error while reading JSON stream.", ErrorKind::io_error);
  }
  if (scan.slice_len == 0) {
    return eof_or_io_error("Could not parse document: unexpected end of data");
  }

  // Always compute ptr fresh after scan_value (which may refill/compact buf_).
  const char* ptr = data_ptr() + pos_;
  yyjson_read_err err{};
  YYJsonDocPtr doc(yyjson_read_opts(const_cast<char*>(ptr), scan.slice_len,
                                    flag_, nullptr, &err));
  if (doc) {
    const size_t read_size = yyjson_doc_get_read_size(doc.get());
    if (max_record_bytes_ > 0 && read_size > max_record_bytes_) {
      return fail("JSON record exceeds maximum allowed size.",
                  ErrorKind::syntax_error);
    }
    pos_ += read_size;
    compact_buffer();
    return Result<YYJsonDocPtr>(std::move(doc));
  }

  done_ = true;
  return error("Could not parse document: " +
                   std::string(err.msg ? err.msg : "unknown error"),
               ErrorKind::syntax_error);
}

std::optional<Result<YYJsonDocPtr>> StreamReaderEngine::next_delimited(
    const bool _skip_rs) {
  const auto skipped = skip_ws(_skip_rs);
  if (skipped == SkipResult::eof) {
    done_ = true;
    return std::nullopt;
  }
  if (skipped == SkipResult::io_error) {
    return fail("I/O error while reading JSON stream.", ErrorKind::io_error);
  }
  if (skipped == SkipResult::unclosed_comment) {
    return fail("Unclosed multi-line comment in JSON stream.",
                ErrorKind::syntax_error);
  }
  return parse_delimited_value(_skip_rs);
}

std::optional<Result<YYJsonDocPtr>> StreamReaderEngine::finish_array() {
  ++pos_;  // consume ']'
  array_state_ = ArrayState::closed;
  done_ = true;
  const auto skipped = skip_ws(false);
  if (skipped == SkipResult::found) {
    return error(
        "Unexpected trailing content after closing ']' of JSON array.",
        ErrorKind::syntax_error);
  }
  if (skipped == SkipResult::io_error) {
    return error("I/O error while reading JSON stream.", ErrorKind::io_error);
  }
  if (skipped == SkipResult::unclosed_comment) {
    return error("Unclosed multi-line comment in JSON stream.",
                 ErrorKind::syntax_error);
  }
  return std::nullopt;
}

std::optional<Result<YYJsonDocPtr>> StreamReaderEngine::next_array_element() {
  auto handle_skip_error =
      [&](const SkipResult _sr,
          const char* _eof_msg) -> std::optional<Result<YYJsonDocPtr>> {
    if (_sr == SkipResult::unclosed_comment) {
      return fail("Unclosed multi-line comment in JSON stream.",
                  ErrorKind::syntax_error);
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
      return fail("Expected '[' at start of JSON array.",
                  ErrorKind::invalid_type);
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
      return fail("Expected ',' or ']' in JSON array.",
                  ErrorKind::syntax_error);
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
      return fail("Trailing comma is not allowed in JSON array.",
                  ErrorKind::syntax_error);
    }
    return parse_array_value();
  }

  done_ = true;
  return std::nullopt;
}

}  // namespace rfl::json::detail
