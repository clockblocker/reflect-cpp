---
title: Maintainer review guide (liuzicheng1987)
created: 2026-11-17
period: 2025-05-17 → 2026-11-17
source: github.com/munich-data-engineering/reflect-cpp
tags: [reflect-cpp, maintainer, review]
stage: grouped
---
> AI-compiled from the maintainer's public GitHub comments; quotes are verbatim, summaries are paraphrased. Full collection: [[maintainer-feedback]]

# Maintainer review guide

This guide groups what `liuzicheng1987` (the maintainer) wrote on reflect-cpp issues and PRs between 2025-05-17 and 2026-11-17. The sources are 520 comments in 238 threads: 469 conversation comments, 47 inline review comments and 4 review summaries. Most of the feedback is in the conversation tab, not in formal reviews.

## TL;DR — how to make his review easy

Ranked by how often, and how firmly, the maintainer says it.

1. **Make CI pass on every compiler and OS, and keep the branch current with `main`.** This is the most common request: failing tests, a broken macOS, Windows or Conan job, merge conflicts, or "merge main"[^1][^2][^3][^4][^5][^6][^7][^8][^9][^10]. The maintainer treats vcpkg flakiness as known and not the contributor's fault[^11][^12].
2. **Add a focused test, and make sure CI actually runs it.** One format is usually enough, because "testing it for all formats would be redundant"[^13]. New test files must be wired into GitHub Actions[^14][^15][^16]. Skip tests for trivial code[^17][^18].
3. **Don't break existing users.** Keep old APIs with a deprecation note[^19][^20]. A non-breaking alternative "is always good"[^21]. New behaviour needs a backwards-compatibility story[^22], and new language features go next to the old path, not in place of it[^23][^24].
4. **Document new features in the README and in `docs/`.** A follow-up docs PR is fine, and the maintainer sometimes writes them[^14][^25][^26][^27][^28][^29].
5. **Report failures through `rfl::Result`, never by throwing or terminating.** Removing `noexcept` is fine, but the result must still be an error[^30][^31][^32][^33]. Never dereference a `Result` without checking it[^34].
6. **No surprising runtime behaviour.** One field must not change how another behaves; the maintainer calls that "very unexpected"[^35]. Where a combination can't work, make it a compile-time error (`static_assert`)[^36][^37][^38].
7. **Keep defaults as they are, and put new strictness in an opt-in processor**[^39][^40][^41][^42].
8. **Keep the vcpkg and Conan packages working.** Don't edit vendored third-party code[^43], and don't depend on anything that isn't released on both package managers[^44][^45][^46][^47].
9. **One problem per PR.** Split agreed points from disputed ones, and put cross-cutting additions in their own PR[^48][^49][^50].
10. **Every header must be self-contained, and CI checks this**[^51][^52][^53].
11. **Watch compile time and performance.** Move code into `src/` files, measure before claiming a speed-up, avoid copies, and bring benchmarks for the fast formats[^14][^54][^55][^56][^57].
12. **Use only what the whole compiler matrix supports.** Avoid `std::format` and anything deprecated, such as `wstring_convert`[^58][^59][^60].
13. **Match existing patterns.** New wrappers get the same accessors as existing ones, and new types follow how similar types are handled[^61][^62][^63][^50].
14. **Follow the house style:** `internal`, not `detail`; namespaces that mirror files; unnamed unused parameters; `rfl::Box` for non-null owners[^64][^65][^66][^67].
15. **Don't fix non-problems or add fragility for small gains.** Field-name `string_view`s are null-terminated[^49][^68]. A brittle fix for "a very minor problem" was closed[^69].
16. **Write error messages for end users,** not library developers[^70][^71].
17. **AI help is welcome, but check its claims.** The maintainer filters AI review suggestions, uses coding agents, and isn't picky about how contributions are packaged[^72][^73][^74][^14][^75][^49].
18. **Process:** mark a PR "Ready for review" when it's done, and don't bother requesting the maintainer as a reviewer. Stale PRs with failing tests get closed and can be reopened[^76][^77][^78][^79].

## Themes

### Design and API shape

- Prefer general building blocks over a one-off feature. For deep comparison the maintainer proposed a `zip` operator or named tuples of references[^80][^81].
  > I wouldn't want to go with a solution that cover just this one specific use case.
- Make invalid combinations fail at compile time. The maintainer asked for a `static_assert` against combining `AddTagsToVariants` and `AddNamespacedTagsToVariants`[^36][^37], and made reading XML into `rfl::Generic` a compile-time error[^38].
  > Therefore, I have decided to add a compile-time error and document the behaviour.
- One field must not change how another behaves at runtime (#786, 2026-11-16). Documentation doesn't excuse it, because "very few people read every page of the documentation before using a library"[^35].
  > All of a sudden, your app behaves differently at runtime then before. This is very unexpected and feels like bug.
- Tie features to fields. The maintainer chose `rfl::Commented<T>` over a separate `rfl::Comment` "because the latter solution would create many dead fields and the former allows us to tie comments directly to fields"[^82].
- Be consistent with existing types. `std::expected` got an error field "because that is how we handle rfl::Result and I wanted to keep it consistent"[^62].
- Some deliberate choices are not bugs:
  - `rfl::Literal` assignment is implicit: "I very consciously did not make it explicit."[^83]
  - `Box` and `Ref` compare addresses, the same way `unique_ptr` and `shared_ptr` do[^49].
  - Enums accept any integer[^39].
- Fix validation holes by validating more, not by removing API[^49].
  > The solution is to validate them as well.
- Readers and writers are a public extension point: "I actually do encourage people to write their own readers and writers"[^55]. Internals can be made public on request, such as `num_fields`, through a thin public header[^84][^65].
- Mechanisms the maintainer flagged in proposals:
  - a `void` pointer[^48]
  - a suspected-UB `const_cast`[^85]
  - `source_location` outside its "very controlled" uses[^69]
- Use the library's own vocabulary types. For a pointer that must not be null, `rfl::Box` "establishes that more clearly" and removes hand-written copies[^67][^86]. Plain `std::variant` was accepted where the reasons were given[^87].

### Backwards compatibility and user impact

- Breaking API changes frustrate users, even when the new design is better (#662, 2026-05-10). The agreed approach: keep the old functions, mark them deprecated, and document only the new style. The PR was merged once that was done[^19][^20][^88].
  > I think that people use them in the productive system and when you have to change that unexpectedly, that makes people angry.
- Non-breaking options win. A separate processor was chosen over changing the existing one (#467 → #494)[^21].
  > It would also be a non-braking change, which is always good.
- Wire-format changes need a compatibility plan. On non-string map keys: "I would have to think about how to make this backwards-compatible"[^22].
- New standards are additive. C++26 reflection is meant to sit alongside C++20 support[^23]. Modules are "just an optional add-on" and headers "will remain the default for the foreseeable future"[^24][^89].

### Defaults, validators and processors

- Defaults stay put, and stricter behaviour goes in a processor. Out-of-range enum integers are intended and documented. The maintainer said so in both 2025-08 and 2026-08 and offered a processor each time[^39][^40].
  > I don't agree that  the default behaviour should be not to accept out-of-range enums, but I think we could add a processor that disallows out-of-range enums.
- Processors are the go-to mechanism. That was the maintainer's first instinct for renaming enum values[^41], and opt-in `FailFast` was merged[^42].
- `rfl::DefaultVal` exists because people expected in-class initialisers to act as defaults. Its behaviour must not depend on other fields[^35].
- Reading a struct with defaults default-constructs it first, so a default value that fails its validator breaks the read. The maintainer saw no clean fix and proposed documenting it better[^90][^91]. An exception thrown there must come back as an error[^32].
- `rfl::Field` can't take default values, because every Field has to be set explicitly[^92].
- When a proposal might add runtime overhead, the maintainer offers an opt-in validator, such as `rfl::CheckMinMax<int>`[^93].

### Error handling and error messages

- Errors go through `Result`, consistently (#674, 2026-06)[^30][^31][^32][^33].
  > This is what happens for just about any other mistake that is made and it should be consistend.

  Users who want exceptions can call `.value()`[^30].
- Never dereference an unchecked `Result`; use `.transform(...)` (#463, 2025-08)[^34].
  > This i a cardinal sin - because when the parsing fails, as it did in your second test case, you get UB.
- Throwing is reserved for truly exceptional cases, "such as running out of disc space or memory"[^94][^95].
- Error messages are for end users. A `NoExtraFields` message that told users to remove a processor was "obviously a mistake"[^70]. An improved invalid-enum message was merged[^96].
- Tagged variants give clearer errors for complex structs: "It also leads to better error messages." For variants of primitives, the maintainer would skip the tags[^71].

### Naming and code style

- Use `internal`, not `detail`[^64].
  > minor nitpick: We usually use "internal" instead of "detail" in this library.
- Namespaces mirror the file layout[^65].
  > I think the namespaces should always follow the file structure, wherever possible.
- Leave unused parameters unnamed (`(auto&&)`) rather than using `[[maybe_unused]]`[^66][^97].
  > That is what we do in the rest of the codebase and it is also more concise.
- Module names follow namespaces, not Java-style reverse domains[^98].
- New wrapper types need `operator*`, `.value()`, `.get()`, `operator()` and a non-const `.get()`[^61][^99]. The maintainer prefers `()` to `->` but accepted `->` on request[^100].
- Remove redundant includes[^101]. Every header must be self-contained[^51][^52].

### Tests and CI

- Each behaviour change needs a test, usually for JSON only[^13][^102][^103].
  > Just for JSON is fine...testing it for all formats would be redundant.
- Don't over-test: "overkill" for `Error::what()`, and "I don't think this is necessary" for extra AI-suggested cases[^17][^18][^104].
- Tests must run in CI, not only locally[^14][^15][^16].
  > You need to update the Github Actions files to actually run your tests.

  Smoke tests are enough for large features such as modules[^105]. For a new format, copy the BSON tests[^14].
- Settle factual disputes with a test: "The most obvious apprach here would be to write a simple test."[^83]
- Failing jobs come back to the contributor[^1][^2][^3][^4][^106][^5][^107][^108]. Platform failures count too: "your solution fails for macos-latest", perhaps needing "a version-specific solution"[^6].
- Infrastructure flakiness is known and isn't held against PRs. Unrelated breakage gets fixed separately[^11][^12][^109][^110].

### Documentation

- Significant features belong in the README and in a section of the docs[^14][^25][^27].
  > This is a significant and very cool new functionality that should be highlighted in the README.
- Docs can come in a separate PR[^26][^28]. Sometimes the maintainer writes them instead[^29].
- Limits that can't be removed should be documented better[^90][^111]. In response to docs criticism, the maintainer added Doxygen docstrings[^112].

### PR scope, size and process

- Merge criteria (2025-08)[^113]:
  > if the checks pass, the contribution is something that we would be willing to maintain the code quality is good, we merge.
- One problem per PR (#680)[^48].
  > I don't think it's a good idea to be trying to fix two problems at once
- Split mixed PRs (#618)[^49].
  > My suggestion would be to open a separate PR with all the points on which we agree and continue the discussion on the other points.
- A change that would apply to every wrapper is a separate PR[^50].
- Mark a PR "Ready for review" when it's done[^76]. There's no need to request the maintainer as a reviewer[^77].
- Inactive PRs with failing tests get closed, and reopening is welcome[^78][^114][^115][^79]. Issues without a reply for weeks get closed[^116][^117].
- The maintainer may fix things in parallel, so a contributor's PR can be superseded[^118][^119][^48].
- Reports need exact reproduction steps and versions[^120][^121].
- Releases happen when needed[^122]. In 2026-10 the maintainer tied a 1.0.0 release to a decision on #719[^123].

### AI-generated contributions and AI review

- The AI review bot is advisory:
  - Ignore false positives: "Gemini sometimes has false positives."[^72][^73]
  - Low-value suggestions can be skipped: "Feel free to just ignore Gemini on this."[^124][^125][^126]
  - Some get one-word rejections[^127][^128][^129].
  - Good ones are endorsed: "I think that the solution suggested by Gemini is spot on."[^33][^30]
- Coding agents are fine for tedious work and for docs[^74][^14].
  > Feel free to write that using a coding agent of your choice (if you don't have one, I can do that).
- AI-found "bugs" still get checked: "Again, I am quite certain you (or the AI) are wrong about this."[^49] After more discussion the maintainer conceded some points[^130].
- On #737, the maintainer's own PR, the posted review reads as an AI agent's report; it ends "Say the word and I'll dispatch the 4 reviewer agents to complete the sweep."[^131]
- No strong view on packaging AI-assisted fixes as several PRs or one: "Either way is fine. I'm not too picky."[^75]

### Dependencies, build and packaging

- Don't modify vendored third-party libraries[^43].
  > As a general rule, we shouldn't make changes to third-party libraries.
- vcpkg and Conan decide dependency choices:
  - TOML moved between toml++ and toml11 depending on whether the recipe exported CMake config[^132].
  - New upstream APIs wait for a release on both package managers[^44].
  - Duplicate vendored copies of a library "can cause all sort of problems"[^47].
- Breaking Conan blocks a PR[^7][^133][^45]. For enchantum, which isn't on Conan, the maintainer wanted a separate flag rather than edited recipes[^134][^46].
- Safe build fixes with "absolutely no downsides" are taken without a repro[^135]. Warnings from third-party code are outside the library's control[^136].
- When the vcpkg port fails but a build from the repo works, the problem should be reported to vcpkg[^137]. Distro packages were made without the maintainer's involvement[^138][^139].

### C++ standard and compilers

- Code must compile on every compiler in CI: "I am not sure that std::format is supported by all the compiler versions we test for."[^58]
- No deprecated facilities[^59].
  > we will have to avoid wstring_convert. It is deprecated and will be removed in C++ 26.
- C++23 on older compilers gets limited support. For gcc 13 with `std::expected`, "you should probably be using gcc 14 or above anyway", though the maintainer still made it work[^60].
- C++26 reflection:
  - 2025-10: planned as an addition, keeping C++20 support[^23].
  - 2026-03: "Absolutely, that's the plan."[^140]
  - 2026-08: support added[^141].
  - Expected to remove limits such as flag enums with value 0[^142].
- On the gcc 12 crash (#719), the maintainer suspected a compiler bug and preferred the simplest workaround[^143].

### Performance and compile time

- Compile time is "a major issue", so move code into source files[^14].
  > Compile time is a major issue in a project like this and anything we can do to reduce it, we should do.
- Measure first. Profiling showed struct reflection takes under 5% of compile time in the README test and about 20–25% in a 70-field test[^54][^144][^145].
  > The bottleneck isn't the reflection, it is generating the code for serialization and deserialization.
- Users can keep compile time down by calling `read`, `write` and `to_schema` only in `.cpp` files[^146].
- Avoid deep copies, but take safety fixes that don't add copies[^55][^68].
- Acceptable overhead depends on the format. Avro and YAML are slow anyway, but flexbuffers and msgpack need benchmarks first[^56][^147].
  > For Avro I don't really care that your changes introduce overhead. This format is slow AF anyway.
- Performance is a priority from 2026-10[^57].
  > But I think that performance improvements should be a major goal moving forward. We are still slower than YYJSON on most benchmarks.

  The maintainer welcomed "perfect result" baseline benchmarks[^148] and merged a performance series[^149][^150][^151].
- Benchmarks stay in CI rather than in the README: "If we write them into the README, they will become static."[^152] New backends should join the benchmarks[^14].

### CLI (`rfl::cli`)

- The maintainer was enthusiastic about the parser ("awesome stuff") but asked for README coverage[^153][^25].
- Review points on #613: `internal`, not `detail`; add the standard accessors; the bot's `requires` suggestion is optional[^64][^61][^124]. Final review: "beautiful, no further comments."[^154]
- API improvements are welcome: const `argv` and ranges (#623) "sound great"[^155], and a help or usage generator is welcome as a PR[^156].
- `wchar_t` argv: the maintainer called it trickier than expected[^157], ruled out `wstring_convert`[^59], and chose to convert on input while still allowing `std::wstring` targets[^158].
- clockblocker's CLI PRs were merged once `main` had been merged in (#783, #785)[^9][^159][^160][^161]. #784 is waiting on conflict resolution[^10].

### Schema generation

- The internal schema type also serves Avro and Cap'n Proto, so changes must suit those formats. Tagged unions could be added, and `Literal` already covers enums[^162][^163].
- No separate schema type for `Vectorstring`, because it's the same as `Bytestring` to every format[^164].
- `DefaultVal` in schemas (#679 and #396): fix them separately, avoid a `void` pointer, and handle #396 through `rfl::Generic`[^48][^165]. Headers must be self-contained before merge[^51].
- `additionalProperties: false` with `NoExtraFields` "makes a lot of sense", once the tests were adapted[^166][^167].
- Integer min and max in the schema: the maintainer first worried about overhead[^93], then wrote "sorry, I misunderstood the issue. I think this is very possible."[^168]
- Schema features need docs (`rfl::Deprecated`, enum descriptions)[^26][^27][^169].

### Per-format notes

- **JSON**
  - Embedded `\0` comes from yyjson, maybe through a flag[^170][^171].
  - Merged: out-of-range integer rejection[^172] and Base64 bytestrings[^173].
- **XML**
  - Reading into `rfl::Generic` is a compile-time error, because "it is practically impossible to read XML into generics without any form of pre-defined structure"; writing is fine[^38].
- **YAML**
  - `on`/`off` are booleans per the YAML spec and yaml-cpp[^174][^175].
  - Speed isn't a concern for YAML users[^147].
- **TOML**
  - toml++ was chosen for speed[^132].
  - Exception-free builds are worth fixing[^176].
  - toml++ doesn't support `char`[^177].
  - Formatting flags can be exposed: "We do something similar for JSON."[^63]
  - Reading an integer as a double was merged[^178].
- **msgpack and binary formats**
  - Floats should keep their width, and other binary formats should be checked too[^179][^180].
  - Variants of primitives don't need tags[^71].
- **CBOR**
  - Slow because "there simply is no faster library"[^181].
- **Avro**
  - Slow, so overhead is acceptable[^56].
  - Handling write errors means dropping `noexcept`[^94].
- **Boost.Serialization and cereal**
  - Boost support inspired cereal support.
  - Security concern: there is little validation, though that's Boost's problem[^14].
- **Validators and regexes**
  - CTRE 3.10 rejects the trailing `\0` in compile-time strings, which needed a fix[^182].

### Where the maintainer's view changed or was inconsistent

- **C++ modules:**
  - 2025-08: asked for them to be optional[^89].
  - 2025-09: go ahead[^183].
  - 2026-01: declined to merge #578 because of compiler issues[^184].

    > I just don't feel comfortable committing to maintaining these kind of things for now.
  - 2026-05: "I'd be happy to give them another try."[^185]
  - 2026-09: "If we can get it to work reliably, let's go for it."[^186]
- **Deserializing `std::string_view`:** unsure about the side effect on 2025-07-06[^187], then supportive two days later[^188].
- **Strict-warnings toggle (#620):** "honestly unsure" on 2026-03-02[^136]. The PR was merged on 2026-03-16[^189].
- **Schema min and max (#683):** overhead worry on 2026-06-23, then "very possible" on 2026-07-23[^93][^168].
- **#618 disputed points:** on 2026-02-25 the maintainer questioned issues 4 and 28, and said issue 16 solves "a problem that doesn't exist" unless the fix is easy[^49]. On 2026-03-04 the maintainer conceded 4 and 28 and approved 16[^130].
- **`[[maybe_unused]]`:** #547 was merged "as is" on 2025-11-23[^190], and a contributor said it had added `[[maybe_unused]]`[^191]. On 2026-01-26 the maintainer asked for unnamed parameters instead[^66].
- **`noexcept`:** in 2025-10, dropping it so writers could throw on disk or memory failures was fine[^95]. In 2026-06, validation failures had to be errors, not exceptions[^32]. The distinction is truly exceptional failures versus user mistakes.
- **#719 fix:** the maintainer's own #737 caused a performance regression and other problems[^192]. The simpler fix from Altren was preferred[^143][^193].

## Proposals he accepted / rejected

Verdicts: **A** accepted, **A+c** accepted with conditions, **R** rejected or declined, **P** pending or undecided.

| Proposal | Verdict | Maintainer's reason | Date | Refs |
|---|---|---|---|---|
| #441 enchantum enum backend | A+c | Contributor to report when it's on vcpkg and Conan | 2025-05 | [^47][^194] |
| #443 deep comparison via `to_view` | P | Wants a general solution, not one use case | 2025-06 | [^80] |
| #454 deserialize `std::string_view` | A+c | User frees the memory, like raw pointers | 2025-07 | [^187][^188] |
| #455 fallback type for tagged unions | P | Use `std::variant<TaggedUnion<...>, FallbackType>` | 2025-07 | [^195] |
| #459/#466 `vector<char>` as bytestring | A+c | No separate schema type | 2025-07 | [^196][^164][^197] |
| #467→#494 namespaced tags | A+c | Separate, non-breaking processor plus `static_assert` | 2025-09 | [^21][^37][^198] |
| #504/#505 `Error::what() &&` | A | No tests needed | 2025-09 | [^199][^17] |
| #538 don't assume null-terminated `string_view` | A+c | Only where no copies are added; benchmark fast formats | 2025-11 | [^68][^56] |
| #575 `std::atomic` fields | A | Implemented by the maintainer | 2026-01 | [^200][^201] |
| #578 C++ modules | R | Too many compiler issues; moved to a branch | 2026-01 | [^184] |
| #669 modules, second attempt; edits to vendored yyjson | P / R | Retry welcome, tests must run in CI; don't edit third-party code | 2026-05 | [^185][^15][^43] |
| #581 bundled-deps flag covering enchantum | R | Would break Conan; wanted a separate flag | 2026-01 | [^45][^134][^114] |
| #582 unused-parameter warnings | A+c | Unnamed parameters, not `[[maybe_unused]]` | 2026-01 | [^66][^202] |
| #584 arbitrary duration representations | A+c | Add a JSON test | 2026-01 | [^13][^102] |
| #594 benchmark table in README | R | Would become static | 2026-02 | [^152] |
| #597 serialize comments | A+c | As `rfl::Commented<T>` | 2026-02 | [^203][^82] |
| `operator*` in `Commented` | A+c | Separate PR for all wrappers | 2026-02 | [^50][^74] |
| #613 CLI parser | A+c | README, accessors, `internal` naming | 2026-02 | [^25][^61][^154] |
| #618 30 bug fixes | A+c | Disputed items split out; some conceded later | 2026-03 | [^49][^130][^204] |
| #620 strict-warnings toggle | A | Initially unsure, merged later | 2026-03 | [^136][^189] |
| #623 const argv and ranges for `rfl::cli::read` | A | "sound great" | 2026-02 | [^155] |
| #636/#641 Boost.Serialization | A+c | Docs, CI, `src/` files, benchmarks, more tests | 2026-04 | [^205][^14][^206] |
| #637 C++26 reflection | A | "that's the plan"; added | 2026-08 | [^140][^141] |
| #662 unified `Settings::with<>()` | A+c | Keep the old functions, deprecated | 2026-05 | [^19][^88] |
| #674 drop `noexcept` and let defaults throw | R | Must return an error; replaced by the maintainer's #686 | 2026-06 | [^32][^119] |
| #680 `DefaultVal` in schema | A+c | One issue per PR, no `void` pointer, self-contained headers | 2026-06 | [^48][^51][^207] |
| #683 integer min and max in schema | P | First an overhead worry, then "very possible" | 2026-07 | [^93][^168] |
| #684 `operator->` on wrappers | A | Maintainer prefers `()`; implemented anyway | 2026-07 | [^100][^208] |
| #685 `rfl::Field` plus `DefaultVal` | R | Fields must all be set explicitly | 2026-06 | [^92] |
| #692 CLI help generation | A | PR welcome | 2026-07 | [^156] |
| #693/#696 `operator==` for `Generic` and `Object` | A | Merged | 2026-07 | [^209][^210] |
| #706 `wchar_t` argv | A+c | Convert on input; no `wstring_convert` | 2026-07 | [^59][^158] |
| #707 validators next to defaults via `source_location` | R | Brittle; a very minor problem | 2026-08 | [^69][^211] |
| #708/#710 keep float width in msgpack | A | No good reason for the current behaviour | 2026-07 | [^179][^180] |
| #463/#714 reject unknown enum integers by default | R | Valid C++; offer a processor instead | 2025-08, 2026-08 | [^39][^40][^212] |
| #716 serialize `std::expected` | A | Error field, consistent with `Result` | 2026-08 | [^213][^62] |
| #720 reject multi-field external tags | A | "good catch" | 2026-08 | [^214][^215] |
| #723 native non-string map keys | P | Needs a backwards-compatible design | 2026-09 | [^22] |
| #728 `additionalProperties: false` | A+c | Adapt the tests | 2026-10 | [^166][^167] |
| #729 `from_reflection` | A | "I love this idea." | 2026-09 | [^216] |
| #731 renaming enum values | P | Probably a processor; needs thought | 2026-09 | [^41] |
| #738–#761 Altren performance series, incl. opt-in `FailFast` | A | Merged | 2026-10 | [^149][^150][^42][^217] |
| #737 the maintainer's own #719 fix | R | Regressions; preferred Altren's simpler #752 | 2026-10 | [^192][^143][^193] |
| #766 `Error::details()` (clockblocker) | A+c | Use `rfl::Box`; fix the build | 2026-11 | [^67][^8][^218] |
| #767 fewer copies and allocations (clockblocker) | A | Merged | 2026-11 | [^219] |
| #769 `CopyableBox` assignment (clockblocker) | A | Merged | 2026-11 | [^220] |
| #776, #778, #779, #782 throwing defaults (clockblocker) | A | Merged | 2026-11 | [^221][^222][^223][^224] |
| #783, #785 CLI negatives and optionals (clockblocker) | A+c | Merge `main` first | 2026-11 | [^9][^159][^160][^161] |
| #787 const fields with defaults (clockblocker) | A | Merged | 2026-11 | [^225] |
| #784 const argv and ranges (clockblocker) | P | Fix merge conflicts | 2026-11 | [^10] |
| #786 `DefaultVal` in NamedTuples and schemas (clockblocker) | P | Rejected the behaviour change (since dropped); flagged a cast as UB | 2026-11 | [^35][^85][^226] |

## Sources

[^1]: <https://github.com/munich-data-engineering/reflect-cpp/pull/484#issuecomment-3204435777> — #484 Better forwarding to improve read performance a little (2025-08-20)
[^2]: <https://github.com/munich-data-engineering/reflect-cpp/pull/538#issuecomment-3478262099> — #538 Avoid assuming that "std::string_view" is nullterminated. Generally it is really not. (2025-11-02)
[^3]: <https://github.com/munich-data-engineering/reflect-cpp/pull/610#issuecomment-3885253719> — #610 Json Schema: Add enum value descriptions (2026-02-11)
[^4]: <https://github.com/munich-data-engineering/reflect-cpp/pull/621#issuecomment-3948800483> — #621 Allows usage of custom Reflector for enums types (2026-02-24)
[^5]: <https://github.com/munich-data-engineering/reflect-cpp/pull/668#issuecomment-4455165657> — #668 fix(json): don't crash on non-ASCII std::wstring / std::filesystem::path (2026-05-14)
[^6]: <https://github.com/munich-data-engineering/reflect-cpp/pull/688#issuecomment-4882920657> — #688 Avoid clang 21/22 C++26-mode frontend crash in StringLiteral's variadic constructor (2026-07-04)
[^7]: <https://github.com/munich-data-engineering/reflect-cpp/pull/568#issuecomment-3708452488> — #568 fix: CMake package and imported library target names (2026-01-04)
[^8]: <https://github.com/munich-data-engineering/reflect-cpp/pull/766#issuecomment-6655198583> — #766 Expose the location of each parse failure via rfl::Error::details() (2026-11-05)
[^9]: <https://github.com/munich-data-engineering/reflect-cpp/pull/783#issuecomment-6897318462> — #783 Read a bare negative number such as `-5` as a positional argument in rfl::cli (2026-11-15)
[^10]: <https://github.com/munich-data-engineering/reflect-cpp/pull/784#issuecomment-6926170862> — #784 Accept const argv and ranges of arguments in rfl::cli::read (2026-11-16)
[^11]: <https://github.com/munich-data-engineering/reflect-cpp/pull/668#issuecomment-4461108502> — #668 fix(json): don't crash on non-ASCII std::wstring / std::filesystem::path (2026-05-15)
[^12]: <https://github.com/munich-data-engineering/reflect-cpp/pull/654#issuecomment-4330340815> — #654 Feat/parse null into nullopt optionals (2026-04-27)
[^13]: <https://github.com/munich-data-engineering/reflect-cpp/pull/584#issuecomment-3801626810> — #584 Support parsing arbitrary duration representations (2026-01-26)
[^14]: <https://github.com/munich-data-engineering/reflect-cpp/pull/641#issuecomment-4148265384> — #641 Boost serialization implementation (2026-03-28)
[^15]: <https://github.com/munich-data-engineering/reflect-cpp/pull/669#issuecomment-4487775677> — #669 Add C++20 modules support and format btw. (2026-05-19)
[^16]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3771909519> — #578 Add support for C++ modules (2026-01-20)
[^17]: <https://github.com/munich-data-engineering/reflect-cpp/pull/505#issuecomment-3344975674> — #505 added explicit ref-qualifiers for Error::what() and a additional over… (2025-09-29)
[^18]: <https://github.com/munich-data-engineering/reflect-cpp/pull/466#discussion_r2217284535> — #466 Serialize/Deserialize `vector<char>` as bytestring (2025-07-19)
[^19]: <https://github.com/munich-data-engineering/reflect-cpp/pull/662#issuecomment-4415121180> — #662 Add unified rfl::Settings::with<>() API for format-specific settings (2026-05-10)
[^20]: <https://github.com/munich-data-engineering/reflect-cpp/pull/662#issuecomment-4415661959> — #662 Add unified rfl::Settings::with<>() API for format-specific settings (2026-05-10)
[^21]: <https://github.com/munich-data-engineering/reflect-cpp/pull/467#issuecomment-3270729365> — #467 Allow using `rfl::AddTagsToVariants` with `rfl::Generic` and same-name-different-namespace structs (2025-09-09)
[^22]: <https://github.com/munich-data-engineering/reflect-cpp/issues/723#issuecomment-5575319188> — #723 Support native non-string map keys for formats that allow them (2026-09-07)
[^23]: <https://github.com/munich-data-engineering/reflect-cpp/pull/515#issuecomment-3392202985> — #515 Cleanup includes (2025-10-10)
[^24]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3780867900> — #578 Add support for C++ modules (2026-01-21)
[^25]: <https://github.com/munich-data-engineering/reflect-cpp/pull/613#issuecomment-3912367693> — #613 Add CLI argument parser (rfl::cli::read) (2026-02-17)
[^26]: <https://github.com/munich-data-engineering/reflect-cpp/pull/624#issuecomment-3970177511> — #624 Add rfl::Deprecated, which adds deprecated and deprecationMessage to the generated schema field. (2026-02-27)
[^27]: <https://github.com/munich-data-engineering/reflect-cpp/pull/626#issuecomment-4005858995> — #626 json schema: Add enum value descriptions (2026-03-05)
[^28]: <https://github.com/munich-data-engineering/reflect-cpp/pull/588#issuecomment-3814491068> — #588 Add camel case to snake case conversion (2026-01-28)
[^29]: <https://github.com/munich-data-engineering/reflect-cpp/pull/626#issuecomment-4068885980> — #626 json schema: Add enum value descriptions (2026-03-16)
[^30]: <https://github.com/munich-data-engineering/reflect-cpp/pull/674#discussion_r3356042928> — #674 fix abort when default value does not satisfy Validator (2026-06-04)
[^31]: <https://github.com/munich-data-engineering/reflect-cpp/pull/674#discussion_r3358663932> — #674 fix abort when default value does not satisfy Validator (2026-06-04)
[^32]: <https://github.com/munich-data-engineering/reflect-cpp/pull/674#discussion_r3409480624> — #674 fix abort when default value does not satisfy Validator (2026-06-14)
[^33]: <https://github.com/munich-data-engineering/reflect-cpp/pull/674#issuecomment-4639782500> — #674 fix abort when default value does not satisfy Validator (2026-06-06)
[^34]: <https://github.com/munich-data-engineering/reflect-cpp/issues/463#issuecomment-3156286737> — #463 Json Read rfl::UnderlyingEnum processor is broken (2025-08-05)
[^35]: <https://github.com/munich-data-engineering/reflect-cpp/pull/786#discussion_r4590305835> — #786 Fix rfl::DefaultVal in NamedTuples and TaggedUnion schemas (2026-11-16)
[^36]: <https://github.com/munich-data-engineering/reflect-cpp/pull/494#discussion_r2337360190> — #494 Add `rfl::AddNamespacedTagsToVariants` for `rfl::Generic` and same-name-different-namespace structs (2025-09-10)
[^37]: <https://github.com/munich-data-engineering/reflect-cpp/pull/494#discussion_r2337369193> — #494 Add `rfl::AddNamespacedTagsToVariants` for `rfl::Generic` and same-name-different-namespace structs (2025-09-10)
[^38]: <https://github.com/munich-data-engineering/reflect-cpp/issues/614#issuecomment-4016844417> — #614 Can't parse XML into a rfl::Generic (2026-03-07)
[^39]: <https://github.com/munich-data-engineering/reflect-cpp/issues/463#issuecomment-3170674087> — #463 Json Read rfl::UnderlyingEnum processor is broken (2025-08-09)
[^40]: <https://github.com/munich-data-engineering/reflect-cpp/issues/714#issuecomment-5380578754> — #714 Enum can be deserialized from raw integers that don't map to enum values (2026-08-22)
[^41]: <https://github.com/munich-data-engineering/reflect-cpp/issues/731#issuecomment-5919842882> — #731 Add ability to rename enum values (2026-09-30)
[^42]: <https://github.com/munich-data-engineering/reflect-cpp/pull/744#issuecomment-6248920396> — #744 Add opt-in FailFast processor for object field errors (2026-10-18)
[^43]: <https://github.com/munich-data-engineering/reflect-cpp/pull/669#discussion_r3264491128> — #669 Add C++20 modules support and format btw. (2026-05-19)
[^44]: <https://github.com/munich-data-engineering/reflect-cpp/issues/429#issuecomment-3127475908> — #429 Support CBOR parsing using error codes (2025-07-28)
[^45]: <https://github.com/munich-data-engineering/reflect-cpp/pull/581#issuecomment-3796985261> — #581 Make REFLECTCPP_USE_BUNDLED_DEPENDENCIES option consistent with enchantum (2026-01-25)
[^46]: <https://github.com/munich-data-engineering/reflect-cpp/pull/581#issuecomment-3799839213> — #581 Make REFLECTCPP_USE_BUNDLED_DEPENDENCIES option consistent with enchantum (2026-01-26)
[^47]: <https://github.com/munich-data-engineering/reflect-cpp/pull/441#issuecomment-2918859182> — #441 Add `enchantum` as an enum reflection backend (2025-05-29)
[^48]: <https://github.com/munich-data-engineering/reflect-cpp/pull/680#issuecomment-4653351883> — #680 WIP: respects DefaultVal in schema and adds default value to schema. fixes #679, #396 (2026-06-08)
[^49]: <https://github.com/munich-data-engineering/reflect-cpp/pull/618#issuecomment-3956502226> — #618 Fix 30 bugs found during code review (2026-02-25)
[^50]: <https://github.com/munich-data-engineering/reflect-cpp/issues/597#issuecomment-3865848094> — #597 Feature Request: Add support for serializing comments (2026-02-08)
[^51]: <https://github.com/munich-data-engineering/reflect-cpp/pull/680#issuecomment-4692394505> — #680 WIP: respects DefaultVal in schema and adds default value to schema. fixes #679, #396 (2026-06-12)
[^52]: <https://github.com/munich-data-engineering/reflect-cpp/issues/528#issuecomment-3423603187> — #528 Missing Includes (2025-10-20)
[^53]: <https://github.com/munich-data-engineering/reflect-cpp/pull/725#issuecomment-5760700690> — #725 fix `Generic.cpp` build error due to missing includes (2026-09-21)
[^54]: <https://github.com/munich-data-engineering/reflect-cpp/pull/537#issuecomment-3482911915> — #537 Speed up the reflection (2025-11-03)
[^55]: <https://github.com/munich-data-engineering/reflect-cpp/pull/445#issuecomment-2954191769> — #445 Improved support for string_views for TOML and XML (2025-06-08)
[^56]: <https://github.com/munich-data-engineering/reflect-cpp/pull/538#issuecomment-3478313524> — #538 Avoid assuming that "std::string_view" is nullterminated. Generally it is really not. (2025-11-02)
[^57]: <https://github.com/munich-data-engineering/reflect-cpp/pull/743#issuecomment-6250642872> — #743 Improve shared parser inlining and generated code layout (2026-10-18)
[^58]: <https://github.com/munich-data-engineering/reflect-cpp/pull/515#discussion_r2422957317> — #515 Cleanup includes (2025-10-11)
[^59]: <https://github.com/munich-data-engineering/reflect-cpp/issues/706#issuecomment-5101116536> — #706 [Feature Request] ref::cli::read wchar_t **argv[] support (2026-07-28)
[^60]: <https://github.com/munich-data-engineering/reflect-cpp/issues/570#issuecomment-3734495690> — #570 Compilation Error with REFLECTCPP_USE_STD_EXPECTED=ON (2026-01-11)
[^61]: <https://github.com/munich-data-engineering/reflect-cpp/pull/613#discussion_r2815046702> — #613 Add CLI argument parser (rfl::cli::read) (2026-02-17)
[^62]: <https://github.com/munich-data-engineering/reflect-cpp/issues/716#issuecomment-5417625424> — #716 Support serialising std::expected (2026-08-25)
[^63]: <https://github.com/munich-data-engineering/reflect-cpp/issues/651#issuecomment-4245833548> — #651 Option to disable toml indentation (2026-04-14)
[^64]: <https://github.com/munich-data-engineering/reflect-cpp/pull/613#discussion_r2815036048> — #613 Add CLI argument parser (rfl::cli::read) (2026-02-17)
[^65]: <https://github.com/munich-data-engineering/reflect-cpp/pull/465#discussion_r2211644334> — #465 Make `rfl::num_fields` public (2025-07-16)
[^66]: <https://github.com/munich-data-engineering/reflect-cpp/pull/582#issuecomment-3799727965> — #582 Fix some unused parameters warnings (2026-01-26)
[^67]: <https://github.com/munich-data-engineering/reflect-cpp/pull/766#discussion_r4479032468> — #766 Expose the location of each parse failure via rfl::Error::details() (2026-11-05)
[^68]: <https://github.com/munich-data-engineering/reflect-cpp/pull/538#issuecomment-3478243342> — #538 Avoid assuming that "std::string_view" is nullterminated. Generally it is really not. (2025-11-02)
[^69]: <https://github.com/munich-data-engineering/reflect-cpp/pull/707#issuecomment-5084356396> — #707 Allow validated fields to work alongside defaulted ones (2026-07-26)
[^70]: <https://github.com/munich-data-engineering/reflect-cpp/issues/598#issuecomment-3856583675> — #598 Update error message for unused values (2026-02-05)
[^71]: <https://github.com/munich-data-engineering/reflect-cpp/issues/619#issuecomment-3942309638> — #619 Question: When using rfl::Variant with the msgpack processor, is AddTagsToVariants nessasary? (2026-02-23)
[^72]: <https://github.com/munich-data-engineering/reflect-cpp/pull/549#discussion_r2554191718> — #549 `rfl::Generic`: Allow `int64_t` -> `double` conversion in `to_double` (2025-11-23)
[^73]: <https://github.com/munich-data-engineering/reflect-cpp/pull/588#discussion_r2733957156> — #588 Add camel case to snake case conversion (2026-01-27)
[^74]: <https://github.com/munich-data-engineering/reflect-cpp/issues/597#issuecomment-3865898797> — #597 Feature Request: Add support for serializing comments (2026-02-08)
[^75]: <https://github.com/munich-data-engineering/reflect-cpp/issues/777#issuecomment-6808150931> — #777 Inquiry about the format of contributions   (2026-11-11)
[^76]: <https://github.com/munich-data-engineering/reflect-cpp/pull/466#issuecomment-3085195208> — #466 Serialize/Deserialize `vector<char>` as bytestring (2025-07-17)
[^77]: <https://github.com/munich-data-engineering/reflect-cpp/pull/720#issuecomment-5462298442> — #720 Reject externally tagged variants with more than one field (2026-08-29)
[^78]: <https://github.com/munich-data-engineering/reflect-cpp/pull/568#issuecomment-3938729923> — #568 fix: CMake package and imported library target names (2026-02-21)
[^79]: <https://github.com/munich-data-engineering/reflect-cpp/pull/568#issuecomment-3940896026> — #568 fix: CMake package and imported library target names (2026-02-22)
[^80]: <https://github.com/munich-data-engineering/reflect-cpp/issues/443#issuecomment-2948042584> — #443 Suggestion: Deep comparison for `rfl::to_view()` (2025-06-06)
[^81]: <https://github.com/munich-data-engineering/reflect-cpp/issues/443#issuecomment-2948044880> — #443 Suggestion: Deep comparison for `rfl::to_view()` (2025-06-06)
[^82]: <https://github.com/munich-data-engineering/reflect-cpp/issues/597#issuecomment-3865833423> — #597 Feature Request: Add support for serializing comments (2026-02-08)
[^83]: <https://github.com/munich-data-engineering/reflect-cpp/pull/618#issuecomment-3949057935> — #618 Fix 30 bugs found during code review (2026-02-24)
[^84]: <https://github.com/munich-data-engineering/reflect-cpp/issues/451#issuecomment-2982140014> — #451 Reflect on the number of fields of a too-complex type at compile time? (2025-06-17)
[^85]: <https://github.com/munich-data-engineering/reflect-cpp/pull/786#discussion_r4579173329> — #786 Fix rfl::DefaultVal in NamedTuples and TaggedUnion schemas (2026-11-15)
[^86]: <https://github.com/munich-data-engineering/reflect-cpp/pull/766#discussion_r4479036242> — #766 Expose the location of each parse failure via rfl::Error::details() (2026-11-05)
[^87]: <https://github.com/munich-data-engineering/reflect-cpp/pull/766#issuecomment-6655158829> — #766 Expose the location of each parse failure via rfl::Error::details() (2026-11-05)
[^88]: <https://github.com/munich-data-engineering/reflect-cpp/pull/662#issuecomment-4416085351> — #662 Add unified rfl::Settings::with<>() API for format-specific settings (2026-05-10)
[^89]: <https://github.com/munich-data-engineering/reflect-cpp/issues/482#issuecomment-3190822384> — #482 C++20 Modules? (2025-08-15)
[^90]: <https://github.com/munich-data-engineering/reflect-cpp/issues/592#issuecomment-3820550697> — #592 Issue with parsing string validation and default fields in the same nested struct (2026-01-29)
[^91]: <https://github.com/munich-data-engineering/reflect-cpp/issues/700#issuecomment-5051630483> — #700 Reading JSON into a struct with a `DefaultVal` field and a size validator field always raises a size validation error (2026-07-22)
[^92]: <https://github.com/munich-data-engineering/reflect-cpp/issues/685#issuecomment-4773255591> — #685 Cannot use rfl::DefaultVal and rfl::Field together in one strcut (2026-06-22)
[^93]: <https://github.com/munich-data-engineering/reflect-cpp/issues/683#issuecomment-4783241206> — #683 Automatic min and max in json schema for integers (2026-06-23)
[^94]: <https://github.com/munich-data-engineering/reflect-cpp/pull/515#discussion_r2422005380> — #515 Cleanup includes (2025-10-10)
[^95]: <https://github.com/munich-data-engineering/reflect-cpp/pull/515#discussion_r2422954698> — #515 Cleanup includes (2025-10-11)
[^96]: <https://github.com/munich-data-engineering/reflect-cpp/pull/606#issuecomment-3885185694> — #606 Improve invalid enum error message (2026-02-11)
[^97]: <https://github.com/munich-data-engineering/reflect-cpp/pull/582#issuecomment-3801631579> — #582 Fix some unused parameters warnings (2026-01-26)
[^98]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3774694481> — #578 Add support for C++ modules (2026-01-20)
[^99]: <https://github.com/munich-data-engineering/reflect-cpp/pull/613#discussion_r2815047219> — #613 Add CLI argument parser (rfl::cli::read) (2026-02-17)
[^100]: <https://github.com/munich-data-engineering/reflect-cpp/issues/684#issuecomment-4763193090> — #684 Request: Implement 'member-of-pointer' access operator (->) for Rfl types (2026-06-21)
[^101]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3773917724> — #578 Add support for C++ modules (2026-01-20)
[^102]: <https://github.com/munich-data-engineering/reflect-cpp/pull/584#issuecomment-3807344075> — #584 Support parsing arbitrary duration representations (2026-01-27)
[^103]: <https://github.com/munich-data-engineering/reflect-cpp/issues/609#issuecomment-3938631576> — #609 Bug or possible regression with custom rfl::Reflectors (2026-02-21)
[^104]: <https://github.com/munich-data-engineering/reflect-cpp/pull/466#discussion_r2217284568> — #466 Serialize/Deserialize `vector<char>` as bytestring (2025-07-19)
[^105]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3777497499> — #578 Add support for C++ modules (2026-01-21)
[^106]: <https://github.com/munich-data-engineering/reflect-cpp/pull/646#issuecomment-4184197435> — #646 boost src/ offloading (2026-04-03)
[^107]: <https://github.com/munich-data-engineering/reflect-cpp/pull/618#issuecomment-4005878901> — #618 Fix 30 bugs found during code review (2026-03-05)
[^108]: <https://github.com/munich-data-engineering/reflect-cpp/pull/726#issuecomment-5858112905> — #726 fix `CountFieldsHelper<T>` when a field is `std::optional<Eigen::Matrix4d>` (2026-09-27)
[^109]: <https://github.com/munich-data-engineering/reflect-cpp/pull/582#issuecomment-3808118426> — #582 Fix some unused parameters warnings (2026-01-27)
[^110]: <https://github.com/munich-data-engineering/reflect-cpp/pull/659#pullrequestreview-4259179735> — #659 Fix MkDocs deployment on public mkdocs-material (2026-05-10)
[^111]: <https://github.com/munich-data-engineering/reflect-cpp/issues/591#issuecomment-3818043658> — #591 Custom class `ReflectionType` cannot be directly in the class on Linux (2026-01-29)
[^112]: <https://github.com/munich-data-engineering/reflect-cpp/issues/661#issuecomment-4415424236> — #661 Deserialising std::map Enum Keys (2026-05-10)
[^113]: <https://github.com/munich-data-engineering/reflect-cpp/pull/485#issuecomment-3227286954> — #485 make read_one_alternative more general (2025-08-27)
[^114]: <https://github.com/munich-data-engineering/reflect-cpp/pull/581#issuecomment-3938731911> — #581 Make REFLECTCPP_USE_BUNDLED_DEPENDENCIES option consistent with enchantum (2026-02-21)
[^115]: <https://github.com/munich-data-engineering/reflect-cpp/pull/610#issuecomment-3938732656> — #610 Json Schema: Add enum value descriptions (2026-02-21)
[^116]: <https://github.com/munich-data-engineering/reflect-cpp/issues/470#issuecomment-3236997483> — #470 sqlgen cannot find reflectcpp-config.cmake (2025-08-29)
[^117]: <https://github.com/munich-data-engineering/reflect-cpp/issues/480#issuecomment-3476008974> — #480 Unable to generate build files for the YAML serializer (2025-11-01)
[^118]: <https://github.com/munich-data-engineering/reflect-cpp/pull/571#issuecomment-3734492165> — #571 Hotfix -DREFLECTCPP_USE_STD_EXPECTED=ON (2026-01-11)
[^119]: <https://github.com/munich-data-engineering/reflect-cpp/pull/674#issuecomment-4783213934> — #674 fix abort when default value does not satisfy Validator (2026-06-23)
[^120]: <https://github.com/munich-data-engineering/reflect-cpp/issues/470#issuecomment-3136438924> — #470 sqlgen cannot find reflectcpp-config.cmake (2025-07-30)
[^121]: <https://github.com/munich-data-engineering/reflect-cpp/issues/463#issuecomment-3071255153> — #463 Json Read rfl::UnderlyingEnum processor is broken (2025-07-14)
[^122]: <https://github.com/munich-data-engineering/reflect-cpp/issues/575#issuecomment-3765833451> — #575 Support std::atomic<T> fields (2026-01-18)
[^123]: <https://github.com/munich-data-engineering/reflect-cpp/pull/743#issuecomment-6250610047> — #743 Improve shared parser inlining and generated code layout (2026-10-18)
[^124]: <https://github.com/munich-data-engineering/reflect-cpp/pull/613#discussion_r2806700306> — #613 Add CLI argument parser (rfl::cli::read) (2026-02-14)
[^125]: <https://github.com/munich-data-engineering/reflect-cpp/pull/654#discussion_r3143529104> — #654 Feat/parse null into nullopt optionals (2026-04-26)
[^126]: <https://github.com/munich-data-engineering/reflect-cpp/pull/465#discussion_r2211645358> — #465 Make `rfl::num_fields` public (2025-07-16)
[^127]: <https://github.com/munich-data-engineering/reflect-cpp/pull/644#discussion_r3032721532> — #644 Make sure that flag enums with value zero are correctly serialized; fixes #640 (2026-04-03)
[^128]: <https://github.com/munich-data-engineering/reflect-cpp/pull/645#discussion_r3033074620> — #645 Make sure that optional inside commented works as expected; fixes #642 (2026-04-03)
[^129]: <https://github.com/munich-data-engineering/reflect-cpp/pull/664#discussion_r3231982749> — #664 Adapt Conan to support the new formats (2026-05-13)
[^130]: <https://github.com/munich-data-engineering/reflect-cpp/pull/618#issuecomment-3995463198> — #618 Fix 30 bugs found during code review (2026-03-04)
[^131]: <https://github.com/munich-data-engineering/reflect-cpp/pull/737#issuecomment-6157304107> — #737 Initialize structs using default values before parsing; resolves #719 and #700 (2026-10-13)
[^132]: <https://github.com/munich-data-engineering/reflect-cpp/issues/436#issuecomment-2895652651> — #436 rfl::toml::read() doesn't compile (cannot convert from 'toml::v3::noex::parse_result *' to 'rfl::toml::InputVarType') (2025-05-20)
[^133]: <https://github.com/munich-data-engineering/reflect-cpp/pull/568#issuecomment-3722737275> — #568 fix: CMake package and imported library target names (2026-01-08)
[^134]: <https://github.com/munich-data-engineering/reflect-cpp/pull/581#issuecomment-3796986846> — #581 Make REFLECTCPP_USE_BUNDLED_DEPENDENCIES option consistent with enchantum (2026-01-25)
[^135]: <https://github.com/munich-data-engineering/reflect-cpp/issues/551#issuecomment-3620235083> — #551 msgpack library name error for ubuntu (24.04 and earlier) (2025-12-06)
[^136]: <https://github.com/munich-data-engineering/reflect-cpp/pull/620#issuecomment-3985284149> — #620 Add CMake option for toggling strict warnings (2026-03-02)
[^137]: <https://github.com/munich-data-engineering/reflect-cpp/issues/622#issuecomment-3976692535> — #622 vcpkg build fail (2026-02-28)
[^138]: <https://github.com/munich-data-engineering/reflect-cpp/issues/576#issuecomment-3765555696> — #576 Provide CMake components for different serialization formats (2026-01-18)
[^139]: <https://github.com/munich-data-engineering/reflect-cpp/issues/616#issuecomment-3931487081> — #616 reflect-cpp in Fedora/EPEL (2026-02-20)
[^140]: <https://github.com/munich-data-engineering/reflect-cpp/issues/637#issuecomment-4115549228> — #637 C++26 reflection adoption (2026-03-24)
[^141]: <https://github.com/munich-data-engineering/reflect-cpp/issues/637#issuecomment-5416711147> — #637 C++26 reflection adoption (2026-08-25)
[^142]: <https://github.com/munich-data-engineering/reflect-cpp/issues/689#issuecomment-4901862496> — #689 Enum-Flag Default Values (2026-07-07)
[^143]: <https://github.com/munich-data-engineering/reflect-cpp/issues/719#issuecomment-6288222777> — #719 Segfault in rfl::json::read on GCC 12 -O2: read_struct builds its view over a T whose lifetime never began (2026-10-20)
[^144]: <https://github.com/munich-data-engineering/reflect-cpp/pull/537#issuecomment-3482959077> — #537 Speed up the reflection (2025-11-03)
[^145]: <https://github.com/munich-data-engineering/reflect-cpp/pull/537#issuecomment-3482985628> — #537 Speed up the reflection (2025-11-03)
[^146]: <https://github.com/munich-data-engineering/reflect-cpp/issues/473#issuecomment-3151838490> — #473 Question: Limit scope of reflection (2025-08-04)
[^147]: <https://github.com/munich-data-engineering/reflect-cpp/pull/538#issuecomment-3478316661> — #538 Avoid assuming that "std::string_view" is nullterminated. Generally it is really not. (2025-11-02)
[^148]: <https://github.com/munich-data-engineering/reflect-cpp/pull/743#issuecomment-6250198533> — #743 Improve shared parser inlining and generated code layout (2026-10-18)
[^149]: <https://github.com/munich-data-engineering/reflect-cpp/pull/738#issuecomment-6175960297> — #738 Avoid eager error allocation when parsing named tuples (2026-10-14)
[^150]: <https://github.com/munich-data-engineering/reflect-cpp/pull/742#issuecomment-6224835177> — #742 Inline frequently used parsing functions (2026-10-16)
[^151]: <https://github.com/munich-data-engineering/reflect-cpp/pull/740#issuecomment-6224736544> — #740 Optimize numeric vectors in Cereal, YAS, and Boost binary (2026-10-16)
[^152]: <https://github.com/munich-data-engineering/reflect-cpp/issues/594#issuecomment-3836193722> — #594 Add benchmark comparisons to other solutions in the README.md? (2026-02-02)
[^153]: <https://github.com/munich-data-engineering/reflect-cpp/pull/613#issuecomment-3900326453> — #613 Add CLI argument parser (rfl::cli::read) (2026-02-14)
[^154]: <https://github.com/munich-data-engineering/reflect-cpp/pull/613#pullrequestreview-3835490850> — #613 Add CLI argument parser (rfl::cli::read) (2026-02-21)
[^155]: <https://github.com/munich-data-engineering/reflect-cpp/issues/623#issuecomment-3976650777> — #623 Potential improvements to rfl::cli::read() API (2026-02-28)
[^156]: <https://github.com/munich-data-engineering/reflect-cpp/issues/692#issuecomment-5013198984> — #692 Feature Request: CLI usage/help generation (2026-07-18)
[^157]: <https://github.com/munich-data-engineering/reflect-cpp/issues/706#issuecomment-5096583892> — #706 [Feature Request] ref::cli::read wchar_t **argv[] support (2026-07-27)
[^158]: <https://github.com/munich-data-engineering/reflect-cpp/issues/706#issuecomment-5107111366> — #706 [Feature Request] ref::cli::read wchar_t **argv[] support (2026-07-28)
[^159]: <https://github.com/munich-data-engineering/reflect-cpp/pull/783#issuecomment-6926138698> — #783 Read a bare negative number such as `-5` as a positional argument in rfl::cli (2026-11-16)
[^160]: <https://github.com/munich-data-engineering/reflect-cpp/pull/785#issuecomment-6897351640> — #785 Write optional fields wrapped in rfl::Short or rfl::Positional (2026-11-15)
[^161]: <https://github.com/munich-data-engineering/reflect-cpp/pull/785#issuecomment-6926201156> — #785 Write optional fields wrapped in rfl::Short or rfl::Positional (2026-11-16)
[^162]: <https://github.com/munich-data-engineering/reflect-cpp/issues/552#issuecomment-3822490752> — #552 Better support for custom schemas (2026-01-30)
[^163]: <https://github.com/munich-data-engineering/reflect-cpp/issues/552#issuecomment-3822493608> — #552 Better support for custom schemas (2026-01-30)
[^164]: <https://github.com/munich-data-engineering/reflect-cpp/pull/466#issuecomment-3092276979> — #466 Serialize/Deserialize `vector<char>` as bytestring (2025-07-19)
[^165]: <https://github.com/munich-data-engineering/reflect-cpp/pull/680#issuecomment-4653479945> — #680 WIP: respects DefaultVal in schema and adds default value to schema. fixes #679, #396 (2026-06-08)
[^166]: <https://github.com/munich-data-engineering/reflect-cpp/pull/728#issuecomment-5858127531> — #728 emit `"additionalProperties": false` with `rfl::NoExtraFields` (2026-09-27)
[^167]: <https://github.com/munich-data-engineering/reflect-cpp/pull/728#issuecomment-6427123127> — #728 emit `"additionalProperties": false` with `rfl::NoExtraFields` (2026-10-26)
[^168]: <https://github.com/munich-data-engineering/reflect-cpp/issues/683#issuecomment-5062399330> — #683 Automatic min and max in json schema for integers (2026-07-23)
[^169]: <https://github.com/munich-data-engineering/reflect-cpp/pull/624#pullrequestreview-3864365435> — #624 Add rfl::Deprecated, which adds deprecated and deprecationMessage to the generated schema field. (2026-02-27)
[^170]: <https://github.com/munich-data-engineering/reflect-cpp/issues/559#issuecomment-3667551103> — #559 Embedded nulls dropped from JSON strings (2025-12-17)
[^171]: <https://github.com/munich-data-engineering/reflect-cpp/issues/559#issuecomment-3674772989> — #559 Embedded nulls dropped from JSON strings (2025-12-19)
[^172]: <https://github.com/munich-data-engineering/reflect-cpp/pull/746#issuecomment-6254524641> — #746 Reject out-of-range JSON integer values (2026-10-18)
[^173]: <https://github.com/munich-data-engineering/reflect-cpp/pull/770#issuecomment-6825506047> — #770 Support JSON bytestrings as Base64 strings (2026-11-12)
[^174]: <https://github.com/munich-data-engineering/reflect-cpp/issues/715#issuecomment-5384696180> — #715 YAML doesn't round-trip generics (2026-08-23)
[^175]: <https://github.com/munich-data-engineering/reflect-cpp/issues/715#issuecomment-5384778561> — #715 YAML doesn't round-trip generics (2026-08-23)
[^176]: <https://github.com/munich-data-engineering/reflect-cpp/issues/436#issuecomment-2896712005> — #436 rfl::toml::read() doesn't compile (cannot convert from 'toml::v3::noex::parse_result *' to 'rfl::toml::InputVarType') (2025-05-21)
[^177]: <https://github.com/munich-data-engineering/reflect-cpp/issues/564#issuecomment-3814143842> — #564 MSVC Toml std::array not working (2026-01-28)
[^178]: <https://github.com/munich-data-engineering/reflect-cpp/pull/727#issuecomment-5858098113> — #727 toml: allow reading integer as double (2026-09-27)
[^179]: <https://github.com/munich-data-engineering/reflect-cpp/issues/708#issuecomment-5095778455> — #708 MessagePack writer encodes C++ float values as float64 (2026-07-27)
[^180]: <https://github.com/munich-data-engineering/reflect-cpp/pull/710#issuecomment-5127839465> — #710 Preserve float width in MessagePack output (2026-07-30)
[^181]: <https://github.com/munich-data-engineering/reflect-cpp/issues/543#issuecomment-3543000639> — #543 Why would CBOR be slow in the benchmark? (2025-11-17)
[^182]: <https://github.com/munich-data-engineering/reflect-cpp/issues/486#issuecomment-3240342083> — #486 rfl::UUIDv4 field fails JSON parse in struct even for valid UUID (2025-08-31)
[^183]: <https://github.com/munich-data-engineering/reflect-cpp/issues/482#issuecomment-3288135449> — #482 C++20 Modules? (2025-09-13)
[^184]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3801645060> — #578 Add support for C++ modules (2026-01-26)
[^185]: <https://github.com/munich-data-engineering/reflect-cpp/pull/669#issuecomment-4485460998> — #669 Add C++20 modules support and format btw. (2026-05-19)
[^186]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-5517030300> — #578 Add support for C++ modules (2026-09-02)
[^187]: <https://github.com/munich-data-engineering/reflect-cpp/issues/454#issuecomment-3042608420> — #454 Support Deserialize std::string_view (2025-07-06)
[^188]: <https://github.com/munich-data-engineering/reflect-cpp/issues/454#issuecomment-3047551724> — #454 Support Deserialize std::string_view (2025-07-08)
[^189]: <https://github.com/munich-data-engineering/reflect-cpp/pull/620> — #620 Add CMake option for toggling strict warnings (2026-03-16) (PR, merged)
[^190]: <https://github.com/munich-data-engineering/reflect-cpp/pull/547#issuecomment-3568141826> — #547 unused variables warnings (2025-11-23)
[^191]: <https://github.com/munich-data-engineering/reflect-cpp/pull/582#issuecomment-3800705868> — #582 Fix some unused parameters warnings (2026-01-26) (comment by makekryl)
[^192]: <https://github.com/munich-data-engineering/reflect-cpp/issues/719#issuecomment-6250487312> — #719 Segfault in rfl::json::read on GCC 12 -O2: read_struct builds its view over a T whose lifetime never began (2026-10-18)
[^193]: <https://github.com/munich-data-engineering/reflect-cpp/pull/752#issuecomment-6384075398> — #752 Work around GCC 12 optional parsing crash with native tuple storage (2026-10-24)
[^194]: <https://github.com/munich-data-engineering/reflect-cpp/pull/441#issuecomment-2920599342> — #441 Add `enchantum` as an enum reflection backend (2025-05-29)
[^195]: <https://github.com/munich-data-engineering/reflect-cpp/issues/455#issuecomment-3035300942> — #455 Feature request: Allow a 'fallback' type for tagged union (2025-07-04)
[^196]: <https://github.com/munich-data-engineering/reflect-cpp/issues/459#issuecomment-3071255856> — #459 Treat `std::vector<char>` similar to `rfl::Bytestring` (2025-07-14)
[^197]: <https://github.com/munich-data-engineering/reflect-cpp/pull/466#issuecomment-3094441075> — #466 Serialize/Deserialize `vector<char>` as bytestring (2025-07-20)
[^198]: <https://github.com/munich-data-engineering/reflect-cpp/pull/494#issuecomment-3275795858> — #494 Add `rfl::AddNamespacedTagsToVariants` for `rfl::Generic` and same-name-different-namespace structs (2025-09-10)
[^199]: <https://github.com/munich-data-engineering/reflect-cpp/issues/504#issuecomment-3335479745> — #504 [Proposal] Error.what() rvalue move return (2025-09-25)
[^200]: <https://github.com/munich-data-engineering/reflect-cpp/issues/575#issuecomment-3763025843> — #575 Support std::atomic<T> fields (2026-01-17)
[^201]: <https://github.com/munich-data-engineering/reflect-cpp/issues/575#issuecomment-3765565871> — #575 Support std::atomic<T> fields (2026-01-18)
[^202]: <https://github.com/munich-data-engineering/reflect-cpp/pull/582#issuecomment-3807716865> — #582 Fix some unused parameters warnings (2026-01-27)
[^203]: <https://github.com/munich-data-engineering/reflect-cpp/issues/597#issuecomment-3850464965> — #597 Feature Request: Add support for serializing comments (2026-02-05)
[^204]: <https://github.com/munich-data-engineering/reflect-cpp/pull/618#issuecomment-4032408676> — #618 Fix 30 bugs found during code review (2026-03-10)
[^205]: <https://github.com/munich-data-engineering/reflect-cpp/issues/636#issuecomment-4115767904> — #636 Boost serialisation support (2026-03-24)
[^206]: <https://github.com/munich-data-engineering/reflect-cpp/pull/641#issuecomment-4183770423> — #641 Boost serialization implementation (2026-04-03)
[^207]: <https://github.com/munich-data-engineering/reflect-cpp/pull/680#issuecomment-4711655494> — #680 WIP: respects DefaultVal in schema and adds default value to schema. fixes #679, #396 (2026-06-15)
[^208]: <https://github.com/munich-data-engineering/reflect-cpp/issues/684#issuecomment-5052053689> — #684 Request: Implement 'member-of-pointer' access operator (->) for Rfl types (2026-07-22)
[^209]: <https://github.com/munich-data-engineering/reflect-cpp/issues/693#issuecomment-5013200441> — #693 Equality operator for rfl::Generic and rfl::Object (2026-07-18)
[^210]: <https://github.com/munich-data-engineering/reflect-cpp/pull/696#issuecomment-5027543893> — #696 Add operator== for rfl::Generic and rfl::Object (2026-07-20)
[^211]: <https://github.com/munich-data-engineering/reflect-cpp/pull/707#issuecomment-5204408626> — #707 Allow validated fields to work alongside defaulted ones (2026-08-06)
[^212]: <https://github.com/munich-data-engineering/reflect-cpp/issues/714#issuecomment-5416681141> — #714 Enum can be deserialized from raw integers that don't map to enum values (2026-08-25)
[^213]: <https://github.com/munich-data-engineering/reflect-cpp/issues/716#issuecomment-5384716898> — #716 Support serialising std::expected (2026-08-23)
[^214]: <https://github.com/munich-data-engineering/reflect-cpp/pull/720#issuecomment-5462306887> — #720 Reject externally tagged variants with more than one field (2026-08-29)
[^215]: <https://github.com/munich-data-engineering/reflect-cpp/pull/720#issuecomment-5464954581> — #720 Reject externally tagged variants with more than one field (2026-08-29)
[^216]: <https://github.com/munich-data-engineering/reflect-cpp/pull/729#issuecomment-5858102933> — #729 add `from_reflection(ReflectionType&&)` as alternative to ctor (2026-09-27)
[^217]: <https://github.com/munich-data-engineering/reflect-cpp/pull/743#issuecomment-6301368786> — #743 Improve shared parser inlining and generated code layout (2026-10-20)
[^218]: <https://github.com/munich-data-engineering/reflect-cpp/pull/766#issuecomment-6713364820> — #766 Expose the location of each parse failure via rfl::Error::details() (2026-11-07)
[^219]: <https://github.com/munich-data-engineering/reflect-cpp/pull/767#issuecomment-6713277803> — #767 Avoid needless copies, allocations and exceptions when reading and writing (2026-11-07)
[^220]: <https://github.com/munich-data-engineering/reflect-cpp/pull/769#issuecomment-6713390166> — #769 Make rfl::CopyableBox copy-assignable (2026-11-07)
[^221]: <https://github.com/munich-data-engineering/reflect-cpp/pull/776#issuecomment-6837641939> — #776 Return an error when an rfl::Skip field's default value throws (2026-11-12)
[^222]: <https://github.com/munich-data-engineering/reflect-cpp/pull/778#issuecomment-6837804149> — #778 Let DefaultVal's converting constructor throw (2026-11-12)
[^223]: <https://github.com/munich-data-engineering/reflect-cpp/pull/779#issuecomment-6839522261> — #779 Say why default construction failed when reading a struct with defaults (2026-11-12)
[^224]: <https://github.com/munich-data-engineering/reflect-cpp/pull/782#issuecomment-6897308519> — #782 Update a DefaultVal test to the error message from #779 (2026-11-15)
[^225]: <https://github.com/munich-data-engineering/reflect-cpp/pull/787#issuecomment-6926532431> — #787 Don't assign to const fields when reading a struct with defaults (2026-11-16)
[^226]: <https://github.com/munich-data-engineering/reflect-cpp/pull/786#discussion_r4595094739> — #786 Fix rfl::DefaultVal in NamedTuples and TaggedUnion schemas (2026-11-17) (comment by clockblocker)
