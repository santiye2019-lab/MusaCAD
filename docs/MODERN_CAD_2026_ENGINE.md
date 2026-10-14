# MusaCAD modern CAD engine — AutoCAD 2026 function target

**Decision (2026-10-14):** Build a CAD engine with AutoCAD 2026-level 2D/3D
functionality as a comparison baseline, with additional multidisciplinary
engineering AI, takeoff and document-review tools. This is a goal, **not**
a current claim of complete compatibility or superior performance.

## File format versus product version

- AutoCAD **2026** uses the **AutoCAD 2018 DWG file format (AC1032)**.
  The newer application version does not imply a newer DWG magic string.
- A successful **R2000/AC1015** DXF-to-DWG pilot is **not** an AC1032 writer.
  The pinned LibreDWG host pilot explicitly cannot write 2007+ DWG today.
- Older project drawings remain in the acceptance matrix because newer
  features do not guarantee exact legacy object/round-trip semantics.
- Official basis: Autodesk AutoCAD 2026 format compatibility documentation:
  https://help.autodesk.com/cloudhelp/2026/ENU/AutoCAD-Customization/files/GUID-D029509F-A2A0-4A7E-BDD5-FE95E7235C5C.htm

## Modern-first release gates

| Capability | Required evidence to mark complete | Current state |
|---|---|---|
| AC1032 read | Real/pinned AC1032 native decode, no truncation, untouched original, independent object/geometric checks | Public Dynblocks read smoke CI added; independent semantic parity **not established** |
| AC1032 edit/save | Validated writer with 2D/3D/XREF/layout/metadata, original unchanged, saved DWG independently reopened and compared | **Not implemented** |
| 2D editing | MOVE/COPY/TRIM/EXTEND/FILLET/OFFSET/HATCH/DIM/TEXT/BLOCK and undo/redo, edit+save+reopen | Inventory #249: partial; not accepted |
| Advanced DWG object semantics | dynamic block visibility/constraints/actions, INSERT transforms, nested XREF, annotative content | Partial read/diagnostics; modern parity **not accepted** |
| 3D solids | B-Rep/ACIS/SAB, genuine solid boolean/extrude/sweep/revolve/edit, round trip | Polygonal mesh extrusion prototype; read-only triangle area, bounded indexed-edge manifold/winding audit and **topology-gated algebraic signed triangle-mesh volume** added. This does **not** validate true ACIS/B-Rep solids, reliable physical volume, self-intersection, or file round trips; exact kernel **not complete** |
| Viewports and performance | no silent omissions; correct first-frame colors; visible-page tile render; p50/p95 frame times, ANR/OOM on phone | Experimental paging, not enabled in production |
| Engineering AI | evidence-linked discipline detection, full pafta/section/floor coverage, quantities linked to valid current poz book | Partial; case-by-case proof required |

## Native DWG SPLINE geometry (first-paint, not solid kernel)

The native 2D DWG first-paint path now tessellates **actual rational B-spline**
curves through LibreDWG's knot vector, control points, degree and positive
control weights using bounded, homogeneous **de Boor** evaluation
(`app/src/main/cpp/native_nurbs.h`). The old code incorrectly connected
SPLINE fit points or control points with straight lines, misrepresenting
mechanical and architectural curved elements. Supported curves now evaluate
with 12 samples per nonzero knot span, capped at 8,192 points and 4,096
control points to bound memory/CPU cost on Android. Unsupported/corrupted
knot vectors, excessive degrees, invalid weights and fit-only curves that
cannot be reconstructed **fail closed as a partial preview**, instead of
inventing false straight segments. The full DXF vector model remains
authoritative; spline tessellation is a *screen approximation*, not NURBS
model editing or lossless DWG export. Cubic and weighted rational quarter-arc
curves have numerical regression tests in host and Android CI. This **does
not** constitute full dynamic-block or AutoCAD 2026 rendering parity.

## Version-by-version regression

Test AC1015, AC1018, AC1021, AC1024, AC1027 and **AC1032**.
The first test for AC1032 uses the pinned redistributable LibreDWG
`test/test-data/2018/Dynblocks.dwg`. A standalone read smoke checks the exact
six-byte AC1032 signature, successful native host decode and non-truncated
first scene, nonempty DXF geometry, BLOCK/INSERT counts and unchanged SHA-256.

`tests/modern_dwg_acceptance.py` is deliberately strict about *false green*
read status. Unit tests include forged old-version data, truncated/missing
native scene, missing blocks and changed hashes. The existing page-stream
regression independently checks **native indexed versus cached-context
encoding**, but NOT correctness versus Autodesk or an independent CAD reader.

For *full* acceptance, compare against an independently exported baseline:
CAD object handles and class types, exact geometry, model/paper layout, layer
color and visibility (ACI/TrueColor/ByBlock/ByLayer), dimension/annotation,
dynamic-block states, XREF targets, 3D solids and source-file hashes. Unsupported
or unreadable objects must be shown explicitly and block destructive saves.

## AC1032 writer publication barrier (experimental; not an encoder)

`tests/host/com/musa/cad/CadAc1032StagedSave.java` provides only the
**future write path's fail-closed publication protocol**. It is deliberately
**outside Android app sources**: Java NIO hard-link operations require APIs
above MusaCAD's current Android minSdk 24, and this host-only prototype must
not accidentally break app lint or be shipped in an APK. It does
not create valid AC1032 files by itself. A real, licensed/open-source-capable
AC1032 binary encoder and an independent CAD semantic reader still need to
be implemented/integrated before any user-facing DWG Save As is possible.

Required stages, all or nothing:

1. Input original must identify as AC1032. The original DWG and modified DXF
   remain separate, read-only files and get **SHA-256 source snapshots**.
2. Encoder streams to a private temporary file alongside a **new** destination.
   Nothing can overwrite the original DWG, input DXF or an existing target.
3. Candidate must be nonempty and contain the six-byte **AC1032** signature.
   **Header alone does not prove a valid DWG.**
4. A truly **independent** CAD reader must reopen the staged DWG and validate
   actual geometry, handles, blocks/XREF, layouts, layer/color/visibility and
   proxy/ACIS semantics against the edited model. A yes/no test stub is not
   production evidence. Unknown object loss must **reject publication**.
5. Rehash source, edited DXF and staged DWG after verification. Only then
   publish by an exclusive same-directory hard link. The destination must
   not already exist. Filesystems that do not support exclusive hard links
   fail closed rather than silently falling back to an overwrite/move.
6. Stage is removed on success or error; genuine DXF and source DWG remain
   unchanged. Android SAF/cloud destinations require a **separate safe
   publication protocol**. No user-facing AC1032 writer is wired up.

CI currently checks only the barrier's behavior using **synthetic bytes and
stubbed verification**: wrong signature, missing validator, changed source,
changed DXF, changed stage, preexisting output and no accidental overwrite.
These are safety tests, NOT a successful AC1032 conversion/round-trip.

### Strict decoded-DXF semantic parity preflight (new)

`tests/dwg_dxf_semantic_parity.py` compares two independently supplied,
read-only ASCII/UTF-8 DXF exports using an order-independent multiset of
cryptographic hashes of **whole entity group-code streams**. It checks more
than counts/bounding boxes: individual coordinates, ordered polyline
bulges/widths, layer and indexed/TrueColor fields, text, handles,
BLOCK/INSERT/XREF-related references and TABLES/OBJECTS/CLASSES records.
Only number *formatting* is canonicalized (e.g. `10.0` = `1e1`),
without Python's default 28-digit decimal rounding; numeric coordinate
differences at higher precision therefore still fail comparison. The preflight
also fingerprints selected **DXF HEADER engineering state**: `$INSUNITS`,
`$MEASUREMENT`, `$LUNITS`, `$DIMSCALE`, `$DIMLFAC`,
`$INSBASE`, `$UCSORG`, `$CLAYER`, codepage and related settings.
A source/output change from millimetres to metres, a lost dimension scale,
or an altered base point must fail even if every geometric entity and layer
count matches. If neither export carries a particular header variable,
this DXF-only preflight cannot independently recover the omitted original
DWG setting. Independent binary verification remains required.
Missing/extra entities, changed entity attributes, truncated or malformed
DXF, unexpected Unicode, proxy/ACIS entities and ACDSDATA all reject
certification; coordinates and customer text are never printed.

Example, **AFTER** a real future AC1032 encoder and **separate** read-back
converter have produced their DXF files:

```bash
python3 tests/dwg_dxf_semantic_parity.py approved-edit.dxf \
    independently-reopened-ac1032.dxf --report parity.json
```

The DXF acceptance preflight now also has two **structural** gates.
It compares ordered group streams in BLOCKS, TABLES, OBJECTS and CLASSES;
ENTITIES is order-independent only when it contains no associated
POLYLINE/VERTEX/SEQEND or INSERT/ATTRIB sequences. This detects swapped
BLOCK members, wrongly assigned attributes or vertices and changed TABLE
ownership even if every entity hash, handle and count still matches. Block
and table open/close markers are checked for missing, nested and orphaned
groups. Deliberately rejecting harmless reordering in those parent/child
structures is safer than incorrectly certifying a DWG Save As round trip.

**External project dependencies cannot be certified by equal DXF records.**
A matching XREF path does not prove the linked DWG still has identical
geometry, units or layouts; IMAGE/OLE/underlay links can also change behind
their identifiers. The preflight now fails closed on XREF block flags,
external raster/underlay entities and known proprietary AEC/AECC/ACAD/
ACDB/ACSH object classes pending an independent dependency manifest,
file hashes and semantic validation. These checks are about preventing
false-green acceptance, **not** adding XREF/ACIS writing functionality.

This is only a **DXF-level parity preflight**, NOT proof of an independent
DWG decoder, true AC1032 file validity, unchanged dynamic-block state, ACIS
or B-Rep persistence. If both DXFs derive from the *same lossy converter*,
matching output can conceal omitted DWG-only information. An independent
DWG engine and real AutoCAD/ODA semantic acceptance remain required before
this can be wired into the host-only `IndependentReopenVerifier` gate,
let alone enabled as a save action on Android.


## Guardrails

1. Keep working `main`, Android APK and Cloudflare production intact.
2. AC1032 read **cannot** be advertised as AC1032 edit/save.
3. No silent DXF approximation for unsupported ACIS, XREF or proxy classes.
4. No proprietary user/municipal drawings in GitHub CI or server logs.
5. CI PASS precedes, but does not replace, physical Android field acceptance.
6. Baseline against AutoCAD 2026 *functionality*; independently assess whether
   measured performance/engineering accuracy exceeds it.

Tracked by [#249](https://github.com/santiye2019-lab/MusaCAD/issues/249).
Experimental implementation remains in draft
[#269](https://github.com/santiye2019-lab/MusaCAD/pull/269).

## Inventory #08 — Native preview entity-type omission disclosure (draft)

The C native DWG first-paint renderer is **not** the full DXF editing engine.
It has direct dispatcher cases for only a subset of DWG entities; even
objects entering such a case may still fail because of incomplete payloads,
external references, scene limits, unresolved dynamic blocks, or font/asset
substitution. `NativeCadEngine.Stats.unsupportedEntityCount` historically
means **all entities other than six core geometric types**, NOT a count of
truly unrendered native primitives. Do not present that counter as an
accurate missing-entity count.

The **read-only host audit** `tests/native_preview_type_audit.py`, integrated
into `tests/private_dwg_read_audit.py`, now classifies the decoded DXF
`ENTITIES` and `BLOCKS` type inventories into:

- `native_preview_unsupported_entity_types` and
  `native_preview_unsupported_entity_count`: known classes with no native
  preview dispatcher (3DSOLID, BODY, REGION, ACAD_PROXY_ENTITY, etc.) or
  other/unrecognized types, aggregated into a privacy-safe bucket.
- `native_preview_conditional_entity_types`: classes for which a native
  dispatcher exists; this is **not** evidence that each instance was drawn.
- `native_preview_structural_record_count`: subordinate VERTEX/SEQEND DXF
  records that are not separate rendered primitives.
- `native_preview_completeness`: either
  `unsupported_types_detected` or `type_dispatch_only_unverified`.
  **Neither value means complete source-DWG fidelity.**

The type-list synchronizes automatically with actual native C dispatch cases
in `test_native_preview_type_audit.py`; tests use synthetic, public DXF,
never real customer coordinates, names, layers, handles or project text.
The Android normal-open quick-preview status now explicitly warns that
colors AND entity-type completeness are not yet verified.

Limits: DXF conversion may itself omit original DWG-only classes; BLOCK
definitions are counted without resolving per-instance INSERT expansion.
A type count does not validate visibility, handles, dimensions, object
metadata, colors, UI pixels or source-vs-export parity. The next gate
requires original DWG handles/classes, full first-frame render evidence and
independent CAD semantic comparison before #08 can be accepted.

**Progress checkpoint:** AutoCAD 2026 functionality target remains an
approximate **35%** developed, **65%** remaining; this change improves
transparency and test coverage, but does not close any of the 18 final
physical-device/semantic acceptance requirements. Main/release APK remain
unchanged.

## Inventory #08 — Original-DWG identity and layer parity gate (second increment)

The first increment catalogued supported/unsupported native C dispatch types
from an **exported** DXF; a successful export is NOT proof that all source DWG
objects survived. This increment adds a bounded read-only object identity
audit to `tests/dwg_entity_identity_audit.py` and integrates it into the
existing `private_dwg_read_audit.py`.

- Records each decoded `ENTITIES`/`BLOCKS` object's DXF handle (group 5),
  layer (group 8), type and model/block ownership. Hex handles are canonicalized
  (`00A` equals `A`), invalid/missing/duplicate handles and missing/duplicate
  layer fields are counted. Report fields are counts **only**; it does not
  output handles, coordinates or per-object layer labels.
- Optionally compare against a separately obtained, local **source DWG**
  object inventory. The external JSON manifest must be named
  `<source-SHA256>.json`, include `source_sha256` exactly matching the
  unchanged original DWG and `source_kind: "independent_dwg_reader"`, plus
  `records` containing `handle`, `space` (`model`/`block`), `type`
  (DXF-level canonical type), and `layer`. Caller MUST confirm independent
  reader provenance: a label by itself cannot cryptographically attest it.
- Missing/extra handles, changed type, changed layer or changed model/block
  ownership fail the optional parity check. Result is either
  `identity_metadata_parity_only` or `identity_metadata_mismatch` —
  **never** "CAD fully accurate". Reports output mismatch counts only,
  not the source's sensitive per-object identifiers or layer names.
- Invalid/mismatched/oversized manifests fail closed; only an explicit
  `--independent-manifest-dir <local-directory>` opt-in loads such data.
  Neither the source DWG nor reference manifest is uploaded.
- Synthetic tests cover duplicate/invalid handles, identity drift despite
  equal counts, model/block changes, independent-SHA gate, privacy and
  strict reference input validation. Both Android and DWG CI run them.

Example, all paths stay local to the host machine:

```shell
python3 tests/private_dwg_read_audit.py \
  --converter build/musacad_dwg_convert \
  --independent-manifest-dir /private/independent-cad-manifests \
  --report /private/dwg-object-audit.json /private/project.dwg
```

**Remaining open:** Independent reader *actual source* data for engineering
files, link-chain/XREF and hidden-layout parity, semantic geometry/color
comparisons, and Android first-frame/large-project acceptance. Even a perfect
identity match does not certify ACIS solids, dynamic block actions, geometry,
unit scales or written DWG roundtrip. **0/18 final acceptance.**

**Overall progression:** 35% -> 35% estimated developed, 65% remaining
(AutoCAD 2026 target, ±10 pp); audit infrastructure does not count as full
native drawing functionality. Draft PR only; main/production APK untouched.
