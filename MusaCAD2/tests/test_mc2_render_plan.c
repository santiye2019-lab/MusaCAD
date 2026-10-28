#include "mc2_render_plan.h"
#include <assert.h>
#include <stdlib.h>
int main(void) {
    Mc2SceneItem items[2]={0};
    Mc2Affine2 local[2]={ {1,0,0,1,10,0},{1,0,0,1,2,0} };
    Mc2Layer layers[]={{0,0xaabbcc,1,0}};
    Mc2VisibleItem output[2];
    size_t count=777;
    for (int i=0;i<2;++i) {
        items[i].entity.handle=(uint64_t)(i+1);
        items[i].entity.kind=MC2_ENTITY_LINE;
        items[i].entity.geometry.line.end.x=1;
        items[i].color.mode=MC2_COLOR_BY_LAYER;
        items[i].own_visible=1;
    }
    items[1].parent_handle=1;
    assert(mc2_render_plan_build(items,2,local,layers,1,
           (Mc2Bounds2){11.5,-1,14,1},output,2,&count));
    assert(count==1 && output[0].index==1);
    assert(output[0].world_bounds.min_x==12);
    items[0].own_visible=0;
    assert(mc2_render_plan_build(items,2,local,layers,1,
           (Mc2Bounds2){11.5,-1,14,1},output,2,&count));
    assert(count==0);
    items[0].own_visible=1;
    items[1].entity.handle=1;
    output[0].index=999;
    assert(!mc2_render_plan_build(items,2,local,layers,1,
           (Mc2Bounds2){11.5,-1,14,1},output,2,&count));
    assert(output[0].index==999);
    items[1].entity.handle=2;
    items[0].parent_handle=2;
    output[0].index=888;
    count=777;
    assert(!mc2_render_plan_build(items,2,local,layers,1,
           (Mc2Bounds2){11.5,-1,14,1},output,2,&count));
    assert(count==777 && output[0].index==888);
    /* End-to-end stress: nested world transforms and viewport culling
     * must resolve a deep hierarchy without per-entity graph scans. */
    const size_t n=2048;
    Mc2SceneItem *many=calloc(n,sizeof(*many));
    Mc2Affine2 *local_many=calloc(n,sizeof(*local_many));
    assert(many && local_many);
    for (size_t i=0;i<n;++i) {
        many[i].entity.handle=(uint64_t)(i+1);
        many[i].entity.kind=MC2_ENTITY_LINE;
        many[i].entity.geometry.line.end.x=1;
        many[i].parent_handle=i?(uint64_t)i:0;
        many[i].own_visible=1;
        many[i].color.mode=MC2_COLOR_BY_LAYER;
        local_many[i]=mc2_transform_identity();
        local_many[i].tx=1;
    }
    Mc2VisibleItem one;
    count=777;
    assert(mc2_render_plan_build(many,n,local_many,layers,1,
           (Mc2Bounds2){2048.25,-1,2048.5,1},&one,1,&count));
    assert(count==1 && one.index==n-1);
    many[1000].own_visible=0;
    assert(mc2_render_plan_build(many,n,local_many,layers,1,
           (Mc2Bounds2){2048.25,-1,2048.5,1},&one,1,&count));
    assert(count==0);
    free(local_many);
    free(many);
    return 0;
}
