# Large Struct Metadata & Compile-Time Benchmark (`#90` / `#91`)

Self-contained 48-field `ConfigData` benchmark comparing:

- `bench_48_desc.cpp`: inline `rfl::Description<StringLiteral, rfl::DefaultVal<T>>` on every field.
- `bench_48_metadata.cpp`: plain fields + `rfl::DefaultIfMissing` + out-of-band `rfl::Metadata<ConfigData>`.
- `bench_48_plain.cpp`: plain fields + `rfl::DefaultIfMissing` (no descriptions control).

## Reproduction (AppleClang 17 arm64)

```bash
for f in bench_48_desc bench_48_metadata bench_48_plain; do
  /usr/bin/c++ -std=gnu++20 -Iinclude -Iinclude/rfl/thirdparty -O0 -g -c bench/large_struct_metadata/$f.cpp -o /tmp/$f.debug.o
  /usr/bin/c++ -std=gnu++20 -Iinclude -Iinclude/rfl/thirdparty -O2 -g -c bench/large_struct_metadata/$f.cpp -o /tmp/$f.relwithdebinfo.o
done
```

## Measured Results (AppleClang 17 arm64, before vs after `#91`)

### Debug (`-O0 -g`)

| Variant | Wall (s) | `.o` (MiB) | `__debug_str` (MiB) | Mangled syms (MiB) | `__text` (MiB) | Syms | Max sym |
|---|---:|---:|---:|---:|---:|---:|---:|
| `bench_48_desc` (before `#91`) | 5.75 | 148.71 | 82.65 | 62.77 | 0.66 | 8,883 | 43,854 |
| `bench_48_desc` (after `#91`) | 4.78 | 85.57 (-42.5%) | 50.24 (-39.2%) | 32.13 (-48.8%) | 0.66 | 8,176 | 43,836 |
| `bench_48_metadata` (after `#91`) | **2.76 (-52.0%)** | **13.21 (-91.1%)** | **7.31 (-91.2%)** | **3.61 (-94.2%)** | 0.47 | 6,449 | 11,011 |
| `bench_48_plain` (before `#91`) | 2.78 | 19.12 | 10.19 | 6.63 | 0.45 | 6,718 | 11,026 |
| `bench_48_plain` (after `#91`) | 2.65 | 13.19 (-31.0%) | 7.30 (-28.4%) | 3.60 (-45.7%) | 0.47 | 6,341 | 11,011 |

### RelWithDebInfo (`-O2 -g`)

| Variant | Wall (s) | `.o` (MiB) | `__debug_str` (MiB) | Mangled syms (MiB) | `__text` (MiB) | Syms | Max sym |
|---|---:|---:|---:|---:|---:|---:|---:|
| `bench_48_desc` (before `#91`) | 6.98 | 48.25 | 35.40 | 10.11 | 0.15 | 1,324 | 43,724 |
| `bench_48_desc` (after `#91`) | 6.49 | 25.30 (-47.6%) | 19.94 (-43.7%) | 2.58 (-74.5%) | 0.15 | 1,116 | 43,836 |
| `bench_48_metadata` (after `#91`) | **3.68 (-47.3%)** | **5.13 (-89.4%)** | **3.35 (-90.5%)** | **0.25 (-97.5%)** | **0.08 (-46.7%)** | 837 | 11,011 |
| `bench_48_plain` (before `#91`) | 3.95 | 8.04 | 4.80 | 1.29 | 0.12 | 972 | 10,899 |
| `bench_48_plain` (after `#91`) | 3.69 | 5.11 (-36.4%) | 3.35 (-30.2%) | 0.25 (-80.6%) | 0.08 | 739 | 11,011 |

### `slang-server` key TUs (`-O2 -g`, zero source changes)

| TU | Before `.o` | After `.o` | Mangled before → after | Wall before → after |
|---|---:|---:|---:|---:|
| `SlangServer.cpp` | 60.58 MiB | 57.94 MiB (-4.4%) | 2.04 → 1.55 MiB (-24.0%) | 33.39s → 33.15s |
| `reflectcpp_json.cpp` | 12.07 MiB | 11.19 MiB (-7.3%) | 0.26 → 0.17 MiB (-34.6%) | 7.67s → 7.59s |
| `Config.cpp` | 13.01 MiB | 12.03 MiB (-7.5%) | 0.74 → 0.45 MiB (-39.2%) | 4.86s → 4.72s |
| `gen_config_main.cpp` | 9.46 MiB | 8.17 MiB (-13.6%) | 0.81 → 0.26 MiB (-67.9%) | 3.34s → 3.32s |
