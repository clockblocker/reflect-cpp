#include <array>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

namespace bench_48 {

enum class LogLevel { debug, info, warn, error };
enum class Compression { none, zstd, gzip };
enum class HashMode { content, mtime, direct };

// Same 48-field ConfigData using zero-bloat out-of-band rfl::Metadata<T> +
// rfl::DefaultIfMissing (no StringLiteral NTTPs in field types).
struct ConfigData {
  std::string cache_dir = "/var/cache/app";
  std::string temporary_dir = "/tmp/app";
  std::string max_size = "5G";
  int max_files = 0;
  bool compression = true;
  int compression_level = 0;
  Compression compression_codec = Compression::zstd;
  LogLevel log_level = LogLevel::warn;
  std::string log_file = "";
  std::string base_dir = "";
  bool direct_mode = true;
  HashMode hash_mode = HashMode::content;
  bool hash_dir = true;
  bool disable = false;
  bool recache = false;
  bool read_only = false;
  bool read_only_direct = false;
  bool read_only_fallback = false;
  bool stats = true;
  std::string stats_log = "";
  bool remote_only = false;
  std::vector<std::string> remote_storage = {};
  std::vector<std::string> ignore_options = {};
  std::vector<std::string> ignore_headers_in_manifest = {};
  std::string compiler_check = "mtime";
  std::string compiler = "";
  std::string compiler_type = "auto";
  std::string prefix_command = "";
  std::string prefix_command_cpp = "";
  std::string umask = "";
  bool hard_link = false;
  bool file_clone = false;
  bool inode_cache = true;
  bool depend_mode = false;
  bool debug = true;
  std::string debug_dir = "";
  int debug_level = 2;
  int sloppiness_time_window = 1;
  int limit_multiple = 80;
  std::string namespace_name = "";
  std::string extra_files_to_hash = "";
  bool keep_comments_cpp = false;
  bool absolute_paths_in_stderr = false;
  bool reshare = false;
  int cleanup_buckets = 16;
  int remote_timeout_ms = 5000;
  int remote_retries = 2;
  std::optional<std::string> remote_auth_header = std::nullopt;
};

}  // namespace bench_48

template <>
struct rfl::Metadata<bench_48::ConfigData> {
  static constexpr std::array<rfl::FieldMeta, 48> fields = {{
      {"cache_dir", "Absolute path to the root directory where compiled object cache entries and metadata files are stored on disk"},
      {"temporary_dir", "Absolute path to the temporary scratch directory used for staging atomic writes before renaming into the cache"},
      {"max_size", "Maximum total disk space allowed for cached artifacts, parsed with K/M/G binary suffixes or percentage limits"},
      {"max_files", "Maximum number of individual files permitted inside the local cache hierarchy before LRU eviction triggers"},
      {"compression", "Enable Zstandard compression for cached object files and manifest payloads to trade CPU for reduced disk usage"},
      {"compression_level", "Compression level passed to the Zstandard encoder; negative values enable ultra-fast mode while higher values shrink output"},
      {"compression_codec", "Selected compression codec algorithm used when writing new cache entries to local or remote storage backends"},
      {"log_level", "Diagnostic verbosity threshold controlling which internal trace, warning, and error records are emitted during execution"},
      {"log_file", "Path to an append-only log file where per-invocation cache hit/miss diagnostics and timing breakdowns are written"},
      {"base_dir", "Base directory prefix stripped from absolute paths inside debug info and dependency files to improve cross-checkout hits"},
      {"direct_mode", "Enable direct mode hashing of source and included header files directly without invoking the C/C++ preprocessor first"},
      {"hash_mode", "Strategy used to detect whether included headers have changed since the direct-mode manifest entry was recorded"},
      {"hash_dir", "Include the current working directory in the compilation hash so relative debug info paths remain reproducible"},
      {"disable", "Disable the cache completely and pass all invocations straight through to the underlying compiler executable unchanged"},
      {"recache", "Run the underlying compiler normally and write new results into the cache without reading existing cache entries"},
      {"read_only", "Treat the local cache as read-only so hits are served normally while misses skip populating new cache artifacts"},
      {"read_only_direct", "Treat only the direct-mode manifest lookup as read-only while allowing preprocessor-mode hits to update storage"},
      {"read_only_fallback", "Allow the cache to serve hits even when the local cache directory is not writable by the current user"},
      {"stats", "Keep all cache statistics counters updated on every hit, miss, and error invocation for observability dashboards"},
      {"stats_log", "Path to a structured JSON statistics log file appended on every invocation for CI build-cache telemetry pipelines"},
      {"remote_only", "Allow compiler invocations to be served by a shared read-only secondary cache before falling back to local compilation"},
      {"remote_storage", "List of remote storage backend URLs (HTTP, Redis, file share) queried in priority order for distributed cache lookups"},
      {"ignore_options", "List of compiler flags or patterns that should be ignored when computing the compilation cache key hash"},
      {"ignore_headers_in_manifest", "List of header file path patterns ignored by direct mode when recording manifest dependencies for fast lookups"},
      {"compiler_check", "Compiler executable identity check method (mtime, content, string, or none) used to invalidate stale cache entries"},
      {"compiler", "Explicit compiler binary path overriding PATH discovery when running in masquerade or wrapper invocation mode"},
      {"compiler_type", "Compiler family override (gcc, clang, msvc, nvcc, auto) when binary filename heuristics cannot determine the frontend"},
      {"prefix_command", "Prefix command prepended before invoking the real compiler executable, such as distcc or icecc wrappers"},
      {"prefix_command_cpp", "Prefix command prepended specifically when running the standalone C/C++ preprocessor pass on cache misses"},
      {"umask", "File mode creation mask applied when creating cache directories and shared group-writable cache artifact files"},
      {"hard_link", "Attempt to use OS-level hard links instead of copying cached object files into the build output directory"},
      {"file_clone", "Attempt to use copy-on-write filesystem clones (reflink/clonefile) when copying cached artifacts into the build tree"},
      {"inode_cache", "Preserve relative inode caching in memory across calls to avoid repeated stat and content hashing of system headers"},
      {"depend_mode", "Enable depend mode so the compiler writes dependency files in a single pass rather than running a separate preprocessor"},
      {"debug", "Enable debug-info generation support when compiler flags request DWARF split-debug (.dwo) or PDB sidecar files"},
      {"debug_dir", "Directory where per-invocation debug trace bundles (input, preprocessed source, manifest) are dumped for troubleshooting"},
      {"debug_level", "Debug verbosity level controlling which artifacts (metadata, preprocessed output, object) are captured in debug_dir"},
      {"sloppiness_time_window", "Minimum number of seconds an output file must be older than current time before mtime-based direct caching trusts it"},
      {"limit_multiple", "Maximum number of manifest entries retained per source file hash before oldest entries are rotated out"},
      {"namespace_name", "Namespace string mixed into every cache key hash to isolate otherwise identical builds across different projects"},
      {"extra_files_to_hash", "Extra files whose contents are hashed into every compilation key to track external codegen or toolchain configuration"},
      {"keep_comments_cpp", "Keep comments inside preprocessed output when hashing so lint annotations or pragmas inside comments affect cache hits"},
      {"absolute_paths_in_stderr", "Rewrite absolute paths in compiler diagnostic messages relative to base_dir for cleaner IDE problem-matcher links"},
      {"reshare", "Reshare cached results across builds using different relative include paths by normalizing -I and -isystem paths"},
      {"cleanup_buckets", "Number of automatic cache-cleanup buckets swept per cache write when storage exceeds configured max_size or max_files"},
      {"remote_timeout_ms", "Timeout in milliseconds for remote storage network requests before falling back gracefully to local compilation"},
      {"remote_retries", "Maximum number of retry attempts on transient 5xx or connection-reset errors from remote HTTP or Redis cache servers"},
      {"remote_auth_header", "Optional custom user-agent or tenant token header sent with outbound remote cache storage HTTP requests"},
  }};
};

rfl::Result<bench_48::ConfigData> parse_config(const std::string& _json) {
  return rfl::json::read<bench_48::ConfigData, rfl::DefaultIfMissing>(_json);
}

std::string serialize_config(const bench_48::ConfigData& _cfg) {
  return rfl::json::write(_cfg);
}

std::string schema_config() {
  return rfl::json::to_schema<bench_48::ConfigData, rfl::DefaultIfMissing>();
}
