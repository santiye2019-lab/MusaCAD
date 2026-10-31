/* SPDX-License-Identifier: MIT
 * Synthetic post-decode owner-visibility regression, not a DWG decoder.
 */
#include "musa_dwg1032_owner_graph.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static void forest_and_fail_closed(void) {
    MusaDwg1032ObjectLocation entries[]={
        {10,0,1},{20,1,1},{30,2,1},{40,3,1},{50,4,1},{60,5,1}
    };
    MusaDwg1032ObjectIndex index={0};
    assert(musa_dwg1032_object_index_init(&index,entries,6,6));
    uint64_t owners[]={0,10,20,20,0,50};
    uint8_t local[]={1,1,0,1,0,1};
    uint8_t visible[]={9,9,9,9,9,9};
    MusaDwg1032OwnerWork work[8];
    assert(musa_dwg1032_owner_graph_visibility(
        &index,owners,local,work,8,visible));
    const uint8_t expected[]={1,1,0,1,0,0};
    assert(memcmp(visible,expected,sizeof(expected))==0);

    local[1]=0; /* A hidden parent hides all its children. */
    assert(musa_dwg1032_owner_graph_visibility(
        &index,owners,local,work,8,visible));
    const uint8_t hidden_parent[]={1,0,0,0,0,0};
    assert(memcmp(visible,hidden_parent,sizeof(visible))==0);
    local[1]=1;

    owners[1]=40; /* 20 -> 40 -> 20 cycle. */
    assert(!musa_dwg1032_owner_graph_visibility(
        &index,owners,local,work,8,visible));
    assert(memcmp(visible,hidden_parent,sizeof(visible))==0);
    owners[1]=999; /* Dangling owner. */
    assert(!musa_dwg1032_owner_graph_visibility(
        &index,owners,local,work,8,visible));
    assert(memcmp(visible,hidden_parent,sizeof(visible))==0);
    owners[1]=10;
    owners[4]=50; /* Self-owned block. */
    assert(!musa_dwg1032_owner_graph_visibility(
        &index,owners,local,work,8,visible));
    owners[4]=0;
    local[3]=2; /* Unknown flag is not treated as visible. */
    assert(!musa_dwg1032_owner_graph_visibility(
        &index,owners,local,work,8,visible));
    local[3]=1;
    assert(memcmp(visible,hidden_parent,sizeof(visible))==0);

    assert(!musa_dwg1032_owner_graph_visibility(
        &index,owners,local,work,5,visible));
    assert(!musa_dwg1032_owner_graph_visibility(
        &index,owners,local,work,8,(uint8_t *)(void *)owners));
    assert(!musa_dwg1032_owner_graph_visibility(
        &index,owners,local,work,8,(uint8_t *)(void *)&work[6]));
    assert(!musa_dwg1032_owner_graph_visibility(
        &index,owners,local,work,SIZE_MAX,visible));
    assert(memcmp(visible,hidden_parent,sizeof(visible))==0);

    entries[2].handle=20; /* Caller mutated a validated index. */
    assert(!musa_dwg1032_owner_graph_visibility(
        &index,owners,local,work,8,visible));
    assert(memcmp(visible,hidden_parent,sizeof(visible))==0);
    entries[2].handle=30;
    assert(musa_dwg1032_owner_graph_visibility(
        &index,owners,local,work,8,visible));
    assert(memcmp(visible,expected,sizeof(visible))==0);
}

static void large_inherited_visibility(void) {
    enum {N=20000};
    static MusaDwg1032ObjectLocation entries[N];
    static uint64_t owners[N];
    static uint8_t local[N],visible[N];
    static MusaDwg1032OwnerWork work[N];
    for (size_t i=0;i<N;++i) {
        entries[i]=(MusaDwg1032ObjectLocation){(uint64_t)i+1u,(uint64_t)i,1u};
        owners[i]=i?(uint64_t)i:0u;
        local[i]=1;
        visible[i]=7;
    }
    local[N/2]=0;
    MusaDwg1032ObjectIndex index={0};
    assert(musa_dwg1032_object_index_init(&index,entries,N,N));
    assert(musa_dwg1032_owner_graph_visibility(
        &index,owners,local,work,N,visible));
    for (size_t i=0;i<N;++i)
        assert(visible[i]==(uint8_t)(i<N/2));
    owners[0]=N; /* Cycle reaches last object. */
    assert(!musa_dwg1032_owner_graph_visibility(
        &index,owners,local,work,N,visible));
    for (size_t i=0;i<N;++i)
        assert(visible[i]==(uint8_t)(i<N/2));
}
int main(void) {
    forest_and_fail_closed();
    large_inherited_visibility();
    return 0;
}
