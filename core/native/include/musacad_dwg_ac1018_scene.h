/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_SCENE_H
#define MUSACAD_DWG_AC1018_SCENE_H
#include <stddef.h>
#include <stdint.h>
#include "musacad_dwg_ac1018_line.h"
#include "musacad_dwg_ac1018_curve.h"
#include "musacad_dwg_ac1018_lwpolyline.h"
#include "musacad_dwg_ac1018_nonentity.h"
#include "musacad_dwg_ac1018_layer_control.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Read-only, first-party portable C11 R2004 (AC1018) geometry session.
 * Open verifies/decompresses both protected sections once, constructs and
 * CRC-validates the complete bounded handle index, then supports many
 * O(log N) handle lookups without repeatedly decompressing the source.
 * On failure *out remains unchanged, source bytes are never changed.
 * All read/probe output pointers must be disjoint from the opaque session,
 * its cached object bytes and its immutable handle index; otherwise the
 * call fails without writes. The session owns its decoded buffers.
 * No lossless scene, layer/owner semantics, or native DWG writer claim.
 */
typedef struct MusaDwgAc1018Scene MusaDwgAc1018Scene;
int musa_dwg_ac1018_scene_open(const uint8_t *file,size_t file_bytes,
                              MusaDwgAc1018Scene **out);
void musa_dwg_ac1018_scene_close(MusaDwgAc1018Scene *scene);
size_t musa_dwg_ac1018_scene_handle_count(const MusaDwgAc1018Scene *scene);
/* Enumeration of the CRC-backed index in strictly ascending handle order.
 * Probe verifies the individual MS record frame + CRC-16 and reads only
 * the DECLARED BS type, not geometry, owner, class, or layer semantics.
 * Thus an unknown declared type is valid inventory, not accepted geometry.
 * Returns 0 on bad ordinal, malformed record or checksum failure and
 * preserves *out atomically. No scan of records is hidden in scene_open.
 */
typedef struct MusaDwgAc1018RecordProbe {
    uint64_t handle;
    uint64_t object_offset;  /* relative to decoded AcDbObjects */
    size_t record_bytes;     /* framed payload plus length and CRC */
    uint16_t declared_type;  /* untrusted type claim from CRC-backed bits */
    uint8_t record_crc_verified;
} MusaDwgAc1018RecordProbe;
int musa_dwg_ac1018_scene_probe_record_at(
    const MusaDwgAc1018Scene *scene,size_t index_ordinal,
    MusaDwgAc1018RecordProbe *out);
/* O(log N) handle lookup followed by CRC16/frame/type probe of the
 * referenced object. A CRC-backed declared type is NOT a verified AcDb
 * class, embedded self-handle or validated geometry. Atomic on failure. */
int musa_dwg_ac1018_scene_probe_record_handle(
    const MusaDwgAc1018Scene *scene,uint64_t handle,
    MusaDwgAc1018RecordProbe *out);
/* Look up a fixed non-entity target in the cached protected R2004 scene
 * and independently verify its embedded self-H against the CRC-backed
 * scene index. Does not infer a validated AcDb semantic role; no changes
 * on failure, no extra decompression, no hidden full-map copy. */
int musa_dwg_ac1018_scene_probe_nonentity_identity(
    const MusaDwgAc1018Scene *scene,uint64_t handle,
    MusaDwgAc1018NonEntityIdentity *out);
/* Indexed cached-scene access to the first-party ASCII-only LAYER
 * DATA PREFIX. Rejects wrong type/self-H/CRC, unsupported codepage and
 * overlap with immutable cached source. NOT full AcDb semantic decode. */
int musa_dwg_ac1018_scene_read_layer_prefix(
    const MusaDwgAc1018Scene *scene,uint64_t handle,
    MusaDwgAc1018LayerPrefix *out);
/* Verify the cached R2004 LAYER pre-handle CMC fields while preserving
 * the caller's output and immutable DWG bytes on any unsupported case.
 * Reports raw RGB and optional ASCII color/book names ONLY. */
int musa_dwg_ac1018_scene_read_layer_color(
    const MusaDwgAc1018Scene *scene,uint64_t handle,
    MusaDwgAc1018LayerColor *out);
/* Strict one-way R2004 LAYER_CONTROL -> LAYER target verification:
 * decode exact H tail; check all targets in the CRC-backed native scene,
 * their type 51 and embedded self-H, and reject duplicate targets.
 * Fail atomically. NOT a reverse parent graph, AcDb class proof, or parity.
 */
int musa_dwg_ac1018_scene_read_layer_control_refs(
    const MusaDwgAc1018Scene *scene,uint64_t control_handle,
    uint64_t *entry_handles,size_t capacity,
    MusaDwgAc1018LayerControlRefs *out);
/* Reverse membership SEARCH, not the embedded LAYER owner-H grammar.
 * Verify a numeric LAYER 51 self-H, then scan every indexed object for
 * supported, CRC/self-H checked LAYER_CONTROL 50 tables. Each such table's
 * full H target set must resolve, and the requested layer must appear in
 * exactly one of them. Unsupported/corrupt tables cause fail-closed return.
 * O(scene records + validated table refs), one bounded scratch allocation.
 * Never infer a verified AcDb class, owner/parent H, or DWG parity.
 * The output is unchanged on failure.
 */
int musa_dwg_ac1018_scene_find_layer_control_membership(
    const MusaDwgAc1018Scene *scene,uint64_t layer_handle,
    uint64_t *out_control_handle);
/* Closed-world LAYER_CONTROL table MEMBERSHIP coverage for a cached
 * original AC1018 scene, not owner-H semantics or AutoCAD parity.
 * Inspect all CRC-checked indexed records. Every numeric type-51 LAYER must
 * have valid embedded self-H and occur in exactly one complete validated
 * type-50 LAYER_CONTROL H list. Every table reference must resolve to a
 * type-51 self-consistent LAYER. Reject absent tables, orphaned layers,
 * repeated membership and unsupported/malformed tables. Other object
 * types remain opaque; no block/text/geometry coverage is asserted.
 * On any failure the caller output and source session are unchanged.
 */
typedef struct MusaDwgAc1018LayerTableCoverage {
    size_t indexed_layers;
    size_t indexed_controls;
    size_t resolved_layer_references;
    uint8_t all_indexed_layers_have_unique_table;
} MusaDwgAc1018LayerTableCoverage;
int musa_dwg_ac1018_scene_audit_layer_table_coverage(
    const MusaDwgAc1018Scene *scene,
    MusaDwgAc1018LayerTableCoverage *out);
/* Complete supported R2004 LAYER palette ingestion, NOT final display RGB:
 * require full layer-table membership coverage, exact indexed identity and
 * a readable ASCII name + bounded raw CMC for every numerical LAYER 51.
 * Reject missing/unreadable layers and ASCII case-insensitive duplicate
 * names; preserve caller output atomically on all failure paths.
 * Colors are ordered by ascending layer handle (scene H-index order).
 * Deliberately bounded to 65536 layers; this is not visual parity or
 * decoded owner-H, XREF, plotstyle, or True Color presentation semantics.
 */
typedef struct MusaDwgAc1018LayerPaletteReport {
    size_t indexed_layers;
    size_t indexed_layer_controls;
    uint8_t unique_ascii_names_verified;
    uint8_t raw_cmc_fields_verified;
} MusaDwgAc1018LayerPaletteReport;
int musa_dwg_ac1018_scene_read_verified_layer_palette(
    const MusaDwgAc1018Scene *scene,
    MusaDwgAc1018LayerColor *layers,size_t capacity,
    MusaDwgAc1018LayerPaletteReport *out);
/* Find a layer by an ASCII case-insensitive name only after accepting the
 * entire verified LAYER_CONTROL membership and name/CMC palette. Input must
 * be 1..255 printable ASCII bytes followed by NUL. Every layer must be
 * readable, unique under ASCII folding and valid before a match is returned.
 * This bounded O(N) lookup rebuilds the palette; cache the palette instead
 * of calling this for each primitive during rendering. No inferred display
 * RGB, entity layer assignment, or AcDb ownership is provided.
 * Read-only and transactional: *out remains unchanged on every failure.
 */
int musa_dwg_ac1018_scene_find_verified_layer_ascii_name(
    const MusaDwgAc1018Scene *scene,const char *ascii_name,
    MusaDwgAc1018LayerColor *out);
/* Diagnostic read-only bridge from a supported numeric 2D entity
 * (17 ARC, 18 CIRCLE, 19 LINE, 77 LWPOLYLINE) to its referenced LAYER.
 * First verify the entity's CRC-backed fixed H role targets/self identity,
 * then require the COMPLETE supported LAYER_CONTROL membership and layer
 * palette, and find that layer by its indexed H (not a guessed name).
 * In particular this accepts LINK evidence, not decoded entity geometry.
 * It does not infer AcDb CLASSES, draw order, owner-H, layer visibility,
 * effective ByLayer/ByBlock RGB, full DWG parity or a release milestone.
 * This bounded diagnostic recomputes global layer coverage, O(N) memory;
 * never invoke once per entity in a viewport hot path. Outputs are atomic.
 */
typedef struct MusaDwgAc1018EntityLayerEvidence {
    uint64_t entity_handle;
    uint64_t layer_control_handle;
    MusaDwgAc1018LayerColor layer;
    uint8_t fixed_entity_links_verified;
    uint8_t complete_layer_table_verified;
} MusaDwgAc1018EntityLayerEvidence;
int musa_dwg_ac1018_scene_read_verified_entity_layer(
    const MusaDwgAc1018Scene *scene,uint64_t entity_handle,
    MusaDwgAc1018EntityLayerEvidence *out);
/* Performance-oriented, READ-ONLY diagnostic batch, not a renderer:
 * establish complete R2004 LAYER_CONTROL -> LAYER membership and raw CMC
 * once per bounded batch, then independently audit the fixed entity H
 * references (ARC/CIRCLE/LINE/LWPOLYLINE) and resolve by exact layer H.
 * Results keep the caller's entity order. Duplicate entity handles, wrong
 * type, unsupported CMC, corrupted graph or non-ASCII names fail closed.
 * 1..262144 distinct entities per call; page larger input. One global
 * palette and reference traversal, not a full graph scan per entity.
 * raw_cmc_rgb is unprocessed original metadata, NOT final display RGB.
 * NOT a geometry proof, AcDb class/owner proof, visual parity or writer.
 * Source and all caller outputs are unchanged on any failure.
 */
typedef struct MusaDwgAc1018EntityLayerLink {
    uint64_t entity_handle;
    uint64_t layer_handle;
    uint64_t layer_control_handle;
    uint32_t raw_cmc_rgb;
} MusaDwgAc1018EntityLayerLink;
typedef struct MusaDwgAc1018EntityLayerBatchReport {
    size_t verified_entities;
    size_t verified_layers;
    size_t verified_layer_controls;
    uint8_t verified_unique_entity_handles;
    uint8_t complete_layer_membership_verified;
} MusaDwgAc1018EntityLayerBatchReport;
int musa_dwg_ac1018_scene_read_entity_layer_batch(
    const MusaDwgAc1018Scene *scene,
    const uint64_t *entity_handles,size_t entity_count,
    MusaDwgAc1018EntityLayerLink *links,size_t link_capacity,
    MusaDwgAc1018EntityLayerBatchReport *out);
/* Strict indexed original R2004 layer-link collection. Check EVERY H
 * record before admitting the small supported numeric object vocabulary:
 * 17 ARC, 18 CIRCLE, 19 LINE, 77 LWPOLYLINE, plus fixed-prefix-only
 * 42 DICTIONARY, 49 BLOCK_HEADER, 50 LAYER_CONTROL, 51 LAYER.
 * Unknown/unclassified record types are FATAL, never silently skipped.
 * The numeric fixed nonentity prefixes are NOT fully decoded classes,
 * and link-only entities are NOT proof of decoded drawable geometry.
 * Entity links are ordered by original ascending indexed H; no guesswork.
 * Resolve all entities using one bounded verified layer-table/palette
 * batch call; never partially publish or modify the source on failure.
 * Reject zero entities, insufficient capacity or >262144 entities.
 * This is a deliberately narrow subset acceptance diagnostic, NOT
 * full-drawing coverage, 2D geometric parity, color fidelity or writer.
 */
typedef struct MusaDwgAc1018IndexedLayerLinkCoverage {
    size_t indexed_records_checked;
    size_t fixed_nonentity_prefixes_checked;
    size_t supported_entity_links_checked;
    size_t verified_layers;
    size_t verified_layer_controls;
    uint8_t no_unclassified_record_types;
    uint8_t all_supported_entity_links_verified;
} MusaDwgAc1018IndexedLayerLinkCoverage;
int musa_dwg_ac1018_scene_collect_strict_indexed_layer_links(
    const MusaDwgAc1018Scene *scene,
    MusaDwgAc1018EntityLayerLink *links,size_t capacity,
    MusaDwgAc1018IndexedLayerLinkCoverage *out);
/* Independent evidence MATRIX for original indexed AC1018 2D objects.
 * Count only geometry actually accepted by the primitive decoder and only
 * H link roles accepted by fixed target CRC/self-H identity audits.
 * Distinguish both vs geometry-only vs link-only vs neither. Include all
 * indexed records, report opaque/unsupported ones rather than dropping.
 * Requires whole-scene record/physical-range CRC checks first. A positive
 * 2D H-link probe is NOT geometry, named-layer, ByLayer display RGB, AcDb
 * class validation, writer support or AutoCAD parity.
 * A successful diagnostic can report many incomplete records; callers
 * MUST check counters, not treat function success as full CAD acceptance.
 * C11 portable, no external DWG decoder, no source or output mutation
 * on any fatal integrity error. O(N log N) index + scratch vertices.
 */
typedef struct MusaDwgAc1018GeometryLinkEvidence {
    size_t indexed_records;
    size_t declared_supported_2d_entities;
    size_t geometry_decoded;
    size_t fixed_link_identities_verified;
    size_t geometry_and_links_verified;
    size_t geometry_only;
    size_t links_only;
    size_t neither_verified;
    size_t other_declared_records;
    uint8_t all_record_ranges_nonoverlapping;
    uint8_t all_indexed_record_crcs_verified;
} MusaDwgAc1018GeometryLinkEvidence;
int musa_dwg_ac1018_scene_audit_2d_geometry_vs_links(
    const MusaDwgAc1018Scene *scene,
    MusaDwgAc1018GeometryLinkEvidence *out);
/* Bounded per-record evidence page from the original indexed AC1018 H
 * map. This is an OFFLINE diagnostic, not a viewport read hot path.
 * Preflight verifies the entire scene (CRC, nonoverlapping ranges and
 * independent 2D geometry/H-role coverage) on EACH page; costly but no
 * unsupported record may be silently omitted. Returns original indexed
 * numeric H and BS type, sorted by H; nonentities are not assumed to
 * be lost drawable geometry, and unsupported types are UNCLASSIFIED.
 *
 * Geometry-reader rejection vs fixed-target H-audit rejection are separate
 * EVIDENCE LIMITS, not precise root causes. Neither is equivalent to a
 * completely decoded AcDb class, layer style or original screen RGB.
 * 1..4096 records/page; next_index allows bounded, repeatable traversal.
 * Source, caller's rows and report are unchanged upon fatal failure.
 */
#define MUSA_DWG_AC1018_DIAG_2D_ENTITY 1u
#define MUSA_DWG_AC1018_DIAG_FIXED_NONENTITY 2u
#define MUSA_DWG_AC1018_DIAG_UNCLASSIFIED 3u
#define MUSA_DWG_AC1018_DIAG_GEOMETRY_REJECTED 1u
#define MUSA_DWG_AC1018_DIAG_LINK_AUDIT_REJECTED 2u
#define MUSA_DWG_AC1018_DIAG_UNCLASSIFIED_TYPE 4u
typedef struct MusaDwgAc1018RecordDiagnosis {
    uint64_t indexed_handle;
    uint16_t declared_type;
    uint8_t category;
    uint8_t geometry_decoded;
    uint8_t fixed_link_identities_verified;
    uint8_t evidence_issues;
} MusaDwgAc1018RecordDiagnosis;
typedef struct MusaDwgAc1018RecordDiagnosisPage {
    size_t total_indexed_records;
    size_t first_index;
    size_t records_written;
    size_t next_index;
    size_t supported_2d_in_page;
    size_t geometry_rejected_in_page;
    size_t link_audit_rejected_in_page;
    size_t unclassified_in_page;
} MusaDwgAc1018RecordDiagnosisPage;
int musa_dwg_ac1018_scene_read_record_diagnosis_page(
    const MusaDwgAc1018Scene *scene,size_t first_index,
    MusaDwgAc1018RecordDiagnosis *rows,size_t capacity,
    MusaDwgAc1018RecordDiagnosisPage *out);
/* Immutable, self-contained original-H diagnosis snapshot.
 * Audit the entire source once at creation (CRC, interval integrity and
 * independent geometry/link evidence), copy all indexed results, and
 * traverse pages without re-opening, reparsing or rescanning the DWG.
 * Snapshot survives scene_close; the caller owns its lifetime. Results
 * are diagnostics, not a claim of complete geometric CAD equivalence.
 * Creation is bounded by the existing scene 1,000,000-H safety cap.
 * Only the creator allocates; read_page performs NO allocations.
 * On failed create/read, caller output is preserved byte-for-byte.
 */
typedef struct MusaDwgAc1018DiagnosisSnapshot MusaDwgAc1018DiagnosisSnapshot;
int musa_dwg_ac1018_scene_create_diagnosis_snapshot(
    const MusaDwgAc1018Scene *scene,
    MusaDwgAc1018DiagnosisSnapshot **out);
void musa_dwg_ac1018_diagnosis_snapshot_close(
    MusaDwgAc1018DiagnosisSnapshot *snapshot);
size_t musa_dwg_ac1018_diagnosis_snapshot_handle_count(
    const MusaDwgAc1018DiagnosisSnapshot *snapshot);
int musa_dwg_ac1018_diagnosis_snapshot_read_page(
    const MusaDwgAc1018DiagnosisSnapshot *snapshot,
    size_t first_index,MusaDwgAc1018RecordDiagnosis *rows,
    size_t capacity,MusaDwgAc1018RecordDiagnosisPage *out);
int musa_dwg_ac1018_scene_read_line(const MusaDwgAc1018Scene *scene,
      uint64_t handle,MusaDwgAc1018Line *out);
/* Read one CRC-backed ARC (type 17) or CIRCLE (type 18) geometry object
 * from an already opened, verified and cached original R2004 DWG.
 * Uses the same first-party primitive C11 decoder; does not verify
 * graphical ownership/linetype graph or promise commercial CAD parity.
 * Rejects absent/wrong-type/unsupported entity records atomically. */
int musa_dwg_ac1018_scene_read_curve(const MusaDwgAc1018Scene *scene,
      uint64_t handle,MusaDwgAc1018Curve *out);
/* Reads 1..100000 OCS vertices and metadata, never partial output.
 * Caller can convert OCS vertex + elevation/extrusion to WCS using the
 * existing musa_dwg_ac1018_lwpolyline_vertex_to_wcs().
 * Output vertex array and metadata must not overlap.
 */
int musa_dwg_ac1018_scene_read_lwpolyline(
      const MusaDwgAc1018Scene *scene,uint64_t handle,
      MusaDwgAc1018LWVertex *vertices,size_t vertex_capacity,
      MusaDwgAc1018LWPolyline *out);
/* Audit H owner/layer/xdictionary/reactor reference membership for one
 * indexed AC1018 ARC/CIRCLE/LINE/LWPOLYLINE record, without accepting geometry.
 * Supports zero or 1..64 reactors and preserves the original positive-only
 * API semantics. All ref targets must exist in the CRC-verified scene index.
 * Caller buffers, scene, and all DWG bytes remain untouched on failure.
 * Targets are numeric H references only: LAYER table and owner target
 * CLASSES are NOT yet decoded or semantically proven.
 *
 * The cached native handle index is reused without O(N) copies;
 * verified target membership lookups use binary search.
 */
int musa_dwg_ac1018_scene_audit_links(
    const MusaDwgAc1018Scene *scene,uint64_t handle,
    uint64_t *reactor_targets,size_t reactor_capacity,
    MusaDwgAc1018ReactorAudit *out);
/* Strengthen an already supported ARC/CIRCLE/LINE/LWPOLYLINE H-link audit:
 * require EVERY referenced owner, layer, XDictionary and reactor target
 * to also have a valid individual MS-framed CRC16 object record. Return
 * each record's untrusted DECLARED BS type, never infer target class role.
 * Caller outputs remain byte-identical on any missing/invalid target.
 * This is not yet the full layer/owner semantic class graph gate. */
typedef struct MusaDwgAc1018LinkedRecordAudit {
    MusaDwgAc1018ReactorAudit link_roles;
    MusaDwgAc1018RecordProbe owner_record;  /* zero if mode 2/no owner */
    MusaDwgAc1018RecordProbe layer_record;
    MusaDwgAc1018RecordProbe xdictionary_record; /* zero if absent */
} MusaDwgAc1018LinkedRecordAudit;
int musa_dwg_ac1018_scene_audit_link_record_envelopes(
    const MusaDwgAc1018Scene *scene,uint64_t entity_handle,
    MusaDwgAc1018RecordProbe *reactor_records,size_t reactor_capacity,
    MusaDwgAc1018LinkedRecordAudit *out);
/* Reject CRC-correct but incorrectly declared fixed object-role targets:
 * layer must claim 51 (LAYER), graphical owner when present 49
 * (BLOCK_HEADER), and XDictionary when present 42 (DICTIONARY).
 * This is a numerical declared-BS filter only: it does NOT prove an
 * AcDb class, embedded self-handle, or complete ownership semantics.
 * Reactor targets are CRC-probed but their classes are not restricted.
 * Entire audit is transactional. No progress milestone is satisfied. */
int musa_dwg_ac1018_scene_audit_fixed_role_type_claims(
    const MusaDwgAc1018Scene *scene,uint64_t entity_handle,
    MusaDwgAc1018LinkedRecordAudit *out);
/* Validates the SUPPORTED fixed R2004 link TARGETS together:
 * entity H-reference membership, CRC/framing, numeric BS declared type
 * and absolute embedded self-H equal to each indexed target handle.
 * A positive result is still NOT a full AcDb layer/block class decode.
 * Reactors are checked for membership/CRC but cannot be classified yet.
 * All outputs are transactional and scene-owned buffers are immutable. */
typedef struct MusaDwgAc1018FixedLinkIdentities {
    MusaDwgAc1018LinkedRecordAudit roles;
    MusaDwgAc1018NonEntityIdentity layer;
    MusaDwgAc1018NonEntityIdentity owner;
    MusaDwgAc1018NonEntityIdentity xdictionary;
    uint8_t verified_fixed_identity_links;
} MusaDwgAc1018FixedLinkIdentities;
int musa_dwg_ac1018_scene_audit_fixed_link_identities(
    const MusaDwgAc1018Scene *scene,uint64_t entity_handle,
    MusaDwgAc1018FixedLinkIdentities *out);
/* Independently audit EVERY indexed R2004 record: bounded MS-frame,
 * CRC16 and supported 2D geometry self-H checks. Numerical declared types
 * alone are NEVER counted as decoded geometry. Fixed DICTIONARY (42),
 * BLOCK_HEADER (49), LAYER_CONTROL (50) and LAYER (51) numeric claims
 * must pass CRC and indexed embedded self-H identity checks. An invalid
 * fixed nonentity fails the entire inventory atomically.
 * These verified prefixes remain counted as other/opaque records because
 * their complete AcDb fields are not decoded. Unknown records are counted,
 * not dropped. Known-but-unsupported geometry is counted separately.
 * No block, layer, layout, AcDb-class, renderer or writer parity claim.
 * strict_supported_only=1 refuses incomplete subsets atomically; it does
 * not imply complete DWG drawing or AutoCAD 2026 equivalence.
 * Scratch polyline vertices (max 100000) are reused. On any failure
 * caller outputs and source scene stay unchanged.
 */
typedef struct MusaDwgAc1018SceneInventory {
    size_t indexed_record_count;
    size_t decoded_lines;
    size_t decoded_arcs;
    size_t decoded_circles;
    size_t decoded_lwpolylines;
    size_t undecodable_supported_records;
    size_t other_declared_records;
    uint8_t all_indexed_record_crcs_verified;
    uint8_t only_supported_primitives_decoded;
} MusaDwgAc1018SceneInventory;
int musa_dwg_ac1018_scene_inventory(
    const MusaDwgAc1018Scene *scene,int strict_supported_only,
    MusaDwgAc1018SceneInventory *out);
/* Independent, fail-closed physical byte-range audit of ALL indexed
 * AC1018/R2004 object records. Sorts a temporary array by SECTION offset,
 * because H index ordering does not imply increasing physical offsets.
 * Verifies each record CRC and rejects duplicate/overlapping ranges.
 * Gaps are reported, not automatically interpreted as lost entities.
 * O(N log N) time / O(N) temporary scratch; no cached index mutation.
 * Does NOT establish full object/class/geometry semantic coverage.
 * Output and source scene remain untouched on any failure.
 */
typedef struct MusaDwgAc1018RecordRangeAudit {
    size_t indexed_records;
    size_t covered_record_bytes;
    size_t unindexed_section_bytes;
    uint8_t ranges_nonoverlapping;
} MusaDwgAc1018RecordRangeAudit;
int musa_dwg_ac1018_scene_audit_record_ranges(
    const MusaDwgAc1018Scene *scene,
    MusaDwgAc1018RecordRangeAudit *out);
/* First-party original R2004 LINE and straight LWPOLYLINE centerline
 * geometry bounds in WCS (world XYZ). This is ACTUAL decoded geometry,
 * unlike the H-only link audit, but still NOT full drawing coverage.
 * Refuse bulge arcs, nonzero widths, thickness, less than two vertices,
 * or non-finite transforms: otherwise a misleading undersized viewport
 * bound might silently clip actual geometry. No strokes/lineweight or
 * owner/layer/style semantics are covered. No ARC/CIRCLE here.
 * O(N vertices) for LWPOLYLINE, bounded by the existing 100000-vertex
 * native decoder limit. The caller output is unchanged on failure.
 */
typedef struct MusaDwgAc1018LinearWcsBounds {
    uint64_t handle;
    uint16_t type_code; /* 19 LINE, 77 straight LWPOLYLINE */
    uint32_t vertex_count;
    uint32_t segment_count;
    double minimum[3];
    double maximum[3];
} MusaDwgAc1018LinearWcsBounds;
int musa_dwg_ac1018_scene_read_linear_wcs_bounds(
    const MusaDwgAc1018Scene *scene,uint64_t handle,
    MusaDwgAc1018LinearWcsBounds *out);
/* Analytic WCS XYZ extents of a genuinely CRC-decoded original R2004
 * ARC (17) or CIRCLE (18) curve and optional signed-thickness extrusion,
 * including the arbitrary-axis OCS-to-WCS transform. For an ARC include
 * BOTH true endpoints plus all interior axis extrema; never approximate
 * by endpoints/tessellation. The exact translated sweep along the unit
 * extrusion axis expands per-coordinate bounds for nonzero thickness.
 * Reject degenerate/overlong ARC sweeps, invalid angles/nonfinite geometry
 * and unsupported entity types atomically. Not expanded for lineweight,
 * linetype strokes or layer/visibility; no complete CAD semantics.
 * No DWG writer, external CAD SDK or real-file parity is asserted.
 */
typedef struct MusaDwgAc1018CurveWcsBounds {
    uint64_t handle;
    uint16_t type_code; /* 17 ARC, 18 CIRCLE */
    double minimum[3];
    double maximum[3];
    double center_wcs[3];
    double radius;
    double thickness; /* signed OCS-normal extrusion included in bounds */
    double start_angle;
    double sweep_angle; /* full turn for CIRCLE */
} MusaDwgAc1018CurveWcsBounds;
int musa_dwg_ac1018_scene_read_curve_wcs_bounds(
    const MusaDwgAc1018Scene *scene,uint64_t handle,
    MusaDwgAc1018CurveWcsBounds *out);
#ifdef __cplusplus
}
#endif
#endif
