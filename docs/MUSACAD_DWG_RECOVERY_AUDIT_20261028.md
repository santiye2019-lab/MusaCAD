# MusaCAD original-core recovery audit — 2026-10-28

## Decision
Continue development in the existing MusaCAD repository and `core/native` rather than restarting the implementation in MusaCAD2. This branch is based on `main`; no changes to `main` are implied.

## Verified from source (main)
- `app/src/main/cpp/CMakeLists.txt` builds Android `musacad_dwg` with the portable `musacad_core` and returns before host-only LibreDWG linkage.
- `core/native/CMakeLists.txt` includes native AC1015/AC1018/AC1032 readers, geometry, 3D mesh/topology/mass, BVH, camera, DXF, and MC1032 modules.
- `app/src/main/cpp/firstparty_dwg_unavailable_jni.c` deliberately throws `IOException` for the production `NativeCadEngine.openNative` and `NativeDwg.convertNative` paths. The opt-in preview is not a substitute for a complete DWG document.
- `core/native/src/musacad_dwg_ac1032_object_type.c` implements source-record CRC-gated object type decoding.
- `core/native/src/musacad_mesh_mass.c` implements topology-gated signed mesh volume and centroid calculations.

## Critical distinction
The Android runtime no longer linking LibreDWG does **not** mean full DWG import is working. The host-only LibreDWG oracle remains in the repository. GPL-3.0-or-later headers are present in first-party files; independent implementation and license/provenance are separate audit questions.

## Acceptance gates before enabling normal DWG open
1. Real AC1032 files: verified sections, indexed handles, object records, owner graph, layer/visibility, colors, text, and nested INSERT transforms; report unsupported entities explicitly.
2. Scene parity: no invented or silently omitted geometry; source handle and layer evidence retained, deterministic bounds and counts.
3. DXF export: only allow a complete, validated model; test save/reopen without silently discarding unsupported content.
4. Android integration: enable normal JNI open only after real-file end-to-end tests and negative/corruption tests; preserve fail-closed behavior until then.
5. Test on representative AC1015, AC1018 and AC1032 files, including blocks, text, layers and 3D, across host and Android.
6. Verify third-party linkage and provenance with build artifacts, dependency tree, source headers and legal review; do not infer permissive licensing from the absence of runtime linkage.

## Independent bounded file-page assembly (2026-10-30)
- `core/firstparty/dwg1032/include/musa_dwg1032_section_assembly.h` and `src/musa_dwg1032_section_assembly.c` now accept caller-verified physical file offsets and contiguous logical page positions. Physical file order may differ from logical order; the entire file mapping and destination must be disjoint.
- The routine validates all physical and logical bounds before writing, has no dynamic allocations, and runs in O(page count + copied bytes). The existing section-assembly regression test covers reordered physical pages, a late truncated page, aliasing, gaps, overlaps, and 16,384 reversed single-byte pages.
- **Provenance:** independently authored C11 bounds validation and memory copying using only standard C headers. No GPL/AGPL/LGPL or undocumented third-party code was consulted or imported for this change.
- **Scope limit:** this is a bounded raw/already-decompressed page copier, not an AC1032 section-map decoder, decompressor, page authenticator, or proof of opening real DWG files. The production Android JNI guard remains unchanged.

## Existing 3D capabilities requiring regression coverage
Mesh topology, signed volume/centroid, surface primitives, BVH picking, camera/viewport and UCS are present in source. This is not proof of full ACIS/SAB or AutoCAD feature parity.

## Change policy
Implement on a dedicated feature branch, run CI, inspect real-file results and merge only after explicit review. Do not replace the JNI fail-closed guard with a partial preview.
