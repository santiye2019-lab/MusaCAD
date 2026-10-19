/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_SCENE_H
#define MUSACAD_DWG_AC1018_SCENE_H
#include <stddef.h>
#include <stdint.h>
#include "musacad_dwg_ac1018_line.h"
#include "musacad_dwg_ac1018_curve.h"
#include "musacad_dwg_ac1018_lwpolyline.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Read-only, first-party portable C11 R2004 (AC1018) geometry session.
 * Open verifies/decompresses both protected sections once, constructs and
 * CRC-validates the complete bounded handle index, then supports many
 * O(log N) handle lookups without repeatedly decompressing the source.
 * On failure *out remains unchanged, source bytes are never changed.
 * The returned session is opaque and owns its decoded buffers.
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
 * indexed AC1018 LINE/LWPOLYLINE record, without accepting geometry.
 * Supports zero or 1..64 reactors and preserves the original positive-only
 * API semantics. All ref targets must exist in the CRC-verified scene index.
 * Caller buffers, scene, and all DWG bytes remain untouched on failure.
 * Targets are numeric H references only: LAYER table and owner target
 * CLASSES are NOT yet decoded or semantically proven.
 *
 * The map adapter is temporary and O(index_count) per audit call. Geometry
 * reads remain O(log N); bulk link audit optimization comes separately.
 */
int musa_dwg_ac1018_scene_audit_links(
    const MusaDwgAc1018Scene *scene,uint64_t handle,
    uint64_t *reactor_targets,size_t reactor_capacity,
    MusaDwgAc1018ReactorAudit *out);
/* Strengthen an already supported LINE/LWPOLYLINE H-link audit:
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
#ifdef __cplusplus
}
#endif
#endif
