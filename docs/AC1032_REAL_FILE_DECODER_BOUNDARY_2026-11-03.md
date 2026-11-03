# AC1032 real-file decoder boundary — source inspection, 2026-11-03

Repository: MusaCAD, PR #705 feature branch. This is a source-contract audit, not a real-file test or proof of successful DWG opening.

## Existing implementation that must be reused
- `musa_dwg1032_object_map.h`: bounded iterator and decoder for **already verified, decompressed** object-map bytes. Its candidate spans are not authenticated object lengths.
- `musa_dwg1032_record_frame.h`: MS-length-prefixed record framing over a **caller-verified** candidate region. Framing does not decode entity type, handle, owner, layer or geometry.
- `musa_dwg1032_object_spans.h`: validates post-decoded nonoverlapping object ranges.
- `musa_dwg1032_owner_graph.h`, `musa_dwg1032_layer_color.h`, `musa_dwg1032_instance_attributes.h`: existing post-decode semantics.
- `musa_scene2d.h` and `musa_scene2d_layer.h`: existing predecoded scene and layer conversion.

## First unproven boundary
The object-map iterator explicitly assumes verified/decompressed bytes. The record-frame API explicitly assumes a caller-verified candidate region. Neither authenticates AC1032 physical section/page maps, decompression, object-record semantics or complete model/layout ownership. Synthetic tests in `dwg1032_object_map_test.c` cannot establish real-file success.

## Next integration test (red first)
Using a provenance-cleared authentic AC1032 fixture, assert that a complete authenticated object-map payload and its section bounds are obtained **from the file**, not injected as predecoded bytes. Then verify exact map consumption, handle uniqueness, record frame boundaries and object counts. The test must fail closed at the first unsupported source stage and identify it; do not fabricate objects, colors or block membership. Only after this gate passes, exercise existing owner/color/scene APIs.

## Duplication control
Do not implement another object-map iterator, candidate-span generator, MS-frame reader, owner graph, layer-color resolver or scene-layer converter. The recently added source-to-scene layer ID mapper is provisional; integrate only if a failing test demonstrates an actual mapping gap. Keep Android normal-open blocked, main and existing 3D unchanged.

No fixture was supplied or executed during this audit. No claim of CI success.
