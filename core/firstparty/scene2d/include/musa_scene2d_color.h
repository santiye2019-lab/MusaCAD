/* SPDX-License-Identifier: MIT
 * Independent adapter for predecoded CAD ACI and true-color attributes.
 */
#ifndef MUSA_SCENE2D_COLOR_H
#define MUSA_SCENE2D_COLOR_H
#include "musa_scene2d.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaScene2dCadColor {
    uint16_t aci;           /* 0 BYBLOCK, 1..255 indexed, 256 BYLAYER. */
    uint8_t has_truecolor;  /* Exactly 0 or 1; RGB overrides ACI. */
    uint32_t truecolor_rgb; /* 0x00RRGGBB, never an alpha channel. */
} MusaScene2dCadColor;
typedef struct MusaScene2dColorSelection {
    uint8_t mode; /* MusaScene2dColorMode */
    uint32_t rgba; /* 0xAARRGGBB, opaque; ignored for inherited modes. */
} MusaScene2dColorSelection;
/* Resolve a predecoded color to the independent scene graph's color mode.
 * For indexed colors 8..255, a verified 256-entry RGB palette is required.
 * With no palette, only canonical ACI 1..6 and foreground ACI 7 resolve.
 * ACI 7 follows the caller's foreground RGB (e.g. black on white canvas).
 * A supplied palette also determines the value of ACI 7.
 * No DWG bytes, color-book names or transparency are decoded here.
 * Invalid arguments preserve *out. Input and output must not overlap. */
int musa_scene2d_cad_color_select(const MusaScene2dCadColor *color,
                                  const uint32_t *palette_rgb,
                                  size_t palette_count,
                                  uint32_t foreground_rgb,
                                  MusaScene2dColorSelection *out);
#ifdef __cplusplus
}
#endif
#endif
