#ifndef RFL_CLI_SUGGEST_HPP_
#define RFL_CLI_SUGGEST_HPP_

#include <algorithm>
#include <cctype>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rfl::cli {

/// Computes optimal string alignment (restricted Damerau-Levenshtein) edit
/// distance between `_a` and `_b`, counting insertions, deletions,
/// substitutions, and adjacent transpositions as cost 1.
inline size_t damerau_levenshtein(const std::string_view _a,
                                  const std::string_view _b) {
  const size_t n = _a.size();
  const size_t m = _b.size();
  if (n == 0) {
    return m;
  }
  if (m == 0) {
    return n;
  }
  std::vector<size_t> prev2(m + 1, 0);
  std::vector<size_t> prev(m + 1, 0);
  std::vector<size_t> curr(m + 1, 0);
  for (size_t j = 0; j <= m; ++j) {
    prev[j] = j;
  }
  for (size_t i = 1; i <= n; ++i) {
    curr[0] = i;
    for (size_t j = 1; j <= m; ++j) {
      const size_t cost = (_a[i - 1] == _b[j - 1]) ? 0u : 1u;
      size_t best = std::min({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
      if (i > 1 && j > 1 && _a[i - 1] == _b[j - 2] && _a[i - 2] == _b[j - 1]) {
        best = std::min(best, prev2[j - 2] + 1);
      }
      curr[j] = best;
    }
    prev2.swap(prev);
    prev.swap(curr);
  }
  return prev[m];
}

/// Normalized similarity score in `[0.0, 1.0]` derived from
/// Damerau-Levenshtein distance. Case-insensitive for ASCII letters.
inline double similarity_ratio(const std::string_view _a,
                               const std::string_view _b) {
  if (_a.empty() && _b.empty()) {
    return 1.0;
  }
  const size_t max_len = std::max(_a.size(), _b.size());
  if (max_len == 0) {
    return 1.0;
  }
  auto to_lower = [](std::string_view _s) {
    std::string out(_s.size(), '\0');
    for (size_t i = 0; i < _s.size(); ++i) {
      out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(_s[i])));
    }
    return out;
  };
  const std::string la = to_lower(_a);
  const std::string lb = to_lower(_b);
  const size_t dist = damerau_levenshtein(la, lb);
  return 1.0 - (static_cast<double>(dist) / static_cast<double>(max_len));
}

/// Returns up to `_max_results` closest matches from `_candidates` whose
/// similarity to `_input` is at least `_min_similarity`, sorted best-first.
inline std::vector<std::string> suggest_closest(
    const std::string_view _input, const std::vector<std::string>& _candidates,
    const double _min_similarity = 0.6, const size_t _max_results = 1) {
  if (_input.empty() || _candidates.empty() || _max_results == 0) {
    return {};
  }
  struct Scored {
    double score;
    size_t dist;
    std::string candidate;
  };
  std::vector<Scored> scored;
  scored.reserve(_candidates.size());
  for (const auto& c : _candidates) {
    if (c.empty()) {
      continue;
    }
    const double ratio = similarity_ratio(_input, c);
    if (ratio >= _min_similarity) {
      scored.push_back(Scored{ratio, damerau_levenshtein(_input, c), c});
    }
  }
  std::sort(scored.begin(), scored.end(), [](const Scored& _x, const Scored& _y) {
    if (std::abs(_x.score - _y.score) > 1e-9) {
      return _x.score > _y.score;
    }
    if (_x.dist != _y.dist) {
      return _x.dist < _y.dist;
    }
    return _x.candidate < _y.candidate;
  });
  std::vector<std::string> out;
  for (const auto& s : scored) {
    if (std::find(out.begin(), out.end(), s.candidate) == out.end()) {
      out.push_back(s.candidate);
      if (out.size() >= _max_results) {
        break;
      }
    }
  }
  return out;
}

/// Convenience helper formatting `"Did you mean '<match>'?"` for the closest
/// candidate, or `std::nullopt` if no candidate meets `_min_similarity`.
inline std::optional<std::string> did_you_mean(
    const std::string_view _input, const std::vector<std::string>& _candidates,
    const double _min_similarity = 0.6) {
  const auto matches = suggest_closest(_input, _candidates, _min_similarity, 1);
  if (matches.empty()) {
    return std::nullopt;
  }
  return "Did you mean '" + matches.front() + "'?";
}

}  // namespace rfl::cli

#endif
