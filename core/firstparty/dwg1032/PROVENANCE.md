# Independent bounded binary cursor (2026-10-29)

- **Location:** `core/firstparty/dwg1032/` in the original MusaCAD repository. This is not `MusaCAD2/`.
- **Origin:** First-party C11 code independently authored from elementary binary arithmetic, integer bounds checks and pointer-range checks. No legacy MusaCAD core, LibreDWG, GPL/AGPL/LGPL code, other CAD decoder, or externally sourced implementation was copied, adapted, linked or used as a code template.
- **License:** MIT (SPDX headers). Requires only standard C11. The CMake target is standalone; it does not link the historical GPL-marked native core, Android JNI, or the separate scene2d target.
- **Capability:** A rollback-safe, bounded MSB-first generic bit reader, byte-aligned little-endian 16/32-bit reads, skip and byte-slice operations. Explicitly checks malformed cursor state, integer overflow, aliasing and short buffers. Tests include an exhaustive 0–4-byte field matrix and negative cases.
- **Crucial limitation:** The bit order is an explicit *generic* convention and **has not been established as the correct AC1032 DWG bitcode convention**. This module does not parse a DWG header, section map, compressed page, entity record, handle, TEXT/MTEXT or layer table; it does not open a real DWG file. Do not wire it into production JNI until actual format framing, versioned bitcode rules and independently licensed real-file fixtures are verified. No existing 3D source is modified.
- **Local verification:** `cmake -S core/firstparty/dwg1032 -B <build> -DBUILD_TESTING=ON && cmake --build <build> && ctest --test-dir <build> --output-on-failure`. GitHub CI must be checked separately.

## AC1032 signature-only preflight (2026-10-29)

- Added independently authored `musa_dwg1032_probe_signature` for the six ASCII version bytes at offset zero. It distinguishes an `AC1032` *candidate*, a different `AC`-numeric version and malformed/truncated signatures. No third-party parser or copyleft source is used.
- New negative tests cover truncation, invalid bytes, null input, oversized bounds and other version strings. CTest is wired into the isolated target.
- **Security gate:** A matching signature does **not** prove that the DWG structure is valid, that it is safe to render, or that any entity can be decoded. Do not enable JNI imports based on this result; real section/page/object decoding and licensed fixtures remain missing.

## Bounded 64-bit primitives (2026-10-29)

- Added standalone unsigned 64-bit MSB-first generic bit fields and byte-aligned unsigned little-endian 64-bit reads, including rollback on truncation, misalignment and aliased outputs. No external CAD decoder, licensed implementation, or historical GPL source was used.
- Exhaustive 0–9-byte field-length/offset/width regression test. This remains a generic binary reader, not verified AC1032 DWG bitcode interpretation; no production import is enabled.

## Transactional bounded byte frames (2026-10-29)

- Added a byte-aligned, allocation-free `musa_dwg1032_cursor_take_bytes` that advances the parent only when a bounded child frame is valid. Rejects truncated lengths, unaligned parent positions, aliased output and invalid state; a zero-length frame is allowed. Tests exercise successful sequential frames, rollback and empty input.
- This is a general-purpose framing primitive; AC1032 section directories, compression and object layout have **not** been decoded. The returned child is a borrowed view of caller-owned memory, not an independently verified DWG page. No production DWG opening or JNI fallback is enabled.

## Generic aligned byte reads and length-prefixed frames (2026-10-29)

- Independently implemented byte-aligned raw reads, single-byte reads, and a transactional uint32 little-endian length-prefixed frame reader. These use only standard C11, explicit bounds and disjoint destination checks; no third-party parser, GPL/AGPL/LGPL implementation, or unverified CAD decoder code was consulted or linked.
- Negative tests cover misalignment, empty buffers, aliased output, truncated payloads, oversized lengths, and unchanged cursor/output on failure. The caller supplies a maximum frame size.
- **Scope:** Generic binary building blocks only. No claim that AC1032 uses this frame layout. AC1032 section/page/object parsing, real DWG fixtures, and production JNI integration remain blocked pending independently verified format evidence.

## Independently implemented R2018 scalar bitcodes and handles (2026-10-29)

- **Format reference (facts, not source code):** Open Design Alliance, *Open Design Specification for .dwg files*, v5.4.1, sections 2.2 (BS), 2.3 (BL), 2.5 (BD), 2.12 (R2010+ object type), 2.13 (handle references), and 8 (R2018 changes): https://www.opendesign.com/files/guestdownloads/OpenDesign_Specification_for_.dwg_files.pdf . The specification is copyrighted reference material, **not** imported software and **not** a license grant for an SDK. No ODA source implementation, LibreDWG, GPL/AGPL/LGPL code, or legacy CAD implementation was copied, adapted, or linked. C11 implementations and synthetic regression vectors were authored independently from the documented binary field rules.
- **New standalone modules:** `musa_dwg1032_bitcode.[ch]` decodes bitshort, bitlong, finite IEEE-754 bitdouble, and R2010+ object-type fields, including fields starting at non-byte-aligned positions. `musa_dwg1032_handle.[ch]` reads absolute/relative handle references and resolves a supplied base handle with overflow and underflow checks. Both are integrated only with the independent bounded cursor target and CTest.
- **Safety:** Invalid opcodes, truncated payloads, invalid handle forms, aliased outputs, non-finite geometry doubles, and unrepresentable relative handles fail without modifying caller cursor or output. The object-type opcode 3 is deliberately rejected rather than guessing a meaning. A decoded reference is **not** proof of ownership, layer membership, or target existence.
- **Verification:** Standalone synthetic tests exercise scalar values, unaligned handle streams, malformed encodings, truncation and rollback. GitHub Actions results must be checked for the exact final commit; local tests are not evidence of complete AC1032 import.
- **Remaining work:** File-header and section/page map validation, compressed section reconstruction, object map, separate data/string/handle stream positioning, real licensed AC1032 fixtures, layer and entity graph resolution, 2D scene integration, Android rendering, and full 3D regression coverage. Do not activate production DWG import based solely on these scalar primitives.

## Independently authored post-decode object indexing and owner validation (2026-10-29)

- Source: original C11 implementation using standard binary search, bounds arithmetic and bounded parent traversal; no third-party code, GPL/AGPL/LGPL implementation, legacy CAD core, or unverified source was imported or linked. These are generic algorithms on caller-supplied, already decoded records, not DWG reverse-engineering claims.
- `musa_dwg1032_object_index.[ch]`: checks nonzero, strictly sorted unique handles and nonempty in-section byte ranges, then performs O(log n) handle lookup, including previously decoded relative references. It does not verify nonoverlapping ranges or parse the AC1032 object map.
- `musa_dwg1032_owner_chain.[ch]`: validates caller-supplied owner-handle chains with a maximum hop budget, detects missing handles and cycles, and reports the root and depth only on success. It cannot infer owner semantics or visibility until real entity records and table relationships are decoded.
- Synthetic CTest vectors cover malformed bounds, duplicate/unsorted handles, 2048-entry index lookup, missing/cyclic owners, 4096-level ownership chains, output rollback and alias rejection. This is not a real DWG fixture or production import test.
- **Still blocked:** Independently verified AC1032 section/page decompression, object-map extraction, true ownership and layer data, TEXT/MTEXT parsing, rendering and Android integration. Keep existing 3D code untouched and do not activate these modules as a production DWG decoder.

## First-party object-span validation and bounded views (2026-10-29)

- Added `musa_dwg1032_object_spans.[ch]` to the original MusaCAD standalone C11 target. This independently written module sorts caller-decoded object locations by byte offset using an in-place heap in caller-provided workspace, rejects overlapping/invalid ranges, supports logarithmic byte-offset lookup, and returns borrowed bounded cursors for validated spans. It does not allocate or import a CAD decoder.
- Tests cover unsorted offsets with sorted handles, touching spans, gaps, truncation, malformed ranges, alias rejection, `UINT64_MAX` boundaries, 20,000 synthetic objects and 4,096 deterministic boundary queries. Locally built with GCC and Clang, including AddressSanitizer and UndefinedBehaviorSanitizer. These synthetic tests do **not** establish real AC1032 compatibility.
- **Origin and license:** Independently authored from generic integer arithmetic, heapsort and interval-search principles; MIT SPDX. No LibreDWG, GPL/AGPL/LGPL, historical CAD implementation, or source of uncertain provenance was copied, adapted or linked. Existing 3D code, `MusaCAD2/` and production JNI are untouched.
- **Gate:** AC1032 compressed page recovery, authentic object map and record decoding, independently licensed real-file fixtures, TEXT/MTEXT, layer/block graph integration and Android rendering are not solved by this module. Never report a full DWG open based on these tests.

## Full decoded owner-graph validation (2026-10-29)

- **Files:** `include/musa_dwg1032_owner_graph.h`, `src/musa_dwg1032_owner_graph.c`, and the dedicated bulk and forest regression tests. Independently authored MIT C11 from elementary graph traversal, path compression and binary search; no third-party CAD implementation or GPL/AGPL/LGPL source copied, adapted or linked.
- **Behavior:** Resolves root handles and depths for all caller-decoded owner relationships in O(n log n) time without recursion or heap allocation, using caller-owned scratch space. Rejects dangling references, self-ownership, longer cycles and overlapping buffers. Published outputs are untouched on failure; scratch may be modified.
- **Coverage:** 20,000-object deep chain, multiple roots, branching and rollback; an independent slow reference walk checks 12,000 deterministic forest nodes and a late corruption. GCC and Clang ASan/UBSan local tests used a minimal compatible object-index stub; full repository build and GitHub CI are separate gates.
- **Boundary:** Inputs are **already decoded** object handles and owners. This does not locate AC1032 objects, parse compressed pages, validate CAD-specific block ownership semantics, open real DWG files or enable Android JNI. Existing 3D sources, `MusaCAD2/` and `main` remain untouched.

## 2026-10-29 — independent MC/MS scalar reader
- New MIT C11 module `musa_dwg1032_modular` handles bounded modular chars (signed file offsets, unsigned handle deltas) and modular shorts (object sizes), with rollback on malformed/truncated input and overflow.
- Implementation was independently authored using the *descriptive* integer encoding conventions in the publicly published Open Design Alliance DWG specification (section 2.6/2.7, https://www.opendesign.com/files/guestdownloads/OpenDesign_Specification_for_.dwg_files.pdf). No third-party source code was copied, adapted, linked or imported.
- The decoder has synthetic positive/negative/limit/corruption tests. It does not parse AC1032 section maps, decompress pages, decode objects, or enable Android DWG opening; these remain unverified acceptance gates.

## 2026-10-29 — bounded object-map delta decoding
- Original MIT C11 `musa_dwg1032_object_map_decode` converts a caller-verified, already decompressed object-map payload into monotonic handles and bounded absolute byte offsets. Signed offset deltas, malformed encodings, overflow, trailing bytes and output rollback are covered by regression tests including 10,000 records.
- Encoding conventions referenced only as technical facts from the published Open Design Alliance specification (MC, object-map chapter). No copyleft or third-party source code copied, adapted or linked.
- This does **not** locate or decompress AC1032 sections, validate map-page CRCs, infer object lengths, decode actual entity records, or enable Android drawing import. The caller must verify map provenance and boundaries before use.

## 2026-10-29 — first-party 2D/3D coordinate bitcodes
- Added independently authored MIT C11 bounded coordinate readers for 2RD, 3RD, 2BD and 3BD to the original MusaCAD DWG module, based on published bitcode field descriptions and ordinary IEEE-754 arithmetic. No CAD library code or GPL/AGPL/LGPL/uncertain-origin implementation was copied, adapted or linked.
- Atomic cursor/output rollback and nonfinite rejection are covered by synthetic vectors, unaligned bit positions, truncation, reserved opcodes, aliases, 20,000 coordinate pairs, and a nonfinite final coordinate. Local GCC and Clang compilation with AddressSanitizer/UndefinedBehaviorSanitizer was exercised against a minimal test stub; full repository CI remains authoritative.
- These functions operate **only on already validated, decompressed bitstreams**. AC1032 section maps, page decompression, object schema decoding, real-file fixtures, 2D render integration and Android import are **not** implemented by this change. Existing 3D source, MusaCAD2/ and main are untouched.

## 2026-10-29 — bounded unaligned bit windows
- Independently authored MIT C11 `musa_dwg1032_bit_window` allows a caller to isolate arbitrary bit-aligned, borrowed substreams, including nested windows, with strict end-bit enforcement, checked arithmetic, alias rejection and transactional reads. This is a generic binary boundary mechanism, not a claim about the undocumented AC1032 page layout.
- Synthetic tests exercise unaligned boundaries, zero-length windows, 64-bit reads, rollback, corruption and 20,000 differential 32-bit windows plus 4,096 64-bit comparisons. Tests do not constitute real-file validation.
- No LibreDWG, GPL/AGPL/LGPL or unknown-provenance code was copied, adapted or linked. Production JNI, existing 3D code, `MusaCAD2/` and `main` remain untouched. AC1032 compressed page recovery, actual entity schemas and Android 2D rendering remain blocked pending independent real-file fixtures and integration.

## 2026-10-30 — checked logical section assembly

- New independent MIT C11 `musa_dwg1032_section_assembly.[ch]` accepts **caller-verified and already decompressed** page fragments in logical order and copies them into a single caller-owned section buffer. This is a generic contiguous-range operation; no third-party code, LibreDWG, GPL/AGPL/LGPL implementation, uncertain-origin decoder, or legacy CAD core was copied, adapted, linked, or used as a template.
- A validation pass checks complete coverage, ordering, gaps, overlaps, size bounds and destination/source/metadata aliasing **before any destination writes**. O(n + bytes) runtime and constant extra memory, with no internal allocation. Empty sections are accepted.
- Regression tests cover missing and overlapping fragments, truncated sections, null pointers, late corruption with unchanged output, 20,000 one-byte fragments and 65,536-byte partition sweeps with strides 1–1,024, including late alias rejection. GCC/Clang and sanitizer results must be evaluated separately from CI.
- **Critical boundary:** The caller must authenticate AC1032 page map metadata, validate compressed-page checksums and decompress each page. This module does none of those operations and does not claim to open or render an actual DWG. Android JNI, 3D sources, `MusaCAD2/` and `main` remain untouched.

## 2026-10-30 — bounded object-map candidate spans

- Added `musa_dwg1032_object_map_candidate_spans` to connect caller-verified, already decoded object-map handle/offset pairs to bounded *candidate* regions in the logical object section. It uses a caller-provided heap-sort workspace, O(n log n) ordering by physical offset, no allocation, and transactional output publication.
- The independent MIT C11 implementation uses generic integer checks and heap sorting; no GPL/AGPL/LGPL, legacy CAD implementation, SDK or unknown-provenance source was copied, adapted or linked. Synthetic tests include 20,000 reverse-ordered offsets, duplicates, out-of-section offsets, buffer aliases and rollback.
- **Important:** The inferred region extends to the next object's starting offset, so it is only an upper bound. It may include padding or non-record data; it is **not** a decoded object length and must not be passed off as validated entity data. Real AC1032 section maps, compressed pages, record lengths, licensed real-file fixtures and Android integration are still missing. Production JNI stays fail-closed; existing 3D sources are unchanged.

## 2026-10-30 — optional physical file-page directory validation

- Added independently authored MIT C11 `musa_dwg1032_file_pages_validate_physical` to check a **caller-supplied** physical page directory. The algorithm uses generic in-place heapsort over caller-owned index scratch, validates file bounds, and rejects duplicate or intersecting physical page ranges in O(n log n) time without heap allocation. A 16,384-page synthetic reverse-order stress test and malformed-directory tests exercise this gate.
- This validation is intentionally **opt-in**: `musa_dwg1032_section_assemble_file_pages` retains its existing behavior because some callers may intentionally reuse source bytes. The new gate is appropriate only where a verified directory format requires disjoint physical pages. It does not discover, authenticate, decompress or interpret AC1032 pages, and does not prove that a candidate page map is genuine.
- No LibreDWG, GPL/AGPL/LGPL, unknown-provenance CAD code or legacy native implementation was copied, adapted or linked. The original MusaCAD 3D code, production JNI, `MusaCAD2/` and `main` remain untouched. Real licensed AC1032 fixtures and end-to-end entity/layer/text/block decoding remain required before production DWG import.
