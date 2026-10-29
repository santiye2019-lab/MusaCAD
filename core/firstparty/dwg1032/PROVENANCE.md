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
