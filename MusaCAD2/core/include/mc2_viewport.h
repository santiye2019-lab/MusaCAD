#ifndef MC2_VIEWPORT_H
#define MC2_VIEWPORT_H
#include "mc2_scene.h"
#include "mc2_spatial.h"
typedef struct {
    size_t index;
    Mc2Bounds2 world_bounds;
    uint32_t color_rgb;
} Mc2VisibleItem;
/* Fail closed on malformed ownership, appearance, or transforms.
   Returns 1 only if the entire input was validated and output capacity suffices. */
int mc2_viewport_collect(const Mc2SceneItem *items,size_t count,
                         const Mc2Affine2 *world_transforms,
                         const Mc2Layer *layers,size_t layer_count,
                         Mc2Bounds2 viewport,
                         Mc2VisibleItem *out,size_t capacity,size_t *out_count);
#endif
