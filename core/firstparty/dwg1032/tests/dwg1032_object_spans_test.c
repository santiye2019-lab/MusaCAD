/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_object_spans.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
static void reordered_and_gaps(void) {
    const MusaDwg1032ObjectLocation entries[]={
        {2u,8u,2u},{4u,0u,3u},{8u,5u,3u},{16u,10u,1u}
    };
    const MusaDwg1032ObjectIndex index={entries,4u,12u};
    size_t order[4]={99u,99u,99u,99u};
    assert(musa_dwg1032_object_spans_prepare(&index,order,4u));
    assert(order[0]==1u && order[1]==2u && order[2]==0u && order[3]==3u);
    size_t at=77u;
    assert(musa_dwg1032_object_spans_find(&index,order,0u,&at) && at==1u);
    assert(musa_dwg1032_object_spans_find(&index,order,7u,&at) && at==2u);
    assert(musa_dwg1032_object_spans_find(&index,order,8u,&at) && at==0u);
    assert(musa_dwg1032_object_spans_find(&index,order,10u,&at) && at==3u);
    at=77u;
    assert(!musa_dwg1032_object_spans_find(&index,order,3u,&at) && at==77u);
    assert(!musa_dwg1032_object_spans_find(&index,order,11u,&at) && at==77u);
    assert(!musa_dwg1032_object_spans_find(&index,order,12u,&at) && at==77u);
    assert(!musa_dwg1032_object_spans_prepare(&index,order,3u));
    assert(!musa_dwg1032_object_spans_prepare(&index,(size_t *)entries,4u));
    uint8_t bytes[12]={0};
    MusaDwg1032Cursor section={bytes,12u,0u};
    MusaDwg1032Cursor child={NULL,99u,99u};
    assert(musa_dwg1032_object_span_cursor(&index,&section,2u,&child));
    assert(child.bytes==bytes+5u && child.byte_count==3u && !child.bit_position);
    child=(MusaDwg1032Cursor){NULL,99u,99u};
    section.bit_position=1u;
    assert(!musa_dwg1032_object_span_cursor(&index,&section,2u,&child));
    assert(child.bytes==NULL && child.byte_count==99u);
    section.bit_position=0u;
    assert(!musa_dwg1032_object_span_cursor(&index,&section,4u,&child));
    assert(!musa_dwg1032_object_span_cursor(&index,&section,2u,&section));
    assert(!musa_dwg1032_object_span_cursor(&index,&section,2u,
                                             (MusaDwg1032Cursor *)bytes));
}
static void invalid_ranges(void) {
    MusaDwg1032ObjectLocation entries[]={
        {1u,0u,2u},{2u,3u,2u},{3u,5u,1u}
    };
    MusaDwg1032ObjectIndex index={entries,3u,6u};
    size_t order[3]={9u,9u,9u};
    assert(musa_dwg1032_object_spans_prepare(&index,order,3u));
    entries[1].offset=1u;
    assert(!musa_dwg1032_object_spans_prepare(&index,order,3u));
    entries[1].offset=3u;
    entries[2].length=2u;
    assert(!musa_dwg1032_object_spans_prepare(&index,order,3u));
    entries[2].length=1u;
    entries[1].handle=1u;
    assert(!musa_dwg1032_object_spans_prepare(&index,order,3u));
    entries[1].handle=2u;
    entries[1].length=0u;
    assert(!musa_dwg1032_object_spans_prepare(&index,order,3u));
    entries[1].length=2u;
    index.count=0u;
    assert(musa_dwg1032_object_spans_prepare(&index,NULL,0u));
}
static void large_reverse_order(void) {
    enum { N=20000 };
    static MusaDwg1032ObjectLocation entries[N];
    static size_t order[N];
    for (size_t i=0;i<N;++i)
        entries[i]=(MusaDwg1032ObjectLocation){(uint64_t)i+1u,
            (uint64_t)(N-1u-i)*3u,2u};
    const MusaDwg1032ObjectIndex index={entries,N,N*3u};
    assert(musa_dwg1032_object_spans_prepare(&index,order,N));
    for (size_t i=0;i<N;++i) assert(order[i]==N-1u-i);
    size_t at=0;
    assert(musa_dwg1032_object_spans_find(&index,order,0u,&at) && at==N-1u);
    assert(!musa_dwg1032_object_spans_find(&index,order,2u,&at));
    assert(musa_dwg1032_object_spans_find(&index,order,N*3u-2u,&at) && at==0u);
}
int main(void) {reordered_and_gaps();invalid_ranges();large_reverse_order();return 0;}
