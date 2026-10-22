#include "musacad_dwg_ac1032_classes.h"
#include "musacad_dwg_ac1032_sections.h"
#include <stdlib.h>
#include <string.h>
static const uint8_t class_marker[16]={
    0x8d,0xa1,0xc4,0xb8,0xc4,0xa9,0xf8,0xc5,
    0xc0,0xdc,0xf4,0x5f,0xe7,0xcf,0xb6,0x8a
};
static uint32_t u32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8u)|
           ((uint32_t)p[2]<<16u)|((uint32_t)p[3]<<24u);
}
static int nonoverlap(const void *a,size_t n,const void *b,size_t m){
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return !((x<=y&&y-x<n)||(y<x&&x-y<m));
}
int musa_dwg_ac1032_classes_frame_probe(
    const uint8_t *file,size_t len,MusaDwgAc1032ClassesFrame *out){
    if(!file||!out||len<256u||len>64u*1024u*1024u||
       !nonoverlap(file,len,out,sizeof(*out)))return 0;
    size_t count=0u;
    if(!musa_dwg_ac1032_section_catalog(file,len,NULL,0u,&count)||
       !count||count>256u)return 0;
    MusaDwgAc1032SectionCatalogEntry *items=
        (MusaDwgAc1032SectionCatalogEntry*)calloc(count,sizeof(*items));
    if(!items)return 0;
    int ok=musa_dwg_ac1032_section_catalog(file,len,items,count,&count);
    uint64_t logical=0u;
    uint32_t section_id=0u;
    unsigned found=0u;
    if(ok)for(size_t i=0;i<count;i++){
        if(!strcmp((const char*)items[i].name,"AcDb:Classes")){
            found++;
            logical=items[i].logical_bytes;
            section_id=items[i].section_id;
            if(!items[i].all_physical_page_checksums_verified)ok=0;
        }
    }
    free(items);
    if(!ok||found!=1u||logical<29u||
       logical>8u*1024u*1024u)return 0;
    uint8_t *bytes=(uint8_t*)malloc((size_t)logical);
    if(!bytes)return 0;
    size_t written=0u;
    ok=musa_dwg_ac1032_extract_named_section(
       file,len,"AcDb:Classes",bytes,(size_t)logical,&written)&&
       written==logical&&
       !memcmp(bytes,class_marker,sizeof(class_marker));
    MusaDwgAc1032ClassesFrame result={0};
    if(ok){
        /* R2018 stores a 64-bit class data area length (two raw
         * little-endian longs), then a 32-bit bit count. The public
         * genuine AC1032 fixture has area=6531, high=0, bits=52242.
         * Reject >32-bit areas until a bounded streaming reader exists. */
        const uint32_t area=u32(bytes+16u);
        const uint32_t high=u32(bytes+20u);
        const uint32_t bits=u32(bytes+24u);
        ok=area>0u&&!high&&area<=logical-28u&&
           bits>0u&&(uint64_t)bits<=(uint64_t)area*8u;
        if(ok){
            result.class_area_bytes=area;
            result.total_data_bits=bits;
            result.section_logical_bytes=logical;
            result.section_id=section_id;
            result.original_sentinel_verified=1u;
            result.page_crcs_verified=1u;
        }
    }
    free(bytes);
    if(!ok)return 0;
    *out=result;return 1;
}
