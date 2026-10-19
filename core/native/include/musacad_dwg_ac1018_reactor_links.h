/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_REACTOR_LINKS_H
#define MUSACAD_DWG_AC1018_REACTOR_LINKS_H
#include "musacad_dwg_ac1015_handles.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* First-party read-only AC1018/R2004 common entity-link audit for
 * ARC (17), CIRCLE (18), LINE (19), and LWPOLYLINE (77)
 * records with reactor references in the common-header subset.
 *
 * Inputs: independently decompressed AcDbObjects section, strictly ordered
 * CRC-backed AcDb:Handles index (entries use section-relative offsets).
 * Validates MS+CRC, self handle, EED bounds, H link roles and ALL target
 * memberships before returning. Never modifies DWG bytes. No CAD SDK.
 *
 * This is a *link reference* verifier, not a claim that owner/extension
 * dictionary/layer object types have been decoded or that geometry fields
 * are correct. Existing geometry reader fail-closed behavior stays intact.
 */
typedef struct MusaDwgAc1018ReactorAudit {
    uint64_t handle;
    uint64_t owner_handle; /* zero when entity mode has no owner H */
    uint64_t layer_handle;
    uint64_t xdictionary_handle; /* zero when absent */
    uint32_t reactor_count;
    uint32_t eed_raw_bytes_skipped;
    uint8_t entity_mode;
    uint8_t record_crc_verified;
    uint8_t handle_membership_verified;
    uint8_t geometry_decoded; /* Always zero in this API. */
} MusaDwgAc1018ReactorAudit;
/* Output and reactor_targets are byte-for-byte unchanged on error.
 * Reactor count must be in [1,64]. Capacity >= reactor count.
 * This handles the verified common R2004 subset: mode 0 or 2,
 * owner C-H for mode 0, reactors code 4, optional XDict code 3,
 * layer code 5, at most seven final pad bits. Other entities refuse.
 */
int musa_dwg_ac1018_read_reactor_links(
    const uint8_t *objects, size_t object_bytes, size_t record_offset,
    uint64_t indexed_handle,
    const MusaDwgAc1015HandleEntry *index, size_t index_count,
    uint64_t *reactor_targets, size_t reactor_capacity,
    MusaDwgAc1018ReactorAudit *out);
/* Same strict CRC/self-handle/member audit for ARC/CIRCLE/LINE/
 * LWPOLYLINE records
 * whose encoded BL reactor count is exactly ZERO. This is a separate entry
 * point so the original positive-reactor API retains its fail-closed scope.
 * Owner (C H), optional XDictionary (3 H) and layer (5 H) references are
 * checked for sorted-index membership, NOT for actual target class/type.
 * On failure *out and source bytes remain untouched. */
int musa_dwg_ac1018_read_zero_reactor_links(
    const uint8_t *objects,size_t object_bytes,size_t record_offset,
    uint64_t indexed_handle,
    const MusaDwgAc1015HandleEntry *index,size_t index_count,
    MusaDwgAc1018ReactorAudit *out);
#ifdef __cplusplus
}
#endif
#endif
