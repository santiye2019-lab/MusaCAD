/* SPDX-License-Identifier: MIT
 * First-party transactional per-INSERT viewport attributes, post decode.
 * No AC1032 entity/section parsing and no third-party CAD dependencies.
 */
#ifndef MUSA_DWG1032_INSTANCE_ATTRIBUTES_H
#define MUSA_DWG1032_INSTANCE_ATTRIBUTES_H
#include "musa_dwg1032_viewport_visibility.h"
#include "musa_dwg1032_layer_color.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Compose one CONCRETE, authenticated INSERT occurrence in one viewport.
 * The caller supplies a nonempty block child list, original child layer
 * handles/source colors, previously resolved definition colors, owner-only
 * visibility flags and independently verified layer/viewport metadata.
 * Zero-layer children inherit this INSERT's verified layer color and
 * visibility, while nonzero-layer children obey their own layer/viewport
 * freeze and source color. ByBlock children inherit insert_color.
 *
 * insert_color and insert_layer_color MUST be fully resolved, not ByBlock:
 * this is a concrete instance boundary, never a block definition cache.
 * Call again for every INSERT instance including nested occurrences; a
 * hidden ancestor must already be reflected in insert_visible.
 *
 * A color failure OR viewport/owner failure rejects the WHOLE operation:
 * neither published output changes. Scratch may change. No allocations,
 * no DWG byte decoding, no graphics/preview admission. The complete
 * advertised scratch capacities and all inputs/outputs must be disjoint
 * wherever an output could be written.
 * O(count log layer_count + count log override_count).
 */
int musa_dwg1032_instance_attributes(
    const MusaDwg1032ResolvedColor *definition_colors,
    const MusaDwg1032CadColor *source_colors,
    const uint64_t *source_layer_handles,
    const uint8_t *definition_owner_visible, size_t count,
    uint64_t layer_zero_handle, uint8_t insert_visible,
    const MusaDwg1032ResolvedColor *insert_color,
    const MusaDwg1032ResolvedColor *insert_layer_color,
    const MusaDwg1032LayerState *layers, size_t layer_count,
    const MusaDwg1032ViewportFreeze *overrides, size_t override_count,
    MusaDwg1032ResolvedColor *colors_out, uint8_t *visible_out,
    MusaDwg1032ResolvedColor *colors_scratch, size_t color_scratch_capacity,
    uint8_t *visibility_scratch, size_t visibility_scratch_capacity);

#ifdef __cplusplus
}
#endif
#endif
