/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_page_map.h"
#include "musacad_dwg_ac1032_page_map.h"
#include "musacad_dwg_ac1018_lz.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static uint32_t u32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8u)|
           ((uint32_t)p[2]<<16u)|((uint32_t)p[3]<<24u);
}
static uint64_t u64(const uint8_t *p){
    return (uint64_t)u32(p)|((uint64_t)u32(p+4u)<<32u);
}
static uint32_t checksum(uint32_t seed,const uint8_t *p,size_t n){
    uint64_t lo=(uint64_t)(seed&0xffffu),hi=(uint64_t)(seed>>16u);
    size_t i=0u;
    while(i<n){
        const size_t count=n-i<0x15b0u?n-i:0x15b0u;
        for(size_t j=0u;j<count;++j){lo+=(uint64_t)p[i+j];hi+=lo;}
        lo%=65521u;hi%=65521u;i+=count;
    }
    return ((uint32_t)hi<<16u)|(uint32_t)lo;
}
static int shared_page_map_header(const uint8_t *file,size_t n,
    const char required_signature[6],MusaDwgAc1018FileMapHeader *out){
    if(!file||!out||n<256u||
       memcmp(file,required_signature,6u)!=0)return 0;
    uint8_t plain[0x6c];
    uint32_t state=1u;
    for(size_t i=0u;i<sizeof(plain);++i){
        state=state*UINT32_C(0x343fd)+UINT32_C(0x269ec3);
        plain[i]=(uint8_t)(file[0x80u+i]^(uint8_t)(state>>16u));
    }
    if(memcmp(plain,"AcFssFcAJMB",11u)!=0||
       u32(plain+0x10u)!=0x6cu)return 0;
    const uint32_t count=u32(plain+0x40u);
    const uint32_t map_id=u32(plain+0x50u);
    const uint32_t sec_id=u32(plain+0x5cu);
    const uint64_t offset=u64(plain+0x54u);
    if(!count||count>131072u||!map_id||map_id>count||
       !sec_id||sec_id>count||offset>UINT64_MAX-256u)return 0;
    const uint64_t pos=offset+256u;
    if(pos>SIZE_MAX||(size_t)pos>n||n-(size_t)pos<20u)return 0;
    const uint8_t *p=file+(size_t)pos;
    const uint32_t type=u32(p),expanded=u32(p+4u),
                   packed=u32(p+8u),codec=u32(p+12u),
                   stored=u32(p+16u);
    if(type!=UINT32_C(0x41630e3b)||codec!=2u||
       expanded!=(uint64_t)count*8u||!packed||
       (size_t)packed>n-(size_t)pos-20u)return 0;
    uint8_t head[20];memcpy(head,p,20u);memset(head+16u,0,4u);
    if(checksum(checksum(0u,head,20u),p+20u,packed)!=stored)return 0;
    MusaDwgAc1018FileMapHeader result={0};
    result.page_map_offset=pos;result.page_count=count;
    result.page_map_id=map_id;result.section_map_id=sec_id;
    result.map_decoded_bytes=expanded;result.map_compressed_bytes=packed;
    *out=result;return 1;
}
int musa_dwg_ac1018_page_map_header(
    const uint8_t *file,size_t n,MusaDwgAc1018FileMapHeader *out){
    return shared_page_map_header(file,n,"AC1018",out);
}
/* AC1032 is a different OBJECT schema but retains the documented R2004
 * family encrypted HEADER/page-location structural organization. The
 * original AC1018 body readers are deliberately NEVER called here. */
int musa_dwg_ac1032_page_map_header(
    const uint8_t *file,size_t n,MusaDwgAc1032FileMapHeader *out){
    return shared_page_map_header(file,n,"AC1032",out);
}
static int shared_page_locations(const uint8_t *file,size_t n,
    const char required_signature[6],
    MusaDwgAc1018PageLocation *entries,size_t capacity,size_t *out_count){
    if(!file||!out_count||(!entries&&capacity))return 0;
    MusaDwgAc1018FileMapHeader h={0};
    if(!shared_page_map_header(file,n,required_signature,&h)||
       (entries&&(size_t)h.page_count>capacity))return 0;
    uint8_t *plain=(uint8_t*)malloc(h.map_decoded_bytes);
    if(!plain)return 0;
    size_t consumed=0u;
    const int ok=musa_dwg_ac1018_lz_decode(
        file+(size_t)h.page_map_offset+20u,h.map_compressed_bytes,
        plain,h.map_decoded_bytes,&consumed);
    if(!ok||consumed>h.map_compressed_bytes){free(plain);return 0;}
    uint64_t pos=256u;
    int valid=1;
    for(uint32_t i=0u;i<h.page_count;++i){
        const uint32_t id=u32(plain+(size_t)i*8u);
        const uint32_t size=u32(plain+(size_t)i*8u+4u);
        if(id!=i+1u||!size||pos>UINT64_MAX-size||pos>=n){
            valid=0;break;
        }
        if(i==h.page_map_id-1u&&pos!=h.page_map_offset){
            valid=0;break;
        }
        if(i+1u<h.page_count&&(uint64_t)n-pos<size){
            valid=0;break;
        }
        pos+=size;
    }
    if(pos<n||pos-n>1024u)valid=0;
    if(!valid){free(plain);return 0;}
    /* Only after validating all pages do we publish any index entry. */
    if(entries){
        pos=256u;
        for(uint32_t i=0u;i<h.page_count;++i){
            entries[i].page_id=u32(plain+(size_t)i*8u);
            entries[i].file_span_bytes=u32(plain+(size_t)i*8u+4u);
            entries[i].file_offset=pos;
            pos+=entries[i].file_span_bytes;
        }
    }
    free(plain);
    *out_count=h.page_count;
    return 1;
}

/* Never promote AC1032 signatures to the existing AC1018 geometry decoder.
 * Only physical page-map layout and file-bound offsets are shared. */
int musa_dwg_ac1018_page_locations(const uint8_t *file,size_t n,
    MusaDwgAc1018PageLocation *entries,size_t capacity,size_t *count){
    return shared_page_locations(file,n,"AC1018",entries,capacity,count);
}
int musa_dwg_ac1032_page_locations(const uint8_t *file,size_t n,
    MusaDwgAc1032PageLocation *entries,size_t capacity,size_t *count){
    return shared_page_locations(file,n,"AC1032",entries,capacity,count);
}
