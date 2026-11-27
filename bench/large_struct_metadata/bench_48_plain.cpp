#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

namespace bench_48 {

enum class LogLevel { debug, info, warn, error };
enum class Compression { none, zstd, gzip };
enum class HashMode { content, mtime, direct };

// Identical 48-field ConfigData WITHOUT descriptions (plain fields +
// rfl::DefaultIfMissing) as a control baseline.
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

rfl::Result<bench_48::ConfigData> parse_config(const std::string& _json) {
  return rfl::json::read<bench_48::ConfigData, rfl::DefaultIfMissing>(_json);
}

std::string serialize_config(const bench_48::ConfigData& _cfg) {
  return rfl::json::write(_cfg);
}

std::string schema_config() {
  return rfl::json::to_schema<bench_48::ConfigData, rfl::DefaultIfMissing>();
}
