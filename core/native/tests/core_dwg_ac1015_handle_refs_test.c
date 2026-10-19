/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015_handle_refs.h"
#include "musacad_dwg_ac1015_object.h"
#include "musacad_crc16.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct TestBits{uint8_t data[128];size_t at;}TestBits;
static void put(TestBits *w,uint32_t v,unsigned n){
    CHECK(n>=1u&&n<=32u);
    for(unsigned i=0;i<n;i++){
        const size_t p=w->at++;
        CHECK(p<sizeof(w->data)*8u);
        if((v>>(n-i-1u))&1u)w->data[p/8u]|=(uint8_t)(1u<<(7u-p%8u));
    }
}
static void handle(TestBits *w,unsigned code,uint64_t value,unsigned count){
    put(w,code,4u);put(w,count,4u);
    for(unsigned i=count;i>0u;i--)put(w,(uint8_t)(value>>(8u*(i-1u))),8u);
}
static MusaDwgAc1015Bits cursor(const TestBits *w,size_t first,size_t last){
    MusaDwgAc1015Bits c={0};
    CHECK(musa_dwg_ac1015_bits_init(&c,w->data,sizeof(w->data),first,last));
    return c;
}
static void invalid(const TestBits *w,uint64_t base){
    MusaDwgAc1015Bits c=cursor(w,0u,w->at);
    MusaDwgAc1015HandleRef result={0};
    result.target=33u;result.code=42u;result.encoded_value=211u;
    MusaDwgAc1015HandleRef unchanged=result;
    CHECK(!musa_dwg_ac1015_read_handle_ref(&c,base,&result));
    CHECK(c.bit_pos==0u&&!memcmp(&result,&unchanged,sizeof(result)));
}
static void single(void){
    TestBits w={0};put(&w,5u,3u);
    handle(&w,0u,UINT64_C(0x1234),2u);
    handle(&w,2u,0x21u,1u);
    handle(&w,3u,0x22u,1u);
    handle(&w,4u,0x23u,1u);
    handle(&w,5u,0x24u,1u);
    handle(&w,6u,0u,0u);
    handle(&w,8u,0u,0u);
    handle(&w,10u,0x100u,2u);
    handle(&w,12u,0x10u,1u);
    MusaDwgAc1015Bits c=cursor(&w,3u,w.at);
    uint64_t result_expected[]={0x1234u,0x21u,0x22u,0x23u,0x24u,0x201u,0x1ffu,0x300u,0x1f0u};
    uint8_t codes[]={0u,2u,3u,4u,5u,6u,8u,10u,12u};
    for(unsigned i=0u;i<9u;i++){
        MusaDwgAc1015HandleRef ref={0};
        CHECK(musa_dwg_ac1015_read_handle_ref(&c,0x200u,&ref));
        CHECK(ref.target==result_expected[i]&&ref.code==codes[i]);
        CHECK(ref.relative==(uint8_t)(i>=5u));
    }
    CHECK(c.bit_pos==w.at);
    MusaDwgAc1015HandleRef o={0};o.target=999u;
    CHECK(!musa_dwg_ac1015_read_handle_ref(&c,0x200u,&o));
    CHECK(c.bit_pos==w.at&&o.target==999u);

    w=(TestBits){0};handle(&w,1u,1u,1u);invalid(&w,9u); /* code reserved */
    w=(TestBits){0};handle(&w,7u,0u,0u);invalid(&w,9u);
    w=(TestBits){0};handle(&w,0u,0u,0u);invalid(&w,9u);
    w=(TestBits){0};handle(&w,4u,0u,1u);invalid(&w,9u);
    w=(TestBits){0};handle(&w,4u,0x01u,2u);invalid(&w,9u); /* leading 00 */
    w=(TestBits){0};handle(&w,4u,0xffffu,2u);invalid(&w,0u); /* missing base */
    w=(TestBits){0};handle(&w,6u,1u,1u);invalid(&w,9u);
    w=(TestBits){0};handle(&w,8u,0u,0u);invalid(&w,1u); /* target 0 */
    w=(TestBits){0};handle(&w,6u,0u,0u);invalid(&w,UINT64_MAX); /* overflow */
    w=(TestBits){0};handle(&w,10u,2u,1u);invalid(&w,UINT64_MAX-1u);
    w=(TestBits){0};handle(&w,12u,3u,1u);invalid(&w,2u);
    w=(TestBits){0};handle(&w,4u,0x1234u,2u);
    MusaDwgAc1015Bits short_cursor=cursor(&w,0u,w.at-1u);
    MusaDwgAc1015HandleRef sentinel={0};sentinel.target=0x55u;
    CHECK(!musa_dwg_ac1015_read_handle_ref(&short_cursor,0x200u,&sentinel));
    CHECK(short_cursor.bit_pos==0u&&sentinel.target==0x55u);
}
static void set_raw(TestBits*w,size_t at,uint32_t v){
    for(unsigned i=0u;i<32u;i++){
        const size_t pos=at+i;
        const uint8_t mask=(uint8_t)(1u<<(7u-pos%8u));
        if(((v>>(8u*(i/8u)))>>(7u-(i%8u)))&1u)
            w->data[pos/8u]|=mask;
        else w->data[pos/8u]&=(uint8_t)~mask;
    }
}
static void make_record(uint8_t file[512],int extra,int corrupt_pad){
    TestBits w={0};
    put(&w,1u,2u);put(&w,19u,8u);/* BS type 19 */
    const size_t rl=w.at;
    for(unsigned i=0u;i<4u;i++)put(&w,0u,8u);
    /* 3 opaque prehandle bits, intentionally not decoded */
    put(&w,5u,3u);
    set_raw(&w,rl,(uint32_t)w.at);
    handle(&w,5u,0x7fu,1u);
    handle(&w,6u,0u,0u); /* target 0x3b */
    if(extra)handle(&w,5u,0x78u,1u);
    while(w.at%8u)put(&w,corrupt_pad?1u:0u,1u);
    size_t bytes=w.at/8u;
    CHECK(bytes<127u);
    memset(file,0,512u);memcpy(file,"AC1015",6u);
    file[128]=(uint8_t)bytes;
    file[129]=(uint8_t)(bytes>>8u);
    memcpy(file+130u,w.data,bytes);
    uint16_t crc=0u;
    CHECK(musa_core_crc16_arc(0xc0c1u,file+128u,bytes+2u,&crc));
    file[130u+bytes]=(uint8_t)crc;
    file[131u+bytes]=(uint8_t)(crc>>8u);
}
static void tail(void){
    uint8_t f[512],original[512];
    make_record(f,0,0);memcpy(original,f,sizeof(f));
    const MusaDwgAc1015HandleEntry entry={0x3au,128u};
    MusaDwgAc1015ObjectRecord r={0};
    CHECK(musa_dwg_ac1015_read_object_record(f,sizeof(f),&entry,&r));
    CHECK(r.record_crc_verified&&r.geometry_decoded==0);
    MusaDwgAc1015HandleRef out[2]={0};
    CHECK(musa_dwg_ac1015_read_exact_handle_tail(&r,2u,out,2u));
    CHECK(out[0].target==0x7fu&&out[0].code==5u);
    CHECK(out[1].target==0x3bu&&out[1].code==6u);
    CHECK(!memcmp(original,f,sizeof(f)));
    MusaDwgAc1015HandleRef before[2];memcpy(before,out,sizeof(out));
    CHECK(!musa_dwg_ac1015_read_exact_handle_tail(&r,1u,out,2u));
    CHECK(!memcmp(before,out,sizeof(out)));
    CHECK(!musa_dwg_ac1015_read_exact_handle_tail(&r,2u,out,1u));
    CHECK(!memcmp(before,out,sizeof(out)));
    MusaDwgAc1015ObjectRecord tampered=r;
    tampered.record_crc_verified=0;
    CHECK(!musa_dwg_ac1015_read_exact_handle_tail(&tampered,2u,out,2u));
    CHECK(!memcmp(before,out,sizeof(out)));
    make_record(f,1,0);
    CHECK(musa_dwg_ac1015_read_object_record(f,sizeof(f),&entry,&r));
    CHECK(!musa_dwg_ac1015_read_exact_handle_tail(&r,2u,out,2u));
    CHECK(!memcmp(before,out,sizeof(out)));
    make_record(f,0,1);
    CHECK(musa_dwg_ac1015_read_object_record(f,sizeof(f),&entry,&r));
    CHECK(!musa_dwg_ac1015_read_exact_handle_tail(&r,2u,out,2u));
    CHECK(!memcmp(before,out,sizeof(out)));
}
static void indexed_resolution(void){
    uint8_t f[512];
    make_record(f,0,0);
    const MusaDwgAc1015HandleEntry own={0x3au,128u};
    MusaDwgAc1015ObjectRecord record={0};
    CHECK(musa_dwg_ac1015_read_object_record(f,sizeof(f),&own,&record));
    MusaDwgAc1015HandleRef refs[2]={0};
    CHECK(musa_dwg_ac1015_read_exact_handle_tail(&record,2u,refs,2u));
    MusaDwgAc1015HandleEntry map[3]={
        {0x3au,128u},{0x3bu,256u},{0x7fu,384u}
    };
    MusaDwgAc1015ResolvedRef out[2]={0};
    CHECK(musa_dwg_ac1015_resolve_refs_in_map(
        &record,refs,2u,map,3u,sizeof(f),out,2u));
    CHECK(out[0].ref.target==0x7fu&&out[0].file_offset==384u);
    CHECK(out[1].ref.target==0x3bu&&out[1].file_offset==256u);
    const MusaDwgAc1015ResolvedRef unchanged[2]={out[0],out[1]};
#define UNTOUCHED() CHECK(memcmp(out,unchanged,sizeof(out))==0)
    map[2].handle=0x7eu; /* Unknown target. */
    CHECK(!musa_dwg_ac1015_resolve_refs_in_map(
        &record,refs,2u,map,3u,sizeof(f),out,2u));UNTOUCHED();
    map[2].handle=0x7fu;
    map[1].handle=0x3au; /* Duplicate index key. */
    CHECK(!musa_dwg_ac1015_resolve_refs_in_map(
        &record,refs,2u,map,3u,sizeof(f),out,2u));UNTOUCHED();
    map[1].handle=0x3bu;
    map[2].handle=0x35u; /* Non-monotonic index. */
    CHECK(!musa_dwg_ac1015_resolve_refs_in_map(
        &record,refs,2u,map,3u,sizeof(f),out,2u));UNTOUCHED();
    map[2].handle=0x7fu;
    map[1].file_offset=sizeof(f); /* Beyond source file. */
    CHECK(!musa_dwg_ac1015_resolve_refs_in_map(
        &record,refs,2u,map,3u,sizeof(f),out,2u));UNTOUCHED();
    map[1].file_offset=256u;
    map[0].file_offset=127u; /* Origin does not match CRC record. */
    CHECK(!musa_dwg_ac1015_resolve_refs_in_map(
        &record,refs,2u,map,3u,sizeof(f),out,2u));UNTOUCHED();
    map[0].file_offset=128u;
    MusaDwgAc1015ObjectRecord invalid=record;
    invalid.record_crc_verified=0;
    CHECK(!musa_dwg_ac1015_resolve_refs_in_map(
        &invalid,refs,2u,map,3u,sizeof(f),out,2u));UNTOUCHED();
    CHECK(!musa_dwg_ac1015_resolve_refs_in_map(
        &record,refs,2u,map,3u,sizeof(f),out,1u));UNTOUCHED();
    CHECK(!musa_dwg_ac1015_resolve_refs_in_map(
        &record,refs,2u,map,3u,sizeof(f),NULL,0u));UNTOUCHED();
    CHECK(musa_dwg_ac1015_resolve_refs_in_map(
        &record,NULL,0u,map,3u,sizeof(f),NULL,0u));
#undef UNTOUCHED
}
int main(void){
    single();tail();indexed_resolution();
    puts("PASS: R2000 H codec, CRC-backed tail, strict indexed reference targets and rollback");
    return 0;
}
