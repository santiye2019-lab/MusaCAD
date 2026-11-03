# Scratch-backed lazy object-window reads — 2026-11-02

- Scope: original MusaCAD, `core/firstparty/dwg1032/`; no changes to `MusaCAD2/`, `main`, existing 3D or production Android/JNI.
- Origin: independently authored MIT C11 using basic bounds arithmetic, binary-search index lookup and caller-owned scratch buffers. No GPL/AGPL/LGPL, LibreDWG, historical MusaCAD CAD implementation or unverified-origin source was copied, adapted or linked.
- Contract: page directory and object index must already be verified, refer to the same RAW logical section and remain immutable. Caller scratch is fully disjoint from source, index, pages, destination and length output. Failed reads preserve destination and candidate length; scratch contents are disposable.
- Performance intent: for cross-page object windows, copy touched pages once into scratch, then publish the complete window. This avoids the existing multi-page reader's two traversal passes. It adds caller-provided scratch storage and a final contiguous copy; **no measured speedup is claimed**.
- Regression: synthetic reversed-physical pages, cross-page candidate, zero-byte reads, 20,000-page differential windows, invalid handle/offset, insufficient scratch, alias rejection and late-page corruption rollback. The CTest target is `firstparty_dwg1032_scratch_object_windows_20k`. Verify CI on the exact commit; no unrun test is considered passing.
- Limitation: this only reads already-verified, uncompressed RAW page ranges. AC1032 page-map authentication, compressed-page recovery, real-file licensed fixtures, DWG entity TEXT/MTEXT/block/layer decoding and Android full-fidelity rendering are **not established**. Do not activate it as a production DWG importer.

## Scratch-backed modular-short payload windows (2026-11-02)

- Added independent C11 handle and ordinal APIs for bounded, already-verified
  RAW object candidates with an MS-prefixed payload. Prefix reads are bounded,
  including a four-byte prefix split across page boundaries; payload publication
  uses the existing transactional scratch-backed window reader.
- The caller provides scratch space and a maximum payload length. Invalid MS
  prefixes, short buffers, insufficient scratch, invalid handles, overlapping
  public outputs, source/metadata aliasing and late corrupt pages reject without
  modifying destination or payload length. Scratch is disposable.
- Synthetic tests exercise cross-page reads, extended prefixes, zero-byte end
  reads, rollback and 20,000 reverse-physical object candidates against the
  existing bounded reader. CI is authoritative; no real-world speedup or DWG
  import completeness is claimed.
- Independently authored MIT C11 from bounded arithmetic and already-owned
  first-party primitives. No GPL/AGPL/LGPL, LibreDWG, historical CAD source or
  unknown-origin implementation was copied, adapted or linked.
- This does **not** discover/authenticate/decompress AC1032 pages, parse actual
  DWG entities, or enable production Android/JNI imports. The existing 3D
  implementation, `MusaCAD2/` and `main` are unchanged.
