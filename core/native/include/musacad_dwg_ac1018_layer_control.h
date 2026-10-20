/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_LAYER_CONTROL_H
#define MUSACAD_DWG_AC1018_LAYER_CONTROL_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Bounded first-party R2004/AC1018 LAYER_CONTROL (numerical type 50)
 * record grammar. Strict supported subset: zero reactors, missing XDict,
 * NULL root soft pointer (4:0) and 1..4096 absolute 2:H soft-owned LAYER
 * references. CRC16, original self-H and prehandle bit boundary are checked.
 * The references are NOT yet verified as LAYER objects in the indexed scene.
 * This does not claim the whole DWG table/class graph or AutoCAD parity.
 */
typedef struct MusaDwgAc1018LayerControlRefs {
    uint64_t indexed_handle;
    uint32_t entry_count;
    uint8_t crc_verified;
    uint8_t embedded_self_verified;
    uint8_t supported_handle_roles_verified;
} MusaDwgAc1018LayerControlRefs;
/* Pass entry_handles=NULL/capacity=0 for a fully validated count query.
 * Otherwise require capacity>=entry_count. Source and caller outputs are
 * untouched on failure; output ranges must not alias each other or source.
 */
int musa_dwg_ac1018_layer_control_refs_record(
    const uint8_t *objects,size_t object_bytes,size_t object_offset,
    uint64_t indexed_handle,uint64_t *entry_handles,size_t capacity,
    MusaDwgAc1018LayerControlRefs *out);
#ifdef __cplusplus
}
#endif
#endif
