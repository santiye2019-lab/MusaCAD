/* SPDX-License-Identifier: MIT
 * Independent synthetic RAW-page differential and rollback regression.
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

static void cross_page_and_rollback(void) {
    const uint8_t file[]={
        0x90,0x91,0x92,0x93,0x50,0x51,0x52,0x53,
        0x10,0x11,0x12,0x13
    };
    MusaDwg1032FilePage pages[]={
        {0,8,4},{4,4,4},{8,0,4}
    };
    MusaDwg1032ObjectLocation entries[]={
        {10,0,2},{20,2,8},{30,10,2}
    };
    MusaDwg1032ObjectIndex index={0};
    size_t order[3];
    assert(musa_dwg1032_file_pages_validate_window_directory(
        pages,3,sizeof(file),sizeof(file),order,3));
    assert(musa_dwg1032_object_index_init(
        &index,entries,3,sizeof(file)));
    assert(musa_dwg1032_object_spans_prepare(&index,order,3));
    uint8_t old_out[8],new_out[8],scratch[12];
    uint64_t old_length=99,new_length=77;
    memset(old_out,0xa5,sizeof(old_out));
    memset(new_out,0xa5,sizeof(new_out));
    assert(musa_dwg1032_object_candidate_read_window_at_position(
        file,sizeof(file),pages,3,&index,1,1,old_out,7,&old_length));
    assert(musa_dwg1032_object_candidate_read_window_scratch_at_position(
        file,sizeof(file),pages,3,&index,1,1,new_out,7,&new_length,
        scratch,sizeof(scratch)));
    assert(old_length==8 && new_length==8);
    assert(!memcmp(old_out,new_out,sizeof(old_out)));
    const uint8_t expected[]={0x13,0x50,0x51,0x52,0x53,0x90,0x91};
    assert(!memcmp(new_out,expected,sizeof(expected)));
    assert(new_out[7]==0xa5);
    memset(new_out,0xa5,sizeof(new_out));
    new_length=77;
    assert(musa_dwg1032_object_candidate_read_window_scratch(
        file,sizeof(file),pages,3,&index,20,1,new_out,7,&new_length,
        scratch,sizeof(scratch)));
    assert(new_length==8 && !memcmp(new_out,expected,sizeof(expected)));
    assert(musa_dwg1032_object_candidate_read_window_scratch(
        file,sizeof(file),pages,3,&index,20,8,NULL,0,&new_length,
        NULL,0));
    assert(new_length==8);
    memset(new_out,0xa5,sizeof(new_out));
    new_length=77;
    assert(!musa_dwg1032_object_candidate_read_window_scratch(
        file,sizeof(file),pages,3,&index,999,0,new_out,7,&new_length,
        scratch,sizeof(scratch)));
    assert(!musa_dwg1032_object_candidate_read_window_scratch_at_position(
        file,sizeof(file),pages,3,&index,1,2,new_out,7,&new_length,
        scratch,sizeof(scratch)));
    assert(!musa_dwg1032_object_candidate_read_window_scratch_at_position(
        file,sizeof(file),pages,3,&index,1,1,new_out,7,&new_length,
        scratch,6));
    assert(!musa_dwg1032_object_candidate_read_window_scratch_at_position(
        file,sizeof(file),pages,3,&index,1,1,new_out,7,&new_length,
        new_out+1,7));
    assert(!musa_dwg1032_object_candidate_read_window_scratch_at_position(
        file,sizeof(file),pages,3,&index,1,1,new_out,7,&new_length,
        (uint8_t *)(void *)file,12));
    assert(!musa_dwg1032_object_candidate_read_window_scratch_at_position(
        file,sizeof(file),pages,3,&index,1,1,new_out,7,&new_length,
        (uint8_t *)(void *)entries,sizeof(entries)));
    assert(!musa_dwg1032_object_candidate_read_window_scratch_at_position(
        file,sizeof(file),pages,3,&index,1,1,new_out,7,&new_length,
        (uint8_t *)(void *)&new_length,sizeof(new_length)));
    pages[2].file_offset=sizeof(file);
    assert(!musa_dwg1032_object_candidate_read_window_scratch_at_position(
        file,sizeof(file),pages,3,&index,1,1,new_out,7,&new_length,
        scratch,sizeof(scratch)));
    for (size_t i=0;i<sizeof(new_out);++i) assert(new_out[i]==0xa5);
    assert(new_length==77);
}

static void large_viewport_windows(void) {
    enum { N=20000, WINDOW=257 };
    uint8_t *file=malloc(N),*old_out=malloc(WINDOW),
            *new_out=malloc(WINDOW),*scratch=malloc(WINDOW);
    MusaDwg1032FilePage *pages=malloc(N*sizeof(*pages));
    assert(file && old_out && new_out && scratch && pages);
    for (size_t i=0;i<N;++i) {
        file[i]=(uint8_t)(i*37u+(i>>5u));
        pages[i]=(MusaDwg1032FilePage){
            (uint64_t)i,(uint64_t)(N-1u-i),1u};
    }
    MusaDwg1032ObjectLocation entry={77,0,N};
    MusaDwg1032ObjectIndex index={0};
    size_t order[1];
    assert(musa_dwg1032_file_pages_validate_window_directory(
        pages,N,N,N,order,1)==0); /* Insufficient validator workspace. */
    size_t *all_order=malloc(N*sizeof(*all_order));
    assert(all_order);
    assert(musa_dwg1032_file_pages_validate_window_directory(
        pages,N,N,N,all_order,N));
    assert(musa_dwg1032_object_index_init(&index,&entry,1,N));
    assert(musa_dwg1032_object_spans_prepare(&index,order,1));
    for (size_t start=0;start<N;start+=WINDOW) {
        const size_t count=N-start<WINDOW?N-start:WINDOW;
        uint64_t a=0,b=0;
        memset(old_out,0xa5,WINDOW);
        memset(new_out,0xa5,WINDOW);
        assert(musa_dwg1032_object_candidate_read_window(
            file,N,pages,N,&index,77,start,old_out,count,&a));
        assert(musa_dwg1032_object_candidate_read_window_scratch(
            file,N,pages,N,&index,77,start,new_out,count,&b,
            scratch,WINDOW));
        assert(a==N && b==N && !memcmp(old_out,new_out,WINDOW));
    }
    free(all_order);free(pages);free(scratch);
    free(new_out);free(old_out);free(file);
}
int main(void) {
    cross_page_and_rollback();
    large_viewport_windows();
    return 0;
}
