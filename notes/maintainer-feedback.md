---
title: Maintainer comments (liuzicheng1987), collected
created: 2026-11-17
period: 2025-05-17 → 2026-11-17
source: github.com/munich-data-engineering/reflect-cpp
tags: [reflect-cpp, maintainer, review]
stage: collected
---
> AI-collected from public GitHub comments; all text verbatim. Not yet grouped.

# Maintainer comments, collected

Every comment by liuzicheng1987 on munich-data-engineering/reflect-cpp (formerly getml/reflect-cpp) from 2025-05-17 to 2026-11-17: 469 issue and PR conversation comments, 47 inline review comments, and 4 review summaries with a body (44 more reviews had empty bodies and only carry the inline comments). That is 520 entries in 238 threads. GitHub Discussions are turned off for the repo. Threads are ordered by the maintainer's first comment in them. Pure acknowledgements are one-line entries. `Replying to:` lines quote the start of the comment being answered. They are picked automatically: the most recent non-bot comment since the maintainer's previous one, preferring a user the maintainer @-mentions, and the opening post if the maintainer hadn't commented in the thread before.

## Index

| # | Title | Kind | Comments | First | Last |
|---|---|---|---|---|---|
| 435 | Serialising anything other than simple types fails to compile | Issue | 2 | 2025-05-20 | 2025-05-20 |
| 436 | rfl::toml::read() doesn't compile (cannot convert from 'toml::v3::noex::parse_result *' to 'rfl::toml::InputVarType') | Issue | 3 | 2025-05-20 | 2025-06-09 |
| 439 | Field value of named tuple was moved after apply | Issue | 3 | 2025-05-26 | 2025-05-27 |
| 440 | Made sure that apply can never move or modify the tuple values; fixes… | PR | 1 | 2025-05-28 | 2025-05-28 |
| 441 | Add `enchantum` as an enum reflection backend | PR | 6 | 2025-05-29 | 2025-05-29 |
| 442 | Fix cbor encoding for uint64_t's | PR | 1 | 2025-05-31 | 2025-05-31 |
| 443 | Suggestion: Deep comparison for `rfl::to_view()` | Issue | 5 | 2025-06-05 | 2025-06-06 |
| 445 | Improved support for string_views for TOML and XML | PR | 4 | 2025-06-08 | 2025-06-08 |
| 444 | Don't use `std::string_view` with APIs expecting null-terminated strings | Issue | 1 | 2025-06-08 | 2025-06-08 |
| 447 | Added support for exception-free TOML; resolves #436 | PR | 2 | 2025-06-09 | 2025-06-09 |
| 448 | Dont repeat class names template arguements | PR | 1 | 2025-06-10 | 2025-06-10 |
| 449 | Flatten<TaggedUnion<...>> isn't flattened but instead becomes a "variant_" implicit field. | Issue | 1 | 2025-06-12 | 2025-06-12 |
| 451 | Reflect on the number of fields of a too-complex type at compile time? | Issue | 2 | 2025-06-17 | 2025-06-19 |
| 452 | Are there any examples on how to serialize / deserialize a json tree? | Issue | 3 | 2025-06-19 | 2025-06-20 |
| 453 | added REFLECTCPP_INSTALL to allow user to control if required | PR | 1 | 2025-07-04 | 2025-07-04 |
| 455 | Feature request: Allow a 'fallback' type for tagged union | Issue | 1 | 2025-07-04 | 2025-07-04 |
| 454 | Support Deserialize std::string_view | Issue | 2 | 2025-07-06 | 2025-07-08 |
| 456 | rename _res to _result to avoid macro conflict | PR | 1 | 2025-07-08 | 2025-07-08 |
| 457 | Support for `std::span` | Issue | 1 | 2025-07-11 | 2025-07-11 |
| 463 | Json Read rfl::UnderlyingEnum processor is broken | Issue | 5 | 2025-07-14 | 2025-11-01 |
| 459 | Treat `std::vector<char>` similar to `rfl::Bytestring` | Issue | 1 | 2025-07-14 | 2025-07-14 |
| 462 | cbor supports std::optional; fixes #135 | PR | 2 | 2025-07-16 | 2025-07-16 |
| 465 | Make `rfl::num_fields` public | PR | 2 | 2025-07-16 | 2025-07-16 |
| 464 | Fix bug reading with 'tagged' variants when a variant is non-copyable | PR | 1 | 2025-07-16 | 2025-07-16 |
| 466 | Serialize/Deserialize `vector<char>` as bytestring | PR | 5 | 2025-07-17 | 2025-07-20 |
| 468 | Added support for std::span; resolves #457 | PR | 1 | 2025-07-20 | 2025-07-20 |
| 469 | Add support for reading from a byte-container for cbor and ubjson | PR | 1 | 2025-07-26 | 2025-07-26 |
| 470 | sqlgen cannot find reflectcpp-config.cmake | Issue | 5 | 2025-07-28 | 2025-08-29 |
| 429 | Support CBOR parsing using error codes | Issue | 1 | 2025-07-28 | 2025-07-28 |
| 472 | Multiple serialization strategies for a custom non-struct type | Issue | 3 | 2025-07-30 | 2025-07-31 |
| 473 | Question: Limit scope of reflection | Issue | 2 | 2025-08-04 | 2025-08-04 |
| 475 | Using Field in combination with validation | Issue | 2 | 2025-08-07 | 2025-08-07 |
| 480 | Unable to generate build files for the YAML serializer | Issue | 5 | 2025-08-12 | 2025-11-01 |
| 479 | Add interfaces for jwt-cpp compatibility | PR | 1 | 2025-08-12 | 2025-08-12 |
| 482 | C++20 Modules? | Issue | 2 | 2025-08-15 | 2025-09-13 |
| 481 | Externally tagged unions as fields | Issue | 2 | 2025-08-15 | 2025-08-15 |
| 484 | Better forwarding to improve read performance a little | PR | 4 | 2025-08-20 | 2025-09-01 |
| 485 | make read_one_alternative more general | PR | 2 | 2025-08-27 | 2025-08-27 |
| 486 | rfl::UUIDv4 field fails JSON parse in struct even for valid UUID | Issue | 6 | 2025-08-31 | 2025-08-31 |
| 467 | Allow using `rfl::AddTagsToVariants` with `rfl::Generic` and same-name-different-namespace structs | PR | 4 | 2025-09-02 | 2025-09-14 |
| 493 | Can't add vector into extrafields ? | Issue | 1 | 2025-09-08 | 2025-09-08 |
| 495 | "May be used uninitialized" issues reported by GCC 15.2.0 | Issue | 6 | 2025-09-10 | 2025-10-13 |
| 494 | Add `rfl::AddNamespacedTagsToVariants` for `rfl::Generic` and same-name-different-namespace structs | PR | 4 | 2025-09-10 | 2025-09-11 |
| 496 | the demo runtime exception with ctre | Issue | 2 | 2025-09-12 | 2025-09-13 |
| 497 | Update vcpkg | PR | 2 | 2025-09-13 | 2025-09-13 |
| 499 | how to retrive object from variant and taggedunion? | Issue | 1 | 2025-09-13 | 2025-09-13 |
| 502 | Re-add pyproject.tml to fix unknown env error | PR | 1 | 2025-09-15 | 2025-09-15 |
| 503 | Using `Description` marks optionals as required in JSON Schema | Issue | 1 | 2025-09-23 | 2025-09-23 |
| 504 | [Proposal] Error.what() rvalue move return | Issue | 1 | 2025-09-25 | 2025-09-25 |
| 505 | added explicit ref-qualifiers for Error::what() and a additional over… | PR | 1 | 2025-09-29 | 2025-09-29 |
| 507 | use target_compile_definitions for REFLECTCPP_USE_STD_EXPECTED instea… | PR | 1 | 2025-09-30 | 2025-09-30 |
| 514 | clang 20 warnings | PR | 1 | 2025-10-08 | 2025-10-08 |
| 509 | Json Schema with Reflector type | Issue | 2 | 2025-10-08 | 2025-10-19 |
| 515 | Cleanup includes | PR | 6 | 2025-10-10 | 2025-10-11 |
| 519 | Cleanup includes | PR | 1 | 2025-10-13 | 2025-10-13 |
| 521 | Allow specify schema in capnproto's stream interface | PR | 1 | 2025-10-13 | 2025-10-13 |
| 516 | Silent errors, can terminate application | Issue | 2 | 2025-10-13 | 2025-10-25 |
| 522 | Avro serialization for floating point and integer types failing | Issue | 2 | 2025-10-14 | 2025-10-25 |
| 520 | `reflect-cpp` does not generate `.lib` file when building as `shared` on Windows | Issue | 5 | 2025-10-14 | 2025-10-19 |
| 523 | [json schema] Use non-deprecated $defs; Add $comment option | PR | 2 | 2025-10-15 | 2025-10-15 |
| 524 | Please bump vendored yyjson to latest (0.12.0) and fix related tests | Issue | 1 | 2025-10-16 | 2025-10-16 |
| 525 | Feature/shared exports msvc | PR | 1 | 2025-10-19 | 2025-10-19 |
| 529 | [minor] Fix include error of `rfl/Literal.hpp` | PR | 1 | 2025-10-20 | 2025-10-20 |
| 528 | Missing Includes | Issue | 2 | 2025-10-20 | 2025-10-20 |
| 530 | Issues with deserialization of fixarray using msgpack with reflectcpp 0.21 | Issue | 1 | 2025-10-24 | 2025-10-24 |
| 535 | Use exceptions inside the writers | PR | 1 | 2025-10-27 | 2025-10-27 |
| 534 | Add `rfl::Default` | Issue | 1 | 2025-10-27 | 2025-10-27 |
| 538 | Avoid assuming that "std::string_view" is nullterminated. Generally it is really not. | PR | 5 | 2025-11-02 | 2025-11-02 |
| 537 | Speed up the reflection | PR | 3 | 2025-11-03 | 2025-11-03 |
| 541 | Fix missing "hours" when parsing std::chrono::duration | PR | 1 | 2025-11-13 | 2025-11-13 |
| 542 | rfl::io::save_string fails silently | Issue | 1 | 2025-11-14 | 2025-11-14 |
| 543 | Why would CBOR be slow in the benchmark? | Issue | 2 | 2025-11-17 | 2025-11-17 |
| 549 | `rfl::Generic`: Allow `int64_t` -> `double` conversion in `to_double` | PR | 2 | 2025-11-23 | 2025-11-23 |
| 547 | unused variables warnings | PR | 3 | 2025-11-23 | 2025-11-23 |
| 551 | msgpack library name error for ubuntu (24.04 and earlier) | Issue | 2 | 2025-12-03 | 2025-12-06 |
| 557 | Any idea why including reflect.h in MSVC results in internal compiler error? | Issue | 3 | 2025-12-13 | 2025-12-13 |
| 559 | Embedded nulls dropped from JSON strings | Issue | 2 | 2025-12-17 | 2025-12-19 |
| 560 | [Compilation Error] Error when including <windows.h> before <rfl/Result.hpp> | Issue | 2 | 2025-12-17 | 2025-12-20 |
| 561 | Support std::map<..., std::less<>> (transparent comparator) in JSON read/write | Issue | 2 | 2025-12-21 | 2026-01-28 |
| 558 | Improve bad access error for std::expected<T, rfl::Error> | PR | 1 | 2025-12-22 | 2025-12-22 |
| 565 | string_to_enum does not support std::string_view | Issue | 1 | 2025-12-26 | 2025-12-26 |
| 567 | Compatibility between TaggedUnion and NoExtraFields | Issue | 2 | 2026-01-03 | 2026-01-11 |
| 568 | fix: CMake package and imported library target names | PR | 5 | 2026-01-04 | 2026-02-22 |
| 569 | Fix missing space in static_assert | PR | 1 | 2026-01-09 | 2026-01-09 |
| 571 | Hotfix -DREFLECTCPP_USE_STD_EXPECTED=ON | PR | 2 | 2026-01-09 | 2026-01-11 |
| 570 | Compilation Error with REFLECTCPP_USE_STD_EXPECTED=ON | Issue | 2 | 2026-01-10 | 2026-01-11 |
| 572 | Make sure that GCC 13 also compilies with std::expected; resolves #570 | PR | 1 | 2026-01-11 | 2026-01-11 |
| 574 | Enable building as a native ROS package | PR | 1 | 2026-01-14 | 2026-01-14 |
| 575 | Support std::atomic<T> fields | Issue | 6 | 2026-01-17 | 2026-01-18 |
| 576 | Provide CMake components for different serialization formats | Issue | 2 | 2026-01-18 | 2026-01-19 |
| 578 | Add support for C++ modules | PR | 24 | 2026-01-19 | 2026-09-02 |
| 580 | build(cmake): add and install cmake version file | PR | 1 | 2026-01-24 | 2026-01-24 |
| 581 | Make REFLECTCPP_USE_BUNDLED_DEPENDENCIES option consistent with enchantum | PR | 5 | 2026-01-25 | 2026-02-21 |
| 582 | Fix some unused parameters warnings | PR | 4 | 2026-01-26 | 2026-01-27 |
| 584 | Support parsing arbitrary duration representations | PR | 3 | 2026-01-26 | 2026-01-27 |
| 587 | Fix `rfl::DefaultVal` with `rfl::Rename` | Issue | 2 | 2026-01-27 | 2026-01-27 |
| 588 | Add camel case to snake case conversion | PR | 4 | 2026-01-27 | 2026-01-28 |
| 564 | MSVC Toml std::array not working | Issue | 1 | 2026-01-28 | 2026-01-28 |
| 591 | Custom class `ReflectionType` cannot be directly in the class on Linux | Issue | 1 | 2026-01-29 | 2026-01-29 |
| 592 | Issue with parsing string validation and default fields in the same nested struct | Issue | 2 | 2026-01-29 | 2026-01-29 |
| 552 | Better support for custom schemas | Issue | 3 | 2026-01-29 | 2026-01-30 |
| 594 | Add benchmark comparisons to other solutions in the README.md? | Issue | 2 | 2026-02-02 | 2026-02-02 |
| 595 | Inconsistency in rfl::Variant visit overloads prevents stateful visitors | Issue | 1 | 2026-02-03 | 2026-02-03 |
| 596 | Allow usage of stateful visitors inside rfl::Variant | PR | 1 | 2026-02-04 | 2026-02-04 |
| 597 | Feature Request: Add support for serializing comments | Issue | 4 | 2026-02-05 | 2026-02-08 |
| 598 | Update error message for unused values | Issue | 2 | 2026-02-05 | 2026-02-05 |
| 599 | Remove reference to internals in error message for unused values | PR | 1 | 2026-02-07 | 2026-02-07 |
| 607 | Set file name and license in RPM CPack options | PR | 1 | 2026-02-10 | 2026-02-10 |
| 608 | Why not give a default constructor to TaggedUnion? | Issue | 1 | 2026-02-11 | 2026-02-11 |
| 606 | Improve invalid enum error message | PR | 1 | 2026-02-11 | 2026-02-11 |
| 610 | Json Schema: Add enum value descriptions | PR | 2 | 2026-02-11 | 2026-02-21 |
| 609 | Bug or possible regression with custom rfl::Reflectors | Issue | 2 | 2026-02-12 | 2026-02-21 |
| 613 | Add CLI argument parser (rfl::cli::read) | PR | 8 | 2026-02-14 | 2026-02-21 |
| 614 | Can't parse XML into a rfl::Generic | Issue | 2 | 2026-02-16 | 2026-03-07 |
| 615 | std::atomic<T> Reflector bypasses domain validation present on bare T (#575 followup) | Issue | 4 | 2026-02-19 | 2026-02-21 |
| 616 | reflect-cpp in Fedora/EPEL | Issue | 1 | 2026-02-20 | 2026-02-20 |
| 619 | Question: When using rfl::Variant with the msgpack processor, is AddTagsToVariants nessasary? | Issue | 1 | 2026-02-23 | 2026-02-23 |
| 621 | Allows usage of custom Reflector for enums types | PR | 2 | 2026-02-24 | 2026-02-25 |
| 618 | Fix 30 bugs found during code review | PR | 7 | 2026-02-24 | 2026-03-11 |
| 622 | vcpkg build fail | Issue | 2 | 2026-02-25 | 2026-02-28 |
| 624 | Add rfl::Deprecated, which adds deprecated and deprecationMessage to the generated schema field. | PR | 2 | 2026-02-27 | 2026-02-27 |
| 623 | Potential improvements to rfl::cli::read() API | Issue | 1 | 2026-02-28 | 2026-02-28 |
| 620 | Add CMake option for toggling strict warnings | PR | 2 | 2026-03-02 | 2026-03-16 |
| 626 | json schema: Add enum value descriptions | PR | 2 | 2026-03-05 | 2026-03-16 |
| 625 | Docs: update with rfl::Deprecated | PR | 1 | 2026-03-05 | 2026-03-05 |
| 627 | [msgpack] Fixes missing parameter comments, error message | PR | 1 | 2026-03-07 | 2026-03-07 |
| 629 | "May be used uninitialized" warning from g++ 15.2.1 with -O2 | Issue | 2 | 2026-03-08 | 2026-03-08 |
| 632 | Fix "Error()" function. | PR | 2 | 2026-03-10 | 2026-03-13 |
| 631 | Custom validators | Issue | 1 | 2026-03-12 | 2026-03-12 |
| 637 | C++26 reflection adoption | Issue | 2 | 2026-03-24 | 2026-08-25 |
| 634 | Fix capnproto add_value_to_map overwriting all entries at index 0 | PR | 2 | 2026-03-24 | 2026-03-24 |
| 635 | Fix Cap'n Proto serialization of unsigned integer types | PR | 1 | 2026-03-24 | 2026-03-24 |
| 636 | Boost serialisation support | Issue | 1 | 2026-03-24 | 2026-03-24 |
| 638 | Delete MR_DESCRIPTION.md | PR | 1 | 2026-03-24 | 2026-03-24 |
| 639 | How to work with reference with a static lifetime | Issue | 1 | 2026-03-26 | 2026-03-26 |
| 641 | Boost serialization implementation | PR | 7 | 2026-03-28 | 2026-04-03 |
| 640 | Problem (de-)serializing an enum with value 0 if the \| operator is overloaded | Issue | 4 | 2026-03-30 | 2026-04-03 |
| 642 | Attempting to serialize an rfl::Commented<std::optional<std::array<>>> fails when the value is null | Issue | 2 | 2026-03-30 | 2026-04-03 |
| 644 | Make sure that flag enums with value zero are correctly serialized; fixes #640 | PR | 1 | 2026-04-03 | 2026-04-03 |
| 645 | Make sure that optional inside commented works as expected; fixes #642 | PR | 1 | 2026-04-03 | 2026-04-03 |
| 646 | boost src/ offloading | PR | 1 | 2026-04-03 | 2026-04-03 |
| 586 | Add serialization for std::chrono::system_clock::time_point | Issue | 1 | 2026-04-04 | 2026-04-04 |
| 648 | Parser for chrono::time_point | PR | 1 | 2026-04-05 | 2026-04-05 |
| 651 | Option to disable toml indentation | Issue | 1 | 2026-04-14 | 2026-04-14 |
| 653 | [bug] rfl::Processors seems broken | Issue | 2 | 2026-04-16 | 2026-04-16 |
| 652 | Why not allowing to parse null as std::optional in yaml files via `is_empty(null) -> true` | Issue | 1 | 2026-04-16 | 2026-04-16 |
| 655 | Redesign the way processors are identified; fixes #653 | PR | 1 | 2026-04-26 | 2026-04-26 |
| 654 | Feat/parse null into nullopt optionals | PR | 4 | 2026-04-26 | 2026-04-27 |
| 657 | User controllable multiline string serialization for YAML | PR | 1 | 2026-05-03 | 2026-05-03 |
| 658 | clang-tidy & MSVC: Duplicate field names are not allowed | Issue | 1 | 2026-05-06 | 2026-05-06 |
| 659 | Fix MkDocs deployment on public mkdocs-material | PR | 1 | 2026-05-10 | 2026-05-10 |
| 662 | Add unified rfl::Settings::with<>() API for format-specific settings | PR | 3 | 2026-05-10 | 2026-05-10 |
| 661 | Deserialising std::map Enum Keys | Issue | 1 | 2026-05-10 | 2026-05-10 |
| 664 | Adapt Conan to support the new formats | PR | 1 | 2026-05-13 | 2026-05-13 |
| 667 | Version 0.25.0 | PR | 1 | 2026-05-14 | 2026-05-14 |
| 665 | rfl::to_generic() generates MSVC warning C4702 (unreachable code) | Issue | 1 | 2026-05-14 | 2026-05-14 |
| 668 | fix(json): don't crash on non-ASCII std::wstring / std::filesystem::path | PR | 4 | 2026-05-14 | 2026-05-15 |
| 669 | Add C++20 modules support and format btw. | PR | 7 | 2026-05-19 | 2026-07-19 |
| 671 | Fix struct Person definition in json_schema.md | PR | 1 | 2026-05-26 | 2026-05-26 |
| 672 | Generic::to_*() enhancements | PR | 1 | 2026-05-31 | 2026-05-31 |
| 673 | Exception thrown in noexcept code | Issue | 3 | 2026-06-03 | 2026-06-23 |
| 674 | fix abort when default value does not satisfy Validator | PR | 5 | 2026-06-04 | 2026-06-23 |
| 677 | `rfl::json::to_schema<T, rfl::AddTagsToVariants>` fails to compile for variants of plain structs | Issue | 1 | 2026-06-07 | 2026-06-07 |
| 679 | rfl::DefaultVal's are still required in the json schema | Issue | 1 | 2026-06-08 | 2026-06-08 |
| 680 | WIP: respects DefaultVal in schema and adds default value to schema. fixes #679, #396 | PR | 7 | 2026-06-08 | 2026-06-15 |
| 684 | Request: Implement 'member-of-pointer' access operator (->) for Rfl types | Issue | 4 | 2026-06-21 | 2026-07-26 |
| 685 | Cannot use rfl::DefaultVal and rfl::Field together in one strcut | Issue | 2 | 2026-06-21 | 2026-06-22 |
| 683 | Automatic min and max in json schema for integers | Issue | 2 | 2026-06-23 | 2026-07-23 |
| 687 | no internal linkage in headers | PR | 1 | 2026-06-24 | 2026-06-24 |
| 689 | Enum-Flag Default Values | Issue | 5 | 2026-07-03 | 2026-08-25 |
| 688 | Avoid clang 21/22 C++26-mode frontend crash in StringLiteral's variadic constructor | PR | 3 | 2026-07-04 | 2026-07-20 |
| 691 | fix(tests): read cbor byte view via std::span, not basic_string_view<std::byte> | PR | 2 | 2026-07-17 | 2026-07-19 |
| 692 | Feature Request: CLI usage/help generation | Issue | 1 | 2026-07-18 | 2026-07-18 |
| 693 | Equality operator for rfl::Generic and rfl::Object | Issue | 1 | 2026-07-18 | 2026-07-18 |
| 694 | Added support for environment variables | PR | 8 | 2026-07-19 | 2026-07-19 |
| 696 | Add operator== for rfl::Generic and rfl::Object | PR | 1 | 2026-07-20 | 2026-07-20 |
| 690 | FR: Deserialization to std::set with custom comparator | Issue | 2 | 2026-07-21 | 2026-07-21 |
| 700 | Reading JSON into a struct with a `DefaultVal` field and a size validator field always raises a size validation error | Issue | 2 | 2026-07-22 | 2026-10-25 |
| 699 | Compilation error on Clang 15.0.0 | Issue | 2 | 2026-07-22 | 2026-07-22 |
| 707 | Allow validated fields to work alongside defaulted ones | PR | 2 | 2026-07-26 | 2026-08-06 |
| 708 | MessagePack writer encodes C++ float values as float64 | Issue | 1 | 2026-07-27 | 2026-07-27 |
| 706 | [Feature Request] ref::cli::read wchar_t **argv[] support | Issue | 4 | 2026-07-27 | 2026-07-28 |
| 710 | Preserve float width in MessagePack output | PR | 1 | 2026-07-30 | 2026-07-30 |
| 712 | rfl::to_view() does not seem to actually support being run at compile-time | Issue | 2 | 2026-08-17 | 2026-08-19 |
| 714 | Enum can be deserialized from raw integers that don't map to enum values | Issue | 2 | 2026-08-22 | 2026-08-25 |
| 715 | YAML doesn't round-trip generics | Issue | 2 | 2026-08-23 | 2026-08-23 |
| 716 | Support serialising std::expected | Issue | 3 | 2026-08-23 | 2026-08-25 |
| 721 | Output zero flag name if available | PR | 1 | 2026-08-29 | 2026-08-29 |
| 720 | Reject externally tagged variants with more than one field | PR | 3 | 2026-08-29 | 2026-08-29 |
| 723 | Support native non-string map keys for formats that allow them | Issue | 1 | 2026-09-07 | 2026-09-07 |
| 719 | Segfault in rfl::json::read on GCC 12 -O2: read_struct builds its view over a T whose lifetime never began | Issue | 5 | 2026-09-07 | 2026-10-20 |
| 724 | Add support for `std::inplace_vector` | Issue | 1 | 2026-09-14 | 2026-09-14 |
| 725 | fix `Generic.cpp` build error due to missing includes | PR | 1 | 2026-09-21 | 2026-09-21 |
| 727 | toml: allow reading integer as double | PR | 1 | 2026-09-27 | 2026-09-27 |
| 729 | add `from_reflection(ReflectionType&&)` as alternative to ctor | PR | 1 | 2026-09-27 | 2026-09-27 |
| 726 | fix `CountFieldsHelper<T>` when a field is `std::optional<Eigen::Matrix4d>` | PR | 2 | 2026-09-27 | 2026-09-28 |
| 728 | emit `"additionalProperties": false` with `rfl::NoExtraFields` | PR | 2 | 2026-09-27 | 2026-10-26 |
| 730 | fix `NamedTupleParser::to_schema()` for non-`DefaultVal` fields | PR | 1 | 2026-09-30 | 2026-09-30 |
| 731 | Add ability to rename enum values | Issue | 1 | 2026-09-30 | 2026-09-30 |
| 733 | Add inplace vector support | PR | 1 | 2026-10-09 | 2026-10-09 |
| 735 | Add DurationsAsCount preprocessor | PR | 1 | 2026-10-09 | 2026-10-09 |
| 736 | Reject trailing characters when parsing rfl::Timestamp | PR | 3 | 2026-10-09 | 2026-10-11 |
| 737 | Initialize structs using default values before parsing; resolves #719 and #700 | PR | 1 | 2026-10-13 | 2026-10-13 |
| 739 | Reserve vector capacity when array lengths are known | PR | 2 | 2026-10-14 | 2026-10-16 |
| 738 | Avoid eager error allocation when parsing named tuples | PR | 1 | 2026-10-14 | 2026-10-14 |
| 740 | Optimize numeric vectors in Cereal, YAS, and Boost binary | PR | 1 | 2026-10-16 | 2026-10-16 |
| 741 | Optimize positional field dispatch with compile-time indices | PR | 1 | 2026-10-16 | 2026-10-16 |
| 742 | Inline frequently used parsing functions | PR | 1 | 2026-10-16 | 2026-10-16 |
| 743 | Improve shared parser inlining and generated code layout | PR | 5 | 2026-10-18 | 2026-10-20 |
| 744 | Add opt-in FailFast processor for object field errors | PR | 1 | 2026-10-18 | 2026-10-18 |
| 746 | Reject out-of-range JSON integer values | PR | 1 | 2026-10-18 | 2026-10-18 |
| 749 | Add support for `std::monostate` | Issue | 1 | 2026-10-19 | 2026-10-19 |
| 753 | Read files in bulk and report output stream failures | PR | 2 | 2026-10-22 | 2026-10-24 |
| 752 | Work around GCC 12 optional parsing crash with native tuple storage | PR | 1 | 2026-10-24 | 2026-10-24 |
| 758 | Restore CI binary caching and retry vcpkg downloads | PR | 1 | 2026-10-25 | 2026-10-25 |
| 755 | Omit redundant fixed-array lengths in Cereal, YAS and Boost.Serialization | PR | 1 | 2026-10-25 | 2026-10-25 |
| 757 | Avoid copying Cereal input buffers before deserialization | PR | 1 | 2026-10-25 | 2026-10-25 |
| 756 | Preserve typed map keys in MessagePack and binary archives | PR | 1 | 2026-10-25 | 2026-10-25 |
| 759 | Fix fixed-size arrays in typed map entries | PR | 2 | 2026-10-25 | 2026-10-25 |
| 760 | Add bulk I/O missed in the recent std::array optimization | PR | 1 | 2026-10-26 | 2026-10-26 |
| 761 | Add std::bitset support with packed binary and string encodings | PR | 3 | 2026-10-26 | 2026-10-27 |
| 762 | Allow importing enchantum as a thirdparty library | PR | 1 | 2026-11-01 | 2026-11-01 |
| 768 | Clang-23 compilation error with -std=26 | Issue | 1 | 2026-11-05 | 2026-11-05 |
| 766 | Expose the location of each parse failure via rfl::Error::details() | PR | 8 | 2026-11-05 | 2026-11-07 |
| 767 | Avoid needless copies, allocations and exceptions when reading and writing | PR | 2 | 2026-11-07 | 2026-11-07 |
| 769 | Make rfl::CopyableBox copy-assignable | PR | 1 | 2026-11-07 | 2026-11-07 |
| 777 | Inquiry about the format of contributions   | Issue | 1 | 2026-11-11 | 2026-11-11 |
| 770 | Support JSON bytestrings as Base64 strings | PR | 2 | 2026-11-11 | 2026-11-12 |
| 776 | Return an error when an rfl::Skip field's default value throws | PR | 1 | 2026-11-12 | 2026-11-12 |
| 778 | Let DefaultVal's converting constructor throw | PR | 1 | 2026-11-12 | 2026-11-12 |
| 779 | Say why default construction failed when reading a struct with defaults | PR | 1 | 2026-11-12 | 2026-11-12 |
| 782 | Update a DefaultVal test to the error message from #779 | PR | 1 | 2026-11-15 | 2026-11-15 |
| 783 | Read a bare negative number such as `-5` as a positional argument in rfl::cli | PR | 2 | 2026-11-15 | 2026-11-16 |
| 785 | Write optional fields wrapped in rfl::Short or rfl::Positional | PR | 2 | 2026-11-15 | 2026-11-16 |
| 786 | Fix rfl::DefaultVal in NamedTuples and TaggedUnion schemas | PR | 2 | 2026-11-15 | 2026-11-16 |
| 781 | Binary data in `rfl::Generic` | Issue | 1 | 2026-11-15 | 2026-11-15 |
| 784 | Accept const argv and ranges of arguments in rfl::cli::read | PR | 1 | 2026-11-16 | 2026-11-16 |
| 787 | Don't assign to const fields when reading a struct with defaults | PR | 1 | 2026-11-16 | 2026-11-16 |

## #435 Serialising anything other than simple types fails to compile

Issue · opened by @Please-just-dont · closed (completed)

### 2025-05-20 · comment

Replying to: @Please-just-dont (opening post) — "I'm not sure if this is normal. I'm trying to use the [reflect-cpp](https://github.com/getml/reflect-cpp) library to serialise and deserialise json, which is …"

> Hi @Please-just-dont ,
>
> yes, this is normal. The structs need to be aggregates, I will refer you to the documentation:
>
> https://rfl.getml.com/concepts/structs/#important-note
>
> You can however create a custom parser (https://rfl.getml.com/custom_parser/).
>
> As far as inheritance is concerned, this is a very tricky topic, please refer to this part of the documentation:
>
> https://rfl.getml.com/c_arrays_and_inheritance/

[^1]

### 2025-05-20 · comment

> @Please-just-dont , I will also refer you to this part of the documentation that explains how you can accomplish what you are probably trying to accomplish with inheritance:
>
> https://rfl.getml.com/flatten_structs/

[^2]

## #436 rfl::toml::read() doesn't compile (cannot convert from 'toml::v3::noex::parse_result *' to 'rfl::toml::InputVarType')

Issue · opened by @Please-just-dont · closed (completed)

### 2025-05-20 · comment

Replying to: @Please-just-dont — "What's going on? rfl::toml::read<>() doesn't work at all, not even with a string view. It stopped working for me and I don't know what the problem is, as I …"

> @Please-just-dont , yes, we switched back to toml++.
>
> The reason for the back-and-forth is that the vcpkg toml++ recipe did not expose a cmake.config, which made it impossible to support it as a feature. So we switched to toml11. Shortly after that, they actually added a cmake.config, so we switched back, because toml++ is actually faster. Sorry for the confusion this created.
>
> Why it isn't working on your machine, I don't know. But we have automated tests for this on Linux, macOS and Windows using a wide variety of compilers and I can guarantee that it does work. So my guess is that you are maybe somehow still using reflect-cpp 0.17.0 instead of 0.18.0. Maybe you are somehow linking to the wrong version?
>
> The latest version of toml++ is 3.4.0, which is the one we are using as well, so it can't be that?

[^3]

### 2025-05-21 · comment

Replying to: @Please-just-dont — "@liuzicheng1987 Hi, I thought I already posted this but I don't see it so I'll post again. The problem is I defined TOML_EXCEPTION 0. None of the functions to …"

> All right, this is something we should fix. If you want to open a PR, feel free, otherwise I will fix it some time soon.

[^4]

### 2025-06-09 · comment

> @Please-just-dont , this PR (https://github.com/getml/reflect-cpp/pull/447) should resolve the issue.

[^5]

## #439 Field value of named tuple was moved after apply

Issue · opened by @lgpseu · closed (completed)

### 2025-05-26 · comment

Replying to: @lgpseu (opening post) — "- Example 1: `void test() { auto person = rfl::Field<'first_name', std::string>('Bart') * rfl::Field<'last_name', std::string>('Simpson'); // field value will …"

> @lgpseu , thanks for reporting. I will certainly take a look. Can you tell me which compiler/version and which OS you are using?

[^6]

### 2025-05-26 · comment

Replying to: @lgpseu — "MSVC 2022 I think it's moved in the function below, but copy may cause performance issue. Maybe a new type that reference the field value and field name would …"

> @lgpseu , I can reproduce the issue. I am working on a fix.

[^7]

### 2025-05-27 · comment

> @lgpseu , I have a fix. I will wait for the test pipeline and then merge. Again, thank you for reporting this issue.

[^8]

## #440 Made sure that apply can never move or modify the tuple values; fixes…

PR · opened by @liuzicheng1987 · merged

### 2025-05-28 · comment

Replying to: @lgpseu — "@liuzicheng1987 `make_fields` brings unecessary field value copies. I think apply should avoid it as a visitor pattern function. `rfl::to_view` before apply …"

> @lgpseu at the moment, I don't really see another way. To avoid this, we would have to be able to store references, which is very tricky.
>
> You can always use to_view(...).apply(...), which is how this was meant to be used anyway.

[^9]

## #441 Add `enchantum` as an enum reflection backend

PR · opened by @ZXShady · merged

### 2025-05-29 · comment

Replying to: @ZXShady (opening post) — "Disclaimer I am the author of [enchantum](github.com/ZXShady/enchantum). This PR replaces reflect-cpp own enum reflection with enchantum's 1. It reduces …"

> Unless the tests reveal some compiler/OS-specific problems we haven't discovered so far (unlikely), this is ready to be merged, IMO.
>
> @ZXShady , do you agree?
>
> @ZXShady , I have one more request: When you upload enchantum to vcpkg and Conan, please notify me. The maintainers of these package managers want to avoid us including thirdparty libraries that they also ship. If people have two different versions of the same library in their code and don't even know about it that can cause all sort of problems...

[^10]

### 2025-05-29 · comment

Replying to: @ZXShady — "I was thinking about using the split headers instead of the big one since I always keep forgetting to update it and it bloats compile times what do you think? …"

> @ZXShady , there appears to be one more issue on macOS:
>
> ```
> Error: /Users/runner/work/reflect-cpp/reflect-cpp/include/rfl/internal/enums/../../thirdparty/enchantum.hpp:1490:19: error: no member named 'format_to' in namespace 'std'
> [818](https://github.com/getml/reflect-cpp/actions/runs/15320861147/job/43104128808?pr=441#step:8:819)
>       return std::format_to(ctx.out(), "{}", enchantum::to_string_bitflag(e));
> [819](https://github.com/getml/reflect-cpp/actions/runs/15320861147/job/43104128808?pr=441#step:8:820)
>              ~~~~~^
> [820](https://github.com/getml/reflect-cpp/actions/runs/15320861147/job/43104128808?pr=441#step:8:821)
> Error: /Users/runner/work/reflect-cpp/reflect-cpp/include/rfl/internal/enums/../../thirdparty/enchantum.hpp:1492:19: error: no member named 'format_to' in namespace 'std'
> [821](https://github.com/getml/reflect-cpp/actions/runs/15320861147/job/43104128808?pr=441#step:8:822)
>       return std::format_to(ctx.out(), "{}", enchantum::to_string(e));
> [822](https://github.com/getml/reflect-cpp/actions/runs/15320861147/job/43104128808?pr=441#step:8:823)
> ```
>
> I am not entirely sure why this happening, because you are clearly including `<format>`. Maybe the problem will go away once you swap to the multiple header files.
>
> I am totally fine with splitting up the single header file. By the way, if you keep on forgetting to update it, you can write a simple Python script that does it for you and then have a GitHub Actions step automatically file a PR.

[^11]

### 2025-05-29 · comment

> @ZXShady , by the way, I am also taking a closer look at the build pipelines, specifically why they suddenly take so long. The issue appears to be that GitHub Actions fails to connect to the binary cache and then tries to do so over and over again.

[^12]

### 2025-05-29 · comment

Replying to: @ZXShady — "currently overriding enum_range doesn't allow prefix_length to be overriden as well. I think it would be better to also add a recommendation to instead …"

> @ZXShady what exactly does the prefix length do? I think it can be easily added to the configurations...

[^13]

### 2025-05-29 · comment

Replying to: @ZXShady — "@liuzicheng1987 it removes N characters from the start of enum name. example in docs ```cpp #include <enchantum/enchantum.hpp> #include …"

> @ZXShady I See. I suppose that is something we can always add later.

[^14]

### 2025-05-29 · comment

> @ZXShady the tests have finally succeeded. I don't why they take do long these days.
>
> But I am merging. Thank you so much for this great contribution. If you really manage to extend the range to -32,000 to 32,000, please open another PR. I would be very interested.

[^15]

## #442 Fix cbor encoding for uint64_t's

PR · opened by @Ian-Flury · merged

- 2025-05-31 · comment · ack: “@Ian-Flury , thanks for your contribution!” (replying to @Ian-Flury (opening post)) [^16]

## #443 Suggestion: Deep comparison for `rfl::to_view()`

Issue · opened by @bryceschober · open

### 2025-06-05 · comment

Replying to: @bryceschober (opening post) — "Today, I naively thought: ```c++ bool operator==(const struct T& lhs, const struct T& rhs) { return( rfl::to_view(lhs) == rfl::to_view(rhs) ); } ``` That sure …"

> I like the idea. Let me think about it.

[^17]

### 2025-06-06 · comment

> @bryceschober, so here is how I would approach the problem:
>
> ```cpp
>
> bool operator==(const struct T& lhs, const struct T& rhs)
> {
>     auto lhs_view = rfl::to_view(lhs);
>     auto rhs_view = rfl::to_view(rhs)
>     return [&]<int... is>(std::integer_sequence<is...>) {
>         return true && ... && *rfl::get<is>(lhs_view) == *rfl::get<is>(rhs_view);
>     }(std::make_index_sequence<rfl::tuple_size_v<decltype(lhs_view)>>());
> }
> ```
>
> This is certainly a quick fix that should solve your problem.
>
> But I do see that this may be a bit fancy for some people and that a simpler solution might be needed.

[^18]

### 2025-06-06 · comment

> @bryceschober I am thinking about an elegant way that would allow you Cover a whole range of use cases like that, I wouldn't want to go with a solution that cover just this one specific use case.
>
> One possibility is to offer a zip operator that would transform two tuples into a tuple of pairs:
>
> ```cpp
>
> const auto pairs = zip(lhs_view.values(), rhs_view.values());
>
> return rfl::apply([](auto... ps) {
>    return true && ... && *rfl::get<0>(ps) == *rfl::get<1>(ps);
> }, pairs);
>
> ```

[^19]

### 2025-06-06 · comment

> And the other solution is that I should modify named_tuple to allow you to crate named tuples of references

[^20]

### 2025-06-06 · comment

Replying to: @bryceschober — "One thing I'm curious about is whether any one of those solutions will compile down to essential the same machine code as the manual member-wise comparison …"

> @bryceschober, your point on the compiler explorer is well taken.

[^21]

## #445 Improved support for string_views for TOML and XML

PR · opened by @liuzicheng1987 · merged

### 2025-06-08 · comment

Replying to: @ZXShady — "my issue was not comprehensive, there is some other places like https://github.com/getml/reflect-cpp/blob/main/src%2Frfl%2Fyaml%2FWriter.cpp#L65"

> @ZXShady , the writer should be fine. It's not exposed to the end-users. These string views are actually derived from the field names of our structs.

[^22]

### 2025-06-08 · comment

Replying to: @ZXShady — "shouldn't it be in the internal namespace then?"

> @ZXShady , I don't really think so. I actually do encourage people to write their own readers and writers and go at great length to explain how to do so. It's just that no one would ever directly use them. They would be exposed to the library-internal code. It's pretty similar to your custom enum ranges.
>
> Basically, the string_views in the writers are the field names derived from the structs. They originally come in the form of a StringLiteral:
>
> https://github.com/getml/reflect-cpp/blob/main/include/rfl/internal/StringLiteral.hpp
>
> As you can see, they are null-terminated, pretty much precisely for this reason. Where we can use the size, we do, but if we can't, it's not a big problem. I understand this is a potential footgun, but the reality is that yaml-cpp does not support string views. (Let alone any of the C libraries.) If we were to replace them with std::string, this would basically force us to always create a deep copy, which is something I would very much like to avoid.
>
> However, xml::read is exposed to the end user, of course, which is why your issue (https://github.com/getml/reflect-cpp/issues/444) is a really great catch! Thank for this!

[^23]

### 2025-06-08 · comment

Replying to: @ZXShady — "https://github.com/jbeder/yaml-cpp/blob/master/include%2Fyaml-cpp%2Femitter.h#L76 has an overload. But I see what you mean the strings are the Literals …"

> @ZXShady , turns out it actually has a direct overload to std::string_view. That's great.
>
> https://github.com/jbeder/yaml-cpp/blob/master/include%2Fyaml-cpp%2Femitter.h#L210

[^24]

### 2025-06-08 · comment

> @ZXShady , unfortunately, the string_view support does not appear to be included in the latest version of yaml-cpp. However, I was able to get string_view support in pugixml after upgrading to pugixml 1.15.

[^25]

## #444 Don't use `std::string_view` with APIs expecting null-terminated strings

Issue · opened by @ZXShady · closed (completed)

### 2025-06-08 · comment

Replying to: @ZXShady (opening post) — "```cpp template <class T, class... Ps> Result<T> read(const std::string_view _xml_str) { pugi::xml_document doc; const auto result = …"

> @ZXShady , great catch, thank you so much. Issue is resolved with ec6ab3baf5d42bec0e2f680ddd65116184a8f28f.

[^26]

## #447 Added support for exception-free TOML; resolves #436

PR · opened by @liuzicheng1987 · merged

### 2025-06-09 · inline include/rfl/toml/read.hpp:41-45

Replying to: @gemini-code-assist[bot] (inline) — "In this `else` branch (when `TOML_EXCEPTIONS` is not defined), `::toml::parse` returns a `toml::parse_result` and does not throw. However, the subsequent call …"

> No, it is guaranteed not to throw. This is what TOML_EXCEPTIONS 0 means.

[^27]

### 2025-06-09 · inline include/rfl/toml/read.hpp:37-38

Replying to: @gemini-code-assist[bot] (inline) — "The `noexcept` specifier on this function (line 32) promises that no C++ exceptions will propagate out. However, this `catch` block only handles …"

> That is a good point. I will fix it.

[^28]

## #448 Dont repeat class names template arguements

PR · opened by @ZXShady · merged

- 2025-06-10 · comment · ack: “Merged, thanks” (replying to @ZXShady (opening post)) [^29]

## #449 Flatten<TaggedUnion<...>> isn't flattened but instead becomes a "variant_" implicit field.

Issue · opened by @abex-py · closed (completed)

### 2025-06-12 · comment

Replying to: @abex-py (opening post) — "Hi! Context: using JSON reflect-cpp It looks like it's not possible to have a Flatten<TaggedUnion<TypeA, TypeB>>. The TaggedUnion doesn't get flattened but …"

> This is indeed unsupported.
>
> What you can do, is to do the opposite: Flatten `Person` into `Employee` and `ShareHolder`.

[^30]

## #451 Reflect on the number of fields of a too-complex type at compile time?

Issue · opened by @bryceschober · closed (completed)

### 2025-06-17 · comment

Replying to: @bryceschober — "Ah, now I see that I can use `rfl::internal::num_fields<T>` for that purpose... but obviously that's not 'public'. Am I missing a different way to do this? …"

> @bryceschober I think we can make that public, that's not a problem

[^31]

### 2025-06-19 · comment

Replying to: @bryceschober — "Are there other compile-time reflection capabilities that are similarly internal, like field name and type iteration?"

> @bryceschober field names and types are public

[^32]

## #452 Are there any examples on how to serialize / deserialize a json tree?

Issue · opened by @MirrasHue · closed (completed)

### 2025-06-19 · comment

Replying to: @MirrasHue (opening post) — "Hey, I've been looking for an example on how to serialize / deserialize a json or yaml tree, but only found examples for single objects with `write()` and …"

> Hi @MirrasHue could you clarify what you mean by a JSON tree? Do you mean nested structs?

[^33]

### 2025-06-19 · comment

> @MirrasHue have you taken a look at the documentation?
>
> http://rfl.getml.com/docs-readme/

[^34]

### 2025-06-20 · comment

Replying to: @MirrasHue — "Thanks for the quick reply, @liuzicheng1987. I mean getting a reference to a root node and start adding things, like constructing a tree, where we have for …"

> @MirrasHue do you mean something like rfl::Object?
>
> http://rfl.getml.com/object/

[^35]

## #453 added REFLECTCPP_INSTALL to allow user to control if required

PR · opened by @BestITUserEUW · merged

- 2025-07-04 · comment · ack: “@BestITUserEUW , thank you!” (replying to @BestITUserEUW (opening post)) [^36]

## #455 Feature request: Allow a 'fallback' type for tagged union

Issue · opened by @MiSo1289 · closed (completed)

### 2025-07-04 · comment

Replying to: @MiSo1289 (opening post) — "It is useful to me to be able to parse tagged unions where sometimes a discriminant occurs that I did not specify at compile time - eg. I don't care about it …"

> Hi @MiSo1289,
>
> couldn't you just do this?
>
> ```
> std::variant<TaggedUnion<...>, FallbackType>
> ```

[^37]

## #454 Support Deserialize std::string_view

Issue · opened by @lixin-wei · closed (completed)

### 2025-07-06 · comment

Replying to: @lixin-wei — "@liuzicheng1987 Would you pls have a look? Want to know your opinion before start."

> @lixin-wei Hi, I have been thinking about this for a couple of days and I'm honestly unsure. I don't like the fact that there is a side-effect. What is more, we have another solution for raw pointers.
>
> Why do you think that string_views are necessary here? Do you think it might be an alternative to have custom immutable string type?

[^38]

### 2025-07-08 · comment

Replying to: @lixin-wei — "@liuzicheng1987 Thanks for your time to think about this! Sorry I didn't read the doc throughly, you mean `rfl::AllowRawPtrs` right? **So can I extend this …"

> @lixin-wei , exactly I think this is something we could do. The user would then have to free string_view.data(). I would absolutely support this approach.

[^39]

## #456 rename _res to _result to avoid macro conflict

PR · opened by @lixin-wei · merged

- 2025-07-08 · comment · ack: “@lixin-wei thank you for your contribution !” (replying to @lixin-wei (opening post)) [^40]

## #457 Support for `std::span`

Issue · opened by @ChihHao-Su · closed (completed)

### 2025-07-11 · comment

Replying to: @ChihHao-Su (opening post) — "`std::span` is the common, 'standard' way in C++20 to express a view to a contiguous sequence of values. Assume we have a page function that returns a …"

> Cool idea!

[^41]

## #463 Json Read rfl::UnderlyingEnum processor is broken

Issue · opened by @BestITUserEUW · closed (completed)

### 2025-07-14 · comment

Replying to: @BestITUserEUW (opening post) — "Branch: main Problem: rfl::UnderlyingEnum Processor in rfl::json::read parses anything you give it. Compiler: gcc-14 / clang-20 Reproducible example: ```cpp …"

> @BestITUserEUW it is not that surprising, actually.
>
> The documentation states that enums that cannot be matched will be parsed as a number, so the fact that it parses 20 is to be expected.
>
> http://rfl.getml.com/enums/#what-happens-if-an-enum-cannot-be-matched
>
> As for the "hello", that's a bit more problematic. What version of the library are you using? The last release or the main branch?

[^42]

### 2025-07-20 · comment

Replying to: @BestITUserEUW — "@liuzicheng1987 Reproduced in current main branch. Discovered in 2a0fdeabe35bdab0e07e05d4f869ca88bd6d2b8c. As the enum backend has been swapped to enchantum is …"

> @BestITUserEUW , sorry, I didn't see your message until just now. Yes, let's give that a try. A PR is welcome

[^43]

### 2025-08-05 · comment

> @BestITUserEUW , I have found the root cause for the issue. Thank you so much pointing it out. Turns out you had stumbled over UB.
>
> Here is what happened: The person who wrote enchantum is actually a kid. He/she (I don't even know) is quite brilliant and it is really impressive what they can do, given their age. However, they clearly haven't understood how you are supposed to use monadic result types, because here is what happened:
>
> ```cpp
> return static_cast<T>(
>               *_r.template to_basic_type<std::underlying_type_t<T>>(_var));
> ```
>
> This i a cardinal sin - because when the parsing fails, as it did in your second test case, you get UB. But I can't really be angry at anyone but myself - it is my fault for missing that during the review process.
>
> Luckily the fix is simple:
>
> ```cpp
> return _r.template to_basic_type<std::underlying_type_t<T>>(_var)
>               .transform([](const auto _val) { return static_cast<T>(_val); });
> ```
>
> As for the first test case - I still think that this is what is supposed to happen. Integers can be cast to enums, so any integer is fair game. It is also clearly documented that this is how the library is supposed to behave.
>
> In any case, thank you again for flagging this. Luckily I haven't made a formal release before this was fixed!

[^44]

### 2025-08-09 · comment

Replying to: @BestITUserEUW — "@liuzicheng1987 Thanks for resolving this. I’ve been busy with work and didn’t have time to work on it. Regarding integer-to-enum class parsing, I don’t see a …"

> @BestITUserEUW , my view on the matter is as follows. It is absolutely legal in C++ to do something like this:
>
> ```cpp
> const auto e = static_cast<MyEnum>(some_random_integer);
> ```
>
> You can even do something like this:
>
> ```cpp
> enum byte : unsigned char {}; // byte is a new integer type; see also std::byte (C++17)
> byte b{42};        // OK as of C++17 (direct-list-initialization)
> ```
>
> https://en.cppreference.com/w/cpp/language/enum.html
>
> The enumerated values are _possible values_ but by no means the _only possible values_. Since it is absolutely legal to do this do an enum, the library must be able to serialize and deserialize it.
>
> What I could envision, though, is a processor that would prohibit this.

[^45]

### 2025-11-01 · comment

Replying to: @ZXShady — "I am male, I do know about moands and such also I didn't touch this line of code since it was unrelated to the pr …"

> @ZXShady , you are right. Must have been someone else. Sorry about that.

[^46]

## #459 Treat `std::vector<char>` similar to `rfl::Bytestring`

Issue · opened by @bjia56 · closed (completed)

### 2025-07-14 · comment

Replying to: @bjia56 (opening post) — "I am using msgpack in an RPC library to serialize structs with reflect-cpp and have a situation where msgpack messages are nested, i.e. a `std::vector<char>` …"

> Ok

[^47]

## #462 cbor supports std::optional; fixes #135

PR · opened by @dcorbeil · merged

### 2025-07-16 · comment

Replying to: @dcorbeil (opening post) — "I went for the option to generate indeterminate length maps. This allows to not require knowing the size of a struct (for example) in advance for the top level …"

> @dcorbeil thanks for the PR. Sorry it took me a couple of days to get to it, I was a bit busy. I will take a closer look tonight.

[^48]

### 2025-07-16 · comment

> @dcorbeil thank you for your contribution! @bryceschober , thanks for reviewing it!

[^49]

## #465 Make `rfl::num_fields` public

PR · opened by @bryceschober · merged

### 2025-07-16 · inline include/rfl/internal/num_fields.hpp:210

Replying to: @gemini-code-assist[bot] (inline) — "Moving `num_fields` out of the `internal` namespace makes it a public API, but it's still defined within a header file located in the `internal` directory. …"

> @bryceschober , I agree with this one. I think the namespaces should always follow the file structure, wherever possible. It just makes it easier to navigate and find things.
>
> I think what you can also do is to just include a file include/rfl/num_fields.hpp:
>
> ```cpp
> #include "internal/num_fields.hpp"
>
> namespace rfl {
>   template <class T>
>   constexpr std::size_t num_fields = internal::num_fields<T>;
> }
> ```

[^50]

### 2025-07-16 · inline tests/README.md:35

Replying to: @gemini-code-assist[bot] (inline) — "The cmake command is quite long. Consider splitting it into multiple lines for improved readability. You can use `\` to continue the command on the next line. …"

> @bryceschober , I think we can safely ignore this one.

[^51]

## #464 Fix bug reading with 'tagged' variants when a variant is non-copyable

PR · opened by @jmcken8 · merged

### 2025-07-16 · comment

Replying to: @jmcken8 (opening post) — "Added a test. Needed to change the test to emplace each element rather than initializer_list due to the non-copyable element."

> @jmcken8 , makes sense. Thank you for the contribution!

[^52]

## #466 Serialize/Deserialize `vector<char>` as bytestring

PR · opened by @bjia56 · merged

### 2025-07-17 · comment

Replying to: @bjia56 (opening post) — "Resolves #459"

> @bjia56 , thanks for the PR. It is still marked as work in progress. When you want me to review (and merge) it, just mark it as "Ready for review".

[^53]

### 2025-07-19 · comment

Replying to: @bjia56 — "@liuzicheng1987 ok I think this is ready now, thanks!"

> @bjia56 , it looks good at first sight.
>
> The one thing I have noticed is that I don't think it is necessary to differentiate between Bytestring and Vectorstring in Schema::Type, because to any serialization format these two are the same anyway.

[^54]

### 2025-07-19 · inline tests/cbor/test_vectorstring.cpp:10-12

Replying to: @gemini-code-assist[bot] (inline) — "Consider also testing serialization of a `std::vector<rfl::Vectorstring>`, similar to the test case in `tests/bson/test_vectorstring.cpp`. This would ensure …"

> I don't think this is necessary

[^55]

### 2025-07-19 · inline tests/ubjson/test_vectorstring.cpp:10-12

Replying to: @gemini-code-assist[bot] (inline) — "Consider also testing serialization of a `std::vector<rfl::Vectorstring>`, similar to the test case in `tests/bson/test_vectorstring.cpp`. This would ensure …"

> Again, not necessary

[^56]

- 2025-07-20 · comment · ack: “@bjia56 , thanks for the contribution. I am merging.” [^57]

## #468 Added support for std::span; resolves #457

PR · opened by @liuzicheng1987 · merged

- 2025-07-20 · inline tests/json/test_span_read_write.cpp:52-54 · ack: “fixed” (replying to @gemini-code-assist[bot] (inline)) [^58]

## #469 Add support for reading from a byte-container for cbor and ubjson

PR · opened by @Ian-Flury · merged

- 2025-07-26 · comment · ack: “@Ian-Flury thanks for your contribution.” (replying to @Ian-Flury (opening post)) [^59]

## #470 sqlgen cannot find reflectcpp-config.cmake

Issue · opened by @obqrn · closed (completed)

### 2025-07-28 · comment

Replying to: @obqrn (opening post) — "reflectcpp-config.cmake not generated automatically. I use reflectcpp and sqlgen by cmake FetchContent_Decode. sqlgen cannot find reflectcpp-config.cmake or …"

> @obqrn can you show me exactly what it is you do?
>
> We have GitHub Action pipelines set up in sqlgen and needless to say it works there. So I would need to understand exactly what you do to be able to reproduce the problem.

[^60]

### 2025-07-29 · comment

Replying to: @obqrn — "I need reflectcpp-config.cmake or reflectcppConfig.cmake file, but Cann't find."

> Well, if you follow the instructions in the sqlgen documentation, I assure you it works. It will produce a cmake.config and it will be able to find it.
>
> It works perfectly fine in all of our testing pipelines.
>
> So again: Could you tell me exactly how you are trying to include reflect-cpp in your project?

[^61]

### 2025-07-29 · comment

> @obqrn , I have recompiled reflectcpp exactly as it is described in the README:
>
> ```
> ~/reflect-cpp$ cmake -S . -B build -DCMAKE_CXX_STANDARD=20 -DCMAKE_BUILD_TYPE=Release
> -- The CXX compiler identification is GNU 13.3.0
> -- Detecting CXX compiler ABI info
> -- Detecting CXX compiler ABI info - done
> -- Check for working CXX compiler: /usr/bin/c++ - skipped
> -- Detecting CXX compile features
> -- Detecting CXX compile features - done
> -- Configuring done (0.5s)
> -- Generating done (0.0s)
> -- Build files have been written to: /home/ubuntu/reflect-cpp/build
> ~/reflect-cpp$ cmake --build build -j 4  #
> [ 20%] Building CXX object CMakeFiles/reflectcpp.dir/cmake_pch.hxx.gch
> [ 40%] Building CXX object CMakeFiles/reflectcpp.dir/src/reflectcpp.cpp.o
> [ 60%] Building CXX object CMakeFiles/reflectcpp.dir/src/yyjson.c.o
> [ 80%] Building CXX object CMakeFiles/reflectcpp.dir/src/reflectcpp_json.cpp.o
> [100%] Linking CXX static library libreflectcpp.a
> [100%] Built target reflectcpp
> ~/reflect-cpp$ ls build
> CMakeCache.txt  CMakeFiles  CPackConfig.cmake  CPackSourceConfig.cmake  Makefile  cmake_install.cmake  libreflectcpp.a  reflectcpp-config.cmake
> ```
>
> And as you can see, there is a `reflectcpp-config.cmake` in the build folder, just as expected.

[^62]

### 2025-07-30 · comment

Replying to: @obqrn — "Sorry. I am not compiling reflectcpp alone. I use the FetchContent_Decode method of CMake to include reflectcpp, similar to including source code. The …"

> @obqrn , can you send me the relevant contents of your CMake? A minimal example to reproduce the problem?

[^63]

### 2025-08-29 · comment

> Cannot reproduce, no response in over 4 weeks, closing.

[^64]

## #429 Support CBOR parsing using error codes

Issue · opened by @bryceschober · closed (completed)

### 2025-07-28 · comment

Replying to: @bryceschober — "I've finally started work on this, and I think I've gotten a workable `concept` started, that can detect the new function and provide alternate …"

> @bryceschober I would wait until there is an official release on vcpkg and Conan. Otherwise it's just too much hassle.

[^65]

## #472 Multiple serialization strategies for a custom non-struct type

Issue · opened by @tatutimonen · closed (completed)

### 2025-07-30 · comment

Replying to: @tatutimonen (opening post) — "Does the library provide any facilities for defining multiple ways to serialize a custom non-struct type, especially non-invasively? Say I'd want to control …"

> You could always implement the parser template for your type. That would basically enable you to do whatever you want. And it is non-invasive.
>
> https://rfl.getml.com/custom_parser/#example
>
> https://github.com/getml/reflect-cpp/tree/main/include/rfl/parsing

[^66]

### 2025-07-30 · comment

> By the way, rfl::Reflector can be used for non-struct types as well.

[^67]

### 2025-07-31 · comment

Replying to: @tatutimonen — "Is there a way to have a given reader/writer pair support multiple different `Impl`s?"

> You could have a variant as an Impl

[^68]

## #473 Question: Limit scope of reflection

Issue · opened by @ansible42 · closed (completed)

### 2025-08-04 · comment

Replying to: @ansible42 (opening post) — "This is a great tool, and I was so happy to find it. Question: Is it possible to limit the scope of the reflection as this would speed up the compile time. We …"

> Great question!
>
> The best way to do this is to run something like json::read, json::write or json::to_schema in your cpp files only.
>
> I know it's just a single line of code and it's tempting to put these in your header files, but this one line of code abstracts away A LOT of boilerplate code. All this boilerplate code needs to be generated and then compiled under-the-hood.

[^69]

- 2025-08-04 · comment · ping: “@ansible42” [^70]

## #475 Using Field in combination with validation

Issue · opened by @ansible42 · closed (completed)

### 2025-08-07 · comment

Replying to: @ansible42 (opening post) — "Working with structs that will become config files down the line. Trying to generate schema as well as have a bit of custom json formatting. Structs ```cpp …"

> @ansible42 , I will take a look tonight. Sorry it took me a while to respond.

[^71]

### 2025-08-07 · comment

> @ansible42 this should fix the issue. Thanks for reporting!

[^72]

## #480 Unable to generate build files for the YAML serializer

Issue · opened by @ShinyMacadamia · closed (completed)

### 2025-08-12 · comment

Replying to: @ShinyMacadamia (opening post) — "I ran into a weird issue of not being able to generate build files via CMake when using additional serializers UNLESS i run cmake without them. I tried both on …"

> Hi @ShinyMacadamia ,
>
> I cannot reproduce the issue:
>
> ```
> ubuntu@getml:~/reflect-cpp$ rm -r build
> ubuntu@getml:~/reflect-cpp$ cmake -S . -B build -DCMAKE_CXX_STANDARD=23 -DREFLECTCPP_YAML=ON -DCMAKE_BUILD_TYPE=Release
> -- Running vcpkg install
> ...
> yaml-cpp provides CMake targets:
>
>   # this is heuristically generated, and may not be correct
>   find_package(yaml-cpp CONFIG REQUIRED)
>   target_link_libraries(main PRIVATE yaml-cpp::yaml-cpp)
>
> yaml-cpp provides pkg-config modules:
>
>   # A YAML parser and emitter for C++
>   yaml-cpp
>
> -- Running vcpkg install - done
> -- The CXX compiler identification is GNU 13.3.0
> -- Detecting CXX compiler ABI info
> -- Detecting CXX compiler ABI info - done
> -- Check for working CXX compiler: /usr/bin/c++ - skipped
> -- Detecting CXX compile features
> -- Detecting CXX compile features - done
> -- Configuring done (1.2s)
> -- Generating done (0.0s)
> -- Build files have been written to: /home/ubuntu/reflect-cpp/build
> ```
>
> What is more, we are testing pretty much this scenario in all of our Github Action pipelines:
>
> <img width="303" height="296" alt="Image" src="https://github.com/user-attachments/assets/92dd6657-84a3-4a93-9a16-a799d1bbafbb" />
>
> https://github.com/getml/reflect-cpp/actions/runs/16907952807
>
> And it works fine every time, I have never seem this issue.
>
> Are you sure the issue couldn't be your local setup?

[^73]

### 2025-08-23 · comment

Replying to: @ShinyMacadamia — "@liuzicheng1987 I tried several distros (Fedora, Ubuntu) natively and i cannot seem to build it. Maybe you could provide some info on what could i look into to …"

> @ShinyMacadamia Could you show me what you do before you call CMake? How do you clone the repository and initialise vcpkg?

[^74]

### 2025-08-24 · comment

> @ShinyMacadamia , I think I know what the issue is. If I'm right, I will upload a fix tomorrow night.

[^75]

### 2025-08-25 · comment

> @ShinyMacadamia , I cannot reliably reproduce the issue, but I suspect it might disappear if you remove some of the versions from vcpkg.json.
>
> Specifically, change these lines in vcpkg.json:
>
> ```
>     {
>       "name": "vcpkg-cmake",
>       "version>=": "2023-05-04"
>     },
>     {
>       "name": "vcpkg-cmake-config",
>       "version>=": "2022-02-06#1"
>     },
>     {
>       "name": "vcpkg-pkgconfig-get-modules",
>       "version>=": "2024-04-03"
>     }
> ```
>
> Replace them with this:
>
> ```
>     {
>       "name": "vcpkg-cmake"
>     },
>     {
>       "name": "vcpkg-cmake-config"
>     },
>     {
>       "name": "vcpkg-pkgconfig-get-modules"
>     }
> ```
>
> Could you try that and see whether the issue persists?

[^76]

### 2025-11-01 · comment

> No response for over two months. I am closing the issue.

[^77]

## #479 Add interfaces for jwt-cpp compatibility

PR · opened by @aligirayhanozbay · merged

- 2025-08-12 · comment · ack: “@aligirayhanozbay thanks for the contribution!” (replying to @aligirayhanozbay (opening post)) [^78]

## #482 C++20 Modules?

Issue · opened by @jowillianto · closed (not_planned)

### 2025-08-15 · comment

Replying to: @jowillianto (opening post) — "This is just a proposal. Please reject if considered unnecessary. I can try to work on this specifically. i.e: import reflectcpp; import reflectcpp.json;"

> @jowillianto , would there be a way to design this such that it is optional? Meaning you could have either modules or headers?

[^79]

### 2025-09-13 · comment

Replying to: @jowillianto — "@liuzicheng1987 , if you give your say on this matter on which method to go with, I will start implementing the solution we discussed. After doing this, we …"

> @jowillianto , sorry for the late reply on the matter.
>
> Yes, please go ahead and draft a PR using the "more invasive" method you have discussed.

[^80]

## #481 Externally tagged unions as fields

Issue · opened by @GregTheMadMonk · open

### 2025-08-15 · comment

Replying to: @GregTheMadMonk (opening post) — "Hello! After reading https://github.com/getml/reflect-cpp/blob/main/docs/variants_and_tagged_unions.md I was under the impression that I would be able to do …"

> @GregTheMadMonk , you can do the opposite:
>
> ```cpp
> struct Shape {
>     std::string custom_name;
> };
>
> struct Rectangle{ ..., rfl::Flatten<Shape> shape; }
>
> struct Circle{ ..., rfl::Flatten<Shape> shape; }
>
> rfl::Variant<rfl::Field<"rectangle", Rectangle>, rfl::Field<"circle", Circle>> shape;
> ```

[^81]

### 2025-08-15 · comment

Replying to: @GregTheMadMonk — "Thank for your reply! I'm afraid that would create ``` { 'rectangle': { 'custom_name': '...', ... } } { 'circle': { 'custom_name': '...', ... } } ``` rather …"

> @GregTheMadMonk , you are right...what you are asking for is currently unsupported. But it wouldn't be a bad idea. I think being more flexible about what you can flatten has come up quite a lot.

[^82]

## #484 Better forwarding to improve read performance a little

PR · opened by @GregTheMadMonk · merged

### 2025-08-20 · comment

Replying to: @GregTheMadMonk (opening post) — "Hello! This isn't really a significant change, but it helps speed up `rfl::json::read<rfl::Generic>` almost twice in the release build (at least in the …"

> @GregTheMadMonk that sounds great, but some of the tests appear to be failing. Could you take a look?

[^83]

### 2025-08-20 · comment

Replying to: @GregTheMadMonk — "@liuzicheng1987 if you're talking about the compilation errors (`-Werror=maybe-uninitialized`), then I'm honestly kind of out of ideas for what may cause it, …"

> Ok. Could you try to just set err = std::nullopt when you first declare it?

[^84]

### 2025-08-24 · comment

Replying to: @GregTheMadMonk — "It didn't help, sadly, for me at least. Same with replacing `if (err)` with `if (err.has_value())`:"

> @GregTheMadMonk , this should fix the issue. We'll wait for the tests to run through and then this is ready to be merged.
>
> Thank you for the contribution!

[^85]

### 2025-09-01 · comment

Replying to: @GregTheMadMonk — "Thank you! How did you come to this fix? Or was that the warning system in GCC has actually misfired and you had to do it by trial and error?"

> @GregTheMadMonk I had figured that the problem only occurs where we have a mutable optional variable, moreover I recall having seen something like this before. So that's how I figured it out.

[^86]

## #485 make read_one_alternative more general

PR · opened by @ftschindler · merged

### 2025-08-27 · comment

Replying to: @ftschindler — "Hi @liuzicheng1987 @Urfoex, I'm calling you out directly as you're listed in …"

> @ftschindler , if the checks pass, the contribution is something that we would be willing to maintain the code quality is good, we merge.

[^87]

- 2025-08-27 · comment · ack: “@ftschindler , thanks for your contribution!” [^88]

## #486 rfl::UUIDv4 field fails JSON parse in struct even for valid UUID

Issue · opened by @RampantDespair · closed (completed)

### 2025-08-31 · comment

Replying to: @RampantDespair (opening post) — "Hi I’ve run into what looks like a bug with the built-in `rfl::UUIDv4` pattern type when it’s used **directly as a struct field**. ### Minimal repro …"

> @RampantDespair , I cannot reproduce the issue:
>
> https://github.com/getml/reflect-cpp/pull/487
>
> As you can see, I have added a test for exactly the string you provided and it passes just fine on all tested operating systems and with all tested compilers.

[^89]

### 2025-08-31 · comment

> @RampantDespair , what version of CTRE are you using?

[^90]

### 2025-08-31 · comment

Replying to: @RampantDespair — "@liuzicheng1987 I didn't install it directly, it's transitive from `reflectcpp`, here is the version: ``` $ vcpkg list | findstr /i ctre ctre:x64-windows …"

> @RampantDespair , yes, that appears to be the issue. We were using 3.8 and the moment I upgraded to 3.10, all of the regex validators failed.

[^91]

### 2025-08-31 · comment

> @RampantDespair , I figured it out. Fix will come shortly.

[^92]

### 2025-08-31 · comment

> @RampantDespair , here's the fix: [dad40d3](https://github.com/getml/reflect-cpp/pull/487/commits/dad40d3769589d1e3fa257f6d9ba23d46f683822)
>
> If you're interested: The issue was that we have a trailing '\0' in our compile time strings. CTRE used to be OK with that, but but as of v3.10.0, it no longer is. Luckily, there's an easy fix.
>
> I will make sure the tests run through, merge the PR, then close this issue.
>
> Thanks for reporting!

[^93]

### 2025-08-31 · comment

Replying to: @RampantDespair — "@liuzicheng1987 Wow. Really fast fix, thank **you** for being so prompt! :) I will wait for the vcpkg bundle update."

> @RampantDespair , I will update the vcpkg port in 2-3 weeks, there are a couple of things I want to do first.
>
> As a quick fix, just downgrade CTRE to 3.8.

[^94]

## #467 Allow using `rfl::AddTagsToVariants` with `rfl::Generic` and same-name-different-namespace structs

PR · opened by @jmcken8 · closed without merge

### 2025-09-02 · comment

Replying to: @bryceschober — "@jmcken8 Well I'm not sure how you would make your solution work either. I'm guessing that you've looked at the test failures reported here by now. Looking at …"

> Hi @jmcken8 and @bryceschober , how should we proceed with this PR? Should I take a look?

[^95]

### 2025-09-07 · comment

Replying to: @jmcken8 — "Thanks, @liuzicheng1987, yes, that'd be helpful. @bryceschober that would work for the custom case, but it's really the `variant` case that's the issue, fixing …"

> @jmcken8 @bryceschober it may not be the most elegant solution I ever came up with, but it gets the job done.

[^96]

### 2025-09-09 · comment

> @bryceschober I like your second proposal of simply having two separate processors. It would also be a non-braking change, which is always good.

[^97]

### 2025-09-14 · comment

Replying to: @bryceschober — "@liuzicheng1987, I have published #494 as my alternative for your and @jmcken8 to review."

> @jmcken8 @bryceschober I am going to close this, because the issue was solved in another PR.

[^98]

## #493 Can't add vector into extrafields ?

Issue · opened by @jncoueron · closed (completed)

### 2025-09-08 · comment

Replying to: @jncoueron (opening post) — "I want to be able to add data into an extrafield of my object, it's work well with a int, double or string but it's doesn't work with vector. There is …"

> Hi @jncoueron , this doesn't have anything to do with the extra fields, it's because of the generic:
>
> https://rfl.getml.com/generic/
>
> You need to add an rfl::Generic::Array

[^99]

## #495 "May be used uninitialized" issues reported by GCC 15.2.0

Issue · opened by @Harry-Chen · closed (completed)

### 2025-09-10 · comment

Replying to: @Harry-Chen (opening post) — "When trying to compile the projects with GCC 15.2.0, multiple similar warnings (in my example they are errors due to `-Wall -Werror`) would occur: ``` …"

> @Harry-Chen , what version are you using here? I am fairly sure that if you use the current main branch, this issue should already be resolved.

[^100]

### 2025-09-10 · comment

Replying to: @Harry-Chen — "@liuzicheng1987 I'm working on v0.20.0. Is it https://github.com/getml/reflect-cpp/commit/3dc74c9596a5ef361b462b938082e09bc01a7e3c that fixed this?"

> @Harry-Chen , no https://github.com/getml/reflect-cpp/commit/dcc4c1d5c9a327b7163c9d97baf5ac53b6e39b76

[^101]

### 2025-09-10 · comment

> <img width="1966" height="343" alt="Image" src="https://github.com/user-attachments/assets/ccbce66c-2b22-4b91-963b-0df20bcf0258" />

[^102]

### 2025-09-10 · comment

Replying to: @Harry-Chen — "@liuzicheng1987 Thanks! This works on my side. Could I ask if there is plan to release 0.21.0 soon (to include the fixes and new features)?"

> @Harry-Chen , 0.21.0 is just around the corner. I'm just waiting for https://github.com/getml/reflect-cpp/pull/494 and then I will wrap it up.

[^103]

### 2025-09-18 · comment

Replying to: @Harry-Chen — "@liuzicheng1987 Unfortunately, I have discovered more similar warnings in the 0.21.0 release: ``` FAILED: …"

> @Harry-Chen OK

[^104]

### 2025-10-13 · comment

> Should be resolved now

[^105]

## #494 Add `rfl::AddNamespacedTagsToVariants` for `rfl::Generic` and same-name-different-namespace structs

PR · opened by @bryceschober · merged

### 2025-09-10 · inline include/rfl/Processors.hpp:42

Replying to: @bryceschober (inline) — "FYI, at one point I considered setting both `add_tags_to_variants_` and `is_add_namespaced_tags_to_variants` for the new `rfl::AddNamespacedTagsToVariants` to …"

> @bryceschober , I agree with your approach. I think we should simply insert a static_assert to make sure that people aren't adding both processors at the same time.

[^106]

### 2025-09-10 · inline include/rfl/Processors.hpp:42

Replying to: @bryceschober (inline) — "FYI, at one point I considered setting both `add_tags_to_variants_` and `is_add_namespaced_tags_to_variants` for the new `rfl::AddNamespacedTagsToVariants` to …"

> @bryceschober , I tried doing it myself, but apparently I can't push into the PR. No problem, here's the idea:
>
> ```cpp
>   template <class T, class NamedTupleType>
>   static auto process(NamedTupleType&& _named_tuple) {
>     static_assert(!add_tags_to_variants_ || !add_namespaced_tags_to_variants_,
>                   "You cannot add both rfl::AddTagsToVariants and "
>                   "rfl::AddNamespacedTagsToVariants.");
>     return Processors<Tail...>::template process<T>(
>         Head::template process<T>(std::move(_named_tuple)));
>   }
> ```

[^107]

### 2025-09-10 · comment

Replying to: @bryceschober (opening post) — "As an alternative to #467, I offer this draft proposal. Details: - Add a separate `rfl::AddNamespacedTagsToVariants` processor to support the usage of …"

> @bryceschober , other than the one comment I made, which you can basically just copy + paste, I like this very much. This is basically ready to be merged.

[^108]

- 2025-09-11 · comment · ack: “@bryceschober thank you!” (replying to @bryceschober) [^109]

## #496 the demo runtime exception with ctre

Issue · opened by @warsark · closed (completed)

### 2025-09-12 · comment

Replying to: @warsark (opening post) — "VS2022 17.14.14 WINDOWS 11 ` using Age = rfl::Validator<int, rfl::Minimum<0>, rfl::Maximum<130>>; struct Person { rfl::Rename<'firstName', std::string> …"

> @warsark what version of reflect-cpp and what version of ctre are you using here?

[^110]

### 2025-09-13 · comment

Replying to: @warsark — "I USE VCPKG install them reflect-cpp:0.20 ctre:3.10"

> @warsark , yeah, that's the problem. This is already fixed in the current main branch. I will release reflectcpp 0.21.0 this weekend, but until then, you can downgrade ctre to 3.8.

[^111]

## #497 Update vcpkg

PR · opened by @liuzicheng1987 · merged

### 2025-09-13 · comment

Replying to: @bryceschober — "@liuzicheng1987 I'm curious if you're planning on updating the `jsoncons` dependency to 1.4.0, per the needs of #429, or at least have a test build …"

> @bryceschober OK, will do.

[^112]

### 2025-09-13 · comment

> @bryceschober , I have now updated jsoncons to 1.4.0. It will be part of the 0.21.0 release, which I will draft this weekend.

[^113]

## #499 how to retrive object from variant and taggedunion?

Issue · opened by @warsark · closed (completed)

### 2025-09-13 · comment

Replying to: @warsark (opening post) — "the sample code like this: ` struct Circle { using Tag = rfl::Literal<'circle', 'Circle'>; double radius; }; struct Rectangle { using Tag = …"

> @warsark you have to use the visitor pattern:
>
> http://rfl.getml.com/variants_and_tagged_unions/#the-visitor-pattern
>
> https://en.cppreference.com/w/cpp/utility/variant/visit

[^114]

## #502 Re-add pyproject.tml to fix unknown env error

PR · opened by @awaismirza92 · merged

- 2025-09-15 · comment · ack: “Thanks @awaismirza92 !” (replying to @awaismirza92 (opening post)) [^115]

## #503 Using `Description` marks optionals as required in JSON Schema

Issue · opened by @Laura-Berkers · closed (completed)

### 2025-09-23 · comment

Replying to: @Laura-Berkers (opening post) — "When using the `rfl::Description` wrapper, the JSON Schema generated by rfl does not match the actual JSON data generated by rfl. Given the following …"

> OK, I will take a look. Shoudn't be too hard to fix.

[^116]

## #504 [Proposal] Error.what() rvalue move return

Issue · opened by @BestITUserEUW · closed (completed)

### 2025-09-25 · comment

Replying to: @BestITUserEUW (opening post) — "Motivation: I have a custom error class which currently has to copy Error::what_ which can get expensive with large error messages. Possible Solution: ``` …"

> Looks good to me. Feel free to open a PR.

[^117]

## #505 added explicit ref-qualifiers for Error::what() and a additional over…

PR · opened by @BestITUserEUW · merged

### 2025-09-29 · comment

Replying to: @BestITUserEUW — "@liuzicheng1987 Do you want me to setup tests for Error::what() ?"

> @BestITUserEUW , no, I think that would be overkill. Thanks for your contribution!

[^118]

## #507 use target_compile_definitions for REFLECTCPP_USE_STD_EXPECTED instea…

PR · opened by @BestITUserEUW · merged

- 2025-09-30 · comment · ack: “@BestITUserEUW thanks!” (replying to @BestITUserEUW (opening post)) [^119]

## #514 clang 20 warnings

PR · opened by @KostinPavel · merged

- 2025-10-08 · comment · ack: “@KostinPavel , merged. Thanks for your contribution!” (replying to @KostinPavel (opening post)) [^120]

## #509 Json Schema with Reflector type

Issue · opened by @Wittypal · closed (completed)

### 2025-10-08 · comment

Replying to: @Wittypal (opening post) — "I'm currently using rfl 0.19, but the change notes in the recent versions didnt indicate anything about this behaviour would have changed. I'm working on a Qt …"

> All right, I'll take a look.

[^121]

### 2025-10-19 · comment

> @Wittypal , issue is resolved. Thanks for flagging this!

[^122]

## #515 Cleanup includes

PR · opened by @KostinPavel · closed without merge

- 2025-10-10 · comment · ack: “@KostinPavel , excellent work, thank you so much!” (replying to @KostinPavel (opening post)) [^123]

### 2025-10-10 · inline include/rfl/avro/Writer.hpp:195-199

Replying to: @gemini-code-assist[bot] (inline) — "The null check for `_var.c_str()` results in a silent return, causing data to be dropped without any notification to the caller. This can lead to silent data …"

> @KostinPavel , why do we need this check? How would std::string.c_str() ever be a nullptr?

[^124]

### 2025-10-10 · inline include/rfl/avro/Writer.hpp:137-140

Replying to: @KostinPavel (inline) — "what code will solve the problem?"

> @KostinPavel , handling these errors properly would require us to get rid of all of the noexcept and start throwing exceptions. It would make sense in my view, because these would truly be exceptional circumstances (such as running out of disc space or memory).

[^125]

### 2025-10-10 · comment

Replying to: @KostinPavel — "This is for everyone's benefit. I just removed the unnecessary stuff. I didn't ruin anything. We're all waiting for C++26."

> @KostinPavel , yeah, when C++ 26 is out, I will refactor this library to support both the C++ 26 reflection features, but continue supporting the old C++ 20 approaches.

[^126]

### 2025-10-11 · inline include/rfl/avro/Writer.hpp:137-140

Replying to: @KostinPavel (inline) — "Are exceptions allowed? example: `throw std::runtime_error(std::format('avro_value_append error: {:s}', avro_strerror()));`"

> @KostinPavel , sure. Just remove all the noexcepts so this won't result in a crash.

[^127]

### 2025-10-11 · inline include/rfl/avro/Writer.hpp:137-140

Replying to: @gemini-code-assist[bot] (inline) — "The error handling for `avro_value_append` appears to be incorrect. When `result != 0`, an error has occurred, but the function returns …"

> @KostinPavel , by the way, I am not sure that std::format is supported by all the compiler versions we test for. I think some of the older versions don't support it yet.

[^128]

## #519 Cleanup includes

PR · opened by @KostinPavel · merged

### 2025-10-13 · comment

Replying to: @KostinPavel — "Please let us know what else is needed to merge the branches?"

> @KostinPavel , nothing. I will merge right now. Thank you so much for your effort!

[^129]

## #521 Allow specify schema in capnproto's stream interface

PR · opened by @lixin-wei · merged

- 2025-10-13 · comment · ack: “@lixin-wei , thanks for your contribution!” (replying to @lixin-wei (opening post)) [^130]

## #516 Silent errors, can terminate application

Issue · opened by @KostinPavel · closed (completed)

### 2025-10-13 · comment

Replying to: @KostinPavel (opening post) — "[include/rfl/avro/Writer.hpp](https://github.com/getml/reflect-cpp/pull/515/files/ee7dfe2dec448cfbb406c7e0dc0ae981be71f7ff#diff-7405a3b414d995aba824308efecc5ede …"

> @KostinPavel , I think we can close this now that your PR is merged, right?

[^131]

### 2025-10-25 · comment

Replying to: @KostinPavel — "I think that since we've added exceptions, we need to write negative tests! We might also have to remove noexcept in some other places. We also need to figure …"

> @KostinPavel , I have fixed the remaining two issues, so I am closing this.
>
> Thank you again for your initiative and contribution!

[^132]

## #522 Avro serialization for floating point and integer types failing

Issue · opened by @savannahsalverda · closed (completed)

### 2025-10-14 · comment

Replying to: @savannahsalverda (opening post) — "Using the following code: ``` struct float_struct { float i; }; struct double_struct { double i; }; struct int_struct { int i; }; struct long_struct { long i; …"

> All right, I'll take a look

[^133]

### 2025-10-25 · comment

> @savannahsalverda , this PR (https://github.com/getml/reflect-cpp/pull/532) fixes the issue.
>
> Thank you so much for reporting this. This was a very important issue.

[^134]

## #520 `reflect-cpp` does not generate `.lib` file when building as `shared` on Windows

Issue · opened by @ErniGH · closed (completed)

### 2025-10-14 · comment

Replying to: @BestITUserEUW — "@liuzicheng1987 Are you interested in adding this to your library ?"

> @BestITUserEUW , absolutely. If you want to do this, go ahead.

[^135]

### 2025-10-15 · comment

Replying to: @BestITUserEUW — "The `#include 'rfl/reflectcpp_export.hpp'` inside `rfl.hpp` won’t work. As soon as someone includes a header like `<rfl/json/write.hpp>`, which possibly …"

> @BestITUserEUW , sounds like a great solution. Thank you so much!
>
> @ErniGH , thank you as well for raising the issue and proposing a solution!

[^136]

### 2025-10-15 · comment

> @ErniGH , once we have this implemented should we do a patch release to be used by Conan? Should we do the same thing for sqlgen (that's how this all got started after all...)?

[^137]

### 2025-10-16 · comment

Replying to: @ErniGH — "@liuzicheng1987 Yes, it would be great to add a new release with these changes. As for sqlgen, if it has the same issue, it should be fixed as well"

> @ErniGH ， I'll take care of it.

[^138]

### 2025-10-19 · comment

> @ErniGH , @BestITUserEUW has resolved the issue. Thank you so much again.
>
> I will make a new release for reflect-cpp and submit a PR to Conan.

[^139]

## #523 [json schema] Use non-deprecated $defs; Add $comment option

PR · opened by @AndrewNolte · merged

- 2025-10-15 · comment · ack: “@AndrewNolte thanks for the contribution!” (replying to @AndrewNolte) [^140]

### 2025-10-15 · comment

> @AndrewNolte , if you want a coding assistant based on Gemini, you can try Jules.
>
> https://jules.google/
>
> They give you 15 free sessions a day, which is crazy.

[^141]

## #524 Please bump vendored yyjson to latest (0.12.0) and fix related tests

Issue · opened by @Harry-Chen · closed (completed)

### 2025-10-16 · comment

Replying to: @Harry-Chen (opening post) — "Recently I received a debian bug report [#1117428](https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=1117428) with [detailed error …"

> @Harry-Chen , done. Thanks for flagging this.

[^142]

## #525 Feature/shared exports msvc

PR · opened by @BestITUserEUW · merged

- 2025-10-19 · comment · ack: “@BestITUserEUW , thank you so much for your effort!” (replying to @BestITUserEUW) [^143]

## #529 [minor] Fix include error of `rfl/Literal.hpp`

PR · opened by @lixin-wei · merged

- 2025-10-20 · comment · ack: “@lixin-wei , thanks!” (replying to @lixin-wei (opening post)) [^144]

## #528 Missing Includes

Issue · opened by @BestITUserEUW · closed (completed)

### 2025-10-20 · comment

Replying to: @BestITUserEUW (opening post) — "This issue was introduced in 732e9fe85fcbff2874a51dbe7c2d7b4a944fc078. Reproducable with the following: ```cpp #include <print> #include <string> #include …"

> @BestITUserEUW , yeah, you are right.

[^145]

### 2025-10-20 · comment

> @BestITUserEUW , I have added comprehensive checks to make sure that every header is self-contained. I am not done adapting everything, but this will likely be merged tomorrow.

[^146]

## #530 Issues with deserialization of fixarray using msgpack with reflectcpp 0.21

Issue · opened by @fdila · closed (completed)

### 2025-10-24 · comment

Replying to: @BestITUserEUW — "Have a look at 372345888cd1664ca9e6cce15e99d12f58849b71 and #459. Since v0.20.0 ` std::vector<char>` is being treated as `rfl::Bytestring`"

> @fdila , could you try replacing your `std::vector<char>` with `std::vector<int8_t>`?
>
> I am pretty sure that this would not be interpreted as a byte and therefore `std::vector<int8_t>` is not a bytestring:
>
> https://github.com/getml/reflect-cpp/blob/main/include/rfl/concepts.hpp

[^147]

## #535 Use exceptions inside the writers

PR · opened by @liuzicheng1987 · merged

- 2025-10-27 · inline include/rfl/msgpack/Writer.hpp:66-68 · ack: “Fixed” (replying to @gemini-code-assist[bot] (inline)) [^148]

## #534 Add `rfl::Default`

Issue · opened by @autoantwort · closed (completed)

### 2025-10-27 · comment

Replying to: @autoantwort (opening post) — "In https://github.com/getml/reflect-cpp/issues/163#issuecomment-2334965785 / https://github.com/getml/reflect-cpp/issues/167#issuecomment-2335120005 the …"

> @autoantwort let me think about that. It might be possible to do both.

[^149]

## #538 Avoid assuming that "std::string_view" is nullterminated. Generally it is really not.

PR · opened by @ActuallyaDeviloper · merged

### 2025-11-02 · comment

Replying to: @ActuallyaDeviloper (opening post) — "Hello, when writing my own parser (reader/writer) support, I noticed that the library incorrectly assumes that 'string_view's are nullterminated. They aren't …"

> @ActuallyaDeviloper, in practice, the string are always derived from here:
>
> https://github.com/getml/reflect-cpp/blob/main/include/rfl/internal/StringLiteral.hpp
>
> So actually, it is OK to assume that they are null-terminated. But where you can increase safety without generating unnecessary copies, I think it is a good idea.

[^150]

### 2025-11-02 · comment

> @ActuallyaDeviloper , some of the tests are failing to compile. Could you have a look?

[^151]

### 2025-11-02 · comment

> @ActuallyaDeviloper , out of curiosity: For which format did you want to write your own parser? Would you be willing to contribute it to the library?

[^152]

### 2025-11-02 · comment

Replying to: @ActuallyaDeviloper — "A crap, YAML is also failing because the API I am using is unreleased: …"

> @ActuallyaDeviloper , I agree, it is brittle design.
>
> For Avro I don't really care that your changes introduce overhead. This format is slow AF anyway.
>
> But flexbuf is one of the two fastest formats supported (the other one being msgpack). Once the code fully compiles, we should take a look at the benchmarks to see how much of an impact this makes. Then, we can make a decision how to proceed here.

[^153]

### 2025-11-02 · comment

> > A crap, YAML is also failing because the API I am using is unreleased: https://github.com/jbeder/yaml-cpp/blob/a83cd31548b19d50f3f983b069dceb4f4d50756d/include/yaml-cpp/emitter.h#L210.
> >
> > The [last release](https://github.com/jbeder/yaml-cpp/issues/1360) 0.8 is from 2023 and does not contain that.
>
> Yeah, YAML is fine as well. This is the slowest format we have and no one who uses YAML really cares about speed.

[^154]

## #537 Speed up the reflection

PR · opened by @liuzicheng1987 · merged

### 2025-11-03 · comment

Replying to: @ZXShady — "Any tests that proved it actually improved it?"

> @ZXShady , I have done a lot of profiling with Clang's `-f-time-trace` and to be honest, none of this is going to move the needle very much. You can replace all of the structs by rfl::NamedTuple thus completely eliminating the need for any struct reflection and there wouldn't be much of a change in compile time.
>
> The bottleneck isn't the reflection, it is generating the code for serialization and deserialization. We have already done a lot of optimization there, such as implementing a non-recursive tuple (rfl::Tuple) class rather than using the recursively implemented std::tuple. glaze does something similar, but they use a third-party tuple implementation rather than their own. Same idea nonetheless.
>
> However, I liked your idea of nesting the field extractions as a binary tree of constexpr if. It's easy to do and a clear win for larger structs. So I thought why not do it.

[^155]

### 2025-11-03 · comment

> @ZXShady , I will show you what I mean. This is the profile for the test for the README function. The total duration of the struct reflection is about 92 milliseconds, or less than 5% of the total compile time.
>
> <img width="1330" height="1101" alt="image" src="https://github.com/user-attachments/assets/a04ee1d7-5f61-442b-8ae6-addd7aa9f693" />

[^156]

### 2025-11-03 · comment

> @ZXShady , here is the profile for the `test_many_fields` - test, which is a struct with 70 float fields.
>
> This is obviously a construed example that is unlikely to ever occur in the real world. If you are going to have structs with that many fields, the fields are probably going to be more complex than just floats. In other words, this is probably the greatest share of time spent on struct reflection in any of our tests.
>
> <img width="1330" height="1101" alt="image" src="https://github.com/user-attachments/assets/e77d24c9-79d2-4562-827d-ba017160b210" />
>
> But even for this construed example...we talking about 20-25% of the total compile time. If you can cut that in half, you will have reduced the overall compile time by 10-12% or so.
>
> But this is really an upper bound...in the real world it's going to be a lot less.

[^157]

## #541 Fix missing "hours" when parsing std::chrono::duration

PR · opened by @gibbz00 · merged

- 2025-11-13 · comment · ack: “@gibbz00 thanks!” (replying to @gibbz00 (opening post)) [^158]

## #542 rfl::io::save_string fails silently

Issue · opened by @YaaZ · closed (completed)

### 2025-11-14 · comment

Replying to: @YaaZ (opening post) — "`rfl::io::save_string` catches exceptions and returns their content in the result, but this is not enough. `ofstream::open` fails without throwing an …"

> Thanks for pointing this out.

[^159]

## #543 Why would CBOR be slow in the benchmark?

Issue · opened by @wuyuanyi135 · closed (completed)

### 2025-11-17 · comment

Replying to: @wuyuanyi135 (opening post) — "By plotting the latest [benchmark](https://github.com/getml/reflect-cpp/actions/runs/19347614319) I noticed the CBOR is of the slowest among all protocols. I …"

> Yes, because the underlying library is slow.

[^160]

### 2025-11-17 · comment

> @wuyuanyi135 unfortunately, there simply is no faster library, we've tried a few.

[^161]

## #549 `rfl::Generic`: Allow `int64_t` -> `double` conversion in `to_double`

PR · opened by @Curve · merged

### 2025-11-23 · inline include/rfl/Generic.hpp:142-144

Replying to: @Curve (inline) — "The other conversion functions also call `rfl::error`, so I think this is fine."

> @Curve , yeah, you are right. Gemini sometimes has false positives. This is one of them.

[^162]

- 2025-11-23 · comment · ack: “@Curve , thanks for your contribution!” (replying to @Curve (opening post)) [^163]

## #547 unused variables warnings

PR · opened by @KostinPavel · merged

### 2025-11-23 · inline include/rfl/OneOf.hpp:49-51

Replying to: @KostinPavel (inline) — "The AI suggests: `return std::forward<decltype(_result)>(_result);` Is it correct in this context?"

> @KostinPavel , sorry for the late reply.
>
> I guess technically it's not wrong to use forwarding here, but the underlying value is a const reference, so it will be copied either way. So it doesn't make a difference.

[^164]

### 2025-11-23 · comment

Replying to: @KostinPavel (opening post) — "unused variables warnings"

> @KostinPavel , I am just going to merge it as is. Thank you for the contribution!

[^165]

### 2025-11-23 · comment

> @KostinPavel , I will just remove your comment with the next commit.

[^166]

## #551 msgpack library name error for ubuntu (24.04 and earlier)

Issue · opened by @cooperrunyan · closed (completed)

### 2025-12-03 · comment

Replying to: @cooperrunyan (opening post) — "Apologies if this is hard to follow, I am not super experienced with CMake. I'm on Ubuntu 24.04 and have msgpack installed via `apt install libmsgpack-dev`. …"

> @cooperrunyan , thanks for reporting, I will take a look.

[^167]

### 2025-12-06 · comment

> @cooperrunyan , I actually wasn't able to reproduce the issue, even though I also use Ubuntu 24.04. However, adding msgpackc to the aliases should resolve the issue and as far as I can tell has absolutely no downsides either. So I'm just going to do that.

[^168]

## #557 Any idea why including reflect.h in MSVC results in internal compiler error?

Issue · opened by @Please-just-dont · closed (completed)

### 2025-12-13 · comment

Replying to: @Please-just-dont (opening post) — "This has been here for a really long time, and I didn't file a bug for it, bug I guess I'll have to. Here is simple minimal example, it's literally just an …"

> @Please-just-dont I am pretty sure that there is no file called "reflect.h" in this repository. Where did you get this from?

[^169]

### 2025-12-13 · comment

> @Please-just-dont on the very top of your reflect.h file it says:
>
> The MIT License (MIT)
> //
> // Copyright (c) 2024 Kris Jusiak <kris@jusiak.net>
>
> Mate, you've got the wrong library. I'm not Kris Jusiak.

[^170]

### 2025-12-13 · comment

> @Please-just-dont by the way, I absolutely assure you that the actual reflect-cpp absolutely does compile with MSVC, we have GitHub Actions pipelines set up just for that.

[^171]

## #559 Embedded nulls dropped from JSON strings

Issue · opened by @DRMacIver · open

### 2025-12-17 · comment

Replying to: @DRMacIver (opening post) — "When parsing JSON strings with embedded null characters, reflect-cpp's JSON parser seems to drop the string from the null onwards. Here's an example extracted …"

> @DRMacIver this seems to be a thing with the underlying YYJSON library, I am not sure that there is anything we can do here, because \0 is probably not valid JSON, but I will take a look.

[^172]

### 2025-12-19 · comment

Replying to: @DRMacIver — "'Valid JSON' is complicated (there isn't really a JSON spec, it's all implementation defined) but it seems to be handled fine by at least some JSON parsers - I …"

> @DRMacIver it is possible that they have a flag for this, which you could pass to this library as well.

[^173]

## #560 [Compilation Error] Error when including <windows.h> before <rfl/Result.hpp>

Issue · opened by @NiNeDelixe · open

### 2025-12-17 · comment

Replying to: @NiNeDelixe (opening post) — "### Description There is a compilation error when including Windows headers before the librarys Result.hpp file. The compilation succeeds only when the include …"

> @NiNeDelixe thanks for reporting!

[^174]

### 2025-12-20 · comment

Replying to: @saxbophone — "@liuzicheng1987 I'm interested to get your opinion on whether fixing this in reflect-cpp is in-scope or not? IIRC, the first time I experienced this issue, it …"

> @saxbophone good question. I was thinking about that as well. At the end of the day...it's just one line we need to add, right?

[^175]

## #561 Support std::map<..., std::less<>> (transparent comparator) in JSON read/write

Issue · opened by @RampantDespair · closed (completed)

### 2025-12-21 · comment

Replying to: @RampantDespair (opening post) — "`reflect-cpp` currently supports `std::map<K, V>` with the default comparator, but fails when the map uses a transparent comparator like `std::less<>`. This is …"

> @RampantDespair great idea, should be fairly easy to do. We just need to refine the implementation of rfl::parsing::is_map_like and rfl::parsing::is_map_like_not_multimap

[^176]

### 2026-01-28 · comment

> @RampantDespair , this PR should resolve the issue:
>
> https://github.com/getml/reflect-cpp/pull/590

[^177]

## #558 Improve bad access error for std::expected<T, rfl::Error>

PR · opened by @hos-b · merged

### 2025-12-22 · comment

Replying to: @hos-b — "The [cppreference page](https://en.cppreference.com/w/cpp/utility/expected/bad_expected_access) does not mention anything about the program being ill-formed in …"

> @hos-b thanks for the contribution! Sorry it took me a couple of days to merge it, I was a bit busy these days.

[^178]

## #565 string_to_enum does not support std::string_view

Issue · opened by @mincequi · open

### 2025-12-26 · comment

Replying to: @mincequi (opening post) — "Hi there, i just discovered that reflect-cpp does not seem to support std::string_view to enum conversion. Could this be added? Thanks already."

> @mincequi, yes, I think that should be possible

[^179]

## #567 Compatibility between TaggedUnion and NoExtraFields

Issue · opened by @danielmohansahu · closed (completed)

### 2026-01-03 · comment

Replying to: @danielmohansahu (opening post) — "Apologies if the solution to this problem has been called out in the Documentation. I wasn't able to find anything. --- I'm running into an issue with a simple …"

> No, you are right. This is a mistake, it should be fixed.

[^180]

### 2026-01-11 · comment

> @danielmohansahu , the aforementioned PR should fix the issue. Thanks for bringing this to my attention!

[^181]

## #568 fix: CMake package and imported library target names

PR · opened by @ta7mid · closed without merge

### 2026-01-04 · comment

Replying to: @ta7mid (opening post) — "This PR addresses the following issues encountered/noted when [creating](https://github.com/Homebrew/homebrew-core/pull/258846) a Homebrew formula for …"

> @ta7mid thanks for the PR.
>
> However, it appears that this has broken the Conan build. Could you take another look?

[^182]

### 2026-01-08 · comment

Replying to: @ta7mid — "@liuzicheng1987 Can you check if it works now?"

> @ta7mid , there still appear to be the same issue with Conan...

[^183]

### 2026-02-21 · comment

Replying to: @ta7mid — "I haven't been able to reproduce that error locally. And it's probably not fixed yet, but this time around I've added some log statements to list all imported …"

> @ta7mid , I am closing this PR due to inactivity and since some of the tests do not pass. If you want to continue your work here, feel free to repoen.

[^184]

### 2026-02-22 · comment

Replying to: @ta7mid — "@liuzicheng1987 Can this be reopened?"

> Sure.

[^185]

### 2026-02-22 · comment

> @ta7mid I just reopened the PR. Thanks for continuing to work on this

[^186]

## #569 Fix missing space in static_assert

PR · opened by @cacharle · merged

- 2026-01-09 · comment · ack: “@cacharle thanks!” (replying to @cacharle (opening post)) [^187]

## #571 Hotfix -DREFLECTCPP_USE_STD_EXPECTED=ON

PR · opened by @danielmohansahu · closed without merge

### 2026-01-09 · inline include/rfl/Result.hpp:457

Replying to: @danielmohansahu (inline) — "Hmm, perhaps this indicates I'm just not using a sufficiently new version of GCC?"

> @danielmohansahu I think so.
>
> However, I think we don't have to use this feature here. I will take a closer look tomorrow.

[^188]

### 2026-01-11 · comment

Replying to: @danielmohansahu (opening post) — "Minor change required to locally build with `-DREFLECTCPP_USE_STD_EXPECTED=ON`. Addresses #570 ."

> @danielmohansahu , I am replacing this with #572

[^189]

## #570 Compilation Error with REFLECTCPP_USE_STD_EXPECTED=ON

Issue · opened by @danielmohansahu · closed (completed)

### 2026-01-10 · comment

Replying to: @danielmohansahu (opening post) — "I'm seeing the following compilation error when building `reflect-cpp` v0.23.0 (commit `c241a201fed6386acc7f1484ade4adaaaf75d2f8`) using the …"

> @danielmohansahu , I can reproduce the problem.

[^190]

### 2026-01-11 · comment

> @danielmohansahu , I have checked - we indeed did not support using gcc 13 with std::expected. To be fair, if you want to fully make use of C++ 23 features, you should probably be using gcc 14 or above anyway.
>
> However, I found out that with a few tweaks here and there you can get it to work, so I have done that, see #572

[^191]

## #572 Make sure that GCC 13 also compilies with std::expected; resolves #570

PR · opened by @liuzicheng1987 · merged

- 2026-01-11 · inline include/rfl/Result.hpp:461-464 · ack: “Resolved” (replying to @gemini-code-assist[bot] (inline)) [^192]

## #574 Enable building as a native ROS package

PR · opened by @TomCC7 · merged

- 2026-01-14 · comment · ack: “@TomCC7 thank you!” (replying to @TomCC7 (opening post)) [^193]

## #575 Support std::atomic<T> fields

Issue · opened by @RampantDespair · closed (completed)

### 2026-01-17 · comment

Replying to: @RampantDespair (opening post) — "**Summary** Add native support for reflecting and (de)serializing `std::atomic<T>` fields, where `T` is already supported by reflect-cpp. **Motivation** In …"

> Good idea. Easy to do and would be an improvement.

[^194]

### 2026-01-17 · comment

> @RampantDespair , turns out this is a but trickier than I originally assumed. What I didn't think of is the fact that atomic variables cannot be copied or moved. So something like this cannot work:
>
> ```cpp
> struct Stats {
>     std::atomic<std::uint64_t> bytes_downloaded;
>     std::atomic<bool> finished;
> };
>
> auto stats = rfl::json::read<Stats>(json_string);
> ```
>
> The problem here is that we are need to copy or move the struct in the last line.
>
> The best solution I can think of is something like this:
>
> ```cpp
> struct Stats {
>     std::atomic<std::uint64_t> bytes_downloaded;
>     std::atomic<bool> finished;
> };
>
> auto stats_wrapped = rfl::json::read<rfl::AtomicWrapper<Stats>>(json_string).value();
>
> Stats stats;
> stats_wrapped.assign(&stats);
> ```
>
> Alternatively, it would be incumbent upon the user to define the move constructor:
>
> ```cpp
> struct Stats {
>   std::atomic<std::uint64_t> bytes_downloaded;
>   std::atomic<bool> finished;
>
>   Stats(Stats&& other) noexcept
>       : bytes_downloaded(
>             other.bytes_downloaded.load(std::memory_order_relaxed)),
>         finished(other.finished.load(std::memory_order_relaxed)) {}
>
>   Stats& operator=(Stats&& other) noexcept {
>     bytes_downloaded.store(
>         other.bytes_downloaded.load(std::memory_order_relaxed),
>         std::memory_order_relaxed);
>     finished.store(other.finished.load(std::memory_order_relaxed),
>                    std::memory_order_relaxed);
>     return *this;
>   }
>
>   Stats(const std::uint64_t _bytes_downloaded, const bool _finished)
>       : bytes_downloaded(_bytes_downloaded), finished(_finished) {}
> };
> ```

[^195]

### 2026-01-17 · comment

> @RampantDespair , what are your thoughts on this?

[^196]

### 2026-01-18 · comment

Replying to: @RampantDespair — "@liuzicheng1987 That sounds absolutely reasonable to me 👍 Given the constraints around `std::atomic` not being copyable/movable, I don’t really see a better …"

> @RampantDespair , it's not quite ready for prime time, but this PR resolves the issue:
>
> https://github.com/getml/reflect-cpp/pull/577
>
> There are now two possibilities to handle this, both of which are more elegant than the wrapper:
>
> ```cpp
> struct Stats {
>   std::atomic<std::uint64_t> bytes_downloaded;
>   std::atomic<bool> finished;
> };
>
>  Stats stats{};
>
>  const rfl::Result<rfl::Nothing> res =
>       rfl::json::read<rfl::atomic::remove_atomic_t<Stats>>(json_str).transform(
>           [&](auto&& t) {
>             return rfl::atomic::set_atomic(std::move(t), &stats);
>           });
>
> if(!res) {
>     std::cout << "Error parsing Stats: " << res.error().what() << std::endl;
> }
> ```
>
> The other possibility is to wrap Stats in std::shared_ptr, std::unique_ptr, rfl::Ref, or rfl::Box:
>
> ```cpp
> const auto stats = rfl::json::read<std::shared_ptr<Stats>>(json_str);
> ```
>
> What are your thoughts?

[^197]

### 2026-01-18 · comment

Replying to: @RampantDespair — "@liuzicheng1987 Thanks for the update! One small clarification: from my perspective it doesn’t matter *how* it’s done internally, **as long as the final logic …"

> @RampantDespair, yes, it certainly does that. You can take a look at include/rfl/atomic/is_atomic.hpp:
>
> ```
> template <class T>
> struct is_atomic<std::atomic<T>> {
>   static constexpr bool value = true;
>   using RemoveAtomicT = T;
>   static void set(RemoveAtomicT&& val, std::atomic<T>* _t) {
>     _t->store(std::forward<RemoveAtomicT>(val), std::memory_order_relaxed);
>   };
> };
> ```
>
> And the same is happening for load as well.

[^198]

### 2026-01-18 · comment

Replying to: @RampantDespair — "Amazing and fast work as always! Thanks for the clarification and for taking care of the atomic load/store semantics so thoroughly, really appreciate it! Do …"

> @RampantDespair I will certainly be able to merge into main within the next couple of days.
>
> We can basically do a release whenever we want to...is this something you urgently need?

[^199]

## #576 Provide CMake components for different serialization formats

Issue · opened by @deckstose · open

### 2026-01-18 · comment

Replying to: @deckstose (opening post) — "CMake supports finding specific package components, this is most prominently known with Qt, where you can call `find_package(Qt6 REQUIRED COMPONENTS Widgets)` …"

> @deckstose, I have to admit, I am not entirely sure what you want is to do here.
>
> The library itself absolutely provides that. If you only compile the JSON and TOML features than only those dependencies are installed and required.
>
> I wasn't involved in making the Debian package, and frankly, didn't even know that one existed, so I can't really say much about that.

[^200]

### 2026-01-19 · comment

Replying to: @deckstose — "I suggest to make consuming the library more modular. You're right, but this is only applicable if you control the build process. What solves this problem is …"

> @deckstose, OK, I think that could be done.

[^201]

## #578 Add support for C++ modules

PR · opened by @mikomikotaishi · closed without merge

### 2026-01-19 · comment

Replying to: @mikomikotaishi (opening post) — "This PR adds support for C++ modules, enabled using `REFLECTCPP_BUILD_MODULES`. The main library is part of `rfl`, while each serialisation format is a …"

> @mikomikotaishi awesome PR. Thank you so
> much! I will take a closer look later!

[^202]

### 2026-01-20 · comment

> @mikomikotaishi , this looks great. There is one minor comment regarding your rfl module file.
>
> Also, we need automated tests for this and should update the Github Actions pipeline accordingly. Do you know how to do it or should I do it?

[^203]

### 2026-01-20 · comment

Replying to: @mikomikotaishi — "@liuzicheng1987 Did you leave a comment somewhere in the review? If so, I haven't been able to find it. @GregTheMadMonk If I am understanding correctly, the …"

> I did...I said that most of your includes in the rfl modules are redundant, because they are included in rfl.hpp anyway.

[^204]

- 2026-01-20 · comment · ping: “@mikomikotaishi” [^205]

### 2026-01-20 · comment

Replying to: @mikomikotaishi — "That works for me, I was thinking of that. Also, @liuzicheng1987, any thoughts on prefixing the module name with the organisation (i.e. Java-style), and making …"

> @mikomikotaishi we shouldn't use Java style. The names of the modules should align with namespaces.

[^206]

### 2026-01-20 · comment

> @mikomikotaishi , and by the way I agree with @GregTheMadMonk 's suggestion: There should be a way to only compile what you actually want. Some dependencies can be a bit tedious and forcing people to go through this even though it's completely unnecessary for what they actually want is a problem.
>
> So I like the solution you came up with.

[^207]

### 2026-01-20 · comment

Replying to: @mikomikotaishi — "@liuzicheng1987 This would require fine-tuned configuration, because CMake has to know ahead of time which modules need to be built. Are there already such …"

> @mikomikotaishi , yes, there are flags for that in the CMake:
>
> ```
> option(REFLECTCPP_ALL_FORMATS "Enable all supported formats" OFF)
> option(REFLECTCPP_JSON "Enable JSON support" ON) # enabled by default
> option(REFLECTCPP_AVRO "Enable AVRO support" ${REFLECTCPP_ALL_FORMATS})
> option(REFLECTCPP_BSON "Enable BSON support" ${REFLECTCPP_ALL_FORMATS})
> option(REFLECTCPP_CAPNPROTO "Enable Cap’n Proto support" ${REFLECTCPP_ALL_FORMATS})
> option(REFLECTCPP_CBOR "Enable CBOR support" ${REFLECTCPP_ALL_FORMATS})
> option(REFLECTCPP_CSV "Enable CSV support" ${REFLECTCPP_ALL_FORMATS})
> option(REFLECTCPP_FLEXBUFFERS "Enable flexbuffers support" ${REFLECTCPP_ALL_FORMATS})
> option(REFLECTCPP_MSGPACK "Enable msgpack support" ${REFLECTCPP_ALL_FORMATS})
> option(REFLECTCPP_PARQUET "Enable parquet support" ${REFLECTCPP_ALL_FORMATS})
> option(REFLECTCPP_XML "Enable XML support" ${REFLECTCPP_ALL_FORMATS})
> option(REFLECTCPP_TOML "Enable TOML support" ${REFLECTCPP_ALL_FORMATS})
> option(REFLECTCPP_UBJSON "Enable UBJSON support" ${REFLECTCPP_ALL_FORMATS})
> option(REFLECTCPP_YAML "Enable YAML support" ${REFLECTCPP_ALL_FORMATS})
> ```

[^208]

### 2026-01-20 · comment

Replying to: @mikomikotaishi — "I've decided that I think the best way is just to keep one module, `rfl`, but the `export import` for partitions will be controlled by the build system (put …"

> @mikomikotaishi, sure that works as well.

[^209]

### 2026-01-21 · comment

Replying to: @mikomikotaishi — "@liuzicheng1987 I've applied these changes now"

> @mikomikotaishi , awesome stuff.
>
> We're still missing the tests, though. I don't think we need to test everything, but we should make sure things compile and write some basic smoke tests.
>
> Also, I have just merged the atomic branch, which adds support for atomic variables. The three new functions in include/rfl/atomic/ should be exposed as well.
>
> Do you want do this or should I take care of it?

[^210]

### 2026-01-21 · comment

Replying to: @deckstose — "@mikomikotaishi @GregTheMadMonk https://github.com/getml/reflect-cpp/issues/576 Not sure if modules integrate well with CMake Components"

> @deckstose , that's fine. The modules are just an optional add-on. We continue to support the old structure and it will remain the default for the foreseeable future. @mikomikotaishi did a very good job with his design there.

[^211]

### 2026-01-21 · comment

Replying to: @mikomikotaishi — "Hi, is rfl.module.cppm a mistake? The file is rfl.cppm."

> @mikomikotaishi , I in the process of building the tests. I have fixed two minor issues...however, somehow my pushing into the PR has messed up something about the branch name. I'll have to take close look tomorrow.

[^212]

### 2026-01-21 · comment

> @mikomikotaishi you are right, that is a mistake.

[^213]

### 2026-01-22 · comment

Replying to: @mikomikotaishi — "We should either un-export `rfl::default_value` or remove `static` from its declaration."

> @mikomikotaishi , I tweaked it a bit and tried to add tests, but it won't compile. I have pushed the current status. Do you have any ideas what is going wrong here?

[^214]

### 2026-01-22 · comment

> > Why were the partition exports removed?
>
> Because otherwise I couldn't even get the library to compile.

[^215]

### 2026-01-23 · comment

Replying to: @mikomikotaishi — "@liuzicheng1987 Could you send me the error logs? Also, are you using Clang or GCC?"

> @mikomikotaishi ...sorry about that, it was late at night. Here are the error logs:
>
> If I build without the tests, it works fine (but only after I had removed the partition exports):
>
> ```
> ubuntu@dev:~/reflect-cpp$ rm -r build
> ubuntu@dev:~/reflect-cpp$ cmake -S . -B build -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=Release -DREFLECTCPP_BUILD_MODULES=ON -G Ninja -DCMAKE_CXX_COMPILER=g++-14
> -- The CXX compiler identification is GNU 14.2.0
> -- Detecting CXX compiler ABI info
> -- Detecting CXX compiler ABI info - done
> -- Check for working CXX compiler: /usr/bin/g++-14 - skipped
> -- Detecting CXX compile features
> -- Detecting CXX compile features - done
> -- Configuring done (0.2s)
> -- Generating done (0.0s)
> -- Build files have been written to: /home/ubuntu/reflect-cpp/build
> ubuntu@dev:~/reflect-cpp$ cmake --build build -j4
> [11/11] Linking CXX static library src/modules/libreflectcpp_module.a
> ```
>
> But building with the tests fails:
>
> ```
> ubuntu@dev:~/reflect-cpp$ rm -r build
> ubuntu@dev:~/reflect-cpp$ cmake -S . -B build -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=Release -DREFLECTCPP_BUILD_MODULES=ON -G Ninja -DCMAKE_CXX_COMPILER=g++-14 -DREFLECTCPP_BUILD_TESTS=ON
> -- Running vcpkg install
> Detecting compiler hash for triplet arm64-linux...
> ...
> ubuntu@dev:~/reflect-cpp$ cmake --build build -j4
> [1/18] Building CXX object src/modules/CMakeFiles/reflect-cpp-modules-tests.dir/tests.cpp.o
> FAILED: src/modules/CMakeFiles/reflect-cpp-modules-tests.dir/tests.cpp.o
> /usr/bin/g++-14  -I/home/ubuntu/reflect-cpp/include -O3 -DNDEBUG -std=gnu++23 -MD -MT src/modules/CMakeFiles/reflect-cpp-modules-tests.dir/tests.cpp.o -MF src/modules/CMakeFiles/reflect-cpp-modules-tests.dir/tests.cpp.o.d -o src/modules/CMakeFiles/reflect-cpp-modules-tests.dir/tests.cpp.o -c /home/ubuntu/reflect-cpp/src/modules/tests.cpp
> /home/ubuntu/reflect-cpp/src/modules/tests.cpp:1:1: error: ‘import’ does not name a type
>     1 | import rfl;
> /home/ubuntu/reflect-cpp/src/modules/tests.cpp:1:1: note: C++20 ‘import’ only available with ‘-fmodules-ts’, which is not yet enabled with ‘-std=c++20’
> /home/ubuntu/reflect-cpp/src/modules/tests.cpp:3:1: error: ‘import’ does not name a type
>
> ```
>
> For some reason it doesn't know to add the `-fmodules-ts` flag, which really shouldn't be a problem. Normally CMake knows to do that. When I hardcode that into the CMake, it complains that the "rfl" module doesn't exist.

[^216]

### 2026-01-23 · comment

Replying to: @GregTheMadMonk — "@liuzicheng1987 maybe GCC14 does not support the modern modules toolchain yet (they introduced the required features in either 14 or 15 and I don't remember …"

> @GregTheMadMonk , this error suggests that it cannot find the rfl module and it can't even figure out it's supposed to use modules:
>
> ```
> /media/ssd/projects/cpp/demos/rfl-mod/reflect-cpp/src/modules/tests.cpp:1:1: error: ‘import’ does not name a type
>     1 | import rfl;
> ```
>
> So it's basically the same problem I have with g++ 14. Moreover, I am certain that modules work in principle, because I can easily build this simple example using g++ 14:
>
> https://stackoverflow.com/questions/57300495/how-to-use-c20-modules-with-cmake

[^217]

### 2026-01-23 · comment

Replying to: @GregTheMadMonk — "If I replace `CMAKE_MINIMUM_REQUIRED` version in the toplevel project (not the `src/modules` one) to `3.28`, it gets it to recognize the keyword. Maybe the …"

> @GregTheMadMonk , you are right, this resolves the issue, but then the next one pops up:
>
> ```
> /usr/bin/g++-14  -I/home/ubuntu/reflect-cpp/include -O3 -DNDEBUG -std=gnu++23 -MD -MT src/modules/CMakeFiles/reflect-cpp-modules-tests.dir/tests.cpp.o -MF src/modules/CMakeFiles/reflect-cpp-modules-tests.dir/tests.cpp.o.d -fmodules-ts -fmodule-mapper=src/modules/CMakeFiles/reflect-cpp-modules-tests.dir/tests.cpp.o.modmap -MD -fdeps-format=p1689r5 -x c++ -o src/modules/CMakeFiles/reflect-cpp-modules-tests.dir/tests.cpp.o -c /home/ubuntu/reflect-cpp/src/modules/tests.cpp
> In file included from /usr/include/features.h:502,
>                  from /usr/include/aarch64-linux-gnu/c++/14/bits/os_defines.h:39,
>                  from /usr/include/aarch64-linux-gnu/c++/14/bits/c++config.h:680,
>                  from /usr/include/c++/14/bits/requires_hosted.h:31,
>                  from /usr/include/c++/14/string:38,
>                  from /home/ubuntu/reflect-cpp/src/modules/tests.cpp:3:
> /usr/include/aarch64-linux-gnu/bits/wchar2.h:24:1: error: redefinition of ‘wchar_t* wmemcpy(wchar_t*, const wchar_t*, size_t)’
>    24 | __NTH (wmemcpy (wchar_t *__restrict __s1, const wchar_t *__restrict __s2,
>       | ^~~~~
> In file included from /usr/include/features.h:502,
>                  from /usr/include/aarch64-linux-gnu/c++/14/bits/os_defines.h:39,
>                  from /usr/include/aarch64-linux-gnu/c++/14/bits/c++config.h:680,
>                  from /usr/include/c++/14/type_traits:38,
>                  from /usr/include/c++/14/bits/stl_pair.h:60,
>                  from /usr/include/c++/14/tuple:38,
>                  from /home/ubuntu/reflect-cpp/include/rfl/AddStructName.hpp:4,
>                  from /home/ubuntu/reflect-cpp/include/rfl.hpp:10,
>                  from /home/ubuntu/reflect-cpp/src/modules/rfl.cppm:3,
> of module rfl, imported at /home/ubuntu/reflect-cpp/src/modules/tests.cpp:1:
> /usr/include/aarch64-linux-gnu/bits/wchar2.h:24:1: note: ‘wchar_t* wmemcpy(wchar_t*, const wchar_t*, size_t)’ previously defined here
> ```
>
> Apparently this is because we are importing header files into the module and `import std` won't work for whatever reason. Maybe I do need to switch to g++-15.

[^218]

### 2026-01-23 · comment

> @mikomikotaishi @GregTheMadMonk , so I have bad news. Apparently we need https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p1815r2.html, which won't be implemented until g++-16. g++-16 has not been published yet.
>
> https://gcc.gnu.org/projects/cxx-status.html
>
> In other words, it is unlikely we will get this to run on g++ until module support is fully implemented.

[^219]

### 2026-01-23 · comment

> @mikomikotaishi @GregTheMadMonk , but the good news are that it works like a charm on Clang 20 after I removed the static from rfl::default_value.
>
> Here's what I did:
>
> ```
> cmake -S . -B build -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=Release -DREFLECTCPP_BUILD_MODULES=ON -G Ninja -DCMAKE_CXX_COMPILER=clang++-20 -DREFLECTCPP_BUILD_TESTS=ON
> cmake --build build -j4
> ./build/src/modules/reflect-cpp-modules-tests
> ```
>
> @GregTheMadMonk I think if you try to recompile your project with Clang, it should work.

[^220]

### 2026-01-24 · comment

> @GregTheMadMonk , @mikomikotaishi , I just hit a bug in Clang:
>
> ```
> ubuntu@dev:~/reflect-cpp$ cmake --build build -j4
> [0/2] Re-checking globbed directories...
> [580/1173] Building CXX object tests/modules/CMakeFiles/reflect-cpp-modules-tests.dir/test_cbor.cpp.o
> FAILED: tests/modules/CMakeFiles/reflect-cpp-modules-tests.dir/test_cbor.cpp.o
> /usr/bin/clang++-20 -DBSON_STATIC -DREFLECTCPP_BSON -DREFLECTCPP_CBOR -DREFLECTCPP_FLEXBUFFERS -DREFLECTCPP_JSON -DREFLECTCPP_MSGPACK -DREFLECTCPP_TOML -DREFLECTCPP_UBJSON -DREFLECTCPP_XML -DREFLECTCPP_YAML -DYAML_CPP_STATIC_DEFINE -I/home/ubuntu/reflect-cpp/PUBLIC -I/home/ubuntu/reflect-cpp/include -I/home/ubuntu/reflect-cpp/include/rfl/thirdparty -I/home/ubuntu/reflect-cpp/build/include -isystem /home/ubuntu/reflect-cpp/build/vcpkg_installed/arm64-linux/include -O3 -Wall -Werror -ggdb -ftemplate-backtrace-limit=0 -O3 -DNDEBUG -std=gnu++23 -DTOML_HEADER_ONLY=0 -MD -MT tests/modules/CMakeFiles/reflect-cpp-modules-tests.dir/test_cbor.cpp.o -MF tests/modules/CMakeFiles/reflect-cpp-modules-tests.dir/test_cbor.cpp.o.d @tests/modules/CMakeFiles/reflect-cpp-modules-tests.dir/test_cbor.cpp.o.modmap -o tests/modules/CMakeFiles/reflect-cpp-modules-tests.dir/test_cbor.cpp.o -c /home/ubuntu/reflect-cpp/tests/modules/test_cbor.cpp
> PLEASE submit a bug report to https://github.com/llvm/llvm-project/issues/ and include the crash backtrace, preprocessed source, and associated run script.
> ...
> ```
>
> How should we proceed here?
>
> I mean...we got pretty far and we've done a lot of work that will be useful in the future here. However, it appears to me that C++ modules are not quite ready for prime time yet. (Not this PR...just C++ modules in general.)
>
> Maybe we should keep the branch open, but wait for a while until we merge it into main. What are your thoughts?

[^221]

### 2026-01-24 · comment

Replying to: @mikomikotaishi — "Is there any more information with the failure? Is it a compiler crash? Why not try Clang 21? Maybe it could resolve whatever the bug is here. I would support …"

> @mikomikotaishi , yes, it is a compiler crash.
>
> To reproduce:
>
> ```
> ubuntu@dev:~/reflect-cpp$ rm -r build
> ubuntu@dev:~/reflect-cpp$ cmake -S . -B build -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=Release -DREFLECTCPP_BUILD_MODULES=ON -G Ninja -DCMAKE_CXX_COMPILER=clang++-20 -DREFLECTCPP_BUILD_TESTS=ON -DREFLECTCPP_MSGPACK=ON -DREFLECTCPP_BSON=ON -DREFLECTCPP_CBOR=ON -DREFLECTCPP_FLEXBUFFERS=ON -DREFLECTCPP_UBJSON=ON -DREFLECTCPP_XML=ON -DREFLECTCPP_TOML=ON -DREFLECTCPP_YAML=ON
> ubuntu@dev:~/reflect-cpp$ cmake --build build -j4
> ```
>
> There is a pretty long stack trace and it appears that the issue is somehow related to the CBOR tests:
>
> ```
> PLEASE ATTACH THE FOLLOWING FILES TO THE BUG REPORT:
> Preprocessed source(s) and associated run script(s) are located at:
> clang++-20: note: diagnostic msg: /tmp/test_cbor-06b210.cpp
> clang++-20: note: diagnostic msg: /tmp/test_cbor-06b210.sh
> clang++-20: note: diagnostic msg:
> ```

[^222]

### 2026-01-25 · comment

Replying to: @GregTheMadMonk — "Can't confirm compiler crashing on Clang 20 (`clang version 20.1.8`) There are regular C++ errors, but no compiler crash: <details><summary>Error log</summary> …"

> @GregTheMadMonk , hmm...these errors appear to be emanating from the jsoncons library, which we use as the underlying library for CBOR and UBJSON. However, it must have something to with modules. When compile the exact same thing, but without modules, it works just fine, no compiler crash, no errors:
>
> ```cpp
> ubuntu@dev:~/reflect-cpp$ cmake -S . -B build -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=Release -G Ninja -DCMAKE_CXX_COMPILER=clang++-20 -DREFLECTCPP_BUILD_TESTS=ON -DREFLECTCPP_MSGPACK=ON -DREFLECTCPP_BSON=ON -DREFLECTCPP_CBOR=ON -DREFLECTCPP_FLEXBUFFERS=ON -DREFLECTCPP_UBJSON=ON -DREFLECTCPP_XML=ON -DREFLECTCPP_TOML=ON -DREFLECTCPP_YAML=ON
> ...
> ubuntu@dev:~/reflect-cpp$ cmake --build build -j4
> [0/2] Re-checking globbed directories...
> [1131/1131] Linking CXX executable tests/yaml/reflect-cpp-yaml-tests
> ```

[^223]

### 2026-01-26 · comment

> @mikomikotaishi , @GregTheMadMonk , I have slept over it and have decided that I don't want to merge this right now, just because there are too many weird compiler issues and I just don't feel comfortable committing to maintaining these kind of things for now.
>
> However, I am very excited about this and hope that C++ modules will soon reach a level of maturity that allows us to merge this work.
>
> I have moved this over into the branch https://github.com/getml/reflect-cpp/tree/f/modules and hope that I will be able to merge this branch later this year.
>
> So thank you again for all of your great work and great input and I am very sorry that I do not feel comfortable merging this for now. But I will do so when the time is right.

[^224]

### 2026-09-02 · comment

Replying to: @mikomikotaishi — "@liuzicheng1987 Any chance we could re-investigate this feature again?"

> @mikomikotaishi absolutely. If we can get it to work reliably, let's go for it. There is an open PR, but there hasn't been any activity there for quite a while.

[^225]

## #580 build(cmake): add and install cmake version file

PR · opened by @deckstose · merged

- 2026-01-24 · comment · ack: “@deckstose , thanks for the contribution!” (replying to @deckstose (opening post)) [^226]

## #581 Make REFLECTCPP_USE_BUNDLED_DEPENDENCIES option consistent with enchantum

PR · opened by @DNKpp · closed without merge

### 2026-01-25 · comment

Replying to: @DNKpp (opening post) — "Hello, I'm currently reviewing whether I can adapt reflect-cpp for the project I'm working on. Unfortunately, the integration does not work as of now, because …"

> @DNKpp , thanks for your PR!
>
> However, the problem is that enchantum is not on Conan. If I accept this PR, it will for sure break our Conan package.

[^227]

### 2026-01-25 · comment

> @DNKpp , it appears to me that the pragmatic solution is to have a separate flag for whether or not we want to used the bundled enchantum.

[^228]

### 2026-01-26 · comment

Replying to: @DNKpp — "@liuzicheng1987 I think a separate option for each bundled dependency would be the best. I'm certain there are already lots of CTRE users out there, while …"

> @DNKpp , that means we would have to tweak the existing recipes. So I rather just have a separate flag for enchantum.
>
> The difference between CTRE and YYJSON and enchantum is that the former two can be found on all relevant package managers. So if we do not include a bundled version, there is always an alternative. Enchantum, on the other hand , is not found on all relevant package managers, so we need to treat it differently.

[^229]

### 2026-01-26 · comment

> @DNKpp , yes, all of the pipeline failures are related to the Conan issue.

[^230]

### 2026-02-21 · comment

Replying to: @ZXShady — "I should get myself to work on Conan, I made vcpkg but haven't updated it to latest version."

> @DNKpp, I am closing this PR due to inactivity and since some of the tests do not pass. If you want to continue your work here, feel free to reopen this PR.

[^231]

## #582 Fix some unused parameters warnings

PR · opened by @makekryl · merged

### 2026-01-26 · comment

Replying to: @makekryl (opening post) — ""

> @makekryl , thanks for the PR.
>
> In these three instances, instead of writing [maybe_unused], could you simply get rid of the variable name of the unused parameter.
>
> So instead of
>
> `([maybe_unused] auto&& _var)`
>
> Just type
>
> `(auto&&)`
>
> That is what we do in the rest of the codebase and it is also more concise.

[^232]

### 2026-01-26 · comment

Replying to: @makekryl — "I thought `[[maybe_unused]]` is preferred, as it was introduced to some other pieces of code in a similar PR (#547), it also seems quite less misleading than …"

> @makekryl , sure, go ahead.

[^233]

### 2026-01-27 · comment

> @makekryl , looks great. I will wait for the tests, then merge. Thanks for your contribution!

[^234]

### 2026-01-27 · comment

Replying to: @makekryl — "Seems like one job failed due to vcpkg related infra problem :/"

> @makekryl , yes, it happens. I'll just rerun the test. No worries.

[^235]

## #584 Support parsing arbitrary duration representations

PR · opened by @vittorioromeo · merged

### 2026-01-26 · comment

Replying to: @vittorioromeo (opening post) — "Attempt to fix #583"

> @vittorioromeo , thanks for the PR. I agree with Gemini though, that you should also add a test. Just for JSON is fine...testing it for all formats would be redundant.

[^236]

### 2026-01-26 · comment

> @vittorioromeo , if you are unsure how to add tests to our pipeline, don't hesitate to ask, I can help you.

[^237]

- 2026-01-27 · comment · ack: “@vittorioromeo I am merging. Thanks for the contribution!” (replying to @vittorioromeo) [^238]

## #587 Fix `rfl::DefaultVal` with `rfl::Rename`

Issue · opened by @cacharle · closed (completed)

### 2026-01-27 · comment

Replying to: @cacharle (opening post) — "I have the following toml file: ```toml foo = true ``` and cpp code: ```cpp struct Config { rfl::DefaultVal<rfl::Rename<'foo', bool>> Foo = false; }; auto …"

> @cacharle it is possible that wrapping rename in default_var voids the rename. If so, it's certainly not specific to TOML and something we should take a look at.
>
> For the time being, you could use the DefaultIfMissing preprocessor instead of DefaultVar.

[^239]

### 2026-01-27 · comment

> @cacharle I have taken a look at this and found that it is quite easy to fix.
>
> https://github.com/getml/reflect-cpp/pull/589
>
> Basically, after the PR is merged, you will be able to write this:
>
> ```cpp
> struct Config {
>     rfl::Rename<"foo", rfl::DefaultVal<bool>> Foo = false;
> };
> ```
>
> This will then work as expected.

[^240]

## #588 Add camel case to snake case conversion

PR · opened by @cacharle · merged

### 2026-01-27 · inline include/rfl/internal/transform_case.hpp:68-73

Replying to: @cacharle (inline) — "FirstName is pascal case not camel case, why am I getting reviewed by a bot?"

> It often points out useful things. In this case, I agree, it's a false positive.

[^241]

- 2026-01-27 · review (COMMENTED) · ack: “Looks good to me.” [^242]
- 2026-01-27 · comment · ack: “@cacharle thanks for the contribution. I am merging.” (replying to @cacharle (opening post)) [^243]

### 2026-01-28 · comment

Replying to: @cacharle — "Just realized I forgot to add anything to the doc"

> Right...if you want to, you can open a new PR for the doc, otherwise I'll just quickly write something myself.

[^244]

## #564 MSVC Toml std::array not working

Issue · opened by @HaydnTrigg · open

### 2026-01-28 · comment

Replying to: @HaydnTrigg (opening post) — "I have the following code snippet, the json version works but the toml version breaks. ```cpp #include <rfl/json.hpp> #include <rfl/toml.hpp> #include …"

> Hi @HaydnTrigg ,
>
> I can reproduce the problem. I think the error message is fairly clear:
>
> The underlying toml++ library simply does not support the type "char". What is it you are trying to accomplish? What did you expect to see? An array of chars or a string?

[^245]

## #591 Custom class `ReflectionType` cannot be directly in the class on Linux

Issue · opened by @cacharle · closed (completed)

### 2026-01-29 · comment

Replying to: @cacharle (opening post) — "I'm hitting an `evaluation depth exceeds maximum of 512` error on Linux when I try to compile the following: ```cpp class Foo { public: Foo() = default; struct …"

> @cacharle , I would argue that it does say in the documentation that you have to use typedef or using:
>
> <img width="1167" height="420" alt="Image" src="https://github.com/user-attachments/assets/6035dbd9-f8d9-4af6-8aef-dc806f9a8907" />
>
> However, we could include a sentence like "Note that directly declaring a child struct called ReflectionType will NOT work."

[^246]

## #592 Issue with parsing string validation and default fields in the same nested struct

Issue · opened by @devarsirawal · closed (completed)

### 2026-01-29 · comment

Replying to: @devarsirawal (opening post) — "I'm getting an interesting failure when trying to parse structs that have fields with string validation and default values on v0.23.0. For example, when trying …"

> @devarsirawal
> Yeah, that makes sense...you need to set an explicit default value for your UUIDv4

[^247]

### 2026-01-29 · comment

Replying to: @devarsirawal — "Ahhh that would do it... That makes sense for the `rfl::DefaultIfMissing` processor case, but why do I need to specify an explicit default value for `uuid` in …"

> @devarsirawal , the problem is that in both cases the deserialization works by first constructing a default-initialized struct and then filling it with the values from the serialized data. I don't really see a technical way around it, TBH...the only thing we can do is document it better.

[^248]

## #552 Better support for custom schemas

Issue · opened by @FrancescoMerenda · open

### 2026-01-29 · comment

Replying to: @FrancescoMerenda (opening post) — "I am exploring using this library to generate [Zod](https://zod.dev/api) Schemas (which is a Typescript validation library). The current JSON Schema generation …"

> Hi @FrancescoMerenda , sorry for the late reply.
>
> I have actually used Zod for frontend project I have written. Generally speaking, schema generation in this library works as follows: First, we translate the types into an abstract schema, defined here:
>
> https://github.com/getml/reflect-cpp/blob/main/include/rfl/parsing/schema/Definition.hpp
> https://github.com/getml/reflect-cpp/blob/main/include/rfl/parsing/schema/Type.hpp
>
> Then, this abstract definition is translated into whatever schema we would like. That could be a Zod schema.
>
> As far as I know, Zod even supports an experimental API for JSON schemata:
>
> https://zod.dev/json-schema

[^249]

### 2026-01-30 · comment

Replying to: @FrancescoMerenda — "Thank you for your reply @liuzicheng1987 . That's exactly what I did. However, my point is that the current abstract schema is too broad and seems to be …"

> @FrancescoMerenda we use this for more than just the JSON Schema. For instance, we also generate schemata for Avro or Cap'n Proto.
>
> The concept of a tagged union never came up, but that is something we could add to the Schema.

[^250]

### 2026-01-30 · comment

> @FrancescoMerenda as far as enums are concerned, you can just use Literal.

[^251]

## #594 Add benchmark comparisons to other solutions in the README.md?

Issue · opened by @ghost · closed (completed)

### 2026-02-02 · comment

Replying to: @ghost (opening post) — "Like this: https://github.com/stephenberry/glaze?tab=readme-ov-file#performance BTW, where can I ask for newbie questions? README.md mentions Github …"

> @Curculigo the benchmarks are run automatically via a GitHub Actions pipelines and we always publish the results. This compares reflect-cpp to other solutions as well as the different formats. We provide links to that in the README.
>
> I think the problem with glaze's benchmarks is that they are basically run on Stephen Berry's PC, and they are static, whereas our benchmarks are run on different operating systems and architectures and adapt to every change to the code.
>
> I think our approach is much better. If we write them into the README, they will become static.

[^252]

### 2026-02-02 · comment

Replying to: @ghost — "@liuzicheng1987 How to have reflect-cpp to errors on unknown keys (the fields I have not included in the struct) like glaze? Currently, it will happily ignore …"

> @Curculigo you can check the documentation. If you think that something is missing, just open an issue.
>
> As far as your question is concerned, you can use the rfl::NoExtraFields processor:
>
> https://rfl.getml.com/concepts/processors/#supported-processors

[^253]

## #595 Inconsistency in rfl::Variant visit overloads prevents stateful visitors

Issue · opened by @FrancescoMerenda · closed (completed)

### 2026-02-03 · comment

Replying to: @FrancescoMerenda (opening post) — "I'm using rfl::Variant and I noticed it is currently not possible to use a stateful visitor even when passing it as non-const reference to visit. Example: …"

> @FrancescoMerenda I would agree that this is a bug. A PR is very welcome!

[^254]

## #596 Allow usage of stateful visitors inside rfl::Variant

PR · opened by @FrancescoMerenda · merged

- 2026-02-04 · comment · ack: “@FrancescoMerenda thank you for the contribution!” (replying to @FrancescoMerenda (opening post)) [^255]

## #597 Feature Request: Add support for serializing comments

Issue · opened by @Lazrius · closed (completed)

### 2026-02-05 · comment

Replying to: @Lazrius (opening post) — "Good evening. I know such things have been discussed before, but I couldn't find in the issue a history a consensus on the idea of supporting comments. The …"

> @Lazrius it has come up before, but so far we haven't prioritised it, because it only affects two formats.
>
> But basically my idea would be this: We could implement an rfl::Comment wrapper around std::string.
>
> XML and YAML would serialise this as comments, all other formats would treat it like normal strings.
>
> What are your thoughts?

[^256]

### 2026-02-08 · comment

Replying to: @Lazrius — "I think that is a great idea. I would maybe add a prefix or something to non-XML/non-YAML like `__comment` or the like just to be very explicit about it, but …"

> @Lazrius , this PR should resolve the issue:
>
> https://github.com/getml/reflect-cpp/pull/602
>
> I have decided that it is better to go with `rfl::Commented<T>` as a field wrapper rather than `rfl::Comment`, because the latter solution would create many dead fields and the former allows us to tie comments directly to fields.
>
> Here's the documentation:
>
> https://github.com/getml/reflect-cpp/blob/f/comments/docs/commented.md

[^257]

### 2026-02-08 · comment

Replying to: @Lazrius — "This looks fantastic. I'll have a clone of it tomorrow and see how it fairs, but this seems very good to me; much better than anything I'd have come up with! …"

> @Lazrius , if we are going to add the asterisk, we should add it to all relevant wrappers (there are quite a few at this point). I am not opposed to it, but that should be a different PR, in my view.

[^258]

### 2026-02-08 · comment

Replying to: @Lazrius — "Fair enough, that makes sense to me! Thanks again for the good work."

> @Lazrius , I have written a separate PR that would add operator* to all relevant classes. (More precisely, I had a coding agent do that for me...they tend to be quite good at these tedious tasks). So now I will add it to rfl::Commented as well:
>
> https://github.com/getml/reflect-cpp/pull/603

[^259]

## #598 Update error message for unused values

Issue · opened by @BugelNiels · closed (completed)

### 2026-02-05 · comment

Replying to: @BugelNiels (opening post) — "First of all, great library; it allows us to simplify our config parsing significantly. I have one change request: currently when you set `rfl::NoExtraFields` …"

> Yeah, this is obviously a mistake. A PR to remove it is very welcome!

[^260]

- 2026-02-05 · comment · ping: “@BugelNiels” [^261]

## #599 Remove reference to internals in error message for unused values

PR · opened by @BugelNiels · merged

- 2026-02-07 · comment · ack: “@BugelNiels thanks for the contribution!” (replying to @BugelNiels (opening post)) [^262]

## #607 Set file name and license in RPM CPack options

PR · opened by @BugelNiels · merged

- 2026-02-10 · comment · ack: “@BugelNiels thanks for the contribution!” (replying to @BugelNiels (opening post)) [^263]

## #608 Why not give a default constructor to TaggedUnion?

Issue · opened by @sunwayking · closed (completed)

### 2026-02-11 · comment

Replying to: @sunwayking (opening post) — "Putting TaggedUnion in STL container, e.g. std::map , leaking of default constructor may cause compiling errors. ` [build] …"

> @sunwayking , OK.

[^264]

## #606 Improve invalid enum error message

PR · opened by @BugelNiels · merged

- 2026-02-11 · comment · ack: “@BugelNiels Merged. Thanks for the contribution!” (replying to @BugelNiels) [^265]

## #610 Json Schema: Add enum value descriptions

PR · opened by @AndrewNolte · closed without merge

### 2026-02-11 · comment

Replying to: @AndrewNolte (opening post) — ""

> @AndrewNolte it appears that a lot of the tests are failing to compile. Could you take a look?

[^266]

### 2026-02-21 · comment

> @AndrewNolte, I am closing this PR due to inactivity and since some of the tests do not pass. If you want to continue your work here, feel free to reopen the PR.

[^267]

## #609 Bug or possible regression with custom rfl::Reflectors

Issue · opened by @FrancescoMerenda · closed (completed)

### 2026-02-12 · comment

Replying to: @FrancescoMerenda (opening post) — "After upgrading from 0.22 to 0.23 custom Reflectors for enum types (and basic types as well, but I'm not sure if that was possible before) are no longer …"

> @FrancescoMerenda , OK, I'll take a look.

[^268]

### 2026-02-21 · comment

> @FrancescoMerenda , you are right, there is now a specialized parser for enums which has to be updated accordingly to make this possible. So far, the idea that people might use reflectors for enums hasn't really crossed my mind, but it is a possibility.
>
> If you want to submit a PR with your suggested updates and a little test, that would be more than welcome!

[^269]

## #613 Add CLI argument parser (rfl::cli::read)

PR · opened by @Centimo · merged

### 2026-02-14 · inline include/rfl/Positional.hpp:18

Replying to: @gemini-code-assist[bot] (inline) — "The default constructor `Positional() : value_(Type()) {}` does not have a `requires std::is_default_constructible_v<Type>` constraint. If `Type` is not …"

> @Centimo you can do this if you want to but I personally don't care. Feel free to just ignore Gemini on this.

[^270]

### 2026-02-14 · comment

Replying to: @Centimo (opening post) — "## Summary - Add `rfl::cli::read<T>(argc, argv)` — parse command-line arguments into any reflectable struct using reflection - Automatic `snake_case` → …"

> @Centimo, This is awesome stuff! I just read through the code a bit and I love it. I'm going to give it a try later but I like what I'm seeing so far.

[^271]

### 2026-02-17 · inline include/rfl/cli/resolve_args.hpp:1

> @Centimo minor nitpick: We usually use "internal" instead of "detail" in this library.

[^272]

### 2026-02-17 · inline include/rfl/Positional.hpp:1

> @Centimo you probably forked before we did this, but we recently added operator* as an alternative to .value(), .get() and operator(). Moreover, we made sure that there is a non-const .get() for all classes. These changes do not seem to be reflected here. Could you add that?

[^273]

### 2026-02-17 · inline include/rfl/Short.hpp:1

> @Centimo you probably forked before we did this, but we recently added operator* as an alternative to .value(), .get() and operator(). Moreover, we made sure that there is a non-const .get() for all classes. These changes do not seem to be reflected here. Could you add that?

[^274]

### 2026-02-17 · comment

Replying to: @Centimo — "I'm planning on making a few more fixes, so you probably shouldn't rush into a full-blown review."

> @Centimo , first of all, great job! Thank you so much.
>
> There are a couple of minor review comments, which should take you like 10 minutes to resolve. Very minor stuff.
>
> Also, could you update the README.md as well? This is a significant and very cool new functionality that should be highlighted in the README.

[^275]

### 2026-02-21 · review (COMMENTED)

> @Centimo , beautiful, no further comments. Thank you so much!
>
> In my view, this is ready to be merged. Do you agree?

[^276]

- 2026-02-21 · comment · ack: “@Centimo merged. Thank you for this great contribution!” (replying to @Centimo) [^277]

## #614 Can't parse XML into a rfl::Generic

Issue · opened by @JX75 · closed (completed)

### 2026-02-16 · comment

Replying to: @JX75 — "In xml/Reader.hpp: rfl::xml::Reader::to_basic_type<bool>(const InputVarType _var) returns a boolean equal to true if the text content of the node is 'true', …"

> @JX75 thanks for reporting, I will take a look

[^278]

### 2026-03-07 · comment

> Hi @JX75 ,
>
> I have taken a look at the issue...after much consideration, I have found that it is practically impossible to read XML into generics without any form of pre-defined structure. Because of the nature of the format, there are just too many edge cases with extremely weird behaviour.
>
> (Writing generics to XML is fine, though)
>
> Therefore, I have decided to add a compile-time error and document the behaviour.

[^279]

## #615 std::atomic<T> Reflector bypasses domain validation present on bare T (#575 followup)

Issue · opened by @RampantDespair · closed (completed)

### 2026-02-19 · comment

Replying to: @RampantDespair (opening post) — "Following up on #575, now that `std::atomic<T>` support has been implemented, I discovered a validation inconsistency between bare `T` and `std::atomic<T>` …"

> OK, I'll take a look

[^280]

### 2026-02-21 · comment

> @RampantDespair , I cannot reproduce this behaviour and I don't think the problem is related to atomic variables. Here is what I did:
>
> ```cpp
> #include <gtest/gtest.h>
>
> #include <atomic>
> #include <cstdint>
> #include <rfl.hpp>
> #include <rfl/json.hpp>
>
> namespace test_atomic_negative {
>
> struct TestObject {
>   std::uint64_t number{0};
> };
>
> struct AtomicTestObject {
>   std::atomic<std::uint64_t> number{0};
> };
>
> TEST(json, test_atomic_negative) {
>   auto req1 = rfl::json::read<TestObject>(R"({"number": -1})");
>   EXPECT_FALSE(req1.has_value());
>
>   auto req2 = rfl::json::read<rfl::Ref<AtomicTestObject>>(R"({"number": -1})");
>   EXPECT_FALSE(req2.has_value());
> }
>
> }  // namespace test_atomic_negative
> ```
>
> Here is what I get:
>
> ```
> /home/ubuntu/reflect-cpp/tests/json/test_atomic_negative.cpp:20: Failure
> Value of: req1.has_value()
>   Actual: true
> Expected: false
>
> /home/ubuntu/reflect-cpp/tests/json/test_atomic_negative.cpp:23: Failure
> Value of: req2.has_value()
>   Actual: true
> Expected: false
> ```

[^281]

### 2026-02-21 · comment

> @RampantDespair , I have found the problem and implemented a fix.

[^282]

### 2026-02-21 · comment

Replying to: @RampantDespair — "@liuzicheng1987 I'm curious why did it work the first time for you? Is it environment specific? I'm on Windows x64."

> I'm on Linux arm64

[^283]

## #616 reflect-cpp in Fedora/EPEL

Issue · opened by @BugelNiels · open

### 2026-02-20 · comment

Replying to: @BugelNiels (opening post) — "Are there any plans to put this library in Fedora and/or EPEL? Currently, it seems that the only C++ reflection library available in EPEL is …"

> @BugelNiels I know that there are  packages for some Linux distros out there, but these have been created without my involvement. I only learned about their existence by accident.
>
> So I guess the answer is that I have no idea whether anyone is planning to do this, but if you want to take it up, feel free to do so.

[^284]

## #619 Question: When using rfl::Variant with the msgpack processor, is AddTagsToVariants nessasary?

Issue · opened by @Lazrius · closed (completed)

### 2026-02-23 · comment

Replying to: @Lazrius (opening post) — "Hey again. I was wondering when using the msgpack processor, is it required (read: optimal) to use `rfl::AddTagsToVariants` when using only primitive/built in …"

> @Lazrius the short answer is that I would almost certainly use the untagged variant here.
>
> Long answer: The tags are meant for complex structs, where they can offer a real optimisation. Instead of having to read all the fields and seeing whether they match the struct, the parser can simply read the tag and match that, which is a lot faster.
>
> It also leads to better error messages. Without tags, error messages read like this:  Couldn't match option 1, missing field A and B. Couldn't match option 2, missing field C, D and E, etc. With tags, error messages are a lot easier to decipher.
>
> But none of that applies when it's just primitive types. The only catch is that the program might convert the types when reading (but certainly not string to int or vice versa).

[^285]

## #621 Allows usage of custom Reflector for enums types

PR · opened by @FrancescoMerenda · merged

### 2026-02-24 · comment

Replying to: @FrancescoMerenda (opening post) — "This PR allows the usage of custom Reflector for enums types. It should now be possible to correctly use reflector to read, write and defines schemas for this …"

> @FrancescoMerenda there still seem to be some issues on macOS. Could you take a look?

[^286]

- 2026-02-25 · comment · ack: “@FrancescoMerenda , thanks for the contribution.” [^287]

## #618 Fix 30 bugs found during code review

PR · opened by @Centimo · merged

### 2026-02-24 · comment

Replying to: @Centimo (opening post) — "# Fix 30 bugs found during code review ## Summary This MR fixes 30 bugs across core types, JSON, XML, TOML, MsgPack, BSON, Avro, Cap'n Proto, and CLI modules. …"

> HI @Centimo , thanks for this PR. I will write a more comprehensive review tomorrow, but here are my first thoughts:
>
> Issue 1: You are right, I hadn't thought of that. But I think the right approach is to also validate the other operators (maybe with the exception of operator*), not to get rid of them altogether.
>
> Issue 2: I very consciously did not make it explicit. I want people to be able to set Literal fields like this:
>
> ```
> my_struct.my_literal = "some_value"
> ```
>
> Issue 14: I am quite certain you are wrong about this one. timegm ignores the locale settings of your machine, but it also ignores the value of tm_gmtoff. It simply interprets the other fields as already being in UTC and sets tm_gmtoff and tm_isdst to zero.
>
> This is implied in here:
>
> https://ftp.gnu.org/old-gnu/Manuals/glibc-2.2.3/html_node/libc_425.html
>
> And here:
>
> https://nxmnpg.lemoda.net/3/timegm#google_vignette
>
> Furthermore, this specifies that timegm sets tm_gmtoff to zero:
>
> https://man.freebsd.org/cgi/man.cgi?gmtime(3)
>
> The most obvious apprach here would be to write a simple test.
>
> Most of the other things seem like common sense, but I have to take a closer look tomorrow.

[^288]

### 2026-02-25 · comment

> @Centimo , I agree with most of your points, but not all of them. My suggestion would be to open a separate PR with all the points on which we agree and continue the discussion on the other points.
>
> Here are my thoughts:
>
> 1. I agree, but the solution isn't to get rid of all the modifying member functions. The solution is to validate them as well.
>
> 2. Like I said - in my view, this is a feature, not a bug.
>
> 3. OK
>
> 4. I've looked at the code, I see nothing wrong with it. The body is certainly executed. Are you familiar with fold expressions?
>
> 5. OK
>
> 6. OK
>
> 7. OK
>
> 8. True, but the question is whether this is a bug. After all Box and Ref do exactly what unique_ptr and shared_ptr do when you apply the spaceship operator to them. They compare the addresses of the objects they point to, not the values of those objects. So in that sense, they are consistent with the behavior of unique_ptr and shared_ptr.
>
> 9. OK
>
> 10. OK
>
> 11. OK
>
> 12. OK
>
> 13. OK - the person who originally contributed this insisted that camelCase does not start with a capital letter by definition and he's right. But I think this is something we can catch as well, because not everyone might be aware of this convention.
>
> 14. Again, I am quite certain you (or the AI) are wrong about this.
>
> 15. OK
>
> 16. This is a discussion I have had many times. These string views originate from our field names, which are always null-terminated. If there is an easy way to add the length, go for it, but if not you are solving a problem that doesn't exist.
>
> 17. OK
>
> 18. OK
>
> 19. OK
>
> 20. OK
>
> 21. OK
>
> 22. OK
>
> 23. OK
>
> 24. OK
>
> 25. OK
>
> 26. OK
>
> 27. OK
>
> 28. Where do you see a raw pointer allocated with new[]?
>
> 29. OK
>
> 30. OK

[^289]

### 2026-03-04 · comment

Replying to: @Centimo — "@liuzicheng1987 Thanks for the detailed review! I've reverted issues 1, 2, 8, and 14 as discussed — those can be separate PRs if needed. Updated the PR …"

> @Centimo
>
> Issue 4: OK, you are right. Sorry for the confusion here.
>
> Issue 13: OK...like I said, I'm fine either way. Even if we agree with the strict definition of camel case, there is no harm in catching these kind of things.
>
> Issue 16: OK, then go ahead
>
> Issue 28. OK, you are right.

[^290]

### 2026-03-05 · comment

> @Centimo , this looks good, but some of the tests are failing. Could you take a look?

[^291]

### 2026-03-07 · comment

> @Centimo, is this ready for another review?

[^292]

### 2026-03-10 · comment

Replying to: @Centimo — "@liuzicheng1987 Sorry for the mess — I forgot to push the latest changes before posting the comment. All tests pass now after CI fix, so it's ready for review."

> @Centimo , OK, looks good to me. Thank you so much for this contribution.
>
> In my view, this is ready to be merged. Do you agree?

[^293]

- 2026-03-11 · comment · ack: “@Centimo I am merging. Thanks for the contribution!” (replying to @Centimo) [^294]

## #622 vcpkg build fail

Issue · opened by @warsark · open

### 2026-02-25 · comment

Replying to: @warsark (opening post) — "CMake Error at x:/vcpkg/scripts/buildsystems/vcpkg.cmake:908 (_find_package): Could not find a package configuration file provided by 'bson-1.0' with any of …"

> @warsark , can you provide the exact step you have undertaken, so we can reproduce this?

[^295]

### 2026-02-28 · comment

Replying to: @warsark — "reflectcpp:0.24 windows 11 vs2022 17.14.21"

> @warsark , you are right, the reflectcpp package on vcpkg fails to build whenever you add the bson feature. However, it works perfectly fine when you build it from this repository:
>
> ```bash
> ubuntu@dev:~/reflect-cpp$ cmake -S . -B build -DCMAKE_CXX_STANDARD=20 -DCMAKE_BUILD_TYPE=Release -DREFLECTCPP_BSON=ON
> -- Running vcpkg install
> Detecting compiler hash for triplet arm64-linux...
> ...
> ubuntu@dev:~/reflect-cpp$ cmake --build build -j4
> [ 16%] Building CXX object CMakeFiles/reflectcpp.dir/cmake_pch.hxx.gch
> [ 83%] Building CXX object CMakeFiles/reflectcpp.dir/src/reflectcpp_bson.cpp.o
> [ 83%] Building CXX object CMakeFiles/reflectcpp.dir/src/yyjson.c.o
> [ 83%] Building CXX object CMakeFiles/reflectcpp.dir/src/reflectcpp_json.cpp.o
> [ 83%] Building CXX object CMakeFiles/reflectcpp.dir/src/reflectcpp.cpp.o
> [100%] Linking CXX static library libreflectcpp.a
> [100%] Built target reflectcpp
> ```
>
> I'm pretty sure it's going to work on Windows as well, because we have test runners for that.
>
> In my view, whatever the problem is, it must be in vcpkg. Could you maybe file a bug report with them? I'd be happy to take a look.

[^296]

## #624 Add rfl::Deprecated, which adds deprecated and deprecationMessage to the generated schema field.

PR · opened by @AndrewNolte · merged

### 2026-02-27 · review (COMMENTED)

>  Cool idea, well executed. Thank you!

[^297]

### 2026-02-27 · comment

Replying to: @AndrewNolte (opening post) — ""

> @AndrewNolte , I just realised that this doesn't have documentation. Could you add documentation in a separate PR?
>
> Both in the README.ms and the docs folder...

[^298]

## #623 Potential improvements to rfl::cli::read() API

Issue · opened by @axel-dewalt · open

### 2026-02-28 · comment

Replying to: @axel-dewalt (opening post) — "First of all, thanks for such a useful addition to the library! I have some questions and suggestions for further improvements to its API: 1. Could …"

> Hi @axel-dewalt , I think all three changes sound great!

[^299]

## #620 Add CMake option for toggling strict warnings

PR · opened by @axel-dewalt · merged

### 2026-03-02 · comment

Replying to: @axel-dewalt — "@liuzicheng1987 when you have the time, I would be grateful for some feedback from a maintainer on this pull request, thank you."

> @axel-dewalt I am honestly unsure what to do with this one. I think that at least some of the compiler warnings originate from third party libraries, so we can't really fix that.

[^300]

### 2026-03-16 · comment

Replying to: @axel-dewalt — "@liuzicheng1987 any more thoughts, given the explanation above?"

> @axel-dewalt , sorry, I missed your last message. I will take a look tonight.

[^301]

## #626 json schema: Add enum value descriptions

PR · opened by @AndrewNolte · merged

### 2026-03-05 · comment

Replying to: @AndrewNolte (opening post) — ""

> @AndrewNolte , this looks cool. Could you also add some documentation here?

[^302]

### 2026-03-16 · comment

> @AndrewNolte , I will merge this and add the documentation myself. Thanks for the contribution.

[^303]

## #625 Docs: update with rfl::Deprecated

PR · opened by @AndrewNolte · merged

- 2026-03-05 · comment · ack: “@AndrewNolte , thanks for the contribution. I am merging.” (replying to @AndrewNolte (opening post)) [^304]

## #627 [msgpack] Fixes missing parameter comments, error message

PR · opened by @index1207 · merged

- 2026-03-07 · comment · ack: “@index1207 thanks for the contribution, I am merging!” (replying to @index1207 (opening post)) [^305]

## #629 "May be used uninitialized" warning from g++ 15.2.1 with -O2

Issue · opened by @Kirisummer · open

### 2026-03-08 · comment

Replying to: @Kirisummer (opening post) — "First of all, thanks for the library, it is very useful! I included the library sources (`reflectcpp.cpp`, `reflectcpp_json.cpp` and `yyjson.c`) to my sources …"

> ```
> >::_Vector_impl_data::_M_end_of_storage’ may be used uninitialized [-Werror=maybe-uninitialized]
>   113 |           _M_end_of_storage(__x._M_end_of_storage)
> ```
>
> @Kirisummer , this does not even appear to be from our library, this piece of code is from the C++ standard library. I also don't know how exactly Object<...>:i_ could be used uninitialized.

[^306]

### 2026-03-08 · comment

> @Kirisummer , this is very weird, I'll have to take a closer look. Thanks for reporting.

[^307]

## #632 Fix "Error()" function.

PR · opened by @Ra-LM · merged

### 2026-03-10 · comment

Replying to: @Ra-LM (opening post) — "Simple fix for a problem I ran into."

> Hi @Ra-LM , thank you for this contribution.
>
> However, I think this is already covered by https://github.com/getml/reflect-cpp/pull/618, which we will merge soon.

[^308]

### 2026-03-13 · comment

> @Ra-LM apparently it is still relevant. Thanks for the contribution, I am merging.

[^309]

## #631 Custom validators

Issue · opened by @vBazilevich · open

### 2026-03-12 · comment

Replying to: @vBazilevich (opening post) — "Hello! Could you please add a documentation entry on custom validators? Suddenly, not all validity constraints can be implemented using provided facilities. …"

> @vBazilevich sure...I never really thought that this is something people might want to do...

[^310]

## #637 C++26 reflection adoption

Issue · opened by @lixin-wei · closed (completed)

### 2026-03-24 · comment

Replying to: @lixin-wei (opening post) — "@liuzicheng1987 It might be too early to discuss now. but I'm curious whether you're willing to use C++26's reflection to repalce the core part of this …"

> @lixin-wei Absolutely, that's the plan.

[^311]

### 2026-08-25 · comment

Replying to: @Altren — "GCC 16 got reflection support, enabled with -freflection"

> @lixin-wei, by the way, we have now added support for C++-26 reflection:
>
> https://github.com/getml/reflect-cpp/pull/713

[^312]

## #634 Fix capnproto add_value_to_map overwriting all entries at index 0

PR · opened by @lixin-wei · merged

### 2026-03-24 · comment

Replying to: @lixin-wei — "@liuzicheng1987 would you pls have look at my recent 2 PRs when you have time?"

> @lixin-wei Yes. Sorry I missed them until now.

[^313]

- 2026-03-24 · comment · ack: “@lixin-wei , looks great. Thanks for the contribution.” [^314]

## #635 Fix Cap'n Proto serialization of unsigned integer types

PR · opened by @lixin-wei · merged

- 2026-03-24 · comment · ack: “@lixin-wei , same here. Looks great. Thanks!” (replying to @lixin-wei) [^315]

## #636 Boost serialisation support

Issue · opened by @ron0studios · closed (completed)

### 2026-03-24 · comment

Replying to: @ron0studios (opening post) — "It would be nice to have boost serialisation support in this library. Quick intro/tutorial to the format here: …"

> @ron0studios , I remember taking a look at this some two years ago. I am open to a PR along these lines.

[^316]

## #638 Delete MR_DESCRIPTION.md

PR · opened by @cmnrd · merged

- 2026-03-24 · comment · ack: “@cmnrd , thanks!” (replying to @cmnrd (opening post)) [^317]

## #639 How to work with reference with a static lifetime

Issue · opened by @cacharle · closed (completed)

### 2026-03-26 · comment

Replying to: @cacharle (opening post) — "I have a bunch of reference with a static lifetime in one of my project which I'd like to (de)serialize. Is there any workaround for this or would a …"

> @cacharle , I don't think reading into that could work, because you would have to somehow automatically allocate the integer. There is a way to do that with pointers, but you need to use the appropriate processor.

[^318]

## #641 Boost serialization implementation

PR · opened by @ron0studios · merged

### 2026-03-28 · comment

Replying to: @ron0studios — "undid the gemini code assist suggestion ([57031b0](https://github.com/getml/reflect-cpp/commit/57031b083d4b6a2f745424e1accc4d4868e4568e))"

> @ron0studios let me know when this is ready for review

[^319]

### 2026-03-28 · comment

> @ron0studios I just had a quick look. I think you're on the right track. I look forward to having this.

[^320]

### 2026-03-28 · comment

Replying to: @ron0studios — "@liuzicheng1987 Ready for review - everything but docs is written through. The remaining work was a matter of writing tests to see if some edge cases already …"

> @ron0studios , first of all, thanks for the contribution. This looks great and it inspired me to support https://github.com/USCiLab/cereal next.
>
> I have a couple of minor remarks:
>
> 1. As you have already observed, this still needs documentation. It needs to be mentioned in the README and it needs its own section in the general documentation. Feel free to write that using a coding agent of your choice (if you don't have one, I can do that).
>
> 2. You need to update the Github Actions files to actually run your tests. I was able to run them locally and they pass, of course, but they need to run in an automated way.
>
> 3. I think you could outsource some of the functionality (such as writer) into source files. Compile time is a major issue in a project like this and anything we can do to reduce it, we should do.
>
> 4. It would also be cool to include this in the benchmarks. I am actually very curious how this performs in comparison to other serialization formats.
>
> 5. I think we could add a few more tests and it wouldn't be hard to do...you could simply copy the tests for BSON, say, and they just search + replace bson with boost_serialization.
>
> As a final remark, I am a bit concerned about security here, as there appears to be next to no validation...I wonder to what extent it would be possible to add a malicious file to get the code to do things it's not supposed. But this seems to be a general problem with Boost.Serialization and it's certainly not our job to fix it.
>
> If you want to me to take over any of the points 1-5, I'd be happy to do so.

[^321]

### 2026-04-03 · comment

Replying to: @ron0studios — "some benchmarking. I think I could make this faster by reducing the copies on the read path."

> @ron0studios, I have since added the cereal support, so please merge main once and resolve the merge conflicts in the CMakeLists (which should be very easy).

[^322]

### 2026-04-03 · comment

> @ron0studios other than that, looks good to me. If the tests run smoothly, I see no reason not to merge this, unless you want to add your optimization first.

[^323]

### 2026-04-03 · comment

> @ron0studios, the tests look good. The two failures are obviously a server issue, nothing to do with your code. So if you could merge main, that would be great. I would then merge the PR, unless you want to do the optimization first.

[^324]

- 2026-04-03 · comment · ack: “@ron0studios merged. Thanks for your contribution!” (replying to @ron0studios) [^325]

## #640 Problem (de-)serializing an enum with value 0 if the | operator is overloaded

Issue · opened by @florian-n · closed (completed)

### 2026-03-30 · comment

Replying to: @florian-n (opening post) — "Hello, I am unfortunately running into an issue with the newest version of the reflect-cpp lib. Assume I have something like this: ``` enum class TestEnum : …"

> @florian-n thanks for reporting, I'll take a look

[^326]

### 2026-03-30 · comment

> @florian-n the problem is that you cannot assign 0 to a flag enum. As stated in the documentation, values for flag enums must be 1 or 2^N, but zero is not among the supported values.
>
> The moment you assign operator|, your enum is recognised as a flag enum.

[^327]

### 2026-03-31 · comment

Replying to: @florian-n — "Thank you for the quick response. I understand that part. However, I'd argue that 0 is a legitimate value as in 'I chose none of those flags'. Hence, I can see …"

> @florian-n, OK, this seems to be a bug in the enchantum library. I will take a look.

[^328]

### 2026-04-03 · comment

Replying to: @florian-n — "Thanks, I appreciate it! I'd suspect its this bit in `enums.hpp:70`, that essentially skips over the `0` value: ``` while (val != 0) { const auto bit = val & …"

> @florian-n , this was indeed an unhandled edge case; this PR should fix it:
>
> https://github.com/getml/reflect-cpp/pull/644
>
> Thanks for reporting!

[^329]

## #642 Attempting to serialize an rfl::Commented<std::optional<std::array<>>> fails when the value is null

Issue · opened by @Lazrius · closed (completed)

### 2026-03-30 · comment

Replying to: @Lazrius (opening post) — "Good evening. I've stumbled upon a rather specific problem. When a std::array is marked as optional within an rfl::Commented descriptor, and the value is null, …"

> @Lazrius thanks for reporting, I'll take a look

[^330]

### 2026-04-03 · comment

> @Lazrius, this PR should fix it. Thanks for reporting.

[^331]

## #644 Make sure that flag enums with value zero are correctly serialized; fixes #640

PR · opened by @liuzicheng1987 · merged

### 2026-04-03 · inline include/rfl/enums.hpp:80-82

Replying to: @gemini-code-assist[bot] (inline) — "Instead of hardcoding the string '0', it is better to use the to_string_or_number helper. This ensures that if the enum has a named constant for the zero value …"

> Nope

[^332]

## #645 Make sure that optional inside commented works as expected; fixes #642

PR · opened by @liuzicheng1987 · merged

### 2026-04-03 · inline include/rfl/Commented.hpp:138-140

Replying to: @gemini-code-assist[bot] (inline) — "For completeness and to support potential mutation via reflection (e.g., when deserializing into an existing object), it is recommended to provide both `const` …"

> Not important

[^333]

## #646 boost src/ offloading

PR · opened by @ron0studios · merged

### 2026-04-03 · comment

Replying to: @ron0studios (opening post) — "@liuzicheng1987 sorry I think this change got lost during a rebase, but slipped under the radar since all tests still pass without it."

> @ron0studios, yes, somehow the rebase broke that. But there still seems to be something wrong with it, some tests fail. COuld you take a look?

[^334]

## #586 Add serialization for std::chrono::system_clock::time_point

Issue · opened by @cacharle · closed (completed)

### 2026-04-04 · comment

Replying to: @ron0studios — "Hey, if this issue is still open I'd like to have a go at it. A non-intrusive solution would be perhaps implementing a Parser specialisation for time_point? …"

> Sure, have a go!

[^335]

## #648 Parser for chrono::time_point

PR · opened by @ron0studios · merged

### 2026-04-05 · comment

Replying to: @ron0studios — "Timezone conversion and docs done, implementing a parser for std::chrono::zoned_time next would let users preserve timezone data when reading/writing. I can …"

> @ron0studios looks great. No notes. Should we merge?

[^336]

## #651 Option to disable toml indentation

Issue · opened by @PeakKS · open

### 2026-04-14 · comment

Replying to: @PeakKS (opening post) — "Not a fan of the indentation added to toml writes, looks like there are [some flags](https://github.com/marzer/tomlplusplus/issues/120#issuecomment-952922998) …"

> @PeakKS yes, I think that should be possible. We do something similar for JSON.

[^337]

## #653 [bug] rfl::Processors seems broken

Issue · opened by @roman-orekhov · closed (completed)

### 2026-04-16 · comment

Replying to: @roman-orekhov (opening post) — "Was trying to follow advice from [here](https://rfl.getml.com/concepts/processors/#combining-several-processors) ```C++ #include <iostream> #include <rfl.hpp> …"

> @roman-orekhov that is really weird. The C++ code uses processors under-the-hood. I'll take a look. Thanks for reporting.

[^338]

### 2026-04-16 · comment

> @roman-orekhov , I know what the issue is. Fix is on the way. Again, thanks for reporting.

[^339]

## #652 Why not allowing to parse null as std::optional in yaml files via `is_empty(null) -> true`

Issue · opened by @Fellfalla · open

### 2026-04-16 · comment

Replying to: @Fellfalla (opening post) — "Hey, i was interested in explicitly point out 'null' for `std::nullopt` values inside yaml files. Would there a specific reason against applying this patch …"

> @Fellfalla I don't see anything wrong with it. PR is welcome.

[^340]

## #655 Redesign the way processors are identified; fixes #653

PR · opened by @liuzicheng1987 · merged

- 2026-04-26 · inline include/rfl/internal/no_optionals_v.hpp:7-8 · ack: “Resolved” (replying to @gemini-code-assist[bot] (inline)) [^341]

## #654 Feat/parse null into nullopt optionals

PR · opened by @Fellfalla · merged

### 2026-04-26 · inline tests/yaml/test_read_null.cpp:32

Replying to: @gemini-code-assist[bot] (inline) — "The expression `res && true` contains a redundant `&& true`. For better clarity and consistency with other tests in this file (such as lines 133 and 139), it …"

> @Fellfalla , feel free to ignore this one

[^342]

### 2026-04-26 · comment

Replying to: @Fellfalla (opening post) — "Implements the possibility to explicitly write `value: null` for optionals with `std::nullopt` values inside yaml files. Related discussion: …"

> @Fellfalla , the PR looks good to me. Happy to merge this once the tests run through. Thanks for the contribution!

[^343]

### 2026-04-27 · comment

Replying to: @Fellfalla — "Tests seem to fail due to flaky pipeline. Is it related to my changes?"

> @Fellfalla , no, that is because unfortunately vcpkg in Github is indeed a bit flaky. Nothing to do with your changes.

[^344]

### 2026-04-27 · comment

> @Fellfalla , I have merged the PR. Thank you very much for your contribution!

[^345]

## #657 User controllable multiline string serialization for YAML

PR · opened by @microdee · merged

### 2026-05-03 · comment

Replying to: @microdee — "The check failures seem to be an issue with VCPKG or how CMake cannot find ninja. Can this be caused by my edits in this PR? The only project structural change …"

> @microdee, sorry it took me a couple of days to get to this. Unfortunately, the Github Actions pipeline is a bit unstable lately, it has nothing to do with your PR.
>
> Your PR looks great, I will merge it. Thank you for your contribution.

[^346]

## #658 clang-tidy & MSVC: Duplicate field names are not allowed

Issue · opened by @PapeCoding · open

### 2026-05-06 · comment

Replying to: @PapeCoding (opening post) — "Dear reflect-cpp team, I currently encounter some errors using clang-tidy while building using MSVC (cl.exe). Without clang-tidy or with other compilers this …"

> @PapeCoding , interesting, thanks for reporting.

[^347]

## #659 Fix MkDocs deployment on public mkdocs-material

PR · opened by @awaismirza92 · merged

### 2026-05-10 · review (APPROVED)

> Hi @awaismirza92 , looks like your PR fixes the issue. The Conan thing clearly has nothing to do with your PR, so it need to be fixed separately.

[^348]

## #662 Add unified rfl::Settings::with<>() API for format-specific settings

PR · opened by @Centimo · merged

### 2026-05-10 · comment

Replying to: @Centimo (opening post) — "## Summary Replaces ad-hoc `with_<field>()` accessors on `csv::Settings` and `parquet::Settings` with a single uniform `with<>()` API injected by the …"

> Hi @Centimo, I think this is certainly a step in the right direction, but I am always hesitant about API-breaking changes like this, because they always tend to frustrate people.
>
> As a compromise, could you reinsert the old functions, add a comment in the code that they are deprecated and will be removed at a later date? Since the markdown documentation only includes the new style, most people will start using that. When we add new functions it will only be in the new style.
>
> What do you think about that?

[^349]

### 2026-05-10 · comment

Replying to: @Centimo — "Hi @liuzicheng1987 Do you mean the old with_* functions?"

> @Centimo, yes.
>
> I think that people use them in the productive system and when you have to change that unexpectedly, that makes people angry.
>
> So we should keep them for the time being but add a deprecation notice, because I do agree that your approach is better.

[^350]

### 2026-05-10 · comment

Replying to: @Centimo — "@liuzicheng1987 done"

> @Centimo, all right, thank you very much for the contribution. I am merging.

[^351]

## #661 Deserialising std::map Enum Keys

Issue · opened by @stellarpower · open

### 2026-05-10 · comment

Replying to: @stellarpower (opening post) — "std::map from an enum key to a value T is a setup I often use, it's important as it ensures our keys are well-defined and that we handle them (all, …"

> Hi @stellarpower,
>
> thanks for your constructive criticism.
>
> I have opened a PR that adds DOxygen-style docstrings to most of the codebase. Does that help you?
>
> https://github.com/getml/reflect-cpp/pull/663

[^352]

## #664 Adapt Conan to support the new formats

PR · opened by @liuzicheng1987 · merged

### 2026-05-13 · inline conanfile.py:85

Replying to: @gemini-code-assist[bot] (inline) — "The version `boost/1.90.0` does not exist in Conan Center. The latest stable version of Boost is currently `1.87.0`. Using a non-existent version will cause …"

> Sure it is

[^353]

## #667 Version 0.25.0

PR · opened by @liuzicheng1987 · merged

### 2026-05-14 · inline vcpkg.json:3

Replying to: @gemini-code-assist[bot] (inline) — "The version has been bumped to 0.25.0, and support for Boost.Serialization and YAS has been added. However, these new dependencies (or optional features) are …"

> Sure they are in vcpkg.json

[^354]

## #665 rfl::to_generic() generates MSVC warning C4702 (unreachable code)

Issue · opened by @axel-dewalt · closed (completed)

### 2026-05-14 · comment

Replying to: @axel-dewalt (opening post) — "I refer you to this fragment from `include/rfl/generic/Writer.hpp`: ```c++ private: template <class T> OutputVarType to_generic(const T& _var) const noexcept { …"

> @axel-dewalt , yes, it appears the final     return OutputVarType{}; should be inside the bracket or removed altogether.
>
> Thanks for reporting.

[^355]

## #668 fix(json): don't crash on non-ASCII std::wstring / std::filesystem::path

PR · opened by @mrgyatso · merged

### 2026-05-14 · comment

Replying to: @mrgyatso (opening post) — "## Summary A struct field of type `std::wstring` or `std::filesystem::path` containing any non-ASCII character crashes `rfl::json::read` / `rfl::json::write` …"

> @mrgyatso , thanks for the contribution. This looks great. I was about to do the 0.25.0 release, but I want to squeeze your commit in, so I will delay it for another day.

[^356]

### 2026-05-14 · comment

> @mrgyatso it appears your solution does not work for Windows/MSVC. Could you take a look?

[^357]

### 2026-05-15 · comment

Replying to: @mrgyatso — "Looks like this one failed due to a 502 error upstream with vcpkg, unrelated to the fix. Should work fine on re-run."

> @mrgyatso , yes, unfortunately the build pipeline is a bit unstable. We have too many things running at the same time, so the vcpkg servers tend to block us. It's a known issue and clearly has nothing to do with your PR.

[^358]

### 2026-05-15 · comment

> @mrgyatso , I have merged this. Thankl you for your contribution!

[^359]

## #669 Add C++20 modules support and format btw.

PR · opened by @Perdixky · open (draft)

### 2026-05-19 · comment

Replying to: @Perdixky (opening post) — "As the library depends on C++20, I think the support for modules is definitely important 😊 So I’ve added this. If this change is approved, I’ll add the …"

> Hi @Perdixky , thanks for your contribution. To be honest, we have tried adding module support a while ago and back then there were so many problems that we decided modules are not yet ready for prime time. But I'd be happy to give them another try.

[^360]

### 2026-05-19 · inline include/rfl/thirdparty/yyjson.h:332-334

Replying to: @Perdixky (inline) — "Actually, in the CMake configuration, this file is considered a C++ file: So we don’t need to worry about C compatibility."

> @Perdixky , actually this is an issue. People might decide not to use the shipped version, but the version installed via vcpkg or Conan. As a general rule, we shouldn't make changes to third-party libraries.

[^361]

### 2026-05-19 · comment

> @Perdixky , could you also add the module tests that you have written to the automated Github Actions testing pipeline? It does not appear that your tests are actually compiled or executed.

[^362]

### 2026-06-09 · comment

> @Perdixky, are you planning to continue working on this? Otherwise I will have to close this due to inactivity?

[^363]

- 2026-06-10 · comment · ack: “@Perdixky no problem!” (replying to @Perdixky) [^364]

### 2026-07-16 · comment

Replying to: @Perdixky — "Hi @liuzicheng1987! There are a few issues I need to report: - Some binary module interfaces (BMIs) are affected by macros like `REFLECTCPP_ENUM_RANGE_MIN`. …"

> @Perdixky you should merge main into your feature branch. Some of these issues might already be resolved.

[^365]

### 2026-07-19 · comment

Replying to: @Perdixky — "@liuzicheng1987 These commits have already been shipped. Please feel free to review them and let me know if you have any feedback!"

> @Perdixky there seem to be significant issues. Could you take another look?

[^366]

## #671 Fix struct Person definition in json_schema.md

PR · opened by @evanwporter · merged

- 2026-05-26 · comment · ack: “@AndrewNolte thanks!” (replying to @evanwporter (opening post)) [^367]

## #672 Generic::to_*() enhancements

PR · opened by @kone-tlammi · merged

- 2026-05-31 · comment · ack: “@kone-tlammi , thanks for the contribution! I am merging.” (replying to @kone-tlammi (opening post)) [^368]

## #673 Exception thrown in noexcept code

Issue · opened by @kone-tlammi · closed (completed)

### 2026-06-03 · comment

Replying to: @kone-tlammi (opening post) — "The below code is somewhat malformed, as `DefaultIfMissing`'s default value does not match what the validator expects. In these cases, IMO, it is fine to throw …"

> @kone-tlammi thanks for reporting

[^369]

### 2026-06-03 · comment

> @kone-tlammi since you have a fix...do you want to open a PR?

[^370]

### 2026-06-23 · comment

> Fixed by https://github.com/getml/reflect-cpp/pull/686

[^371]

## #674 fix abort when default value does not satisfy Validator

PR · opened by @kone-tlammi · closed without merge

### 2026-06-04 · inline include/rfl/parsing/Parser_default.hpp:54

Replying to: @kone-tlammi (inline) — "The exception is only thrown if the default value of the member does not match the validator, which is, IMO, the right approach. As a side-effect, removing the …"

> I actually agree with Gemini on this...the exception should be caught and then the result should passed through as an error. This is what happens for just about any other mistake that is made and it should be consistend.
>
> If you want this to through, you are always free to call rfl::json::read(...).value();

[^372]

### 2026-06-04 · inline include/rfl/parsing/Parser_default.hpp:54

Replying to: @kone-tlammi (inline) — "What about things like `std::bad_alloc`? Seems like `error(e.what())` may throw that. Though that change might be a separate pull request. Seems like that …"

> @kone-tlammi, that's true, but these are very exceptional circumstances. For validation errors, we should return a normal error. So maybe you could remove the noexcept, but still implement the try...catch pattern as suggested by Gemini.

[^373]

### 2026-06-06 · comment

Replying to: @kone-tlammi — "Fixes #673. I revisited the changes and found a minimal change to fix the issue."

> @kone-tlammi, I took another look at this and I think that the solution suggested by Gemini is spot on. Could you just accept that suggested solution?

[^374]

### 2026-06-14 · inline include/rfl/parsing/Parser_default.hpp:54

Replying to: @kone-tlammi (inline) — "That does have the small issue with `std::bad_alloc` that `error()` creates a copy of the string and is therefore itself potentially throwing. I guess I can do …"

> @kone-tlammi you can remove the noexcept, I have no problems with that. But I think this should return an error, not an exception

[^375]

### 2026-06-23 · comment

> https://github.com/getml/reflect-cpp/pull/686 fixes the issue, so I am closing this one.

[^376]

## #677 `rfl::json::to_schema<T, rfl::AddTagsToVariants>` fails to compile for variants of plain structs

Issue · opened by @gllmflndn · closed (completed)

### 2026-06-07 · comment

Replying to: @gllmflndn (opening post) — "When `rfl::AddTagsToVariants` is passed as a processor to `rfl::json::to_schema`, compilation fails with a recursive template-instantiation error for any type …"

> @gllmflndn thanks for reporting. I will take a look.

[^377]

## #679 rfl::DefaultVal's are still required in the json schema

Issue · opened by @autoantwort · closed (completed)

### 2026-06-08 · comment

Replying to: @autoantwort (opening post) — "For the following struct: ```c++ struct Settings { rfl::DefaultVal<std::uint16_t> port = 80; }; ``` the following schema is generated ```json { '$schema': …"

> @autoantwort thanks for reporting

[^378]

## #680 WIP: respects DefaultVal in schema and adds default value to schema. fixes #679, #396

PR · opened by @autoantwort · merged

### 2026-06-08 · comment

Replying to: @autoantwort (opening post) — "fixes #679, #396"

> @autoantwort, I don't think it's a good idea to be trying to fix two problems at once and fixing #396 is tricky. There is a reason no one really took it up in years.
>
> Also I am very concerned about the proposed the solution of passing a void ptr.
>
> I didn't know you were going to propose a fix, so I started working on my own:
>
> https://github.com/getml/reflect-cpp/pull/681
>
> This fixes #679, but still needs some adaptions for some non-JSON formats.

[^379]

### 2026-06-08 · comment

> @autoantwort, if you want to fix #396, I think the best way would be to transform the struct with the default values into an rfl::Generic and then pass that to the to_schema function. That might actually work.

[^380]

### 2026-06-09 · comment

Replying to: @autoantwort — "Yeah that worked. Ok so you want to continue yours and I than rebase this on yours or you use the idea from here in #681 or in a new PR?"

> @autoantwort, I think I can continue on mine to fix #679 and then we can dedicate this PR to fixing #396

[^381]

### 2026-06-09 · comment

> @autoantwort I have just merged my PR which fixes #679, you can merge main into this branch and then continue working on #396.

[^382]

### 2026-06-10 · comment

Replying to: @autoantwort — "Done"

> @autoantwort, that's great work. We'll see how the tests do, but I am quite happy.

[^383]

### 2026-06-12 · comment

> @autoantwort, there appear to be minor issues with missing includes (we always want to make sure that every header is self-contained). Could you fix that?

[^384]

### 2026-06-15 · comment

> @autoantwort , merged with a minor edit. Thanks for the contribution!

[^385]

## #684 Request: Implement 'member-of-pointer' access operator (->) for Rfl types

Issue · opened by @Lazrius · closed (completed)

### 2026-06-21 · comment

Replying to: @Lazrius (opening post) — "Heya. I was wondering if it would be possible to implement the 'ptr' operator for many of the types to make accessing underlying data structures a bit easier …"

> OK, I personally prefer (), but if you like ->, a PR is welcome!

[^386]

### 2026-06-23 · comment

> @Lazrius a PR is welcome!

[^387]

### 2026-07-22 · comment

Replying to: @Lazrius — "I will try take a look at it at the weekend 🙂"

> @Lazrius here is a PR:
>
> https://github.com/getml/reflect-cpp/pull/704

[^388]

- 2026-07-26 · comment · ack: “@Lazrius no problem” (replying to @Lazrius) [^389]

## #685 Cannot use rfl::DefaultVal and rfl::Field together in one strcut

Issue · opened by @Liclute · closed (completed)

### 2026-06-21 · comment

Replying to: @Liclute (opening post) — "define struct like: struct A { rfl::Field<'a', int> a; rfl::Field<'b', rfl::DefaultVal<int>> b = 1; }; will get error: Parser_default.hpp:695:10: error: could …"

> @Liclute , thanks for reporting, I will take a look

[^390]

### 2026-06-22 · comment

> @Liclute , sure that cannot work...the idea of fields is that you have to explicitly set all of them, so will not work in conjunction with default values. I don't think there's a lot we can do about this, unfortunately. Therefore, I am going to close this issue, but if you think you could come up with a solution, feel free to reopen.

[^391]

## #683 Automatic min and max in json schema for integers

Issue · opened by @autoantwort · open

### 2026-06-23 · comment

Replying to: @autoantwort — "There is a `rfl::capnproto::schema::UInt8` but it is not used in the Schema or did I miss something?"

> @autoantwort , the problem I have with that is that it adds overhead. But I think we could offer a validator that automatically does that for you, something like:
>
> ```cpp
> struct MyStruct {
>     rfl::CheckMinMax<int> my_field;
> };
> ```
>
> (But maybe with a better name.)

[^392]

### 2026-07-23 · comment

Replying to: @autoantwort — "Do you mean overhead in the schema, compilation overhead or parsing overhead?"

> @autoantwort , sorry, I misunderstood the issue. I think this is very possible.

[^393]

## #687 no internal linkage in headers

PR · opened by @autoantwort · merged

- 2026-06-24 · comment · ack: “@autoantwort , thanks for the contribution!” (replying to @autoantwort (opening post)) [^394]

## #689 Enum-Flag Default Values

Issue · opened by @PapeCoding · closed (completed)

### 2026-07-03 · comment

Replying to: @PapeCoding (opening post) — "Currently I try to use reflect-cpp to serialize a struct to json and back. ```cpp enum class PlatformFlag { NONE = 0, LINUX = 1 << 0, WINDOWS = 1 << 1, MAC = 1 …"

> @PapeCoding what version are you using? I am pretty sure we have seen this before and fixed it already.

[^395]

### 2026-07-04 · comment

> @PapeCoding I cannot reproduce the issue on the newest version of reflect-cpp. Here is the test I wrote:
>
> ```cpp
> enum class PlatformFlag {
>   NONE = 0,
>   LINUX = 1 << 0,
>   WINDOWS = 1 << 1,
>   MAC = 1 << 2,
>   ALL = LINUX | WINDOWS | MAC
> };
>
> constexpr PlatformFlag operator&(PlatformFlag const a, PlatformFlag const b) {
>   return static_cast<PlatformFlag>(static_cast<uint64_t>(a) &
>                                    static_cast<uint64_t>(b));
> }
>
> constexpr PlatformFlag operator|(PlatformFlag const a, PlatformFlag const b) {
>   return static_cast<PlatformFlag>(static_cast<uint64_t>(a) |
>                                    static_cast<uint64_t>(b));
> }
>
> inline PlatformFlag& operator|=(PlatformFlag& a, PlatformFlag const b) {
>   a = a | b;
>   return a;
> }
>
> struct myStruct {
>   PlatformFlag supportedPlatforms = PlatformFlag::NONE;
> };
>
> TEST(json, test_enum9) {
>   myStruct t{.supportedPlatforms = PlatformFlag::NONE};
>
>   write_and_read(t, R"({"supportedPlatforms":"0"})");
> }
> ```
>
> The test passes just fine. The same issue was already addressed with this commit: https://github.com/getml/reflect-cpp/commit/498d9d04e560a961da900cc1742b40a85afe1a9e
>
> So all you need to do is to update to reflect-cpp v0.25.0 and the problem is fixed.

[^396]

### 2026-07-04 · comment

> @PapeCoding if you have upgraded to the newest version of reflect-cpp and it still doesn't work, feel free to reopen the issue and then please tell me exactly what compiler and OS you are using, so I can investigate further.

[^397]

### 2026-07-07 · comment

Replying to: @PapeCoding — "@liuzicheng1987 Thanks for your fast help! I did not notice that I was still using v0.24.0 and with the new one the problem is gone. I still think it is weird, …"

> @PapeCoding this is due to the current limitations that C++ imposes on us, and it is documented:
>
> https://rfl.getml.com/enums/#flag-enums
>
> The problem is that 0 is neither 2^N nor can it be expressed as bitwise ORs of numbers that are.
>
> However, once C++ 26 reflection is fully supported by the compilers, we will have no such limitatation.

[^398]

### 2026-08-25 · comment

Replying to: @ZXShady — "@liuzicheng1987 This is not a limitation- enchantum does reflect the zero value by default all you need to do is replace this line …"

> @ZXShady the issue is already fixed using pretty much exactly the solution you proposed.
>
> By the way, I have recently added support for C++-26.

[^399]

## #688 Avoid clang 21/22 C++26-mode frontend crash in StringLiteral's variadic constructor

PR · opened by @aruisdante · merged

### 2026-07-04 · comment

Replying to: @aruisdante (opening post) — "## Problem Any translation unit that includes reflect-cpp headers fails to compile at `-std=c++26` on clang 21.1+, 22.1.x, and current trunk: the compiler …"

> @aruisdante your solution fails for macos-latest. Could you take another look? Maybe we need to implement a version-specific solution….

[^400]

### 2026-07-19 · comment

Replying to: @aruisdante — "## The `macos-latest (tests)` failure is unrelated to this PR Every other matrix cell is green — all Linux (gcc 11–14, llvm 16–18, C++20/23), all Windows/MSVC …"

> @aruisdante I will merge your other PR, then push main into this one and merge it as well.

[^401]

### 2026-07-20 · comment

> @aruisdante now all of the tests pass. I am merging. Thank you so much for your contribution and sorry that it took a bit longer than usual to merge your PR.

[^402]

## #691 fix(tests): read cbor byte view via std::span, not basic_string_view<std::byte>

PR · opened by @aruisdante · merged

### 2026-07-17 · comment

Replying to: @aruisdante (opening post) — "## Problem `macos-latest (tests)` is red on current CI (and will fail on `main` the next time that job runs — `main` is green only because its last macOS run …"

> @aruisdante sorry, it took me quite a while to notice your PR

[^403]

### 2026-07-19 · comment

> @aruisdante I have fixed the issues in the build pipelines and pushed the main branch into your PR. This has triggered the test runs. Once they have all passed, I will merge your PR and then repeat the same procedure for your other PR.

[^404]

## #692 Feature Request: CLI usage/help generation

Issue · opened by @solly-skyral · open

### 2026-07-18 · comment

Replying to: @solly-skyral (opening post) — "# Automated Usage Generation ## Context Hey, firstly just wanted to say thanks for the addition of the CLI parser. One feature that I've ended up hand-rolling …"

> @solly-skyral sorry for the late response. A PR is very welcome!

[^405]

## #693 Equality operator for rfl::Generic and rfl::Object

Issue · opened by @jinchengJL · closed (completed)

### 2026-07-18 · comment

Replying to: @jinchengJL (opening post) — "Hello, we sometimes embed Generic inside of types like: ```cpp struct Foo { std::string x; rfl::Generic extras; }; ``` But then, in order to default …"

> @jinchengJL A PR is very welcome!

[^406]

## #694 Added support for environment variables

PR · opened by @liuzicheng1987 · merged

- 2026-07-19 · inline include/rfl/env/Reader.hpp:169 · ack: “Done” (replying to @gemini-code-assist[bot] (inline)) [^407]
- 2026-07-19 · inline include/rfl/env/Reader.hpp:232 · ack: “Done” (replying to @gemini-code-assist[bot] (inline)) [^408]
- 2026-07-19 · inline src/rfl/env/Writer.cpp:35-37 · ack: “Done” (replying to @gemini-code-assist[bot] (inline)) [^409]
- 2026-07-19 · inline src/rfl/env/Writer.cpp:132 · ack: “Done” (replying to @gemini-code-assist[bot] (inline)) [^410]
- 2026-07-19 · inline tests/env/write_and_read.hpp:15-17 · ack: “Done” (replying to @gemini-code-assist[bot] (inline)) [^411]
- 2026-07-19 · inline tests/env/write_and_read.hpp:62-70 · ack: “Done” (replying to @gemini-code-assist[bot] (inline)) [^412]
- 2026-07-19 · inline include/rfl/env/Reader.hpp:172-176 · ack: “Done” (replying to @gemini-code-assist[bot] (inline)) [^413]
- 2026-07-19 · inline src/rfl/env/Writer.cpp:108 · ack: “Done” (replying to @gemini-code-assist[bot] (inline)) [^414]

## #696 Add operator== for rfl::Generic and rfl::Object

PR · opened by @jinchengJL · merged

- 2026-07-20 · comment · ack: “@jinchengJL Looks great. Thanks for your contribution. I am merging.” (replying to @jinchengJL (opening post)) [^415]

## #690 FR: Deserialization to std::set with custom comparator

Issue · opened by @PapeCoding · closed (completed)

### 2026-07-21 · comment

Replying to: @PapeCoding (opening post) — "# Problem During testing your library I noticed that I can easily serialize and de-serialize data structures containing `std::set`s. Unfortunately as soon as I …"

> @PapeCoding we have had similar problems for maps...the solution is to simply write overloads for the parsers. It's actually an easy fix.

[^416]

### 2026-07-21 · comment

> @PapeCoding the issue is now resolved!

[^417]

## #700 Reading JSON into a struct with a `DefaultVal` field and a size validator field always raises a size validation error

Issue · opened by @wbernoudy · open

### 2026-07-22 · comment

Replying to: @wbernoudy (opening post) — "For example, using `rfl::json::read` with the struct ```c++ struct Person { rfl::DefaultVal<std::string> name = 'Homer Simpson'; …"

> @wbernoudy, yes, I am aware of this issue and at the moment, i can't really think of a good way to fix it.
>
> Here is what is happening: For normal parsing, we use this little trick:
>
> ```cpp
>     alignas(T) unsigned char buf[sizeof(T)]{};
>     auto ptr = internal::ptr_cast<T*>(&buf);
>     auto view = ProcessorsType::template process<T>(to_view(*ptr));
>     using ViewType = std::remove_cvref_t<decltype(view)>;
>     const auto [set, err] =
>         Parser<R, W, ViewType, ProcessorsType>::read_view(_r, _var, &view);
> ```
>
> This allows us to read into the struct without actually initializing it. But when the struct has default values, we need to be able to use them, so we have to do this instead:
>
> ```cpp
>       auto t = T{};  // This might fail, for instance if the default value does
>                      // not satisfy the validator, but in that case we will just
>                      // return the error.
>       auto view = ProcessorsType::template process<T>(to_view(t));
>       using ViewType = decltype(view);
>       const auto err =
>           Parser<R, W, ViewType, ProcessorsType>::read_view_with_default(
>               _r, _var, &view);
> ```
>
> And as the comment indicates, we ran into this issue before. Honestly, I can't really think of a good way to get around this.
>
> But if you have an idea, I am always open to suggestions.

[^418]

### 2026-10-25 · comment

Replying to: @cmami — "Note that the issue is even more serious than a mere 'raises an exception' as Result.hpp throws std::runtime_exceptions in several functions while the call …"

> @cmami the noexcept problem should be resolved by now.

[^419]

## #699 Compilation error on Clang 15.0.0

Issue · opened by @wbernoudy · closed (completed)

### 2026-07-22 · comment

Replying to: @wbernoudy (opening post) — "I am getting this compilation error on Clang 15.0.0 (using Xcode 15.4.0 on MacOS): ``` ../subprojects/reflect-cpp/src/rfl/env/Writer.cpp:139:39: error: …"

> This is weird, because we are explicitly testing this on Clang 15 in out Github Actions pipelines and we don't get the error.
>
> However, the code line you are referring to is very new and was added to the main branch yesterday. So this is probably something we should take another look at. Thanks for reporting!

[^420]

### 2026-07-22 · comment

> @wbernoudy , here is a potential fix:
>
> https://github.com/getml/reflect-cpp/pull/705
>
> Here is the branch:
>
> https://github.com/getml/reflect-cpp/tree/f/env_clang_fix
>
> Can you confirm it fixes the issue for you?

[^421]

## #707 Allow validated fields to work alongside defaulted ones

PR · opened by @wbernoudy · closed without merge

### 2026-07-26 · comment

Replying to: @wbernoudy (opening post) — "Fixes #700 Like outlined in my comment in that issue, this works by giving the constructor of `rfl::Validator` an 'escape-hatch' using `std::source_location`. …"

> @wbernoudy there are multiple build failures in the test pipeline, could you take a look?
>
> Also, what if people use the DefaultIfMissing processor? What if they wrap their validator in DefaultValue? In that case, the validator would no longer guarantee that the condition it sets always holds.
>
> Also, it introduces brittleness to the code - by moving around the code a bit, you could inadvertently invalidate all of the validators.
>
> To be honest, I am also a bit uncomfortable with the solution here...I understand that we use source_location in other places, but we do so in a very controlled manner and it is absolutely essential to the functionality of the library. In this case, it really solves a very minor problem (all you have to do is assign a default value to the field that does fulfill the validator's condition and then there's no problem), so I am unsure whether this is the right way forward and might create more problems in the future than it solves.

[^422]

### 2026-08-06 · comment

Replying to: @wbernoudy — "You're right, with the current solution, if you provide a some data to be deserialized that does with that field missing, and the default value for the field …"

> @wbernoudy I am closing this. But thanks for the initiative anyway. If you have another idea for this problem or another one, an issue and/or PR is always welcome.

[^423]

## #708 MessagePack writer encodes C++ float values as float64

Issue · opened by @Miguel-OYeah · closed (completed)

### 2026-07-27 · comment

Replying to: @Miguel-OYeah (opening post) — "### Summary The MessagePack writer currently serializes every C++ floating-point type with `msgpack_pack_double`: ```cpp } else if constexpr …"

> @Miguel-OYeah you are right, there really is no good reason for this. As you correctly observed, the deserialization is already fine, we just need two more special cases for serialization.
>
> A PR is welcome. Once we're at it, we could take a look at the other binary formats as well.

[^424]

## #706 [Feature Request] ref::cli::read wchar_t **argv[] support

Issue · opened by @levicki · closed (completed)

### 2026-07-27 · comment

Replying to: @levicki (opening post) — "Please add `wchar_t` support for argument parsing."

> @levicki this is a bit trickier than I originally thought. I will think about an elegant solution

[^425]

### 2026-07-28 · comment

Replying to: @levicki — "Yes I know, especially if you want to keep multi-platform support. Do you think something like this would work? ``` #include <codecvt> #include <concepts> …"

> @levicki we will have to avoid wstring_convert. It is deprecated and will be removed in C++ 26.
> https://en.cppreference.com/cpp/locale/wstring_convert

[^426]

### 2026-07-28 · comment

> @levicki I have an idea. The PR will come later today.

[^427]

### 2026-07-28 · comment

Replying to: @levicki — "Can you please clarify if you plan to change storage and return type for string arguments also (so that we can parse them into std::wstring for example) or …"

> @levicki I will convert on input, but it is absolutely possible to parse into std::wstring, as you can see from the tests I have added in the relevant PR:
>
> https://github.com/getml/reflect-cpp/pull/709

[^428]

## #710 Preserve float width in MessagePack output

PR · opened by @Miguel-OYeah · merged

- 2026-07-30 · comment · ack: “@Miguel-OYeah looks great. Thanks for your contribution!” (replying to @Miguel-OYeah (opening post)) [^429]

## #712 rfl::to_view() does not seem to actually support being run at compile-time

Issue · opened by @saxbophone · open

### 2026-08-17 · comment

Replying to: @saxbophone — "I did a bit of further digging and hacking on reflect-cpp. The underlying issue seems to be that a lot of the library's own types (NamedTuple, Tuple, etc...) …"

> @saxbophone what we mean by "at compile time" is that the fields are resolved at compile time, not that the function itself is applied at compile time.
>
> The difficulty is that the tuple implementation uses placement new, which makes it difficult to make this fully constexpr.
>
> But I will take a look at what can be done for this particular use case.

[^430]

### 2026-08-19 · comment

Replying to: @saxbophone — "I think I understand. Perhaps the docs could be clarified as the current wording is a bit confusing as to this nuance. Yeah, I looked into replacing it with …"

> @saxbophone C++ 26 offers a much simpler way to implement a non-recursive tuple. There isn't a lot of compiler support for it yet, but it might be the future.

[^431]

## #714 Enum can be deserialized from raw integers that don't map to enum values

Issue · opened by @cacharle · closed (completed)

### 2026-08-22 · comment

Replying to: @cacharle (opening post) — "**Any** integer value is accepted to deserialize enums which is imo a risky default behavior since it means enum values absolutely need to be checked after …"

> @cacharle , yes, this is on purpose and documented behaviour:
>
> https://rfl.getml.com/enums/#what-happens-if-an-enum-cannot-be-matched
>
> I don't agree that  the default behaviour should be not to accept out-of-range enums, but I think we could add a processor that disallows out-of-range enums.

[^432]

### 2026-08-25 · comment

> @cacharle https://github.com/getml/reflect-cpp/pull/718

[^433]

## #715 YAML doesn't round-trip generics

Issue · opened by @zewt · open

### 2026-08-23 · comment

Replying to: @zewt (opening post) — "``` string s = rfl::yaml::write('On's); auto g = rfl::yaml::read<rfl::Generic>(s); printf('%s\n', rfl::json::write(g).c_str()); ``` This prints true instead of …"

> @zewt to be honest, I think this might even be a YAML issue. The YAML spec specifies that on/off are supposed to be interpreted as booleans.
>
> https://yaml.org/type/bool.html

[^434]

### 2026-08-23 · comment

> @zewt the issue is in yaml-cpp:
>
> https://github.com/jbeder/yaml-cpp/blob/master/src/convert.cpp
>
>
> As you can see, on/off are interpreted as booleans. And they explicitly reference the same page that I shared as well.

[^435]

## #716 Support serialising std::expected

Issue · opened by @saxbophone · closed (completed)

### 2026-08-23 · comment

Replying to: @saxbophone (opening post) — "If the user is using C++23, then `std::expected` is available, but reflect-cpp doesn't support serialising it. I suppose for now I can just write a polyfill …"

> @saxbophone I think representing std::expected as either a variant or a tagged Union would be the best option.

[^436]

### 2026-08-24 · comment

> @saxbophone the issue is resolved. Thanks for the idea.

[^437]

### 2026-08-25 · comment

Replying to: @saxbophone — "Wow, that was quick! Thank you very much for the addition, this will be a great help in some downstream projects. I like the decision to use an explicit error …"

> @saxbophone I decided to add the error field because that is how we handle rfl::Result and I wanted to keep it consistent.

[^438]

## #721 Output zero flag name if available

PR · opened by @ZXShady · merged

- 2026-08-29 · comment · ack: “@ZXShady thanks for the contribution!” (replying to @ZXShady (opening post)) [^439]

## #720 Reject externally tagged variants with more than one field

PR · opened by @ciprianf-flexion · merged

### 2026-08-29 · comment

Replying to: @ciprianf-flexion — "@liuzicheng1987 I cannot add you explicitly as a reviewer, but would you mind taking a look? Thanks!"

> @ciprianf-flexion you don't have to explicitly add me as a reviewer, I will take a look.

[^440]

### 2026-08-29 · comment

> @ciprianf-flexion good catch. I never really thought of that problem.

[^441]

- 2026-08-29 · comment · ack: “@ciprianf-flexion thanks for the contribution. I am merging.” [^442]

## #723 Support native non-string map keys for formats that allow them

Issue · opened by @awake23 · open

### 2026-09-07 · comment

Replying to: @awake23 (opening post) — "Some serialization formats supported by reflect-cpp, such as CBOR and MessagePack, allow map keys to be types other than strings. Currently, reflect-cpp's …"

> @awake23 OK, but I would have to think about how to make this backwards-compatible.

[^443]

## #719 Segfault in rfl::json::read on GCC 12 -O2: read_struct builds its view over a T whose lifetime never began

Issue · opened by @shaulnv · closed (completed)

### 2026-09-07 · comment

Replying to: @shaulnv (opening post) — "## Summary `rfl::json::read<T>()` segfaults with GCC 12 at `-O2` for certain structs. The generated field view holds garbage pointers, and …"

> @shaulnv thanks for pointing this out. I remember that there was a similar issue with GCC 12 (but not other compiler). I will take a look.

[^444]

### 2026-10-18 · comment

> @shaulnv I cannot reproduce the issue. I copied your code into a file called "test_gcc12.cpp" and then ran the following:
>
> ```
> patrick@patrick-NucBox-EVO-X2:~/reflect-cpp$ g++-12 --version
> g++-12 (Ubuntu 12.4.0-2ubuntu1~24.04.1) 12.4.0
> Copyright (C) 2022 Free Software Foundation, Inc.
> This is free software; see the source for copying conditions.  There is NO
> warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
>
> patrick@patrick-NucBox-EVO-X2:~/reflect-cpp$ gcc-12 -O2 -I "./include/rfl/thirdparty" -c "./src/yyjson.c" -o yyjson.o
> patrick@patrick-NucBox-EVO-X2:~/reflect-cpp$ g++-12 -std=c++20 -O2 -I "./include"       test_gcc12.cpp "./src/reflectcpp.cpp" "./src/reflectcpp_json.cpp" yyjson.o -o repro
> patrick@patrick-NucBox-EVO-X2:~/reflect-cpp$ ./repro
> OK
> ```
>
> And it worked just fine.

[^445]

### 2026-10-18 · comment

> @shaulnv , there is an open PR which should resolve the issue, but it leads to some performance regression as well as other problems, so I am unsure how to proceed. But, can you confirm that this resolves the issue?
>
> https://github.com/getml/reflect-cpp/pull/737

[^446]

### 2026-10-18 · comment

> @shaulnv alternatively, there is this less invasive approach (not very polished):
>
> https://github.com/getml/reflect-cpp/tree/f/resolve_g%2B%2B12_issue
>
> Could you try this as well?

[^447]

### 2026-10-20 · comment

Replying to: @Altren — "I managed to reproduce issue inside compiler explorer. Replacing [`using DataType = std::array<unsigned char, …"

> @Altren excellent.
>
> I do get the impression that this is a compiler bug.
>
> I know that both of my fixes are pretty unpolished. If your solution does the trick, let's just go with that.

[^448]

## #724 Add support for `std::inplace_vector`

Issue · opened by @cacharle · closed (completed)

### 2026-09-14 · comment

Replying to: @cacharle (opening post) — "https://cppreference.net/cpp/container/inplace_vector.html It got added in C++26. Would probably be easy to implement since it can be (de)serialized the same …"

> OK

[^449]

## #725 fix `Generic.cpp` build error due to missing includes

PR · opened by @gg-sr · merged

### 2026-09-21 · comment

Replying to: @gg-sr (opening post) — "I'm not sure why it works for others, but not for me. Maybe different compilation flags, since I'm wrapping the library in Bazel and not using the provided …"

> @gg-sr that is weird, because we have pipeline checks just for that. But thanks for the contribution!

[^450]

## #727 toml: allow reading integer as double

PR · opened by @gg-sr · merged

### 2026-09-27 · comment

Replying to: @gg-sr (opening post) — "This allows reading `a = 2` if `a` is a `double` or `float`, rather than having to rewrite `a = 2.0`."

> @gg-sr thank you for this contribution. I have merged.

[^451]

## #729 add `from_reflection(ReflectionType&&)` as alternative to ctor

PR · opened by @gg-sr · merged

### 2026-09-27 · comment

Replying to: @gg-sr (opening post) — "So classes can still be aggregate-initialized."

> @gg-sr I love this idea. Thank you for the contribution. I am merging.

[^452]

## #726 fix `CountFieldsHelper<T>` when a field is `std::optional<Eigen::Matrix4d>`

PR · opened by @gg-sr · closed without merge

### 2026-09-27 · comment

Replying to: @gg-sr (opening post) — "`CountFieldsHelper<T>` inferred the number of fields of a struct by aggregate-initializing it with as many `any` values as possible, `any` being a type that …"

> @gg-sr there still seem to be quite a few issues with this. Could you take another look?

[^453]

### 2026-09-28 · comment

Replying to: @gg-sr — "@liuzicheng1987 I kept trying to make it work, in vain. I think we'll keep this patch on our side since it's necessary for `std::optional<Eigen::Matrix4d>` …"

> @gg-sr No bother at all. Thank you for all your excellent contributions!

[^454]

## #728 emit `"additionalProperties": false` with `rfl::NoExtraFields`

PR · opened by @gg-sr · merged

### 2026-09-27 · comment

Replying to: @gg-sr (opening post) — ""

> @gg-sr I think this makes a lot of sense, but the tests need to be adapted to make this work. Could you take care of that?

[^455]

### 2026-10-26 · comment

Replying to: @Altren — "@liuzicheng1987 The only CI failure here is vcpkg hiccup, I think it should be merged if that was the only blocker"

> @Altren, indeed, you are right.
> @gg-sr thanks for the contribution and sorry it took me so long to merge.

[^456]

## #730 fix `NamedTupleParser::to_schema()` for non-`DefaultVal` fields

PR · opened by @gg-sr · merged

- 2026-09-30 · comment · ack: “@gg-sr thanks for the contribution. I am merging.” (replying to @gg-sr (opening post)) [^457]

## #731 Add ability to rename enum values

Issue · opened by @gg-sr · open

### 2026-09-30 · comment

Replying to: @gg-sr (opening post) — "Right now, processors allow us to rename field names e.g. from `snake_case` to `camelCase` (good to go e.g. from C++ to JS). It would be nice if we could do …"

> @gg-sr Good question, let me think about what the best approach here would be. My first instinct would be a processor, but I need to give it some thought.

[^458]

## #733 Add inplace vector support

PR · opened by @cacharle · merged

- 2026-10-09 · comment · ack: “@cacharle thanks for the contribution! I am merging.” (replying to @cacharle (opening post)) [^459]

## #735 Add DurationsAsCount preprocessor

PR · opened by @cacharle · merged

- 2026-10-09 · comment · ack: “@cacharle thanks for the contribution! I am merging this one as well.” (replying to @cacharle (opening post)) [^460]

## #736 Reject trailing characters when parsing rfl::Timestamp

PR · opened by @fhgffy · merged

### 2026-10-09 · comment

Replying to: @fhgffy (opening post) — "`rfl::Timestamp` accepts strings with extra characters after the format: ```cpp using TS = rfl::Timestamp<'%Y-%m-%d'>; TS::from_string('1987-04-19T10:00:00'); …"

> @fhgffy this look good to me. Let me run the test pipelines and if they run through cleanly, I will merge.

[^461]

### 2026-10-09 · comment

Replying to: @fhgffy — "Thanks for running the pipelines. The Linux and Windows workflows passed. The remaining macOS Intel benchmark job stopped while vcpkg was downloading Thrift: …"

> @fhgffy OK, I'm re-running

[^462]

- 2026-10-11 · comment · ack: “@fhgffy thanks for the contribution! I am merging.” [^463]

## #737 Initialize structs using default values before parsing; resolves #719 and #700

PR · opened by @liuzicheng1987 · closed without merge

### 2026-10-13 · comment

>  PR #737 Review — Main Findings
>
>  PR: "Initialize structs using default values before parsing" (fixes #719, #700). Replaces the raw-byte-buffer + placement-new + manual-destructor parsing scheme with real value initialization (T t{}) + assignment; makes rfl::Validator store a Result<T> so it can
>  hold an error state without throwing.
>
>  Bugs (verified at PR head, GCC 13.3, repro code in /tmp/rflprobe/)
>
>  1. HIGH — std::terminate (SIGABRT) parsing a map with an invalid Validator key (regression)
>  rfl::json::read<std::map<rfl::Validator<int, rfl::Minimum<1>>, std::string>>(R"({"0": "zero"})") → terminate called..., exit 134.
>  Root cause: new MapReader::make_key is_validator_v branch rebuilds the key via KeyT(std::move(_p.first)) — the throwing Validator(T&&) constructor (validates, .value() throws) — inside a Result::transform lambda (no exception handling, verified in
>  internal/cpp20/Result.hpp), and make_key is const noexcept → escape = terminate. Pre-PR, Validator keys went through the ReflectionType branch whose lambda had try/catch → graceful Result error. Invalid map keys that used to return an error now abort the program.
>
>  2. HIGH — non-default-constructible structs no longer compile (regression)
>  read<NDC> where NDC has only a parameterized constructor → hard error at Parser_default.hpp:692 (T t{}; — "no matching function for NDC::NDC"). The old placement-new approach explicitly supported such types (the comment still above read_struct claims so — now
>  stale). Same new hard requirement propagates to named tuples (NamedTuple() = default + nt; in NamedTupleParser::read), arrays (std::array<...> arr{} in ParserArray), and tuples (tup{} in TupleParser). No test covers non-default-constructible targets, so CI won't
>  catch it.
>
>  3. HIGH — silent data corruption: const Validator field loses its parsed value
>  struct { const rfl::Validator<unsigned, rfl::Minimum<0>> age; const std::string name; } ← {"age": 8, "name": "Maggie"} → parse succeeds with age == 1 (write-back: {"age":1,...}). Verified: the has_const_v branch in read_struct is taken (instrumented), and the
>  identical manual steps (read<named_tuple_t<T>> + from_named_tuple<T>) yield age == 8 — so the corruption is inside the branch's pipeline; exact line not yet pinned down. Silent wrong value on the PR's headline new path is the worst failure mode.
>
>  Confirmed fixes (PR goals work)
>
>  - #700 fixed: DefaultVal + Size-validator struct now parses (favorite_foods: ["donuts","pizza"] succeeds; {} returns a proper "required field" error instead of aborting).
>  - #719 path fixed: the 12-optional-fields + nested-optional-struct repro reads fine at -O2. The UB that caused the GCC 12 segfault (view bound to members of a buffer whose lifetime never began) is gone — T t{} is a real object. (The GCC-12-specific miscompile itself
>    can't be reproduced here; only GCC 13 available.)
>  - Const std::string fields (plain, rfl::Rename, nested in std::vector) parse and write correctly when all fields are present in the JSON. Missing const fields still error "Field named X not found" — same as pre-PR, consistent.
>  - Destructor machinery removed cleanly: zero dangling references to the three deleted call_destructors_* headers (grepped include/, tests/, src/).
>
>  Secondary findings (code-reading, not runtime-verified)
>
>  - Delayed-error landmine (by design, undocumented): a parse can succeed while a Validator field holds an error state (invalid default + field missing under DefaultIfMissing); the first later value()/get()/* call then throws. ParserValidator::write also throws via
>    .value() for error-state validators.
>  - NamedTuple with const non-pointer fields: no has_const_v escape hatch in NamedTupleParser → likely compile error at the rfl::get<i>(*_view) = ... assignment (and move_to's surviving const_cast branch is now UB if ever reached, since views point at real const
>    members).
>  - Dead code: internal/is_validator.hpp is now unused (leftover include in ParserBasicType.hpp:18); ParserValidator.hpp drags in unused heavy headers (FieldVariantParser, VariantVariantWrapper, <variant>, visit, …) into every TU that includes it.
>  - read_struct_with_default lost its try/catch around T t{} — user default constructors that throw (bad_alloc etc.) now escape json::read instead of becoming a Result error.
>  - test_const_fields.cpp is fully commented out with a bare // TODO — the const-field feature ships untested (and finding #3 lives on that path).
>
>  Bottom line
>
>  The core mechanism (real objects instead of placement-new; error-holding Validator) is sound and fixes both target issues, but as-is the PR should not merge: two crash/regression bugs (map-key terminate, non-default-constructible compile break), one silent
>  corruption bug (const Validator), and the flagship const-field test is disabled. Findings 1–2 are small fixes; finding 3 needs a root-cause pass in the read_struct const branch.
>
>  Still open when interrupted: pinning the exact corruption line (3), a backtrace for the terminate (1), and per-file pass over the untouched areas (MapParser write refactor, ArrowReader — its return std::move(*ptr) suggests it may still use the old buffer,
>  tuple/array const reconstruction, leak tests). Say the word and I'll dispatch the 4 reviewer agents to complete the sweep.

[^464]

## #739 Reserve vector capacity when array lengths are known

PR · opened by @Altren · merged

- 2026-10-14 · comment · ack: “@Altren no problem” (replying to @Altren) [^465]
- 2026-10-16 · comment · ack: “@Altren thanks for the contribution.” [^466]

## #738 Avoid eager error allocation when parsing named tuples

PR · opened by @Altren · merged

### 2026-10-14 · comment

Replying to: @Altren — "FYI. I'm trying to make reflect-cpp work as fast as native Cereal serialization does, in my production case it is 2x slower for write and 3x slower for read. …"

> @Altren, great.
>
> Thank for this. I am merging.

[^467]

## #740 Optimize numeric vectors in Cereal, YAS, and Boost binary

PR · opened by @Altren · merged

- 2026-10-16 · comment · ack: “That's a great contribution. Thanks!” (replying to @Altren (opening post)) [^468]

## #741 Optimize positional field dispatch with compile-time indices

PR · opened by @Altren · merged

- 2026-10-16 · comment · ack: “Thanks for the contribution.” (replying to @Altren (opening post)) [^469]

## #742 Inline frequently used parsing functions

PR · opened by @Altren · merged

- 2026-10-16 · comment · ack: “Thanks for the contribution! This is excellent!” (replying to @Altren (opening post)) [^470]

## #743 Improve shared parser inlining and generated code layout

PR · opened by @Altren · merged

### 2026-10-18 · comment

Replying to: @Altren — "While testing #744 with Linux+GCC I found that this PR decrease LTO GCC performance by ~40% and increase no-LTO by ~25%, so I guess it might be better cancel …"

> @Altren OK, if you think that it is better to cancel the PR, then do so.

[^471]

### 2026-10-18 · comment

Replying to: @Altren — "I'll try to make it good on lixux+gcc and macos+clang. Doing experiments takes much more time, but I think I can get to a point where reflect-cpp is within …"

> @Altren yes, I think adding a "perfect result benchmark" for cereal, yas, etc would be a very good idea. We have something like this for JSON and it would be interesting to see that for these formats as well.

[^472]

### 2026-10-18 · comment

> @Altren by the way, I would also like to do a release quite today.
>
> I will do a 0.26.0 release for now, but I think we should move this out of beta (ergo a 1.0.0 release) once I have made a decision on how to proceed with https://github.com/getml/reflect-cpp/issues/719.
>
> So, in other words, we will have another release quite soon.
>
> I am also going to move the repo to a different account, but that shouldn't affect your work too much.

[^473]

### 2026-10-18 · comment

> @Altren But I think that performance improvements should be a major goal moving forward. We are still slower than YYJSON on most benchmarks. Not by much, so it wasn't something I cared about in the past, but I think now is the time to really close that gap.

[^474]

- 2026-10-20 · comment · ack: “Thanks! I am merging!” (replying to @Altren) [^475]

## #744 Add opt-in FailFast processor for object field errors

PR · opened by @Altren · merged

### 2026-10-18 · comment

Replying to: @Altren (opening post) — "EDIT: Turned out that main performance gains are applicable in top of #743 , and without it the gain is small. And overall default non-FailFast version is …"

> @Altren OK, this one looks good to me. I will merge it. Thank you for the contribution.

[^476]

## #746 Reject out-of-range JSON integer values

PR · opened by @sean0407 · merged

- 2026-10-18 · comment · ack: “@sean0407 thanks for the contribution. I am merging.” (replying to @sean0407 (opening post)) [^477]

## #749 Add support for `std::monostate`

Issue · opened by @cacharle · open

### 2026-10-19 · comment

Replying to: @cacharle (opening post) — "https://en.cppreference.com/cpp/utility/variant/monostate Declared in the standard to be used as an empty alternative for a `std::variant`. I can currently use …"

> @cacharle looks good. Do you want to open a PR or should I do it?

[^478]

## #753 Read files in bulk and report output stream failures

PR · opened by @Altren · merged

### 2026-10-22 · comment

Replying to: @Altren — "The only test failure is an `apt update` issue"

> @Altren is this ready for review/merge?

[^479]

- 2026-10-24 · comment · ack: “@Altren thank you for your contribution. I am merging!” (replying to @Altren) [^480]

## #752 Work around GCC 12 optional parsing crash with native tuple storage

PR · opened by @Altren · merged

- 2026-10-24 · comment · ack: “@Altren great, thank you!” (replying to @Altren (opening post)) [^481]

## #758 Restore CI binary caching and retry vcpkg downloads

PR · opened by @Altren · merged

- 2026-10-25 · comment · ack: “@Altren thanks for the contribution.” (replying to @Altren (opening post)) [^482]

## #755 Omit redundant fixed-array lengths in Cereal, YAS and Boost.Serialization

PR · opened by @Altren · merged

- 2026-10-25 · comment · ack: “@Altren thanks!” (replying to @Altren (opening post)) [^483]

## #757 Avoid copying Cereal input buffers before deserialization

PR · opened by @Altren · merged

- 2026-10-25 · comment · ack: “@Altren thanks” (replying to @Altren (opening post)) [^484]

## #756 Preserve typed map keys in MessagePack and binary archives

PR · opened by @Altren · merged

- 2026-10-25 · comment · ack: “@Altren thanks” (replying to @Altren) [^485]

## #759 Fix fixed-size arrays in typed map entries

PR · opened by @Altren · merged

### 2026-10-25 · comment

Replying to: @Altren — "That covers all the major performance issues I found while porting my project’s serialization to reflect-cpp, so I don’t expect to submit any more major …"

> @Altren, OK. There is one more thing I have to do before I can do a release, though. I promised the vcpkg-people that I would fix some issues related to enchantum.
>
> But I will take care of it over the next couple of days.

[^486]

- 2026-10-25 · comment · ack: “@Altren thank you for these contributions.” [^487]

## #760 Add bulk I/O missed in the recent std::array optimization

PR · opened by @Altren · merged

### 2026-10-26 · comment

Replying to: @Altren (opening post) — "Numeric std::array values still use element-by-element I/O in Cereal, YAS, and Boost binary archives, despite the existing bulk paths for numeric vectors. Add …"

> @Altren all right, thanks for the contribution

[^488]

## #761 Add std::bitset support with packed binary and string encodings

PR · opened by @Altren · merged

- 2026-10-26 · comment · ack: “@Altren thanks for the contribution” (replying to @Altren (opening post)) [^489]

### 2026-10-26 · comment

> @Altren there appear to be merge conflicts. Could you take a look?

[^490]

- 2026-10-27 · comment · ack: “@Altren thanks” (replying to @Altren) [^491]

## #762 Allow importing enchantum as a thirdparty library

PR · opened by @liuzicheng1987 · merged

### 2026-11-01 · comment

Replying to: @Altren — "@liuzicheng1987 What is the current plan for release?"

> @Altren sorry, I've been quite busy over the weekend. I will take care of it tomorrow.

[^492]

## #768 Clang-23 compilation error with -std=26

Issue · opened by @anders-wind · open

### 2026-11-05 · comment

Replying to: @anders-wind (opening post) — "There's a bug in clang when compiling with -std=26, and reflectcpp is hit by that. ``` e.hpp:23:10: error: no matching constructor for initialization of …"

> @anders-wind OK. Since you seem to have a fix, could you open a PR?

[^493]

## #766 Expose the location of each parse failure via rfl::Error::details()

PR · opened by @clockblocker · merged

### 2026-11-05 · inline include/rfl/internal/Error.hpp:121

> @clockblocker Could you use rfl::Box here? Clearly, the implicit assumption here is that DetailsPtr should not be NULL, so rfl::Box establishes that more clearly.
>
> Moreover, if you use rfl::Box, you won't have to implement the copy operations by hand.

[^494]

### 2026-11-05 · inline include/rfl/internal/Error.hpp:124

> This is what I mean...rfl::Box can handle that more elegantly.

[^495]

### 2026-11-05 · comment

Replying to: @clockblocker — "A note on why `ErrorDetail::PathSegment` is a `std::variant<std::string, size_t>` rather than an `rfl::Variant`, even though the docs recommend `rfl::Variant`: …"

> > A note on why `ErrorDetail::PathSegment` is a `std::variant<std::string, size_t>` rather than an `rfl::Variant`, even though the docs recommend `rfl::Variant`:
> >
> > * `rfl::Variant` is recommended mainly for compile times, "particularly for variants with many alternatives". A path segment has two alternatives, so the gain would be negligible.
> > * `internal/Error.hpp` reaches almost every translation unit through `Result.hpp`. Depending only on the standard library keeps it light and avoids pulling `rfl/Variant.hpp` and its internals into all of them.
> > * It matches `rfl::Generic`, whose public `VariantType` is a `std::variant` too, and users can read a segment with plain `std::visit` or `std::get_if`.
> >
> > If you'd prefer `rfl::Variant` here, it's a one-line type change plus the visits in the tests and docs. I'm happy to switch.
>
> `std::variant` is fine.

[^496]

### 2026-11-05 · comment

> @clockblocker the PR looks great. Thanks for the initiative. I have minor comments (see above). Also, the build fails on at least some of the compiler settings. Could you take a look?

[^497]

### 2026-11-05 · comment

Replying to: @clockblocker — "Thank you for the review. Don't see any 'Minor comments' beyond the failing build, though. --- Ai-authored: **Build failure:** fixed in 0cc1fcd. The new test …"

> @clockblocker you should see the minor comments now

[^498]

### 2026-11-07 · inline include/rfl/internal/Error.hpp:121

Replying to: @clockblocker (inline) — "Sorry for the delay. This turned out to be less straightforward than it seemed. While the end user sees both `path` and `message` as non-nullable, internally …"

> OK

[^499]

### 2026-11-07 · inline include/rfl/internal/Error.hpp:124

Replying to: @clockblocker (inline) — "Ai-authored: `copy_details` is gone in a038bc0: the deep copy now lives in `Error::OptionalDetails`, and `Error`'s copy operations are `= default` again. The …"

> OK

[^500]

### 2026-11-07 · comment

> @clockblocker all right, thank you very much for this contribution. I will wait for the remaining checks to run through and then merge.

[^501]

## #767 Avoid needless copies, allocations and exceptions when reading and writing

PR · opened by @clockblocker · merged

- 2026-11-07 · comment · ack: “@clockblocker thanks for the contribution!” (replying to @clockblocker (opening post)) [^502]

### 2026-11-07 · comment

> @clockblocker I am merging this now. (Sorry, I accidentally clicked on close.)

[^503]

## #769 Make rfl::CopyableBox copy-assignable

PR · opened by @clockblocker · merged

### 2026-11-07 · comment

Replying to: @clockblocker (opening post) — "`rfl::CopyableBox` is documented as copyable, but only its copy constructor works. `box2 = box1` does not compile, and neither does copy-assigning a struct …"

> @clockblocker OK, looks great. I will merge this as well. Thank you again for your contributions!
>
> And since we're both from Munich, maybe we will one day meet in person at some C++ meetup.

[^504]

## #777 Inquiry about the format of contributions  

Issue · opened by @clockblocker · open

### 2026-11-11 · comment

Replying to: @clockblocker (opening post) — "I am trying to use the rfl::cli::read<T> as an entry point for a refactor of a complex CLI parser in prod. When using it as is, some behavior gaps showed up. …"

> Either way is fine. I'm not too picky.

[^505]

## #770 Support JSON bytestrings as Base64 strings

PR · opened by @hazby2002 · merged

- 2026-11-11 · comment · ack: “@hazby2002 thanks so much for your PR!” (replying to @hazby2002 (opening post)) [^506]
- 2026-11-12 · comment · ack: “@hazby2002 , thanks, I am merging!” [^507]

## #776 Return an error when an rfl::Skip field's default value throws

PR · opened by @clockblocker · merged

- 2026-11-12 · comment · ack: “@clockblocker thanks. I am merging.” (replying to @clockblocker (opening post)) [^508]

## #778 Let DefaultVal's converting constructor throw

PR · opened by @clockblocker · merged

- 2026-11-12 · comment · ack: “@clockblocker thanks for the contribution. I am merging.” (replying to @clockblocker (opening post)) [^509]

## #779 Say why default construction failed when reading a struct with defaults

PR · opened by @clockblocker · merged

- 2026-11-12 · comment · ack: “@clockblocker thanks for the contribution. I am merging.” (replying to @clockblocker (opening post)) [^510]

## #782 Update a DefaultVal test to the error message from #779

PR · opened by @clockblocker · merged

- 2026-11-15 · comment · ack: “@clockblocker thanks” (replying to @clockblocker (opening post)) [^511]

## #783 Read a bare negative number such as `-5` as a positional argument in rfl::cli

PR · opened by @clockblocker · merged

### 2026-11-15 · comment

Replying to: @clockblocker (opening post) — "## Why In rfl::cli an `rfl::Positional<int>` can't take a negative number: `-5` fails with `Unknown short argument: -5`. …"

> @clockblocker could you merge main so that the tests run through?

[^512]

- 2026-11-16 · comment · ack: “@clockblocker thanks for the contribution. I am merging.” [^513]

## #785 Write optional fields wrapped in rfl::Short or rfl::Positional

PR · opened by @clockblocker · merged

### 2026-11-15 · comment

Replying to: @clockblocker (opening post) — "## Problem Currently, using `rfl::json::write` fails to compile (valid for`rfl::cli::read`) structs with optional fields wrapped in `rfl::Short` or …"

> @clockblocker looks good to me, could you merge main, so the tests run through?

[^514]

- 2026-11-16 · comment · ack: “@clockblocker thanks for the contribution. I am merging.” [^515]

## #786 Fix rfl::DefaultVal in NamedTuples and TaggedUnion schemas

PR · opened by @clockblocker · open

### 2026-11-15 · inline include/rfl/parsing/NamedTupleParser.hpp:1

> I am pretty sure this is UB...
>
> "Modifying a const object through a non-const access path and referring to a volatile object through a non-volatile [glvalue](https://en.cppreference.com/cpp/language/value_category#glvalue) results in undefined behavior."
>
> https://en.cppreference.com/cpp/language/const_cast

[^516]

### 2026-11-16 · inline tests/json/test_default_val_without_default_construction.cpp:104

> To be honest, I do not like that having a field without a default constructor will change the behaviour of `rfl::DefaultVal`.
>
> Here's why:
>
> ```cpp
> struct DefaultAndNoDefaultConstructor {
>   rfl::DefaultVal<int> jobs = 4; // This is the real reason rfl::DefaultVal exists
> };
> ```
>
> The reason we introduced rfl::DefaultVal and the Processor in the first place is that people intuitively expected this to behave this way and were disappointed when it didn't. Imagine you start out with a field like this. Now the default value for jobs is 4. Then you add `NoDefaultConstructor n;`. All of a sudden, your app behaves differently at runtime then before. This is very unexpected and feels like bug.
>
> Sure, it's in the documentation. But very few people read every page of the documentation before using a library...this is going to confuse people and people are going to open issues about this.

[^517]

## #781 Binary data in `rfl::Generic`

Issue · opened by @jinchengJL · open

### 2026-11-15 · comment

Replying to: @jinchengJL (opening post) — "Currently, Generic's variant list is: ```cpp using VariantType = std::variant<bool, int64_t, double, std::string, Object, Array, std::nullopt_t>; ``` This list …"

> @jinchengJL I think adding bytestrings here should be possible

[^518]

## #784 Accept const argv and ranges of arguments in rfl::cli::read

PR · opened by @clockblocker · open

### 2026-11-16 · comment

Replying to: @clockblocker (opening post) — "## AI-authored ### Why Boost.Program_options, CLI11 and cxxopts take `const char* const*` argv, and Boost.Program_options and CLI11 also take a …"

> @clockblocker there appear to be merge conflicts because of your other PR. Could you fix them?

[^519]

## #787 Don't assign to const fields when reading a struct with defaults

PR · opened by @clockblocker · merged

- 2026-11-16 · comment · ack: “@clockblocker thank you for this contribution. I am merging.” (replying to @clockblocker (opening post)) [^520]

## Sources

[^1]: <https://github.com/munich-data-engineering/reflect-cpp/issues/435#issuecomment-2893931183> — #435 Serialising anything other than simple types fails to compile (2025-05-20)
[^2]: <https://github.com/munich-data-engineering/reflect-cpp/issues/435#issuecomment-2893935884> — #435 Serialising anything other than simple types fails to compile (2025-05-20)
[^3]: <https://github.com/munich-data-engineering/reflect-cpp/issues/436#issuecomment-2895652651> — #436 rfl::toml::read() doesn't compile (cannot convert from 'toml::v3::noex::parse_result *' to 'rfl::toml::InputVarType') (2025-05-20)
[^4]: <https://github.com/munich-data-engineering/reflect-cpp/issues/436#issuecomment-2896712005> — #436 rfl::toml::read() doesn't compile (cannot convert from 'toml::v3::noex::parse_result *' to 'rfl::toml::InputVarType') (2025-05-21)
[^5]: <https://github.com/munich-data-engineering/reflect-cpp/issues/436#issuecomment-2956306506> — #436 rfl::toml::read() doesn't compile (cannot convert from 'toml::v3::noex::parse_result *' to 'rfl::toml::InputVarType') (2025-06-09)
[^6]: <https://github.com/munich-data-engineering/reflect-cpp/issues/439#issuecomment-2909619619> — #439 Field value of named tuple was moved after apply (2025-05-26)
[^7]: <https://github.com/munich-data-engineering/reflect-cpp/issues/439#issuecomment-2910651618> — #439 Field value of named tuple was moved after apply (2025-05-26)
[^8]: <https://github.com/munich-data-engineering/reflect-cpp/issues/439#issuecomment-2913949682> — #439 Field value of named tuple was moved after apply (2025-05-27)
[^9]: <https://github.com/munich-data-engineering/reflect-cpp/pull/440#issuecomment-2915070307> — #440 Made sure that apply can never move or modify the tuple values; fixes… (2025-05-28)
[^10]: <https://github.com/munich-data-engineering/reflect-cpp/pull/441#issuecomment-2918859182> — #441 Add `enchantum` as an enum reflection backend (2025-05-29)
[^11]: <https://github.com/munich-data-engineering/reflect-cpp/pull/441#issuecomment-2919089752> — #441 Add `enchantum` as an enum reflection backend (2025-05-29)
[^12]: <https://github.com/munich-data-engineering/reflect-cpp/pull/441#issuecomment-2919093414> — #441 Add `enchantum` as an enum reflection backend (2025-05-29)
[^13]: <https://github.com/munich-data-engineering/reflect-cpp/pull/441#issuecomment-2920117831> — #441 Add `enchantum` as an enum reflection backend (2025-05-29)
[^14]: <https://github.com/munich-data-engineering/reflect-cpp/pull/441#issuecomment-2920143633> — #441 Add `enchantum` as an enum reflection backend (2025-05-29)
[^15]: <https://github.com/munich-data-engineering/reflect-cpp/pull/441#issuecomment-2920599342> — #441 Add `enchantum` as an enum reflection backend (2025-05-29)
[^16]: <https://github.com/munich-data-engineering/reflect-cpp/pull/442#issuecomment-2925737095> — #442 Fix cbor encoding for uint64_t's (2025-05-31)
[^17]: <https://github.com/munich-data-engineering/reflect-cpp/issues/443#issuecomment-2942932289> — #443 Suggestion: Deep comparison for `rfl::to_view()` (2025-06-05)
[^18]: <https://github.com/munich-data-engineering/reflect-cpp/issues/443#issuecomment-2948030613> — #443 Suggestion: Deep comparison for `rfl::to_view()` (2025-06-06)
[^19]: <https://github.com/munich-data-engineering/reflect-cpp/issues/443#issuecomment-2948042584> — #443 Suggestion: Deep comparison for `rfl::to_view()` (2025-06-06)
[^20]: <https://github.com/munich-data-engineering/reflect-cpp/issues/443#issuecomment-2948044880> — #443 Suggestion: Deep comparison for `rfl::to_view()` (2025-06-06)
[^21]: <https://github.com/munich-data-engineering/reflect-cpp/issues/443#issuecomment-2948784453> — #443 Suggestion: Deep comparison for `rfl::to_view()` (2025-06-06)
[^22]: <https://github.com/munich-data-engineering/reflect-cpp/pull/445#issuecomment-2954183843> — #445 Improved support for string_views for TOML and XML (2025-06-08)
[^23]: <https://github.com/munich-data-engineering/reflect-cpp/pull/445#issuecomment-2954191769> — #445 Improved support for string_views for TOML and XML (2025-06-08)
[^24]: <https://github.com/munich-data-engineering/reflect-cpp/pull/445#issuecomment-2954201886> — #445 Improved support for string_views for TOML and XML (2025-06-08)
[^25]: <https://github.com/munich-data-engineering/reflect-cpp/pull/445#issuecomment-2954207997> — #445 Improved support for string_views for TOML and XML (2025-06-08)
[^26]: <https://github.com/munich-data-engineering/reflect-cpp/issues/444#issuecomment-2954227642> — #444 Don't use `std::string_view` with APIs expecting null-terminated strings (2025-06-08)
[^27]: <https://github.com/munich-data-engineering/reflect-cpp/pull/447#discussion_r2136049571> — #447 Added support for exception-free TOML; resolves #436 (2025-06-09)
[^28]: <https://github.com/munich-data-engineering/reflect-cpp/pull/447#discussion_r2136050335> — #447 Added support for exception-free TOML; resolves #436 (2025-06-09)
[^29]: <https://github.com/munich-data-engineering/reflect-cpp/pull/448#issuecomment-2960747406> — #448 Dont repeat class names template arguements (2025-06-10)
[^30]: <https://github.com/munich-data-engineering/reflect-cpp/issues/449#issuecomment-2967779131> — #449 Flatten<TaggedUnion<...>> isn't flattened but instead becomes a "variant_" implicit field. (2025-06-12)
[^31]: <https://github.com/munich-data-engineering/reflect-cpp/issues/451#issuecomment-2982140014> — #451 Reflect on the number of fields of a too-complex type at compile time? (2025-06-17)
[^32]: <https://github.com/munich-data-engineering/reflect-cpp/issues/451#issuecomment-2989290233> — #451 Reflect on the number of fields of a too-complex type at compile time? (2025-06-19)
[^33]: <https://github.com/munich-data-engineering/reflect-cpp/issues/452#issuecomment-2989288268> — #452 Are there any examples on how to serialize / deserialize a json tree? (2025-06-19)
[^34]: <https://github.com/munich-data-engineering/reflect-cpp/issues/452#issuecomment-2989289027> — #452 Are there any examples on how to serialize / deserialize a json tree? (2025-06-19)
[^35]: <https://github.com/munich-data-engineering/reflect-cpp/issues/452#issuecomment-2989897637> — #452 Are there any examples on how to serialize / deserialize a json tree? (2025-06-20)
[^36]: <https://github.com/munich-data-engineering/reflect-cpp/pull/453#issuecomment-3035287283> — #453 added REFLECTCPP_INSTALL to allow user to control if required (2025-07-04)
[^37]: <https://github.com/munich-data-engineering/reflect-cpp/issues/455#issuecomment-3035300942> — #455 Feature request: Allow a 'fallback' type for tagged union (2025-07-04)
[^38]: <https://github.com/munich-data-engineering/reflect-cpp/issues/454#issuecomment-3042608420> — #454 Support Deserialize std::string_view (2025-07-06)
[^39]: <https://github.com/munich-data-engineering/reflect-cpp/issues/454#issuecomment-3047551724> — #454 Support Deserialize std::string_view (2025-07-08)
[^40]: <https://github.com/munich-data-engineering/reflect-cpp/pull/456#issuecomment-3050541974> — #456 rename _res to _result to avoid macro conflict (2025-07-08)
[^41]: <https://github.com/munich-data-engineering/reflect-cpp/issues/457#issuecomment-3062903211> — #457 Support for `std::span` (2025-07-11)
[^42]: <https://github.com/munich-data-engineering/reflect-cpp/issues/463#issuecomment-3071255153> — #463 Json Read rfl::UnderlyingEnum processor is broken (2025-07-14)
[^43]: <https://github.com/munich-data-engineering/reflect-cpp/issues/463#issuecomment-3094799514> — #463 Json Read rfl::UnderlyingEnum processor is broken (2025-07-20)
[^44]: <https://github.com/munich-data-engineering/reflect-cpp/issues/463#issuecomment-3156286737> — #463 Json Read rfl::UnderlyingEnum processor is broken (2025-08-05)
[^45]: <https://github.com/munich-data-engineering/reflect-cpp/issues/463#issuecomment-3170674087> — #463 Json Read rfl::UnderlyingEnum processor is broken (2025-08-09)
[^46]: <https://github.com/munich-data-engineering/reflect-cpp/issues/463#issuecomment-3476031040> — #463 Json Read rfl::UnderlyingEnum processor is broken (2025-11-01)
[^47]: <https://github.com/munich-data-engineering/reflect-cpp/issues/459#issuecomment-3071255856> — #459 Treat `std::vector<char>` similar to `rfl::Bytestring` (2025-07-14)
[^48]: <https://github.com/munich-data-engineering/reflect-cpp/pull/462#issuecomment-3077986185> — #462 cbor supports std::optional; fixes #135 (2025-07-16)
[^49]: <https://github.com/munich-data-engineering/reflect-cpp/pull/462#issuecomment-3080896201> — #462 cbor supports std::optional; fixes #135 (2025-07-16)
[^50]: <https://github.com/munich-data-engineering/reflect-cpp/pull/465#discussion_r2211644334> — #465 Make `rfl::num_fields` public (2025-07-16)
[^51]: <https://github.com/munich-data-engineering/reflect-cpp/pull/465#discussion_r2211645358> — #465 Make `rfl::num_fields` public (2025-07-16)
[^52]: <https://github.com/munich-data-engineering/reflect-cpp/pull/464#issuecomment-3080981922> — #464 Fix bug reading with 'tagged' variants when a variant is non-copyable (2025-07-16)
[^53]: <https://github.com/munich-data-engineering/reflect-cpp/pull/466#issuecomment-3085195208> — #466 Serialize/Deserialize `vector<char>` as bytestring (2025-07-17)
[^54]: <https://github.com/munich-data-engineering/reflect-cpp/pull/466#issuecomment-3092276979> — #466 Serialize/Deserialize `vector<char>` as bytestring (2025-07-19)
[^55]: <https://github.com/munich-data-engineering/reflect-cpp/pull/466#discussion_r2217284535> — #466 Serialize/Deserialize `vector<char>` as bytestring (2025-07-19)
[^56]: <https://github.com/munich-data-engineering/reflect-cpp/pull/466#discussion_r2217284568> — #466 Serialize/Deserialize `vector<char>` as bytestring (2025-07-19)
[^57]: <https://github.com/munich-data-engineering/reflect-cpp/pull/466#issuecomment-3094441075> — #466 Serialize/Deserialize `vector<char>` as bytestring (2025-07-20)
[^58]: <https://github.com/munich-data-engineering/reflect-cpp/pull/468#discussion_r2217800641> — #468 Added support for std::span; resolves #457 (2025-07-20)
[^59]: <https://github.com/munich-data-engineering/reflect-cpp/pull/469#issuecomment-3122117858> — #469 Add support for reading from a byte-container for cbor and ubjson (2025-07-26)
[^60]: <https://github.com/munich-data-engineering/reflect-cpp/issues/470#issuecomment-3127096672> — #470 sqlgen cannot find reflectcpp-config.cmake (2025-07-28)
[^61]: <https://github.com/munich-data-engineering/reflect-cpp/issues/470#issuecomment-3133330464> — #470 sqlgen cannot find reflectcpp-config.cmake (2025-07-29)
[^62]: <https://github.com/munich-data-engineering/reflect-cpp/issues/470#issuecomment-3133892549> — #470 sqlgen cannot find reflectcpp-config.cmake (2025-07-29)
[^63]: <https://github.com/munich-data-engineering/reflect-cpp/issues/470#issuecomment-3136438924> — #470 sqlgen cannot find reflectcpp-config.cmake (2025-07-30)
[^64]: <https://github.com/munich-data-engineering/reflect-cpp/issues/470#issuecomment-3236997483> — #470 sqlgen cannot find reflectcpp-config.cmake (2025-08-29)
[^65]: <https://github.com/munich-data-engineering/reflect-cpp/issues/429#issuecomment-3127475908> — #429 Support CBOR parsing using error codes (2025-07-28)
[^66]: <https://github.com/munich-data-engineering/reflect-cpp/issues/472#issuecomment-3138009653> — #472 Multiple serialization strategies for a custom non-struct type (2025-07-30)
[^67]: <https://github.com/munich-data-engineering/reflect-cpp/issues/472#issuecomment-3138012016> — #472 Multiple serialization strategies for a custom non-struct type (2025-07-30)
[^68]: <https://github.com/munich-data-engineering/reflect-cpp/issues/472#issuecomment-3140921295> — #472 Multiple serialization strategies for a custom non-struct type (2025-07-31)
[^69]: <https://github.com/munich-data-engineering/reflect-cpp/issues/473#issuecomment-3151838490> — #473 Question: Limit scope of reflection (2025-08-04)
[^70]: <https://github.com/munich-data-engineering/reflect-cpp/issues/473#issuecomment-3151839154> — #473 Question: Limit scope of reflection (2025-08-04)
[^71]: <https://github.com/munich-data-engineering/reflect-cpp/issues/475#issuecomment-3163122679> — #475 Using Field in combination with validation (2025-08-07)
[^72]: <https://github.com/munich-data-engineering/reflect-cpp/issues/475#issuecomment-3166028659> — #475 Using Field in combination with validation (2025-08-07)
[^73]: <https://github.com/munich-data-engineering/reflect-cpp/issues/480#issuecomment-3179404227> — #480 Unable to generate build files for the YAML serializer (2025-08-12)
[^74]: <https://github.com/munich-data-engineering/reflect-cpp/issues/480#issuecomment-3217427025> — #480 Unable to generate build files for the YAML serializer (2025-08-23)
[^75]: <https://github.com/munich-data-engineering/reflect-cpp/issues/480#issuecomment-3218416446> — #480 Unable to generate build files for the YAML serializer (2025-08-24)
[^76]: <https://github.com/munich-data-engineering/reflect-cpp/issues/480#issuecomment-3221422000> — #480 Unable to generate build files for the YAML serializer (2025-08-25)
[^77]: <https://github.com/munich-data-engineering/reflect-cpp/issues/480#issuecomment-3476008974> — #480 Unable to generate build files for the YAML serializer (2025-11-01)
[^78]: <https://github.com/munich-data-engineering/reflect-cpp/pull/479#issuecomment-3179410920> — #479 Add interfaces for jwt-cpp compatibility (2025-08-12)
[^79]: <https://github.com/munich-data-engineering/reflect-cpp/issues/482#issuecomment-3190822384> — #482 C++20 Modules? (2025-08-15)
[^80]: <https://github.com/munich-data-engineering/reflect-cpp/issues/482#issuecomment-3288135449> — #482 C++20 Modules? (2025-09-13)
[^81]: <https://github.com/munich-data-engineering/reflect-cpp/issues/481#issuecomment-3191118685> — #481 Externally tagged unions as fields (2025-08-15)
[^82]: <https://github.com/munich-data-engineering/reflect-cpp/issues/481#issuecomment-3191224557> — #481 Externally tagged unions as fields (2025-08-15)
[^83]: <https://github.com/munich-data-engineering/reflect-cpp/pull/484#issuecomment-3204435777> — #484 Better forwarding to improve read performance a little (2025-08-20)
[^84]: <https://github.com/munich-data-engineering/reflect-cpp/pull/484#issuecomment-3204804353> — #484 Better forwarding to improve read performance a little (2025-08-20)
[^85]: <https://github.com/munich-data-engineering/reflect-cpp/pull/484#issuecomment-3218437586> — #484 Better forwarding to improve read performance a little (2025-08-24)
[^86]: <https://github.com/munich-data-engineering/reflect-cpp/pull/484#issuecomment-3243353541> — #484 Better forwarding to improve read performance a little (2025-09-01)
[^87]: <https://github.com/munich-data-engineering/reflect-cpp/pull/485#issuecomment-3227286954> — #485 make read_one_alternative more general (2025-08-27)
[^88]: <https://github.com/munich-data-engineering/reflect-cpp/pull/485#issuecomment-3227290524> — #485 make read_one_alternative more general (2025-08-27)
[^89]: <https://github.com/munich-data-engineering/reflect-cpp/issues/486#issuecomment-3240044041> — #486 rfl::UUIDv4 field fails JSON parse in struct even for valid UUID (2025-08-31)
[^90]: <https://github.com/munich-data-engineering/reflect-cpp/issues/486#issuecomment-3240044638> — #486 rfl::UUIDv4 field fails JSON parse in struct even for valid UUID (2025-08-31)
[^91]: <https://github.com/munich-data-engineering/reflect-cpp/issues/486#issuecomment-3240320183> — #486 rfl::UUIDv4 field fails JSON parse in struct even for valid UUID (2025-08-31)
[^92]: <https://github.com/munich-data-engineering/reflect-cpp/issues/486#issuecomment-3240330602> — #486 rfl::UUIDv4 field fails JSON parse in struct even for valid UUID (2025-08-31)
[^93]: <https://github.com/munich-data-engineering/reflect-cpp/issues/486#issuecomment-3240342083> — #486 rfl::UUIDv4 field fails JSON parse in struct even for valid UUID (2025-08-31)
[^94]: <https://github.com/munich-data-engineering/reflect-cpp/issues/486#issuecomment-3240368920> — #486 rfl::UUIDv4 field fails JSON parse in struct even for valid UUID (2025-08-31)
[^95]: <https://github.com/munich-data-engineering/reflect-cpp/pull/467#issuecomment-3246817469> — #467 Allow using `rfl::AddTagsToVariants` with `rfl::Generic` and same-name-different-namespace structs (2025-09-02)
[^96]: <https://github.com/munich-data-engineering/reflect-cpp/pull/467#issuecomment-3263935641> — #467 Allow using `rfl::AddTagsToVariants` with `rfl::Generic` and same-name-different-namespace structs (2025-09-07)
[^97]: <https://github.com/munich-data-engineering/reflect-cpp/pull/467#issuecomment-3270729365> — #467 Allow using `rfl::AddTagsToVariants` with `rfl::Generic` and same-name-different-namespace structs (2025-09-09)
[^98]: <https://github.com/munich-data-engineering/reflect-cpp/pull/467#issuecomment-3289671074> — #467 Allow using `rfl::AddTagsToVariants` with `rfl::Generic` and same-name-different-namespace structs (2025-09-14)
[^99]: <https://github.com/munich-data-engineering/reflect-cpp/issues/493#issuecomment-3268084277> — #493 Can't add vector into extrafields ? (2025-09-08)
[^100]: <https://github.com/munich-data-engineering/reflect-cpp/issues/495#issuecomment-3275755248> — #495 "May be used uninitialized" issues reported by GCC 15.2.0 (2025-09-10)
[^101]: <https://github.com/munich-data-engineering/reflect-cpp/issues/495#issuecomment-3275780414> — #495 "May be used uninitialized" issues reported by GCC 15.2.0 (2025-09-10)
[^102]: <https://github.com/munich-data-engineering/reflect-cpp/issues/495#issuecomment-3275782867> — #495 "May be used uninitialized" issues reported by GCC 15.2.0 (2025-09-10)
[^103]: <https://github.com/munich-data-engineering/reflect-cpp/issues/495#issuecomment-3275810684> — #495 "May be used uninitialized" issues reported by GCC 15.2.0 (2025-09-10)
[^104]: <https://github.com/munich-data-engineering/reflect-cpp/issues/495#issuecomment-3307534423> — #495 "May be used uninitialized" issues reported by GCC 15.2.0 (2025-09-18)
[^105]: <https://github.com/munich-data-engineering/reflect-cpp/issues/495#issuecomment-3399105360> — #495 "May be used uninitialized" issues reported by GCC 15.2.0 (2025-10-13)
[^106]: <https://github.com/munich-data-engineering/reflect-cpp/pull/494#discussion_r2337360190> — #494 Add `rfl::AddNamespacedTagsToVariants` for `rfl::Generic` and same-name-different-namespace structs (2025-09-10)
[^107]: <https://github.com/munich-data-engineering/reflect-cpp/pull/494#discussion_r2337369193> — #494 Add `rfl::AddNamespacedTagsToVariants` for `rfl::Generic` and same-name-different-namespace structs (2025-09-10)
[^108]: <https://github.com/munich-data-engineering/reflect-cpp/pull/494#issuecomment-3275795858> — #494 Add `rfl::AddNamespacedTagsToVariants` for `rfl::Generic` and same-name-different-namespace structs (2025-09-10)
[^109]: <https://github.com/munich-data-engineering/reflect-cpp/pull/494#issuecomment-3278248332> — #494 Add `rfl::AddNamespacedTagsToVariants` for `rfl::Generic` and same-name-different-namespace structs (2025-09-11)
[^110]: <https://github.com/munich-data-engineering/reflect-cpp/issues/496#issuecomment-3286862274> — #496 the demo runtime exception with ctre (2025-09-12)
[^111]: <https://github.com/munich-data-engineering/reflect-cpp/issues/496#issuecomment-3287674596> — #496 the demo runtime exception with ctre (2025-09-13)
[^112]: <https://github.com/munich-data-engineering/reflect-cpp/pull/497#issuecomment-3287677530> — #497 Update vcpkg (2025-09-13)
[^113]: <https://github.com/munich-data-engineering/reflect-cpp/pull/497#issuecomment-3288125059> — #497 Update vcpkg (2025-09-13)
[^114]: <https://github.com/munich-data-engineering/reflect-cpp/issues/499#issuecomment-3288067861> — #499 how to retrive object from variant and taggedunion? (2025-09-13)
[^115]: <https://github.com/munich-data-engineering/reflect-cpp/pull/502#issuecomment-3291876516> — #502 Re-add pyproject.tml to fix unknown env error (2025-09-15)
[^116]: <https://github.com/munich-data-engineering/reflect-cpp/issues/503#issuecomment-3325223019> — #503 Using `Description` marks optionals as required in JSON Schema (2025-09-23)
[^117]: <https://github.com/munich-data-engineering/reflect-cpp/issues/504#issuecomment-3335479745> — #504 [Proposal] Error.what() rvalue move return (2025-09-25)
[^118]: <https://github.com/munich-data-engineering/reflect-cpp/pull/505#issuecomment-3344975674> — #505 added explicit ref-qualifiers for Error::what() and a additional over… (2025-09-29)
[^119]: <https://github.com/munich-data-engineering/reflect-cpp/pull/507#issuecomment-3350749893> — #507 use target_compile_definitions for REFLECTCPP_USE_STD_EXPECTED instea… (2025-09-30)
[^120]: <https://github.com/munich-data-engineering/reflect-cpp/pull/514#issuecomment-3382820096> — #514 clang 20 warnings (2025-10-08)
[^121]: <https://github.com/munich-data-engineering/reflect-cpp/issues/509#issuecomment-3383346103> — #509 Json Schema with Reflector type (2025-10-08)
[^122]: <https://github.com/munich-data-engineering/reflect-cpp/issues/509#issuecomment-3419782889> — #509 Json Schema with Reflector type (2025-10-19)
[^123]: <https://github.com/munich-data-engineering/reflect-cpp/pull/515#issuecomment-3392016651> — #515 Cleanup includes (2025-10-10)
[^124]: <https://github.com/munich-data-engineering/reflect-cpp/pull/515#discussion_r2421990009> — #515 Cleanup includes (2025-10-10)
[^125]: <https://github.com/munich-data-engineering/reflect-cpp/pull/515#discussion_r2422005380> — #515 Cleanup includes (2025-10-10)
[^126]: <https://github.com/munich-data-engineering/reflect-cpp/pull/515#issuecomment-3392202985> — #515 Cleanup includes (2025-10-10)
[^127]: <https://github.com/munich-data-engineering/reflect-cpp/pull/515#discussion_r2422954698> — #515 Cleanup includes (2025-10-11)
[^128]: <https://github.com/munich-data-engineering/reflect-cpp/pull/515#discussion_r2422957317> — #515 Cleanup includes (2025-10-11)
[^129]: <https://github.com/munich-data-engineering/reflect-cpp/pull/519#issuecomment-3398172262> — #519 Cleanup includes (2025-10-13)
[^130]: <https://github.com/munich-data-engineering/reflect-cpp/pull/521#issuecomment-3398174284> — #521 Allow specify schema in capnproto's stream interface (2025-10-13)
[^131]: <https://github.com/munich-data-engineering/reflect-cpp/issues/516#issuecomment-3398178142> — #516 Silent errors, can terminate application (2025-10-13)
[^132]: <https://github.com/munich-data-engineering/reflect-cpp/issues/516#issuecomment-3446972638> — #516 Silent errors, can terminate application (2025-10-25)
[^133]: <https://github.com/munich-data-engineering/reflect-cpp/issues/522#issuecomment-3402681866> — #522 Avro serialization for floating point and integer types failing (2025-10-14)
[^134]: <https://github.com/munich-data-engineering/reflect-cpp/issues/522#issuecomment-3446588553> — #522 Avro serialization for floating point and integer types failing (2025-10-25)
[^135]: <https://github.com/munich-data-engineering/reflect-cpp/issues/520#issuecomment-3403043338> — #520 `reflect-cpp` does not generate `.lib` file when building as `shared` on Windows (2025-10-14)
[^136]: <https://github.com/munich-data-engineering/reflect-cpp/issues/520#issuecomment-3408446947> — #520 `reflect-cpp` does not generate `.lib` file when building as `shared` on Windows (2025-10-15)
[^137]: <https://github.com/munich-data-engineering/reflect-cpp/issues/520#issuecomment-3408455257> — #520 `reflect-cpp` does not generate `.lib` file when building as `shared` on Windows (2025-10-15)
[^138]: <https://github.com/munich-data-engineering/reflect-cpp/issues/520#issuecomment-3409579191> — #520 `reflect-cpp` does not generate `.lib` file when building as `shared` on Windows (2025-10-16)
[^139]: <https://github.com/munich-data-engineering/reflect-cpp/issues/520#issuecomment-3419716071> — #520 `reflect-cpp` does not generate `.lib` file when building as `shared` on Windows (2025-10-19)
[^140]: <https://github.com/munich-data-engineering/reflect-cpp/pull/523#issuecomment-3407140487> — #523 [json schema] Use non-deprecated $defs; Add $comment option (2025-10-15)
[^141]: <https://github.com/munich-data-engineering/reflect-cpp/pull/523#issuecomment-3407153762> — #523 [json schema] Use non-deprecated $defs; Add $comment option (2025-10-15)
[^142]: <https://github.com/munich-data-engineering/reflect-cpp/issues/524#issuecomment-3413134272> — #524 Please bump vendored yyjson to latest (0.12.0) and fix related tests (2025-10-16)
[^143]: <https://github.com/munich-data-engineering/reflect-cpp/pull/525#issuecomment-3419713150> — #525 Feature/shared exports msvc (2025-10-19)
[^144]: <https://github.com/munich-data-engineering/reflect-cpp/pull/529#issuecomment-3423317950> — #529 [minor] Fix include error of `rfl/Literal.hpp` (2025-10-20)
[^145]: <https://github.com/munich-data-engineering/reflect-cpp/issues/528#issuecomment-3423320147> — #528 Missing Includes (2025-10-20)
[^146]: <https://github.com/munich-data-engineering/reflect-cpp/issues/528#issuecomment-3423603187> — #528 Missing Includes (2025-10-20)
[^147]: <https://github.com/munich-data-engineering/reflect-cpp/issues/530#issuecomment-3445000092> — #530 Issues with deserialization of fixarray using msgpack with reflectcpp 0.21 (2025-10-24)
[^148]: <https://github.com/munich-data-engineering/reflect-cpp/pull/535#discussion_r2466955107> — #535 Use exceptions inside the writers (2025-10-27)
[^149]: <https://github.com/munich-data-engineering/reflect-cpp/issues/534#issuecomment-3453139757> — #534 Add `rfl::Default` (2025-10-27)
[^150]: <https://github.com/munich-data-engineering/reflect-cpp/pull/538#issuecomment-3478243342> — #538 Avoid assuming that "std::string_view" is nullterminated. Generally it is really not. (2025-11-02)
[^151]: <https://github.com/munich-data-engineering/reflect-cpp/pull/538#issuecomment-3478262099> — #538 Avoid assuming that "std::string_view" is nullterminated. Generally it is really not. (2025-11-02)
[^152]: <https://github.com/munich-data-engineering/reflect-cpp/pull/538#issuecomment-3478266146> — #538 Avoid assuming that "std::string_view" is nullterminated. Generally it is really not. (2025-11-02)
[^153]: <https://github.com/munich-data-engineering/reflect-cpp/pull/538#issuecomment-3478313524> — #538 Avoid assuming that "std::string_view" is nullterminated. Generally it is really not. (2025-11-02)
[^154]: <https://github.com/munich-data-engineering/reflect-cpp/pull/538#issuecomment-3478316661> — #538 Avoid assuming that "std::string_view" is nullterminated. Generally it is really not. (2025-11-02)
[^155]: <https://github.com/munich-data-engineering/reflect-cpp/pull/537#issuecomment-3482911915> — #537 Speed up the reflection (2025-11-03)
[^156]: <https://github.com/munich-data-engineering/reflect-cpp/pull/537#issuecomment-3482959077> — #537 Speed up the reflection (2025-11-03)
[^157]: <https://github.com/munich-data-engineering/reflect-cpp/pull/537#issuecomment-3482985628> — #537 Speed up the reflection (2025-11-03)
[^158]: <https://github.com/munich-data-engineering/reflect-cpp/pull/541#issuecomment-3529951291> — #541 Fix missing "hours" when parsing std::chrono::duration (2025-11-13)
[^159]: <https://github.com/munich-data-engineering/reflect-cpp/issues/542#issuecomment-3534948153> — #542 rfl::io::save_string fails silently (2025-11-14)
[^160]: <https://github.com/munich-data-engineering/reflect-cpp/issues/543#issuecomment-3542998305> — #543 Why would CBOR be slow in the benchmark? (2025-11-17)
[^161]: <https://github.com/munich-data-engineering/reflect-cpp/issues/543#issuecomment-3543000639> — #543 Why would CBOR be slow in the benchmark? (2025-11-17)
[^162]: <https://github.com/munich-data-engineering/reflect-cpp/pull/549#discussion_r2554191718> — #549 `rfl::Generic`: Allow `int64_t` -> `double` conversion in `to_double` (2025-11-23)
[^163]: <https://github.com/munich-data-engineering/reflect-cpp/pull/549#issuecomment-3568135376> — #549 `rfl::Generic`: Allow `int64_t` -> `double` conversion in `to_double` (2025-11-23)
[^164]: <https://github.com/munich-data-engineering/reflect-cpp/pull/547#discussion_r2554195911> — #547 unused variables warnings (2025-11-23)
[^165]: <https://github.com/munich-data-engineering/reflect-cpp/pull/547#issuecomment-3568141826> — #547 unused variables warnings (2025-11-23)
[^166]: <https://github.com/munich-data-engineering/reflect-cpp/pull/547#issuecomment-3568142077> — #547 unused variables warnings (2025-11-23)
[^167]: <https://github.com/munich-data-engineering/reflect-cpp/issues/551#issuecomment-3609024723> — #551 msgpack library name error for ubuntu (24.04 and earlier) (2025-12-03)
[^168]: <https://github.com/munich-data-engineering/reflect-cpp/issues/551#issuecomment-3620235083> — #551 msgpack library name error for ubuntu (24.04 and earlier) (2025-12-06)
[^169]: <https://github.com/munich-data-engineering/reflect-cpp/issues/557#issuecomment-3649779867> — #557 Any idea why including reflect.h in MSVC results in internal compiler error? (2025-12-13)
[^170]: <https://github.com/munich-data-engineering/reflect-cpp/issues/557#issuecomment-3649785880> — #557 Any idea why including reflect.h in MSVC results in internal compiler error? (2025-12-13)
[^171]: <https://github.com/munich-data-engineering/reflect-cpp/issues/557#issuecomment-3649793240> — #557 Any idea why including reflect.h in MSVC results in internal compiler error? (2025-12-13)
[^172]: <https://github.com/munich-data-engineering/reflect-cpp/issues/559#issuecomment-3667551103> — #559 Embedded nulls dropped from JSON strings (2025-12-17)
[^173]: <https://github.com/munich-data-engineering/reflect-cpp/issues/559#issuecomment-3674772989> — #559 Embedded nulls dropped from JSON strings (2025-12-19)
[^174]: <https://github.com/munich-data-engineering/reflect-cpp/issues/560#issuecomment-3667553167> — #560 [Compilation Error] Error when including <windows.h> before <rfl/Result.hpp> (2025-12-17)
[^175]: <https://github.com/munich-data-engineering/reflect-cpp/issues/560#issuecomment-3677555682> — #560 [Compilation Error] Error when including <windows.h> before <rfl/Result.hpp> (2025-12-20)
[^176]: <https://github.com/munich-data-engineering/reflect-cpp/issues/561#issuecomment-3678736816> — #561 Support std::map<..., std::less<>> (transparent comparator) in JSON read/write (2025-12-21)
[^177]: <https://github.com/munich-data-engineering/reflect-cpp/issues/561#issuecomment-3814280238> — #561 Support std::map<..., std::less<>> (transparent comparator) in JSON read/write (2026-01-28)
[^178]: <https://github.com/munich-data-engineering/reflect-cpp/pull/558#issuecomment-3683958431> — #558 Improve bad access error for std::expected<T, rfl::Error> (2025-12-22)
[^179]: <https://github.com/munich-data-engineering/reflect-cpp/issues/565#issuecomment-3693449281> — #565 string_to_enum does not support std::string_view (2025-12-26)
[^180]: <https://github.com/munich-data-engineering/reflect-cpp/issues/567#issuecomment-3706983352> — #567 Compatibility between TaggedUnion and NoExtraFields (2026-01-03)
[^181]: <https://github.com/munich-data-engineering/reflect-cpp/issues/567#issuecomment-3734781460> — #567 Compatibility between TaggedUnion and NoExtraFields (2026-01-11)
[^182]: <https://github.com/munich-data-engineering/reflect-cpp/pull/568#issuecomment-3708452488> — #568 fix: CMake package and imported library target names (2026-01-04)
[^183]: <https://github.com/munich-data-engineering/reflect-cpp/pull/568#issuecomment-3722737275> — #568 fix: CMake package and imported library target names (2026-01-08)
[^184]: <https://github.com/munich-data-engineering/reflect-cpp/pull/568#issuecomment-3938729923> — #568 fix: CMake package and imported library target names (2026-02-21)
[^185]: <https://github.com/munich-data-engineering/reflect-cpp/pull/568#issuecomment-3940894969> — #568 fix: CMake package and imported library target names (2026-02-22)
[^186]: <https://github.com/munich-data-engineering/reflect-cpp/pull/568#issuecomment-3940896026> — #568 fix: CMake package and imported library target names (2026-02-22)
[^187]: <https://github.com/munich-data-engineering/reflect-cpp/pull/569#issuecomment-3728921293> — #569 Fix missing space in static_assert (2026-01-09)
[^188]: <https://github.com/munich-data-engineering/reflect-cpp/pull/571#discussion_r2677525473> — #571 Hotfix -DREFLECTCPP_USE_STD_EXPECTED=ON (2026-01-09)
[^189]: <https://github.com/munich-data-engineering/reflect-cpp/pull/571#issuecomment-3734492165> — #571 Hotfix -DREFLECTCPP_USE_STD_EXPECTED=ON (2026-01-11)
[^190]: <https://github.com/munich-data-engineering/reflect-cpp/issues/570#issuecomment-3733583722> — #570 Compilation Error with REFLECTCPP_USE_STD_EXPECTED=ON (2026-01-10)
[^191]: <https://github.com/munich-data-engineering/reflect-cpp/issues/570#issuecomment-3734495690> — #570 Compilation Error with REFLECTCPP_USE_STD_EXPECTED=ON (2026-01-11)
[^192]: <https://github.com/munich-data-engineering/reflect-cpp/pull/572#discussion_r2679508834> — #572 Make sure that GCC 13 also compilies with std::expected; resolves #570 (2026-01-11)
[^193]: <https://github.com/munich-data-engineering/reflect-cpp/pull/574#issuecomment-3749981125> — #574 Enable building as a native ROS package (2026-01-14)
[^194]: <https://github.com/munich-data-engineering/reflect-cpp/issues/575#issuecomment-3763025843> — #575 Support std::atomic<T> fields (2026-01-17)
[^195]: <https://github.com/munich-data-engineering/reflect-cpp/issues/575#issuecomment-3764349470> — #575 Support std::atomic<T> fields (2026-01-17)
[^196]: <https://github.com/munich-data-engineering/reflect-cpp/issues/575#issuecomment-3764349731> — #575 Support std::atomic<T> fields (2026-01-17)
[^197]: <https://github.com/munich-data-engineering/reflect-cpp/issues/575#issuecomment-3765565871> — #575 Support std::atomic<T> fields (2026-01-18)
[^198]: <https://github.com/munich-data-engineering/reflect-cpp/issues/575#issuecomment-3765702560> — #575 Support std::atomic<T> fields (2026-01-18)
[^199]: <https://github.com/munich-data-engineering/reflect-cpp/issues/575#issuecomment-3765833451> — #575 Support std::atomic<T> fields (2026-01-18)
[^200]: <https://github.com/munich-data-engineering/reflect-cpp/issues/576#issuecomment-3765555696> — #576 Provide CMake components for different serialization formats (2026-01-18)
[^201]: <https://github.com/munich-data-engineering/reflect-cpp/issues/576#issuecomment-3765924908> — #576 Provide CMake components for different serialization formats (2026-01-19)
[^202]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3769424687> — #578 Add support for C++ modules (2026-01-19)
[^203]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3771909519> — #578 Add support for C++ modules (2026-01-20)
[^204]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3773917724> — #578 Add support for C++ modules (2026-01-20)
[^205]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3773919521> — #578 Add support for C++ modules (2026-01-20)
[^206]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3774694481> — #578 Add support for C++ modules (2026-01-20)
[^207]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3774941365> — #578 Add support for C++ modules (2026-01-20)
[^208]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3774988543> — #578 Add support for C++ modules (2026-01-20)
[^209]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3775155323> — #578 Add support for C++ modules (2026-01-20)
[^210]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3777497499> — #578 Add support for C++ modules (2026-01-21)
[^211]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3780867900> — #578 Add support for C++ modules (2026-01-21)
[^212]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3781267857> — #578 Add support for C++ modules (2026-01-21)
[^213]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3781447408> — #578 Add support for C++ modules (2026-01-21)
[^214]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3787096725> — #578 Add support for C++ modules (2026-01-22)
[^215]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3787245067> — #578 Add support for C++ modules (2026-01-22)
[^216]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3788801376> — #578 Add support for C++ modules (2026-01-23)
[^217]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3789987900> — #578 Add support for C++ modules (2026-01-23)
[^218]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3792497605> — #578 Add support for C++ modules (2026-01-23)
[^219]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3792548217> — #578 Add support for C++ modules (2026-01-23)
[^220]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3792578022> — #578 Add support for C++ modules (2026-01-23)
[^221]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3794560211> — #578 Add support for C++ modules (2026-01-24)
[^222]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3795078321> — #578 Add support for C++ modules (2026-01-24)
[^223]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3796417443> — #578 Add support for C++ modules (2026-01-25)
[^224]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-3801645060> — #578 Add support for C++ modules (2026-01-26)
[^225]: <https://github.com/munich-data-engineering/reflect-cpp/pull/578#issuecomment-5517030300> — #578 Add support for C++ modules (2026-09-02)
[^226]: <https://github.com/munich-data-engineering/reflect-cpp/pull/580#issuecomment-3794576523> — #580 build(cmake): add and install cmake version file (2026-01-24)
[^227]: <https://github.com/munich-data-engineering/reflect-cpp/pull/581#issuecomment-3796985261> — #581 Make REFLECTCPP_USE_BUNDLED_DEPENDENCIES option consistent with enchantum (2026-01-25)
[^228]: <https://github.com/munich-data-engineering/reflect-cpp/pull/581#issuecomment-3796986846> — #581 Make REFLECTCPP_USE_BUNDLED_DEPENDENCIES option consistent with enchantum (2026-01-25)
[^229]: <https://github.com/munich-data-engineering/reflect-cpp/pull/581#issuecomment-3799839213> — #581 Make REFLECTCPP_USE_BUNDLED_DEPENDENCIES option consistent with enchantum (2026-01-26)
[^230]: <https://github.com/munich-data-engineering/reflect-cpp/pull/581#issuecomment-3799843118> — #581 Make REFLECTCPP_USE_BUNDLED_DEPENDENCIES option consistent with enchantum (2026-01-26)
[^231]: <https://github.com/munich-data-engineering/reflect-cpp/pull/581#issuecomment-3938731911> — #581 Make REFLECTCPP_USE_BUNDLED_DEPENDENCIES option consistent with enchantum (2026-02-21)
[^232]: <https://github.com/munich-data-engineering/reflect-cpp/pull/582#issuecomment-3799727965> — #582 Fix some unused parameters warnings (2026-01-26)
[^233]: <https://github.com/munich-data-engineering/reflect-cpp/pull/582#issuecomment-3801631579> — #582 Fix some unused parameters warnings (2026-01-26)
[^234]: <https://github.com/munich-data-engineering/reflect-cpp/pull/582#issuecomment-3807716865> — #582 Fix some unused parameters warnings (2026-01-27)
[^235]: <https://github.com/munich-data-engineering/reflect-cpp/pull/582#issuecomment-3808118426> — #582 Fix some unused parameters warnings (2026-01-27)
[^236]: <https://github.com/munich-data-engineering/reflect-cpp/pull/584#issuecomment-3801626810> — #584 Support parsing arbitrary duration representations (2026-01-26)
[^237]: <https://github.com/munich-data-engineering/reflect-cpp/pull/584#issuecomment-3801655087> — #584 Support parsing arbitrary duration representations (2026-01-26)
[^238]: <https://github.com/munich-data-engineering/reflect-cpp/pull/584#issuecomment-3807344075> — #584 Support parsing arbitrary duration representations (2026-01-27)
[^239]: <https://github.com/munich-data-engineering/reflect-cpp/issues/587#issuecomment-3807339031> — #587 Fix `rfl::DefaultVal` with `rfl::Rename` (2026-01-27)
[^240]: <https://github.com/munich-data-engineering/reflect-cpp/issues/587#issuecomment-3807839727> — #587 Fix `rfl::DefaultVal` with `rfl::Rename` (2026-01-27)
[^241]: <https://github.com/munich-data-engineering/reflect-cpp/pull/588#discussion_r2733957156> — #588 Add camel case to snake case conversion (2026-01-27)
[^242]: <https://github.com/munich-data-engineering/reflect-cpp/pull/588#pullrequestreview-3713619313> — #588 Add camel case to snake case conversion (2026-01-27)
[^243]: <https://github.com/munich-data-engineering/reflect-cpp/pull/588#issuecomment-3807710716> — #588 Add camel case to snake case conversion (2026-01-27)
[^244]: <https://github.com/munich-data-engineering/reflect-cpp/pull/588#issuecomment-3814491068> — #588 Add camel case to snake case conversion (2026-01-28)
[^245]: <https://github.com/munich-data-engineering/reflect-cpp/issues/564#issuecomment-3814143842> — #564 MSVC Toml std::array not working (2026-01-28)
[^246]: <https://github.com/munich-data-engineering/reflect-cpp/issues/591#issuecomment-3818043658> — #591 Custom class `ReflectionType` cannot be directly in the class on Linux (2026-01-29)
[^247]: <https://github.com/munich-data-engineering/reflect-cpp/issues/592#issuecomment-3819713799> — #592 Issue with parsing string validation and default fields in the same nested struct (2026-01-29)
[^248]: <https://github.com/munich-data-engineering/reflect-cpp/issues/592#issuecomment-3820550697> — #592 Issue with parsing string validation and default fields in the same nested struct (2026-01-29)
[^249]: <https://github.com/munich-data-engineering/reflect-cpp/issues/552#issuecomment-3820646696> — #552 Better support for custom schemas (2026-01-29)
[^250]: <https://github.com/munich-data-engineering/reflect-cpp/issues/552#issuecomment-3822490752> — #552 Better support for custom schemas (2026-01-30)
[^251]: <https://github.com/munich-data-engineering/reflect-cpp/issues/552#issuecomment-3822493608> — #552 Better support for custom schemas (2026-01-30)
[^252]: <https://github.com/munich-data-engineering/reflect-cpp/issues/594#issuecomment-3836193722> — #594 Add benchmark comparisons to other solutions in the README.md? (2026-02-02)
[^253]: <https://github.com/munich-data-engineering/reflect-cpp/issues/594#issuecomment-3837483420> — #594 Add benchmark comparisons to other solutions in the README.md? (2026-02-02)
[^254]: <https://github.com/munich-data-engineering/reflect-cpp/issues/595#issuecomment-3843365112> — #595 Inconsistency in rfl::Variant visit overloads prevents stateful visitors (2026-02-03)
[^255]: <https://github.com/munich-data-engineering/reflect-cpp/pull/596#issuecomment-3844768170> — #596 Allow usage of stateful visitors inside rfl::Variant (2026-02-04)
[^256]: <https://github.com/munich-data-engineering/reflect-cpp/issues/597#issuecomment-3850464965> — #597 Feature Request: Add support for serializing comments (2026-02-05)
[^257]: <https://github.com/munich-data-engineering/reflect-cpp/issues/597#issuecomment-3865833423> — #597 Feature Request: Add support for serializing comments (2026-02-08)
[^258]: <https://github.com/munich-data-engineering/reflect-cpp/issues/597#issuecomment-3865848094> — #597 Feature Request: Add support for serializing comments (2026-02-08)
[^259]: <https://github.com/munich-data-engineering/reflect-cpp/issues/597#issuecomment-3865898797> — #597 Feature Request: Add support for serializing comments (2026-02-08)
[^260]: <https://github.com/munich-data-engineering/reflect-cpp/issues/598#issuecomment-3856583675> — #598 Update error message for unused values (2026-02-05)
[^261]: <https://github.com/munich-data-engineering/reflect-cpp/issues/598#issuecomment-3856585043> — #598 Update error message for unused values (2026-02-05)
[^262]: <https://github.com/munich-data-engineering/reflect-cpp/pull/599#issuecomment-3864903718> — #599 Remove reference to internals in error message for unused values (2026-02-07)
[^263]: <https://github.com/munich-data-engineering/reflect-cpp/pull/607#issuecomment-3875211396> — #607 Set file name and license in RPM CPack options (2026-02-10)
[^264]: <https://github.com/munich-data-engineering/reflect-cpp/issues/608#issuecomment-3883029996> — #608 Why not give a default constructor to TaggedUnion? (2026-02-11)
[^265]: <https://github.com/munich-data-engineering/reflect-cpp/pull/606#issuecomment-3885185694> — #606 Improve invalid enum error message (2026-02-11)
[^266]: <https://github.com/munich-data-engineering/reflect-cpp/pull/610#issuecomment-3885253719> — #610 Json Schema: Add enum value descriptions (2026-02-11)
[^267]: <https://github.com/munich-data-engineering/reflect-cpp/pull/610#issuecomment-3938732656> — #610 Json Schema: Add enum value descriptions (2026-02-21)
[^268]: <https://github.com/munich-data-engineering/reflect-cpp/issues/609#issuecomment-3888491256> — #609 Bug or possible regression with custom rfl::Reflectors (2026-02-12)
[^269]: <https://github.com/munich-data-engineering/reflect-cpp/issues/609#issuecomment-3938631576> — #609 Bug or possible regression with custom rfl::Reflectors (2026-02-21)
[^270]: <https://github.com/munich-data-engineering/reflect-cpp/pull/613#discussion_r2806700306> — #613 Add CLI argument parser (rfl::cli::read) (2026-02-14)
[^271]: <https://github.com/munich-data-engineering/reflect-cpp/pull/613#issuecomment-3900326453> — #613 Add CLI argument parser (rfl::cli::read) (2026-02-14)
[^272]: <https://github.com/munich-data-engineering/reflect-cpp/pull/613#discussion_r2815036048> — #613 Add CLI argument parser (rfl::cli::read) (2026-02-17)
[^273]: <https://github.com/munich-data-engineering/reflect-cpp/pull/613#discussion_r2815046702> — #613 Add CLI argument parser (rfl::cli::read) (2026-02-17)
[^274]: <https://github.com/munich-data-engineering/reflect-cpp/pull/613#discussion_r2815047219> — #613 Add CLI argument parser (rfl::cli::read) (2026-02-17)
[^275]: <https://github.com/munich-data-engineering/reflect-cpp/pull/613#issuecomment-3912367693> — #613 Add CLI argument parser (rfl::cli::read) (2026-02-17)
[^276]: <https://github.com/munich-data-engineering/reflect-cpp/pull/613#pullrequestreview-3835490850> — #613 Add CLI argument parser (rfl::cli::read) (2026-02-21)
[^277]: <https://github.com/munich-data-engineering/reflect-cpp/pull/613#issuecomment-3938959201> — #613 Add CLI argument parser (rfl::cli::read) (2026-02-21)
[^278]: <https://github.com/munich-data-engineering/reflect-cpp/issues/614#issuecomment-3905866277> — #614 Can't parse XML into a rfl::Generic (2026-02-16)
[^279]: <https://github.com/munich-data-engineering/reflect-cpp/issues/614#issuecomment-4016844417> — #614 Can't parse XML into a rfl::Generic (2026-03-07)
[^280]: <https://github.com/munich-data-engineering/reflect-cpp/issues/615#issuecomment-3927827078> — #615 std::atomic<T> Reflector bypasses domain validation present on bare T (#575 followup) (2026-02-19)
[^281]: <https://github.com/munich-data-engineering/reflect-cpp/issues/615#issuecomment-3938648935> — #615 std::atomic<T> Reflector bypasses domain validation present on bare T (#575 followup) (2026-02-21)
[^282]: <https://github.com/munich-data-engineering/reflect-cpp/issues/615#issuecomment-3938667446> — #615 std::atomic<T> Reflector bypasses domain validation present on bare T (#575 followup) (2026-02-21)
[^283]: <https://github.com/munich-data-engineering/reflect-cpp/issues/615#issuecomment-3939082947> — #615 std::atomic<T> Reflector bypasses domain validation present on bare T (#575 followup) (2026-02-21)
[^284]: <https://github.com/munich-data-engineering/reflect-cpp/issues/616#issuecomment-3931487081> — #616 reflect-cpp in Fedora/EPEL (2026-02-20)
[^285]: <https://github.com/munich-data-engineering/reflect-cpp/issues/619#issuecomment-3942309638> — #619 Question: When using rfl::Variant with the msgpack processor, is AddTagsToVariants nessasary? (2026-02-23)
[^286]: <https://github.com/munich-data-engineering/reflect-cpp/pull/621#issuecomment-3948800483> — #621 Allows usage of custom Reflector for enums types (2026-02-24)
[^287]: <https://github.com/munich-data-engineering/reflect-cpp/pull/621#issuecomment-3956369500> — #621 Allows usage of custom Reflector for enums types (2026-02-25)
[^288]: <https://github.com/munich-data-engineering/reflect-cpp/pull/618#issuecomment-3949057935> — #618 Fix 30 bugs found during code review (2026-02-24)
[^289]: <https://github.com/munich-data-engineering/reflect-cpp/pull/618#issuecomment-3956502226> — #618 Fix 30 bugs found during code review (2026-02-25)
[^290]: <https://github.com/munich-data-engineering/reflect-cpp/pull/618#issuecomment-3995463198> — #618 Fix 30 bugs found during code review (2026-03-04)
[^291]: <https://github.com/munich-data-engineering/reflect-cpp/pull/618#issuecomment-4005878901> — #618 Fix 30 bugs found during code review (2026-03-05)
[^292]: <https://github.com/munich-data-engineering/reflect-cpp/pull/618#issuecomment-4016488494> — #618 Fix 30 bugs found during code review (2026-03-07)
[^293]: <https://github.com/munich-data-engineering/reflect-cpp/pull/618#issuecomment-4032408676> — #618 Fix 30 bugs found during code review (2026-03-10)
[^294]: <https://github.com/munich-data-engineering/reflect-cpp/pull/618#issuecomment-4035442758> — #618 Fix 30 bugs found during code review (2026-03-11)
[^295]: <https://github.com/munich-data-engineering/reflect-cpp/issues/622#issuecomment-3959623057> — #622 vcpkg build fail (2026-02-25)
[^296]: <https://github.com/munich-data-engineering/reflect-cpp/issues/622#issuecomment-3976692535> — #622 vcpkg build fail (2026-02-28)
[^297]: <https://github.com/munich-data-engineering/reflect-cpp/pull/624#pullrequestreview-3864365435> — #624 Add rfl::Deprecated, which adds deprecated and deprecationMessage to the generated schema field. (2026-02-27)
[^298]: <https://github.com/munich-data-engineering/reflect-cpp/pull/624#issuecomment-3970177511> — #624 Add rfl::Deprecated, which adds deprecated and deprecationMessage to the generated schema field. (2026-02-27)
[^299]: <https://github.com/munich-data-engineering/reflect-cpp/issues/623#issuecomment-3976650777> — #623 Potential improvements to rfl::cli::read() API (2026-02-28)
[^300]: <https://github.com/munich-data-engineering/reflect-cpp/pull/620#issuecomment-3985284149> — #620 Add CMake option for toggling strict warnings (2026-03-02)
[^301]: <https://github.com/munich-data-engineering/reflect-cpp/pull/620#issuecomment-4066871343> — #620 Add CMake option for toggling strict warnings (2026-03-16)
[^302]: <https://github.com/munich-data-engineering/reflect-cpp/pull/626#issuecomment-4005858995> — #626 json schema: Add enum value descriptions (2026-03-05)
[^303]: <https://github.com/munich-data-engineering/reflect-cpp/pull/626#issuecomment-4068885980> — #626 json schema: Add enum value descriptions (2026-03-16)
[^304]: <https://github.com/munich-data-engineering/reflect-cpp/pull/625#issuecomment-4005864300> — #625 Docs: update with rfl::Deprecated (2026-03-05)
[^305]: <https://github.com/munich-data-engineering/reflect-cpp/pull/627#issuecomment-4016778506> — #627 [msgpack] Fixes missing parameter comments, error message (2026-03-07)
[^306]: <https://github.com/munich-data-engineering/reflect-cpp/issues/629#issuecomment-4019266202> — #629 "May be used uninitialized" warning from g++ 15.2.1 with -O2 (2026-03-08)
[^307]: <https://github.com/munich-data-engineering/reflect-cpp/issues/629#issuecomment-4019268300> — #629 "May be used uninitialized" warning from g++ 15.2.1 with -O2 (2026-03-08)
[^308]: <https://github.com/munich-data-engineering/reflect-cpp/pull/632#issuecomment-4032435677> — #632 Fix "Error()" function. (2026-03-10)
[^309]: <https://github.com/munich-data-engineering/reflect-cpp/pull/632#issuecomment-4051633653> — #632 Fix "Error()" function. (2026-03-13)
[^310]: <https://github.com/munich-data-engineering/reflect-cpp/issues/631#issuecomment-4043273382> — #631 Custom validators (2026-03-12)
[^311]: <https://github.com/munich-data-engineering/reflect-cpp/issues/637#issuecomment-4115549228> — #637 C++26 reflection adoption (2026-03-24)
[^312]: <https://github.com/munich-data-engineering/reflect-cpp/issues/637#issuecomment-5416711147> — #637 C++26 reflection adoption (2026-08-25)
[^313]: <https://github.com/munich-data-engineering/reflect-cpp/pull/634#issuecomment-4115562055> — #634 Fix capnproto add_value_to_map overwriting all entries at index 0 (2026-03-24)
[^314]: <https://github.com/munich-data-engineering/reflect-cpp/pull/634#issuecomment-4115754204> — #634 Fix capnproto add_value_to_map overwriting all entries at index 0 (2026-03-24)
[^315]: <https://github.com/munich-data-engineering/reflect-cpp/pull/635#issuecomment-4115758358> — #635 Fix Cap'n Proto serialization of unsigned integer types (2026-03-24)
[^316]: <https://github.com/munich-data-engineering/reflect-cpp/issues/636#issuecomment-4115767904> — #636 Boost serialisation support (2026-03-24)
[^317]: <https://github.com/munich-data-engineering/reflect-cpp/pull/638#issuecomment-4119469028> — #638 Delete MR_DESCRIPTION.md (2026-03-24)
[^318]: <https://github.com/munich-data-engineering/reflect-cpp/issues/639#issuecomment-4132633419> — #639 How to work with reference with a static lifetime (2026-03-26)
[^319]: <https://github.com/munich-data-engineering/reflect-cpp/pull/641#issuecomment-4147686135> — #641 Boost serialization implementation (2026-03-28)
[^320]: <https://github.com/munich-data-engineering/reflect-cpp/pull/641#issuecomment-4147691446> — #641 Boost serialization implementation (2026-03-28)
[^321]: <https://github.com/munich-data-engineering/reflect-cpp/pull/641#issuecomment-4148265384> — #641 Boost serialization implementation (2026-03-28)
[^322]: <https://github.com/munich-data-engineering/reflect-cpp/pull/641#issuecomment-4183371682> — #641 Boost serialization implementation (2026-04-03)
[^323]: <https://github.com/munich-data-engineering/reflect-cpp/pull/641#issuecomment-4183378978> — #641 Boost serialization implementation (2026-04-03)
[^324]: <https://github.com/munich-data-engineering/reflect-cpp/pull/641#issuecomment-4183729963> — #641 Boost serialization implementation (2026-04-03)
[^325]: <https://github.com/munich-data-engineering/reflect-cpp/pull/641#issuecomment-4183770423> — #641 Boost serialization implementation (2026-04-03)
[^326]: <https://github.com/munich-data-engineering/reflect-cpp/issues/640#issuecomment-4152420827> — #640 Problem (de-)serializing an enum with value 0 if the | operator is overloaded (2026-03-30)
[^327]: <https://github.com/munich-data-engineering/reflect-cpp/issues/640#issuecomment-4152471047> — #640 Problem (de-)serializing an enum with value 0 if the | operator is overloaded (2026-03-30)
[^328]: <https://github.com/munich-data-engineering/reflect-cpp/issues/640#issuecomment-4162422276> — #640 Problem (de-)serializing an enum with value 0 if the | operator is overloaded (2026-03-31)
[^329]: <https://github.com/munich-data-engineering/reflect-cpp/issues/640#issuecomment-4183313845> — #640 Problem (de-)serializing an enum with value 0 if the | operator is overloaded (2026-04-03)
[^330]: <https://github.com/munich-data-engineering/reflect-cpp/issues/642#issuecomment-4152426247> — #642 Attempting to serialize an rfl::Commented<std::optional<std::array<>>> fails when the value is null (2026-03-30)
[^331]: <https://github.com/munich-data-engineering/reflect-cpp/issues/642#issuecomment-4183687458> — #642 Attempting to serialize an rfl::Commented<std::optional<std::array<>>> fails when the value is null (2026-04-03)
[^332]: <https://github.com/munich-data-engineering/reflect-cpp/pull/644#discussion_r3032721532> — #644 Make sure that flag enums with value zero are correctly serialized; fixes #640 (2026-04-03)
[^333]: <https://github.com/munich-data-engineering/reflect-cpp/pull/645#discussion_r3033074620> — #645 Make sure that optional inside commented works as expected; fixes #642 (2026-04-03)
[^334]: <https://github.com/munich-data-engineering/reflect-cpp/pull/646#issuecomment-4184197435> — #646 boost src/ offloading (2026-04-03)
[^335]: <https://github.com/munich-data-engineering/reflect-cpp/issues/586#issuecomment-4186898328> — #586 Add serialization for std::chrono::system_clock::time_point (2026-04-04)
[^336]: <https://github.com/munich-data-engineering/reflect-cpp/pull/648#issuecomment-4188871164> — #648 Parser for chrono::time_point (2026-04-05)
[^337]: <https://github.com/munich-data-engineering/reflect-cpp/issues/651#issuecomment-4245833548> — #651 Option to disable toml indentation (2026-04-14)
[^338]: <https://github.com/munich-data-engineering/reflect-cpp/issues/653#issuecomment-4256627680> — #653 [bug] rfl::Processors seems broken (2026-04-16)
[^339]: <https://github.com/munich-data-engineering/reflect-cpp/issues/653#issuecomment-4258120209> — #653 [bug] rfl::Processors seems broken (2026-04-16)
[^340]: <https://github.com/munich-data-engineering/reflect-cpp/issues/652#issuecomment-4256631962> — #652 Why not allowing to parse null as std::optional in yaml files via `is_empty(null) -> true` (2026-04-16)
[^341]: <https://github.com/munich-data-engineering/reflect-cpp/pull/655#discussion_r3143515383> — #655 Redesign the way processors are identified; fixes #653 (2026-04-26)
[^342]: <https://github.com/munich-data-engineering/reflect-cpp/pull/654#discussion_r3143529104> — #654 Feat/parse null into nullopt optionals (2026-04-26)
[^343]: <https://github.com/munich-data-engineering/reflect-cpp/pull/654#issuecomment-4322109791> — #654 Feat/parse null into nullopt optionals (2026-04-26)
[^344]: <https://github.com/munich-data-engineering/reflect-cpp/pull/654#issuecomment-4330340815> — #654 Feat/parse null into nullopt optionals (2026-04-27)
[^345]: <https://github.com/munich-data-engineering/reflect-cpp/pull/654#issuecomment-4330355838> — #654 Feat/parse null into nullopt optionals (2026-04-27)
[^346]: <https://github.com/munich-data-engineering/reflect-cpp/pull/657#issuecomment-4365736949> — #657 User controllable multiline string serialization for YAML (2026-05-03)
[^347]: <https://github.com/munich-data-engineering/reflect-cpp/issues/658#issuecomment-4391295616> — #658 clang-tidy & MSVC: Duplicate field names are not allowed (2026-05-06)
[^348]: <https://github.com/munich-data-engineering/reflect-cpp/pull/659#pullrequestreview-4259179735> — #659 Fix MkDocs deployment on public mkdocs-material (2026-05-10)
[^349]: <https://github.com/munich-data-engineering/reflect-cpp/pull/662#issuecomment-4415121180> — #662 Add unified rfl::Settings::with<>() API for format-specific settings (2026-05-10)
[^350]: <https://github.com/munich-data-engineering/reflect-cpp/pull/662#issuecomment-4415661959> — #662 Add unified rfl::Settings::with<>() API for format-specific settings (2026-05-10)
[^351]: <https://github.com/munich-data-engineering/reflect-cpp/pull/662#issuecomment-4416085351> — #662 Add unified rfl::Settings::with<>() API for format-specific settings (2026-05-10)
[^352]: <https://github.com/munich-data-engineering/reflect-cpp/issues/661#issuecomment-4415424236> — #661 Deserialising std::map Enum Keys (2026-05-10)
[^353]: <https://github.com/munich-data-engineering/reflect-cpp/pull/664#discussion_r3231982749> — #664 Adapt Conan to support the new formats (2026-05-13)
[^354]: <https://github.com/munich-data-engineering/reflect-cpp/pull/667#discussion_r3242232017> — #667 Version 0.25.0 (2026-05-14)
[^355]: <https://github.com/munich-data-engineering/reflect-cpp/issues/665#issuecomment-4453337082> — #665 rfl::to_generic() generates MSVC warning C4702 (unreachable code) (2026-05-14)
[^356]: <https://github.com/munich-data-engineering/reflect-cpp/pull/668#issuecomment-4454616857> — #668 fix(json): don't crash on non-ASCII std::wstring / std::filesystem::path (2026-05-14)
[^357]: <https://github.com/munich-data-engineering/reflect-cpp/pull/668#issuecomment-4455165657> — #668 fix(json): don't crash on non-ASCII std::wstring / std::filesystem::path (2026-05-14)
[^358]: <https://github.com/munich-data-engineering/reflect-cpp/pull/668#issuecomment-4461108502> — #668 fix(json): don't crash on non-ASCII std::wstring / std::filesystem::path (2026-05-15)
[^359]: <https://github.com/munich-data-engineering/reflect-cpp/pull/668#issuecomment-4461247523> — #668 fix(json): don't crash on non-ASCII std::wstring / std::filesystem::path (2026-05-15)
[^360]: <https://github.com/munich-data-engineering/reflect-cpp/pull/669#issuecomment-4485460998> — #669 Add C++20 modules support and format btw. (2026-05-19)
[^361]: <https://github.com/munich-data-engineering/reflect-cpp/pull/669#discussion_r3264491128> — #669 Add C++20 modules support and format btw. (2026-05-19)
[^362]: <https://github.com/munich-data-engineering/reflect-cpp/pull/669#issuecomment-4487775677> — #669 Add C++20 modules support and format btw. (2026-05-19)
[^363]: <https://github.com/munich-data-engineering/reflect-cpp/pull/669#issuecomment-4659033995> — #669 Add C++20 modules support and format btw. (2026-06-09)
[^364]: <https://github.com/munich-data-engineering/reflect-cpp/pull/669#issuecomment-4668810543> — #669 Add C++20 modules support and format btw. (2026-06-10)
[^365]: <https://github.com/munich-data-engineering/reflect-cpp/pull/669#issuecomment-4991855580> — #669 Add C++20 modules support and format btw. (2026-07-16)
[^366]: <https://github.com/munich-data-engineering/reflect-cpp/pull/669#issuecomment-5017695229> — #669 Add C++20 modules support and format btw. (2026-07-19)
[^367]: <https://github.com/munich-data-engineering/reflect-cpp/pull/671#issuecomment-4549704763> — #671 Fix struct Person definition in json_schema.md (2026-05-26)
[^368]: <https://github.com/munich-data-engineering/reflect-cpp/pull/672#issuecomment-4588188766> — #672 Generic::to_*() enhancements (2026-05-31)
[^369]: <https://github.com/munich-data-engineering/reflect-cpp/issues/673#issuecomment-4609763700> — #673 Exception thrown in noexcept code (2026-06-03)
[^370]: <https://github.com/munich-data-engineering/reflect-cpp/issues/673#issuecomment-4609773810> — #673 Exception thrown in noexcept code (2026-06-03)
[^371]: <https://github.com/munich-data-engineering/reflect-cpp/issues/673#issuecomment-4783216690> — #673 Exception thrown in noexcept code (2026-06-23)
[^372]: <https://github.com/munich-data-engineering/reflect-cpp/pull/674#discussion_r3356042928> — #674 fix abort when default value does not satisfy Validator (2026-06-04)
[^373]: <https://github.com/munich-data-engineering/reflect-cpp/pull/674#discussion_r3358663932> — #674 fix abort when default value does not satisfy Validator (2026-06-04)
[^374]: <https://github.com/munich-data-engineering/reflect-cpp/pull/674#issuecomment-4639782500> — #674 fix abort when default value does not satisfy Validator (2026-06-06)
[^375]: <https://github.com/munich-data-engineering/reflect-cpp/pull/674#discussion_r3409480624> — #674 fix abort when default value does not satisfy Validator (2026-06-14)
[^376]: <https://github.com/munich-data-engineering/reflect-cpp/pull/674#issuecomment-4783213934> — #674 fix abort when default value does not satisfy Validator (2026-06-23)
[^377]: <https://github.com/munich-data-engineering/reflect-cpp/issues/677#issuecomment-4644020888> — #677 `rfl::json::to_schema<T, rfl::AddTagsToVariants>` fails to compile for variants of plain structs (2026-06-07)
[^378]: <https://github.com/munich-data-engineering/reflect-cpp/issues/679#issuecomment-4651035826> — #679 rfl::DefaultVal's are still required in the json schema (2026-06-08)
[^379]: <https://github.com/munich-data-engineering/reflect-cpp/pull/680#issuecomment-4653351883> — #680 WIP: respects DefaultVal in schema and adds default value to schema. fixes #679, #396 (2026-06-08)
[^380]: <https://github.com/munich-data-engineering/reflect-cpp/pull/680#issuecomment-4653479945> — #680 WIP: respects DefaultVal in schema and adds default value to schema. fixes #679, #396 (2026-06-08)
[^381]: <https://github.com/munich-data-engineering/reflect-cpp/pull/680#issuecomment-4663840467> — #680 WIP: respects DefaultVal in schema and adds default value to schema. fixes #679, #396 (2026-06-09)
[^382]: <https://github.com/munich-data-engineering/reflect-cpp/pull/680#issuecomment-4664759239> — #680 WIP: respects DefaultVal in schema and adds default value to schema. fixes #679, #396 (2026-06-09)
[^383]: <https://github.com/munich-data-engineering/reflect-cpp/pull/680#issuecomment-4674115807> — #680 WIP: respects DefaultVal in schema and adds default value to schema. fixes #679, #396 (2026-06-10)
[^384]: <https://github.com/munich-data-engineering/reflect-cpp/pull/680#issuecomment-4692394505> — #680 WIP: respects DefaultVal in schema and adds default value to schema. fixes #679, #396 (2026-06-12)
[^385]: <https://github.com/munich-data-engineering/reflect-cpp/pull/680#issuecomment-4711655494> — #680 WIP: respects DefaultVal in schema and adds default value to schema. fixes #679, #396 (2026-06-15)
[^386]: <https://github.com/munich-data-engineering/reflect-cpp/issues/684#issuecomment-4763193090> — #684 Request: Implement 'member-of-pointer' access operator (->) for Rfl types (2026-06-21)
[^387]: <https://github.com/munich-data-engineering/reflect-cpp/issues/684#issuecomment-4783221690> — #684 Request: Implement 'member-of-pointer' access operator (->) for Rfl types (2026-06-23)
[^388]: <https://github.com/munich-data-engineering/reflect-cpp/issues/684#issuecomment-5052053689> — #684 Request: Implement 'member-of-pointer' access operator (->) for Rfl types (2026-07-22)
[^389]: <https://github.com/munich-data-engineering/reflect-cpp/issues/684#issuecomment-5085409062> — #684 Request: Implement 'member-of-pointer' access operator (->) for Rfl types (2026-07-26)
[^390]: <https://github.com/munich-data-engineering/reflect-cpp/issues/685#issuecomment-4763194946> — #685 Cannot use rfl::DefaultVal and rfl::Field together in one strcut (2026-06-21)
[^391]: <https://github.com/munich-data-engineering/reflect-cpp/issues/685#issuecomment-4773255591> — #685 Cannot use rfl::DefaultVal and rfl::Field together in one strcut (2026-06-22)
[^392]: <https://github.com/munich-data-engineering/reflect-cpp/issues/683#issuecomment-4783241206> — #683 Automatic min and max in json schema for integers (2026-06-23)
[^393]: <https://github.com/munich-data-engineering/reflect-cpp/issues/683#issuecomment-5062399330> — #683 Automatic min and max in json schema for integers (2026-07-23)
[^394]: <https://github.com/munich-data-engineering/reflect-cpp/pull/687#issuecomment-4794371402> — #687 no internal linkage in headers (2026-06-24)
[^395]: <https://github.com/munich-data-engineering/reflect-cpp/issues/689#issuecomment-4879794143> — #689 Enum-Flag Default Values (2026-07-03)
[^396]: <https://github.com/munich-data-engineering/reflect-cpp/issues/689#issuecomment-4881678955> — #689 Enum-Flag Default Values (2026-07-04)
[^397]: <https://github.com/munich-data-engineering/reflect-cpp/issues/689#issuecomment-4881682483> — #689 Enum-Flag Default Values (2026-07-04)
[^398]: <https://github.com/munich-data-engineering/reflect-cpp/issues/689#issuecomment-4901862496> — #689 Enum-Flag Default Values (2026-07-07)
[^399]: <https://github.com/munich-data-engineering/reflect-cpp/issues/689#issuecomment-5417648546> — #689 Enum-Flag Default Values (2026-08-25)
[^400]: <https://github.com/munich-data-engineering/reflect-cpp/pull/688#issuecomment-4882920657> — #688 Avoid clang 21/22 C++26-mode frontend crash in StringLiteral's variadic constructor (2026-07-04)
[^401]: <https://github.com/munich-data-engineering/reflect-cpp/pull/688#issuecomment-5017692931> — #688 Avoid clang 21/22 C++26-mode frontend crash in StringLiteral's variadic constructor (2026-07-19)
[^402]: <https://github.com/munich-data-engineering/reflect-cpp/pull/688#issuecomment-5027423997> — #688 Avoid clang 21/22 C++26-mode frontend crash in StringLiteral's variadic constructor (2026-07-20)
[^403]: <https://github.com/munich-data-engineering/reflect-cpp/pull/691#issuecomment-5004977963> — #691 fix(tests): read cbor byte view via std::span, not basic_string_view<std::byte> (2026-07-17)
[^404]: <https://github.com/munich-data-engineering/reflect-cpp/pull/691#issuecomment-5017690555> — #691 fix(tests): read cbor byte view via std::span, not basic_string_view<std::byte> (2026-07-19)
[^405]: <https://github.com/munich-data-engineering/reflect-cpp/issues/692#issuecomment-5013198984> — #692 Feature Request: CLI usage/help generation (2026-07-18)
[^406]: <https://github.com/munich-data-engineering/reflect-cpp/issues/693#issuecomment-5013200441> — #693 Equality operator for rfl::Generic and rfl::Object (2026-07-18)
[^407]: <https://github.com/munich-data-engineering/reflect-cpp/pull/694#discussion_r3611275587> — #694 Added support for environment variables (2026-07-19)
[^408]: <https://github.com/munich-data-engineering/reflect-cpp/pull/694#discussion_r3611275945> — #694 Added support for environment variables (2026-07-19)
[^409]: <https://github.com/munich-data-engineering/reflect-cpp/pull/694#discussion_r3611281454> — #694 Added support for environment variables (2026-07-19)
[^410]: <https://github.com/munich-data-engineering/reflect-cpp/pull/694#discussion_r3611281636> — #694 Added support for environment variables (2026-07-19)
[^411]: <https://github.com/munich-data-engineering/reflect-cpp/pull/694#discussion_r3611288166> — #694 Added support for environment variables (2026-07-19)
[^412]: <https://github.com/munich-data-engineering/reflect-cpp/pull/694#discussion_r3611288413> — #694 Added support for environment variables (2026-07-19)
[^413]: <https://github.com/munich-data-engineering/reflect-cpp/pull/694#discussion_r3611290693> — #694 Added support for environment variables (2026-07-19)
[^414]: <https://github.com/munich-data-engineering/reflect-cpp/pull/694#discussion_r3611291514> — #694 Added support for environment variables (2026-07-19)
[^415]: <https://github.com/munich-data-engineering/reflect-cpp/pull/696#issuecomment-5027543893> — #696 Add operator== for rfl::Generic and rfl::Object (2026-07-20)
[^416]: <https://github.com/munich-data-engineering/reflect-cpp/issues/690#issuecomment-5031193923> — #690 FR: Deserialization to std::set with custom comparator (2026-07-21)
[^417]: <https://github.com/munich-data-engineering/reflect-cpp/issues/690#issuecomment-5038785173> — #690 FR: Deserialization to std::set with custom comparator (2026-07-21)
[^418]: <https://github.com/munich-data-engineering/reflect-cpp/issues/700#issuecomment-5051630483> — #700 Reading JSON into a struct with a `DefaultVal` field and a size validator field always raises a size validation error (2026-07-22)
[^419]: <https://github.com/munich-data-engineering/reflect-cpp/issues/700#issuecomment-6400336175> — #700 Reading JSON into a struct with a `DefaultVal` field and a size validator field always raises a size validation error (2026-10-25)
[^420]: <https://github.com/munich-data-engineering/reflect-cpp/issues/699#issuecomment-5051666539> — #699 Compilation error on Clang 15.0.0 (2026-07-22)
[^421]: <https://github.com/munich-data-engineering/reflect-cpp/issues/699#issuecomment-5051836045> — #699 Compilation error on Clang 15.0.0 (2026-07-22)
[^422]: <https://github.com/munich-data-engineering/reflect-cpp/pull/707#issuecomment-5084356396> — #707 Allow validated fields to work alongside defaulted ones (2026-07-26)
[^423]: <https://github.com/munich-data-engineering/reflect-cpp/pull/707#issuecomment-5204408626> — #707 Allow validated fields to work alongside defaulted ones (2026-08-06)
[^424]: <https://github.com/munich-data-engineering/reflect-cpp/issues/708#issuecomment-5095778455> — #708 MessagePack writer encodes C++ float values as float64 (2026-07-27)
[^425]: <https://github.com/munich-data-engineering/reflect-cpp/issues/706#issuecomment-5096583892> — #706 [Feature Request] ref::cli::read wchar_t **argv[] support (2026-07-27)
[^426]: <https://github.com/munich-data-engineering/reflect-cpp/issues/706#issuecomment-5101116536> — #706 [Feature Request] ref::cli::read wchar_t **argv[] support (2026-07-28)
[^427]: <https://github.com/munich-data-engineering/reflect-cpp/issues/706#issuecomment-5101166277> — #706 [Feature Request] ref::cli::read wchar_t **argv[] support (2026-07-28)
[^428]: <https://github.com/munich-data-engineering/reflect-cpp/issues/706#issuecomment-5107111366> — #706 [Feature Request] ref::cli::read wchar_t **argv[] support (2026-07-28)
[^429]: <https://github.com/munich-data-engineering/reflect-cpp/pull/710#issuecomment-5127839465> — #710 Preserve float width in MessagePack output (2026-07-30)
[^430]: <https://github.com/munich-data-engineering/reflect-cpp/issues/712#issuecomment-5321007407> — #712 rfl::to_view() does not seem to actually support being run at compile-time (2026-08-17)
[^431]: <https://github.com/munich-data-engineering/reflect-cpp/issues/712#issuecomment-5349012751> — #712 rfl::to_view() does not seem to actually support being run at compile-time (2026-08-19)
[^432]: <https://github.com/munich-data-engineering/reflect-cpp/issues/714#issuecomment-5380578754> — #714 Enum can be deserialized from raw integers that don't map to enum values (2026-08-22)
[^433]: <https://github.com/munich-data-engineering/reflect-cpp/issues/714#issuecomment-5416681141> — #714 Enum can be deserialized from raw integers that don't map to enum values (2026-08-25)
[^434]: <https://github.com/munich-data-engineering/reflect-cpp/issues/715#issuecomment-5384696180> — #715 YAML doesn't round-trip generics (2026-08-23)
[^435]: <https://github.com/munich-data-engineering/reflect-cpp/issues/715#issuecomment-5384778561> — #715 YAML doesn't round-trip generics (2026-08-23)
[^436]: <https://github.com/munich-data-engineering/reflect-cpp/issues/716#issuecomment-5384716898> — #716 Support serialising std::expected (2026-08-23)
[^437]: <https://github.com/munich-data-engineering/reflect-cpp/issues/716#issuecomment-5402458384> — #716 Support serialising std::expected (2026-08-24)
[^438]: <https://github.com/munich-data-engineering/reflect-cpp/issues/716#issuecomment-5417625424> — #716 Support serialising std::expected (2026-08-25)
[^439]: <https://github.com/munich-data-engineering/reflect-cpp/pull/721#issuecomment-5462280532> — #721 Output zero flag name if available (2026-08-29)
[^440]: <https://github.com/munich-data-engineering/reflect-cpp/pull/720#issuecomment-5462298442> — #720 Reject externally tagged variants with more than one field (2026-08-29)
[^441]: <https://github.com/munich-data-engineering/reflect-cpp/pull/720#issuecomment-5462306887> — #720 Reject externally tagged variants with more than one field (2026-08-29)
[^442]: <https://github.com/munich-data-engineering/reflect-cpp/pull/720#issuecomment-5464954581> — #720 Reject externally tagged variants with more than one field (2026-08-29)
[^443]: <https://github.com/munich-data-engineering/reflect-cpp/issues/723#issuecomment-5575319188> — #723 Support native non-string map keys for formats that allow them (2026-09-07)
[^444]: <https://github.com/munich-data-engineering/reflect-cpp/issues/719#issuecomment-5575333172> — #719 Segfault in rfl::json::read on GCC 12 -O2: read_struct builds its view over a T whose lifetime never began (2026-09-07)
[^445]: <https://github.com/munich-data-engineering/reflect-cpp/issues/719#issuecomment-6250148622> — #719 Segfault in rfl::json::read on GCC 12 -O2: read_struct builds its view over a T whose lifetime never began (2026-10-18)
[^446]: <https://github.com/munich-data-engineering/reflect-cpp/issues/719#issuecomment-6250487312> — #719 Segfault in rfl::json::read on GCC 12 -O2: read_struct builds its view over a T whose lifetime never began (2026-10-18)
[^447]: <https://github.com/munich-data-engineering/reflect-cpp/issues/719#issuecomment-6250517806> — #719 Segfault in rfl::json::read on GCC 12 -O2: read_struct builds its view over a T whose lifetime never began (2026-10-18)
[^448]: <https://github.com/munich-data-engineering/reflect-cpp/issues/719#issuecomment-6288222777> — #719 Segfault in rfl::json::read on GCC 12 -O2: read_struct builds its view over a T whose lifetime never began (2026-10-20)
[^449]: <https://github.com/munich-data-engineering/reflect-cpp/issues/724#issuecomment-5670913506> — #724 Add support for `std::inplace_vector` (2026-09-14)
[^450]: <https://github.com/munich-data-engineering/reflect-cpp/pull/725#issuecomment-5760700690> — #725 fix `Generic.cpp` build error due to missing includes (2026-09-21)
[^451]: <https://github.com/munich-data-engineering/reflect-cpp/pull/727#issuecomment-5858098113> — #727 toml: allow reading integer as double (2026-09-27)
[^452]: <https://github.com/munich-data-engineering/reflect-cpp/pull/729#issuecomment-5858102933> — #729 add `from_reflection(ReflectionType&&)` as alternative to ctor (2026-09-27)
[^453]: <https://github.com/munich-data-engineering/reflect-cpp/pull/726#issuecomment-5858112905> — #726 fix `CountFieldsHelper<T>` when a field is `std::optional<Eigen::Matrix4d>` (2026-09-27)
[^454]: <https://github.com/munich-data-engineering/reflect-cpp/pull/726#issuecomment-5879108620> — #726 fix `CountFieldsHelper<T>` when a field is `std::optional<Eigen::Matrix4d>` (2026-09-28)
[^455]: <https://github.com/munich-data-engineering/reflect-cpp/pull/728#issuecomment-5858127531> — #728 emit `"additionalProperties": false` with `rfl::NoExtraFields` (2026-09-27)
[^456]: <https://github.com/munich-data-engineering/reflect-cpp/pull/728#issuecomment-6427123127> — #728 emit `"additionalProperties": false` with `rfl::NoExtraFields` (2026-10-26)
[^457]: <https://github.com/munich-data-engineering/reflect-cpp/pull/730#issuecomment-5919813273> — #730 fix `NamedTupleParser::to_schema()` for non-`DefaultVal` fields (2026-09-30)
[^458]: <https://github.com/munich-data-engineering/reflect-cpp/issues/731#issuecomment-5919842882> — #731 Add ability to rename enum values (2026-09-30)
[^459]: <https://github.com/munich-data-engineering/reflect-cpp/pull/733#issuecomment-6081049738> — #733 Add inplace vector support (2026-10-09)
[^460]: <https://github.com/munich-data-engineering/reflect-cpp/pull/735#issuecomment-6081070563> — #735 Add DurationsAsCount preprocessor (2026-10-09)
[^461]: <https://github.com/munich-data-engineering/reflect-cpp/pull/736#issuecomment-6081096087> — #736 Reject trailing characters when parsing rfl::Timestamp (2026-10-09)
[^462]: <https://github.com/munich-data-engineering/reflect-cpp/pull/736#issuecomment-6084340626> — #736 Reject trailing characters when parsing rfl::Timestamp (2026-10-09)
[^463]: <https://github.com/munich-data-engineering/reflect-cpp/pull/736#issuecomment-6106908533> — #736 Reject trailing characters when parsing rfl::Timestamp (2026-10-11)
[^464]: <https://github.com/munich-data-engineering/reflect-cpp/pull/737#issuecomment-6157304107> — #737 Initialize structs using default values before parsing; resolves #719 and #700 (2026-10-13)
[^465]: <https://github.com/munich-data-engineering/reflect-cpp/pull/739#issuecomment-6175823348> — #739 Reserve vector capacity when array lengths are known (2026-10-14)
[^466]: <https://github.com/munich-data-engineering/reflect-cpp/pull/739#issuecomment-6224650718> — #739 Reserve vector capacity when array lengths are known (2026-10-16)
[^467]: <https://github.com/munich-data-engineering/reflect-cpp/pull/738#issuecomment-6175960297> — #738 Avoid eager error allocation when parsing named tuples (2026-10-14)
[^468]: <https://github.com/munich-data-engineering/reflect-cpp/pull/740#issuecomment-6224736544> — #740 Optimize numeric vectors in Cereal, YAS, and Boost binary (2026-10-16)
[^469]: <https://github.com/munich-data-engineering/reflect-cpp/pull/741#issuecomment-6224804816> — #741 Optimize positional field dispatch with compile-time indices (2026-10-16)
[^470]: <https://github.com/munich-data-engineering/reflect-cpp/pull/742#issuecomment-6224835177> — #742 Inline frequently used parsing functions (2026-10-16)
[^471]: <https://github.com/munich-data-engineering/reflect-cpp/pull/743#issuecomment-6248894351> — #743 Improve shared parser inlining and generated code layout (2026-10-18)
[^472]: <https://github.com/munich-data-engineering/reflect-cpp/pull/743#issuecomment-6250198533> — #743 Improve shared parser inlining and generated code layout (2026-10-18)
[^473]: <https://github.com/munich-data-engineering/reflect-cpp/pull/743#issuecomment-6250610047> — #743 Improve shared parser inlining and generated code layout (2026-10-18)
[^474]: <https://github.com/munich-data-engineering/reflect-cpp/pull/743#issuecomment-6250642872> — #743 Improve shared parser inlining and generated code layout (2026-10-18)
[^475]: <https://github.com/munich-data-engineering/reflect-cpp/pull/743#issuecomment-6301368786> — #743 Improve shared parser inlining and generated code layout (2026-10-20)
[^476]: <https://github.com/munich-data-engineering/reflect-cpp/pull/744#issuecomment-6248920396> — #744 Add opt-in FailFast processor for object field errors (2026-10-18)
[^477]: <https://github.com/munich-data-engineering/reflect-cpp/pull/746#issuecomment-6254524641> — #746 Reject out-of-range JSON integer values (2026-10-18)
[^478]: <https://github.com/munich-data-engineering/reflect-cpp/issues/749#issuecomment-6276272964> — #749 Add support for `std::monostate` (2026-10-19)
[^479]: <https://github.com/munich-data-engineering/reflect-cpp/pull/753#issuecomment-6350626909> — #753 Read files in bulk and report output stream failures (2026-10-22)
[^480]: <https://github.com/munich-data-engineering/reflect-cpp/pull/753#issuecomment-6384093347> — #753 Read files in bulk and report output stream failures (2026-10-24)
[^481]: <https://github.com/munich-data-engineering/reflect-cpp/pull/752#issuecomment-6384075398> — #752 Work around GCC 12 optional parsing crash with native tuple storage (2026-10-24)
[^482]: <https://github.com/munich-data-engineering/reflect-cpp/pull/758#issuecomment-6397213705> — #758 Restore CI binary caching and retry vcpkg downloads (2026-10-25)
[^483]: <https://github.com/munich-data-engineering/reflect-cpp/pull/755#issuecomment-6397226742> — #755 Omit redundant fixed-array lengths in Cereal, YAS and Boost.Serialization (2026-10-25)
[^484]: <https://github.com/munich-data-engineering/reflect-cpp/pull/757#issuecomment-6397237734> — #757 Avoid copying Cereal input buffers before deserialization (2026-10-25)
[^485]: <https://github.com/munich-data-engineering/reflect-cpp/pull/756#issuecomment-6397252310> — #756 Preserve typed map keys in MessagePack and binary archives (2026-10-25)
[^486]: <https://github.com/munich-data-engineering/reflect-cpp/pull/759#issuecomment-6403916214> — #759 Fix fixed-size arrays in typed map entries (2026-10-25)
[^487]: <https://github.com/munich-data-engineering/reflect-cpp/pull/759#issuecomment-6403919712> — #759 Fix fixed-size arrays in typed map entries (2026-10-25)
[^488]: <https://github.com/munich-data-engineering/reflect-cpp/pull/760#issuecomment-6427051474> — #760 Add bulk I/O missed in the recent std::array optimization (2026-10-26)
[^489]: <https://github.com/munich-data-engineering/reflect-cpp/pull/761#issuecomment-6427093124> — #761 Add std::bitset support with packed binary and string encodings (2026-10-26)
[^490]: <https://github.com/munich-data-engineering/reflect-cpp/pull/761#issuecomment-6427099810> — #761 Add std::bitset support with packed binary and string encodings (2026-10-26)
[^491]: <https://github.com/munich-data-engineering/reflect-cpp/pull/761#issuecomment-6438897282> — #761 Add std::bitset support with packed binary and string encodings (2026-10-27)
[^492]: <https://github.com/munich-data-engineering/reflect-cpp/pull/762#issuecomment-6565402703> — #762 Allow importing enchantum as a thirdparty library (2026-11-01)
[^493]: <https://github.com/munich-data-engineering/reflect-cpp/issues/768#issuecomment-6654469544> — #768 Clang-23 compilation error with -std=26 (2026-11-05)
[^494]: <https://github.com/munich-data-engineering/reflect-cpp/pull/766#discussion_r4479032468> — #766 Expose the location of each parse failure via rfl::Error::details() (2026-11-05)
[^495]: <https://github.com/munich-data-engineering/reflect-cpp/pull/766#discussion_r4479036242> — #766 Expose the location of each parse failure via rfl::Error::details() (2026-11-05)
[^496]: <https://github.com/munich-data-engineering/reflect-cpp/pull/766#issuecomment-6655158829> — #766 Expose the location of each parse failure via rfl::Error::details() (2026-11-05)
[^497]: <https://github.com/munich-data-engineering/reflect-cpp/pull/766#issuecomment-6655198583> — #766 Expose the location of each parse failure via rfl::Error::details() (2026-11-05)
[^498]: <https://github.com/munich-data-engineering/reflect-cpp/pull/766#issuecomment-6655482538> — #766 Expose the location of each parse failure via rfl::Error::details() (2026-11-05)
[^499]: <https://github.com/munich-data-engineering/reflect-cpp/pull/766#discussion_r4503840864> — #766 Expose the location of each parse failure via rfl::Error::details() (2026-11-07)
[^500]: <https://github.com/munich-data-engineering/reflect-cpp/pull/766#discussion_r4503841764> — #766 Expose the location of each parse failure via rfl::Error::details() (2026-11-07)
[^501]: <https://github.com/munich-data-engineering/reflect-cpp/pull/766#issuecomment-6713364820> — #766 Expose the location of each parse failure via rfl::Error::details() (2026-11-07)
[^502]: <https://github.com/munich-data-engineering/reflect-cpp/pull/767#issuecomment-6713271851> — #767 Avoid needless copies, allocations and exceptions when reading and writing (2026-11-07)
[^503]: <https://github.com/munich-data-engineering/reflect-cpp/pull/767#issuecomment-6713277803> — #767 Avoid needless copies, allocations and exceptions when reading and writing (2026-11-07)
[^504]: <https://github.com/munich-data-engineering/reflect-cpp/pull/769#issuecomment-6713390166> — #769 Make rfl::CopyableBox copy-assignable (2026-11-07)
[^505]: <https://github.com/munich-data-engineering/reflect-cpp/issues/777#issuecomment-6808150931> — #777 Inquiry about the format of contributions   (2026-11-11)
[^506]: <https://github.com/munich-data-engineering/reflect-cpp/pull/770#issuecomment-6808195570> — #770 Support JSON bytestrings as Base64 strings (2026-11-11)
[^507]: <https://github.com/munich-data-engineering/reflect-cpp/pull/770#issuecomment-6825506047> — #770 Support JSON bytestrings as Base64 strings (2026-11-12)
[^508]: <https://github.com/munich-data-engineering/reflect-cpp/pull/776#issuecomment-6837641939> — #776 Return an error when an rfl::Skip field's default value throws (2026-11-12)
[^509]: <https://github.com/munich-data-engineering/reflect-cpp/pull/778#issuecomment-6837804149> — #778 Let DefaultVal's converting constructor throw (2026-11-12)
[^510]: <https://github.com/munich-data-engineering/reflect-cpp/pull/779#issuecomment-6839522261> — #779 Say why default construction failed when reading a struct with defaults (2026-11-12)
[^511]: <https://github.com/munich-data-engineering/reflect-cpp/pull/782#issuecomment-6897308519> — #782 Update a DefaultVal test to the error message from #779 (2026-11-15)
[^512]: <https://github.com/munich-data-engineering/reflect-cpp/pull/783#issuecomment-6897318462> — #783 Read a bare negative number such as `-5` as a positional argument in rfl::cli (2026-11-15)
[^513]: <https://github.com/munich-data-engineering/reflect-cpp/pull/783#issuecomment-6926138698> — #783 Read a bare negative number such as `-5` as a positional argument in rfl::cli (2026-11-16)
[^514]: <https://github.com/munich-data-engineering/reflect-cpp/pull/785#issuecomment-6897351640> — #785 Write optional fields wrapped in rfl::Short or rfl::Positional (2026-11-15)
[^515]: <https://github.com/munich-data-engineering/reflect-cpp/pull/785#issuecomment-6926201156> — #785 Write optional fields wrapped in rfl::Short or rfl::Positional (2026-11-16)
[^516]: <https://github.com/munich-data-engineering/reflect-cpp/pull/786#discussion_r4579173329> — #786 Fix rfl::DefaultVal in NamedTuples and TaggedUnion schemas (2026-11-15)
[^517]: <https://github.com/munich-data-engineering/reflect-cpp/pull/786#discussion_r4590305835> — #786 Fix rfl::DefaultVal in NamedTuples and TaggedUnion schemas (2026-11-16)
[^518]: <https://github.com/munich-data-engineering/reflect-cpp/issues/781#issuecomment-6897549009> — #781 Binary data in `rfl::Generic` (2026-11-15)
[^519]: <https://github.com/munich-data-engineering/reflect-cpp/pull/784#issuecomment-6926170862> — #784 Accept const argv and ranges of arguments in rfl::cli::read (2026-11-16)
[^520]: <https://github.com/munich-data-engineering/reflect-cpp/pull/787#issuecomment-6926532431> — #787 Don't assign to const fields when reading a struct with defaults (2026-11-16)
