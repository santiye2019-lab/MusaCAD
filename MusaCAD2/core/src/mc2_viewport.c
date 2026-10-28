#include "mc2_viewport.h"
#include <stddef.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
int mc2_viewport_collect(const Mc2SceneItem *items,size_t count,
                         const Mc2Affine2 *world_transforms,
                         const Mc2Layer *layers,size_t layer_count,
                         Mc2Bounds2 viewport,
                         Mc2VisibleItem *out,size_t capacity,size_t *out_count) {
    if (!out_count || (count && (!items || !world_transforms)) ||
        (capacity && !out) || !isfinite(viewport.min_x) || !isfinite(viewport.max_x) ||
        !isfinite(viewport.min_y) || !isfinite(viewport.max_y) ||
        viewport.min_x>viewport.max_x || viewport.min_y>viewport.max_y ||
        count>SIZE_MAX/sizeof(Mc2EntityAppearance) ||
        count>SIZE_MAX/sizeof(Mc2Bounds2)) return 0;
    Mc2EntityAppearance *appearances=count?
        (Mc2EntityAppearance *)malloc(count*sizeof(*appearances)):NULL;
    Mc2Bounds2 *bounds=count?
        (Mc2Bounds2 *)malloc(count*sizeof(*bounds)):NULL;
    int ok=0;
    if (count && (!appearances || !bounds)) goto cleanup;
    /* Resolve ownership and colors once; never rerun a full scene traversal
     * for each candidate entity. Keep outputs untouched until all validation
     * and capacity checks have succeeded. */
    if (!mc2_scene_resolve_all(items,count,layers,layer_count,0xffffff,appearances))
        goto cleanup;
    size_t produced=0;
    for (size_t i=0;i<count;++i) {
        if (!mc2_entity_world_bounds(&items[i].entity,&world_transforms[i],
                                    &bounds[i])) goto cleanup;
        if (appearances[i].visible && mc2_bounds_intersects(&bounds[i],&viewport))
            ++produced;
    }
    if (produced>capacity) goto cleanup;
    size_t written=0;
    for (size_t i=0;i<count;++i) {
        if (!appearances[i].visible || !mc2_bounds_intersects(&bounds[i],&viewport))
            continue;
        out[written].index=i;
        out[written].world_bounds=bounds[i];
        out[written].color_rgb=appearances[i].effective_rgb;
        ++written;
    }
    *out_count=written;
    ok=1;
cleanup:
    free(bounds);
    free(appearances);
    return ok;
}
