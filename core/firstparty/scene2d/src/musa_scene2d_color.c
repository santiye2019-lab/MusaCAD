/* SPDX-License-Identifier: MIT
 * Original, standalone ACI-to-scene color selection; no DWG parser code.
 */
#include "musa_scene2d_color.h"
#include <stdint.h>
#include <stddef.h>
static int overlaps(const void *p, size_t pn, const void *q, size_t qn) {
    if (!pn || !qn) return 0;
    const uintptr_t a=(uintptr_t)p, b=(uintptr_t)q;
    return a<=b ? b-a<pn : a-b<qn;
}
int musa_scene2d_cad_color_select(const MusaScene2dCadColor *color,
                                  const uint32_t *palette_rgb,
                                  size_t palette_count,
                                  uint32_t foreground_rgb,
                                  MusaScene2dColorSelection *out) {
    static const uint32_t basic[7]={0,0xff0000u,0xffff00u,0x00ff00u,
                                     0x00ffffu,0x0000ffu,0xff00ffu};
    if (!color || !out || color->aci>256u || color->has_truecolor>1u ||
        foreground_rgb>0xffffffu ||
        ((palette_rgb==NULL) != (palette_count==0)) ||
        (palette_rgb && palette_count!=256u) ||
        overlaps(out,sizeof(*out),color,sizeof(*color)) ||
        overlaps(out,sizeof(*out),palette_rgb,palette_count*sizeof(uint32_t)))
        return 0;
    MusaScene2dColorSelection result={0,0};
    if (color->has_truecolor) {
        if (color->truecolor_rgb>0xffffffu) return 0;
        result.mode=MUSA_SCENE2D_EXPLICIT;
        result.rgba=0xff000000u | color->truecolor_rgb;
    } else if (color->aci==0u) {
        result.mode=MUSA_SCENE2D_BYBLOCK;
    } else if (color->aci==256u) {
        result.mode=MUSA_SCENE2D_BYLAYER;
    } else {
        uint32_t rgb=0;
        if (palette_rgb) rgb=palette_rgb[color->aci];
        else if (color->aci<=6u) rgb=basic[color->aci];
        else if (color->aci==7u) rgb=foreground_rgb;
        else return 0; /* Never silently invent the remaining ACI colors. */
        if (rgb>0xffffffu) return 0;
        result.mode=MUSA_SCENE2D_EXPLICIT;
        result.rgba=0xff000000u | rgb;
    }
    *out=result;
    return 1;
}
