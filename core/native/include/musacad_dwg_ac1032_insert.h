/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_H
#define MUSACAD_DWG_AC1032_INSERT_H
#include "musacad_dwg_ac1032_object_streams.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original OT7 INSERT R2000+ numeric DATA only. Not BLOCK_HEADER H graph,
 * attribute ownership, block base point, nested transform or rendering.
 * Scale/rotation/OCS are source values; no model-space display acceptance. */
typedef struct MusaDwgAc1032InsertPlacement {
    uint64_t indexed_handle;
    double insertion[3],scale[3],rotation,extrusion[3];
    uint32_t owned_attrib_count;
    uint8_t scale_data_flags;
    uint8_t has_attribs;
    uint8_t original_record_crc_verified;
    uint8_t numeric_fields_decoded;
    uint8_t block_graph_and_display_accepted; /* ALWAYS 0 */
} MusaDwgAc1032InsertPlacement;
/* Source-only original R2018 INSERT H graph: first H5 LAYER, next H5
 * BLOCK_HEADER, optional one H3 ATTRIB and H3 SEQEND. No block name,
 * owner/model-space display, recursion or attribute text is resolved. */
typedef struct MusaDwgAc1032InsertSourceGraph {
    uint64_t indexed_handle;
    uint64_t layer_handle;
    uint64_t block_header_handle;
    uint64_t attrib_handle;
    uint64_t seqend_handle;
    uint32_t owned_attrib_count;
    uint8_t original_entity_and_targets_crc_verified;
    uint8_t source_edges_verified;
    uint8_t block_expansion_and_display_accepted; /* ALWAYS 0 */
} MusaDwgAc1032InsertSourceGraph;
/* The caller must supply an independently CRC/H-authenticated original
 * payload and the corresponding common-entity DATA start. Any unsupported
 * tail, malformed numeric field, alias or unconsumed bit fails atomically. */
int musa_dwg_ac1032_insert_placement_at(
    const uint8_t *payload,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,size_t geometry_start_bit,
    MusaDwgAc1032InsertPlacement *out);
#ifdef __cplusplus
}
#endif
#endif
