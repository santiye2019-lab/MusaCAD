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


/* Distinct viewport overrides must be composed with the *same* authenticated
 * owner forest, without allowing a thawed child to reveal a hidden parent. */
static void viewport_owner_contexts(void) {
    MusaDwg1032ObjectLocation entries[]={
        {10,0,1},{20,1,1},{30,2,1},{40,3,1},{50,4,1},{60,5,1}
    };
    MusaDwg1032ObjectIndex index={0};
    assert(musa_dwg1032_object_index_init(&index,entries,6,6));
    uint64_t owners[]={0,10,20,20,0,50};
    uint64_t layer_refs[]={100,100,200,100,100,200};
    uint8_t entity[]={1,1,1,1,1,1};
    MusaDwg1032LayerState layers[]={{100,1,1},{200,1,1}};
    MusaDwg1032ViewportFreeze override[]={{200,1}};
    MusaDwg1032OwnerWork work[8];
    uint8_t local[6],out[]={9,9,9,9,9,9};
    assert(musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        work,8,local,out));
    const uint8_t frozen[]={1,1,0,1,1,0};
    assert(!memcmp(out,frozen,sizeof(out)));
    override[0].frozen=0;
    assert(musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        work,8,local,out));
    const uint8_t thawed[]={1,1,1,1,1,1};
    assert(!memcmp(out,thawed,sizeof(out)));
    override[0].layer_handle=100;
    override[0].frozen=1;
    assert(musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        work,8,local,out));
    const uint8_t hidden[]={0,0,0,0,0,0};
    assert(!memcmp(out,hidden,sizeof(out)));
    /* A hidden ancestor prevents revealing even a thawed child. */
    override[0].layer_handle=200;
    override[0].frozen=0;
    entity[1]=0;
    assert(musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        work,8,local,out));
    const uint8_t hidden_parent[]={1,0,0,0,1,1};
    assert(!memcmp(out,hidden_parent,sizeof(out)));
    entity[1]=1;
    uint8_t saved[6]; memcpy(saved,out,sizeof(saved));
    owners[1]=30; /* A late cycle must not publish a prefix. */
    assert(!musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        work,8,local,out));
    assert(!memcmp(out,saved,sizeof(out)));
    owners[1]=999;
    assert(!musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        work,8,local,out));
    assert(!memcmp(out,saved,sizeof(out)));
    owners[1]=10;
    layer_refs[5]=999;
    assert(!musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        work,8,local,out));
    assert(!memcmp(out,saved,sizeof(out)));
    layer_refs[5]=200;
    override[0].frozen=2;
    assert(!musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        work,8,local,out));
    assert(!memcmp(out,saved,sizeof(out)));
    override[0].frozen=0;
    assert(!musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        work,8,out,out));
    assert(!musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        (MusaDwg1032OwnerWork *)(void *)layers,8,local,out));
    assert(!musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        work,SIZE_MAX,local,out));
    assert(!memcmp(out,saved,sizeof(out)));
    entries[5].handle=20; /* Caller-mutated index must be rejected. */
    assert(!musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        work,8,local,out));
    assert(!memcmp(out,saved,sizeof(out)));
}

static void large_viewport_owner_chain(void) {
    enum { N=20000 };
    static MusaDwg1032ObjectLocation entries[N];
    static uint64_t owners[N],layer_refs[N];
    static uint8_t entity[N],local[N],out[N],saved[N];
    static MusaDwg1032OwnerWork work[N];
    const MusaDwg1032LayerState layers[]={{100,1,1},{200,1,1}};
    MusaDwg1032ViewportFreeze override[]={{200,1}};
    for (size_t i=0u;i<N;++i) {
        entries[i]=(MusaDwg1032ObjectLocation){
            (uint64_t)i+1u,(uint64_t)i,1u};
        owners[i]=i?(uint64_t)i:0u;
        layer_refs[i]=(i==N/2u)?200u:100u;
        entity[i]=1u;
        out[i]=9u;
    }
    MusaDwg1032ObjectIndex index={0};
    assert(musa_dwg1032_object_index_init(&index,entries,N,N));
    assert(musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        work,N,local,out));
    for (size_t i=0u;i<N;++i)
        assert(out[i]==(uint8_t)(i<N/2u));
    override[0].frozen=0;
    assert(musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        work,N,local,out));
    for (size_t i=0u;i<N;++i) assert(out[i]==1u);
    memcpy(saved,out,sizeof(out));
    owners[0]=N; /* 20k-node cycle is discovered before publishing. */
    assert(!musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        work,N,local,out));
    assert(!memcmp(out,saved,sizeof(out)));
    owners[0]=0u;
    entity[N-1u]=2u;
    assert(!musa_dwg1032_viewport_owner_visibility(
        &index,owners,layer_refs,entity,layers,2,override,1,
        work,N,local,out));
    assert(!memcmp(out,saved,sizeof(out)));
}


/* Layer zero is definition-local: the same BLOCK must use the selected
 * INSERT instance's visibility in each viewport, not its original state. */
static void viewport_insert_instances(void) {
    MusaDwg1032LayerState layers[]={
        {10,0,0},{20,1,1},{30,1,1},{40,1,0}
    };
    uint64_t handles[]={10,20,30,40,10,30};
    uint8_t owner[]={1,1,1,1,0,0};
    MusaDwg1032ViewportFreeze vp[]={{10,1},{20,1},{30,0}};
    uint8_t out[]={9,9,9,9,9,9};
    assert(musa_dwg1032_viewport_insert_visibility(
        handles,owner,6,10,1,layers,4,vp,3,out));
    const uint8_t a[]={1,0,1,0,0,0};
    assert(!memcmp(out,a,sizeof(out)));
    vp[1].frozen=0;
    vp[2].frozen=1;
    assert(musa_dwg1032_viewport_insert_visibility(
        handles,owner,6,10,1,layers,4,vp,3,out));
    const uint8_t b[]={1,1,0,0,0,0};
    assert(!memcmp(out,b,sizeof(out)));
    assert(musa_dwg1032_viewport_insert_visibility(
        handles,owner,6,10,0,layers,4,vp,3,out));
    const uint8_t hidden[]={0,0,0,0,0,0};
    assert(!memcmp(out,hidden,sizeof(out)));
    memcpy(out,b,sizeof(out));
    uint8_t saved[6]; memcpy(saved,out,sizeof(saved));
    handles[5]=999;
    assert(!musa_dwg1032_viewport_insert_visibility(
        handles,owner,6,10,1,layers,4,vp,3,out));
    assert(!memcmp(saved,out,sizeof(out)));
    handles[5]=30;
    owner[5]=2;
    assert(!musa_dwg1032_viewport_insert_visibility(
        handles,owner,6,10,1,layers,4,vp,3,out));
    assert(!memcmp(saved,out,sizeof(out)));
    owner[5]=0;
    vp[2].layer_handle=20; /* Repeated viewport override. */
    assert(!musa_dwg1032_viewport_insert_visibility(
        handles,owner,6,10,1,layers,4,vp,3,out));
    vp[2].layer_handle=30;
    vp[2].frozen=2; /* Boolean only. */
    assert(!musa_dwg1032_viewport_insert_visibility(
        handles,owner,6,10,1,layers,4,vp,3,out));
    vp[2].frozen=1;
    layers[3].on=2;
    assert(!musa_dwg1032_viewport_insert_visibility(
        handles,owner,6,10,1,layers,4,vp,3,out));
    layers[3].on=1;
    assert(!musa_dwg1032_viewport_insert_visibility(
        handles,owner,6,999,1,layers,4,vp,3,out));
    assert(!musa_dwg1032_viewport_insert_visibility(
        handles,owner,6,10,2,layers,4,vp,3,out));
    assert(!musa_dwg1032_viewport_insert_visibility(
        handles,owner,6,10,1,layers,4,vp,3,owner));
    assert(!musa_dwg1032_viewport_insert_visibility(
        handles,owner,6,10,1,layers,4,vp,3,(uint8_t *)(void *)handles));
    assert(!memcmp(saved,out,sizeof(out)));
    assert(musa_dwg1032_viewport_insert_visibility(
        NULL,NULL,0,10,1,layers,4,vp,3,NULL));
}
/* Differential reference for 20,000 children and two instances. Unlike the
 * implementation, this oracle uses simple linear layer/override search. */
static void large_viewport_insert_differential(void) {
    enum { N=20000, L=1024, V=128 };
    static uint64_t handles[N];
    static uint8_t owner[N],out[N],saved[N];
    static MusaDwg1032LayerState layers[L];
    static MusaDwg1032ViewportFreeze vp[V];
    for (size_t j=0u;j<L;++j) {
        layers[j]=(MusaDwg1032LayerState){
            (uint64_t)j+1u,
            (uint8_t)(j%17u!=0u),(uint8_t)(j%19u!=0u)};
    }
    for (size_t j=0u;j<V;++j)
        vp[j]=(MusaDwg1032ViewportFreeze){
            (uint64_t)(j*8u+1u),(uint8_t)(j%3u==0u)};
    for (size_t i=0u;i<N;++i) {
        handles[i]=(uint64_t)((i*37u)%L)+1u;
        owner[i]=(uint8_t)(i%11u!=0u);
        out[i]=9u;
    }
    for (uint8_t insert=0;insert<=1u;++insert) {
        assert(musa_dwg1032_viewport_insert_visibility(
            handles,owner,N,1u,insert,layers,L,vp,V,out));
        for (size_t i=0u;i<N;++i) {
            const uint64_t h=handles[i];
            uint8_t expected=(uint8_t)(insert && owner[i]);
            if (h!=1u) {
                size_t j=0u;
                while (j<L && layers[j].handle!=h) ++j;
                assert(j<L);
                uint8_t frozen=0u;
                for (size_t k=0u;k<V;++k)
                    if (vp[k].layer_handle==h) frozen=vp[k].frozen;
                expected=(uint8_t)(expected && layers[j].on &&
                                   layers[j].thawed && !frozen);
            }
            assert(out[i]==expected);
        }
    }
    memcpy(saved,out,sizeof(saved));
    owner[N-1u]=2u;
    assert(!musa_dwg1032_viewport_insert_visibility(
        handles,owner,N,1u,1u,layers,L,vp,V,out));
    assert(!memcmp(saved,out,sizeof(out)));
    owner[N-1u]=1u;
    handles[N-1u]=UINT64_MAX;
    assert(!musa_dwg1032_viewport_insert_visibility(
        handles,owner,N,1u,1u,layers,L,vp,V,out));
    assert(!memcmp(saved,out,sizeof(out)));
}

/* Page a large block definition through one INSERT without publishing an
 * invalid later page or overwriting future input records. */
static void paged_insert_children(void) {
    enum { N=20000, PAGE=127 };
    static uint64_t handles[N];
    static uint8_t owners[N],whole[N],page[PAGE];
    const MusaDwg1032LayerState layers[]={
        {10,0,0},{20,1,1},{30,1,1}
    };
    const MusaDwg1032ViewportFreeze vp[]={{30,1}};
    for (size_t i=0u;i<N;++i) {
        handles[i]=(i%3u==0u)?10u:((i%3u==1u)?20u:30u);
        owners[i]=(uint8_t)(i%17u!=0u);
    }
    assert(musa_dwg1032_viewport_insert_visibility(
        handles,owners,N,10u,1u,layers,3u,vp,1u,whole));
    for (size_t first=0u;first<N;first+=PAGE) {
        const size_t n=N-first<PAGE?N-first:PAGE;
        memset(page,0xa5,sizeof(page));
        assert(musa_dwg1032_viewport_insert_visibility_page(
            handles,owners,N,first,n,10u,1u,layers,3u,vp,1u,page));
        assert(!memcmp(page,whole+first,n));
        if (n<PAGE) assert(page[n]==0xa5);
    }
    memset(page,0x5a,sizeof(page));
    handles[N-1u]=UINT64_MAX;
    assert(!musa_dwg1032_viewport_insert_visibility_page(
        handles,owners,N,N-PAGE,PAGE,10u,1u,layers,3u,vp,1u,page));
    for (size_t i=0u;i<PAGE;++i) assert(page[i]==0x5a);
    /* An earlier page remains independently valid. */
    assert(musa_dwg1032_viewport_insert_visibility_page(
        handles,owners,N,0u,PAGE,10u,1u,layers,3u,vp,1u,page));
    assert(!memcmp(page,whole,PAGE));
    handles[N-1u]=30u;
    assert(!musa_dwg1032_viewport_insert_visibility_page(
        handles,owners,N,0u,0u,10u,1u,layers,3u,vp,1u,page));
    assert(!musa_dwg1032_viewport_insert_visibility_page(
        handles,owners,N,N+1u,1u,10u,1u,layers,3u,vp,1u,page));
    assert(!musa_dwg1032_viewport_insert_visibility_page(
        handles,owners,N,N-1u,2u,10u,1u,layers,3u,vp,1u,page));
    /* Even a disjoint selected page may not overwrite future inputs. */
    assert(!musa_dwg1032_viewport_insert_visibility_page(
        handles,owners,N,0u,4u,10u,1u,layers,3u,vp,1u,owners+N-4u));
    assert(!musa_dwg1032_viewport_insert_visibility_page(
        handles,owners,N,0u,4u,10u,1u,layers,3u,vp,1u,
        (uint8_t *)(void *)(handles+N-4u)));
    assert(musa_dwg1032_viewport_insert_visibility_page(
        handles,owners,N,0u,PAGE,10u,0u,layers,3u,vp,1u,page));
    for (size_t i=0u;i<PAGE;++i) assert(page[i]==0u);
}
int main(void) {
    viewport_contexts();
    large_viewport_differential();
    viewport_owner_contexts();
    large_viewport_owner_chain();
    viewport_insert_instances();
    large_viewport_insert_differential();
    paged_insert_children();
    return 0;
}

