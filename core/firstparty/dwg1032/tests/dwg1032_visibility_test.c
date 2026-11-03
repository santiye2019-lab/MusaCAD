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

static void layer_and_owner_visibility(void) {
    MusaDwg1032ObjectLocation entries[]={
        {10,0,1},{20,1,1},{30,2,1},{40,3,1},{50,4,1},{60,5,1}
    };
    MusaDwg1032ObjectIndex index={0};
    assert(musa_dwg1032_object_index_init(&index,entries,6,6));
    uint64_t owners[]={0,10,20,20,0,50};
    uint64_t layer_refs[]={100,100,100,200,300,100};
    uint8_t entity[]={1,1,1,1,1,1};
    MusaDwg1032LayerState layers[]={
        {100,1,1},{200,0,1},{300,1,0}
    };
    MusaDwg1032OwnerWork work[8];
    uint8_t local_work[6],visible[]={9,9,9,9,9,9};
    const uint8_t expected[]={1,1,1,0,0,0};
    assert(musa_dwg1032_owner_graph_layer_visibility(
        &index,owners,layer_refs,entity,layers,3,work,8,local_work,visible));
    assert(memcmp(visible,expected,sizeof(expected))==0);
    const uint8_t preserved[]={1,1,1,0,0,0};

    /* A valid but hidden ancestor must hide a child on a visible layer. */
    entity[1]=0;
    assert(musa_dwg1032_owner_graph_layer_visibility(
        &index,owners,layer_refs,entity,layers,3,work,8,local_work,visible));
    const uint8_t hidden[]={1,0,0,0,0,0};
    assert(memcmp(visible,hidden,sizeof(hidden))==0);
    entity[1]=1;
    assert(musa_dwg1032_owner_graph_layer_visibility(
        &index,owners,layer_refs,entity,layers,3,work,8,local_work,visible));
    assert(memcmp(visible,preserved,sizeof(preserved))==0);

    layer_refs[2]=999; /* Unknown layer must never default to visible. */
    assert(!musa_dwg1032_owner_graph_layer_visibility(
        &index,owners,layer_refs,entity,layers,3,work,8,local_work,visible));
    assert(memcmp(visible,preserved,sizeof(preserved))==0);
    layer_refs[2]=100;
    layer_refs[2]=0; /* Missing layer is not implicitly layer zero. */
    assert(!musa_dwg1032_owner_graph_layer_visibility(
        &index,owners,layer_refs,entity,layers,3,work,8,local_work,visible));
    layer_refs[2]=100;
    layers[1].handle=100; /* Unsorted/duplicate layer table. */
    assert(!musa_dwg1032_owner_graph_layer_visibility(
        &index,owners,layer_refs,entity,layers,3,work,8,local_work,visible));
    layers[1].handle=200;
    layers[1].on=2;
    assert(!musa_dwg1032_owner_graph_layer_visibility(
        &index,owners,layer_refs,entity,layers,3,work,8,local_work,visible));
    layers[1].on=0;
    layers[2].thawed=2;
    assert(!musa_dwg1032_owner_graph_layer_visibility(
        &index,owners,layer_refs,entity,layers,3,work,8,local_work,visible));
    layers[2].thawed=0;
    entity[2]=2;
    assert(!musa_dwg1032_owner_graph_layer_visibility(
        &index,owners,layer_refs,entity,layers,3,work,8,local_work,visible));
    entity[2]=1;
    owners[1]=30; /* 20 <-> 30 cycle detected after layer lookup. */
    assert(!musa_dwg1032_owner_graph_layer_visibility(
        &index,owners,layer_refs,entity,layers,3,work,8,local_work,visible));
    owners[1]=10;
    assert(!musa_dwg1032_owner_graph_layer_visibility(
        &index,owners,layer_refs,entity,layers,3,work,8,visible,visible));
    assert(!musa_dwg1032_owner_graph_layer_visibility(
        &index,owners,layer_refs,entity,layers,3,work,8,
        (uint8_t *)(void *)&work[7],visible));
    assert(!musa_dwg1032_owner_graph_layer_visibility(
        &index,owners,layer_refs,entity,layers,3,work,SIZE_MAX,
        local_work,visible));
    assert(memcmp(visible,preserved,sizeof(preserved))==0);
}

static void large_layer_owner_chain(void) {
    enum {N=20000};
    static MusaDwg1032ObjectLocation entries[N];
    static uint64_t owners[N],layer_refs[N];
    static uint8_t entity[N],local_work[N],visible[N];
    static MusaDwg1032OwnerWork work[N];
    MusaDwg1032LayerState layers[]={{100,1,1},{200,1,0}};
    for (size_t i=0;i<N;++i) {
        entries[i]=(MusaDwg1032ObjectLocation){(uint64_t)i+1u,(uint64_t)i,1};
        owners[i]=i?(uint64_t)i:0;
        layer_refs[i]=(i==N/2)?200u:100u;
        entity[i]=1;
        visible[i]=9;
    }
    MusaDwg1032ObjectIndex index={0};
    assert(musa_dwg1032_object_index_init(&index,entries,N,N));
    assert(musa_dwg1032_owner_graph_layer_visibility(
        &index,owners,layer_refs,entity,layers,2,work,N,local_work,visible));
    for (size_t i=0;i<N;++i)
        assert(visible[i]==(uint8_t)(i<N/2));
    owners[0]=N; /* Late cycle: output must remain unchanged. */
    assert(!musa_dwg1032_owner_graph_layer_visibility(
        &index,owners,layer_refs,entity,layers,2,work,N,local_work,visible));
    for (size_t i=0;i<N;++i)
        assert(visible[i]==(uint8_t)(i<N/2));
}


/* Two INSERT instances of the same definition must not share layer-0
 * visibility; layer 0 in the definition can be off while the instance is on. */
static void insert_instance_layer_zero(void) {
    const uint64_t layers[]={100u,200u,300u,100u,200u};
    const uint8_t owners[]={1u,1u,1u,0u,1u};
    const uint8_t layer_on[]={0u,1u,0u,1u,1u};
    uint8_t a[5]={9u,9u,9u,9u,9u};
    uint8_t b[5]={9u,9u,9u,9u,9u};
    const uint8_t expected[]={1u,1u,0u,0u,1u};
    const uint8_t hidden[]={0u,0u,0u,0u,0u};
    assert(musa_dwg1032_owner_graph_apply_insert_context(
        layers,owners,layer_on,5u,100u,1u,a));
    assert(memcmp(a,expected,sizeof(a))==0);
    assert(musa_dwg1032_owner_graph_apply_insert_context(
        layers,owners,layer_on,5u,100u,0u,b));
    assert(memcmp(b,hidden,sizeof(b))==0);
    assert(memcmp(a,expected,sizeof(a))==0);
    assert(!musa_dwg1032_owner_graph_apply_insert_context(
        layers,owners,layer_on,5u,100u,2u,b));
    assert(!musa_dwg1032_owner_graph_apply_insert_context(
        layers,owners,layer_on,5u,0u,1u,b));
    assert(!musa_dwg1032_owner_graph_apply_insert_context(
        layers,owners,layer_on,5u,100u,1u,(uint8_t *)(void *)layers));
    assert(memcmp(b,hidden,sizeof(b))==0);
    assert(musa_dwg1032_owner_graph_apply_insert_context(
        NULL,NULL,NULL,0u,100u,1u,NULL));
}


/* Large synthetic definition: instance visibility is O(n), and a malformed
 * last record cannot leak a partially resolved scene to the renderer. */
static void insert_instance_large_atomic(void) {
    enum { N=20000 };
    static uint64_t layers[N];
    static uint8_t owner[N],layer_on[N],out[N],saved[N];
    for (size_t i=0;i<N;++i) {
        layers[i]=i%3u==0u ? 100u : 200u;
        owner[i]=(uint8_t)(i%7u!=0u);
        layer_on[i]=(uint8_t)(i%5u!=0u);
        out[i]=9u;
    }
    assert(musa_dwg1032_owner_graph_apply_insert_context(
        layers,owner,layer_on,N,100u,1u,out));
    for (size_t i=0;i<N;++i) {
        const uint8_t expected=(uint8_t)(
            owner[i] && (layers[i]==100u || layer_on[i]));
        assert(out[i]==expected);
    }
    memcpy(saved,out,sizeof(out));
    layer_on[N-1u]=2u;
    assert(!musa_dwg1032_owner_graph_apply_insert_context(
        layers,owner,layer_on,N,100u,1u,out));
    assert(memcmp(saved,out,sizeof(out))==0);
    layer_on[N-1u]=1u;
    layers[N-1u]=0u;
    assert(!musa_dwg1032_owner_graph_apply_insert_context(
        layers,owner,layer_on,N,100u,1u,out));
    assert(memcmp(saved,out,sizeof(out))==0);
}

/* A nested INSERT receives its actual enclosing instance visibility;
 * a hidden outer instance cannot reveal a layer-zero inner child. */
static void nested_insert_instance_visibility(void) {
    const uint64_t outer_layers[]={100u,200u};
    const uint8_t outer_owner[]={1u,1u};
    const uint8_t outer_layer_on[]={0u,1u};
    const uint64_t inner_layers[]={100u,300u};
    const uint8_t inner_owner[]={1u,1u};
    const uint8_t inner_layer_on[]={0u,1u};
    uint8_t outer[2]={9u,9u},inner[2]={9u,9u};
    assert(musa_dwg1032_owner_graph_apply_insert_context(
        outer_layers,outer_owner,outer_layer_on,2u,100u,1u,outer));
    assert(outer[0]==1u && outer[1]==1u);
    assert(musa_dwg1032_owner_graph_apply_insert_context(
        inner_layers,inner_owner,inner_layer_on,2u,100u,outer[0],inner));
    assert(inner[0]==1u && inner[1]==1u);
    assert(musa_dwg1032_owner_graph_apply_insert_context(
        outer_layers,outer_owner,outer_layer_on,2u,100u,0u,outer));
    assert(musa_dwg1032_owner_graph_apply_insert_context(
        inner_layers,inner_owner,inner_layer_on,2u,100u,outer[0],inner));
    assert(inner[0]==0u && inner[1]==0u);
}

int main(void) {
    insert_instance_large_atomic();
    nested_insert_instance_visibility();
    insert_instance_layer_zero();
    forest_and_fail_closed();
    large_inherited_visibility();
    layer_and_owner_visibility();
    large_layer_owner_chain();
    return 0;
}
