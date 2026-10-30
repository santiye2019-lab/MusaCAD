/* SPDX-License-Identifier: MIT
 * First-party adapter for already decoded CAD layer attributes.
 */
#ifndef MUSA_SCENE2D_LAYER_H
#define MUSA_SCENE2D_LAYER_H
#include "musa_scene2d_color.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaScene2dCadLayer {
    uint64_t handle;
    int16_t signed_aci; /* -255..-1 off, 1..255 on; sign survives truecolor. */
    uint8_t frozen; /* Model/global frozen, exactly 0 or 1. */
    uint8_t viewport_frozen; /* Selected viewport, exactly 0 or 1. */
    uint8_t has_truecolor; /* Exactly 0 or 1. */
    uint32_t truecolor_rgb; /* 0x00RRGGBB if present. */
} MusaScene2dCadLayer;
/* Converts one decoded layer to a scene layer, preserving off/frozen state
 * independently from RGB. The palette, when present, has 256 RGB entries.
 * ACI 8..255 requires a vetted palette; no palette is synthesized.
 * Fails atomically; input and output must not overlap. */
int musa_scene2d_layer_from_cad(const MusaScene2dCadLayer *cad,
                                const uint32_t *palette_rgb,
                                size_t palette_count,
                                uint32_t foreground_rgb,
                                MusaScene2dLayer *out);
/* Maps caller-owned arrays with an all-or-nothing preflight. No allocation,
 * no aliasing, and no changes to outputs on any failure. */
int musa_scene2d_layers_from_cad(const MusaScene2dCadLayer *cad,
                                 size_t count,
                                 const uint32_t *palette_rgb,
                                 size_t palette_count,
                                 uint32_t foreground_rgb,
                                 MusaScene2dLayer *out, size_t capacity);
/* Convert a bounded page of caller-decoded layer attributes without
 * visiting preceding layers or allocating a whole-document output.
 * start >= count returns an empty page; capacity counts layers, not bytes.
 * Only requested layers are validated. All input/output buffers must be
 * disjoint; on failure out, *written and *total remain unchanged.
 * This is NOT a DWG layer-table reader. */
int musa_scene2d_layers_from_cad_page(const MusaScene2dCadLayer *cad,
                                      size_t count, size_t start,
                                      const uint32_t *palette_rgb,
                                      size_t palette_count,
                                      uint32_t foreground_rgb,
                                      MusaScene2dLayer *out, size_t capacity,
                                      size_t *written, size_t *total);
#ifdef __cplusplus
}
#endif
#endif
