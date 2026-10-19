/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_sections.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
/* Entirely synthetic AC1018 miniature container. No user DWG bytes. */
static void put32(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8u);
    p[2]=(uint8_t)(x>>16u);p[3]=(uint8_t)(x>>24u);
}
static void put64(uint8_t *p,uint64_t x){
    put32(p,(uint32_t)x);put32(p+4u,(uint32_t)(x>>32u));
}
static uint32_t adler(uint32_t seed,const uint8_t *p,size_t n){
    uint32_t a=seed&65535u,b=seed>>16u;
    for(size_t i=0u;i<n;i++){a=(a+p[i])%65521u;b=(b+a)%65521u;}
    return (b<<16u)|a;
}
static void syshead(uint8_t *p,uint32_t type,uint32_t expanded,
                    uint32_t packed,const uint8_t *stream){
    put32(p,type);put32(p+4u,expanded);put32(p+8u,packed);
    put32(p+12u,2u);put32(p+16u,0u);
    put32(p+16u,adler(adler(0u,p,20u),stream,packed));
}
static void build(uint8_t file[768]){
    memset(file,0,768u);memcpy(file,"AC1018",6u);
    uint8_t hdr[0x6c]={0};
    memcpy(hdr,"AcFssFcAJMB",11u);
    put32(hdr+0x10u,0x6cu);put32(hdr+0x40u,3u);
    put32(hdr+0x50u,3u);put32(hdr+0x5cu,2u);
    put64(hdr+0x54u,448u);
    uint32_t state=1u;
    for(size_t i=0u;i<sizeof(hdr);i++){
        state=state*UINT32_C(0x343fd)+UINT32_C(0x269ec3);
        file[0x80u+i]=(uint8_t)(hdr[i]^(uint8_t)(state>>16u));
    }
    /* A fully decoded 29696-byte data page (all X) compressed to 126 bytes
     * using an R2004 overlap match with distance one. */
    uint8_t *payload=file+288u;
    payload[0]=1u;memset(payload+1u,'X',4u);
    payload[5]=0x20u;payload[6]=0u;
    memset(payload+7u,0u,115u);
    payload[122u]=79u;payload[123u]=0u;
    payload[124u]=0u;payload[125u]=0x11u;
    uint32_t words[8]={UINT32_C(0x4163043b),4u,126u,160u,0u,0u,0u,0u};
    words[7]=adler(0u,payload,126u);
    uint8_t head[32]={0};
    for(size_t i=0u;i<8u;i++)put32(head+4u*i,words[i]);
    words[6]=adler(words[7],head,32u);
    const uint32_t mask=UINT32_C(0x4164536b)^256u;
    for(size_t i=0u;i<8u;i++)put32(file+256u+4u*i,words[i]^mask);
    /* Section map: one AcDb:Handles section, 16 bytes, one physical page. */
    uint8_t section[132]={0};put32(section,1u);
    put64(section+20u,16u);put32(section+28u,1u);
    put32(section+40u,2u);put32(section+44u,4u);
    memcpy(section+52u,"AcDb:Handles",13u);
    put32(section+116u,1u);put32(section+120u,126u);
    put64(section+124u,0u);
    uint8_t *section_payload=file+468u;
    section_payload[0]=0u;section_payload[1]=114u;
    memcpy(section_payload+2u,section,sizeof(section));
    section_payload[134u]=0x11u;
    syshead(file+448u,UINT32_C(0x4163003b),132u,135u,section_payload);
    /* Physical page list: data at 256, section map at 448, map at 704. */
    uint8_t page_map[24]={0};
    put32(page_map,1u);put32(page_map+4u,192u);
    put32(page_map+8u,2u);put32(page_map+12u,256u);
    put32(page_map+16u,3u);put32(page_map+20u,64u);
    uint8_t *map_payload=file+724u;
    map_payload[0]=0u;map_payload[1]=6u;
    memcpy(map_payload+2u,page_map,sizeof(page_map));
    map_payload[26u]=0x11u;
    syshead(file+704u,UINT32_C(0x41630e3b),24u,27u,map_payload);
}
int main(void){
    uint8_t file[768];build(file);
    size_t count=900u;
    MusaDwgAc1018SectionDescriptor desc={0};
    assert(musa_dwg_ac1018_section_pages(file,sizeof(file),
        MUSA_DWG_AC1018_HANDLES,NULL,0u,&desc,&count));
    assert(count==1u&&desc.logical_bytes==16u&&
           desc.map_checksum_verified&&desc.data_page_checksums_verified);
    MusaDwgAc1018SectionPage page[1]={{0}};
    assert(musa_dwg_ac1018_section_pages(file,sizeof(file),
        MUSA_DWG_AC1018_HANDLES,page,1u,&desc,&count));
    assert(page[0].page_id==1u&&page[0].payload_offset==288u&&
           page[0].compressed_bytes==126u&&
           page[0].data_checksum_verified&&page[0].header_checksum_verified);
    uint8_t result[16]={0};size_t bytes=0u;
    assert(musa_dwg_ac1018_extract_section(file,sizeof(file),
        MUSA_DWG_AC1018_HANDLES,result,sizeof(result),&bytes));
    assert(bytes==sizeof(result));
    for(size_t i=0u;i<sizeof(result);i++)assert(result[i]=='X');
    uint8_t preserve[16];memcpy(preserve,result,sizeof(result));
    const size_t saved=bytes;
    assert(!musa_dwg_ac1018_extract_section(file,sizeof(file),
        MUSA_DWG_AC1018_HANDLES,result,sizeof(result)-1u,&bytes));
    assert(!musa_dwg_ac1018_extract_section(file,sizeof(file),
        MUSA_DWG_AC1018_HANDLES,file,sizeof(file),&bytes));
    file[288u]^=1u; /* Data checksum mismatch */
    assert(!musa_dwg_ac1018_extract_section(file,sizeof(file),
        MUSA_DWG_AC1018_HANDLES,result,sizeof(result),&bytes));
    build(file);file[256u+24u]^=1u; /* Encoded header checksum mismatch */
    assert(!musa_dwg_ac1018_extract_section(file,sizeof(file),
        MUSA_DWG_AC1018_HANDLES,result,sizeof(result),&bytes));
    build(file);file[468u+10u]^=1u; /* Section-map checksum mismatch */
    assert(!musa_dwg_ac1018_extract_section(file,sizeof(file),
        MUSA_DWG_AC1018_HANDLES,result,sizeof(result),&bytes));
    assert(!memcmp(preserve,result,sizeof(result))&&bytes==saved);
    return 0;
}
