# MusaCAD — Core inventory, actual integration and missing features (2026-10-22)

> **Snapshot, NOT an equivalence certificate.** Based on default `main` at
> `46813862942159c818974111271fcd3d712c3428`, all available pull request
> history, `core/native` C/CMake/test inventory, Android JNI, fixed milestone
> gates, and 51-family acceptance ledger. Audit PR is documentation only.
> Do not increment any progress percentages merely because this was written.

## Decisions / baseline

- Target **AutoCAD 2027 functional parity** (then MusaCAD-only improvements).
- One owned C/C++ CAD engine in `core/native`, reusable Android/Windows/iOS.
  **Never silently restore LibreDWG to any production or app runtime.**
- Keep externally compatible DWG `AC1032` distinct from MusaCAD-only
  `MC1032` `.mcad`, which is **not an AutoCAD DWG**.
- No APK, desktop installer, App Store release or cloud deployment until the
  owner explicitly approves the consolidated release.

**Current measurements (three DIFFERENT denominators):**
- Core acceptance: **10/25 = 40%** (fixed gates).
- Independently evidenced R2004/AC1018 2D ingest milestones:
  **11/16 = 68.75%**; smaller evidence index **92/128 = 71.875%**,
  which is NOT the same acceptance measure.
- **2027 full independently certified capability families: 0/51**.
  Historical 2026 capability audit is **24 partial, 6 blocked, 21 unassessed**.
  These labels document existing work, not 2027 equivalence.

## Existing code — do not rebuild as a new capability

| Area | Already exists in repository | Boundaries and evidence |
| --- | --- | --- |
| Shared native CAD architecture | One C11 `musacad_core` CMake target, public C ABI, immutable/failure-atomic contract, native adapters | `core/native/CMakeLists.txt`, `musacad_core.h`; [#290](../pull/290), [#346](../pull/346), [#358](../pull/358), [#378](../pull/378) |
| DWG signatures and legacy R2000 | `AC1009`–`AC1032` signature detection; AC1015 section, CRC, handle map, primitive data, common record and bounded reference codecs | `musacad_dwg_signature.c`, `musacad_dwg_ac1015*.c`; [#381](../pull/381)–[#393](../pull/393) are PARTIAL parsers |
| R2004 actual binary decode | Protected header/page map, LZ, section assembly, indexed CRC object map, LINE, ARC, CIRCLE, LWPOLYLINE OCS/WCS+width/bulge | `musacad_dwg_ac1018*.c`; [#394](../pull/394)–[#418](../pull/418); supported subset, not arbitrary AcDb classes |
| Original object references | Indexed source H/CRC, reactors, selected owner and layer target checks; LAYER_CONTROL, ASCII layer names and raw CMC palettes; entity–layer membership | [#416](../pull/416), [#426](../pull/426), [#433](../pull/433)–[#447](../pull/447); owner/class/color display semantics still incomplete |
| Curved LW geometric representation | Signed bulge analytic bounds, width-correct edge curves and tangents, bevel/miter candidates, fan and 3D strip triangulation, source snapshots | [#452](../pull/452)–[#470](../pull/470); this is NOT a complete DWG stroke, block/layout renderer or B-Rep |
| 2D/DXF native | Document model, read-only parser subset, DXF group/section/handle/stream rules, safe writer+reopen supported subset, safe source SHA-256; 2D affine/color/bulge helpers | `musacad_dxf_*.c`, `musacad_document.c`; [#292](../pull/292), [#296](../pull/296), [#349](../pull/349)–[#361](../pull/361); not lossless full DXF |
| Native proprietary file | `MC1032` original binary v1/v2, bounded layer-aware v2 3D LINE/POINT, checked migration and rejection if old entity kinds cannot map | [#362](../pull/362)–[#368](../pull/368); proprietary `.mcad` only; v1 CIRCLE/ARC cannot be losslessly migrated to v2 |
| Android-side 2D editing | MOVE/COPY/ROTATE/SCALE/MIRROR/OFFSET/STRETCH, BREAK, ARC/POLYLINE operations, text/annotation/view and DXF saving in partial Android implementation | [#275](../pull/275)–[#280](../pull/280), [#329](../pull/329)–[#335](../pull/335); **NOT all migrated** to the one C engine |
| Shared 3D computational foundations | 3D affine & UCS, WCS/screen projection, mesh volume/surface area/centroid, topology, ray picking, BVH and source-preserving checks | `musacad_mesh_*.c`, `musacad_ucs.c`, `musacad_gizmo.c`; [#299](../pull/299)–[#314](../pull/314), [#369](../pull/369)–[#375](../pull/375); NOT exact ACIS/B-Rep |
| Android-side 3D experimental editing | Bounded triangle mesh / surface rendering, 3D coordinate editing, polygonal EXTRUDE/REVOLVE/SWEEP with guarded DXF | [#283](../pull/283), [#286](../pull/286), [#287](../pull/287), [#337](../pull/337); NOT AutoCAD solid-command parity |
| Quality / source integrity | CRC, record-range guards, incomplete/unsupported entity reporting, regression test scripts, 2027 family ledger | [#317](../pull/317), [#347](../pull/347), [#429](../pull/429), [#469](../pull/469); no independent reference certificate |

At snapshot: **60** first-party `core/native/src/*.c` units,
**64** headers in `core/native/include`, **77** native C tests in
`core/native/tests`, and **22** Android `app/src/main/cpp/*.c` units.
Presence ≠ complete UI integration, real DWG parity, or complete 51 families.

### Important NOT-completed Android runtime bridge

`app/src/main/cpp/CMakeLists.txt` intentionally builds
`firstparty_dwg_unavailable_jni.c` on Android, **not** the old
LibreDWG-backed host `dwg_bridge.c`. Every Android
`NativeCadEngine.openNative` and `exportDxfNative` JNI invocation
currently returns an explicit unsupported exception. That means the
many tested original R2000/R2004 C parsers **have not been wired up
to a user-operational Android DWG open path**. The Android app's
native DWG open is NOT completed. Do not claim an installable APK opens DWG
through the new first-party runtime. The old decoder continues to exist
**host-only** for independent regression/oracle use, never as an Android
runtime dependency. This is the first integration blocker to resolve
once decoder/scene fidelity is sufficiently safe.

## DWG/MC1032 version-compatibility evidence (NOT version marketing)

| Signature | Source version family | Actual first-party capacity on main |
| --- | --- | --- |
| `AC1009` | R12 | Six-byte recognition only; no verified full reader/writer |
| `AC1012` | R13 | Six-byte recognition only; no verified full reader/writer |
| `AC1014` | R14 | Six-byte recognition only; no verified full reader/writer |
| `AC1015` | R2000 | Independently bounded section/object/primitive **partial read**, not whole-drawing write/parity |
| `AC1018` | R2004 | Original binary page/index and selected 2D primitive **partial read**, no full graph or writer |
| `AC1021` | R2007 | Six-byte recognition only in MusaCAD-owned engine |
| `AC1024` | R2010 | Six-byte recognition only in MusaCAD-owned engine |
| `AC1027` | R2013 | Six-byte recognition only in MusaCAD-owned engine |
| `AC1032` | R2018-family (AutoCAD 2018–2027) | Six-byte recognition and **NOT full binary decode or binary write** in first-party engine |
| `MC1032` | Original MusaCAD `.mcad` v1/v2 | Portable own limited geometric read/write and migration; never a substitute for `AC1032` |

**An app version is not a DWG signature.** AutoCAD 2027 functional capability
coverage and historical DWG versions require separate tests. Do not claim
support across all AutoCAD releases merely because header detection works.

## Cross-platform acceptance (one source ≠ three finished apps)

| Environment | Verified proof | Still required |
| --- | --- | --- |
| Android | Same `musacad_core` linked in Android NDK and Android quality gate; `arm64-v8a`, `x86_64`, configured minSDK 24 | Working original binary DWG scene JNI, native editor/render bridge, actual phone open/edit/save verification |
| Windows | Host shared C11 CMake compile and native unit tests via `windows-latest` | Independent Windows desktop UI and physical desktop file/edit/render/print acceptance |
| iOS | macOS compiler + all core C units object-compiled for iPhoneOS arm64 (archive contract) | Swift/Objective-C adapter, UI, iOS application, actual iPhone/iPad source/edit/render acceptance |
| Linux / macOS | C core CI / CTest host evidence | Not additional shipping application parity |

At baseline `docs/UNIFIED_CAD_ENGINE_INVENTORY.json`: **110 Java candidate
modules**: **63 need core audit, 7 partially migrated, 40 platform-adapter
candidates**, **0** is automatically verified as a wholly migrated feature.
No `ios/`, `windows/` or `desktop/` shipping UI trees exist in
the indexed `main` tree. The cross-platform test matrix is not three apps.

## Missing work — prioritize REAL missing capability, not another wrapper

### P0-A (highest) Independent user-operational modern DWG reader

**Scope**: A complete streaming first-party `AC1032` decoder and validated
document/scene pipeline; evolve existing R2000/R2004 units, not rewrite them.
Support real handles, blocks, layouts, layers/true color, annotations, dynamic
entities, XREF identities, unknown/proxy data and source preservation.
The original header parser alone is insufficient. Keep unsupported inputs
fail-closed until lossless read and reliable render/identity checks exist.
**Required tests**: genuine independent original DWG fixture corpus across
R12/R13/R14, R2000, R2004, R2007, R2010, R2013 and R2018-family, with
per-H counts, exact geometry/properties, protected source SHA and CRC/error
cases. Android first-party session must become operational, not a stub.

### P0-B Lossless modern DWG write / Save As

Actual native binary `AC1032` (plus demanded previous supported versions)
serializer, indexed handles, preserved ownership/objects, dynamic block, XREF,
layout, layers/colors/annotations/proxy, independent AutoCAD 2027 reopen,
second-reader semantic validation and source-preserving failure rollback.
Current isolated host-only LibreDWG **R2000 experimental pilot is not this**.
No silent file conversion on missing feature coverage.

### P0-C Layer/owner/class BLOCK/INSERT/XREF and render bridge

Finish real typed owner/linetype/plotstyle/material targets, block definition
records, nested INSERT/MINSERT, attribute graphs, dynamic blocks, circular
XREF guard, native first-frame layer/RGB/visibility and viewport integration.
Do not confuse raw CMC RGB with calculated final ByLayer/ByBlock display color.

### P0-D External real-file + large project + multi-platform integration

Independently produced multi-entity files; per-entity orientation/width/
bulge/annotation/block/layout/material; compare original DWG -> native
semantics -> edit -> independently reopened DWG, and cross-platform same
goldens. Profile cold open/first frame, p50/p95 zoom/pan/selection, bounded
peak memory, no old-to-new color flash; test Android handset, Windows PC,
iPhone and iPad. Unknown losses are not 'pass'.

### P1 Remaining 2D command/annotation/drawing model parity

Port Android-only CAD math to single `musacad_core`; implement full entity
editing, associative hatch and dimension, annotation scale, rich text SHX,
parametric constraints, dynamic blocks, accurate model/paper viewports,
PLOT/print to scale, PDFIMPORT, raster and Sheet Set workflows.
Track family IDs `draw_07..draw_14`, `annotation_15..19`,
`blocks_20..23`, `documents_24..28`; independent same-input tests.

### P2 Exact 3D solid / ACIS-B-Rep parity

Full analytic topology and robust surfaces, read/preserve/write SAT/SAB/3DSOLID,
EXTRUDE/REVOLVE/SWEEP/LOFT, Boolean union/subtract/intersect, SOLIDEDIT,
actual 3D UCS, material/render workflows, verified mesh-to-exact solid.
A watertight triangulated display shell is **not** a universal solid model.
Track `3d_29..36`.

### P3 Automation, specialization and 2027 deltas

LISP/Visual LISP API, platform-safe plugin interfaces, action macros,
DWG compare/TRACE, remote cooperation, GIS, architecture/mechanical/electrical/
MEP/Plant 3D/Map/Raster Design tools. Capture 2027 Geometry Cleanup,
Checkout, Connected References, improved Assistant and Forma Data Management
as 2027-specific scenarios mapped to the existing fixed 51 families.
No Autodesk proprietary code copied; functional behavior compared independently.

## Existing audit blockers should NOT be recreated as new progress

- The fixed AC1018 16-milestone list has **five PENDING groups**:
  `typed_layer_owner_targets`, `block_insert_reference_graph`,
  `real_dwg_full_batch_parity`, `android_native_scene_render`,
  `large_file_visual_correctness`. Detailed subtests already exist in
  `docs/MUSACAD_AC1018_2D_READ_DETAIL.json`.
- Re-check existing native `musa_dwg_ac1018_scene_read_verified_entity_layer`,
  `musa_dwg_ac1018_scene_read_entity_layer_batch`, palette,
  bounded geometry, source snapshot and JNI adapters BEFORE adding parallel
  APIs that merely repackage points/line segments.
- `PR #471` at this audit time is **open**. It adds original segment
  access to the already-merged source snapshot and has no new independent
  DWG entity type or completion gate; evaluate separately for maintainability,
  not as an additional 51-family achievement.
- Authoritative implementation completion count must only change when
  the exact missing capability passes end-to-end acceptance with code,
  negative cases and multi-platform evidence.

## Living verification references

- `docs/MUSACAD_CORE_COMPLETION_GATES.json`
- `docs/MUSACAD_AC1018_2D_INGEST_PROGRESS.json`
- `docs/MUSACAD_AC1018_2D_READ_DETAIL.json`
- `docs/UNIFIED_CAD_ENGINE_INVENTORY.json`
- `docs/AUTOCAD_2027_TARGET.json`
- `docs/AUTOCAD_2026_PARITY_MATRIX.json` (historical status)
- `app/src/main/cpp/CMakeLists.txt`
- `app/src/main/cpp/firstparty_dwg_unavailable_jni.c`
- `.github/workflows/cad-crossplatform-core.yml`

**Next code PR should target P0-A usable native DWG record-to-scene
integration or an actual AC1032 decoder slice with a new externally
validated object category. Do NOT count another wrapper as a new family.
Never build an APK without owner approval.**
