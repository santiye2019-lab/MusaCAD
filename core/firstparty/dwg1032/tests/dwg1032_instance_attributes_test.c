/* SPDX-License-Identifier: MIT
 * Synthetic, already-decoded INSERT and viewport composition tests.
 * This is NOT a genuine AC1032 DWG fixture.
 */
#include "musa_dwg1032_instance_attributes.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void two_instances_atomicity(void) {
    const MusaDwg1032ResolvedColor definition[5]={
        {{7,0,0},0},{{3,0,0},0},{{0,0,0},1},
        {{256,1,0xabcdefu},0},{{5,0,0},0}
    };
    const MusaDwg1032CadColor raw[5]={
        {256,0,0},{256,0,0},{0,0,0},
        {256,1,0xabcdefu},{256,0,0}
    };
    uint64_t handles[5]={10,20,10,30,20};
    uint8_t owner[5]={1,1,1,1,0};
    const MusaDwg1032LayerState layers[3]={
        {10,0,0},{20,1,1},{30,1,1}
    };
    MusaDwg1032ViewportFreeze freeze[1]={{20,1}};
    const MusaDwg1032ResolvedColor red={{1,0,0},0};
    const MusaDwg1032ResolvedColor blue={{5,0,0},0};
    const MusaDwg1032ResolvedColor green={{3,0,0},0};
    const MusaDwg1032ResolvedColor rgb={{256,1,0x123456u},0};
    MusaDwg1032ResolvedColor color[5],tmp[6],saved_color[5];
    uint8_t visible[5],scratch_visible[6],saved_visible[5];
    memset(color,0xa5,sizeof(color));
    memset(visible,0xa5,sizeof(visible));
#define COMPOSE(IC,LC,IV) musa_dwg1032_instance_attributes(\
    definition,raw,handles,owner,5,10,IV,IC,LC,layers,3,\
    freeze,1,color,visible,tmp,6,scratch_visible,6)
    assert(COMPOSE(&red,&blue,1));
    assert(color[0].color.aci==5 && color[1].color.aci==3);
    assert(color[2].color.aci==1 && color[3].color.truecolor_rgb==0xabcdefu);
    assert(color[4].color.aci==5);
    const uint8_t expected_a[]={1,0,1,1,0};
    assert(!memcmp(visible,expected_a,sizeof(visible)));
    freeze[0].frozen=0;
    assert(COMPOSE(&green,&rgb,1));
    assert(color[0].color.truecolor_rgb==0x123456u &&
           color[2].color.aci==3);
    const uint8_t expected_b[]={1,1,1,1,0};
    assert(!memcmp(visible,expected_b,sizeof(visible)));
    assert(COMPOSE(&green,&rgb,0));
    for (size_t i=0;i<5u;++i) assert(!visible[i]);
    memcpy(saved_color,color,sizeof(color));
    memcpy(saved_visible,visible,sizeof(visible));
    handles[4]=999; /* Color conversion passes, viewport rejects. */
    assert(!COMPOSE(&red,&blue,1));
    assert(!memcmp(color,saved_color,sizeof(color)));
    assert(!memcmp(visible,saved_visible,sizeof(visible)));
    handles[4]=20;
    owner[4]=2; /* Late owner corruption, no output published. */
    assert(!COMPOSE(&red,&blue,1));
    assert(!memcmp(color,saved_color,sizeof(color)));
    assert(!memcmp(visible,saved_visible,sizeof(visible)));
    owner[4]=0;
    MusaDwg1032ResolvedColor unresolved={{0,0,0},1};
    assert(!COMPOSE(&unresolved,&blue,1));
    assert(!COMPOSE(&red,&unresolved,1));
    assert(!COMPOSE(&red,&blue,2));
    assert(!memcmp(color,saved_color,sizeof(color)));
    assert(!memcmp(visible,saved_visible,sizeof(visible)));
    assert(!musa_dwg1032_instance_attributes(
        definition,raw,handles,owner,5,10,1,&red,&blue,
        layers,3,freeze,1,color,visible,color,5,scratch_visible,6));
    assert(!musa_dwg1032_instance_attributes(
        definition,raw,handles,owner,5,10,1,&red,&blue,
        layers,3,freeze,1,color,visible,tmp,6,visible,5));
    assert(!musa_dwg1032_instance_attributes(
        definition,raw,handles,owner,5,10,1,&red,&blue,
        layers,3,freeze,1,color,visible,tmp,4,scratch_visible,6));
    assert(!memcmp(color,saved_color,sizeof(color)));
    assert(!memcmp(visible,saved_visible,sizeof(visible)));
#undef COMPOSE
}
static void large_instance_crosscheck(void) {
    enum { N=20000,L=128,V=16 };
    MusaDwg1032ResolvedColor *definition=calloc(N,sizeof(*definition));
    MusaDwg1032CadColor *raw=calloc(N,sizeof(*raw));
    uint64_t *handles=calloc(N,sizeof(*handles));
    uint8_t *owners=calloc(N,sizeof(*owners));
    MusaDwg1032ResolvedColor *out=calloc(N,sizeof(*out));
    MusaDwg1032ResolvedColor *scratch=calloc(N,sizeof(*scratch));
    uint8_t *visible=calloc(N,sizeof(*visible));
    uint8_t *vwork=calloc(N,sizeof(*vwork));
    assert(definition && raw && handles && owners && out && scratch &&
           visible && vwork);
    MusaDwg1032LayerState layers[L];
    MusaDwg1032ViewportFreeze vp[V];
    for(size_t j=0u;j<L;++j) {
        layers[j]=(MusaDwg1032LayerState){
            (uint64_t)j+1u,
            (uint8_t)(j%17u!=0u),(uint8_t)(j%19u!=0u)};
    }
    for(size_t j=0u;j<V;++j)
        vp[j]=(MusaDwg1032ViewportFreeze){
            (uint64_t)(j*8u+1u),(uint8_t)(j%3u==0u)};
    for(size_t i=0u;i<N;++i) {
        handles[i]=(uint64_t)(i%L)+1u;
        owners[i]=(uint8_t)(i%7u!=0u);
        if (i%3u==0u) {
            raw[i]=(MusaDwg1032CadColor){0,0,0};
            definition[i]=(MusaDwg1032ResolvedColor){{0,0,0},1};
        } else if(i%3u==1u) {
            raw[i]=(MusaDwg1032CadColor){256,0,0};
            definition[i]=(MusaDwg1032ResolvedColor){{7,0,0},0};
        } else {
            raw[i]=(MusaDwg1032CadColor){4,0,0};
            definition[i]=(MusaDwg1032ResolvedColor){{4,0,0},0};
        }
    }
    const MusaDwg1032ResolvedColor red={{1,0,0},0};
    const MusaDwg1032ResolvedColor blue={{5,0,0},0};
    assert(musa_dwg1032_instance_attributes(
        definition,raw,handles,owners,N,1u,1u,&red,&blue,
        layers,L,vp,V,out,visible,scratch,N,vwork,N));
    for(size_t i=0u;i<N;++i) {
        const uint16_t aci=(uint16_t)(
            i%3u==0u ? 1u :
            i%3u==1u && handles[i]==1u ? 5u :
            i%3u==1u ? 7u : 4u);
        assert(!out[i].byblock && out[i].color.aci==aci);
        uint8_t expected=owners[i];
        if (handles[i]!=1u) {
            const size_t at=(size_t)handles[i]-1u;
            uint8_t frozen=0u;
            for(size_t j=0u;j<V;++j)
                if(vp[j].layer_handle==handles[i]) frozen=vp[j].frozen;
            expected=(uint8_t)(expected && layers[at].on &&
                               layers[at].thawed && !frozen);
        }
        assert(visible[i]==expected);
    }
    /* Later failure in viewport logic after valid color computation must
     * leave BOTH completed public arrays fully unchanged. */
    owners[N-1u]=2u;
    assert(!musa_dwg1032_instance_attributes(
        definition,raw,handles,owners,N,1u,1u,&red,&blue,
        layers,L,vp,V,out,visible,scratch,N,vwork,N));
    for(size_t i=0u;i<N;++i) {
        assert(!out[i].byblock);
        assert(visible[i]<=1u);
    }
    owners[N-1u]=1u;
    raw[N-1u].truecolor_rgb=0xffu;
    assert(!musa_dwg1032_instance_attributes(
        definition,raw,handles,owners,N,1u,1u,&red,&blue,
        layers,L,vp,V,out,visible,scratch,N,vwork,N));
    raw[N-1u].truecolor_rgb=0;
    free(vwork);free(visible);free(scratch);free(out);
    free(owners);free(handles);free(raw);free(definition);
}
int main(void) {
    two_instances_atomicity();
    large_instance_crosscheck();
    return 0;
}
