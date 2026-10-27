/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_handles.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* Hand-authored independent synthetic R2004 pages: NO user DWG content.
 * The handle value accumulates across pages, object offset resets each page.
 * Third page explicitly tests negative signed offset delta.
 */
static void crc_page(uint8_t *p,size_t size) {
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,p,size,&crc));
    p[size]=(uint8_t)(crc>>8u);
    p[size+1u]=(uint8_t)crc;
}
static size_t make_map(uint8_t *dst){
    /* page 1: handles 1,2; positions 4,16 */
    static const uint8_t p1[]={0,6,1,4,1,12,0,0};
    /* page 2: handles 3,5; positions 0,32 */
    static const uint8_t p2[]={0,6,1,0,2,32,0,0};
    /* page 3: handles 6,7; positions 12,6 */
    static const uint8_t p3[]={0,6,1,12,1,0x46,0,0};
    /* mandatory terminal page */
    static const uint8_t end[]={0,2,0,0};
    memcpy(dst,p1,sizeof(p1));crc_page(dst,6u);
    memcpy(dst+8u,p2,sizeof(p2));crc_page(dst+8u,6u);
    memcpy(dst+16u,p3,sizeof(p3));crc_page(dst+16u,6u);
    memcpy(dst+24u,end,sizeof(end));crc_page(dst+24u,2u);
    return 28u;
}
int main(void){
    uint8_t buf[80]={0};
    size_t size=make_map(buf),count=1234u;
    assert(musa_dwg_ac1018_handles_index(buf,size,40u,NULL,0u,&count));
    assert(count==6u);
    MusaDwgAc1018HandleEntry out[6]={0};
    assert(musa_dwg_ac1018_handles_index(buf,size,40u,out,6u,&count));
    const uint64_t handles[]={1,2,3,5,6,7};
    const uint64_t offsets[]={4,16,0,32,12,6};
    for(unsigned i=0;i<6u;i++){
        assert(out[i].handle==handles[i]);
        assert(out[i].object_offset==offsets[i]);
    }
    MusaDwgAc1018HandleEntry prev[6];
    memcpy(prev,out,sizeof(prev));const size_t saved=count;
    assert(!musa_dwg_ac1018_handles_index(buf,size,40u,out,5u,&count));
    assert(!memcmp(prev,out,sizeof(prev))&&count==saved);
    /* Bad caller buffers must never damage original DWG bytes or leak a
     * partially written index, even if all handle-map CRCs were valid. */
    MusaDwgAc1018HandleEntry source_alias[6]={{0}};
    memcpy(source_alias,buf,size);
    assert(!musa_dwg_ac1018_handles_index(
        (const uint8_t*)source_alias,size,40u,source_alias,6u,&count));
    assert(!memcmp(source_alias,buf,size)&&count==saved);
    size_t count_alias[8]={0};
    memcpy(count_alias,buf,size);
    assert(!musa_dwg_ac1018_handles_index(
        (const uint8_t*)count_alias,size,40u,NULL,0u,count_alias));
    assert(!memcmp(count_alias,buf,size)&&count==saved);
    union {
        MusaDwgAc1018HandleEntry entries[6];
        size_t count;
    } output_alias;
    memcpy(output_alias.entries,prev,sizeof(prev));
    assert(!musa_dwg_ac1018_handles_index(
        buf,size,40u,output_alias.entries,6u,&output_alias.count));
    assert(!memcmp(output_alias.entries,prev,sizeof(prev))&&count==saved);
    assert(!musa_dwg_ac1018_handles_index(
        buf,size,40u,out,SIZE_MAX/sizeof(out[0])+1u,&count));
    assert(!memcmp(prev,out,sizeof(prev))&&count==saved);
    assert(!musa_dwg_ac1018_handles_index(buf,size-4u,40u,out,6u,&count));
    assert(!musa_dwg_ac1018_handles_index(buf,size,32u,out,6u,&count));
    assert(!musa_dwg_ac1018_handles_index(buf,size,0u,out,6u,&count));
    assert(!musa_dwg_ac1018_handles_index(buf,size,40u,NULL,1u,&count));
    assert(!memcmp(prev,out,sizeof(prev))&&count==saved);
    /* Synthetic multi-page R2018 continuous object-offset profile. */
    uint8_t modern[80]={0};
    memcpy(modern,buf,size);
    modern[11]=4u;modern[13]=16u;crc_page(modern+8u,6u);
    modern[19]=4u;crc_page(modern+16u,6u);
    MusaDwgAc1018HandleEntry continuous[6]={{0}};
    const uint64_t offsets_continuous[]={4u,16u,20u,36u,40u,34u};
    assert(musa_dwg_ac1018_handles_index_continuous_offsets(
           modern,size,64u,NULL,0u,&count)&&count==6u);
    assert(musa_dwg_ac1018_handles_index_continuous_offsets(
           modern,size,64u,continuous,6u,&count)&&count==6u);
    for(unsigned i=0u;i<6u;i++){
        assert(continuous[i].handle==handles[i]);
        assert(continuous[i].object_offset==offsets_continuous[i]);
    }
    MusaDwgAc1018HandleEntry saved_continuous[6];
    memcpy(saved_continuous,continuous,sizeof(continuous));
    const size_t saved_continuous_count=count;
    assert(!musa_dwg_ac1018_handles_index_continuous_offsets(
           modern,size,40u,continuous,6u,&count));
    assert(!memcmp(continuous,saved_continuous,sizeof(continuous))&&
           count==saved_continuous_count);
    modern[13]^=1u;
    assert(!musa_dwg_ac1018_handles_index_continuous_offsets(
           modern,size,64u,continuous,6u,&count));
    assert(!memcmp(continuous,saved_continuous,sizeof(continuous))&&
           count==saved_continuous_count);
    uint8_t damaged[80]={0};
    memcpy(damaged,buf,size);
    damaged[18]^=0x1u; /* page 3 CRC must reject */
    assert(!musa_dwg_ac1018_handles_index(damaged,size,40u,out,6u,&count));
    memcpy(damaged,buf,size);
    damaged[7]^=0x01u; /* CRC corruption */
    assert(!musa_dwg_ac1018_handles_index(damaged,size,40u,out,6u,&count));
    memcpy(damaged,buf,size);
    damaged[4]=0u; /* handle delta 0 is noncanonical; recompute CRC */
    crc_page(damaged,6u);
    assert(!musa_dwg_ac1018_handles_index(damaged,size,40u,out,6u,&count));
    memcpy(damaged,buf,size);
    damaged[4]=0x81u; /* overlong/unterminated within page */
    crc_page(damaged,6u);
    assert(!musa_dwg_ac1018_handles_index(damaged,size,40u,out,6u,&count));
    memcpy(damaged,buf,size);
    damaged[1]=0xffu; /* declared page length too large */
    assert(!musa_dwg_ac1018_handles_index(damaged,size,40u,out,6u,&count));
    assert(!memcmp(prev,out,sizeof(prev))&&count==saved);
    return 0;
}
