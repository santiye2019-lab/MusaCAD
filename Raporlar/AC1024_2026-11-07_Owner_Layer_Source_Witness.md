# AC1024 Original DWG Owner / Layer Source Witness — 2026-11-07

## Provenance

- Evidence run: [GitHub Actions 42370529045](https://github.com/santiye2019-lab/MusaCAD/actions/runs/42370529045).
- Input: unchanged original `Projeler` Git LFS real-DWG corpus; **10 AC1024 files**, **23 original DWG files** source integrity audited.
- Proven prerequisite on the AC1024 sources: **1,651,448/1,651,448** original Handles-to-Objects candidate offset / CRC16 / canonical embedded self-H / numeric H comparisons.
- Read-only C11 native source-probe, not an APK acceptance test. No original DWG or source bytes altered.

## Bounded original-source graph sample

Deterministic uniform source-index sample of **at most 512 positions per file**. The sample is not a full graph census.

| Measurement | Original-source result |
|---|---:|
| AC1024 original drawings with index verified | 10 / 10 |
| Indexed source records sampled | 4,890 |
| Sampled fixed OT7 / OT17 / OT18 / OT19 entity candidates | 2,236 |
| OT49 BLOCK_HEADER owner **and** OT51 LAYER target both source-CRC/self-H verified (explicit owner subset) | 1,242 |
| OT19 LINE mode 1/2, implicit owner **not serialized**, source-CRC/self-H verified OT51 LAYER target only | 364 |
| Supported exact-relationship witnesses (mutually exclusive) | **1,606** |
| Original AC1024 drawings with at least one qualified owner/layer or implicit-layer relationship | **10 / 10** |

The common entity fields have further variants, reactors, EED, XDictionary and other reference modes that this proof does **not** cover. A refused or unproven sample is **not** necessarily malformed; it can be outside the narrow supported witness API. Some genuine drawings return **zero** owner+layer evidence while returning **positive implicit-LINE layer evidence**; such files must not be excluded or supplied with an invented owner.

## CI protection

The regression gate `require_original_ac1024_owner_layer_source_witnesses` requires each original AC1024 input to have a complete independently authenticated source index, bounded sample fields, and at least one positive source-CRC-backed relationship. The schema forbids counts exceeding the sampled fixed-entity candidate population, and explicitly rejects any geometry/rendering claim. Synthetic tests verify that original CRC/self-H identity alone cannot manufacture a graph.

## What remains **unverified**

- Exhaustive owner/layer coverage across **all** indexed records; block INSERT ownership, nested transforms and XREF; layer on/off, freeze, lock and color provenance; visual correctness.
- General entity geometry, dimensions, hatches, annotations, 3D ACIS/B-Rep, editing, DWG writer compatibility, and AutoCAD functional equivalence.
- These raw source-related numeric witnesses must not automatically enable CAD scene drawing. Progress from here must be additional source-backed evidence with no false 100% reader claim.
