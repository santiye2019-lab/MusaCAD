/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_object_index.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
static void normal(void) {
    const MusaDwg1032ObjectLocation entries[]={
        {1u,30u,5u},{7u,0u,10u},{100u,50u,1u},{UINT64_MAX,51u,49u}
    };
    MusaDwg1032ObjectIndex idx={0};
    assert(musa_dwg1032_object_index_init(&idx,entries,4u,100u));
    size_t at=999u;
    assert(musa_dwg1032_object_index_find(&idx,1u,&at) && at==0u);
    assert(musa_dwg1032_object_index_find(&idx,100u,&at) && at==2u);
    assert(musa_dwg1032_object_index_find(&idx,UINT64_MAX,&at) && at==3u);
    assert(!musa_dwg1032_object_index_find(&idx,0u,&at) && at==3u);
    assert(!musa_dwg1032_object_index_find(&idx,99u,&at) && at==3u);
    const MusaDwg1032Handle plus={93u,10u,1u};
    assert(musa_dwg1032_object_index_find_ref(&idx,&plus,7u,&at) && at==2u);
    const MusaDwg1032Handle missing={1u,10u,1u};
    assert(!musa_dwg1032_object_index_find_ref(&idx,&missing,7u,&at) && at==2u);
    assert(!musa_dwg1032_object_index_find(&idx,7u,(size_t *)&idx));
    assert(!musa_dwg1032_object_index_find(&idx,7u,NULL));
    assert(!musa_dwg1032_object_index_find_ref(&idx,&plus,UINT64_MAX,&at));
}
static void malformed(void) {
    const MusaDwg1032ObjectLocation bad[][2]={
        {{2u,0u,1u},{2u,2u,1u}},
        {{3u,0u,1u},{2u,2u,1u}},
        {{0u,0u,1u},{2u,2u,1u}},
        {{1u,0u,0u},{2u,2u,1u}},
        {{1u,0u,1u},{2u,99u,2u}},
        {{1u,0u,1u},{2u,UINT64_MAX,2u}}
    };
    MusaDwg1032ObjectIndex idx={NULL,88u,77u};
    for (size_t i=0;i<sizeof bad/sizeof bad[0];++i) {
        assert(!musa_dwg1032_object_index_init(&idx,bad[i],2u,100u));
        assert(idx.count==88u && idx.section_bytes==77u);
    }
    assert(!musa_dwg1032_object_index_init(&idx,NULL,1u,100u));
    assert(!musa_dwg1032_object_index_init(&idx,(const MusaDwg1032ObjectLocation *)&idx,1u,100u));
    assert(musa_dwg1032_object_index_init(&idx,NULL,0u,0u));
    size_t pos=123u;
    assert(!musa_dwg1032_object_index_find(&idx,3u,&pos) && pos==123u);
}
static void many(void) {
    MusaDwg1032ObjectLocation entries[2048];
    for (size_t i=0;i<2048u;++i) {
        entries[i].handle=(uint64_t)i*3u+1u;
        entries[i].offset=(uint64_t)i*16u;
        entries[i].length=16u;
    }
    MusaDwg1032ObjectIndex idx;
    assert(musa_dwg1032_object_index_init(&idx,entries,2048u,32768u));
    for (size_t i=0;i<2048u;++i) {
        size_t found=SIZE_MAX;
        assert(musa_dwg1032_object_index_find(&idx,entries[i].handle,&found));
        assert(found==i);
        assert(!musa_dwg1032_object_index_find(&idx,entries[i].handle+1u,&found));
        assert(found==i);
    }
}
int main(void) {normal();malformed();many();return 0;}
