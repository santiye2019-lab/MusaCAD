/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_BLOCK_OWNER_GAP_CENSUS_H
#define MUSACAD_DWG_AC1032_BLOCK_OWNER_GAP_CENSUS_H
#include "musacad_dwg_ac1032_scene_index.h"
#include <stdint.h>

/* Source-only forensic histogram for the six supported OT child families.
 * Each slot is based on a separately CRC/H-checked original OT49 inventory.
 * No failure is converted into verified ownership or paint permission. */
#define MUSACAD_AC1032_OWNER_GAP_FAMILIES 6u
typedef struct MusaDwgAc1032BlockOwnerGapCensus {
    uint64_t insert_handle,block_header_handle;
    uint16_t supported_types[MUSACAD_AC1032_OWNER_GAP_FAMILIES];
    uint32_t verified[MUSACAD_AC1032_OWNER_GAP_FAMILIES];
    uint32_t unreadable[MUSACAD_AC1032_OWNER_GAP_FAMILIES];
    uint32_t mismatched[MUSACAD_AC1032_OWNER_GAP_FAMILIES];
    uint64_t first_unreadable_handle[MUSACAD_AC1032_OWNER_GAP_FAMILIES];
    uint32_t unsupported_count,child_count;
    uint8_t source_inventory_and_statuses_verified;
    uint8_t final_visibility_color_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockOwnerGapCensus;

static inline int musa_dwg_ac1032_block_owner_gap_census(
    const MusaDwgAc1032InsertBlockOwnerCoverage *coverage,
    MusaDwgAc1032BlockOwnerGapCensus *out){
    if(!coverage||!out||!coverage->source_inventory_verified||
       coverage->final_display_accepted||
       !coverage->insert_handle||!coverage->block_header_handle||
       !coverage->child_count||
       coverage->child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT||
       !musa_dwg_ac1032_block_child_handles_unique(
           coverage->child_handles,coverage->child_count))return 0;
    MusaDwgAc1032BlockOwnerGapCensus local={0};
    const uint16_t families[MUSACAD_AC1032_OWNER_GAP_FAMILIES]={
        17u,18u,19u,27u,35u,77u};
    local.insert_handle=coverage->insert_handle;
    local.block_header_handle=coverage->block_header_handle;
    local.child_count=coverage->child_count;
    for(unsigned i=0u;i<MUSACAD_AC1032_OWNER_GAP_FAMILIES;i++)
        local.supported_types[i]=families[i];
    uint32_t verified=0u,unreadable=0u,mismatched=0u;
    for(uint32_t i=0u;i<coverage->child_count;i++){
        const uint16_t type=coverage->child_types[i];
        const uint8_t status=coverage->child_status[i];
        unsigned family=MUSACAD_AC1032_OWNER_GAP_FAMILIES;
        for(unsigned j=0u;j<MUSACAD_AC1032_OWNER_GAP_FAMILIES;j++)
            if(families[j]==type){family=j;break;}
        if(family==MUSACAD_AC1032_OWNER_GAP_FAMILIES){
            if(status!=MUSACAD_AC1032_BLOCK_OWNER_UNSUPPORTED)return 0;
            local.unsupported_count++;
            continue;
        }
        switch(status){
            case MUSACAD_AC1032_BLOCK_OWNER_VERIFIED:
                local.verified[family]++;verified++;break;
            case MUSACAD_AC1032_BLOCK_OWNER_UNREADABLE:
                local.unreadable[family]++;unreadable++;
                if(!local.first_unreadable_handle[family])
                    local.first_unreadable_handle[family]=
                        coverage->child_handles[i];
                break;
            case MUSACAD_AC1032_BLOCK_OWNER_MISMATCH:
                local.mismatched[family]++;mismatched++;break;
            default:return 0;
        }
    }
    if(verified!=coverage->verified_count||
       unreadable!=coverage->unreadable_count||
       mismatched!=coverage->mismatched_count||
       local.unsupported_count!=coverage->unsupported_count||
       verified+unreadable+mismatched+local.unsupported_count!=
           coverage->child_count||
       !!coverage->complete_reverse_owner_links_verified!=
           (verified==coverage->child_count))return 0;
    local.source_inventory_and_statuses_verified=1u;
    *out=local;
    return 1;
}
#endif
