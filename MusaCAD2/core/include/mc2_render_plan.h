#ifndef MC2_RENDER_PLAN_H
#define MC2_RENDER_PLAN_H
#include "mc2_viewport.h"
#include "mc2_block_transform.h"
/* Builds all world transforms from owner hierarchy, then collects visible
   entities. All caller outputs remain unchanged when validation fails. */
int mc2_render_plan_build(const Mc2SceneItem *items,size_t count,
                          const Mc2Affine2 *local_transforms,
                          const Mc2Layer *layers,size_t layer_count,
                          Mc2Bounds2 viewport,Mc2VisibleItem *out,
                          size_t capacity,size_t *out_count);
#endif
