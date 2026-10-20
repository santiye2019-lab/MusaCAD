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
#ifdef __cplusplus
}
#endif
#endif
