/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Single platform-independent R2004 original-binary admission entry point. */
#include "musacad_dwg_ac1018_inspection.h"
#include "musacad_dwg_ac1018_scene.h"
#include "musacad_dwg_ac1018_sections.h"
#include "musacad_dwg_signature.h"
#include <stdint.h>
#include <string.h>

static int disjoint(const void *p,size_t n,const void *o,size_t m){
    const uintptr_t a=(uintptr_t)p,b=(uintptr_t)o;
    if(!n||!m)return 1;
    return !((a<=b&&b-a<n)||(b<a&&a-b<m));
}
int musa_dwg_ac1018_inspect_file(
    const uint8_t *file,size_t n,MusaDwgAc1018FileInspection *out){
    /* This is a bounded in-memory Android diagnostic, not the large-file
     * streaming reader. Defend the same core API on Windows/iOS callers. */
    if(!file||!out||n<256u||n>32u*1024u*1024u||
       !disjoint(file,n,out,sizeof(*out))||
       musa_core_dwg_identify_signature(file,n)!=MUSA_DWG_2004)
        return 0;
    MusaDwgAc1018SectionDescriptor objects={0},handles={0};
    size_t object_pages=0u,handle_pages=0u;
    if(!musa_dwg_ac1018_section_pages(
                file,n,MUSA_DWG_AC1018_OBJECTS,NULL,0u,
                &objects,&object_pages)||
       !musa_dwg_ac1018_section_pages(
                file,n,MUSA_DWG_AC1018_HANDLES,NULL,0u,
                &handles,&handle_pages)||
       !object_pages||!handle_pages||
       objects.logical_bytes>64u*1024u*1024u||
       handles.logical_bytes>32u*1024u*1024u)
        return 0;
    MusaDwgAc1018Scene *scene=NULL;
    if(!musa_dwg_ac1018_scene_open(file,n,&scene)||!scene)
        return 0;
    MusaDwgAc1018SceneInventory src={0};
    MusaDwgAc1018RecordRangeAudit ranges={0};
    const int valid=musa_dwg_ac1018_scene_inventory(scene,0,&src)&&
              musa_dwg_ac1018_scene_audit_record_ranges(scene,&ranges)&&
              src.all_indexed_record_crcs_verified&&
              ranges.ranges_nonoverlapping&&
              src.indexed_record_count==ranges.indexed_records&&
              src.indexed_record_count>0u&&
              src.decoded_lines+src.decoded_arcs+src.decoded_circles+
              src.decoded_lwpolylines+
              src.undecodable_supported_records+
              src.other_declared_records==src.indexed_record_count;
    MusaDwgAc1018FileInspection result={0};
    if(valid){
        result.indexed_records=src.indexed_record_count;
        result.decoded_lines=src.decoded_lines;
        result.decoded_arcs=src.decoded_arcs;
        result.decoded_circles=src.decoded_circles;
        result.decoded_lwpolylines=src.decoded_lwpolylines;
        result.undecodable_supported_records=src.undecodable_supported_records;
        result.other_declared_records=src.other_declared_records;
        result.covered_record_bytes=ranges.covered_record_bytes;
        result.unindexed_section_bytes=ranges.unindexed_section_bytes;
        result.all_record_crcs_verified=1u;
        result.record_ranges_nonoverlapping=1u;
    }
    musa_dwg_ac1018_scene_close(scene);
    if(!valid)return 0;
    *out=result;
    return 1;
}
