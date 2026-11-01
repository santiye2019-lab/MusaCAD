/* SPDX-License-Identifier: MIT
 * Independently authored, post-decode viewport layer visibility.
 */
#ifndef MUSA_DWG1032_VIEWPORT_VISIBILITY_H
#define MUSA_DWG1032_VIEWPORT_VISIBILITY_H
#include "musa_dwg1032_owner_graph.h"
#ifdef __cplusplus
extern "C" {
#endif
/* A caller-authenticated viewport override, sorted by layer handle.
 * Missing overrides mean NOT frozen in this viewport, never that the
 * base layer is on or thawed. This is not a DWG viewport decoder. */
typedef struct MusaDwg1032ViewportFreeze {
    uint64_t layer_handle;
    uint8_t frozen;
} MusaDwg1032ViewportFreeze;

/* Combine caller-verified entity flags, sorted layer states and optional
 * viewport-specific freeze overrides. Every entity layer must exist.
 * The result is a LOCAL visibility mask; it must still be combined with
 * authenticated owner/INSERT context before drawing. No final rendering
 * permission, viewport parsing or compressed DWG reading is inferred.
 * All arrays must be disjoint from the output. On failure the ENTIRE
 * output is unchanged. O(n log layers + n log overrides), no heap. */
int musa_dwg1032_viewport_local_visibility(
    const uint64_t *entity_layer_handles,
    const uint8_t *entity_visible, size_t entity_count,
    const MusaDwg1032LayerState *layers, size_t layer_count,
    const MusaDwg1032ViewportFreeze *overrides, size_t override_count,
    uint8_t *local_visible);
#ifdef __cplusplus
}
#endif
#endif
