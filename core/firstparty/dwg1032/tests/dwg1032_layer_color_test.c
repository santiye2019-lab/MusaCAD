/* SPDX-License-Identifier: MIT
 * Synthetic post-decode layer color provenance; not real AC1032 fixtures.
 */
#include "musa_dwg1032_layer_color.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static void inheritance_and_fail_closed(void) {
    MusaDwg1032ObjectLocation entries[]={
        {10,0,1},{20,1,1},{30,2,1},{40,3,1},{50,4,1}
    };
    MusaDwg1032ObjectIndex index={0};
    assert(musa_dwg1032_object_index_init(&index,entries,5,5));
    uint64_t layer_refs[]={100,200,100,200,100};
    MusaDwg1032CadColor entity[]={
        {256,0,0},{256,0,0},{0,0,0},{3,0,0},{0,1,0x123456}
    };
    MusaDwg1032LayerColor layers[]={
        {100,{1,0,0}},{200,{7,1,0xabcdef}}
    };
    MusaDwg1032ResolvedColor out[5];
    memset(out,0xa5,sizeof(out));
    assert(musa_dwg1032_layer_color_select(
        &index,layer_refs,entity,layers,2,out));
    assert(out[0].byblock==0 && out[0].color.aci==1 &&
           out[0].color.has_truecolor==0);
    assert(out[1].byblock==0 && out[1].color.has_truecolor==1 &&
           out[1].color.truecolor_rgb==0xabcdef);
    assert(out[2].byblock==1 && out[2].color.aci==0);
    assert(out[3].byblock==0 && out[3].color.aci==3);
    assert(out[4].byblock==0 && out[4].color.has_truecolor==1 &&
           out[4].color.truecolor_rgb==0x123456);
    /* ByBlock is intentionally NOT resolved from the definition's owner:
     * an INSERT instance may supply a different color for each occurrence. */
    MusaDwg1032ResolvedColor saved[5];
    memcpy(saved,out,sizeof(out));
    layer_refs[4]=999;
    assert(!musa_dwg1032_layer_color_select(
        &index,layer_refs,entity,layers,2,out));
    assert(memcmp(out,saved,sizeof(out))==0);
    layer_refs[4]=100;
    layer_refs[4]=0;
    assert(!musa_dwg1032_layer_color_select(
        &index,layer_refs,entity,layers,2,out));
    layer_refs[4]=100;
    layers[1].handle=100;
    assert(!musa_dwg1032_layer_color_select(
        &index,layer_refs,entity,layers,2,out));
    layers[1].handle=200;
    layers[0].color.aci=256;
    assert(!musa_dwg1032_layer_color_select(
        &index,layer_refs,entity,layers,2,out));
    layers[0].color.aci=1;
    entity[4].truecolor_rgb=0x1000000;
    assert(!musa_dwg1032_layer_color_select(
        &index,layer_refs,entity,layers,2,out));
    entity[4].truecolor_rgb=0x123456;
    entity[4].has_truecolor=2;
    assert(!musa_dwg1032_layer_color_select(
        &index,layer_refs,entity,layers,2,out));
    entity[4].has_truecolor=1;
    entries[3].handle=30; /* Mutation after index validation. */
    assert(!musa_dwg1032_layer_color_select(
        &index,layer_refs,entity,layers,2,out));
    entries[3].handle=40;
    assert(!musa_dwg1032_layer_color_select(
        &index,layer_refs,entity,layers,2,
        (MusaDwg1032ResolvedColor *)(void *)entity));
    assert(!musa_dwg1032_layer_color_select(
        &index,layer_refs,entity,layers,2,
        (MusaDwg1032ResolvedColor *)(void *)entries));
    assert(!musa_dwg1032_layer_color_select(
        &index,layer_refs,entity,layers,SIZE_MAX,out));
    assert(memcmp(out,saved,sizeof(out))==0);
    assert(musa_dwg1032_layer_color_select(
        &index,layer_refs,entity,layers,2,out));
}

static void large_layer_table_and_rollback(void) {
    enum { N=20000, L=1024 };
    static MusaDwg1032ObjectLocation entries[N];
    static uint64_t layer_refs[N];
    static MusaDwg1032CadColor entity[N];
    static MusaDwg1032LayerColor layers[L];
    static MusaDwg1032ResolvedColor out[N];
    for (size_t j=0;j<L;++j) {
        layers[j].handle=(uint64_t)j+1u;
        layers[j].color=(MusaDwg1032CadColor){(uint16_t)(j%255u+1u),0,0};
    }
    for (size_t i=0;i<N;++i) {
        entries[i]=(MusaDwg1032ObjectLocation){(uint64_t)i+1u,(uint64_t)i,1};
        layer_refs[i]=(uint64_t)(i%L)+1u;
        entity[i]=(MusaDwg1032CadColor){256,0,0};
        out[i].byblock=9;
    }
    MusaDwg1032ObjectIndex index={0};
    assert(musa_dwg1032_object_index_init(&index,entries,N,N));
    assert(musa_dwg1032_layer_color_select(
        &index,layer_refs,entity,layers,L,out));
    for (size_t i=0;i<N;++i)
        assert(out[i].byblock==0 &&
               out[i].color.aci==(uint16_t)(i%L%255u+1u));
    layer_refs[N-1]=UINT64_MAX;
    assert(!musa_dwg1032_layer_color_select(
        &index,layer_refs,entity,layers,L,out));
    for (size_t i=0;i<N;++i)
        assert(out[i].byblock==0 &&
               out[i].color.aci==(uint16_t)(i%L%255u+1u));
}
int main(void) {
    inheritance_and_fail_closed();
    large_layer_table_and_rollback();
    return 0;
}
