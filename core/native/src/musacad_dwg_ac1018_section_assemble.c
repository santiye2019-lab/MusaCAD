/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_section_assemble.h"
#include "musacad_dwg_ac1018_lz.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define MUSA_R2004_DATA_BLOCK 29696u
int musa_dwg_ac1018_assemble_section(const uint8_t *file,size_t file_bytes,
    const MusaDwgAc1018PageInput *pages,size_t pages_count,
    uint8_t *output,size_t logical_bytes){
    if(!file||!pages||!output||!pages_count||
       logical_bytes==0u||logical_bytes>128u*1024u*1024u||
       pages_count>16384u||
       pages_count!=(logical_bytes-1u)/MUSA_R2004_DATA_BLOCK+1u)
        return 0;
    /* Pre-validate all metadata and logical coverage before decompression,
     * ensuring invalid bounds are never passed to the page LZ decoder. */
    for(size_t i=0u;i<pages_count;++i){
        const MusaDwgAc1018PageInput *p=&pages[i];
        if(p->header_verified!=1u ||
           p->logical_offset!=i*MUSA_R2004_DATA_BLOCK ||
           p->packed_offset>file_bytes ||
           p->packed_length==0u||
           p->packed_length>file_bytes-p->packed_offset)
            return 0;
    }
    uint8_t *temp=(uint8_t*)malloc(logical_bytes);
    uint8_t *block=(uint8_t*)malloc(MUSA_R2004_DATA_BLOCK);
    if(!temp||!block){free(temp);free(block);return 0;}
    int good=1;
    size_t covered=0u;
    for(size_t i=0u;i<pages_count;++i){
        const MusaDwgAc1018PageInput *p=&pages[i];
        size_t consumed=0u;
        if(!musa_dwg_ac1018_lz_decode(
                file+p->packed_offset,p->packed_length,
                block,MUSA_R2004_DATA_BLOCK,&consumed) ||
           consumed==0u||consumed>p->packed_length){
            good=0;break;
        }
        const size_t remaining=logical_bytes-covered;
        const size_t n=remaining<MUSA_R2004_DATA_BLOCK?
                       remaining:MUSA_R2004_DATA_BLOCK;
        memcpy(temp+covered,block,n);
        covered+=n;
    }
    if(covered!=logical_bytes)good=0;
    if(good)memcpy(output,temp,logical_bytes);
    free(block);free(temp);
    return good;
}
