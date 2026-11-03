/* Source-authenticated non-monotone AC1032 physical page-ID regression.
 * Synthetic raw DWG envelope only: NO object or geometry decode is tested.
 * Preserve strict AC1018 ordering and fail-atomic duplicate rejection. */
#include "musacad_dwg_ac1032_page_map.h"
#include "musacad_dwg_ac1018_page_map.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

static void put32(uint8_t *p,uint32_t v){
    for(unsigned i=0u;i<4u;i++)p[i]=(uint8_t)(v>>(i*8u));
}
static uint32_t adler(uint32_t seed,const uint8_t *p,size_t n){
    uint64_t lo=seed&65535u,hi=seed>>16u;
    for(size_t i=0u;i<n;i++){lo+=p[i];hi+=lo;lo%=65521u;hi%=65521u;}
    return (uint32_t)((hi<<16u)|lo);
}
static void fixture(uint8_t out[448],int duplicate,int legacy){
    memset(out,0,448u);
    memcpy(out,legacy?"AC1018":"AC1032",6u);
    uint8_t plain[0x6c]={0};
    memcpy(plain,"AcFssFcAJMB",11u);
    put32(plain+0x10u,0x6cu);
    put32(plain+0x40u,3u);
    put32(plain+0x50u,3u);
    put32(plain+0x5cu,1u);
    put32(plain+0x54u,128u); /* final page starts at 0x180 */
    uint32_t state=1u;
    for(size_t i=0u;i<sizeof(plain);i++){
        state=state*UINT32_C(0x343fd)+UINT32_C(0x269ec3);
        out[0x80u+i]=(uint8_t)(plain[i]^(uint8_t)(state>>16u));
    }
    const size_t pos=384u;
    put32(out+pos,UINT32_C(0x41630e3b));
    put32(out+pos+4u,24u);
    put32(out+pos+8u,27u);
    put32(out+pos+12u,2u);
    uint8_t *payload=out+pos+20u;
    payload[0]=0u;payload[1]=6u; /* exactly 24 literal bytes */
    put32(payload+2u,duplicate?1u:2u);
    put32(payload+6u,64u);
    put32(payload+10u,1u);
    put32(payload+14u,64u);
    put32(payload+18u,3u);
    put32(payload+22u,64u);
    payload[26u]=0x11u;
    put32(out+pos+16u,adler(
         adler(0u,out+pos,20u),payload,27u));
}
int main(void){
    uint8_t bytes[448]={0};
    fixture(bytes,0,0);
    size_t n=91u;
    MusaDwgAc1032PageLocation entries[3]={
        {78u,56u,34u},{79u,57u,35u},{80u,58u,36u}};
    assert(musa_dwg_ac1032_page_locations(bytes,sizeof(bytes),NULL,0u,&n));
    assert(n==3u);
    assert(musa_dwg_ac1032_page_locations(
        bytes,sizeof(bytes),entries,3u,&n));
    /* Numeric ID order differs from physical file order. */
    assert(entries[0].page_id==1u&&entries[0].file_offset==320u);
    assert(entries[1].page_id==2u&&entries[1].file_offset==256u);
    assert(entries[2].page_id==3u&&entries[2].file_offset==384u);
    for(size_t i=0u;i<3u;i++)assert(entries[i].file_span_bytes==64u);
    const size_t saved_count=n;
    MusaDwgAc1032PageLocation saved[3];
    memcpy(saved,entries,sizeof(saved));

    fixture(bytes,1,0);
    assert(!musa_dwg_ac1032_page_locations(bytes,sizeof(bytes),NULL,0u,&n));
    assert(!musa_dwg_ac1032_page_locations(
        bytes,sizeof(bytes),entries,3u,&n));
    assert(n==saved_count&&!memcmp(entries,saved,sizeof(saved)));

    fixture(bytes,0,0);
    bytes[384u+20u+2u]^=0x01u; /* CRC-protected map must fail */
    assert(!musa_dwg_ac1032_page_locations(
        bytes,sizeof(bytes),entries,3u,&n));
    assert(n==saved_count&&!memcmp(entries,saved,sizeof(saved)));

    /* R2004 behavior remains strict, not silently broadened. */
    fixture(bytes,0,1);
    assert(!musa_dwg_ac1018_page_locations(
        bytes,sizeof(bytes),NULL,0u,&n));
    assert(n==saved_count);
    return 0;
}
