#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

namespace bench_48 {

enum class LogLevel { debug, info, warn, error };
enum class Compression { none, zstd, gzip };
enum class HashMode { content, mtime, direct };

// 48-field realistic config struct wrapped in
// rfl::Description<StringLiteral, rfl::DefaultVal<T>>.
struct ConfigData {
  rfl::Description<"Absolute path to the root directory where compiled object cache entries and metadata files are stored on disk", rfl::DefaultVal<std::string>> cache_dir = "/var/cache/app";
  rfl::Description<"Absolute path to the temporary scratch directory used for staging atomic writes before renaming into the cache", rfl::DefaultVal<std::string>> temporary_dir = "/tmp/app";
  rfl::Description<"Maximum total disk space allowed for cached artifacts, parsed with K/M/G binary suffixes or percentage limits", rfl::DefaultVal<std::string>> max_size = "5G";
  rfl::Description<"Maximum number of individual files permitted inside the local cache hierarchy before LRU eviction triggers", rfl::DefaultVal<int>> max_files = 0;
  rfl::Description<"Enable Zstandard compression for cached object files and manifest payloads to trade CPU for reduced disk usage", rfl::DefaultVal<bool>> compression = true;
  rfl::Description<"Compression level passed to the Zstandard encoder; negative values enable ultra-fast mode while higher values shrink output", rfl::DefaultVal<int>> compression_level = 0;
  rfl::Description<"Selected compression codec algorithm used when writing new cache entries to local or remote storage backends", rfl::DefaultVal<Compression>> compression_codec = Compression::zstd;
  rfl::Description<"Diagnostic verbosity threshold controlling which internal trace, warning, and error records are emitted during execution", rfl::DefaultVal<LogLevel>> log_level = LogLevel::warn;
  rfl::Description<"Path to an append-only log file where per-invocation cache hit/miss diagnostics and timing breakdowns are written", rfl::DefaultVal<std::string>> log_file = "";
  rfl::Description<"Base directory prefix stripped from absolute paths inside debug info and dependency files to improve cross-checkout hits", rfl::DefaultVal<std::string>> base_dir = "";
  rfl::Description<"Enable direct mode hashing of source and included header files directly without invoking the C/C++ preprocessor first", rfl::DefaultVal<bool>> direct_mode = true;
  rfl::Description<"Strategy used to detect whether included headers have changed since the direct-mode manifest entry was recorded", rfl::DefaultVal<HashMode>> hash_mode = HashMode::content;
  rfl::Description<"Include the current working directory in the compilation hash so relative debug info paths remain reproducible", rfl::DefaultVal<bool>> hash_dir = true;
  rfl::Description<"Disable the cache completely and pass all invocations straight through to the underlying compiler executable unchanged", rfl::DefaultVal<bool>> disable = false;
  rfl::Description<"Run the underlying compiler normally and write new results into the cache without reading existing cache entries", rfl::DefaultVal<bool>> recache = false;
  rfl::Description<"Treat the local cache as read-only so hits are served normally while misses skip populating new cache artifacts", rfl::DefaultVal<bool>> read_only = false;
  rfl::Description<"Treat only the direct-mode manifest lookup as read-only while allowing preprocessor-mode hits to update storage", rfl::DefaultVal<bool>> read_only_direct = false;
  rfl::Description<"Allow the cache to serve hits even when the local cache directory is not writable by the current user", rfl::DefaultVal<bool>> read_only_fallback = false;
  rfl::Description<"Keep all cache statistics counters updated on every hit, miss, and error invocation for observability dashboards", rfl::DefaultVal<bool>> stats = true;
  rfl::Description<"Path to a structured JSON statistics log file appended on every invocation for CI build-cache telemetry pipelines", rfl::DefaultVal<std::string>> stats_log = "";
  rfl::Description<"Allow compiler invocations to be served by a shared read-only secondary cache before falling back to local compilation", rfl::DefaultVal<bool>> remote_only = false;
  rfl::Description<"List of remote storage backend URLs (HTTP, Redis, file share) queried in priority order for distributed cache lookups", rfl::DefaultVal<std::vector<std::string>>> remote_storage = {};
  rfl::Description<"List of compiler flags or patterns that should be ignored when computing the compilation cache key hash", rfl::DefaultVal<std::vector<std::string>>> ignore_options = {};
  rfl::Description<"List of header file path patterns ignored by direct mode when recording manifest dependencies for fast lookups", rfl::DefaultVal<std::vector<std::string>>> ignore_headers_in_manifest = {};
  rfl::Description<"Compiler executable identity check method (mtime, content, string, or none) used to invalidate stale cache entries", rfl::DefaultVal<std::string>> compiler_check = "mtime";
  rfl::Description<"Explicit compiler binary path overriding PATH discovery when running in masquerade or wrapper invocation mode", rfl::DefaultVal<std::string>> compiler = "";
  rfl::Description<"Compiler family override (gcc, clang, msvc, nvcc, auto) when binary filename heuristics cannot determine the frontend", rfl::DefaultVal<std::string>> compiler_type = "auto";
  rfl::Description<"Prefix command prepended before invoking the real compiler executable, such as distcc or icecc wrappers", rfl::DefaultVal<std::string>> prefix_command = "";
  rfl::Description<"Prefix command prepended specifically when running the standalone C/C++ preprocessor pass on cache misses", rfl::DefaultVal<std::string>> prefix_command_cpp = "";
  rfl::Description<"File mode creation mask applied when creating cache directories and shared group-writable cache artifact files", rfl::DefaultVal<std::string>> umask = "";
  rfl::Description<"Attempt to use OS-level hard links instead of copying cached object files into the build output directory", rfl::DefaultVal<bool>> hard_link = false;
  rfl::Description<"Attempt to use copy-on-write filesystem clones (reflink/clonefile) when copying cached artifacts into the build tree", rfl::DefaultVal<bool>> file_clone = false;
  rfl::Description<"Preserve relative inode caching in memory across calls to avoid repeated stat and content hashing of system headers", rfl::DefaultVal<bool>> inode_cache = true;
  rfl::Description<"Enable depend mode so the compiler writes dependency files in a single pass rather than running a separate preprocessor", rfl::DefaultVal<bool>> depend_mode = false;
  rfl::Description<"Enable debug-info generation support when compiler flags request DWARF split-debug (.dwo) or PDB sidecar files", rfl::DefaultVal<bool>> debug = true;
  rfl::Description<"Directory where per-invocation debug trace bundles (input, preprocessed source, manifest) are dumped for troubleshooting", rfl::DefaultVal<std::string>> debug_dir = "";
  rfl::Description<"Debug verbosity level controlling which artifacts (metadata, preprocessed output, object) are captured in debug_dir", rfl::DefaultVal<int>> debug_level = 2;
  rfl::Description<"Minimum number of seconds an output file must be older than current time before mtime-based direct caching trusts it", rfl::DefaultVal<int>> sloppiness_time_window = 1;
  rfl::Description<"Maximum number of manifest entries retained per source file hash before oldest entries are rotated out", rfl::DefaultVal<int>> limit_multiple = 80;
  rfl::Description<"Namespace string mixed into every cache key hash to isolate otherwise identical builds across different projects", rfl::DefaultVal<std::string>> namespace_name = "";
  rfl::Description<"Extra files whose contents are hashed into every compilation key to track external codegen or toolchain configuration", rfl::DefaultVal<std::string>> extra_files_to_hash = "";
  rfl::Description<"Keep comments inside preprocessed output when hashing so lint annotations or pragmas inside comments affect cache hits", rfl::DefaultVal<bool>> keep_comments_cpp = false;
  rfl::Description<"Rewrite absolute paths in compiler diagnostic messages relative to base_dir for cleaner IDE problem-matcher links", rfl::DefaultVal<bool>> absolute_paths_in_stderr = false;
  rfl::Description<"Reshare cached results across builds using different relative include paths by normalizing -I and -isystem paths", rfl::DefaultVal<bool>> reshare = false;
  rfl::Description<"Number of automatic cache-cleanup buckets swept per cache write when storage exceeds configured max_size or max_files", rfl::DefaultVal<int>> cleanup_buckets = 16;
  rfl::Description<"Timeout in milliseconds for remote storage network requests before falling back gracefully to local compilation", rfl::DefaultVal<int>> remote_timeout_ms = 5000;
  rfl::Description<"Maximum number of retry attempts on transient 5xx or connection-reset errors from remote HTTP or Redis cache servers", rfl::DefaultVal<int>> remote_retries = 2;
  rfl::Description<"Optional custom user-agent or tenant token header sent with outbound remote cache storage HTTP requests", rfl::DefaultVal<std::optional<std::string>>> remote_auth_header = std::nullopt;
};

}  // namespace bench_48

rfl::Result<bench_48::ConfigData> parse_config(const std::string& _json) {
  return rfl::json::read<bench_48::ConfigData>(_json);
}

std::string serialize_config(const bench_48::ConfigData& _cfg) {
  return rfl::json::write(_cfg);
}

std::string schema_config() {
  return rfl::json::to_schema<bench_48::ConfigData>();
}
