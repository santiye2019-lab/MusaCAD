/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_object_type.h"
#include "musacad_dwg_ac1032_record.h"
#include "musacad_dwg_ac1032_handles.h"
#include "musacad_dwg_ac1032_sections.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MUSA_R2018_INPUT_LIMIT (64u*1024u*1024u)
#define MUSA_R2018_TYPE_HANDLES_LIMIT 1000000u

static int overlap(const void *first,size_t n,const void *second,size_t m){
    if(!n||!m)return 0;
    const uintptr_t a=(uintptr_t)first,b=(uintptr_t)second;
    return a<=b?b-a<n:a-b<m;
}
static int read_ot_msb(const uint8_t *raw,size_t payload_bytes,
                       uint16_t *code,uint8_t *opcode,uint8_t *width){
    if(!raw||!code||!opcode||!width||!payload_bytes)return 0;
    /* The DWG object stream stores the 2-bit OT selector MSB-first.
     * Raw bytes are read across (not rounded up to) bit boundaries. */
    const uint8_t first=raw[0];
    const uint8_t mode=first>>6u;
    if(mode==3u)return 0; /* reserved (fail closed) */
    const unsigned raw_width=mode==2u?16u:8u;
    if((uint64_t)payload_bytes*8u<2u+(uint64_t)raw_width)return 0;
    uint32_t value=0u;
    for(unsigned i=0u;i<raw_width;i++){
        const size_t at=2u+(size_t)i;
        const uint8_t b=(raw[at/8u]>>(7u-(at%8u)))&1u;
        /* raw short value is 2 x little-endian raw 8-bit groups */
        if(mode==2u)value|=(uint32_t)b << ((i/8u)*8u+(7u-i%8u));
        else value=(value<<1u)|b;
    }
    if(mode==1u)value+=0x1f0u;
    if(!value||value>UINT16_MAX)return 0;
    *code=(uint16_t)value;
    *opcode=mode;
    *width=(uint8_t)(2u+raw_width);
    return 1;
}
int musa_dwg_ac1032_object_type_bytes(
    const uint8_t *objects,size_t logical_bytes,
    uint64_t indexed_handle,uint64_t section_offset,
    MusaDwgAc1032ObjectType *out){
    if(!objects||!out||!indexed_handle||
       logical_bytes<8u||logical_bytes>MUSA_R2018_INPUT_LIMIT||
       section_offset>=logical_bytes||
       overlap(objects,logical_bytes,out,sizeof(*out)))return 0;
    MusaDwgAc1032RecordEnvelope verified={0};
    if(!musa_dwg_ac1032_record_envelope_bytes(
          objects,logical_bytes,indexed_handle,section_offset,&verified)||
       !verified.record_crc_verified||
       verified.total_record_bytes>logical_bytes-(size_t)section_offset)
        return 0;
    const size_t begin=(size_t)section_offset+
        (size_t)verified.ms_field_bytes+(size_t)verified.mc_field_bytes;
    if(begin>logical_bytes ||
       verified.payload_bytes>logical_bytes-begin)return 0;
    MusaDwgAc1032ObjectType candidate={0};
    if(!read_ot_msb(objects+begin,verified.payload_bytes,
                    &candidate.type_code,&candidate.ot_opcode,
                    &candidate.type_field_bits))return 0;
    candidate.indexed_handle=indexed_handle;
    candidate.object_section_offset=section_offset;
    candidate.record_crc_verified=1u;
    *out=candidate;return 1;
}
int musa_dwg_ac1032_object_type_file(
    const uint8_t *file,size_t file_bytes,uint64_t handle,
    MusaDwgAc1032ObjectType *out){
    if(!file||!out||!handle||file_bytes<256u||
       file_bytes>MUSA_R2018_INPUT_LIMIT||
       memcmp(file,"AC1032",6u)!=0||
       overlap(file,file_bytes,out,sizeof(*out)))return 0;
    size_t count=0u;
    if(!musa_dwg_ac1032_handles_index(file,file_bytes,NULL,0u,&count)||
       !count||count>MUSA_R2018_TYPE_HANDLES_LIMIT||
       count>SIZE_MAX/sizeof(MusaDwgAc1032HandleEntry))return 0;
    MusaDwgAc1032HandleEntry *entries=calloc(count,sizeof(*entries));
    if(!entries)return 0;
    size_t listed=0u;
    int ok=musa_dwg_ac1032_handles_index(
             file,file_bytes,entries,count,&listed)&&listed==count;
    size_t low=0u,high=count;
    if(ok){
        while(low<high){
            const size_t mid=low+(high-low)/2u;
            if(entries[mid].handle<handle)low=mid+1u;
            else high=mid;
        }
        if(low==count||entries[low].handle!=handle)ok=0;
    }
    const uint64_t offset=ok?entries[low].object_offset:0u;
    free(entries);
    if(!ok)return 0;
    size_t names=0u,matched=0u,object_bytes=0u;
    if(!musa_dwg_ac1032_section_catalog(
        file,file_bytes,NULL,0u,&names)||!names||names>256u)return 0;
    MusaDwgAc1032SectionCatalogEntry *catalog=calloc(names,sizeof(*catalog));
    if(!catalog)return 0;
    size_t got=0u;
    ok=musa_dwg_ac1032_section_catalog(
       file,file_bytes,catalog,names,&got)&&got==names;
    if(ok)for(size_t i=0u;i<names;i++){
        if(!strcmp((const char*)catalog[i].name,"AcDb:AcDbObjects")){
            matched++;
            if(!catalog[i].logical_bytes||
               catalog[i].logical_bytes>MUSA_R2018_INPUT_LIMIT)ok=0;
            else object_bytes=(size_t)catalog[i].logical_bytes;
        }
    }
    free(catalog);
    if(!ok||matched!=1u)return 0;
    uint8_t *objects=malloc(object_bytes);
    if(!objects)return 0;
    size_t bytes_read=0u;
    MusaDwgAc1032ObjectType candidate={0};
    ok=musa_dwg_ac1032_extract_named_section(
             file,file_bytes,"AcDb:AcDbObjects",objects,object_bytes,
             &bytes_read)&&bytes_read==object_bytes&&
       musa_dwg_ac1032_object_type_bytes(
             objects,object_bytes,handle,offset,&candidate);
    free(objects);
    if(!ok)return 0;
    *out=candidate;return 1;
}
