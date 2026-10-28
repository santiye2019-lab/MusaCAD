#include "mc2_render_plan.h"
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
int mc2_render_plan_build(const Mc2SceneItem *items,size_t count,
                          const Mc2Affine2 *local_transforms,
                          const Mc2Layer *layers,size_t layer_count,
                          Mc2Bounds2 viewport,Mc2VisibleItem *out,
                          size_t capacity,size_t *out_count) {
    if (!out_count || (count && (!items || !local_transforms)) ||
        (capacity && !out) ||
        count>SIZE_MAX/sizeof(Mc2BlockTransformNode) ||
        count>SIZE_MAX/sizeof(Mc2Affine2) ||
        capacity>SIZE_MAX/sizeof(Mc2VisibleItem)) return 0;
    Mc2BlockTransformNode *nodes=count?malloc(count*sizeof(*nodes)):NULL;
    Mc2Affine2 *world=count?malloc(count*sizeof(*world)):NULL;
    Mc2VisibleItem *temporary=capacity?malloc(capacity*sizeof(*temporary)):NULL;
    if ((count && (!nodes || !world)) || (capacity && !temporary)) {
        free(nodes);free(world);free(temporary);return 0;
    }
    for (size_t i=0;i<count;++i) {
        nodes[i].ownership.handle=items[i].entity.handle;
        nodes[i].ownership.parent_handle=items[i].parent_handle;
        nodes[i].ownership.visible=items[i].own_visible;
        nodes[i].local_transform=local_transforms[i];
    }
    int ok=1;
    for (size_t i=0;i<count;++i) {
        if (!mc2_block_world_transform(nodes,count,nodes[i].ownership.handle,&world[i])) {
            ok=0;break;
        }
    }
    size_t produced=0;
    if (ok) ok=mc2_viewport_collect(items,count,world,layers,layer_count,
                                   viewport,temporary,capacity,&produced);
    if (ok) {
        for (size_t i=0;i<produced;++i) out[i]=temporary[i];
        *out_count=produced;
    }
    free(nodes);free(world);free(temporary);
    return ok;
}
