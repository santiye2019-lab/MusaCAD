/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_object_spans.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
static void adjacent_and_extreme(void) {
    MusaDwg1032ObjectLocation entries[]={
        {1u,UINT64_MAX-4u,2u},
        {2u,0u,1u},
        {3u,UINT64_MAX-2u,2u}
    };
    MusaDwg1032ObjectIndex index={entries,3u,UINT64_MAX};
    size_t order[3]={99u,99u,99u};
    assert(musa_dwg1032_object_spans_prepare(&index,order,3u));
    assert(order[0]==1u && order[1]==0u && order[2]==2u);
    size_t pos=88u;
    assert(musa_dwg1032_object_spans_find(&index,order,UINT64_MAX-1u,&pos) && pos==2u);
    assert(!musa_dwg1032_object_spans_find(&index,order,UINT64_MAX,&pos));
    assert(!musa_dwg1032_object_spans_find(&index,order,1u,&pos));
    assert(pos==2u);
    entries[2].offset=UINT64_MAX-3u;
    assert(!musa_dwg1032_object_spans_prepare(&index,order,3u));
    entries[2].offset=UINT64_MAX-2u;
    entries[2].length=3u;
    assert(!musa_dwg1032_object_spans_prepare(&index,order,3u));
}
static void section_guard(void) {
    uint8_t bytes[8]={0};
    MusaDwg1032ObjectLocation entries[]={{1u,0u,4u},{2u,4u,4u}};
    MusaDwg1032ObjectIndex index={entries,2u,8u};
    MusaDwg1032Cursor section={bytes,8u,0u};
    MusaDwg1032Cursor out={NULL,55u,77u};
    assert(musa_dwg1032_object_span_cursor(&index,&section,1u,&out));
    assert(out.bytes==bytes+4u && out.byte_count==4u && out.bit_position==0u);
    out=(MusaDwg1032Cursor){NULL,55u,77u};
    section.byte_count=7u;
    assert(!musa_dwg1032_object_span_cursor(&index,&section,1u,&out));
    assert(out.bytes==NULL && out.byte_count==55u && out.bit_position==77u);
    section.byte_count=8u;
    entries[1].length=5u;
    assert(!musa_dwg1032_object_span_cursor(&index,&section,1u,&out));
    entries[1].length=4u;
    assert(!musa_dwg1032_object_span_cursor(&index,&section,1u,(MusaDwg1032Cursor *)bytes));
}
static void deterministic_stress(void) {
    enum { N=4096 };
    static MusaDwg1032ObjectLocation entries[N];
    static size_t order[N];
    for (size_t i=0;i<N;++i)
        entries[i]=(MusaDwg1032ObjectLocation){(uint64_t)i+1u,
            (uint64_t)(N-1u-i)*5u,5u};
    MusaDwg1032ObjectIndex index={entries,N,N*5u};
    assert(musa_dwg1032_object_spans_prepare(&index,order,N));
    for (size_t i=0;i<N;++i) {
        size_t at=N;
        assert(musa_dwg1032_object_spans_find(&index,order,(uint64_t)i*5u,&at));
        assert(at==N-1u-i);
        assert(musa_dwg1032_object_spans_find(&index,order,(uint64_t)i*5u+4u,&at));
        assert(at==N-1u-i);
    }
    entries[N/2u].length=6u;
    assert(!musa_dwg1032_object_spans_prepare(&index,order,N));
}
int main(void) {adjacent_and_extreme();section_guard();deterministic_stress();return 0;}
