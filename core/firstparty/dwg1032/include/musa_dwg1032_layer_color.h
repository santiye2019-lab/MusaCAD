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
#ifdef __cplusplus
}
#endif
#endif
