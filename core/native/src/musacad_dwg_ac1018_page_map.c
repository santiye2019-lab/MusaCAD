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
    /* Real DWG files may contain negative-numbered physical *gaps*.
     * Their file page IDs are sparse: map and section-map IDs must not
     * be assumed equal to an array position or <= stored page count. */
    if(!count||count>131072u||!map_id||!sec_id||
       map_id==sec_id||map_id>1048576u||sec_id>1048576u||
       offset>UINT64_MAX-256u)return 0;
    const uint64_t pos=offset+256u;
    if(pos>SIZE_MAX||(size_t)pos>n||n-(size_t)pos<20u)return 0;
    const uint8_t *p=file+(size_t)pos;
    const uint32_t type=u32(p),expanded=u32(p+4u),
                   packed=u32(p+8u),codec=u32(p+12u),
                   stored=u32(p+16u);
    if(type!=UINT32_C(0x41630e3b)||codec!=2u||
       expanded<(uint64_t)count*8u||
       expanded>16u*1024u*1024u||!packed||
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
    /* Parse the ENTIRE variable-length physical page map. Positive IDs
     * are actual section pages; negative IDs are file gaps and carry an
     * additional 16-byte gap-tree record (R2004+ physical format).
     * Never trust a sequential page-id == index assumption. Validation
     * pass is separate from publication of caller-owned output. */
    uint64_t pos=256u;
    uint32_t actual=0u,last_abs_id=0u,found_map=0u,found_section=0u;
    int valid=1,verified_secondary_header=0;
    size_t at=0u;
    while(at<h.map_decoded_bytes){
        if(h.map_decoded_bytes-at<8u){valid=0;break;}
        const uint32_t raw=u32(plain+at);
        const uint32_t size=u32(plain+at+4u);
        const int gap=(raw&UINT32_C(0x80000000))!=0u;
        if(gap&&h.map_decoded_bytes-at<24u){valid=0;break;}
        const uint32_t id=gap?(uint32_t)(-(int64_t)(int32_t)raw):raw;
        const size_t stride=gap?24u:8u;
        const int final=(at+stride==h.map_decoded_bytes);
        /* On real R2018 files the last physical SYSTEM page's reported
         * allocated span may exceed the actual file EOF; its encoded
         * compressed content must still be fully present and checksummed.
         * Earlier pages and gaps MUST fit entirely in source bytes. */
        if(!id||id<=last_abs_id||id>1048576u||
           !size||pos>UINT64_MAX-size||pos>=n){
            valid=0;break;
        }
        const uint64_t available=(uint64_t)n-pos;
        if(final){
            if(gap||id!=h.page_map_id||available<20u){
                valid=0;break;
            }
            if(available>size&&available-size!=106u){
                /* A genuine R2018 file can store a SECOND copy of the
                 * authenticated 108-byte encrypted file header at EOF,
                 * preceded by exactly two zero alignment bytes.
                 * Only admit the precise byte-for-byte copy, NEVER an
                 * arbitrary unindexed trailer or injected hidden data.
                 * Observed original 2018/Line.dwg has exactly this form.
                 * Earlier original Dynblocks leaves no such trailer. */
                if(available-size!=110u||
                   pos+(uint64_t)size>SIZE_MAX||
                   file[(size_t)(pos+(uint64_t)size)]!=0u||
                   file[(size_t)(pos+(uint64_t)size)+1u]!=0u||
                   memcmp(file+(size_t)(pos+(uint64_t)size)+2u,
                          file+0x80u,0x6cu)!=0){
                    valid=0;break;
                }
                verified_secondary_header=1;
            }
            /* Only the exact R2018 106-byte terminal form is allowed to
             * reach the authenticated empty-SYSTEM-footer validation
             * below; no unindexed trailer is accepted by itself. */
        }else if(available<size){
            valid=0;break;
        }
        last_abs_id=id;
        if(!gap){
            if(actual>=h.page_count){valid=0;break;}
            if(id==h.page_map_id){
                if(pos!=h.page_map_offset){valid=0;break;}
                found_map=1u;
            }
            if(id==h.section_map_id)found_section=1u;
            /* Publish entries only after validation finishes. */
            ++actual;
        }
        pos+=size;
        at+=gap?24u:8u;
    }
    /* Another original R2018 physical terminal form is present in the
     * independent AutoCAD 2018/Point.dwg. It has a EXACT empty 20-byte
     * SYSTEM-page marker after the CRC-authenticated compressed map body,
     * then the original 108-byte encrypted file header copied at EOF.
     * The allocated final 160-byte map span ends two bytes into the
     * duplicated header: n - declared_map_end = 106. Do not accept
     * arbitrary 106-byte garbage or an unverified footer. */
    if(valid&&!verified_secondary_header&&
       !memcmp(required_signature,"AC1032",6u)&&
       pos<n&&n-(size_t)pos==106u&&n>=108u&&
       memcmp(file+n-108u,file+0x80u,0x6cu)==0){
        static const uint8_t exact_empty_system[20]={
            0x3bu,0x0eu,0x63u,0x41u,0u,0u,0u,0u,
            0u,0u,0u,0u,2u,0u,0u,0u,0u,0u,0u,0u
        };
        const uint64_t body_end=h.page_map_offset+20u+
                                 (uint64_t)h.map_compressed_bytes;
        if(body_end<=SIZE_MAX&&body_end<=n&&
           n-(size_t)body_end>=20u+108u&&
           body_end+20u==n-108u&&
           body_end+22u==pos&&
           memcmp(file+(size_t)body_end,exact_empty_system,
                  sizeof(exact_empty_system))==0)
           verified_secondary_header=2;
    }
    if(!valid||at!=h.map_decoded_bytes||
       actual!=h.page_count||!found_map||!found_section||
       (pos<n&&
        !((verified_secondary_header==1&&n-(size_t)pos==110u)||
          (verified_secondary_header==2&&n-(size_t)pos==106u)))){
        free(plain);return 0;
    }
    /* To uphold atomic publication, validate without writing entries
     * when a caller result array is provided: otherwise malformed later
     * entries could partially overwrite existing output on failure. */
    if(entries){
        size_t cursor=0u;uint64_t off=256u;size_t idx=0u;
        while(cursor<h.map_decoded_bytes){
            const uint32_t raw=u32(plain+cursor);
            const uint32_t span=u32(plain+cursor+4u);
            const int gap=(raw&UINT32_C(0x80000000))!=0u;
            if(!gap){
                entries[idx].page_id=raw;
                entries[idx].file_span_bytes=span;
                entries[idx].file_offset=off;
                ++idx;
            }
            off+=span;
            cursor+=gap?24u:8u;
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
