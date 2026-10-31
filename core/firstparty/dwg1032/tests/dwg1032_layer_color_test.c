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
static void per_insert_byblock_provenance(void) {
    MusaDwg1032ResolvedColor definition[4]={
        {{7,0,0},0},{{0,0,0},1},{{0,0,0},1},
        {{256,1,0xabcdef},0}
    };
    const MusaDwg1032ResolvedColor red={{1,0,0},0};
    const MusaDwg1032ResolvedColor blue={{5,0,0},0};
    const MusaDwg1032ResolvedColor nested={{0,0,0},1};
    MusaDwg1032ResolvedColor a[4],b[4],c[4],saved[4];
    assert(musa_dwg1032_layer_color_apply_insert(definition,4,&red,a));
    assert(musa_dwg1032_layer_color_apply_insert(definition,4,&blue,b));
    assert(a[0].color.aci==7 && b[0].color.aci==7);
    assert(a[1].color.aci==1 && b[1].color.aci==5);
    assert(a[2].color.aci==1 && b[2].color.aci==5);
    assert(a[3].color.has_truecolor && a[3].color.truecolor_rgb==0xabcdef);
    assert(b[3].color.has_truecolor && b[3].color.truecolor_rgb==0xabcdef);
    assert(musa_dwg1032_layer_color_apply_insert(definition,4,&nested,c));
    assert(c[1].byblock && c[2].byblock);
    assert(musa_dwg1032_layer_color_apply_insert(c,4,&blue,a));
    assert(a[1].color.aci==5 && a[2].color.aci==5);
    memcpy(saved,b,sizeof(saved));
    definition[3].byblock=2;
    assert(!musa_dwg1032_layer_color_apply_insert(definition,4,&red,b));
    assert(memcmp(saved,b,sizeof(saved))==0);
    definition[3].byblock=0;
    definition[3].color.truecolor_rgb=0x1000000;
    assert(!musa_dwg1032_layer_color_apply_insert(definition,4,&red,b));
    assert(memcmp(saved,b,sizeof(saved))==0);
    definition[3].color.truecolor_rgb=0xabcdef;
    assert(!musa_dwg1032_layer_color_apply_insert(definition,4,&red,definition));
    assert(!musa_dwg1032_layer_color_apply_insert(definition,4,&b[0],b));
    assert(!musa_dwg1032_layer_color_apply_insert(definition,SIZE_MAX,&red,b));
    assert(memcmp(saved,b,sizeof(saved))==0);
}

static void large_per_insert_atomicity(void) {
    enum { N=20000 };
    static MusaDwg1032ResolvedColor definition[N],out[N];
    const MusaDwg1032ResolvedColor insert={{256,1,0x123456},0};
    for (size_t i=0;i<N;++i) {
        definition[i]=i%2u
            ? (MusaDwg1032ResolvedColor){{0,0,0},1}
            : (MusaDwg1032ResolvedColor){{7,0,0},0};
    }
    assert(musa_dwg1032_layer_color_apply_insert(definition,N,&insert,out));
    for (size_t i=0;i<N;++i) {
        assert(!out[i].byblock);
        if (i%2u) assert(out[i].color.has_truecolor &&
                         out[i].color.truecolor_rgb==0x123456);
        else assert(out[i].color.aci==7 && !out[i].color.has_truecolor);
    }
    definition[N-1].color.aci=256; /* Invalid unresolved ByBlock. */
    assert(!musa_dwg1032_layer_color_apply_insert(definition,N,&insert,out));
    for (size_t i=0;i<N;++i)
        if (i%2u) assert(out[i].color.truecolor_rgb==0x123456);
        else assert(out[i].color.aci==7);
}
/* A stale RGB field must not silently become an explicit ACI result.
 * It would otherwise pass selection but fail later INSERT validation. */
static void reject_stale_rgb_across_color_stages(void) {
    MusaDwg1032ObjectLocation entry={10,0,1};
    MusaDwg1032ObjectIndex index={0};
    uint64_t layer_ref=100;
    MusaDwg1032CadColor entity={256,0,0};
    MusaDwg1032LayerColor layer={100,{7,0,0}};
    MusaDwg1032ResolvedColor out={{0,0,0},0};
    MusaDwg1032ResolvedColor saved;
    assert(musa_dwg1032_object_index_init(&index,&entry,1,1));
    assert(musa_dwg1032_layer_color_select(
        &index,&layer_ref,&entity,&layer,1,&out));
    assert(out.color.aci==7 && !out.byblock);
    memcpy(&saved,&out,sizeof(saved));
    entity.truecolor_rgb=0x112233;
    assert(!musa_dwg1032_layer_color_select(
        &index,&layer_ref,&entity,&layer,1,&out));
    assert(memcmp(&out,&saved,sizeof(out))==0);
    entity.truecolor_rgb=0;
    layer.color.truecolor_rgb=0x445566;
    assert(!musa_dwg1032_layer_color_select(
        &index,&layer_ref,&entity,&layer,1,&out));
    assert(memcmp(&out,&saved,sizeof(out))==0);
    layer.color.truecolor_rgb=0;
    entity=(MusaDwg1032CadColor){0,0,0};
    assert(musa_dwg1032_layer_color_select(
        &index,&layer_ref,&entity,&layer,1,&out));
    assert(out.byblock && out.color.aci==0 &&
           out.color.truecolor_rgb==0);
    const MusaDwg1032ResolvedColor insert={{2,0,0},0};
    MusaDwg1032ResolvedColor rendered={{0,0,0},0};
    assert(musa_dwg1032_layer_color_apply_insert(
        &out,1,&insert,&rendered));
    assert(!rendered.byblock && rendered.color.aci==2);
    memcpy(&saved,&rendered,sizeof(saved));
    MusaDwg1032ResolvedColor stale_insert={{2,0,0x112233},0};
    assert(!musa_dwg1032_layer_color_apply_insert(
        &out,1,&stale_insert,&rendered));
    assert(memcmp(&rendered,&saved,sizeof(rendered))==0);
    MusaDwg1032ResolvedColor stale_definition={{7,0,0x112233},0};
    assert(!musa_dwg1032_layer_color_apply_insert(
        &stale_definition,1,&insert,&rendered));
    assert(memcmp(&rendered,&saved,sizeof(rendered))==0);
}

/* Layer 0 is inherited from each INSERT instance, never from the shared
 * block definition. This is synthetic post-decode evidence only. */
static void per_insert_layer_zero_context(void) {
    const uint64_t zero_layer=100;
    uint64_t child_layers[6]={100,200,100,100,100,100};
    MusaDwg1032CadColor raw[6]={
        {256,0,0},{256,0,0},{0,0,0},{4,0,0},
        {256,1,0xabcdef},{256,0,0}
    };
    MusaDwg1032ResolvedColor definition[6]={
        {{7,0,0},0},{{3,0,0},0},{{0,0,0},1},
        {{4,0,0},0},{{256,1,0xabcdef},0},{{7,0,0},0}
    };
    const MusaDwg1032ResolvedColor red={{1,0,0},0};
    const MusaDwg1032ResolvedColor blue={{5,0,0},0};
    const MusaDwg1032ResolvedColor green={{3,0,0},0};
    const MusaDwg1032ResolvedColor rgb={{256,1,0x112233},0};
    const MusaDwg1032ResolvedColor nested={{0,0,0},1};
    MusaDwg1032ResolvedColor a[6],b[6],saved[6];
    assert(musa_dwg1032_layer_color_apply_insert_context(
        definition,raw,child_layers,6,zero_layer,&red,&blue,a));
    assert(musa_dwg1032_layer_color_apply_insert_context(
        definition,raw,child_layers,6,zero_layer,&green,&rgb,b));
    assert(a[0].color.aci==5 && b[0].color.truecolor_rgb==0x112233);
    assert(a[1].color.aci==3 && b[1].color.aci==3);
    assert(a[2].color.aci==1 && b[2].color.aci==3);
    assert(a[3].color.aci==4 && b[3].color.aci==4);
    assert(a[4].color.truecolor_rgb==0xabcdef &&
           b[4].color.truecolor_rgb==0xabcdef);
    assert(a[5].color.aci==5 && b[5].color.truecolor_rgb==0x112233);
    assert(musa_dwg1032_layer_color_apply_insert_context(
        definition,raw,child_layers,6,zero_layer,&nested,&blue,a));
    assert(a[2].byblock && a[0].color.aci==5);
    memcpy(saved,b,sizeof(saved));
    raw[5].truecolor_rgb=0x123456; /* Late malformed input. */
    assert(!musa_dwg1032_layer_color_apply_insert_context(
        definition,raw,child_layers,6,zero_layer,&red,&blue,b));
    assert(memcmp(saved,b,sizeof(b))==0);
    raw[5].truecolor_rgb=0;
    child_layers[5]=0;
    assert(!musa_dwg1032_layer_color_apply_insert_context(
        definition,raw,child_layers,6,zero_layer,&red,&blue,b));
    assert(memcmp(saved,b,sizeof(b))==0);
    child_layers[5]=100;
    definition[3].color.aci=9; /* Explicit source/definition mismatch. */
    assert(!musa_dwg1032_layer_color_apply_insert_context(
        definition,raw,child_layers,6,zero_layer,&red,&blue,b));
    definition[3].color.aci=4;
    assert(!musa_dwg1032_layer_color_apply_insert_context(
        definition,raw,child_layers,6,zero_layer,&red,&nested,b));
    assert(!musa_dwg1032_layer_color_apply_insert_context(
        definition,raw,child_layers,6,0,&red,&blue,b));
    assert(!musa_dwg1032_layer_color_apply_insert_context(
        definition,raw,child_layers,6,zero_layer,&red,&blue,definition));
    assert(!musa_dwg1032_layer_color_apply_insert_context(
        definition,raw,child_layers,6,zero_layer,&red,&blue,
        (MusaDwg1032ResolvedColor *)(void *)raw));
    assert(!musa_dwg1032_layer_color_apply_insert_context(
        definition,raw,child_layers,SIZE_MAX,zero_layer,&red,&blue,b));
    assert(memcmp(saved,b,sizeof(b))==0);
}

/* Large synthetic block definition: O(n) context selection, no allocation,
 * and no partial writes when the final child's provenance is corrupt. */
static void large_layer_zero_instance_atomicity(void) {
    enum { N=20000 };
    static MusaDwg1032ResolvedColor definition[N],out[N];
    static MusaDwg1032CadColor raw[N];
    static uint64_t layers[N];
    const MusaDwg1032ResolvedColor insert={{1,0,0},0};
    const MusaDwg1032ResolvedColor insert_layer={{5,0,0},0};
    for (size_t i=0;i<N;++i) {
        layers[i]=i%2u ? 200u : 100u;
        raw[i]=(MusaDwg1032CadColor){256,0,0};
        definition[i]=i%2u
            ? (MusaDwg1032ResolvedColor){{7,0,0},0}
            : (MusaDwg1032ResolvedColor){{3,0,0},0};
    }
    raw[N-2]=(MusaDwg1032CadColor){0,0,0};
    definition[N-2]=(MusaDwg1032ResolvedColor){{0,0,0},1};
    assert(musa_dwg1032_layer_color_apply_insert_context(
        definition,raw,layers,N,100,&insert,&insert_layer,out));
    for (size_t i=0;i<N;++i) {
        const uint16_t expected=i==N-2 ? 1u : (i%2u ? 7u : 5u);
        assert(!out[i].byblock && out[i].color.aci==expected);
    }
    layers[N-1]=0; /* Last-record corruption must not publish a prefix. */
    assert(!musa_dwg1032_layer_color_apply_insert_context(
        definition,raw,layers,N,100,&insert,&insert_layer,out));
    for (size_t i=0;i<N;++i) {
        const uint16_t expected=i==N-2 ? 1u : (i%2u ? 7u : 5u);
        assert(!out[i].byblock && out[i].color.aci==expected);
    }
}

int main(void) {
    large_layer_zero_instance_atomicity();
    per_insert_layer_zero_context();
    reject_stale_rgb_across_color_stages();
    inheritance_and_fail_closed();
    large_layer_table_and_rollback();
    per_insert_byblock_provenance();
    large_per_insert_atomicity();
    return 0;
}
