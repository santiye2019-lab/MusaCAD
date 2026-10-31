/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_object_window.h"
#include "musa_dwg1032_object_spans.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void small(void) {
    const uint8_t file[]={9,10,11,12,1,2,3,4,5,6,7,8};
    MusaDwg1032FilePage pages[]={{0,4,4},{4,8,4},{8,0,4}};
    const MusaDwg1032ObjectLocation entries[]={
        {10,2,5},{20,8,3}
    };
    MusaDwg1032ObjectIndex index={0};
    size_t order[3];
    assert(musa_dwg1032_file_pages_validate_window_directory(
        pages,3,sizeof(file),12,order,3));
    assert(musa_dwg1032_object_index_init(&index,entries,2,12));
    assert(musa_dwg1032_object_spans_prepare(&index,order,3));
    uint8_t out[8];
    memset(out,0xA5,sizeof(out));
    uint64_t length=999;
    assert(musa_dwg1032_object_candidate_read_window(
        file,sizeof(file),pages,3,&index,10,1,out,4,&length));
    const uint8_t expected[]={4,5,6,7};
    assert(length==5 && memcmp(out,expected,4)==0);
    for (size_t i=4;i<sizeof(out);++i) assert(out[i]==0xA5);
    memset(out,0xA5,sizeof(out));
    length=999;
    assert(musa_dwg1032_object_candidate_read_window(
        file,sizeof(file),pages,3,&index,20,2,out,1,&length));
    assert(out[0]==11 && length==3 && out[1]==0xA5);
    memset(out,0xA5,sizeof(out));
    length=999;
    assert(musa_dwg1032_object_candidate_read_window(
        file,sizeof(file),pages,3,&index,10,5,NULL,0,&length));
    assert(length==5);
    length=999;
    assert(!musa_dwg1032_object_candidate_read_window(
        file,sizeof(file),pages,3,&index,11,0,out,4,&length));
    assert(!musa_dwg1032_object_candidate_read_window(
        file,sizeof(file),pages,3,&index,10,6,out,1,&length));
    assert(!musa_dwg1032_object_candidate_read_window(
        file,sizeof(file),pages,3,&index,10,4,out,2,&length));
    assert(!musa_dwg1032_object_candidate_read_window(
        file,sizeof(file),pages,3,&index,10,UINT64_MAX,out,1,&length));
    assert(!musa_dwg1032_object_candidate_read_window(
        file,sizeof(file),pages,3,&index,10,0,NULL,1,&length));
    assert(!musa_dwg1032_object_candidate_read_window(
        file,sizeof(file),pages,3,&index,10,0,out,1,NULL));
    assert(!musa_dwg1032_object_candidate_read_window(
        file,sizeof(file),pages,3,&index,10,0,out,1,
        (uint64_t *)(void *)out));
    assert(!musa_dwg1032_object_candidate_read_window(
        file,sizeof(file),pages,3,&index,10,0,
        (uint8_t *)(void *)entries,1,&length));
    assert(!musa_dwg1032_object_candidate_read_window(
        file,sizeof(file),pages,3,&index,10,0,
        (uint8_t *)(void *)file,1,&length));
    assert(!musa_dwg1032_object_candidate_read_window(
        file,sizeof(file),pages,3,&index,10,0,out,1,
        (uint64_t *)(void *)pages));
    assert(length==999);
    for (size_t i=0;i<sizeof(out);++i) assert(out[i]==0xA5);

    /* A late bad physical page must not expose a partial object window. */
    pages[1].file_offset=sizeof(file);
    assert(!musa_dwg1032_object_candidate_read_window(
        file,sizeof(file),pages,3,&index,10,0,out,5,&length));
    assert(length==999);
    for (size_t i=0;i<sizeof(out);++i) assert(out[i]==0xA5);
    pages[1].file_offset=8;
    pages[2].file_offset=sizeof(file);
    assert(!musa_dwg1032_object_candidate_read_window(
        file,sizeof(file),pages,3,&index,20,0,out,3,&length));
    assert(length==999);
    for (size_t i=0;i<sizeof(out);++i) assert(out[i]==0xA5);
}

static void large_reverse_physical(void) {
    enum { N=20000 };
    uint8_t *file=malloc(N);
    MusaDwg1032FilePage *pages=malloc(N*sizeof(*pages));
    MusaDwg1032ObjectLocation *entries=malloc(N*sizeof(*entries));
    size_t *order=malloc(N*sizeof(*order));
    assert(file && pages && entries && order);
    for (size_t i=0;i<N;++i) {
        file[i]=(uint8_t)((i*29u)^(i>>4));
        pages[i]=(MusaDwg1032FilePage){
            (uint64_t)i,(uint64_t)(N-1u-i),1u};
        entries[i]=(MusaDwg1032ObjectLocation){
            (uint64_t)(2u*i+1u),(uint64_t)i,1u};
    }
    MusaDwg1032ObjectIndex index={0};
    assert(musa_dwg1032_file_pages_validate_window_directory(
        pages,N,N,N,order,N));
    assert(musa_dwg1032_object_index_init(&index,entries,N,N));
    assert(musa_dwg1032_object_spans_prepare(&index,order,N));
    for (size_t i=0;i<N;++i) {
        const size_t at=(i*7919u)%N;
        uint8_t value=0xFF;
        uint64_t length=99;
        assert(musa_dwg1032_object_candidate_read_window(
            file,N,pages,N,&index,(uint64_t)(2u*at+1u),
            0,&value,1,&length));
        assert(value==file[N-1u-at] && length==1);
    }
    uint8_t value=0xA5;
    uint64_t length=77;
    pages[N-1u].file_offset=N;
    assert(!musa_dwg1032_object_candidate_read_window(
        file,N,pages,N,&index,(uint64_t)(2u*N-1u),
        0,&value,1,&length));
    assert(value==0xA5 && length==77);
    free(order);free(entries);free(pages);free(file);
}

int main(void) {
    small();
    large_reverse_physical();
    return 0;
}
