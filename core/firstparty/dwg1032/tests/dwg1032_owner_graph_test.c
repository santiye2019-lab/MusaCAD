/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_owner_graph.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static void branching_and_rollback(void) {
    const MusaDwg1032ObjectLocation entries[]={
        {10,0,1},{20,1,1},{30,2,1},{40,3,1},{50,4,1},{60,5,1}
    };
    MusaDwg1032ObjectIndex index;
    assert(musa_dwg1032_object_index_init(&index,entries,6,6));
    uint64_t owners[]={0,10,20,20,0,50};
    MusaDwg1032OwnerWork work[6];
    uint64_t roots[6]={0};
    size_t depths[6]={0};
    assert(musa_dwg1032_owner_graph_resolve(&index,owners,work,6,roots,depths));
    const uint64_t expected_roots[]={10,10,10,10,50,50};
    const size_t expected_depths[]={0,1,2,2,0,1};
    assert(memcmp(roots,expected_roots,sizeof roots)==0);
    assert(memcmp(depths,expected_depths,sizeof depths)==0);
    uint64_t saved_roots[6];
    size_t saved_depths[6];
    memcpy(saved_roots,roots,sizeof roots);
    memcpy(saved_depths,depths,sizeof depths);
    owners[1]=40; /* 20 -> 40 -> 20 */
    assert(!musa_dwg1032_owner_graph_resolve(&index,owners,work,6,roots,depths));
    assert(memcmp(roots,saved_roots,sizeof roots)==0);
    assert(memcmp(depths,saved_depths,sizeof depths)==0);
    owners[1]=999; /* Dangling owner. */
    assert(!musa_dwg1032_owner_graph_resolve(&index,owners,work,6,roots,depths));
    assert(memcmp(roots,saved_roots,sizeof roots)==0);
    owners[1]=10;
    owners[4]=50; /* Self cycle. */
    assert(!musa_dwg1032_owner_graph_resolve(&index,owners,work,6,roots,depths));
    owners[4]=0;
    assert(!musa_dwg1032_owner_graph_resolve(&index,owners,work,5,roots,depths));
    assert(!musa_dwg1032_owner_graph_resolve(&index,owners,work,6,
        (uint64_t *)(void *)owners,depths));
    assert(!musa_dwg1032_owner_graph_resolve(&index,owners,work,6,roots,
        (size_t *)(void *)roots));
    assert(memcmp(roots,saved_roots,sizeof roots)==0);
    assert(memcmp(depths,saved_depths,sizeof depths)==0);
}

/* Caller-owned index entries may be modified after object_index_init.
 * Never resolve a stale, malformed index or publish partial results. */
static void mutated_index_is_rejected(void) {
    MusaDwg1032ObjectLocation entries[]={
        {10,0,1},{20,1,1},{30,2,1}
    };
    MusaDwg1032ObjectIndex index;
    assert(musa_dwg1032_object_index_init(&index,entries,3,3));
    const uint64_t owners[]={0,10,20};
    MusaDwg1032OwnerWork work[3];
    uint64_t roots[]={91,92,93};
    size_t depths[]={91,92,93};
    assert(musa_dwg1032_owner_graph_resolve(
        &index,owners,work,3,roots,depths));
    const uint64_t expected_roots[]={10,10,10};
    const size_t expected_depths[]={0,1,2};
    assert(memcmp(roots,expected_roots,sizeof roots)==0);
    assert(memcmp(depths,expected_depths,sizeof depths)==0);
    const MusaDwg1032ObjectLocation original=entries[1];
    const MusaDwg1032ObjectLocation malformed[]={
        {10,1,1}, /* Duplicate handle. */
        {0,1,1},  /* Zero handle. */
        {9,1,1},  /* Unsorted handle. */
        {20,3,1}, /* Span exceeds section. */
        {20,1,0}, /* Empty span. */
        {20,2,2}  /* Span extends past section end. */
    };
    for (size_t i=0;i<sizeof(malformed)/sizeof(malformed[0]);++i) {
        entries[1]=malformed[i];
        assert(!musa_dwg1032_owner_graph_resolve(
            &index,owners,work,3,roots,depths));
        assert(memcmp(roots,expected_roots,sizeof roots)==0);
        assert(memcmp(depths,expected_depths,sizeof depths)==0);
    }
    entries[1]=original;
    assert(musa_dwg1032_owner_graph_resolve(
        &index,owners,work,3,roots,depths));
    assert(memcmp(roots,expected_roots,sizeof roots)==0);
    assert(memcmp(depths,expected_depths,sizeof depths)==0);
}

static void large_chain(void) {
    enum {N=20000};
    static MusaDwg1032ObjectLocation entries[N];
    static uint64_t owners[N], roots[N];
    static size_t depths[N];
    static MusaDwg1032OwnerWork work[N];
    for (size_t i=0;i<N;++i) {
        entries[i]=(MusaDwg1032ObjectLocation){(uint64_t)i+1,(uint64_t)i,1};
        owners[i]=i?(uint64_t)i:0;
    }
    MusaDwg1032ObjectIndex index;
    assert(musa_dwg1032_object_index_init(&index,entries,N,N));
    assert(musa_dwg1032_owner_graph_resolve(&index,owners,work,N,roots,depths));
    for (size_t i=0;i<N;++i) assert(roots[i]==1 && depths[i]==i);
    owners[0]=N;
    assert(!musa_dwg1032_owner_graph_resolve(&index,owners,work,N,roots,depths));
    assert(roots[N-1]==1 && depths[N-1]==N-1);
}
int main(void) {branching_and_rollback();mutated_index_is_rejected();large_chain();return 0;}
