#include "mc2_viewport.h"
#include <stddef.h>
#include <stdint.h>
int mc2_viewport_collect(const Mc2SceneItem *items,size_t count,
                         const Mc2Affine2 *world_transforms,
                         const Mc2Layer *layers,size_t layer_count,
                         Mc2Bounds2 viewport,
                         Mc2VisibleItem *out,size_t capacity,size_t *out_count) {
    if (!out_count || (count && (!items || !world_transforms)) ||
        (capacity && !out) || viewport.min_x>viewport.max_x ||
        viewport.min_y>viewport.max_y) return 0;
    size_t produced=0;
    for (size_t i=0;i<count;++i) {
        Mc2EntityAppearance appearance;
        Mc2Bounds2 bounds;
        if (!mc2_scene_resolve(items,count,i,layers,layer_count,0xffffff,&appearance) ||
            !mc2_entity_world_bounds(&items[i].entity,&world_transforms[i],&bounds)) return 0;
        if (!appearance.visible || !mc2_bounds_intersects(&bounds,&viewport)) continue;
        if (produced>=capacity) return 0;
        out[produced].index=i;
        out[produced].world_bounds=bounds;
        out[produced].color_rgb=appearance.effective_rgb;
        ++produced;
    }
    *out_count=produced;
    return 1;
}
