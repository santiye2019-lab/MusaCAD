/* SPDX-License-Identifier: MIT
 * Synthetic post-decode viewport evidence; not a real DWG fixture.
 */
#include "musa_dwg1032_viewport_visibility.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>

static void viewport_contexts(void) {
    const MusaDwg1032LayerState layers[]={
        {10,1,1},{20,1,1},{30,0,1},{40,1,0}
    };
    uint64_t handles[]={10,20,30,40,10,20};
    uint8_t entity[]={1,1,1,1,0,1};
    MusaDwg1032ViewportFreeze vp_a[]={{20,1},{40,0}};
    MusaDwg1032ViewportFreeze vp_b[]={{10,1},{20,0}};
    uint8_t out[6]={9,9,9,9,9,9};
    assert(musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_a,2,out));
    const uint8_t expected_a[]={1,0,0,0,0,0};
    assert(!memcmp(out,expected_a,sizeof(out)));
    assert(musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_b,2,out));
    const uint8_t expected_b[]={0,1,0,0,0,1};
    assert(!memcmp(out,expected_b,sizeof(out)));
    assert(musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,NULL,0,out));
    const uint8_t expected_default[]={1,1,0,0,0,1};
    assert(!memcmp(out,expected_default,sizeof(out)));
    uint8_t saved[6]; memcpy(saved,out,sizeof(saved));
    handles[5]=999;
    assert(!musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_a,2,out));
    assert(!memcmp(out,saved,sizeof(out)));
    handles[5]=20;
    entity[5]=2;
    assert(!musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_a,2,out));
    assert(!memcmp(out,saved,sizeof(out)));
    entity[5]=1;
    vp_a[1].frozen=2;
    assert(!musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_a,2,out));
    assert(!memcmp(out,saved,sizeof(out)));
    vp_a[1].frozen=0;
    vp_a[1].layer_handle=20;
    assert(!musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_a,2,out));
    vp_a[1].layer_handle=40;
    assert(!musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_a,2,(uint8_t *)(void *)handles));
    assert(!musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_a,2,entity));
    assert(!memcmp(out,saved,sizeof(out)));
}

/* A large synthetic pafta must preserve all original layer-state decisions.
 * Compare indexed lookup with a deliberately simple independent reference. */
static void large_viewport_differential(void) {
    enum { N=20000, L=1024, V=128 };
    static uint64_t handles[N];
    static uint8_t entities[N], out[N], saved[N];
    static MusaDwg1032LayerState layers[L];
    static MusaDwg1032ViewportFreeze overrides[V];
    for (size_t j=0u;j<L;++j) {
        layers[j].handle=(uint64_t)j+1u;
        layers[j].on=(uint8_t)(j%17u!=0u);
        layers[j].thawed=(uint8_t)(j%19u!=0u);
    }
    for (size_t j=0u;j<V;++j) {
        overrides[j].layer_handle=(uint64_t)(j*8u+1u);
        overrides[j].frozen=(uint8_t)(j%3u==0u);
    }
    for (size_t i=0u;i<N;++i) {
        handles[i]=(uint64_t)((i*37u)%L)+1u;
        entities[i]=(uint8_t)(i%23u!=0u);
        out[i]=9u;
    }
    assert(musa_dwg1032_viewport_local_visibility(
        handles,entities,N,layers,L,overrides,V,out));
    for (size_t i=0u;i<N;++i) {
        const size_t j=(size_t)handles[i]-1u;
        uint8_t frozen=0u;
        for (size_t k=0u;k<V;++k)
            if (overrides[k].layer_handle==handles[i])
                frozen=overrides[k].frozen;
        const uint8_t expected=(uint8_t)(
            entities[i] && layers[j].on && layers[j].thawed && !frozen);
        assert(out[i]==expected);
    }
    memcpy(saved,out,sizeof(saved));
    handles[N-1u]=UINT64_MAX;
    assert(!musa_dwg1032_viewport_local_visibility(
        handles,entities,N,layers,L,overrides,V,out));
    assert(!memcmp(out,saved,sizeof(out)));
    handles[N-1u]=(uint64_t)(((N-1u)*37u)%L)+1u;
    overrides[V-1u].layer_handle=UINT64_MAX;
    assert(!musa_dwg1032_viewport_local_visibility(
        handles,entities,N,layers,L,overrides,V,out));
    assert(!memcmp(out,saved,sizeof(out)));
    overrides[V-1u].layer_handle=(uint64_t)((V-1u)*8u+1u);
    assert(!musa_dwg1032_viewport_local_visibility(
        handles,entities,N,layers,L,overrides,SIZE_MAX,out));
    assert(!memcmp(out,saved,sizeof(out)));
    assert(musa_dwg1032_viewport_local_visibility(
        handles,entities,N,layers,L,overrides,V,out));
    assert(!memcmp(out,saved,sizeof(out)));
}

int main(void) { viewport_contexts(); large_viewport_differential(); return 0; }
