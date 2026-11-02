/* SPDX-License-Identifier: MIT
 * Synthetic 20k-entity validated viewport plan regressions.
 * No real AC1032 page decompression or entity decoding is exercised.
 */
#include "musa_dwg1032_viewport_plan.h"
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
    for (size_t i=0u;i<L;++i)
        layers[i]=(MusaDwg1032LayerState){
            (uint64_t)i+1u,(uint8_t)(i%11u!=0u),
            (uint8_t)(i%17u!=0u)};
    for (size_t i=0u;i<V;++i)
        overrides[i]=(MusaDwg1032ViewportFreeze){
            (uint64_t)(i*8u+1u),(uint8_t)(i%3u==0u)};
    for (size_t i=0u;i<N;++i) {
        handles[i]=(uint64_t)((i*53u)%L)+1u;
        flags[i]=(uint8_t)(i%19u!=0u);
    }
}
static void compare_pages(void) {
    MusaDwg1032ViewportPlan plan;
    assert(musa_dwg1032_viewport_plan_init(
        &plan,layers,L,overrides,V));
    assert(musa_dwg1032_viewport_local_visibility(
        handles,flags,N,layers,L,overrides,V,expected));
    const size_t sizes[]={1u,7u,257u,1024u,N};
    for (size_t k=0u;k<sizeof(sizes)/sizeof(sizes[0]);++k) {
        memset(actual,0xa5,sizeof(actual));
        for (size_t first=0u;first<N;) {
            const size_t remaining=N-first;
            const size_t count=sizes[k]<remaining?sizes[k]:remaining;
            assert(musa_dwg1032_viewport_plan_page(
                &plan,handles,flags,N,first,count,actual+first));
            first+=count;
        }
        assert(memcmp(actual,expected,N)==0);
    }
    /* A viewport change requires a new plan, never a stale mask. */
    for (size_t i=0u;i<V;++i)
        overrides[i].frozen=(uint8_t)(i%5u==0u);
    assert(musa_dwg1032_viewport_plan_init(
        &plan,layers,L,overrides,V));
    assert(musa_dwg1032_viewport_local_visibility(
        handles,flags,N,layers,L,overrides,V,expected));
    for (size_t first=0u;first<N;first+=257u) {
        const size_t count=N-first<257u?N-first:257u;
        assert(musa_dwg1032_viewport_plan_page(
            &plan,handles,flags,N,first,count,actual+first));
    }
    assert(memcmp(actual,expected,N)==0);
}
static void reject_invalid_metadata_atomically(void) {
    MusaDwg1032ViewportPlan plan, saved;
    assert(musa_dwg1032_viewport_plan_init(
        &plan,layers,L,overrides,V));
    saved=plan;
    layers[L-1u].handle=layers[L-2u].handle;
    assert(!musa_dwg1032_viewport_plan_init(
        &plan,layers,L,overrides,V));
    assert(memcmp(&plan,&saved,sizeof(plan))==0);
    layers[L-1u].handle=L;
    overrides[V-1u].frozen=2u;
    assert(!musa_dwg1032_viewport_plan_init(
        &plan,layers,L,overrides,V));
    assert(memcmp(&plan,&saved,sizeof(plan))==0);
    overrides[V-1u].frozen=(uint8_t)((V-1u)%5u==0u);
    overrides[V-1u].layer_handle=UINT64_MAX;
    assert(!musa_dwg1032_viewport_plan_init(
        &plan,layers,L,overrides,V));
    assert(memcmp(&plan,&saved,sizeof(plan))==0);
    overrides[V-1u].layer_handle=(uint64_t)((V-1u)*8u+1u);
    assert(!musa_dwg1032_viewport_plan_init(
        &plan,layers,L,overrides,V-1u+(size_t)SIZE_MAX));
    assert(memcmp(&plan,&saved,sizeof(plan))==0);
    assert(!musa_dwg1032_viewport_plan_init(
        (MusaDwg1032ViewportPlan *)(void *)layers,
        layers,L,overrides,V));
    assert(musa_dwg1032_viewport_plan_init(&plan,layers,L,NULL,0u));
    uint8_t baseline[257],page[257];
    assert(musa_dwg1032_viewport_local_visibility(
        handles,flags,257u,layers,L,NULL,0u,baseline));
    assert(musa_dwg1032_viewport_plan_page(
        &plan,handles,flags,N,0u,257u,page));
    assert(memcmp(baseline,page,257u)==0);
}
static void reject_invalid_page_atomically(void) {
    MusaDwg1032ViewportPlan plan;
    assert(musa_dwg1032_viewport_plan_init(
        &plan,layers,L,overrides,V));
    uint8_t page[257];
    memset(page,0xa5,sizeof(page));
    handles[N-1u]=UINT64_MAX;
    assert(!musa_dwg1032_viewport_plan_page(
        &plan,handles,flags,N,N-257u,257u,page));
    handles[N-1u]=(uint64_t)(((N-1u)*53u)%L)+1u;
    flags[N-1u]=2u;
    assert(!musa_dwg1032_viewport_plan_page(
        &plan,handles,flags,N,N-257u,257u,page));
    flags[N-1u]=(uint8_t)((N-1u)%19u!=0u);
    assert(!musa_dwg1032_viewport_plan_page(
        &plan,handles,flags,N,N,1u,page));
    assert(!musa_dwg1032_viewport_plan_page(
        &plan,handles,flags,N,SIZE_MAX,1u,page));
    assert(!musa_dwg1032_viewport_plan_page(
        &plan,handles,flags,SIZE_MAX,0u,257u,page));
    assert(!musa_dwg1032_viewport_plan_page(
        &plan,handles,flags,N,0u,0u,page));
    assert(!musa_dwg1032_viewport_plan_page(
        &plan,handles,flags,N,0u,257u,flags+1000u));
    assert(!musa_dwg1032_viewport_plan_page(
        &plan,handles,flags,N,0u,257u,
        (uint8_t *)(void *)(handles+1000u)));
    assert(!musa_dwg1032_viewport_plan_page(
        &plan,handles,flags,N,0u,257u,
        (uint8_t *)(void *)layers));
    assert(!musa_dwg1032_viewport_plan_page(
        &plan,handles,flags,N,0u,257u,
        (uint8_t *)(void *)overrides));
    assert(!musa_dwg1032_viewport_plan_page(
        &plan,handles,flags,N,0u,257u,
        (uint8_t *)(void *)&plan));
    assert(!musa_dwg1032_viewport_plan_page(
        &plan,handles,flags,N,0u,257u,NULL));
    for (size_t i=0u;i<sizeof(page);++i) assert(page[i]==0xa5u);
    assert(musa_dwg1032_viewport_plan_page(
        &plan,handles,flags,N,N-257u,257u,page));
    assert(memcmp(page,expected+N-257u,257u)==0);
}
int main(void) {
    setup();
    compare_pages();
    reject_invalid_metadata_atomically();
    reject_invalid_page_atomically();
    return 0;
}
