/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_section_window_scratch.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void small_windows(void) {
    const uint8_t file[]={7,8,9,1,2,3,4,5,6};
    MusaDwg1032FilePage pages[]={{0,3,3},{3,6,3},{6,0,3}};
    uint8_t out[8],scratch[8];
    const uint8_t expected[]={3,4,5,6,7};
    memset(out,0xA5,sizeof(out));
    assert(musa_dwg1032_section_read_window_scratch(
        file,sizeof(file),pages,3,9,2,out,5,scratch,sizeof(scratch)));
    assert(!memcmp(out,expected,sizeof(expected)));
    for (size_t i=5;i<sizeof(out);++i) assert(out[i]==0xA5);
    memset(out,0xA5,sizeof(out));
    assert(musa_dwg1032_section_read_window_scratch(
        file,sizeof(file),pages,3,9,6,out,3,scratch,sizeof(scratch)));
    assert(out[0]==7 && out[1]==8 && out[2]==9 && out[3]==0xA5);
    memset(out,0xA5,sizeof(out));
    assert(musa_dwg1032_section_read_window_scratch(
        file,sizeof(file),pages,3,9,9,NULL,0,NULL,0));
    assert(!musa_dwg1032_section_read_window_scratch(
        file,sizeof(file),pages,3,9,8,out,2,scratch,sizeof(scratch)));
    assert(!musa_dwg1032_section_read_window_scratch(
        file,sizeof(file),pages,3,9,2,out,5,scratch,4));
    assert(!musa_dwg1032_section_read_window_scratch(
        file,sizeof(file),pages,3,9,2,out,5,out+5,3));
    assert(!musa_dwg1032_section_read_window_scratch(
        file,sizeof(file),pages,3,9,2,out,5,(uint8_t *)(void *)file,8));
    assert(!musa_dwg1032_section_read_window_scratch(
        file,sizeof(file),pages,3,9,2,out,5,(uint8_t *)(void *)pages,8));
    for (size_t i=0;i<sizeof(out);++i) assert(out[i]==0xA5);
    pages[2].file_offset=8; /* Late physically truncated page. */
    assert(!musa_dwg1032_section_read_window_scratch(
        file,sizeof(file),pages,3,9,2,out,5,scratch,sizeof(scratch)));
    for (size_t i=0;i<sizeof(out);++i) assert(out[i]==0xA5);
    pages[2].file_offset=0;
    pages[2].logical_offset=5; /* Late overlapping logical page. */
    assert(!musa_dwg1032_section_read_window_scratch(
        file,sizeof(file),pages,3,9,2,out,5,scratch,sizeof(scratch)));
    for (size_t i=0;i<sizeof(out);++i) assert(out[i]==0xA5);
    pages[2].logical_offset=6;
    pages[0].byte_count=2;
    pages[1].logical_offset=4; /* Gap before the first touched page. */
    assert(!musa_dwg1032_section_read_window_scratch(
        file,sizeof(file),pages,3,9,4,out,2,scratch,sizeof(scratch)));
    for (size_t i=0;i<sizeof(out);++i) assert(out[i]==0xA5);
}

static void large_reversed_pages(void) {
    enum { PAGE_COUNT=20000, WINDOW=257 };
    uint8_t *file=malloc(PAGE_COUNT), *out=malloc(WINDOW);
    uint8_t *scratch=malloc(WINDOW);
    MusaDwg1032FilePage *pages=malloc(PAGE_COUNT*sizeof(*pages));
    assert(file && out && scratch && pages);
    for (size_t i=0;i<PAGE_COUNT;++i) {
        file[i]=(uint8_t)((i*29u)^(i>>4u));
        pages[i]=(MusaDwg1032FilePage){
            (uint64_t)i,(uint64_t)(PAGE_COUNT-1u-i),1u};
    }
    for (size_t start=0;start<PAGE_COUNT;start+=WINDOW) {
        const size_t length=PAGE_COUNT-start<WINDOW?PAGE_COUNT-start:WINDOW;
        memset(out,0xA5,WINDOW);
        assert(musa_dwg1032_section_read_window_scratch(
            file,PAGE_COUNT,pages,PAGE_COUNT,PAGE_COUNT,start,
            out,length,scratch,WINDOW));
        for (size_t j=0;j<length;++j)
            assert(out[j]==file[PAGE_COUNT-1u-(start+j)]);
        for (size_t j=length;j<WINDOW;++j) assert(out[j]==0xA5);
    }
    memset(out,0xA5,WINDOW);
    pages[PAGE_COUNT-1u].file_offset=PAGE_COUNT;
    assert(!musa_dwg1032_section_read_window_scratch(
        file,PAGE_COUNT,pages,PAGE_COUNT,PAGE_COUNT,PAGE_COUNT-WINDOW,
        out,WINDOW,scratch,WINDOW));
    for (size_t j=0;j<WINDOW;++j) assert(out[j]==0xA5);
    free(pages);free(scratch);free(out);free(file);
}
int main(void) { small_windows();large_reversed_pages();return 0; }
