/* SPDX-License-Identifier: MIT
 * MusaCAD-owned post-decode INSERT visibility, scratch-backed.
 */
#ifndef MUSA_DWG1032_INSERT_SCRATCH_H
#define MUSA_DWG1032_INSERT_SCRATCH_H
#include "musa_dwg1032_viewport_visibility.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Resolve one authenticated INSERT's children in a selected viewport.
 * The caller supplies definition-owner visibility (NOT precombined layer
 * visibility). A child on layer zero inherits the already-validated INSERT
 * visibility; other children require their own on/thawed/viewport state.
 * Layer and override tables must be sorted, unique, and fully verified.
 * The entire output is unchanged on failure. Scratch may change on failure.
 * Scratch capacity includes all bytes checked for aliasing. No allocation.
 * Does not parse DWG, authenticate owner handles or authorize rendering.
 * O(count * (log(layer_count) + log(override_count))) time. */
int musa_dwg1032_insert_visibility_scratch(
    const uint64_t *child_layers, const uint8_t *owner_visible,
    size_t count, uint64_t zero_layer, uint8_t insert_visible,
    const MusaDwg1032LayerState *layers, size_t layer_count,
    const MusaDwg1032ViewportFreeze *overrides, size_t override_count,
    uint8_t *result, uint8_t *scratch, size_t scratch_capacity);
#ifdef __cplusplus
}
#endif
#endif
