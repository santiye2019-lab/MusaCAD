/* SPDX-License-Identifier: MIT
 * End-to-end scene graph checks for predecoded layer attributes only.
 */
#include "musa_scene2d_layer.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
static void check(const MusaScene2dCadLayer *cad, uint8_t visible) {
    MusaScene2dLayer layers[2];
    const MusaScene2dCadLayer root={0,7,0,0,0,0};
    assert(musa_scene2d_layer_from_cad(&root,NULL,0,0xffffffu,&layers[0]));
    assert(musa_scene2d_layer_from_cad(cad,NULL,0,0,&layers[1]));
    const MusaScene2dNode nodes[]={
        {100,0,10,0,MUSA_SCENE2D_INSERT,1,MUSA_SCENE2D_BYLAYER,
         {1,0,0,1,3,4}},
        {101,100,0,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYBLOCK,
         {1,0,0,1,2,5}},
        {102,101,0,0,MUSA_SCENE2D_ENTITY,1,MUSA_SCENE2D_BYLAYER,
         {1,0,0,1,1,1}}
    };
    /* Entity 101 is not a valid parent INSERT: reject instead of
     * silently inheriting an invalid ownership graph. */
    MusaScene2dResolved result[3];
    size_t written=777;
    assert(!musa_scene2d_resolve(nodes,3,layers,2,result,3,&written));
    assert(written==777);
    MusaScene2dNode valid[3]={nodes[0],nodes[1],nodes[2]};
    valid[2].parent_insert=100;
    assert(musa_scene2d_resolve(valid,3,layers,2,result,3,&written));
    assert(written==3);
    assert(result[0].visible==visible);
    assert(result[1].visible==visible);
    assert(result[2].visible==visible);
    assert(result[0].rgba==0xff00ff00u);
    assert(result[1].rgba==0xff00ff00u);
    assert(result[2].rgba==0xff00ff00u);
    assert(result[1].effective_layer==10 && result[2].effective_layer==10);
    assert(result[1].world[4]==5 && result[1].world[5]==9);
}
int main(void) {
    MusaScene2dCadLayer cad={10,3,0,0,0,0};
    check(&cad,1);
    cad.signed_aci=-3;
    check(&cad,0);
    cad.signed_aci=3; cad.frozen=1;
    check(&cad,0);
    cad.frozen=0; cad.viewport_frozen=1;
    check(&cad,0);
    cad.viewport_frozen=0; cad.has_truecolor=1;
    cad.truecolor_rgb=0x00ff00u;
    check(&cad,1);
    return 0;
}
