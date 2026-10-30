/* SPDX-License-Identifier: MIT */
#include "musa_scene2d_cache.h"
#include "musa_scene2d_index.h"
#include "musa_scene2d_viewport.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <float.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define I {1,0,0,1,0,0}
static const MusaScene2dLayer layers[] = {
    {0,0xffffffffu,1}, {10,0xff00ff00u,1}, {20,0xff0000ffu,0}
};
static const MusaScene2dNode nodes[] = {
    {100,0,10,0,MUSA_SCENE2D_INSERT,1,MUSA_SCENE2D_BYLAYER,{1,0,0,1,10,20}},
    {200,100,0,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYBLOCK,{1,0,0,1,2,3}},
    {300,100,20,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYLAYER,I},
    {400,0,10,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYLAYER,{0,1,-1,0,0,0}},
    {500,0,10,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYLAYER,{1,0,0,1,1000,1000}}
};
static const MusaScene2dBox local[] = {
    {NAN,NAN,NAN,NAN}, {0,0,2,2}, {0,0,2,2}, {0,0,2,2}, {0,0,2,2}
};
static void compare_query(const MusaScene2dResolved *r,
                          const MusaScene2dPreparedBounds *cache,
                          MusaScene2dBox view) {
    size_t old_indices[5] = {99,99,99,99,99};
    size_t new_indices[5] = {98,98,98,98,98};
    size_t old_count = 77, new_count = 88;
    assert(musa_scene2d_collect_visible(nodes,r,local,5,
                                        view,old_indices,5,&old_count));
    assert(musa_scene2d_collect_prepared(cache,5,view,
                                        new_indices,5,&new_count));
    assert(old_count == new_count);
    for (size_t i=0; i<old_count; ++i)
        assert(old_indices[i] == new_indices[i]);
}
static void test_cache_equivalence(void) {
    MusaScene2dResolved r[5];
    MusaScene2dPreparedBounds cache[5];
    size_t written = 77;
    assert(musa_scene2d_resolve(nodes,5,layers,3,r,5,&written));
    assert(musa_scene2d_prepare_bounds(nodes,r,local,5,cache,5,&written));
    assert(written == 5 && cache[0].visible == 0 &&
           cache[1].visible == 1 && cache[2].visible == 0);
    compare_query(r,cache,(MusaScene2dBox){12,23,14,25});
    compare_query(r,cache,(MusaScene2dBox){14,25,14,25});
    compare_query(r,cache,(MusaScene2dBox){-2,0,0,2});
    compare_query(r,cache,(MusaScene2dBox){-10000,-10000,10000,10000});
    compare_query(r,cache,(MusaScene2dBox){20000,20000,20001,20001});
    size_t indices[5] = {91,92,93,94,95};
    written = 77;
    assert(!musa_scene2d_collect_prepared(cache,5,
             (MusaScene2dBox){-10000,-10000,10000,10000},
             indices,2,&written));
    assert(written == 77 && indices[0] == 91 && indices[1] == 92);
    assert(musa_scene2d_collect_prepared(cache,5,
             (MusaScene2dBox){20000,20000,20001,20001},
             indices,0,&written));
    assert(written == 0);
    const uint8_t freeze_insert[] = {1,0,1};
    assert(musa_scene2d_resolve_viewport(nodes,5,layers,3,
                                         freeze_insert,r,5,&written));
    assert(musa_scene2d_prepare_bounds(nodes,r,local,5,cache,5,&written));
    assert(!cache[1].visible && !cache[2].visible && !cache[3].visible);
    compare_query(r,cache,(MusaScene2dBox){-10000,-10000,10000,10000});
}
static void test_atomic_failures(void) {
    MusaScene2dResolved r[5];
    MusaScene2dPreparedBounds out[5], saved[5];
    size_t written = 0;
    assert(musa_scene2d_resolve(nodes,5,layers,3,r,5,&written));
    memset(out,0xa5,sizeof out);
    memcpy(saved,out,sizeof out);
    written = 77;
    MusaScene2dBox bad[5];
    memcpy(bad,local,sizeof bad);
    bad[1].max_x = NAN;
    assert(!musa_scene2d_prepare_bounds(nodes,r,bad,5,out,5,&written));
    assert(written == 77 && memcmp(out,saved,sizeof out) == 0);
    memcpy(bad,local,sizeof bad);
    MusaScene2dResolved corrupted[5];
    memcpy(corrupted,r,sizeof corrupted);
    corrupted[1].world[4] = INFINITY;
    assert(!musa_scene2d_prepare_bounds(nodes,corrupted,bad,5,out,5,&written));
    assert(written == 77 && memcmp(out,saved,sizeof out) == 0);
    assert(!musa_scene2d_prepare_bounds(nodes,r,local,5,out,4,&written));
    assert(written == 77 && memcmp(out,saved,sizeof out) == 0);
    assert(!musa_scene2d_prepare_bounds(nodes,r,local,5,
            (MusaScene2dPreparedBounds *)(void *)r,5,&written));
    assert(written == 77);
    assert(!musa_scene2d_prepare_bounds(nodes,r,local,5,out,5,
            (size_t *)(void *)out));
    assert(memcmp(out,saved,sizeof out) == 0);
    assert(musa_scene2d_prepare_bounds(nodes,r,local,5,out,5,&written));
    MusaScene2dPreparedBounds copy[5];
    memcpy(copy,out,sizeof copy);
    copy[1].visible = 2;
    size_t indices[5] = {99,99,99,99,99};
    written = 77;
    assert(!musa_scene2d_collect_prepared(copy,5,
             (MusaScene2dBox){0,0,2000,2000},indices,5,&written));
    assert(written == 77 && indices[0] == 99);
    memcpy(copy,out,sizeof copy);
    copy[1].box.min_x = NAN;
    assert(!musa_scene2d_collect_prepared(copy,5,
             (MusaScene2dBox){0,0,2000,2000},indices,5,&written));
    assert(written == 77 && indices[0] == 99);
    assert(!musa_scene2d_collect_prepared(out,5,
             (MusaScene2dBox){0,0,2000,2000},
             (size_t *)(void *)out,5,&written));
    assert(written == 77);
    assert(musa_scene2d_prepare_bounds(NULL,NULL,NULL,0,NULL,0,&written));
    assert(written == 0);
    assert(musa_scene2d_collect_prepared(NULL,0,
             (MusaScene2dBox){0,0,1,1},NULL,0,&written));
    assert(written == 0);
}
/* A finite drawing point at the largest representable coordinate must
 * remain indexable, rather than becoming an infinite cached AABB. */
static void test_extreme_coordinate_consistency(void) {
    const MusaScene2dNode n[] = {
        {1,0,0,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYLAYER,I},
        {2,0,0,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYLAYER,I}
    };
    const MusaScene2dResolved r[] = {
        {1,0,0xff000000u,1,I},
        {2,0,0xff000000u,1,I}
    };
    const MusaScene2dBox local[] = {
        {DBL_MAX,DBL_MAX,DBL_MAX,DBL_MAX},
        {-DBL_MAX,-DBL_MAX,-DBL_MAX,-DBL_MAX}
    };
    MusaScene2dPreparedBounds prepared[2];
    size_t written=77;
    assert(musa_scene2d_prepare_bounds(n,r,local,2,prepared,2,&written));
    assert(written==2);
    assert(isfinite(prepared[0].box.min_x) &&
           isfinite(prepared[0].box.max_x) &&
           isfinite(prepared[1].box.min_y) &&
           isfinite(prepared[1].box.max_y));
    assert(prepared[0].box.max_x==DBL_MAX &&
           prepared[1].box.min_x==-DBL_MAX);
    MusaScene2dIndexNode tree[4];
    size_t leaves=0;
    assert(musa_scene2d_index_build(prepared,2,tree,4,&leaves));
    assert(leaves==2);
    const MusaScene2dBox views[] = {
        {DBL_MAX,DBL_MAX,DBL_MAX,DBL_MAX},
        {-DBL_MAX,-DBL_MAX,-DBL_MAX,-DBL_MAX}
    };
    for (size_t k=0;k<2;++k) {
        size_t direct[2]={99,99}, cached[2]={98,98}, indexed[2]={97,97};
        size_t nd=77,nc=77,ni=77;
        assert(musa_scene2d_collect_visible(n,r,local,2,views[k],direct,2,&nd));
        assert(musa_scene2d_collect_prepared(prepared,2,views[k],cached,2,&nc));
        assert(musa_scene2d_index_query(tree,2,leaves,views[k],indexed,2,&ni));
        assert(nd==1 && nc==1 && ni==1);
        assert(direct[0]==k && cached[0]==k && indexed[0]==k);
    }
    /* Invalid externally supplied bounds fail consistently and atomically. */
    prepared[0].box.max_x=INFINITY;
    size_t hits[2]={99,99};
    written=77;
    assert(!musa_scene2d_collect_prepared(prepared,2,views[0],hits,2,&written));
    assert(written==77 && hits[0]==99);
    leaves=55;
    assert(!musa_scene2d_index_build(prepared,2,tree,4,&leaves));
    assert(leaves==55);
}
static void test_large_scene(void) {
    const size_t count = 20000;
    MusaScene2dNode *n = calloc(count,sizeof *n);
    MusaScene2dResolved *r = calloc(count,sizeof *r);
    MusaScene2dBox *b = calloc(count,sizeof *b);
    MusaScene2dPreparedBounds *cache = calloc(count,sizeof *cache);
    size_t *indices = calloc(count,sizeof *indices);
    assert(n && r && b && cache && indices);
    for (size_t i=0;i<count;++i) {
        n[i].handle = (uint64_t)i+1;
        n[i].kind = MUSA_SCENE2D_ENTITY;
        r[i].handle = n[i].handle;
        r[i].visible = 1;
        r[i].world[0] = 1; r[i].world[3] = 1;
        r[i].world[4] = (double)(i*10);
        b[i] = (MusaScene2dBox){0,0,1,1};
    }
    size_t written = 0;
    assert(musa_scene2d_prepare_bounds(n,r,b,count,cache,count,&written));
    assert(written == count);
    assert(musa_scene2d_collect_prepared(cache,count,
             (MusaScene2dBox){1000,0,1021,1},indices,count,&written));
    assert(written == 3 && indices[0] == 100 &&
           indices[1] == 101 && indices[2] == 102);
    free(indices);free(cache);free(b);free(r);free(n);
}
int main(void) {
    test_cache_equivalence();
    test_atomic_failures();
    test_extreme_coordinate_consistency();
    test_large_scene();
    return 0;
}
