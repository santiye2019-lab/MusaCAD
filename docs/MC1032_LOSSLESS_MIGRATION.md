# MC1032 — lossless version migration safety gate

Two **different schemas** share MusaCAD's original `MC1032` signature:
v1 stores four 2D entity kinds; v2 stores layered 3D LINE and POINT. They
are not renamed DWG/AC1032 files and should never be decoded as DWG.

To protect old drawings, there is a single native C migrator:

- `musa_mc1032_upgrade_v1_to_v2`: first-party decode v1 and reconstruct
  original handles and exact XY doubles as v2 3D LINE/POINT with Z=0 on
  default layer 0.
- `musa_mc1032_downgrade_v2_to_v1`: only when there are **no extra layers**
  and **all XYZ geometry has zero Z**. It restores original 2D handles,
  fields and types.
- v1 CIRCLE/ARC cannot yet be represented in v2 and **MUST fail** rather
  than disappear. Likewise custom layers, colored entities and nonzero Z
  cannot be silently discarded by a v2→v1 downgrade.
- Both routines strictly validate CRC, version, bounds, source IDs, and
  target model before publishing any output bytes. Query/written-size and
  private buffers allow safe rejection of insufficient destinations.
- The Android Java layer only calls native `Mc1032Migration` byte transport;
  Windows and iOS use the exact same C ABI. There is no Java parser or
  third-party CAD codec fallback.
- Unit tests assert byte-exact roundtrip, retained nonsequential handles,
  double precision, v1 CIRCLE rejection, v2 nonzero Z/named-layer rejection,
  corrupt input and unchanged output buffer on failure.

This is **not** a lossless universal migration between all CAD entities.
Future rich geometry versions must explicitly support each source entity,
or reject. No AutoCAD 2026 equivalence is claimed. No APK generated.
