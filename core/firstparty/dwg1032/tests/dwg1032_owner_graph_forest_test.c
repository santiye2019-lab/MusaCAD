/* SPDX-License-Identifier: MIT
 * Deterministic branching forest, checked against an independent slow walk.
 */
#include "musa_dwg1032_owner_graph.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

int main(void) {
    enum { N=12000 };
    static MusaDwg1032ObjectLocation entries[N];
    static uint64_t owners[N], roots[N];
    static size_t depths[N];
    static MusaDwg1032OwnerWork work[N];
    uint32_t random_state=0x1a2b3c4du;
    for (size_t i=0;i<N;++i) {
        entries[i]=(MusaDwg1032ObjectLocation){(uint64_t)i*2u+1u,(uint64_t)i,1u};
        random_state=random_state*1664525u+1013904223u;
        owners[i]=(i==0 || i%79u==0) ? 0u :
            entries[(size_t)(random_state%(uint32_t)i)].handle;
    }
    MusaDwg1032ObjectIndex index;
    assert(musa_dwg1032_object_index_init(&index,entries,N,N));
    assert(musa_dwg1032_owner_graph_resolve(&index,owners,work,N,roots,depths));
    for (size_t i=0;i<N;++i) {
        size_t pos=i, slow_depth=0;
        while (owners[pos]) {
            size_t next=SIZE_MAX;
            assert(musa_dwg1032_object_index_find(&index,owners[pos],&next));
            assert(next<pos); /* Every generated parent precedes its child. */
            pos=next;
            ++slow_depth;
        }
        assert(roots[i]==entries[pos].handle);
        assert(depths[i]==slow_depth);
    }
    /* Corrupt a late node: failure must not publish partial output. */
    uint64_t saved_roots[N];
    size_t saved_depths[N];
    memcpy(saved_roots,roots,sizeof roots);
    memcpy(saved_depths,depths,sizeof depths);
    owners[N-1]=entries[N-1].handle;
    assert(!musa_dwg1032_owner_graph_resolve(&index,owners,work,N,roots,depths));
    assert(memcmp(roots,saved_roots,sizeof roots)==0);
    assert(memcmp(depths,saved_depths,sizeof depths)==0);
    return 0;
}
