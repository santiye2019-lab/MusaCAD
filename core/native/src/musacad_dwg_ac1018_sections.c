/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_sections.h"
#include "musacad_dwg_ac1018_page_map.h"
#include "musacad_dwg_ac1018_lz.h"
#include "musacad_dwg_ac1018_section_assemble.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MUSA_R2004_BLOCK 29696u
#define MUSA_R2004_MAX_PAGES 65536u
#define MUSA_R2004_MAX_SECTIONS 256u
#define MUSA_R2004_MAX_EXPANDED 1048576u
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8u)|
           ((uint32_t)p[2]<<16u)|((uint32_t)p[3]<<24u);
}
static uint64_t get64(const uint8_t *p){
    return (uint64_t)get32(p)|((uint64_t)get32(p+4u)<<32u);
}
static void put32(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8u);
    p[2]=(uint8_t)(x>>16u);p[3]=(uint8_t)(x>>24u);
}
static uint32_t check_adler(uint32_t seed,const uint8_t *p,size_t n){
    uint64_t low=seed&65535u,high=seed>>16u;
    size_t at=0u;
    while(at<n){
        const size_t span=n-at<0x15b0u?n-at:0x15b0u;
        for(size_t i=0u;i<span;i++){low+=p[at+i];high+=low;}
        low%=65521u;high%=65521u;at+=span;
    }
    return ((uint32_t)high<<16u)|(uint32_t)low;
}
static int scan(const uint8_t *file,size_t file_bytes,
    MusaDwgAc1018SectionKind kind,MusaDwgAc1018SectionPage *pages,
    MusaDwgAc1018SectionDescriptor *desc,size_t *page_count){
    if(!file||!desc||!page_count||
       (kind!=MUSA_DWG_AC1018_OBJECTS && kind!=MUSA_DWG_AC1018_HANDLES))
        return 0;
    MusaDwgAc1018FileMapHeader hdr={0};
    if(!musa_dwg_ac1018_page_map_header(file,file_bytes,&hdr)||
       hdr.page_count>MUSA_R2004_MAX_PAGES)return 0;
    size_t num=0u;
    if(!musa_dwg_ac1018_page_locations(file,file_bytes,NULL,0u,&num)||
       num!=hdr.page_count||num>MUSA_R2004_MAX_PAGES)return 0;
    MusaDwgAc1018PageLocation *locations=(MusaDwgAc1018PageLocation*)
        calloc(num,sizeof(*locations));
    if(!locations)return 0;
    int ok=0;
    uint8_t *decoded=NULL;
    if(!musa_dwg_ac1018_page_locations(file,file_bytes,locations,num,&num))
        goto finish;
    const size_t sid=(size_t)hdr.section_map_id-1u;
    if(sid>=num||locations[sid].page_id!=hdr.section_map_id||
       locations[sid].file_offset>file_bytes||
       file_bytes-(size_t)locations[sid].file_offset<20u)goto finish;
    const size_t start=(size_t)locations[sid].file_offset;
    const uint8_t *map_header=file+start;
    const uint32_t typ=get32(map_header),unpacked=get32(map_header+4u),
        packed=get32(map_header+8u),codec=get32(map_header+12u),
        stored=get32(map_header+16u);
    if(typ!=UINT32_C(0x4163003b)||codec!=2u||
       unpacked<20u||unpacked>MUSA_R2004_MAX_EXPANDED||
       packed==0u||locations[sid].file_span_bytes<20u||
       packed>locations[sid].file_span_bytes-20u||
       packed>file_bytes-start-20u)goto finish;
    uint8_t canonical_head[20];
    memcpy(canonical_head,map_header,20u);
    memset(canonical_head+16u,0,4u);
    if(check_adler(check_adler(0u,canonical_head,20u),
                   map_header+20u,packed)!=stored)goto finish;
    decoded=(uint8_t*)malloc(unpacked);
    if(!decoded)goto finish;
    size_t consumed=0u;
    if(!musa_dwg_ac1018_lz_decode(map_header+20u,packed,
                                  decoded,unpacked,&consumed)||
       consumed>packed)goto finish;
    const uint32_t nsections=get32(decoded);
    if(nsections==0u||nsections>MUSA_R2004_MAX_SECTIONS)goto finish;
    size_t pos=20u;
    size_t selected_pages=0u;
    MusaDwgAc1018SectionDescriptor selected={0};
    int found=0;
    const char *wanted=kind==MUSA_DWG_AC1018_OBJECTS?
        "AcDb:AcDbObjects":"AcDb:Handles";
    for(uint32_t i=0u;i<nsections;i++){
        if(pos>unpacked||unpacked-pos<96u)goto finish;
        const uint8_t *record=decoded+pos;
        const uint64_t logical=get64(record);
        const uint32_t npages=get32(record+8u);
        const uint32_t compression=get32(record+20u);
        const uint32_t section_id=get32(record+24u);
        const uint32_t encrypted=get32(record+28u);
        const uint8_t *name=record+32u;
        pos+=96u;
        if(npages>num||(size_t)npages>(unpacked-pos)/16u)goto finish;
        const int match=memcmp(name,wanted,strlen(wanted))==0 &&
            name[strlen(wanted)]==0u;
        if(match){
            if(found||!logical||logical>128u*1024u*1024u||
               logical>SIZE_MAX||!section_id||compression!=2u||
               encrypted!=0u||!npages||
               npages!=(logical-1u)/MUSA_R2004_BLOCK+1u)goto finish;
            found=1;
            selected.section_id=section_id;
            selected.compression=compression;
            selected.encryption=encrypted;
            selected.page_count=npages;
            selected.logical_bytes=logical;
            selected.map_checksum_verified=1u;
            selected.data_page_checksums_verified=1u;
            selected_pages=npages;
            for(uint32_t j=0u;j<npages;j++){
                const uint8_t *p=decoded+pos+(size_t)j*16u;
                const uint32_t pid=get32(p);
                const uint32_t packed_bytes=get32(p+4u);
                const uint64_t logical_offset=get64(p+8u);
                if(pid==0u||pid>num||!packed_bytes||
                   logical_offset!=(uint64_t)j*MUSA_R2004_BLOCK)
                    goto finish;
                const MusaDwgAc1018PageLocation *loc=&locations[pid-1u];
                if(loc->page_id!=pid||loc->file_offset>file_bytes||
                   loc->file_span_bytes<32u||
                   file_bytes-(size_t)loc->file_offset<32u)goto finish;
                const uint64_t physical=loc->file_offset;
                const size_t offset=(size_t)physical;
                const uint32_t mask=UINT32_C(0x4164536b)^(uint32_t)physical;
                uint32_t values[8];
                for(size_t k=0u;k<8u;k++)
                    values[k]=get32(file+offset+k*4u)^mask;
                if(values[0]!=UINT32_C(0x4163043b)||
                   values[1]!=section_id||values[2]!=packed_bytes||
                   values[3]<packed_bytes||values[3]-packed_bytes>63u||
                   values[4]!=logical_offset||values[5]!=0u||
                   packed_bytes>loc->file_span_bytes-32u||
                   packed_bytes>file_bytes-offset-32u)goto finish;
                /* R2004 data checksum: zero-seeded compressed payload;
                 * decoded header checksum: data-seeded unmasked 32 bytes,
                 * with checksum word 6 set to zero. On observed AC1018
                 * samples checksum fields are words 7 (data) and 6 (head).
                 */
                const uint32_t data_checksum=check_adler(0u,
                    file+offset+32u,packed_bytes);
                if(data_checksum!=values[7])goto finish;
                uint8_t decoded_head[32];
                for(size_t k=0u;k<8u;k++)
                    put32(decoded_head+k*4u,k==6u?0u:values[k]);
                if(check_adler(data_checksum,decoded_head,32u)!=values[6])
                    goto finish;
                if(pages){
                    pages[j].page_id=pid;
                    pages[j].compressed_bytes=packed_bytes;
                    pages[j].physical_span_bytes=loc->file_span_bytes;
                    pages[j].masked_header_decompressed_bytes=values[3];
                    pages[j].physical_offset=physical;
                    pages[j].payload_offset=physical+32u;
                    pages[j].logical_offset=logical_offset;
                    pages[j].data_checksum_verified=1u;
                    pages[j].header_checksum_verified=1u;
                }
            }
        }
        pos+=(size_t)npages*16u;
    }
    if(!found||pos!=unpacked)goto finish;
    *desc=selected;
    *page_count=selected_pages;
    ok=1;
finish:
    free(decoded);
    free(locations);
    return ok;
}
int musa_dwg_ac1018_section_pages(const uint8_t *file,size_t file_bytes,
    MusaDwgAc1018SectionKind kind,
    MusaDwgAc1018SectionPage *pages,size_t capacity,
    MusaDwgAc1018SectionDescriptor *desc,size_t *out_page_count){
    if(!file||!desc||!out_page_count||(!pages&&capacity))return 0;
    MusaDwgAc1018SectionDescriptor info={0};
    size_t n=0u;
    if(!scan(file,file_bytes,kind,NULL,&info,&n))return 0;
    if(pages){
        if(n>capacity)return 0;
        MusaDwgAc1018SectionPage *tmp=(MusaDwgAc1018SectionPage*)
            calloc(n,sizeof(*tmp));
        if(!tmp)return 0;
        MusaDwgAc1018SectionDescriptor again={0};
        size_t n2=0u;
        const int success=scan(file,file_bytes,kind,tmp,&again,&n2)&&n2==n;
        if(success)memcpy(pages,tmp,n*sizeof(*pages));
        free(tmp);
        if(!success)return 0;
    }
    *desc=info;
    *out_page_count=n;
    return 1;
}
int musa_dwg_ac1018_extract_section(const uint8_t *file,size_t file_bytes,
    MusaDwgAc1018SectionKind kind,uint8_t *output,size_t capacity,
    size_t *out_bytes){
    if(!file||!output||!out_bytes)return 0;
    MusaDwgAc1018SectionDescriptor info={0};
    size_t count=0u;
    if(!musa_dwg_ac1018_section_pages(file,file_bytes,kind,NULL,0u,
                                      &info,&count)||
       info.logical_bytes>capacity||info.logical_bytes>SIZE_MAX||
       !count)return 0;
    MusaDwgAc1018SectionPage *pages=(MusaDwgAc1018SectionPage*)
        calloc(count,sizeof(*pages));
    MusaDwgAc1018PageInput *inputs=(MusaDwgAc1018PageInput*)
        calloc(count,sizeof(*inputs));
    if(!pages||!inputs){free(pages);free(inputs);return 0;}
    MusaDwgAc1018SectionDescriptor check={0};
    size_t actual=0u;
    int good=musa_dwg_ac1018_section_pages(file,file_bytes,kind,
                  pages,count,&check,&actual)&&actual==count&&
                  check.logical_bytes==info.logical_bytes&&
                  check.data_page_checksums_verified;
    if(good){
        for(size_t i=0u;i<count;i++){
            const MusaDwgAc1018SectionPage *p=&pages[i];
            if(!p->data_checksum_verified||!p->header_checksum_verified||
               p->payload_offset>SIZE_MAX||p->logical_offset>SIZE_MAX){
                good=0;break;
            }
            inputs[i].packed_offset=(size_t)p->payload_offset;
            inputs[i].packed_length=(size_t)p->compressed_bytes;
            inputs[i].logical_offset=(size_t)p->logical_offset;
            inputs[i].header_verified=1u;
        }
    }
    if(good)good=musa_dwg_ac1018_assemble_section(file,file_bytes,
                       inputs,count,output,(size_t)info.logical_bytes);
    if(good)*out_bytes=(size_t)info.logical_bytes;
    free(pages);
    free(inputs);
    return good;
}
