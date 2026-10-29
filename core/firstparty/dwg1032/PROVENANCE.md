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
