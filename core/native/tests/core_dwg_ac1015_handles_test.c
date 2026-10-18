/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015_handles.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const uint8_t header_sentinel[16]={
    0x95,0xa0,0x4e,0x28,0x99,0x82,0x1a,0xe5,
    0x5e,0x41,0xe0,0x5f,0x9d,0x3a,0x4d,0x00
};
static void put32(uint8_t *p,uint32_t v){
    for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(v>>(i*8u));
}
static void crc_map_page(uint8_t *p,size_t size){
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,p,size,&crc));
    p[size]=(uint8_t)(crc>>8);
    p[size+1u]=(uint8_t)crc;
}
static void make_fixture(uint8_t x[1024]){
    memset(x,0,1024u);
    memcpy(x,"AC1015",6u);x[12u]=1u;
    x[0x13u]=0xe4u;x[0x14u]=0x04u;
    put32(x+0x15u,3u);
    for(unsigned i=0u;i<3u;i++){
        size_t p=0x19u+9u*i;
        x[p]=(uint8_t)i;
        put32(x+p+1u,128u+88u*i);
        put32(x+p+5u,i==2u?20u:32u);
    }
    const size_t e=0x19u+27u;
    memcpy(x+e+2u,header_sentinel,16u);
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0u,x,e,&crc));
    crc^=0xa598u;
    x[e]=(uint8_t)crc;x[e+1u]=(uint8_t)(crc>>8);

    /* Size BE includes its own 2 bytes, not the two CRC bytes.
     * Page 1: (handle 1 -> file 400), (handle +2 -> file 300).
     * signed -100 uses a 0x40-only final byte (canonical!) */
    uint8_t *m=x+304u;
    const uint8_t p1[]={0u,8u,1u,0x90u,3u,2u,0xe4u,0x40u};
    memcpy(m,p1,sizeof(p1));crc_map_page(m,8u);
    /* Page 2: handle +4 -> file +20. */
    const uint8_t p2[]={0u,4u,4u,20u};
    memcpy(m+10u,p2,sizeof(p2));crc_map_page(m+10u,4u);
    m[16u]=0u;m[17u]=2u;crc_map_page(m+16u,2u);
}
static void fails_unchanged(const uint8_t *file,size_t n,
                            size_t cap){
    MusaDwgAc1015HandleEntry entries[4]={
        {111u,222u},{333u,444u},{555u,666u},{777u,888u}
    },snapshot[4];
    memcpy(snapshot,entries,sizeof(entries));
    size_t count=999u;
    assert(!musa_dwg_ac1015_handles_index(file,n,entries,cap,&count));
    assert(count==999u);
    assert(memcmp(snapshot,entries,sizeof(entries))==0);
}
int main(void){
    uint8_t file[1024],original[1024];
    make_fixture(file);memcpy(original,file,sizeof(file));
    size_t count=999u;
    assert(musa_dwg_ac1015_handles_index(file,sizeof(file),NULL,0u,&count));
    assert(count==3u);
    MusaDwgAc1015HandleEntry entries[3]={{0}};
    assert(musa_dwg_ac1015_handles_index(file,sizeof(file),entries,3u,&count));
    assert(count==3u);
    assert(entries[0].handle==1u&&entries[0].file_offset==400u);
    assert(entries[1].handle==3u&&entries[1].file_offset==300u);
    assert(entries[2].handle==7u&&entries[2].file_offset==320u);
    assert(memcmp(file,original,sizeof(file))==0);
    fails_unchanged(file,sizeof(file),2u);
    fails_unchanged(file,320u,4u);
    fails_unchanged(NULL,sizeof(file),4u);
    assert(!musa_dwg_ac1015_handles_index(file,sizeof(file),entries,3u,NULL));
    assert(!musa_dwg_ac1015_handles_index(file,sizeof(file),NULL,2u,&count));
    /* Wrong page CRC and malformed terminal must fail, never half-index. */
    file[307u]^=1u;
    fails_unchanged(file,sizeof(file),4u);
    file[307u]^=1u;
    file[321u]=3u;fails_unchanged(file,sizeof(file),4u);file[321u]=2u;
    /* Valid CRC with a zero handle delta still fails structurally. */
    file[306u]=0u;crc_map_page(file+304u,8u);
    fails_unchanged(file,sizeof(file),4u);
    make_fixture(file);
    /* A signed location delta escaping below absolute file 0 fails. */
    file[311u]=0xc0u; /* zero-magnitude negative is invalid */
    crc_map_page(file+304u,8u);
    fails_unchanged(file,sizeof(file),4u);
    make_fixture(file);
    /* Truncate signed delta by declaring shorter CRC-authenticated page. */
    file[305u]=7u;
    crc_map_page(file+304u,7u);
    fails_unchanged(file,sizeof(file),4u);
    assert(memcmp(original,file,sizeof(file))!=0);
    puts("PASS: first-party R2000 map pages, CRC, signed location, rollback, truncation");
    return 0;
}
