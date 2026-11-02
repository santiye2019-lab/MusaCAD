/* SPDX-License-Identifier: MIT */
#include "musa_scene2d_viewport.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>

#define IDENTITY {1,0,0,1,0,0}
static const MusaScene2dLayer layers[] = {
    {0, 0xffffffffu, 1},
    {10, 0xffff0000u, 1},
    {20, 0xff0000ffu, 1},
    {30, 0xffffff00u, 0}
};
static const MusaScene2dNode nodes[] = {
    {100,0,10,0,MUSA_SCENE2D_INSERT,1,MUSA_SCENE2D_BYLAYER,{1,0,0,1,10,20}},
    {200,100,0,0,MUSA_SCENE2D_INSERT,1,MUSA_SCENE2D_BYBLOCK,{2,0,0,2,3,4}},
    {300,200,0,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYBLOCK,{1,0,0,1,1,2}},
    {400,100,20,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYLAYER,IDENTITY},
    {500,0,0,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYLAYER,IDENTITY},
    {600,200,20,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYLAYER,IDENTITY},
    {700,0,30,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYLAYER,IDENTITY}
};
static void test_viewport_inheritance(void) {
    const uint8_t all[] = {1,1,1,1};
    const uint8_t freeze_insert[] = {1,0,1,1};
    const uint8_t freeze_child[] = {1,1,0,1};
    const uint8_t freeze_zero[] = {0,1,1,1};
    MusaScene2dResolved out[7];
    size_t written = 77;
    assert(musa_scene2d_resolve_viewport(nodes,7,layers,4,all,out,7,&written));
    assert(written == 7);
    /* The allocation-free all-visible viewport must exactly match
     * ordinary scene resolution, including nested block transforms. */
    MusaScene2dResolved plain[7];
    size_t plain_written = 0;
    assert(musa_scene2d_resolve(nodes,7,layers,4,plain,7,&plain_written));
    assert(plain_written == written);
    for (size_t i=0;i<7u;++i) {
        assert(out[i].handle == plain[i].handle);
        assert(out[i].effective_layer == plain[i].effective_layer);
        assert(out[i].rgba == plain[i].rgba);
        assert(out[i].visible == plain[i].visible);
        for (size_t j=0;j<6u;++j)
            assert(out[i].world[j] == plain[i].world[j]);
    }
    assert(out[2].visible && out[2].effective_layer == 10);
    assert(out[2].rgba == 0xffff0000u);
    assert(out[2].world[4] == 15 && out[2].world[5] == 28);
    assert(out[3].visible && out[3].rgba == 0xff0000ffu);
    assert(out[4].visible && !out[6].visible);
    assert(musa_scene2d_resolve_viewport(nodes,7,layers,4,
                                         freeze_insert,out,7,&written));
    assert(!out[0].visible && !out[1].visible && !out[2].visible);
    assert(!out[3].visible && !out[5].visible);
    assert(out[4].visible);
    assert(musa_scene2d_resolve_viewport(nodes,7,layers,4,
                                         freeze_child,out,7,&written));
    assert(out[0].visible && out[1].visible && out[2].visible);
    assert(!out[3].visible && !out[5].visible && out[4].visible);
    assert(musa_scene2d_resolve_viewport(nodes,7,layers,4,
                                         freeze_zero,out,7,&written));
    assert(out[0].visible && out[1].visible && out[2].visible);
    assert(!out[4].visible && out[3].visible);
    /* Caller-owned global layer flags must not change between viewports. */
    assert(layers[1].visible == 1 && layers[2].visible == 1);
}
static void test_rejection_is_atomic(void) {
    uint8_t mask[] = {1,1,1,1};
    MusaScene2dResolved out[7];
    MusaScene2dResolved saved[7];
    memset(out,0xa5,sizeof out);
    memcpy(saved,out,sizeof out);
    size_t written = 77;
    mask[2] = 2;
    assert(!musa_scene2d_resolve_viewport(nodes,7,layers,4,
                                          mask,out,7,&written));
    assert(written == 77 && memcmp(out,saved,sizeof out) == 0);
    mask[2] = 1;
    MusaScene2dLayer invalid_layers[4];
    memcpy(invalid_layers,layers,sizeof invalid_layers);
    invalid_layers[2].visible = 2;
    mask[2] = 0; /* A frozen viewport must not mask malformed input. */
    assert(!musa_scene2d_resolve_viewport(nodes,7,invalid_layers,4,
                                          mask,out,7,&written));
    assert(written == 77 && memcmp(out,saved,sizeof out) == 0);
    mask[2] = 1;
    assert(!musa_scene2d_resolve_viewport(nodes,7,layers,4,
                                          mask,out,6,&written));
    assert(written == 77 && memcmp(out,saved,sizeof out) == 0);
    assert(!musa_scene2d_resolve_viewport(nodes,7,layers,4,
                                          (const uint8_t *)(const void *)out,
                                          out,7,&written));
    assert(written == 77 && memcmp(out,saved,sizeof out) == 0);
    assert(!musa_scene2d_resolve_viewport(nodes,7,layers,4,
                                          mask,out,7,
                                          (size_t *)(void *)mask));
    assert(memcmp(out,saved,sizeof out) == 0);
    assert(!musa_scene2d_resolve_viewport(nodes,7,layers,4,
                                          mask,(MusaScene2dResolved *)(void *)layers,
                                          7,&written));
    assert(written == 77);
    assert(!musa_scene2d_resolve_viewport(nodes,7,layers,4,
                                          mask,out,7,
                                          (size_t *)(void *)layers));
    assert(written == 77 && memcmp(out,saved,sizeof out) == 0);
    assert(musa_scene2d_resolve_viewport(NULL,0,layers,4,
                                         mask,NULL,0,&written));
    assert(written == 0);
    /* Empty drawings must also accept a frozen viewport without needing
     * a disposable copy of the layer table. */
    mask[1]=0;
    written=77;
    assert(musa_scene2d_resolve_viewport(NULL,0,layers,4,
                                         mask,NULL,0,&written));
    assert(written == 0);
}
static void test_redundant_global_freezes(void) {
    /* Layer 30 is globally hidden. A viewport freeze of only that layer
     * must be observationally identical to the all-visible mask. */
    const uint8_t all[] = {1,1,1,1};
    const uint8_t redundant[] = {1,1,1,0};
    const uint8_t real_freeze[] = {1,0,1,1};
    const uint8_t mixed_freeze[] = {1,0,1,0};
    MusaScene2dResolved baseline[7], redundant_out[7];
    MusaScene2dResolved frozen[7], mixed[7];
    size_t count=99, other=99;
    assert(musa_scene2d_resolve_viewport(
        nodes,7,layers,4,all,baseline,7,&count));
    assert(count==7);
    assert(musa_scene2d_resolve_viewport(
        nodes,7,layers,4,redundant,redundant_out,7,&other));
    assert(other==count);
    for (size_t i=0;i<count;++i) {
        assert(redundant_out[i].handle==baseline[i].handle);
        assert(redundant_out[i].effective_layer==baseline[i].effective_layer);
        assert(redundant_out[i].rgba==baseline[i].rgba);
        assert(redundant_out[i].visible==baseline[i].visible);
        for (size_t j=0;j<6u;++j)
            assert(redundant_out[i].world[j]==baseline[i].world[j]);
    }
    assert(musa_scene2d_resolve_viewport(
        nodes,7,layers,4,real_freeze,frozen,7,&count));
    assert(musa_scene2d_resolve_viewport(
        nodes,7,layers,4,mixed_freeze,mixed,7,&other));
    assert(count==other && !frozen[0].visible && !mixed[0].visible);
    for (size_t i=0;i<count;++i) {
        assert(frozen[i].handle==mixed[i].handle);
        assert(frozen[i].effective_layer==mixed[i].effective_layer);
        assert(frozen[i].rgba==mixed[i].rgba);
        assert(frozen[i].visible==mixed[i].visible);
        for (size_t j=0;j<6u;++j)
            assert(frozen[i].world[j]==mixed[i].world[j]);
    }
}
int main(void) {
    test_viewport_inheritance();
    test_redundant_global_freezes();
    test_rejection_is_atomic();
    return 0;
}
