/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_sections.h"
#include "musacad_dwg_ac1024_classes.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
static void u32(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8u);
    p[2]=(uint8_t)(x>>16u);p[3]=(uint8_t)(x>>24u);
}
static void u64(uint8_t *p,uint64_t x){
    u32(p,(uint32_t)x);u32(p+4u,(uint32_t)(x>>32u));
}
static uint32_t adler(uint32_t seed,const uint8_t *p,size_t n){
    uint32_t a=seed&65535u,b=seed>>16u;
    for(size_t i=0u;i<n;i++){a=(a+p[i])%65521u;b=(b+a)%65521u;}
    return (b<<16u)|a;
}
static void system_header(uint8_t *p,uint32_t type,uint32_t expanded,
                          uint32_t packed,const uint8_t *data){
    u32(p,type);u32(p+4u,expanded);u32(p+8u,packed);
    u32(p+12u,2u);u32(p+16u,0u);
    u32(p+16u,adler(adler(0u,p,20u),data,packed));
}
/* This is a fully synthetic DWG structure, not derived from a user file. */
static void build(uint8_t file[768]){
    memset(file,0,768u);memcpy(file,"AC1018",6u);
    uint8_t hdr[0x6c]={0};
    memcpy(hdr,"AcFssFcAJMB",11u);
    u32(hdr+0x10u,0x6cu);u32(hdr+0x40u,3u);
    u32(hdr+0x50u,3u);u32(hdr+0x5cu,2u);
    u64(hdr+0x54u,448u);
    uint32_t state=1u;
    for(size_t i=0u;i<sizeof(hdr);i++){
        state=state*UINT32_C(0x343fd)+UINT32_C(0x269ec3);
        file[0x80u+i]=(uint8_t)(hdr[i]^(uint8_t)(state>>16u));
    }
    uint8_t *payload=file+288u;
    payload[0]=1u;memset(payload+1u,'X',4u);
    payload[5]=0x20u;payload[6]=0u;
    memset(payload+7u,0u,115u);
    payload[122u]=79u;payload[123u]=0u;
    payload[124u]=0u;payload[125u]=0x11u;
    uint32_t words[8]={UINT32_C(0x4163043b),4u,126u,160u,0u,0u,0u,0u};
    words[7]=adler(0u,payload,126u);
    uint8_t raw_header[32]={0};
    for(size_t i=0u;i<8u;i++)u32(raw_header+4u*i,words[i]);
    words[6]=adler(words[7],raw_header,32u);
    const uint32_t mask=UINT32_C(0x4164536b)^256u;
    for(size_t i=0u;i<8u;i++)u32(file+256u+4u*i,words[i]^mask);
    uint8_t section[132]={0};u32(section,1u);
    u64(section+20u,16u);u32(section+28u,1u);
    u32(section+40u,2u);u32(section+44u,4u);
    memcpy(section+52u,"AcDb:Handles",13u);
    u32(section+116u,1u);u32(section+120u,126u);
    u64(section+124u,0u);
    uint8_t *section_payload=file+468u;
    section_payload[0]=0u;section_payload[1]=114u;
    memcpy(section_payload+2u,section,sizeof(section));
    section_payload[134u]=0x11u;
    system_header(file+448u,UINT32_C(0x4163003b),132u,135u,
                  section_payload);
    uint8_t page_map[24]={0};
    u32(page_map,1u);u32(page_map+4u,192u);
    u32(page_map+8u,2u);u32(page_map+12u,256u);
    u32(page_map+16u,3u);u32(page_map+20u,64u);
    uint8_t *map_payload=file+724u;
    map_payload[0]=0u;map_payload[1]=6u;
    memcpy(map_payload+2u,page_map,sizeof(page_map));
    map_payload[26u]=0x11u;
    system_header(file+704u,UINT32_C(0x41630e3b),24u,27u,map_payload);
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
    uint8_t saved[16];memcpy(saved,result,sizeof(result));
    const size_t before=bytes;
    assert(!musa_dwg_ac1018_extract_section(file,sizeof(file),
        MUSA_DWG_AC1018_HANDLES,result,sizeof(result)-1u,&bytes));
    assert(!musa_dwg_ac1018_extract_section(file,sizeof(file),
        MUSA_DWG_AC1018_HANDLES,file,sizeof(file),&bytes));
    file[288u]^=1u;
    assert(!musa_dwg_ac1018_extract_section(file,sizeof(file),
        MUSA_DWG_AC1018_HANDLES,result,sizeof(result),&bytes));
    build(file);file[256u+24u]^=1u;
    assert(!musa_dwg_ac1018_extract_section(file,sizeof(file),
        MUSA_DWG_AC1018_HANDLES,result,sizeof(result),&bytes));
    build(file);file[468u+10u]^=1u;
    assert(!musa_dwg_ac1018_extract_section(file,sizeof(file),
        MUSA_DWG_AC1018_HANDLES,result,sizeof(result),&bytes));
    assert(!memcmp(saved,result,sizeof(result))&&bytes==before);
    /* Synthetic protected R2010 physical Classes page. Contents are
     * opaque 16-byte 'X' payload, NOT valid semantic Classes fields. */
    build(file);
    memcpy(file,"AC1024",6u);
    memcpy(file+468u+2u+52u,"AcDb:Classes",13u);
    system_header(file+448u,UINT32_C(0x4163003b),132u,135u,
                  file+468u);
    uint64_t class_bytes=0u;
    size_t class_pages=0u;
    assert(musa_dwg_ac1024_classes_section_probe(
         file,sizeof(file),&class_bytes,&class_pages));
    assert(class_bytes==16u&&class_pages==1u);
    uint8_t class_out[16]={0};
    size_t written=0u;
    assert(musa_dwg_ac1024_extract_classes_section(
         file,sizeof(file),class_out,sizeof(class_out),&written));
    assert(written==16u);
    for(size_t i=0u;i<16u;i++)assert(class_out[i]=='X');
    uint8_t saved_out[16];
    memcpy(saved_out,class_out,16u);
    assert(!musa_dwg_ac1024_extract_classes_section(
         file,sizeof(file),class_out,15u,&written));
    assert(!musa_dwg_ac1024_extract_classes_section(
         file,sizeof(file),file,sizeof(file),&written));
    file[288u]^=1u; /* authentic data-page payload CRC tampered */
    assert(!musa_dwg_ac1024_classes_section_probe(
         file,sizeof(file),&class_bytes,&class_pages));
    assert(!musa_dwg_ac1024_extract_classes_section(
         file,sizeof(file),class_out,16u,&written));
    file[288u]^=1u;
    memcpy(file,"AC1018",6u);
    assert(!musa_dwg_ac1024_classes_section_probe(
         file,sizeof(file),&class_bytes,&class_pages));
    assert(!musa_dwg_ac1024_extract_classes_section(
         file,sizeof(file),class_out,16u,&written));
    assert(class_bytes==16u&&class_pages==1u&&written==16u);
    assert(!memcmp(saved_out,class_out,16u));
    return 0;
}
