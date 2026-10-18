# MC1032 — MusaCAD's own portable binary drawing format (revision 1)

**Name:** MC1032. **Signature:** ASCII `MC1032`. **Native file extension:** `.mcad`.

**Origin:** Written entirely by MusaCAD, independently of LibreDWG, ODA,
Autodesk DWG/AC1032 or another CAD engine.

**Important:** The six bytes `AC1032` are an Autodesk DWG version signature;
`MC1032` is our own signature and binary layout, NOT an alias for DWG.
Converting an AC1032 DWG into MC1032 will require a separately verified
first-party DWG importer. Renaming file extensions or header bytes is
invalid and explicitly rejected by this codec.

## Portable on-disk specification

All multibyte integers and IEEE-754 binary64 coordinates are little-endian
**regardless of CPU endianness**. No native C struct is written directly.
All offsets are zero-based byte offsets. Unknown version, flags, record kind,
header sizes, or unsupported geometry fails closed.

Header: **exactly 32 bytes**.

| Bytes | Encoding | Meaning |
|---|---|---|
| 0–5 | 6 ASCII bytes | `MC1032` |
| 6 | u8 | Schema version `1` |
| 7 | u8 | Flags `0` (reserved, MUST be zero) |
| 8–11 | u32 LE | Endianness sentinel `0x01020304` |
| 12–15 | u32 LE | Header size `32` |
| 16–19 | u32 LE | Record size `72` |
| 20–23 | u32 LE | Number of entities, max `4096` in revision 1 |
| 24–27 | u32 LE | Payload CRC32 over bytes `32..`end |
| 28–31 | u32 LE | Header CRC32 over bytes `0..27` |

Record: **exactly 72 bytes** per entity.

| Bytes within record | Encoding | Meaning |
|---|---|---|
| 0–7 | u64 LE | Stable nonzero MusaCAD entity handle |
| 8–11 | u32 LE | Kind: LINE `1`, CIRCLE `2`, ARC `3`, POINT `4` |
| 12–15 | u32 LE | Flags `0` (must be zero) |
| 16–23 | f64 LE | `x1` |
| 24–31 | f64 LE | `y1` |
| 32–39 | f64 LE | `x2` (LINE only; zero otherwise) |
| 40–47 | f64 LE | `y2` (LINE only; zero otherwise) |
| 48–55 | f64 LE | Radius (positive on CIRCLE/ARC, zero otherwise) |
| 56–63 | f64 LE | ARC start angle in degrees (zero otherwise) |
| 64–71 | f64 LE | ARC end angle in degrees (zero otherwise) |

Total length = `32 + entity_count * 72`, no trailing data. CRC32 uses
CRC-32/ISO-HDLC (polynomial `0xEDB88320`, reflected, init/final XOR
`0xFFFFFFFF`). Empty document is valid. Golden empty byte sequence:

```text
4D 43 31 30 33 32 01 00 04 03 02 01 20 00 00 00
48 00 00 00 00 00 00 00 00 00 00 00 C3 11 45 B2
```

## API, supported features and integrity contract

- `core/native/include/musacad_mc1032.h` exposes a **single** versioned
  C binary encoder and decoder usable by Android NDK, Windows C ABI,
  Linux/macOS C and iOS Swift/C. No third-party CAD SDK is linked.
- Native serializer validates supported geometry and prepares output in
  private memory. It publishes encoded bytes **only on complete success**;
  insufficient caller capacity leaves the old buffer unchanged.
- Native decoder validates sizes, flags, revision, header+payload CRC,
  shape semantics, finite coordinate fields, duplicate handles and
  object references. Returns a full new document or fails with no model.
- Android `Mc1032Native` JNI is a thin transport adapter, using
  nine 64-bit words per entity; its `writeNew` produces a uniquely named
  `.mcad` file after fsync. It never overwrites the original.
- Unknown future types or fields are **never silently discarded**.

## Explicit limitations (NOT hidden as completed features)

Revision 1 is a **small but operational** native binary document format for
LINE/CIRCLE/ARC/POINT in a default 2D WCS plane only, with no layer table,
color, blocks, layouts, XREF, annotations, OCS/UCS, ACIS/B-Rep, 3D solids,
hatches, meshes, scene graph or transactions. Such content cannot yet be
encoded and must be rejected. The max entity cap is intentionally conservative
until an indexed (rather than O(n²)) handle store is validated.

An AC1032 DWG is **not** an MC1032 file. Genuine AutoCAD 2026 DWG interoperability
still needs a first-party binary DWG reader/writer and independent goldens.
Cross-platform CTest / iOS object compile proves portability of the shared
codec source, not physical Windows/iPhone application acceptance.

AutoCAD 2026 full-feature independent certification remains **0/51**.
No APK or production build is implied by these tests.
