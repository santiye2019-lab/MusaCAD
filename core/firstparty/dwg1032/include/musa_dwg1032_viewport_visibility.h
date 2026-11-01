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
/* Compose verified viewport-local visibility with the validated owner
 * forest. Every indexed object must have a caller-decoded layer and owner.
 * A hidden ancestor remains hidden even when a child's viewport layer is
 * thawed. Per-INSERT layer-zero inheritance must still be evaluated for
 * each instance; this function does not authorize final rendering.
 * All buffers (including full work capacity) must be pairwise disjoint.
 * local_work and work are scratch; effective_visible remains unchanged
 * on failure. O(n log n + n log layers + n log overrides), no heap. */
int musa_dwg1032_viewport_owner_visibility(
    const MusaDwg1032ObjectIndex *index,
    const uint64_t *owner_handles,
    const uint64_t *entity_layer_handles,
    const uint8_t *entity_visible,
    const MusaDwg1032LayerState *layers, size_t layer_count,
    const MusaDwg1032ViewportFreeze *overrides, size_t override_count,
    MusaDwg1032OwnerWork *work, size_t work_count,
    uint8_t *local_work, uint8_t *effective_visible);

/* Resolve direct children of ONE concrete INSERT in ONE viewport.
 * The definition's layer-zero child ignores the definition-time layer-zero
 * on/thawed/frozen state and inherits the already-proven INSERT visibility;
 * every other child uses its own validated layer and viewport freeze.
 * definition_owner_visible is owner-only evidence (strict 0/1), not a
 * precomposed global-layer visibility flag. Call once per INSERT instance,
 * including nested inserts after their parent context has been resolved.
 * All handles and layer/override tables must be authenticated by the caller;
 * this function does not decode DWG, calculate instance ownership, or grant
 * final render permission. On failure output is unchanged. No heap, O(n log
 * layers + n log overrides), all result/source ranges disjoint. Empty
 * instances accept NULL child arrays but still validate layer metadata. */
int musa_dwg1032_viewport_insert_visibility(
    const uint64_t *source_layer_handles,
    const uint8_t *definition_owner_visible, size_t count,
    uint64_t layer_zero_handle, uint8_t insert_visible,
    const MusaDwg1032LayerState *layers, size_t layer_count,
    const MusaDwg1032ViewportFreeze *overrides, size_t override_count,
    uint8_t *effective_visible);

/* Caller-scratch variant for large, already-decoded 2D drawings. Resolve
 * each entity layer once, write only to scratch while validating, and copy
 * the complete mask after success. Scratch may change on failure; the
 * published local_visible mask never does. Scratch capacity (including
 * unused bytes) and output must be disjoint from each other and all inputs.
 * No allocations, no DWG byte parsing, no final render permission.
 * O(n log layers + n log overrides), plus O(n) final publication. */
int musa_dwg1032_viewport_local_visibility_scratch(
    const uint64_t *entity_layer_handles,
    const uint8_t *entity_visible, size_t entity_count,
    const MusaDwg1032LayerState *layers, size_t layer_count,
    const MusaDwg1032ViewportFreeze *overrides, size_t override_count,
    uint8_t *local_visible, uint8_t *scratch, size_t scratch_capacity);
#ifdef __cplusplus
}
#endif
#endif
