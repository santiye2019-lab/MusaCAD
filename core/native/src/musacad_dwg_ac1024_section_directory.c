/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1024_section_directory.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define MAX_SECTION_BYTES 1048576u
#define MAX_SECTIONS 256u
#define MAX_REFERENCES 65536u
static uint32_t read32(const uint8_t *p){
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8u) |
           ((uint32_t)p[2]<<16u) | ((uint32_t)p[3]<<24u);
}
static uint64_t read64(const uint8_t *p){
    return (uint64_t)read32(p) | ((uint64_t)read32(p+4u)<<32u);
}
static int overlap(const void *a,size_t na,const void *b,size_t nb){
    if(!a||!b||!na||!nb)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<na:x-y<nb;
}
int musa_dwg_ac1024_directory_shape(
    const uint8_t *decoded,size_t bytes,MusaDwgAc1024DirectoryShape *out){
    if(!decoded||!out||bytes<20u||bytes>MAX_SECTION_BYTES||
       overlap(decoded,bytes,out,sizeof(*out)))return 0;
    const uint32_t n=read32(decoded);
    if(n==0u||n>MAX_SECTIONS)return 0;
    /* The remaining 16 bytes precede the R2004-family records. This is a
     * layout hypothesis, so failure is diagnostic and never enables an
     * alternate full-DWG opening path. */
    uint8_t names[MAX_SECTIONS][64]={{0}};
    MusaDwgAc1024DirectoryShape evidence={0};
    evidence.sections=n;
    size_t at=20u;
    for(uint32_t i=0;i<n;i++){
        if(at>bytes||bytes-at<96u)return 0;
        const uint8_t *record=decoded+at;
        const uint64_t logical=read64(record);
        const uint32_t pages=read32(record+8u);
        const uint8_t *name=record+32u;
        if(logical>128u*1024u*1024u||
           memchr(name,0,64u)==NULL)return 0;
        if(pages && (!logical||!name[0]))return 0;
        if(i && name[0]){
            for(uint32_t j=0;j<i;j++){
                if(!strcmp((const char*)names[j],(const char*)name)){
                    evidence.duplicate_section_names=1u;
                    break;
                }
            }
        }
        memcpy(names[i],name,64u);
        if(!strcmp((const char*)name,"AcDb:AcDbObjects"))
            ++evidence.objects_name_candidates;
        if(!strcmp((const char*)name,"AcDb:Handles"))
            ++evidence.handles_name_candidates;
        at+=96u;
        if(pages>MAX_REFERENCES-evidence.referenced_pages||
           pages>(bytes-at)/16u)return 0;
        for(uint32_t j=0;j<pages;j++){
            const uint8_t *entry=decoded+at+(size_t)j*16u;
            if(!read32(entry)||!read32(entry+4u))return 0;
        }
        evidence.referenced_pages+=pages;
        at+=(size_t)pages*16u;
    }
    if(at!=bytes)return 0;
    *out=evidence;
    return 1;
}

int musa_dwg_ac1024_directory_physical_links(
    const uint8_t *decoded,size_t bytes,
    const MusaDwgAc1018PageLocation *locations,size_t physical_count,
    MusaDwgAc1024DirectoryPageLinkEvidence *out){
    if(!decoded||!locations||!out||!physical_count||
       physical_count>MAX_REFERENCES||
       physical_count>SIZE_MAX/sizeof(*locations)||
       overlap(decoded,bytes,out,sizeof(*out))||
       overlap(locations,physical_count*sizeof(*locations),
               out,sizeof(*out)))return 0;
    MusaDwgAc1024DirectoryShape shape={0};
    if(!musa_dwg_ac1024_directory_shape(decoded,bytes,&shape))return 0;
    for(size_t i=0;i<physical_count;i++){
        if(!locations[i].page_id ||
           (i&&locations[i-1u].page_id>=locations[i].page_id))
            return 0;
    }
    MusaDwgAc1024DirectoryPageLinkEvidence evidence={0};
    evidence.matched_section_records=shape.sections;
    size_t at=20u;
    for(uint32_t i=0;i<shape.sections;i++){
        const uint32_t npage=read32(decoded+at+8u);
        at+=96u;
        for(uint32_t j=0;j<npage;j++){
            const uint8_t *entry=decoded+at+(size_t)j*16u;
            const uint32_t id=read32(entry),packed=read32(entry+4u);
            size_t lo=0u,hi=physical_count;
            while(lo<hi){
                const size_t mid=lo+(hi-lo)/2u;
                if(locations[mid].page_id<id)lo=mid+1u;
                else hi=mid;
            }
            if(lo==physical_count||locations[lo].page_id!=id||
               locations[lo].file_span_bytes<32u||
               packed>locations[lo].file_span_bytes-32u)
                return 0;
            ++evidence.matched_page_references;
        }
        at+=(size_t)npage*16u;
    }
    if(at!=bytes||
       evidence.matched_page_references!=shape.referenced_pages)
        return 0;
    *out=evidence;
    return 1;
}

/* Chunked Adler32, identical to first-party authenticated R2004 page
 * checksums. Uses integer arithmetic only; never writes original source. */
static uint32_t adler32(uint32_t seed,const uint8_t *p,size_t n){
    uint64_t lo=seed&65535u,hi=seed>>16u;
    for(size_t at=0u;at<n;){
        const size_t count=n-at<5552u?n-at:5552u;
        for(size_t i=0u;i<count;i++){lo+=p[at+i];hi+=lo;}
        lo%=65521u;hi%=65521u;at+=count;
    }
    return ((uint32_t)hi<<16u)|(uint32_t)lo;
}
static void put32(uint8_t *p,uint32_t n){
    p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8u);
    p[2]=(uint8_t)(n>>16u);p[3]=(uint8_t)(n>>24u);
}
int musa_dwg_ac1024_directory_page_checksums(
    const uint8_t *file,size_t file_bytes,
    const uint8_t *decoded,size_t decoded_bytes,
    const MusaDwgAc1018PageLocation *locations,size_t physical_count,
    MusaDwgAc1024PageChecksumEvidence *out){
    if(!file||!decoded||!locations||!out||
       physical_count>SIZE_MAX/sizeof(*locations)||
       overlap(file,file_bytes,out,sizeof(*out))||
       overlap(decoded,decoded_bytes,out,sizeof(*out))||
       overlap(locations,physical_count*sizeof(*locations),
               out,sizeof(*out)))return 0;
    MusaDwgAc1024DirectoryPageLinkEvidence links={0};
    if(!musa_dwg_ac1024_directory_physical_links(
           decoded,decoded_bytes,locations,physical_count,&links))
        return 0;
    MusaDwgAc1024PageChecksumEvidence evidence={0};
    evidence.referenced_pages=links.matched_page_references;
    const uint32_t sections=read32(decoded);
    size_t at=20u;
    for(uint32_t i=0;i<sections;i++){
        const uint8_t *rec=decoded+at;
        const uint32_t npage=read32(rec+8u);
        const uint32_t section_id=read32(rec+24u);
        at+=96u;
        for(uint32_t j=0;j<npage;j++){
            const uint8_t *entry=decoded+at+(size_t)j*16u;
            const uint32_t id=read32(entry),packed=read32(entry+4u);
            const uint64_t logical=read64(entry+8u);
            size_t lo=0u,hi=physical_count;
            while(lo<hi){
                const size_t mid=lo+(hi-lo)/2u;
                if(locations[mid].page_id<id)lo=mid+1u;
                else hi=mid;
            }
            /* Physical link success checked above. A refused source page
             * contributes no geometry and never gets silently skipped. */
            if(lo>=physical_count||locations[lo].page_id!=id)
                continue;
            const MusaDwgAc1018PageLocation *loc=&locations[lo];
            if(!section_id||logical>UINT32_MAX||
               loc->file_offset>file_bytes||
               file_bytes-(size_t)loc->file_offset<32u||
               packed>file_bytes-(size_t)loc->file_offset-32u)
                continue;
            const size_t pos=(size_t)loc->file_offset;
            const uint32_t mask=UINT32_C(0x4164536b)^(uint32_t)pos;
            uint32_t words[8];
            for(size_t k=0u;k<8u;k++)
                words[k]=read32(file+pos+k*4u)^mask;
            if(words[0]!=UINT32_C(0x4163043b)||
               words[1]!=section_id||
               words[2]!=packed||
               words[3]<packed||words[3]-packed>63u||
               words[4]!=(uint32_t)logical||words[5]!=0u)
                continue;
            ++evidence.masked_headers_verified;
            const uint32_t payload_crc=adler32(0u,file+pos+32u,packed);
            if(payload_crc!=words[7])continue;
            ++evidence.payload_adler_verified;
            uint8_t clear[32];
            for(size_t k=0u;k<8u;k++)
                put32(clear+k*4u,k==6u?0u:words[k]);
            if(adler32(payload_crc,clear,sizeof clear)!=words[6])
                continue;
            ++evidence.header_adler_verified;
        }
        at+=(size_t)npage*16u;
    }
    if(at!=decoded_bytes)return 0;
    *out=evidence;
    return 1;
}
