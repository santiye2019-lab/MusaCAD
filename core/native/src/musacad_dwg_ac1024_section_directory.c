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
