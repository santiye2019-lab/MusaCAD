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
