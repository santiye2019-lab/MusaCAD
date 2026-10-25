# MC1032 original binary DWG 2D opening: evidence-based acceptance gates

**Status (2026-10-25): NOT COMPLETE.** This is about the FIRST-PARTY
original AC1032/R2018 *binary DWG* reader, **not** the already implemented
DXF path and **not** the AutoCAD product-version equivalence percentage.

This is a strict engineering release gate, not an estimate. No source-level
class ID, successful physical DWG file open, green compilation, indexed
objects, or partial read-only 2D preview is sufficient to call a drawing
fully open.

| Area | Current evidence in first-party R2018 core | Exact remaining acceptance |
|---|---|---|
| TEXT + MTEXT | CRC/H-checked original raw type IDs **1 and 44** can now be counted; no complete native R2018 content/layout decoder or v6 text emission. | Original binary files with real TEXT and MTEXT; independently verified Unicode, rotation, justification, height, oblique/width; formatted MTEXT and columns; font/style/substitution behavior; repeatable visual comparison and roundtrip tests. |
| BLOCK / INSERT / MINSERT / attributes | Raw IDs **4/5, 7/8, 2/3** can now be counted, not rendered. Unexpanded INSERT references and block definition records are NOT equivalent to visible entities. | Authenticated BLOCK definitions + ENDBLK, ownership and INSERT handles; transform (translate/rotate/scale/mirror) including nested blocks, arrays, attributes/visibility, cycle/invalid-reference handling, model/paper-space scope and XREF policy; real DWG viewport comparisons. |
| Layer visibility | Original LAYER identity/name and narrow raw flags/CMC evidence are verified. Source raw **0xE0** default-layer flag example makes naïve on-bit interpretation unsafe. Source invisible-entity bits are only an entity-specific signal. | Original-file cases for ON/OFF, frozen/thawed, locked, viewport-frozen, XREF/viewport overrides, nested block layer 0 inheritance, ownership/model-space filtering; original rendered truth oracle; no disappearing geometry on unsupported flags. |
| Colors | Some original direct ACI and unambiguous 24-bit RGB are source-decoded. **BYLAYER / BYBLOCK, transparency, layer display color and viewport appearance remain unresolved.** Provisional ACI7 is NOT the source final appearance. | Original-file tests for ACI, truecolor, BYLAYER/BYBLOCK inheritance through nested INSERT, entity visibility, transparency, viewport layer overrides, background-dependent ACI appearance and missing color reference; cross-platform exact or tolerance-specified visual comparisons. |

## What passes today

- AC1032 page/catalog/H-index and original object CRC validation under bounded input.
- Read-only preview can admit some zero-elevation planar LINE, POINT,
  ARC, CIRCLE, LWPOLYLINE, and independently verified flat POLYLINE3D
  structures; explicitly partial and not a full editable DWG model.
- JNI gives source-record admission distinct from rendered primitive count.
- This change distinguishes original TEXT/MTEXT, ATTRIB/ATTDEF, BLOCK/ENDBLK,
  INSERT/MINSERT counts from other indexed records without pretending any
  of those classes are decoded.

## Blocking production/full-2D acceptance

1. Native C11 real-source TEXT/MTEXT decode with independent original-file
   layout/font/Unicode reference comparisons.
2. Native C11 real-source BLOCK/INSERT graph and transform traversal; strict
   cycle/XREF/unknown-object refusal with no guessed geometry.
3. Source-validated layer visibility, then viewport and model-space decisions
   in the same C11 core; tests for ON, OFF, FREEZE, viewport overrides.
4. Final direct-and-inherited style/color/alpha resolve, with visual pixel
   or vector oracle across Android, Windows and iOS.
5. Full, non-destructive real-world DWG project golden tests across all
   supported releases, plus DXF/DWG write/reopen parity if claiming editing.
6. All architecture, 3-platform portability, real DWG, Android and fast CAD
   gates pass on identical commit SHA; no intermediate APK.

**Until all gates above are demonstrated, the product must display
"KISMİ/EXPERIMENTAL DWG PREVIEW" and never "2B %100", "complete DWG open",
"pixel-identical colors" or AutoCAD-equivalent.** Counting missing
original numeric OT records is a prerequisite to the real decoders, not
completion of any of them.

Sources in this repo:
- `core/native/src/musacad_dwg_ac1032_scene_index.c`
- `core/native/src/musacad_dwg_ac1032_preview_v6.c`
- `core/native/src/musacad_dwg_ac1032_style_evidence.c`
- `core/native/src/musacad_dwg_ac1032_color_provenance.c`
- `core/native/include/musacad_dwg_ac1032_layer.h`
- `app/src/main/cpp/ac1032_firstparty_preview_jni.c`
