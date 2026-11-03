# MusaCAD MC1032 / 2D integration ownership inventory — 2026-11-03

Status: source-interface inventory, NOT a passing end-to-end DWG test. Scope is existing MusaCAD PR #705 feature branch; do not modify main, JNI admission, or 3D.

| Responsibility | Existing first-party owner | Contract / remaining gap |
| --- | --- | --- |
| Decoded object handles and offsets | `dwg1032/include/musa_dwg1032_object_index.h`, `musa_dwg1032_object_spans.h` | Checks caller-decoded spans, not authentic AC1032 object-map decoding. |
| Owner ancestry and visibility | `dwg1032/include/musa_dwg1032_owner_graph.h` | Checks predecoded owner references; cannot infer model/layout roots or block-instance membership from bytes. |
| Source layer/color provenance | `dwg1032/include/musa_dwg1032_layer_color.h` | Uses authenticated nonzero layer handles, ACI/truecolor and per-instance BYBLOCK/BYLAYER. Do not duplicate its color resolution. |
| Per-INSERT inherited attributes | `dwg1032/include/musa_dwg1032_instance_attributes.h` | Handles predecoded concrete INSERT contexts; source block relationships remain to be decoded. |
| Layer conversion for scene | `scene2d/include/musa_scene2d_layer.h` | Converts already decoded CAD layer attributes, with off/frozen state and vetted palette. Do not duplicate layer color conversion. |
| Scene ancestry/transform | `scene2d/include/musa_scene2d.h` | Resolves predecoded scene nodes and nested INSERT placement; cannot read AC1032 bytes. |
| Source evidence admission | `dwg1032/include/musa_dwg1032_scene_handoff.h` | Rejects incomplete per-entity provenance, but not proof of full-document decoding. |
| Newly added source/scene ID translation | `dwg1032/include/musa_dwg1032_scene_layer_map.h` | Limited to authenticated source-handle → scene-layer ID mapping. **Review whether any caller needs this before integrating**; no second color/visibility resolver. |
| Android normal open | `app/src/main/cpp/firstparty_dwg_unavailable_jni.c` | Intentionally fails closed. Keep unchanged pending real-file parity and license review. |

## Avoid duplicated implementation

1. Do not create a second owner graph, layer color resolver, INSERT inheritance resolver, or scene layer converter.
2. Existing layer-zero semantics differ: source DWG layer zero is identified by its *authenticated nonzero layer-record handle*; scene2d uses scene-layer ID 0 for layer-zero inheritance. The new ID mapper is a provisional seam only, not proof it is required.
3. Owner-root zero is not the same as model-space scene-root zero; require an authenticated model/layout/block-definition association before constructing scene roots.
4. Do not promote a synthetic green test to a real-DWG feature claim.

## Missing integration, in dependency order

A. Authenticate real AC1032 section/page/object records and the source object inventory. Current standalone cursors and synthetic tests do not establish this.
B. Decode and verify model/layout roots, block definitions, concrete INSERT instances, entity types, layer-table membership and source colors; reject unsupported content with explicit diagnostics.
C. Reuse the existing owner graph, color, instance and scene2d APIs to construct one immutable source-evidenced scene, preserving source handles and object counts.
D. Add golden real-DWG tests for text, blocks, nested INSERTs, layers, colors, visibility, and large sheets; compare counts, geometry, source color and rendered reference frames.
E. Only after independent source parity, provenance/license review and platform tests, evaluate production JNI enablement.

## Immediate next task

Inspect existing AC1032 record and object-map readers against an authentic licensed test fixture and document the first unsupported source record. **Do not write another generic layer/owner/INSERT helper** until the decoder-to-scene gap is proven by a failing integration test.

Evidence: inspected source headers in PR #705 and `docs/AC1032_RENDER_CHAIN_RECOVERY_2026-11-02.md`; this document does not claim CI success.
