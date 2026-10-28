#include "mc2_viewport.h"
#include <assert.h>
#include <stdlib.h>
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
    out[0].index=12345;
    items[2].parent_handle=99;
    assert(!mc2_viewport_collect(items,3,transforms,layers,1,
           (Mc2Bounds2){-1,-1,2,2},out,3,&count));
    assert(out[0].index==12345);
    const size_t n=2048;
    Mc2SceneItem *many=calloc(n,sizeof(*many));
    Mc2Affine2 *world=calloc(n,sizeof(*world));
    assert(many && world);
    for (size_t i=0;i<n;++i) {
        many[i].entity.handle=(uint64_t)(i+1);
        many[i].entity.kind=MC2_ENTITY_LINE;
        many[i].entity.geometry.line.end.x=1;
        many[i].color.mode=MC2_COLOR_BY_LAYER;
        many[i].own_visible=1;
        many[i].parent_handle=i?(uint64_t)i:0;
        world[i]=mc2_transform_identity();
        world[i].tx=(double)i;
    }
    Mc2VisibleItem only;
    count=999;
    assert(mc2_viewport_collect(many,n,world,layers,1,
           (Mc2Bounds2){2047.5,-1,2047.75,1},&only,1,&count));
    assert(count==1 && only.index==n-1);
    many[1000].own_visible=0;
    assert(mc2_viewport_collect(many,n,world,layers,1,
           (Mc2Bounds2){2047.5,-1,2047.75,1},&only,1,&count));
    assert(count==0);
    many[1000].own_visible=1;
    count=777;
    only.index=9999;
    assert(!mc2_viewport_collect(many,n,world,layers,1,
           (Mc2Bounds2){2047.5,-1,2047.75,1},&only,0,&count));
    assert(count==777 && only.index==9999);
    free(world);
    free(many);
    return 0;
}
