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
int main(void) {branching_and_rollback();large_chain();return 0;}
