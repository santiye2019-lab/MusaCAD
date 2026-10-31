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

## 2026-10-30 — strict physical-page assembly gate

- Added `musa_dwg1032_section_assemble_checked_file_pages` as an opt-in bridge between independently validated physical page spans and transactional logical section assembly. It rejects physical overlap, out-of-bounds spans, logical gaps, and scratch-buffer aliasing with file data, page metadata, or destination before copying. Unlike the older permissive assembly function, it disallows reused physical byte ranges.
- Synthetic tests cover in-bounds overlapping pages, duplicate physical spans, logical gaps, scratch aliasing, unchanged destination on failure, and 16,384 reverse-ordered physical pages. Tests must pass GitHub CI before reporting success.
- The implementation is original MIT C11 using only bounded arithmetic and in-place sorting; no GPL/AGPL/LGPL, LibreDWG, legacy native CAD code, SDK, or uncertain-origin source was copied, adapted, or linked.
- **Scope:** The caller must still provide verified physical page descriptors and raw or already decompressed data. This does not discover an AC1032 section directory, decompress pages, validate page CRCs, decode entities or enable Android production DWG import. Existing 3D code, `MusaCAD2/` and `main` remain untouched.

## 2026-10-30 — bounded MS-prefixed record envelopes

- Added `musa_dwg1032_record_frame.[ch]` as an independent MIT C11, allocation-free, transactional reader of an **MS-length-prefixed payload inside a caller-verified bounded byte region**. The MS integer encoding convention is already documented above from the published ODA DWG specification (section 2.7); the implementation is original and uses only this repository's independent cursor and modular-short primitives.
- The decoder requires a nonzero payload length, a caller-defined maximum, complete bytes and disjoint output. It leaves the candidate cursor and output unchanged on malformed/truncated data, supports one- and two-word MS lengths, and does not silently consume trailing candidate-span bytes.
- Synthetic CTest covers 20,000 sequential record frames, 32,768-byte two-word length, truncated/overlong inputs, unaligned state, output aliasing, rollback and zero length. These are **not real AC1032 fixture tests**.
- A candidate object-map interval is only an upper bound. The caller must independently establish whether and where an AC1032 object record uses this prefix, verify record framing and any checksums, and separately decode the object type, handles, layer, owner, block, geometry and text. No real-file opening or production JNI path is enabled by this module.
- No third-party source code, LibreDWG, GPL/AGPL/LGPL implementation, historical native core or uncertain-provenance CAD decoder was copied, adapted, imported or linked. `MusaCAD2/`, `main` and existing 3D sources are untouched.

## 2026-10-30 — strict AC1032 fixed-file preamble (first format-specific file gate)

- Added independently authored MIT C11 `musa_dwg1032_preamble.[ch]` and synthetic corruption/rollback CTest. It reads the **unencrypted fixed 0x80-byte preamble** of a caller-supplied AC1032 file, requires the 0x100-byte initial header extent, validates documented reserved bytes and the 0x80 header pointer, bounds-checks optional preview/summary/VBA offsets, and returns maintenance, writer, codepage and security fields. No heap allocation, no source-buffer modification and no partial output publication.
- **Format facts only:** Open Design Alliance, *Open Design Specification for .dwg files*, v5.4.1, section 4.1 and chapters 6–8: https://www.opendesign.com/files/guestdownloads/OpenDesign_Specification_for_.dwg_files.pdf . The document is a technical reference, not source code or an SDK license. This implementation was written from the listed byte positions and ordinary little-endian integer operations; no ODA/LibreDWG/GPL/AGPL/LGPL/unknown-provenance implementation was copied, adapted, linked or used as a template.
- **Deliberate limits:** This strict preflight may reject nonstandard/recoverable files. It does **not** decrypt or authenticate the 0x6c-byte file-header payload at 0x80, validate its CRC32, follow section maps, decompress pages, decode objects, check real-file fixtures or render geometry. Reported security flags must be interpreted by a future independently verified reader; nonzero encryption bits are **not** permission to read protected sections. Production JNI remains disconnected and existing 3D, `MusaCAD2/` and `main` are unchanged.
- **Verification gate:** The synthetic test is wired into standalone CTest. Record exact GitHub CI results for the final commit; no real DWG fixture was introduced.

## 2026-10-30 — bounded streaming AC1032 preamble preflight

- Added `musa_dwg1032_read_preamble_prefix` to interpret exactly 256 caller-read bytes while checking the unencrypted fixed-preamble offsets against a separate 64-bit total file length. This avoids reading or mapping a multi-gigabyte DWG merely to inspect its first header bytes. The original full-buffer API remains compatible and retains its full-file input/output alias guard.
- First-party MIT C11 implementation derived solely from the previously documented fixed-field layout and elementary integer/pointer bounds arithmetic. No GPL/AGPL/LGPL, legacy core, LibreDWG or uncertain-provenance source was copied, adapted, linked or used as a template. Only the isolated `core/firstparty/dwg1032` target was changed.
- Synthetic tests cover equivalence with the original API, files declared larger than 4 GiB, short/oversized prefixes, offsets outside the declared file length, null pointers, aliased output and transactional failure.
- **Not a full file parser:** The 256-byte prefix is caller-supplied and must really come from the beginning of the same file whose length is reported. The encrypted header, section directory, page compression, object records, layer and block visibility, TEXT/MTEXT, real DWG fixtures and Android rendering remain unverified. A successful preflight must not enable JNI import.

## 2026-10-30 — transactional batch candidate-to-record extraction

- Added `musa_dwg1032_record_batch.[ch]` to connect **already verified** non-overlapping object-map candidate spans to MS-prefixed bounded record payloads. It retains handle identity and publishes exact payload byte spans without candidate padding, allocation or copying. This is a *candidate framing gate*, not an entity decoder.
- Preflight checks physical overlap using caller-owned sorting scratch, rejects output/scratch aliasing with source bytes and metadata, validates every MS record and payload limit before publishing any output. Input must remain immutable during both passes. Time is O(n log n + n record-prefix reads), with O(n) caller-provided output/scratch.
- Synthetic regression checks reversed physical ordering, late malformed records with unchanged outputs, aliasing, zero-sized sections and 20,000 object candidates. CTest and the independent C11 workflow are authoritative only after their run has completed; do not report pending jobs as passed.
- **Provenance:** Original first-party MIT C11 implementation based on generic bounds arithmetic and existing independent MusaCAD modules. No GPL/AGPL/LGPL, LibreDWG, legacy native implementation, third-party CAD source or unknown-origin code was copied, adapted or linked.
- **Still blocked:** Authentic AC1032 page directory, decompression/checksums, record schema, string/handle stream interpretation, layer/owner/block graph, licensed real DWG fixtures and Android drawing. No production JNI, existing 3D source, `MusaCAD2/` or `main` changes.

## 2026-10-30 — immutable owner-index preflight and rollback regression

- Strengthened the independently authored MIT C11 owner-graph resolver to revalidate caller-owned index entries before traversing ownership references. It rejects zero, duplicate or unsorted handles, zero-length entries and out-of-section spans, including indices modified after their initial validation. Binary-search owner lookup relies on these invariants.
- Added synthetic regression cases for six mutations to a previously valid three-object owner chain, verifying unchanged published roots and depths after rejection and successful recovery once the index is restored. Existing 12,000-node forest and 20,000-node chain tests remain wired into standalone CTest.
- This change only validates **predecoded, caller-supplied** handles and spans; it does not decode real AC1032 object ownership, layers, text, INSERT transforms or visibility. No production JNI import is enabled. No LibreDWG, GPL/AGPL/LGPL, third-party implementation or unknown-provenance source code was copied, adapted or linked. The existing 3D implementation, `MusaCAD2/` and `main` are outside this change.

## 2026-10-30 — bounded page extraction of prevalidated object candidates

- Added `musa_dwg1032_record_batch_extract_page` for the original MusaCAD C11 core. A caller first validates **the entire immutable candidate index** with `musa_dwg1032_object_spans_prepare`; subsequent calls extract only requested MS-framed record pages without repeating whole-index sorting or allocating a full-document payload array. O(requested page size) processing, constant auxiliary storage.
- Page-level preflight rejects malformed frames, non-finite bounds, misaligned sections, output/input aliasing and overlapping metadata before publishing any output. On failure, the page and both progress counters remain unchanged. Existing full-batch behavior is preserved.
- Synthetic regression checks compare 20,000 reverse-ordered object candidates across 78 pages against the full-batch output, exercise late page corruption and ensure rollback. GitHub CI results must be verified for the final commit; passing synthetic tests cannot certify DWG decoding.
- Source provenance: independently authored MIT C11 using elementary range checks and bounded iteration. No GPL/AGPL/LGPL code, LibreDWG, legacy CAD implementation, uncertain-origin decoder, or third-party CAD SDK was copied, adapted or linked. The page API does **not** discover or authenticate AC1032 section/page directories, decompress pages, verify true entity record framing, or parse text/layers/blocks. Do not enable production JNI DWG import on this basis. Existing 3D code and `MusaCAD2/` remain untouched.

## Verified map-to-record pipeline (2026-10-30)

- Added `musa_dwg1032_record_batch_from_verified_map` to connect the existing bounded modular object-map decoder, candidate span derivation and MS-framed record validation in one allocation-free call. The map and object section are **supplied by the caller and must already be verified and decompressed**; scratch buffers are caller-owned, disjoint, and may change on failure. Published payload spans remain unchanged on any validation failure.
- Synthetic integration tests cover out-of-physical-order object offsets, truncated/malformed final frames, malformed signed map deltas, scratch/source alias rejection and 20,000 sequential objects. This is test coverage for the intermediate pipeline only, **not** a real DWG fixture or proof of AC1032 entity semantics. GitHub CI results are tracked separately for each commit.
- Source origin: independently authored MIT C11 orchestration of existing first-party functions and ordinary bounds/alias checks. No GPL/AGPL/LGPL, LibreDWG, legacy decoder or unknown-origin code copied, adapted or linked. Existing 3D files, `MusaCAD2/`, production JNI and `main` remain untouched.
- **Release gate:** Authentic AC1032 section/page directories, checksums, compression, object-stream layout, text, layers, block visibility and independently licensed real DWG fixtures remain unresolved. Do not advertise successful 2D DWG import or enable normal Android DWG opening on the strength of synthetic tests.

## 2026-10-31 — incremental object-map delta iterator

- Added `MusaDwg1032MapIterator` in the independent MIT C11 object-map module. It incrementally consumes one unsigned handle delta and signed byte-offset delta from an **already authenticated and decompressed** map payload, without allocating a whole-map output array. The existing all-or-nothing decoder now uses the same single-entry decoding logic, preventing drift between streaming and batch semantics.
- Iterator initialization and each next step reject aliased output, malformed or truncated modular encodings, zero/overflowing handle steps, underflowing/overflowing offsets and offsets outside the supplied object section. A rejected step leaves both iterator state and the last published entry unchanged. The caller must still check the expected entry count and exact end-of-payload consumption; a successful prefix alone is **not** a complete or authenticated map.
- Synthetic CTest includes a 20,000-entry streaming map, negative deltas, truncation, late invalid signed delta and transactional rollback. CI must finish before test success is claimed.
- Provenance: original first-party C11 logic derived from the existing repository's independently authored modular integer conventions and elementary bounds arithmetic; no LibreDWG, GPL/AGPL/LGPL, legacy core or uncertain-provenance implementation copied, adapted or linked. No changes to `main`, `MusaCAD2/`, existing 3D or production JNI.
- **Remaining gate:** This is a low-level building block only. Authentic AC1032 file-header decoding, section maps, decompression/checksums, object schema, original layer/color/text/block interpretation, real-file fixtures and Android 2B rendering are still unverified.

## 2026-10-31 — bounded random-access MS candidate lookup

- Added `musa_dwg1032_record_batch_extract_handle` to the original standalone MIT/C11 DWG1032 target. This is an independently written composition of the existing first-party binary handle index and bounded MS candidate-frame extractor, using elementary binary search and range checks. No LibreDWG, historical native CAD decoder, GPL/AGPL/LGPL code or uncertain-origin source was copied, adapted, or linked.
- Given an **immutable, previously validated** index and caller-verified, already decompressed object section, a handle can be located in O(log n) and its candidate payload checked without sorting or extracting the entire map. Output is unchanged on lookup failure, malformed target, invalid bounds, or overlapping storage. Synthetic tests include reverse physical order, missing handles, corrupted targets, and sparse lookups across 20,000 candidates.
- **Not a DWG import milestone:** The MS envelope is an assumption about caller-supplied candidate records, not proof of authentic AC1032 framing. Real file section maps, decompression, CRC, object/entity semantics, original layers/colors and Android rendering are still unverified. Do not enable JNI or claim real-file compatibility from these tests.

## 2026-10-31 — bounded random-access reads across verified file pages

- Added a one-time logical/physical page-directory validator and an allocation-free window reader in the standalone first-party C11 section assembly module. After validation, a viewport consumer can binary-search a logical offset and copy only the requested raw page fragments rather than assemble the entire section. Complexity per read is O(log(page_count) + touched_pages + window_bytes), with constant auxiliary memory.
- Window preflight checks bounds, physical truncation and destination aliasing before copying. Synthetic tests exercise a three-page cross-boundary window, invalid late page rollback, empty sections, and 20,000 pages in reverse physical order. CI outcomes must be checked for the final commit; synthetic test definitions are not proof that tests passed.
- Provenance: independently authored MIT C11 based on basic binary search, contiguous range checks and standard memory copying. No GPL/AGPL/LGPL, LibreDWG, legacy core or uncertain-provenance CAD source was copied, adapted or linked. Existing 3D, production JNI, `MusaCAD2/` and `main` remain untouched.
- **Critical limitation:** Page descriptors and raw/decompressed bytes are still caller-provided. This does not locate, authenticate, decompress or verify checksums of genuine AC1032 pages, decode entity records, or establish original layer/color/text/block fidelity. A successful synthetic test does not authorize normal DWG opening in Android.

## 2026-10-31 — bounded random-access object windows from verified RAW pages

- Added independent MIT C11 `musa_dwg1032_object_window.[ch]` and synthetic CTest. It bridges the existing validated handle index with the bounded, caller-verified physical page window reader, so a small portion of an object candidate can be read without allocating or assembling the full object section. Lookup is logarithmic in object and page counts, followed by the number of touched pages and requested bytes.
- The caller must first validate the immutable complete page directory with `musa_dwg1032_file_pages_validate_window_directory` and the immutable candidate index with `musa_dwg1032_object_spans_prepare`. Both must describe the same logical section. The reader rejects out-of-range windows and writable-buffer aliasing, preserves output and candidate length on failure, and supports zero-byte reads at the candidate end.
- Synthetic regression tests exercise out-of-order physical pages, a window crossing a page boundary, missing handles, out-of-bounds offsets, alias rejection, a late invalid page with unchanged output, and 20,000 handle lookups across reverse-ordered one-byte pages. CI results must be checked for the exact commit before claiming these tests pass.
- **Provenance:** Original implementation using elementary binary search, bounded range checks and existing first-party section-window APIs. No third-party DWG implementation, LibreDWG, GPL/AGPL/LGPL, SDK code, uncertain-origin source or legacy native CAD implementation was copied, adapted, linked or used as a template.
- **Hard limit:** The supplied pages are already RAW or decompressed and independently verified. Candidate spans are upper bounds, **not decoded AC1032 object lengths**. This change does not locate/authenticate/decompress real AC1032 pages, verify CRCs, decode TEXT/MTEXT, layers or blocks, enable Android DWG import, or alter existing 3D sources, `MusaCAD2/`, or `main`.

## 2026-10-31 — lazy bounded MS payload windows over verified RAW pages

- Added `musa_dwg1032_object_ms_payload_read_window` to the existing standalone first-party C11 object-window module. It composes the previously authored handle-index/page-window reader with the canonical bounded modular-short decoder, fetching only the 2- or 4-byte MS prefix and the requested payload bytes. It does not allocate or assemble a whole object section, and it enforces both the caller's maximum payload size and the selected candidate's physical bounds.
- Before any caller output is written, the prefix and payload bounds are checked; late touched-page truncation, malformed MS encodings, zero-length frames, excessive lengths, missing handles and writable-output aliasing fail without modifying the payload buffer or length. Synthetic CTest exercises physical page reordering, split extended prefixes, rollback and 20,000 random handle reads across reverse-ordered RAW pages.
- **Provenance:** Independently authored MIT C11 using existing MusaCAD first-party primitives, standard C pointer/bounds checks and published MS integer format facts. No LibreDWG, ODA SDK, GPL/AGPL/LGPL, historical native CAD source or uncertain-origin implementation was copied, adapted or linked.
- **Not a real DWG decoder:** The page directory, object index and RAW/decompressed bytes remain supplied and preverified by the caller. MS framing is an assumed candidate envelope, not authenticated AC1032 object semantics. Authentic section-map discovery, page decompression/CRC, actual entity type and handle streams, layers/colors, TEXT/MTEXT, block visibility, real-file fixtures and Android drawing are still missing. No production JNI, existing 3D, `MusaCAD2/` or `main` changes.


## 2026-10-31 — single-lookup lazy MS window optimization

- The independently authored lazy MS candidate reader now resolves a handle once, then uses the previously implemented checked logical-page reader for the two-byte prefix, optional continuation, and requested payload. This removes repeated binary searches for each small viewport object read without allocating or copying the full candidate. Source/page bounds and the transactional no-partial-output guarantee remain enforced.
- Regression assertions explicitly reject writable destination and length aliases into the index or page directory. Existing synthetic split-prefix, truncated-page and 20,000-object tests remain in the test suite; a successful CI run for the exact new commit is required before treating them as passed.
- **Source provenance:** Original MIT C11 changes using only the existing first-party MusaCAD object-index, page-window, and modular-short APIs. No GPL/AGPL/LGPL, LibreDWG, ODA SDK, uncertain-origin code, or legacy CAD source was copied, adapted, or linked.
- **Scope limitation:** The caller still supplies verified RAW/decompressed pages and a prepared candidate index. This optimization is not AC1032 section-map discovery, compression, CRC validation, entity decoding, layer/color/text/block interpretation, or an Android DWG import integration. Existing 3D code, `MusaCAD2/`, and `main` remain untouched.
