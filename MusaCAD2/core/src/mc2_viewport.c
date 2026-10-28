#include "mc2_viewport.h"
#include <stddef.h>
#include <math.h>
#include <stdint.h>
int mc2_viewport_collect(const Mc2SceneItem *items,size_t count,
                         const Mc2Affine2 *world_transforms,
                         const Mc2Layer *layers,size_t layer_count,
                         Mc2Bounds2 viewport,
                         Mc2VisibleItem *out,size_t capacity,size_t *out_count) {
    if (!out_count || (count && (!items || !world_transforms)) ||
        (capacity && !out) || !isfinite(viewport.min_x) || !isfinite(viewport.max_x) ||
        !isfinite(viewport.min_y) || !isfinite(viewport.max_y) ||
        viewport.min_x>viewport.max_x || viewport.min_y>viewport.max_y) return 0;
    /* Two-pass validation prevents partially populated output on failure. */
    size_t produced=0;
    for (size_t i=0;i<count;++i) {
        Mc2EntityAppearance appearance;
        Mc2Bounds2 bounds;
        if (!mc2_scene_resolve(items,count,i,layers,layer_count,0xffffff,&appearance) ||
            !mc2_entity_world_bounds(&items[i].entity,&world_transforms[i],&bounds)) return 0;
        if (!appearance.visible || !mc2_bounds_intersects(&bounds,&viewport)) continue;
        ++produced;
    }
    if (produced>capacity) return 0;
    size_t written=0;
    for (size_t i=0;i<count;++i) {
        Mc2EntityAppearance appearance;
        Mc2Bounds2 bounds;
        if (!mc2_scene_resolve(items,count,i,layers,layer_count,0xffffff,&appearance) ||
            !mc2_entity_world_bounds(&items[i].entity,&world_transforms[i],&bounds)) return 0;
        if (!appearance.visible || !mc2_bounds_intersects(&bounds,&viewport)) continue;
        out[written].index=i;
        out[written].world_bounds=bounds;
        out[written].color_rgb=appearance.effective_rgb;
        ++written;
    }
    *out_count=written;
    return 1;
}
