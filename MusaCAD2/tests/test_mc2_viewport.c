#include "mc2_viewport.h"
#include <assert.h>
int main(void) {
    Mc2SceneItem items[3]={0};
    Mc2Affine2 transforms[3];
    Mc2Layer layers[]={{0,0x112233,1,0}};
    Mc2VisibleItem out[3];
    size_t count=99;
    for (int i=0;i<3;++i) {
        items[i].entity.handle=(uint64_t)(i+1);
        items[i].entity.kind=MC2_ENTITY_LINE;
        items[i].entity.geometry.line.end.x=1;
        items[i].own_visible=1;
        items[i].color.mode=MC2_COLOR_BY_LAYER;
        transforms[i]=mc2_transform_identity();
    }
    transforms[1].tx=100;
    items[2].parent_handle=1;
    items[2].own_visible=0;
    assert(mc2_viewport_collect(items,3,transforms,layers,1,
           (Mc2Bounds2){-1,-1,2,2},out,3,&count));
    assert(count==1 && out[0].index==0 && out[0].color_rgb==0x112233);
    assert(!mc2_viewport_collect(items,3,transforms,layers,1,
           (Mc2Bounds2){-1,-1,2,2},out,0,&count));
    items[2].parent_handle=99;
    assert(!mc2_viewport_collect(items,3,transforms,layers,1,
           (Mc2Bounds2){-1,-1,2,2},out,3,&count));
    return 0;
}
