/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_page_map.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
/* Purely synthetic two-page AC1018 file. Never copies a private DWG. */
static void le32(uint8_t *p,uint32_t v){
    for(unsigned i=0u;i<4u;++i)p[i]=(uint8_t)(v>>(8u*i));
}
static uint32_t checksum(uint32_t seed,const uint8_t *p,size_t n){
    uint32_t lo=seed&65535u,hi=seed>>16u;
    for(size_t i=0u;i<n;++i){
        lo=(lo+p[i])%65521u;hi=(hi+lo)%65521u;
    }
    return (hi<<16u)|lo;
}
static void fixture(uint8_t file[380]){
    memset(file,0,380u);
    memcpy(file,"AC1018",6u);
    uint8_t hdr[0x6c]={0};
    memcpy(hdr,"AcFssFcAJMB",11u);
    le32(hdr+0x10u,0x6cu);
    le32(hdr+0x40u,2u);
    le32(hdr+0x50u,2u);
    le32(hdr+0x5cu,1u);
    le32(hdr+0x54u,64u);
    uint32_t state=1u;
    for(size_t i=0u;i<sizeof(hdr);++i){
        state=state*UINT32_C(0x343fd)+UINT32_C(0x269ec3);
        file[0x80u+i]=(uint8_t)(hdr[i]^(uint8_t)(state>>16u));
    }
    const size_t pos=320u;
    le32(file+pos,UINT32_C(0x41630e3b));
    le32(file+pos+4u,16u);
    le32(file+pos+8u,18u);
    le32(file+pos+12u,2u);
    uint8_t *packed=file+pos+20u;
    packed[0]=13u; /* R2004 literal 16 bytes */
    le32(packed+1u,1u);le32(packed+5u,64u);
    le32(packed+9u,2u);le32(packed+13u,64u);
    packed[17u]=0x11u;
    le32(file+pos+16u,checksum(
        checksum(0u,file+pos,20u),packed,18u));
}
int main(void){
    uint8_t file[380];fixture(file);
    MusaDwgAc1018FileMapHeader h={0};
    assert(musa_dwg_ac1018_page_map_header(file,sizeof(file),&h));
    assert(h.page_count==2u&&h.page_map_id==2u&&h.section_map_id==1u);
    assert(h.page_map_offset==320u&&h.map_decoded_bytes==16u);
    size_t n=999u;
    assert(musa_dwg_ac1018_page_locations(file,sizeof(file),NULL,0u,&n));
    assert(n==2u);
    MusaDwgAc1018PageLocation entries[2]={{0}};
    assert(musa_dwg_ac1018_page_locations(
        file,sizeof(file),entries,2u,&n));
    assert(entries[0].page_id==1u&&entries[0].file_offset==256u&&
           entries[0].file_span_bytes==64u);
    assert(entries[1].page_id==2u&&entries[1].file_offset==320u&&
           entries[1].file_span_bytes==64u);
    MusaDwgAc1018PageLocation before[2];memcpy(before,entries,sizeof(before));
    const size_t n_before=n;
    assert(!musa_dwg_ac1018_page_locations(
        file,sizeof(file),entries,1u,&n));
    file[320u+20u+9u]^=1u; /* bad checksum */
    assert(!musa_dwg_ac1018_page_locations(
        file,sizeof(file),entries,2u,&n));
    assert(n==n_before&&memcmp(before,entries,sizeof(before))==0);
    fixture(file);
    file[0x80u]^=1u; /* corrupt encrypted fixed header */
    assert(!musa_dwg_ac1018_page_map_header(file,sizeof(file),&h));
    fixture(file);
    file[320u+12u]=3u; /* unsupported codec */
    assert(!musa_dwg_ac1018_page_map_header(file,sizeof(file),&h));
    fixture(file);
    assert(!musa_dwg_ac1018_page_locations(file,337u,entries,2u,&n));
    assert(n==n_before&&memcmp(before,entries,sizeof(before))==0);

    /* Preserve historical AC1018 path, and independently admit the
     * same protected HEADER/page-map *shape* under original AC1024
     * signature. No temporary AC1018 signature patch is permitted. */
    fixture(file);
    memcpy(file,"AC1024",6u);
    uint8_t unchanged[380];
    memcpy(unchanged,file,sizeof(file));
    MusaDwgAc1018FileMapHeader r2010={0};
    assert(!musa_dwg_ac1018_page_map_header(file,sizeof(file),&h));
    assert(!musa_dwg_ac1018_page_locations(
        file,sizeof(file),entries,2u,&n));
    assert(musa_dwg_ac1024_page_map_header(file,sizeof(file),&r2010));
    assert(r2010.page_map_offset==320u&&r2010.page_count==2u);
    assert(musa_dwg_ac1024_page_locations(
        file,sizeof(file),entries,2u,&n));
    assert(n==2u&&
           entries[0].page_id==1u&&entries[1].page_id==2u&&
           memcmp(file,unchanged,sizeof(file))==0);
    MusaDwgAc1018FileMapHeader saved_header=r2010;
    MusaDwgAc1018PageLocation saved_locations[2];
    memcpy(saved_locations,entries,sizeof(entries));
    size_t saved_count=n;
    file[320u+20u+9u]^=1u; /* checksum mismatch in actual source */
    assert(!musa_dwg_ac1024_page_map_header(
        file,sizeof(file),&r2010));
    assert(!musa_dwg_ac1024_page_locations(
        file,sizeof(file),entries,2u,&n));
    assert(memcmp(&saved_header,&r2010,sizeof(saved_header))==0);
    assert(memcmp(saved_locations,entries,sizeof(entries))==0);
    assert(saved_count==n);
    file[320u+20u+9u]^=1u;
    assert(!musa_dwg_ac1024_page_locations(
        file,337u,entries,2u,&n));
    assert(memcmp(saved_locations,entries,sizeof(entries))==0);
    assert(saved_count==n);
    /* Fail on wrong original signature, never auto-promote R2018. */
    memcpy(file,"AC1032",6u);
    assert(!musa_dwg_ac1024_page_map_header(
        file,sizeof(file),&r2010));
    assert(!musa_dwg_ac1024_page_locations(
        file,sizeof(file),entries,2u,&n));
    assert(memcmp(&saved_header,&r2010,sizeof(saved_header))==0);
    assert(memcmp(saved_locations,entries,sizeof(entries))==0);
    assert(saved_count==n);
    return 0;
}
