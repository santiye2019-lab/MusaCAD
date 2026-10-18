# MC1032 revision 2 — layered 3D extension (original MusaCAD format)

MC1032 is an original binary format created for MusaCAD. It is **not** DWG
and **not** Autodesk AC1032 with a changed filename/header. All file and
geometry semantics are owned by the single portable C11 MusaCAD core.

## Backwards compatibility

- Original MC1032 **revision 1** (32-byte header, 72-byte 2D records) remains
  byte-for-byte unchanged, including its ABI, golden fixture, tests and JNI.
- Revision 2 uses the SAME `MC1032` signature but schema byte `2` rather
  than `1`. Its C API is **separate** to avoid silently discarding v1
  CIRCLE/ARC geometry or changing a legacy file's meaning.
- Revision 2 currently supports 3D LINE and POINT, layer names, RGBA colors
  and entity-to-layer references. It does **not** claim CIRCLE, ARC, solids,
  NURBS, dimensions, dynamic blocks or unbounded file sizes.
- A v1 decoder MUST reject rev2 as unsupported, not reinterpret or truncate
  it. A v2 decoder MUST reject rev1. Future version negotiation will route
  to the correct decoder without silent downgrade.

## Binary layout (schema 2)

All integer fields use little-endian; all coordinates use IEEE-754 binary64
little-endian. C memory structs are **never** serialized by `memcpy`.

Header: **48 bytes**

| Offset | Type | Meaning |
|---|---|---|
| 0–5 | ASCII | `MC1032` |
| 6 | u8 | Version 2 |
| 7 | u8 | Flags zero |
| 8–11 | u32 | Endian marker 0x01020304 |
| 12–15 | u32 | Header size 48 |
| 16–19 | u32 | Layer record size 64 |
| 20–23 | u32 | Entity record size 80 |
| 24–27 | u32 | Layer count 1–64 |
| 28–31 | u32 | Entity count 0–4096 |
| 32–35 | u32 | Payload length |
| 36–39 | u32 | CRC32 of all layer/entity records |
| 40–43 | u32 | CRC32 of header bytes 0–39 |
| 44–47 | u32 | Reserved zero |

Layer record: **64 bytes**

| Offset | Type | Meaning |
|---|---|---|
| 0–3 | u32 | Layer ID = zero-based record index |
| 4–7 | u32 | Packed RGBA (0xRRGGBBAA) |
| 8–11 | u32 | Visibility 1 (other values unsupported yet) |
| 12–15 | u32 | Name length, 1–48 |
| 16–63 | ASCII | Layer name, zero-padded |

Layer names consist of A–Z, a–z, digits, underscore or dash; must be unique.
Layer 0 is exactly `0`, opaque white. The C ABI can add layers in order.

Entity record: **80 bytes**

| Offset | Type | Meaning |
|---|---|---|
| 0–7 | u64 | Unique, stable nonzero entity handle |
| 8–11 | u32 | Kind: LINE3D=1, POINT3D=2 |
| 12–15 | u32 | Layer ID: must reference an existing layer |
| 16–63 | 6 × f64 | x1, y1, z1, x2, y2, z2 |
| 64–79 | bytes | Zero-reserved for future schema |

A POINT3D has x2=y2=z2=0; LINE3D defines both XYZ endpoints. Unsupported
entities and nonfinite numbers reject the ENTIRE file. The exact total
length is `48+64*layerCount+80*entityCount`, with no trailing bytes.
CRC32 is CRC-32/ISO-HDLC (reversed polynomial EDB88320, init/final XOR
FFFFFFFF). CRC is an accidental-corruption check, **not cryptographic
authentication**, and does not prevent a malicious party from recomputing
checksums.

## Portability and source safety

- The exact **same** C11 source builds via `musacad_core` on Android,
  Windows, Linux and macOS. iOS ARM64 object cross-compilation is tested.
- The Android `Mc1032V2Native` JNI transports arrays of native 64-bit words:
  handle, kind, layer and six double bit patterns. Java owns platform
  file access but does not decode CAD geometry or implement the format.
- Encoding stages bytes privately; short buffers and unsupported objects
  leave caller output unchanged. Decoding allocates a complete temporary
  scene and only publishes it after structural, CRC and identity validation.
- Handle identities never recycle; translate/delete operate on native scene,
  not on a second platform CAD engine.
- Android `writeNew` creates a unique `.mcad` file and fsyncs; it never
  overwrites the source. Unit tests cover actual file reopen and rejected
  malformed inputs.

## Roadmap and honest status

Revision 2 is NOT a complete CAD engine, DWG import/export, arbitrary layer
property model, rich 3D B-Rep, ACIS modeler, XREF/block/layout renderer, or
device benchmark. The 4096-entity cap and linear handle lookups are deliberate
until a tested high-volume indexed object model replaces them.

**Next steps:** validated handle index, lossless v1/v2 migration, broader
entity schemas, write-ahead change safety, complex geometry, blocks/XREF and
real DWG interoperability through the independent CAD core. Full independent
AutoCAD 2026 equivalence remains 0/51; no APK is produced.
