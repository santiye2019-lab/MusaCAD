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
#include <math.h>
/* Numeric-only 2D affine transform for a verified planar OT7 INSERT and
 * OT49 base point. No claim about reverse ownership, model space, XREF,
 * BYBLOCK, viewport, nested INSERT, or final display. Reject tilted OCS,
 * singular scales and nonfinite coordinates. Output is fail-atomic. */
static inline int musa_dwg_ac1032_insert_planar_affine(
    const MusaDwgAc1032InsertPlacement *insert,
    const double base_xyz[3],double out_six[6]){
    if(!insert||!base_xyz||!out_six||
       !insert->original_record_crc_verified||
       !insert->numeric_fields_decoded||
       insert->block_graph_and_display_accepted||
       insert->extrusion[0]!=0.0||insert->extrusion[1]!=0.0||
       insert->extrusion[2]!=1.0||
       insert->insertion[2]!=0.0||base_xyz[2]!=0.0||
       insert->scale[2]==0.0)return 0;
    for(unsigned i=0u;i<3u;i++){
        if(!isfinite(insert->insertion[i])||
           !isfinite(insert->scale[i])||!isfinite(base_xyz[i]))return 0;
    }
    if(!isfinite(insert->rotation)||
       insert->scale[0]==0.0||insert->scale[1]==0.0)return 0;
    const double co=cos(insert->rotation),si=sin(insert->rotation);
    if(!isfinite(co)||!isfinite(si))return 0;
    const double a=co*insert->scale[0],b=-si*insert->scale[1];
    const double c=si*insert->scale[0],d=co*insert->scale[1];
    const double tx=insert->insertion[0]-a*base_xyz[0]-b*base_xyz[1];
    const double ty=insert->insertion[1]-c*base_xyz[0]-d*base_xyz[1];
    const double local[6]={a,b,tx,c,d,ty};
    for(unsigned i=0u;i<6u;i++)if(!isfinite(local[i]))return 0;
    for(unsigned i=0u;i<6u;i++)out_six[i]=local[i];
    return 1;
}
#ifdef __cplusplus
}
#endif
#endif
