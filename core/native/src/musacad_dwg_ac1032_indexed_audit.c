/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_indexed_audit.h"
#include "musacad_dwg_ac1032_handles.h"
#include "musacad_dwg_ac1032_object_type.h"
#include "musacad_dwg_ac1032_record.h"
#include "musacad_dwg_ac1032_sections.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define AUDIT_FILE_MAX (64u * 1024u * 1024u)
#define AUDIT_RECORD_MAX 1000000u
#define AUDIT_SECTION_MAX (64u * 1024u * 1024u)
typedef struct ByteRange {
    uint64_t start,end;
} ByteRange;
static int overlap(const void *a,size_t n,const void *b,size_t m){
    if(!n||!m)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<n:x-y<m;
}
static int range_cmp(const void *a,const void *b){
    const ByteRange *x=(const ByteRange*)a,*y=(const ByteRange*)b;
    return (x->start>y->start)-(x->start<y->start);
}
static uint8_t classify(uint16_t ot){
    switch(ot){
        case 16u: /* 3D POLYLINE */
        case 17u: /* ARC */
        case 18u: /* CIRCLE */
        case 19u: /* LINE */
        case 27u: /* POINT */
        case 77u: /* LWPOLYLINE */
            return MUSA_AC1032_INDEXED_GEOMETRY_CANDIDATE;
        case 6u:  /* SEQEND, not independent drawing geometry */
        case 11u: /* VERTEX3D: requires verified owner graph */
            return MUSA_AC1032_INDEXED_DEPENDENT;
        default: return MUSA_AC1032_INDEXED_OPAQUE;
    }
}
int musa_dwg_ac1032_audit_indexed_objects_file(
    const uint8_t *file,size_t file_bytes,
    MusaDwgAc1032IndexedRecord *rows,size_t capacity,
    MusaDwgAc1032IndexedAudit *out){
    if(!file||!out||file_bytes<256u||file_bytes>AUDIT_FILE_MAX||
       memcmp(file,"AC1032",6u)!=0||
       (!rows&&capacity)||
       capacity>SIZE_MAX/sizeof(*rows)||
       overlap(file,file_bytes,out,sizeof(*out))||
       (rows&&(overlap(file,file_bytes,rows,capacity*sizeof(*rows))||
               overlap(rows,capacity*sizeof(*rows),out,sizeof(*out)))))
       return 0;
    size_t count=0u;
    if(!musa_dwg_ac1032_handles_index(file,file_bytes,NULL,0u,&count)||
       !count||count>AUDIT_RECORD_MAX||
       count>SIZE_MAX/sizeof(MusaDwgAc1032HandleEntry)||
       count>SIZE_MAX/sizeof(ByteRange)||
       (rows&&capacity<count))return 0;

    MusaDwgAc1032HandleEntry *index=NULL;
    MusaDwgAc1032IndexedRecord *stage=NULL;
    ByteRange *ranges=NULL;
    MusaDwgAc1032SectionCatalogEntry *catalog=NULL;
    uint8_t *objects=NULL;
    int success=0;
    index=(MusaDwgAc1032HandleEntry*)malloc(
        count*sizeof(*index));
    ranges=(ByteRange*)malloc(count*sizeof(*ranges));
    if(rows)stage=(MusaDwgAc1032IndexedRecord*)calloc(
        count,sizeof(*stage));
    if(!index||!ranges||(rows&&!stage))goto done;
    size_t got=0u;
    if(!musa_dwg_ac1032_handles_index(
           file,file_bytes,index,count,&got)||got!=count)
        goto done;
    size_t sections=0u;
    if(!musa_dwg_ac1032_section_catalog(
           file,file_bytes,NULL,0u,&sections)||
       !sections||sections>256u)goto done;
    catalog=(MusaDwgAc1032SectionCatalogEntry*)calloc(
        sections,sizeof(*catalog));
    if(!catalog||
       !musa_dwg_ac1032_section_catalog(
            file,file_bytes,catalog,sections,&got)||
       got!=sections)goto done;
    size_t object_bytes=0u,found=0u;
    for(size_t i=0u;i<sections;i++){
        if(!catalog[i].all_physical_page_checksums_verified)
            goto done;
        if(strcmp((const char*)catalog[i].name,"AcDb:AcDbObjects")==0){
            if(++found!=1u||
               !catalog[i].logical_bytes||
               catalog[i].logical_bytes>AUDIT_SECTION_MAX)goto done;
            object_bytes=(size_t)catalog[i].logical_bytes;
        }
    }
    if(found!=1u)goto done;
    objects=(uint8_t*)malloc(object_bytes);
    if(!objects||
       !musa_dwg_ac1032_extract_named_section(
           file,file_bytes,"AcDb:AcDbObjects",
           objects,object_bytes,&got)||got!=object_bytes)goto done;
    MusaDwgAc1032IndexedAudit checked={0};
    checked.indexed_records=count;
    uint64_t prev_handle=0u;
    for(size_t i=0u;i<count;i++){
        const uint64_t h=index[i].handle;
        const uint64_t off=index[i].object_offset;
        if(!h||h<=prev_handle||off>=object_bytes)goto done;
        prev_handle=h;
        MusaDwgAc1032RecordEnvelope rec={0};
        MusaDwgAc1032ObjectType ot={0};
        if(!musa_dwg_ac1032_record_envelope_bytes(
              objects,object_bytes,h,off,&rec)||
           !musa_dwg_ac1032_object_type_bytes(
              objects,object_bytes,h,off,&ot)||
           !rec.record_crc_verified||!ot.record_crc_verified||
           rec.indexed_handle!=h||ot.indexed_handle!=h||
           rec.object_section_offset!=off||
           ot.object_section_offset!=off||
           !rec.total_record_bytes||
           rec.total_record_bytes>object_bytes-off)
           goto done;
        ranges[i].start=off;
        ranges[i].end=off+rec.total_record_bytes;
        const uint8_t category=classify(ot.type_code);
        if(category==MUSA_AC1032_INDEXED_GEOMETRY_CANDIDATE)
            checked.geometry_candidates++;
        else if(category==MUSA_AC1032_INDEXED_DEPENDENT)
            checked.dependent_records++;
        else checked.opaque_records++;
        if(stage){
            stage[i].indexed_handle=h;
            stage[i].object_section_offset=off;
            stage[i].framed_record_bytes=rec.total_record_bytes;
            stage[i].raw_object_type=ot.type_code;
            stage[i].category=category;
            stage[i].record_crc_verified=1u;
        }
    }
    qsort(ranges,count,sizeof(*ranges),range_cmp);
    for(size_t i=1u;i<count;i++){
        if(ranges[i-1u].end>ranges[i].start)goto done;
    }
    checked.all_indexed_record_crcs_verified=1u;
    checked.no_indexed_record_overlap=1u;
    if(rows)memcpy(rows,stage,count*sizeof(*rows));
    *out=checked;
    success=1;
done:
    free(objects);
    free(catalog);
    free(ranges);
    free(stage);
    free(index);
    return success;
}
