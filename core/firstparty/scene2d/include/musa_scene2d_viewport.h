/* SPDX-License-Identifier: MIT
 * Independently authored viewport-specific 2D layer visibility.
 */
#ifndef MUSA_SCENE2D_VIEWPORT_H
#define MUSA_SCENE2D_VIEWPORT_H
#include "musa_scene2d.h"
#ifdef __cplusplus
extern "C" {
#endif
/* viewport_layer_visible is indexed exactly like layers: 0 means frozen
 * in this viewport; 1 means eligible for display. Global layer visibility
 * remains authoritative. An INSERT frozen in the viewport hides its entire
 * descendant graph, including children assigned to other layers.
 * Layer-zero inheritance, BYBLOCK/BYLAYER colors and affine transforms
 * follow musa_scene2d_resolve without changing the original layer table.
 * This consumes already decoded entities, not DWG bytes. All failures
 * preserve out and *written; output may not alias the visibility mask.
 */
int musa_scene2d_resolve_viewport(
    const MusaScene2dNode *nodes, size_t node_count,
    const MusaScene2dLayer *layers, size_t layer_count,
    const uint8_t *viewport_layer_visible,
    MusaScene2dResolved *out, size_t capacity, size_t *written);
#ifdef __cplusplus
}
#endif
#endif
