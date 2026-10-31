/* SPDX-License-Identifier: MIT
 * First-party post-decode CAD color provenance. No DWG bytes are parsed.
 */
#ifndef MUSA_DWG1032_LAYER_COLOR_H
#define MUSA_DWG1032_LAYER_COLOR_H
#include "musa_dwg1032_object_index.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct MusaDwg1032CadColor {
    uint16_t aci;             /* 0 ByBlock, 1..255 ACI, 256 ByLayer. */
    uint8_t has_truecolor;    /* Strict Boolean; RGB overrides ACI. */
    uint32_t truecolor_rgb;   /* 0x00RRGGBB if has_truecolor is set. */
} MusaDwg1032CadColor;

typedef struct MusaDwg1032LayerColor {
    uint64_t handle;          /* Sorted, unique, nonzero layer handle. */
    MusaDwg1032CadColor color;/* Must be an explicit ACI or truecolor. */
} MusaDwg1032LayerColor;

typedef struct MusaDwg1032ResolvedColor {
    MusaDwg1032CadColor color;/* Explicit color unless byblock is set. */
    uint8_t byblock;          /* 1: unresolved until INSERT context. */
} MusaDwg1032ResolvedColor;

/* Resolve caller-verified, predecoded entity colors against a sorted layer
 * table. ACI 256 inherits the layer's explicit ACI/truecolor; ACI 0 remains
 * ByBlock, because a block definition's owner is NOT its INSERT instance.
 * The caller must resolve ByBlock separately for each actual INSERT context.
 * Every entity must refer to an existing nonzero layer. No palette, color
 * book, transparency, visibility, INSERT or DWG record decoding is done.
 * Requires a validated, unmodified object index. All output memory must be
 * disjoint from every input, including the index and its entries. No heap.
 * O(n log layer_count), with an extra preflight pass for atomic rollback.
 * On failure the ENTIRE output array is unchanged. */
int musa_dwg1032_layer_color_select(
    const MusaDwg1032ObjectIndex *index,
    const uint64_t *entity_layer_handles,
    const MusaDwg1032CadColor *entity_colors,
    const MusaDwg1032LayerColor *layers, size_t layer_count,
    MusaDwg1032ResolvedColor *resolved);
/* Apply a caller-verified INSERT instance color to colors already resolved
 * from a block definition. Explicit definition colors remain unchanged;
 * ByBlock entries inherit the instance color. A ByBlock instance color
 * remains unresolved for an enclosing INSERT (nested block context).
 * The caller MUST resolve each INSERT instance independently: never cache
 * these results against the shared block definition handle alone.
 * This post-decode operation does not handle layer-0 inheritance, visibility,
 * transparency, entity parsing, or DWG record decoding.
 * Only explicit ACI/truecolor or canonical unresolved ByBlock colors are
 * accepted. Output must not overlap either input. On failure the complete
 * output array is unchanged. O(count), no allocation. */
int musa_dwg1032_layer_color_apply_insert(
    const MusaDwg1032ResolvedColor *definition_colors, size_t count,
    const MusaDwg1032ResolvedColor *insert_color,
    MusaDwg1032ResolvedColor *resolved);
/* Apply an INSERT's verified layer context to children from a block definition.
 * A ByLayer child on the definition's layer 0 inherits the actual INSERT
 * instance layer color, not the layer-0 color stored in the definition.
 * ByBlock children inherit the INSERT entity color. Explicit colors retain
 * their definition color; nonzero-layer ByLayer colors are unchanged.
 * source_colors and source_layer_handles are original, already decoded data;
 * definition_colors must agree with explicit/ByBlock source provenance.
 * insert_layer_color must be an explicitly resolved color for this specific
 * INSERT instance (including any enclosing layer-0 context). This function
 * does not decode DWG records, locate layer 0, or resolve nested INSERTs.
 * The output must not overlap any input. On any failure the entire
 * output is unchanged. O(count), no allocation. */
int musa_dwg1032_layer_color_apply_insert_context(
    const MusaDwg1032ResolvedColor *definition_colors,
    const MusaDwg1032CadColor *source_colors,
    const uint64_t *source_layer_handles, size_t count,
    uint64_t layer_zero_handle,
    const MusaDwg1032ResolvedColor *insert_color,
    const MusaDwg1032ResolvedColor *insert_layer_color,
    MusaDwg1032ResolvedColor *resolved);
#ifdef __cplusplus
}
#endif
#endif
