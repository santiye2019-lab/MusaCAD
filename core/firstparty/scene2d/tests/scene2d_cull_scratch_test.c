/* SPDX-License-Identifier: MIT
 * Original synthetic large-sheet viewport scratch regression.
 * No real DWG file is decoded or rendered.
 */
#include "musa_scene2d_cull.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

static void rotated_and_atomic(void) {
    const MusaScene2dNode nodes[3] = {
        {.handle=1,.kind=MUSA_SCENE2D_ENTITY},
        {.handle=2,.kind=MUSA_SCENE2D_ENTITY},
        {.handle=3,.kind=MUSA_SCENE2D_INSERT}
    };
    MusaScene2dResolved resolved[3] = {
        {.handle=1,.visible=1,.world={0,1,-1,0,10,20}},
        {.handle=2,.visible=0,.world={1,0,0,1,0,0}},
        {.handle=3,.visible=1,.world={1,0,0,1,0,0}}
    };
    const MusaScene2dBox boxes[3] = {{0,0,2,2},{0,0,2,2},
                                      {NAN,NAN,NAN,NAN}};
    const MusaScene2dBox viewport = {8,20,10,22};
    size_t out[3]={99,98,97}, scratch[3]={0}, written=44;
    assert(musa_scene2d_collect_visible_scratch(
        nodes,resolved,boxes,3,viewport,out,3,&written,scratch,3));
    assert(written==1 && out[0]==0 && out[1]==98);
    const size_t saved[3]={out[0],out[1],out[2]};
    resolved[1].handle=999;
    written=44;
    assert(!musa_scene2d_collect_visible_scratch(
        nodes,resolved,boxes,3,viewport,out,3,&written,scratch,3));
    assert(written==44 && !memcmp(out,saved,sizeof out));
    resolved[1].handle=2;
    assert(!musa_scene2d_collect_visible_scratch(
        nodes,resolved,boxes,3,viewport,out,0,&written,scratch,3));
    assert(!musa_scene2d_collect_visible_scratch(
        nodes,resolved,boxes,3,viewport,out,3,&written,scratch,2));
    assert(!musa_scene2d_collect_visible_scratch(
        nodes,resolved,boxes,3,viewport,out,3,&written,out,3));
    assert(!musa_scene2d_collect_visible_scratch(
        nodes,resolved,boxes,3,viewport,out,3,&written,
        (size_t *)(void *)resolved,3));
    assert(!musa_scene2d_collect_visible_scratch(
        nodes,resolved,boxes,3,viewport,out,3,&out[0],scratch,3));
    assert(written==44 && !memcmp(out,saved,sizeof out));
    assert(musa_scene2d_collect_visible_scratch(
        NULL,NULL,NULL,0,viewport,NULL,0,&written,NULL,0));
    assert(written==0);
    written=44;
    assert(!musa_scene2d_collect_visible_scratch(
        NULL,NULL,NULL,0,(MusaScene2dBox){2,0,1,1},
        NULL,0,&written,NULL,0));
    assert(written==44);
}
static void large_sheet_differential(void) {
    enum { N=20000 };
    static MusaScene2dNode nodes[N];
    static MusaScene2dResolved resolved[N];
    static MusaScene2dBox boxes[N];
    static size_t expected[N], out[N], scratch[N], snapshot[N];
    const MusaScene2dBox viewport={0,-1,30000,2};
    for (size_t i=0;i<N;++i) {
        nodes[i].handle=(uint64_t)i+1u;
        nodes[i].kind=MUSA_SCENE2D_ENTITY;
        resolved[i].handle=(uint64_t)i+1u;
        resolved[i].visible=(uint8_t)(i%5u!=0u);
        resolved[i].world[0]=1;
        resolved[i].world[3]=1;
        resolved[i].world[4]=2.0*(double)i;
        boxes[i]=(MusaScene2dBox){0,0,1,1};
        out[i]=SIZE_MAX;
    }
    size_t baseline_count=0, written=77;
    assert(musa_scene2d_collect_visible(
        nodes,resolved,boxes,N,viewport,expected,N,&baseline_count));
    assert(musa_scene2d_collect_visible_scratch(
        nodes,resolved,boxes,N,viewport,out,N,&written,scratch,N));
    assert(written==baseline_count && baseline_count>10000);
    assert(!memcmp(out,expected,written*sizeof(*out)));
    memcpy(snapshot,out,sizeof out);
    /* Invalid final entity must not leak a partially collected list. */
    resolved[N-1].handle=0;
    written=77;
    assert(!musa_scene2d_collect_visible_scratch(
        nodes,resolved,boxes,N,viewport,out,N,&written,scratch,N));
    assert(written==77 && !memcmp(out,snapshot,sizeof out));
    resolved[N-1].handle=(uint64_t)N;
    assert(!musa_scene2d_collect_visible_scratch(
        nodes,resolved,boxes,N,viewport,out,baseline_count-1u,
        &written,scratch,N));
    assert(written==77 && !memcmp(out,snapshot,sizeof out));
    /* A zero-hit viewport accepts zero output capacity. */
    const MusaScene2dBox far={1000000,0,1000001,1};
    assert(musa_scene2d_collect_visible_scratch(
        nodes,resolved,boxes,N,far,out,0,&written,scratch,N));
    assert(written==0 && !memcmp(out,snapshot,sizeof out));
}
int main(void) {
    rotated_and_atomic();
    large_sheet_differential();
    return 0;
}
