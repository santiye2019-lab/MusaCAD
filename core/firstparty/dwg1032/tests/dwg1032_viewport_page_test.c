/* SPDX-License-Identifier: MIT
 * First-party synthetic 20k-entity viewport paging regressions.
 * Not evidence of actual AC1032 compressed DWG decoding.
 */
#include "musa_dwg1032_viewport_visibility.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

enum { N=20000, L=128, V=16 };
static uint64_t handles[N];
static uint8_t flags[N], expected[N], actual[N];
static MusaDwg1032LayerState layers[L];
static MusaDwg1032ViewportFreeze overrides[V];

static void setup(void) {
    for (size_t i=0;i<L;++i)
        layers[i]=(MusaDwg1032LayerState){
            (uint64_t)i+1u,(uint8_t)(i%11u!=0u),
            (uint8_t)(i%17u!=0u)};
    for (size_t i=0;i<V;++i)
        overrides[i]=(MusaDwg1032ViewportFreeze){
            (uint64_t)(i*8u+1u),(uint8_t)(i%3u==0u)};
    for (size_t i=0;i<N;++i) {
        handles[i]=(uint64_t)((i*53u)%L)+1u;
        flags[i]=(uint8_t)(i%19u!=0u);
    }
}
static void differential(void) {
    assert(musa_dwg1032_viewport_local_visibility(
        handles,flags,N,layers,L,overrides,V,expected));
    const size_t sizes[]={1u,7u,257u,1024u,N};
    for (size_t k=0;k<sizeof(sizes)/sizeof(sizes[0]);++k) {
        memset(actual,0xa5,sizeof(actual));
        for (size_t first=0;first<N;) {
            const size_t remaining=N-first;
            const size_t count=sizes[k]<remaining?sizes[k]:remaining;
            assert(musa_dwg1032_viewport_local_visibility_page(
                handles,flags,N,first,count,layers,L,overrides,V,
                actual+first));
            first+=count;
        }
        assert(memcmp(actual,expected,N)==0);
    }
    /* A new viewport must recompute the page instead of reusing a mask. */
    for (size_t i=0;i<V;++i)
        overrides[i].frozen=(uint8_t)(i%5u==0u);
    assert(musa_dwg1032_viewport_local_visibility(
        handles,flags,N,layers,L,overrides,V,expected));
    for (size_t first=0;first<N;first+=257u) {
        const size_t count=N-first<257u?N-first:257u;
        assert(musa_dwg1032_viewport_local_visibility_page(
            handles,flags,N,first,count,layers,L,overrides,V,
            actual+first));
    }
    assert(memcmp(actual,expected,N)==0);
}
static void atomic_rejections(void) {
    uint8_t page[257];
    memset(page,0xa5,sizeof(page));
    handles[N-1u]=UINT64_MAX;
    assert(!musa_dwg1032_viewport_local_visibility_page(
        handles,flags,N,N-257u,257u,layers,L,overrides,V,page));
    handles[N-1u]=(uint64_t)(((N-1u)*53u)%L)+1u;
    flags[N-1u]=2u;
    assert(!musa_dwg1032_viewport_local_visibility_page(
        handles,flags,N,N-257u,257u,layers,L,overrides,V,page));
    flags[N-1u]=(uint8_t)((N-1u)%19u!=0u);
    overrides[V-1u].frozen=2u;
    assert(!musa_dwg1032_viewport_local_visibility_page(
        handles,flags,N,0u,257u,layers,L,overrides,V,page));
    overrides[V-1u].frozen=(uint8_t)((V-1u)%5u==0u);
    layers[L-1u].handle=layers[L-2u].handle;
    assert(!musa_dwg1032_viewport_local_visibility_page(
        handles,flags,N,0u,257u,layers,L,overrides,V,page));
    layers[L-1u].handle=L;
    assert(!musa_dwg1032_viewport_local_visibility_page(
        handles,flags,N,N,1u,layers,L,overrides,V,page));
    assert(!musa_dwg1032_viewport_local_visibility_page(
        handles,flags,N,SIZE_MAX,1u,layers,L,overrides,V,page));
    assert(!musa_dwg1032_viewport_local_visibility_page(
        handles,flags,SIZE_MAX,0u,257u,layers,L,overrides,V,page));
    assert(!musa_dwg1032_viewport_local_visibility_page(
        handles,flags,N,0u,0u,layers,L,overrides,V,page));
    /* Reject alias with a DIFFERENT source page, not only this page. */
    assert(!musa_dwg1032_viewport_local_visibility_page(
        handles,flags,N,0u,257u,layers,L,overrides,V,flags+1000u));
    assert(!musa_dwg1032_viewport_local_visibility_page(
        handles,flags,N,0u,257u,layers,L,overrides,V,
        (uint8_t *)(void *)(handles+1000u)));
    for (size_t i=0;i<sizeof(page);++i) assert(page[i]==0xa5u);
    assert(musa_dwg1032_viewport_local_visibility_page(
        handles,flags,N,N-257u,257u,layers,L,overrides,V,page));
    assert(memcmp(page,expected+N-257u,257u)==0);
}
int main(void) {
    setup();
    differential();
    atomic_rejections();
    return 0;
}
