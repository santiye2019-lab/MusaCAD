# Bounded per-INSERT attributes paging — 2026-11-03

## Implementation and provenance

- First-party C11 implementation: `include/musa_dwg1032_instance_attributes.h` and `src/musa_dwg1032_instance_attributes.c`.
- This change was independently authored from the local first-party `musa_dwg1032_instance_attributes` API contract and C11 pointer/range arithmetic. No GPL/AGPL/LGPL code, external CAD decoder, or third-party implementation was copied, adapted, or linked for this increment.
- The wrapper validates the requested child interval, checks **all** source arrays (not only the selected page) against both published outputs and caller scratch, and delegates each page to the existing transactional color-and-visibility composer.
- Scratch requirement is proportional to page size rather than the full child count; total runtime across pages depends on repeated layer/viewport metadata validation. This is a memory-bound improvement, **not** a measured speedup.
- `tests/dwg1032_instance_attributes_page_test.c` compares 20,000 synthetic children against full-array composition using 257-child pages, and tests late invalid owners, future-page aliasing, empty pages and invalid ranges.
- CTest registration: `firstparty_dwg1032_atomic_insert_page_20k`. CI outcomes must be checked for the exact branch SHA; source and test registration alone are not a passing result.

## Strict limits

This API consumes caller-verified, **already decoded** child colors, handles, owner-only visibility, layer states and viewport freezes. It does not locate AC1032 sections, decompress pages, decode real entity records, prove block ownership, or open/render a DWG. Per-instance color and visibility must be resolved separately for every concrete INSERT. It must not be wired into the Android normal-open JNI path before real-file provenance and end-to-end scene parity are independently established.

Existing 3D code, `main`, and `MusaCAD2/` are outside this change.
