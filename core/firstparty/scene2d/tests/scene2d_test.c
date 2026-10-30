/* SPDX-License-Identifier: MIT */
#include "musa_scene2d.h"
#ifdef NDEBUG
#undef NDEBUG /* Assertions must run in Release CI builds too. */
#endif
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define I {1,0,0,1,0,0}
static MusaScene2dNode sample[] = {
    {100,0,10,0,MUSA_SCENE2D_INSERT,1,MUSA_SCENE2D_BYLAYER,{1,0,0,1,10,20}},
    {200,100,0,0,MUSA_SCENE2D_INSERT,1,MUSA_SCENE2D_BYBLOCK,{2,0,0,2,3,4}},
    {300,200,0,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYBLOCK,{1,0,0,1,1,2}},
    {400,100,20,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYLAYER,I},
    {500,0,10,0xff00ff00u,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_EXPLICIT,I}
};
static MusaScene2dLayer layers[] = {
    {0,0xffffffffu,1}, {10,0xffff0000u,1}, {20,0xff0000ffu,0}
};
static void test_hierarchy(void) {
    MusaScene2dResolved result[5];
    size_t written = 999;
    assert(musa_scene2d_resolve(sample,5,layers,3,result,5,&written));
    assert(written == 5);
    assert(result[2].handle == 300 && result[2].effective_layer == 10);
    assert(result[2].rgba == 0xffff0000u && result[2].visible == 1);
    assert(result[2].world[0] == 2 && result[2].world[3] == 2);
    assert(result[2].world[4] == 15 && result[2].world[5] == 28);
    assert(result[3].rgba == 0xff0000ffu && result[3].visible == 0);
    assert(result[4].rgba == 0xff00ff00u && result[4].visible == 1);
}
static void test_nested_block_visibility(void) {
    MusaScene2dNode nodes[5];
    MusaScene2dLayer layer_copy[3];
    MusaScene2dResolved result[5];
    size_t written=0;
    memcpy(nodes,sample,sizeof nodes);
    memcpy(layer_copy,layers,sizeof layer_copy);
    nodes[0].visible=0; /* A hidden INSERT hides its descendants. */
    assert(musa_scene2d_resolve(nodes,5,layer_copy,3,result,5,&written));
    assert(written==5 && !result[0].visible && !result[1].visible &&
           !result[2].visible && result[4].visible);
    assert(result[2].effective_layer==10u &&
           result[2].rgba==0xffff0000u);
    nodes[0].visible=1;
    layer_copy[1].visible=0; /* A hidden inherited layer also hides children. */
    assert(musa_scene2d_resolve(nodes,5,layer_copy,3,result,5,&written));
    assert(!result[0].visible && !result[1].visible &&
           !result[2].visible && !result[4].visible);
    layer_copy[1].visible=1;
    nodes[2].layer=20u; /* Explicit child layer overrides layer-0 inheritance. */
    assert(musa_scene2d_resolve(nodes,5,layer_copy,3,result,5,&written));
    assert(result[0].visible && result[1].visible && !result[2].visible);
    assert(result[2].effective_layer==20u &&
           result[2].rgba==0xffff0000u); /* BYBLOCK still inherits color. */
}
static void test_atomic_failures(void) {
    MusaScene2dNode n[5];
    MusaScene2dResolved out[5];
    memset(out,0xa5,sizeof out);
    MusaScene2dResolved saved[5];
    memcpy(saved,out,sizeof out);
    size_t written=42;
#define REJECT() do { \
    assert(!musa_scene2d_resolve(n,5,layers,3,out,5,&written)); \
    assert(written==42 && memcmp(out,saved,sizeof out)==0); \
} while(0)
    memcpy(n,sample,sizeof n);
    n[0].parent_insert=200; REJECT(); /* cycle */
    memcpy(n,sample,sizeof n);
    n[2].parent_insert=500; REJECT(); /* parent not INSERT */
    memcpy(n,sample,sizeof n);
    n[2].parent_insert=999; REJECT(); /* missing parent */
    memcpy(n,sample,sizeof n);
    n[2].handle=200; REJECT(); /* duplicate */
    memcpy(n,sample,sizeof n);
    n[2].layer=999; REJECT(); /* missing layer */
    memcpy(n,sample,sizeof n);
    n[4].color_mode=MUSA_SCENE2D_BYBLOCK; REJECT(); /* BYBLOCK root */
    memcpy(n,sample,sizeof n);
    n[0].affine[4]=INFINITY; REJECT();
    memcpy(n,sample,sizeof n);
    n[0].affine[0]=1e308;
    n[1].affine[0]=1e308; REJECT(); /* composed overflow */
    memcpy(n,sample,sizeof n);
    assert(!musa_scene2d_resolve(n,5,layers,3,out,4,&written));
    assert(written==42 && memcmp(out,saved,sizeof out)==0);
    MusaScene2dLayer bad[3];
    memcpy(bad,layers,sizeof bad);
    bad[1].handle=0;
    assert(!musa_scene2d_resolve(n,5,bad,3,out,5,&written));
    assert(written==42 && memcmp(out,saved,sizeof out)==0);
    /* Reject success-path aliasing too: output must not overwrite source. */
    assert(!musa_scene2d_resolve(n,5,layers,3,
             (MusaScene2dResolved *)(void *)n,5,&written));
    assert(written==42);
    assert(!musa_scene2d_resolve(n,5,layers,3,out,5,
             (size_t *)(void *)out));
    assert(memcmp(out,saved,sizeof out)==0);
#undef REJECT
}
static void test_deep_chain(void) {
    const size_t count=20000;
    MusaScene2dNode *n=calloc(count,sizeof *n);
    MusaScene2dResolved *out=calloc(count,sizeof *out);
    assert(n && out);
    for (size_t i=0;i<count;++i) {
        n[i].handle=(uint64_t)i+1;
        n[i].parent_insert=i?(uint64_t)i:0;
        n[i].layer=10;
        n[i].kind=i==count-1?MUSA_SCENE2D_ENTITY:MUSA_SCENE2D_INSERT;
        n[i].visible=1;
        n[i].color_mode=MUSA_SCENE2D_BYLAYER;
        n[i].affine[0]=1;n[i].affine[3]=1;n[i].affine[4]=1;
    }
    size_t written=0;
    assert(musa_scene2d_resolve(n,count,layers,3,out,count,&written));
    assert(written==count && out[count-1].world[4]==(double)count);
    free(out);free(n);
}
int main(void) {
    test_hierarchy();
    test_nested_block_visibility();
    test_atomic_failures();
    test_deep_chain();
    return 0;
}
