/* SPDX-License-Identifier: MIT */
#include "musa_scene2d_cull.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#define I {1,0,0,1,0,0}
static const MusaScene2dLayer layers[] = {
    {0,0xffffffffu,1}, {10,0xff00ff00u,1}, {20,0xff0000ffu,0}
};
static const MusaScene2dNode nodes[] = {
    {10,0,10,0,MUSA_SCENE2D_INSERT,1,MUSA_SCENE2D_BYLAYER,{1,0,0,1,10,20}},
    {20,10,0,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYBLOCK,{1,0,0,1,2,3}},
    {30,10,20,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYLAYER,I},
    {40,0,10,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYLAYER,{0,1,-1,0,0,0}},
    {50,0,10,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYLAYER,{1,0,0,1,1000,1000}}
};
static const MusaScene2dBox boxes[] = {
    {NAN,NAN,NAN,NAN}, /* INSERT bounds are intentionally ignored. */
    {0,0,2,2}, {0,0,2,2}, {0,0,2,2}, {0,0,2,2}
};
int main(void) {
    MusaScene2dResolved resolved[5];
    size_t written = 99;
    assert(musa_scene2d_resolve(nodes,5,layers,3,resolved,5,&written));
    assert(written == 5 && resolved[2].visible == 0);
    size_t indices[5] = {99,99,99,99,99};
    const MusaScene2dBox near_child = {12,23,14,25};
    assert(musa_scene2d_collect_visible(nodes,resolved,boxes,5,
                                        near_child,indices,5,&written));
    assert(written == 1 && indices[0] == 1);
    const MusaScene2dBox near_rotated = {-2,0,0,2};
    assert(musa_scene2d_collect_visible(nodes,resolved,boxes,5,
                                        near_rotated,indices,5,&written));
    assert(written == 1 && indices[0] == 3);
    const MusaScene2dBox whole = {-10000,-10000,10000,10000};
    assert(musa_scene2d_collect_visible(nodes,resolved,boxes,5,
                                        whole,indices,5,&written));
    assert(written == 3 && indices[0] == 1 &&
           indices[1] == 3 && indices[2] == 4);
    /* A short destination must not be partially populated. */
    size_t untouched[5] = {91,92,93,94,95};
    written = 77;
    assert(!musa_scene2d_collect_visible(nodes,resolved,boxes,5,
                                         whole,untouched,2,&written));
    assert(written == 77 && untouched[0] == 91 && untouched[1] == 92);
    MusaScene2dBox bad[5];
    memcpy(bad,boxes,sizeof bad);
    bad[1].max_x = NAN;
    assert(!musa_scene2d_collect_visible(nodes,resolved,bad,5,
                                         whole,untouched,5,&written));
    assert(written == 77 && untouched[0] == 91);
    memcpy(bad,boxes,sizeof bad);
    bad[1].min_x = 3;
    assert(!musa_scene2d_collect_visible(nodes,resolved,bad,5,
                                         whole,untouched,5,&written));
    assert(written == 77 && untouched[0] == 91);
    MusaScene2dResolved corrupted[5];
    memcpy(corrupted,resolved,sizeof corrupted);
    corrupted[1].handle = 999;
    assert(!musa_scene2d_collect_visible(nodes,corrupted,boxes,5,
                                         whole,untouched,5,&written));
    assert(written == 77 && untouched[0] == 91);
    memcpy(corrupted,resolved,sizeof corrupted);
    corrupted[1].world[0] = 1e308;
    bad[1] = (MusaScene2dBox){0,0,2,2};
    assert(!musa_scene2d_collect_visible(nodes,corrupted,bad,5,
                                         whole,untouched,5,&written));
    assert(written == 77 && untouched[0] == 91);
    assert(!musa_scene2d_collect_visible(nodes,resolved,boxes,5,
                                         (MusaScene2dBox){NAN,0,1,1},
                                         untouched,5,&written));
    assert(written == 77);
    assert(!musa_scene2d_collect_visible(nodes,resolved,boxes,5,
                                         whole,(size_t *)(void *)resolved,
                                         5,&written));
    assert(written == 77);
    assert(musa_scene2d_collect_visible(NULL,NULL,NULL,0,
                                        whole,NULL,0,&written));
    assert(written == 0);
    return 0;
}
