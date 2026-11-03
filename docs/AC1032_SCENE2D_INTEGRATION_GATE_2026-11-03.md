# AC1032 → scene2d integration: first acceptance gate (2026-11-03)

Scope: MusaCAD feature branch only. This is an interface audit and test specification, NOT an implementation of real DWG decoding or authorization to enable Android normal-open.

## Source-verified contracts
- `musa_dwg1032_object_index_init`: accepts caller-decoded object locations ordered by nonzero handles; does not decode object maps.
- `musa_dwg1032_object_spans_prepare`: rejects overlapping ranges, including ranges whose physical order differs from handle order.
- `musa_dwg1032_owner_graph_resolve`: validates predecoded ownership; rejects dangling owners and cycles.
- `musa_dwg1032_scene_handoff_validate`: requires evidence for each decoded entity; rejects unknown kind and missing layer handle.
- `musa_scene2d_resolve`: resolves predecoded node/layer semantics, nested INSERT, BYLAYER/BYBLOCK, and affine placement; does not parse DWG bytes.

## Integration hazard to resolve before writing an adapter
The handoff evidence requires a nonzero `layer_handle`, whereas the scene2d API explicitly treats layer 0 as a valid special layer and inherits it inside INSERTs. A real DWG layer-zero record's source handle must NOT be confused with scene2d's zero sentinel. Define an explicit, validated mapping from authenticated source layer records to scene-layer identifiers; reject missing records and do not invent handles.

The owner graph's root owner (0) and scene2d's `parent_insert == 0` are not interchangeable for all DWG object classes. Only proven model-space roots may become scene roots. Block-definition membership and INSERT instances require authenticated source relationships.

## Required first regression cases
1. Disjoint object spans with handles ordered differently from byte offsets: accept and retain handle identity.
2. Overlapping spans, truncated last span, duplicate/zero handle: reject with no published scene.
3. Missing owner, owner cycle, owner that is not an INSERT for a scene child: reject.
4. Layer-zero inheritance within nested INSERTs; BYBLOCK/BYLAYER with source color provenance: exact expected effective layer/color.
5. A root-level entity with unresolved layer or unsupported entity kind: reject rather than omit.
6. Invalid/nonfinite affine, invalid UTF-16, or missing text placement evidence: reject and preserve prior output.
7. Authenticated empty model versus zero decoded entities in a nonempty model: distinguish explicitly.
8. Real-file golden test: source handle count, owner count, per-layer counts, bounding boxes, source colors, text strings, and rendered reference frames must match an independently checked DWG.

## Acceptance criteria
No promotion to normal Android DWG open until authentic AC1032 section/page/object decoding and complete model/layout ownership have been demonstrated on real files; all unsupported objects must be reported. Keep current fail-closed JNI, main, MusaCAD2, and historical 3D code unchanged. No GPL/AGPL/LGPL code may be copied into the independent implementation.

Next implementation step: add a small independent adapter contract with an explicit source-layer mapping and root/INSERT ownership mapping, then test its atomic rejection behavior using caller-decoded synthetic records before any Android linkage.
