#ifndef RFL_CLI_PRESETS_HPP_
#define RFL_CLI_PRESETS_HPP_

#include "../DefaultIfMissing.hpp"
#include "../NoExtraFields.hpp"
#include "../Processors.hpp"
#include "LastValueWins.hpp"
#include "OptionsFirst.hpp"
#include "PureFlags.hpp"
#include "SeparateOptionValues.hpp"

namespace rfl::cli {

/// Standard GNU-style CLI processor bundle:
/// - `rfl::DefaultIfMissing`: unspecified fields take their struct defaults
/// - `rfl::NoExtraFields`: unknown options are rejected
/// - `rfl::cli::SeparateOptionValues`: `--key value` as well as `--key=value`
/// - `rfl::cli::PureFlags`: short boolean flags never consume positional tokens
using GnuStyle = rfl::Processors<rfl::DefaultIfMissing,
                                 rfl::NoExtraFields,
                                 SeparateOptionValues,
                                 PureFlags>;

/// GNU-style CLI processor bundle where repeated scalar options overwrite
/// earlier occurrences (`--out a --out b` → `"b"`).
using GnuLastWins = rfl::Processors<rfl::DefaultIfMissing,
                                    rfl::NoExtraFields,
                                    SeparateOptionValues,
                                    PureFlags,
                                    LastValueWins>;

/// Strict POSIX-style CLI processor bundle where options stop at the first
/// positional argument (`OptionsFirst`), suitable for wrapper/runner commands.
using PosixStrict = rfl::Processors<rfl::DefaultIfMissing,
                                    rfl::NoExtraFields,
                                    SeparateOptionValues,
                                    PureFlags,
                                    OptionsFirst>;

}  // namespace rfl::cli

#endif
