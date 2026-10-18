# MusaCAD own-engine migration | First-party-only decision

Status: IN PROGRESS; **not** an AutoCAD 2026 or AC1032 compatibility certification.
Decision: 2026-10-18. The user requires an independently implemented CAD core;
third-party CAD engines must NOT be part of the final product.

## Existing bridge we must replace (not claim as ours)

- `app/src/main/cpp/dwg_bridge.c` calls `dwg_read_file` from LibreDWG.
- `app/src/main/cpp/CMakeLists.txt` builds/links `redwg`.
- The experimental DWG writer is host-only R2000 / AC1015, not a production AC1032 writer.
- The existing `core/native/` portable C library is our own first-party geometry,
  safety and DXF helper implementation, but it is not a complete DWG engine.
- Legacy Android Java CAD classes remain migration debt.

## Non-negotiable end state

1. One MusaCAD-owned document/entity database with object identity, transforms,
   units, layers/layouts/blocks and forward/back references in portable C/C++.
2. One proprietary-format-independent scene/geometry/command API — never expose
   LibreDWG objects in public ABI or inside document state.
3. Own bounded DWG binary decoder, including AC1015 through AC1032 and
   fail-closed unsupported/proxy/unknown sections.
4. Own native DWG binary encoder with independently reopened, audited object
   semantics, preserving source bytes on failed edits.
5. Own DXF reader/writer and source-to-document adapter; enforce malformed
   inputs, text encoding, handles, no silent geometry loss.
6. One advanced 2D / 3D modeling engine (NURBS, solid B-Rep, mesh, booleans,
   constraints, dimensions, XREF etc.), one edit/undo/redo transaction model.
7. Thin Android JNI, Windows C ABI and iOS Swift/C adapters. No Java/Swift CAD
   interpretation forks.
8. Absolutely no LibreDWG or other CAD SDK code linked or installed in
   shipping binaries after final independent acceptance.

## First concrete implementation, NOT a completed codec

`core/native/include/musacad_document.h` and
`core/native/src/musacad_document.c` add an owned bounded C document model:
LINE, POINT, CIRCLE, ARC; monotonically allocated stable internal handles;
translate/delete/get and atomic rejection of nonfinite coordinates, memory
limits and arithmetic overflow. This is a *small, verifiable kernel slice*,
not a DWG parser, not user-visible 3D CAD, not a substitute for real DWG.

Its CTest tests run against `musacad_core` directly, without including/linking
LibreDWG/ODA. The compatibility bridge remains isolated until replacement.

## Independent acceptance and cutover checklist

- [ ] Full own text/binary DXF read/write with handles, error/loss ledger
- [ ] Own binary AC1015/AC1018/AC1021/AC1024/AC1027/AC1032 header, streams,
      encryption/decompression where applicable, class/object records
- [ ] Own handle/object reference graph and 2D object semantics
- [ ] Layout, blocks, INSERT, dynamic blocks, annotations, XREF, proxy audit
- [ ] NURBS, ACIS/B-Rep, 3D solids/modeling and exact edit semantics
- [ ] Independent AC1032 encoder and exact object-level reopen proof
- [ ] Real AutoCAD 2026 same-input acceptance (51 families separately)
- [ ] Android + Windows + iOS functional and physical acceptance
- [ ] **Remove** `dwg_bridge.c`, `redwg`, pinned LibreDWG build/linkage from
      the shipped code and prove no runtime CAD SDK dependency

No checkbox is implicitly satisfied by code presence, a smoke test, signature
recognition, or successful CI compilation. Unverified support remains unverified.
