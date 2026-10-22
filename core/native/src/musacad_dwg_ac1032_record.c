/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_record.h"
#include "musacad_dwg_ac1032_handles.h"
#include "musacad_dwg_ac1032_sections.h"
#include "musacad_crc16.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define RECORD_SOURCE_MAX (64u*1024u*1024u)
#define RECORD_HANDLES_MAX 1000000u

static int overlap(const void *p,size_t pn,const void *q,size_t qn){
    if(!pn||!qn)return 0;
    const uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;
    return a<=b?b-a<pn:a-b<qn;
}
/* R2018 common MS: little-endian 15-bit groups, continuation in bit15;
 * second group MUST be final and non-zero (canonical representation). */
static int read_ms(const uint8_t *src,size_t available,
                   size_t *field_bytes,uint32_t *length){
    if(available<2u||!src||!field_bytes||!length)return 0;
    uint32_t lo=(uint32_t)src[0]|((uint32_t)src[1]<<8u);
    uint32_t value=lo&0x7fffu;
    size_t fields=2u;
    if(lo&0x8000u){
        if(available<4u)return 0;
        const uint32_t hi=(uint32_t)src[2]|((uint32_t)src[3]<<8u);
        if(!hi||(hi&0x8000u))return 0;
        value|=hi<<15u;
        fields=4u;
    }
    if(value<2u)return 0;
    *field_bytes=fields;*length=value;
    return 1;
}
/* R2010+ MC is an UNSIGNED little-endian 7-bit modular char here;
 * unlike a signed object-map offset, terminal bit 0x40 is payload. */
static int read_mc(const uint8_t *p,size_t size,
                   size_t *bytes,uint32_t *bits){
    uint64_t value=0u;
    if(!p||!bytes||!bits)return 0;
    for(unsigned i=0u;i<5u&&i<size;i++){
        const uint8_t b=p[i];
        value|=(uint64_t)(b&0x7fu)<<(i*7u);
        if(!(b&0x80u)){
            if((i&&!(b&0x7fu))||value>UINT32_MAX)return 0;
            *bytes=(size_t)i+1u;*bits=(uint32_t)value;
            return 1;
        }
    }
    return 0;
}
int musa_dwg_ac1032_record_envelope_bytes(
    const uint8_t *section,size_t n,
    uint64_t handle,uint64_t at,MusaDwgAc1032RecordEnvelope *out){
    if(!section||!out||!handle||!n||n>RECORD_SOURCE_MAX||
       at>=n||overlap(section,n,out,sizeof(*out)))return 0;
    const size_t start=(size_t)at,remaining=n-start;
    size_t ms_bytes=0u,mc_bytes=0u;
    uint32_t payload=0u,handle_bits=0u;
    /* Genuine R2018 evidence: MS excludes the *separate* MC field.
     * The first public record's 2-byte MC shifts the original verified
     * CRC exactly +2 bytes; do not miscount the MC inside MS payload. */
    if(!read_ms(section+start,remaining,&ms_bytes,&payload)||
       remaining<ms_bytes+2u||
       !read_mc(section+start+ms_bytes,remaining-ms_bytes-2u,
                &mc_bytes,&handle_bits)||
       mc_bytes>remaining-ms_bytes-2u||
       (size_t)payload>remaining-ms_bytes-mc_bytes-2u||
       !handle_bits||
       (uint64_t)handle_bits>(uint64_t)payload*8u)return 0;
    const size_t crc_at=start+ms_bytes+mc_bytes+(size_t)payload;
    uint16_t computed=0u;
    if(!musa_core_crc16_arc(0xc0c1u,section+start,
                            ms_bytes+mc_bytes+(size_t)payload,&computed)||
       computed!=((uint16_t)section[crc_at]|
                   ((uint16_t)section[crc_at+1u]<<8u)))return 0;
    MusaDwgAc1032RecordEnvelope result={0};
    result.indexed_handle=handle;
    result.object_section_offset=at;
    result.payload_bytes=payload;
    result.handle_stream_bits=handle_bits;
    result.total_record_bytes=(uint32_t)(ms_bytes+mc_bytes+(size_t)payload+2u);
    result.ms_field_bytes=(uint8_t)ms_bytes;
    result.mc_field_bytes=(uint8_t)mc_bytes;
    result.record_crc_verified=1u;
    *out=result;return 1;
}
int musa_dwg_ac1032_record_envelope_file(
    const uint8_t *file,size_t file_bytes,uint64_t handle,
    MusaDwgAc1032RecordEnvelope *out){
    if(!file||!out||!handle||file_bytes<256u||
       file_bytes>RECORD_SOURCE_MAX||
       memcmp(file,"AC1032",6u)||
       overlap(file,file_bytes,out,sizeof(*out)))return 0;
    /* Count and then acquire the ENTIRE source-authenticated map, not
     * an unverified caller-supplied offset or an AC1018 object record. */
    size_t count=0u;
    if(!musa_dwg_ac1032_handles_index(file,file_bytes,NULL,0u,&count)||
       !count||count>RECORD_HANDLES_MAX||
       count>SIZE_MAX/sizeof(MusaDwgAc1032HandleEntry))return 0;
    MusaDwgAc1032HandleEntry *index=calloc(count,sizeof(*index));
    if(!index)return 0;
    size_t read_count=0u;
    int ok=musa_dwg_ac1032_handles_index(
        file,file_bytes,index,count,&read_count)&&read_count==count;
    size_t low=0u,high=count;
    if(ok){
        while(low<high){
            const size_t mid=low+(high-low)/2u;
            if(index[mid].handle<handle)low=mid+1u;
            else high=mid;
        }
        if(low==count||index[low].handle!=handle)ok=0;
    }
    uint64_t offset=ok?index[low].object_offset:0u;
    free(index);
    if(!ok)return 0;

    size_t entries_count=0u;
    if(!musa_dwg_ac1032_section_catalog(
         file,file_bytes,NULL,0u,&entries_count)||
       !entries_count||entries_count>256u)return 0;
    MusaDwgAc1032SectionCatalogEntry *entries=
       calloc(entries_count,sizeof(*entries));
    if(!entries)return 0;
    size_t got=0u,bytes=0u,found=0u;
    ok=musa_dwg_ac1032_section_catalog(file,file_bytes,
       entries,entries_count,&got)&&got==entries_count;
    if(ok)for(size_t i=0u;i<got;i++){
        if(!strcmp((const char*)entries[i].name,"AcDb:AcDbObjects")){
            found++;
            if(!entries[i].logical_bytes||
               entries[i].logical_bytes>RECORD_SOURCE_MAX)ok=0;
            else bytes=(size_t)entries[i].logical_bytes;
        }
    }
    free(entries);
    if(!ok||found!=1u)return 0;
    uint8_t *plain=malloc(bytes);
    if(!plain)return 0;
    size_t extracted=0u;
    MusaDwgAc1032RecordEnvelope record={0};
    ok=musa_dwg_ac1032_extract_named_section(file,file_bytes,
       "AcDb:AcDbObjects",plain,bytes,&extracted)&&
       extracted==bytes&&
       musa_dwg_ac1032_record_envelope_bytes(
           plain,bytes,handle,offset,&record);
    free(plain);
    if(!ok)return 0;
    *out=record;return 1;
}
