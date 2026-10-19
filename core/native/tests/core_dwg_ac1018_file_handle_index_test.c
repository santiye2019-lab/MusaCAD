/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_file_index.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#define RAW_BYTES 992u
static void u32(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8u);
    p[2]=(uint8_t)(x>>16u);p[3]=(uint8_t)(x>>24u);
}
static void u64(uint8_t *p,uint64_t x){
    u32(p,(uint32_t)x);u32(p+4u,(uint32_t)(x>>32u));
}
static uint32_t adler(uint32_t seed,const uint8_t *p,size_t n){
    uint32_t a=seed&65535u,b=seed>>16u;
    for(size_t i=0u;i<n;++i){a=(a+p[i])%65521u;b=(b+a)%65521u;}
    return (b<<16u)|a;
}
static void sys_header(uint8_t *p,uint32_t type,
                       uint32_t expanded,uint32_t packed){
    u32(p,type);u32(p+4u,expanded);u32(p+8u,packed);
    u32(p+12u,2u);u32(p+16u,0u);
    u32(p+16u,adler(adler(0u,p,20u),p+20u,packed));
}
static void crc_page(uint8_t *p,size_t bytes){
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,p,bytes,&crc));
    p[bytes]=(uint8_t)(crc>>8u);p[bytes+1u]=(uint8_t)crc;
}
/* Synthesized handle deltas: handles 1..4 -> object offsets 0,2,4,6. */
static void make_handles(uint8_t h[19]){
    memset(h,0u,19u);h[1]=10u;
    h[2]=1u;h[3]=0u;
    h[4]=1u;h[5]=2u;
    h[6]=1u;h[7]=2u;
    h[8]=1u;h[9]=2u;
    crc_page(h,10u);
    h[12]=0u;h[13]=2u;crc_page(h+12u,2u);
    h[18]=0x53u;
}
/* Literal first 19 bytes, then exact LZ match to form a 29,696-byte page.
 * This is generated locally, not a snippet of any user-owned DWG. */
static void data_page(uint8_t *file,size_t at,uint32_t section_id,
                      const uint8_t head19[19]){
    uint8_t *p=file+at+32u;
    p[0]=0u;p[1]=1u;memcpy(p+2u,head19,19u);
    p[21]=0x20u;p[22]=0u;
    memset(p+23u,0u,115u);p[138]=64u;
    p[139]=0u;p[140]=0u;p[141]=0x11u;
    uint32_t words[8]={UINT32_C(0x4163043b),section_id,
                       142u,160u,0u,0u,0u,0u};
    words[7]=adler(0u,p,142u);
    uint8_t clean[32]={0};
    for(unsigned i=0u;i<8u;i++)u32(clean+4u*i,words[i]);
    words[6]=adler(words[7],clean,32u);
    const uint32_t mask=UINT32_C(0x4164536b)^(uint32_t)at;
    for(unsigned i=0u;i<8u;i++)
        u32(file+at+4u*i,words[i]^mask);
}
static void put_section(uint8_t *record,uint64_t logical,
                        uint32_t section_id,const char *name,
                        uint32_t physical_page_id){
    u64(record,logical);u32(record+8u,1u);
    u32(record+20u,2u);u32(record+24u,section_id);
    memcpy(record+32u,name,strlen(name));
    u32(record+96u,physical_page_id);
    u32(record+100u,142u);u64(record+104u,0u);
}
static void build(uint8_t file[RAW_BYTES]){
    memset(file,0u,RAW_BYTES);
    memcpy(file,"AC1018",6u);
    uint8_t protected_header[0x6c]={0};
    memcpy(protected_header,"AcFssFcAJMB",11u);
    u32(protected_header+0x10u,0x6cu);
    u32(protected_header+0x40u,4u);
    u32(protected_header+0x50u,4u);
    u32(protected_header+0x5cu,3u);
    u64(protected_header+0x54u,672u);
    uint32_t state=1u;
    for(size_t i=0u;i<sizeof(protected_header);i++){
        state=state*UINT32_C(0x343fd)+UINT32_C(0x269ec3);
        file[0x80u+i]=(uint8_t)(protected_header[i]^(uint8_t)(state>>16u));
    }
    uint8_t handles[19];make_handles(handles);
    uint8_t objects[19];
    memset(objects,'O',sizeof(objects));
    data_page(file,256u,10u,handles);
    data_page(file,448u,11u,objects);
    uint8_t section[244]={0};
    u32(section,2u);
    put_section(section+20u,16u,10u,"AcDb:Handles",1u);
    put_section(section+132u,19u,11u,"AcDb:AcDbObjects",2u);
    uint8_t *s=file+640u+20u;
    s[0]=0u;s[1]=226u;
    memcpy(s+2u,section,sizeof(section));s[246]=0x11u;
    sys_header(file+640u,UINT32_C(0x4163003b),244u,247u);
    uint8_t map[32]={0};
    u32(map,1u);u32(map+4u,192u);
    u32(map+8u,2u);u32(map+12u,192u);
    u32(map+16u,3u);u32(map+20u,288u);
    u32(map+24u,4u);u32(map+28u,64u);
    uint8_t *m=file+928u+20u;
    m[0]=0u;m[1]=14u;
    memcpy(m+2u,map,sizeof(map));m[34]=0x11u;
    sys_header(file+928u,UINT32_C(0x41630e3b),32u,35u);
}
int main(void){
    uint8_t file[RAW_BYTES];
    build(file);
    size_t count=999u;
    assert(musa_dwg_ac1018_file_handle_index(file,sizeof(file),
                NULL,0u,&count));
    assert(count==4u);
    MusaDwgAc1018HandleEntry index[4]={{0}};
    assert(musa_dwg_ac1018_file_handle_index(file,sizeof(file),
                index,4u,&count));
    assert(count==4u);
    for(unsigned i=0u;i<4u;i++){
        assert(index[i].handle==i+1u);
        assert(index[i].object_offset==2u*i);
    }
    MusaDwgAc1018HandleEntry snapshot[4];
    memcpy(snapshot,index,sizeof(snapshot));
    const size_t saved=count;
    assert(!musa_dwg_ac1018_file_handle_index(file,sizeof(file),
                index,3u,&count));
    assert(!musa_dwg_ac1018_file_handle_index(file,sizeof(file),
                NULL,4u,&count));
    assert(!musa_dwg_ac1018_file_handle_index(file,sizeof(file),
                (MusaDwgAc1018HandleEntry*)(void*)(file+16u),
                4u,&count));
    assert(!musa_dwg_ac1018_file_handle_index(file,RAW_BYTES-30u,
                index,4u,&count));
    build(file);file[288u]^=1u;
    assert(!musa_dwg_ac1018_file_handle_index(file,sizeof(file),
                index,4u,&count));
    build(file);file[480u]^=1u;
    assert(!musa_dwg_ac1018_file_handle_index(file,sizeof(file),
                index,4u,&count));
    build(file);file[660u]^=1u;
    assert(!musa_dwg_ac1018_file_handle_index(file,sizeof(file),
                index,4u,&count));
    assert(count==saved&&!memcmp(snapshot,index,sizeof(index)));
    return 0;
}
