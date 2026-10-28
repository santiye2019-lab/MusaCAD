#ifndef MC2_SCENE_H
#define MC2_SCENE_H
#include "mc2_geometry.h"
#include "mc2_layers.h"
#include "mc2_ownership.h"
#include <stddef.h>
#include <stdint.h>
typedef struct {
    Mc2Entity2 entity;
    uint64_t parent_handle;
    Mc2Color color;
    int own_visible;
} Mc2SceneItem;
/* Resolve scene appearance only after validating the full ownership graph.
   parent_block_rgb is the resolved parent insertion color supplied by caller. */
/* Resolve the entire scene in one indexed pass. Validates all ownership
 * chains before publishing output. On failure, out remains unchanged.
 * Visibility includes ancestor layer off/frozen states. */
int mc2_scene_resolve_all(const Mc2SceneItem *items, size_t count,
                          const Mc2Layer *layers, size_t layer_count,
                          uint32_t parent_block_rgb, Mc2EntityAppearance *out);
int mc2_scene_resolve(const Mc2SceneItem *items, size_t count,
                      size_t item_index, const Mc2Layer *layers, size_t layer_count,
                      uint32_t parent_block_rgb, Mc2EntityAppearance *out);
#endif
