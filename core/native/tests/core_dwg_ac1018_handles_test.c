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
    assert(!musa_dwg_ac1018_handles_index(buf,size-4u,40u,out,6u,&count));
    assert(!musa_dwg_ac1018_handles_index(buf,size,32u,out,6u,&count));
    assert(!musa_dwg_ac1018_handles_index(buf,size,0u,out,6u,&count));
    assert(!musa_dwg_ac1018_handles_index(buf,size,40u,NULL,1u,&count));
    assert(!memcmp(prev,out,sizeof(prev))&&count==saved);
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
