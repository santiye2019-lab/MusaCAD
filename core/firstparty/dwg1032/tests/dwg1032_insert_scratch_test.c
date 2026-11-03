/* SPDX-License-Identifier: MIT
 * Independent synthetic differential test, NOT a real DWG acceptance test.
 */
#include "musa_dwg1032_insert_scratch.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>

enum { N=20000, L=1024, V=128 };
static uint64_t handles[N];
static uint8_t owners[N], expected[N], actual[N], scratch[N], saved[N];
static MusaDwg1032LayerState layers[L];
static MusaDwg1032ViewportFreeze freeze[V];

static void prepare(void) {
    for (size_t i=0;i<L;++i)
        layers[i]=(MusaDwg1032LayerState){
            (uint64_t)i+1u,(uint8_t)(i%17u!=0u),
            (uint8_t)(i%19u!=0u)};
    for (size_t i=0;i<V;++i)
        freeze[i]=(MusaDwg1032ViewportFreeze){
            (uint64_t)(i*8u+1u),(uint8_t)(i%3u==0u)};
    for (size_t i=0;i<N;++i) {
        handles[i]=(uint64_t)((i*37u)%L)+1u;
        owners[i]=(uint8_t)(i%11u!=0u);
    }
}
static void differential(void) {
    for (uint8_t insert=0;insert<=1u;++insert) {
        for (size_t j=0;j<V;++j)
            freeze[j].frozen=(uint8_t)((j+insert)%3u==0u);
        memset(actual,0xa5,sizeof(actual));
        assert(musa_dwg1032_viewport_insert_visibility(
            handles,owners,N,1u,insert,layers,L,freeze,V,expected));
        assert(musa_dwg1032_insert_visibility_scratch(
            handles,owners,N,1u,insert,layers,L,freeze,V,
            actual,scratch,N));
        assert(!memcmp(expected,actual,N));
        for (size_t i=0;i<N;++i) {
            const uint64_t h=handles[i];
            uint8_t reference=(uint8_t)(insert && owners[i]);
            if (h!=1u) {
                const size_t layer=(size_t)(h-1u);
                uint8_t frozen=0u;
                for (size_t j=0;j<V;++j)
                    if (freeze[j].layer_handle==h)
                        frozen=freeze[j].frozen;
                reference=(uint8_t)(reference && layers[layer].on &&
                                    layers[layer].thawed && !frozen);
            }
            assert(actual[i]==reference);
        }
    }
}
static void fail_closed(void) {
    memcpy(saved,actual,N);
    handles[N-1]=UINT64_MAX;
    assert(!musa_dwg1032_insert_visibility_scratch(
        handles,owners,N,1u,1u,layers,L,freeze,V,actual,scratch,N));
    assert(!memcmp(actual,saved,N));
    handles[N-1]=(uint64_t)(((N-1u)*37u)%L)+1u;
    owners[N-1]=2u;
    assert(!musa_dwg1032_insert_visibility_scratch(
        handles,owners,N,1u,1u,layers,L,freeze,V,actual,scratch,N));
    assert(!memcmp(actual,saved,N));
    owners[N-1]=1u;
    freeze[V-1].layer_handle=UINT64_MAX;
    assert(!musa_dwg1032_insert_visibility_scratch(
        handles,owners,N,1u,1u,layers,L,freeze,V,actual,scratch,N));
    assert(!memcmp(actual,saved,N));
    freeze[V-1].layer_handle=(uint64_t)((V-1u)*8u+1u);
    assert(!musa_dwg1032_insert_visibility_scratch(
        handles,owners,N,1u,1u,layers,L,freeze,V,actual,scratch,N-1u));
    assert(!memcmp(actual,saved,N));
    assert(!musa_dwg1032_insert_visibility_scratch(
        handles,owners,N,1u,1u,layers,L,freeze,V,owners,scratch,N));
    assert(!musa_dwg1032_insert_visibility_scratch(
        handles,owners,N,1u,1u,layers,L,freeze,V,
        (uint8_t *)(void *)handles,scratch,N));
    assert(!musa_dwg1032_insert_visibility_scratch(
        handles,owners,N,1u,1u,layers,L,freeze,V,actual,actual,N));
    assert(!memcmp(actual,saved,N));
    /* Disjoint used scratch bytes are insufficient: the unused capacity
     * also must not alias a future published output byte. */
    static uint8_t arena[2u*N+2u];
    assert(!musa_dwg1032_insert_visibility_scratch(
        handles,owners,N,1u,1u,layers,L,freeze,V,
        arena+N+1u,arena,N+2u));
    assert(musa_dwg1032_insert_visibility_scratch(
        NULL,NULL,0u,1u,1u,layers,L,freeze,V,NULL,NULL,0u));
    assert(!musa_dwg1032_insert_visibility_scratch(
        NULL,NULL,0u,UINT64_MAX,1u,layers,L,freeze,V,NULL,NULL,0u));
}

/* Differential proof across all 20,000 children, but only 257 scratch
 * bytes: no full-size temporary visibility array is needed. */
static uint8_t paged[N], page_scratch[257];
static void paged_differential_and_fail_closed(void) {
    assert(musa_dwg1032_viewport_insert_visibility(
        handles,owners,N,1u,1u,layers,L,freeze,V,expected));
    memset(paged,0xa5,sizeof(paged));
    for (size_t first=0u;first<N;first+=sizeof(page_scratch)) {
        const size_t remaining=N-first;
        const size_t count=remaining<sizeof(page_scratch)
            ? remaining : sizeof(page_scratch);
        assert(musa_dwg1032_insert_visibility_page_scratch(
            handles,owners,N,first,count,1u,1u,layers,L,freeze,V,
            paged+first,page_scratch,sizeof(page_scratch)));
        assert(!memcmp(paged+first,expected+first,count));
    }
    assert(!memcmp(paged,expected,N));
    const size_t first=N-sizeof(page_scratch);
    uint8_t guarded[sizeof(page_scratch)];
    memset(guarded,0x6c,sizeof(guarded));
    handles[N-1u]=UINT64_MAX;
    assert(!musa_dwg1032_insert_visibility_page_scratch(
        handles,owners,N,first,sizeof(guarded),1u,1u,layers,L,freeze,V,
        guarded,page_scratch,sizeof(page_scratch)));
    for (size_t i=0u;i<sizeof(guarded);++i)
        assert(guarded[i]==0x6c);
    assert(!memcmp(paged,expected,N));
    handles[N-1u]=(uint64_t)(((N-1u)*37u)%L)+1u;

    /* Future source pages must never be used as writable output/scratch. */
    assert(!musa_dwg1032_insert_visibility_page_scratch(
        handles,owners,N,0u,1u,1u,1u,layers,L,freeze,V,
        (uint8_t *)(void *)&handles[N-1u],page_scratch,1u));
    assert(!musa_dwg1032_insert_visibility_page_scratch(
        handles,owners,N,0u,1u,1u,1u,layers,L,freeze,V,
        guarded,&owners[N-1u],1u));
    assert(!musa_dwg1032_insert_visibility_page_scratch(
        handles,owners,N,0u,2u,1u,1u,layers,L,freeze,V,
        guarded,page_scratch,1u));
    assert(!musa_dwg1032_insert_visibility_page_scratch(
        handles,owners,N,N,1u,1u,1u,layers,L,freeze,V,
        guarded,page_scratch,sizeof(page_scratch)));
    assert(!musa_dwg1032_insert_visibility_page_scratch(
        handles,owners,N,0u,1u,1u,1u,layers,L,freeze,V,
        page_scratch,page_scratch,sizeof(page_scratch)));
    for (size_t i=0u;i<sizeof(guarded);++i)
        assert(guarded[i]==0x6c);
    assert(!memcmp(paged,expected,N));
}

int main(void) {
    prepare();
    differential();
    paged_differential_and_fail_closed();
    fail_closed();
    return 0;
}
