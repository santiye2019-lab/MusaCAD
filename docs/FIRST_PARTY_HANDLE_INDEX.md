# MusaCAD Core — first-party 64-bit object handle index

## Purpose

MusaCAD MC1032 rev2 previously scanned every 3D entity to find a handle, including while decoding a file. This led to quadratic duplicate-check work as files grew. The new own-engine C11 hash table maps stable 64-bit handles to ordered scene indices.

- No LibreDWG, ODA, external CAD SDK, platform SDK or Java/Swift CAD algorithm.
- Shared C ABI is core/native/include/musacad_handle_index.h.
- All MC1032 v2 insertion, lookup, deletion, translation and decoder duplicate checks use the SAME index.
- Stable-order delete shifts array entries and updates their index entries; deleted handles are never reissued automatically.
- Portable 64-bit hashing with power-of-two table size >= 2 x maximum entries, tombstone probing and bounded collision loops.
- Tests exercise 50,000 mixed insert/lookup/delete/reinsert/update operations, plus 4096 MC1032 v2 entities with ordered index coherence after deletion.
- Linux, Windows, macOS and iOS ARM64 source portability validated in shared CMake target; Android uses that same C library.

## Limits

This is a handle index, not a complete CAD database. MC1032 revision 2 remains limited to 4096 entities, 64 layers, LINE3D and POINT3D. The generic index supports up to 1,000,000 handles, but larger document storage and streaming formats need independent acceptance. Stable-order deletion remains O(n) due to array shifting, and this optimization is not an AutoCAD parity certification.
