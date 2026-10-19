/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_SCENE_H
#define MUSACAD_DWG_AC1018_SCENE_H
#include <stddef.h>
#include <stdint.h>
#include "musacad_dwg_ac1018_line.h"
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
int musa_dwg_ac1018_scene_read_line(const MusaDwgAc1018Scene *scene,
      uint64_t handle,MusaDwgAc1018Line *out);
/* Reads 1..100000 OCS vertices and metadata, never partial output.
 * Caller can convert OCS vertex + elevation/extrusion to WCS using the
 * existing musa_dwg_ac1018_lwpolyline_vertex_to_wcs().
 * Output vertex array and metadata must not overlap.
 */
int musa_dwg_ac1018_scene_read_lwpolyline(
      const MusaDwgAc1018Scene *scene,uint64_t handle,
      MusaDwgAc1018LWVertex *vertices,size_t vertex_capacity,
      MusaDwgAc1018LWPolyline *out);
#ifdef __cplusplus
}
#endif
#endif
