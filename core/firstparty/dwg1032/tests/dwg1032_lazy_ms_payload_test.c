/* SPDX-License-Identifier: MIT
 * Synthetic RAW page and MS envelope vectors only; NOT real AC1032 DWGs.
 */
#include "musa_dwg1032_object_window.h"
#include "musa_dwg1032_object_spans.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void small_reordered_pages(void) {
    /* Logical [04 00 01 02] [03 04 ee ff] [02 00 09 08],
     * with physical page order deliberately different. */
    uint8_t file[]={3,4,0xee,0xff,2,0,9,8,4,0,1,2};
    MusaDwg1032FilePage pages[]={{0,8,4},{4,0,4},{8,4,4}};
    const MusaDwg1032ObjectLocation entries[]={{10,0,8},{20,8,4}};
    MusaDwg1032ObjectIndex index={0};
    size_t order[3];
    assert(musa_dwg1032_file_pages_validate_window_directory(
        pages,3,sizeof(file),12,order,3));
    assert(musa_dwg1032_object_index_init(&index,entries,2,12));
    assert(musa_dwg1032_object_spans_prepare(&index,order,3));
    uint8_t out[8];
    memset(out,0xa5,sizeof(out));
    uint64_t length=77;
    assert(musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,1,4,out,3,&length));
    assert(length==4 && out[0]==2 && out[1]==3 && out[2]==4);
    for (size_t i=3;i<sizeof(out);++i) assert(out[i]==0xa5);
    memset(out,0xa5,sizeof(out));
    assert(musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,20,0,2,out,2,&length));
    assert(length==2 && out[0]==9 && out[1]==8);
    memset(out,0xa5,sizeof(out));
    length=77;
    assert(musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,4,4,NULL,0,&length));
    assert(length==4);
    length=77;
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,0,3,out,3,&length));
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,3,4,out,2,&length));
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,11,0,4,out,1,&length));
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,0,0,out,1,&length));
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,0,4,NULL,1,&length));
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,0,4,out,1,NULL));
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,0,4,out,1,
        (uint64_t *)(void *)out));
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,0,4,out,1,
        (uint64_t *)(void *)pages));
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,0,4,file,1,&length));
    assert(length==77 && out[0]==0xa5);
    /* Even with a cached single lookup, output may not corrupt metadata. */
    const MusaDwg1032ObjectIndex saved_index=index;
    MusaDwg1032FilePage saved_pages[3];
    memcpy(saved_pages,pages,sizeof(pages));
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,0,4,
        (uint8_t *)(void *)&index,1,&length));
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,0,4,
        (uint8_t *)(void *)pages,1,&length));
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,0,4,
        out,1,(uint64_t *)(void *)&index));
    assert(memcmp(&saved_index,&index,sizeof(index))==0);
    assert(memcmp(saved_pages,pages,sizeof(pages))==0);
    assert(length==77 && out[0]==0xa5);
    file[8]=7; /* Claimed record exceeds its candidate span. */
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,0,8,out,1,&length));
    file[8]=0; /* Zero-sized record. */
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,0,8,out,1,&length));
    file[8]=4;
    pages[1].file_offset=sizeof(file); /* Late touched page is truncated. */
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,sizeof(file),pages,3,&index,10,1,4,out,3,&length));
    assert(length==77);
    for (size_t i=0;i<sizeof(out);++i) assert(out[i]==0xa5);
}

static void extended_prefix_across_pages(void) {
    enum { BYTES=32780, PAYLOAD=32770 };
    uint8_t *file=malloc(BYTES);
    assert(file);
    memset(file,0x5a,BYTES);
    /* MS length: 0x8002 followed by 0x0001 => 32770 bytes. */
    file[0]=2;file[1]=0x80;file[2]=1;file[3]=0;
    for (size_t i=0;i<PAYLOAD;++i)
        file[4u+i]=(uint8_t)(i*17u);
    MusaDwg1032FilePage pages[]={{0,0,3},{3,3,BYTES-3}};
    MusaDwg1032ObjectLocation entries[]={{33,0,BYTES}};
    MusaDwg1032ObjectIndex index={0};
    size_t order[2];
    assert(musa_dwg1032_file_pages_validate_window_directory(
        pages,2,BYTES,BYTES,order,2));
    assert(musa_dwg1032_object_index_init(&index,entries,1,BYTES));
    assert(musa_dwg1032_object_spans_prepare(&index,order,2));
    uint8_t out[4]={0xa5,0xa5,0xa5,0xa5};
    uint64_t length=99;
    assert(musa_dwg1032_object_ms_payload_read_window(
        file,BYTES,pages,2,&index,33,32768,PAYLOAD,out,2,&length));
    assert(length==PAYLOAD && out[0]==(uint8_t)(32768u*17u) &&
           out[1]==(uint8_t)(32769u*17u));
    assert(out[2]==0xa5 && out[3]==0xa5);
    assert(musa_dwg1032_object_ms_payload_read_window(
        file,BYTES,pages,2,&index,33,PAYLOAD,PAYLOAD,NULL,0,&length));
    out[0]=0xa5;out[1]=0xa5;length=99;
    file[3]=0x80; /* Second MS word may not have its high bit set. */
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,BYTES,pages,2,&index,33,0,PAYLOAD,out,2,&length));
    file[3]=0;file[2]=0; /* Noncanonical extended zero word. */
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,BYTES,pages,2,&index,33,0,PAYLOAD,out,2,&length));
    file[2]=1;
    entries[0].length=PAYLOAD+3u; /* Record is one byte too large. */
    assert(musa_dwg1032_object_spans_prepare(&index,order,2));
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,BYTES,pages,2,&index,33,0,PAYLOAD,out,2,&length));
    entries[0].length=BYTES;
    assert(musa_dwg1032_object_spans_prepare(&index,order,2));
    pages[1].file_offset=BYTES; /* Prefix continuation missing. */
    assert(!musa_dwg1032_object_ms_payload_read_window(
        file,BYTES,pages,2,&index,33,0,PAYLOAD,out,2,&length));
    assert(length==99 && out[0]==0xa5 && out[1]==0xa5);
    free(file);
}

static void large_reverse_physical(void) {
    enum { N=20000, STRIDE=4 };
    uint8_t *file=malloc(N*STRIDE);
    MusaDwg1032FilePage *pages=malloc(N*sizeof(*pages));
    MusaDwg1032ObjectLocation *entries=malloc(N*sizeof(*entries));
    size_t *order=malloc(N*sizeof(*order));
    assert(file && pages && entries && order);
    for (size_t i=0;i<N;++i) {
        const size_t physical=(N-1u-i)*STRIDE;
        file[physical]=2;
        file[physical+1u]=0;
        file[physical+2u]=(uint8_t)i;
        file[physical+3u]=(uint8_t)(i>>8u);
        pages[i]=(MusaDwg1032FilePage){
            (uint64_t)(i*STRIDE),(uint64_t)physical,STRIDE};
        entries[i]=(MusaDwg1032ObjectLocation){
            (uint64_t)(i+1u),(uint64_t)(i*STRIDE),STRIDE};
    }
    MusaDwg1032ObjectIndex index={0};
    assert(musa_dwg1032_file_pages_validate_window_directory(
        pages,N,N*STRIDE,N*STRIDE,order,N));
    assert(musa_dwg1032_object_index_init(&index,entries,N,N*STRIDE));
    assert(musa_dwg1032_object_spans_prepare(&index,order,N));
    for (size_t i=0;i<N;++i) {
        const size_t at=(i*7919u)%N;
        uint8_t out[2]={0xa5,0xa5};
        uint64_t length=99;
        assert(musa_dwg1032_object_ms_payload_read_window(
            file,N*STRIDE,pages,N,&index,(uint64_t)(at+1u),
            0,2,out,2,&length));
        assert(length==2 && out[0]==(uint8_t)at &&
               out[1]==(uint8_t)(at>>8u));
    }
    free(order);free(entries);free(pages);free(file);
}

int main(void) {
    small_reordered_pages();
    extended_prefix_across_pages();
    large_reverse_physical();
    return 0;
}
