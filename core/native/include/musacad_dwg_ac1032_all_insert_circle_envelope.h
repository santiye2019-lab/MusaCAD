/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_ALL_INSERT_CIRCLE_ENVELOPE_H
#define MUSACAD_DWG_AC1032_ALL_INSERT_CIRCLE_ENVELOPE_H
#include "musacad_dwg_ac1032_insert_circle_source_envelope.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Source-only all-instance numeric audit for original R2018 OT7 INSERTs.
 * Repeated BLOCK_HEADERs are independent placements.  Numeric CIRCLE
 * envelopes are not evidence of owner/layer/space/viewport/paint.
 * Unknown H4/H5 remains UNKNOWN; do not create v6 draw records. */
#define MUSACAD_AC1032_ALL_CIRCLE_MAX_INSERTS 4096u
typedef struct MusaDwgAc1032AllInsertCircleEnvelope {
    size_t total_indexed_records,candidate_insert_records;
    size_t verified_insert_records,unsupported_insert_records;
    size_t circle_candidate_records,verified_circle_records;
    size_t unverified_circle_records,h4_h5_unresolved_circle_records;
    double combined_min_xy[2],combined_max_xy[2];
    uint8_t has_numeric_bounds;
    uint8_t source_only_partial; /* ALWAYS ONE */
    uint8_t final_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032AllInsertCircleEnvelope;

static inline int musa_dwg_ac1032_all_insert_circle_envelope(
    const MusaDwgAc1032SceneIndex *scene,
    MusaDwgAc1032AllInsertCircleEnvelope *out){
    if(!scene||!out)return 0;
    const size_t count=musa_dwg_ac1032_scene_index_count(scene);
    if(!count||count>1000000u)return 0;
    MusaDwgAc1032AllInsertCircleEnvelope v={0};
    v.total_indexed_records=count;
    v.source_only_partial=1u;
    uint64_t previous=0u;
    for(size_t i=0u;i<count;i++){
        MusaDwgAc1032HandleEntry entry={0};
        uint16_t type=0u;
        if(!musa_dwg_ac1032_scene_index_entry(scene,i,&entry)||
           !entry.handle||entry.handle<=previous||
           !musa_dwg_ac1032_scene_index_object_type(
               scene,entry.handle,&type))return 0;
        previous=entry.handle;
        if(type!=7u)continue;
        if(v.candidate_insert_records>=
           MUSACAD_AC1032_ALL_CIRCLE_MAX_INSERTS)return 0;
        v.candidate_insert_records++;
        MusaDwgAc1032InsertCircleEnvelope one={0};
        if(!musa_dwg_ac1032_insert_circle_source_envelope(
               scene,entry.handle,&one)){
            v.unsupported_insert_records++;
            continue;
        }
        if(one.insert_handle!=entry.handle||
           !one.block_header_handle||
           !one.original_numeric_source_bounds_verified||
           one.final_visibility_and_paint_accepted||
           one.verified_circles+one.unverified_circles!=
               one.circle_candidates||
           (one.verified_circles>0u)!=(one.has_numeric_bounds!=0u)||
           one.h4_h5_unresolved_circles>one.verified_circles)return 0;
        v.verified_insert_records++;
        if(v.circle_candidate_records>SIZE_MAX-one.circle_candidates||
           v.verified_circle_records>SIZE_MAX-one.verified_circles||
           v.unverified_circle_records>SIZE_MAX-one.unverified_circles||
           v.h4_h5_unresolved_circle_records>
               SIZE_MAX-one.h4_h5_unresolved_circles)return 0;
        v.circle_candidate_records+=one.circle_candidates;
        v.verified_circle_records+=one.verified_circles;
        v.unverified_circle_records+=one.unverified_circles;
        v.h4_h5_unresolved_circle_records+=
            one.h4_h5_unresolved_circles;
        if(one.has_numeric_bounds){
            for(unsigned k=0u;k<2u;k++){
                const double lo=one.combined_min_xy[k];
                const double hi=one.combined_max_xy[k];
                if(!isfinite(lo)||!isfinite(hi)||lo>hi)return 0;
                if(!v.has_numeric_bounds){
                    v.combined_min_xy[k]=lo;
                    v.combined_max_xy[k]=hi;
                }else{
                    if(lo<v.combined_min_xy[k])v.combined_min_xy[k]=lo;
                    if(hi>v.combined_max_xy[k])v.combined_max_xy[k]=hi;
                }
            }
            v.has_numeric_bounds=1u;
        }
    }
    if(v.candidate_insert_records!=
           v.verified_insert_records+v.unsupported_insert_records||
       v.circle_candidate_records!=
           v.verified_circle_records+v.unverified_circle_records||
       (v.verified_circle_records>0u)!=(v.has_numeric_bounds!=0u))
        return 0;
    *out=v;
    return 1;
}
#endif
