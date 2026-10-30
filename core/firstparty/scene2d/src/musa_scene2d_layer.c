/* SPDX-License-Identifier: MIT */
#include "musa_scene2d_layer.h"
#include <stdint.h>
#include <stddef.h>
static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int convert(const MusaScene2dCadLayer *cad,
                   const uint32_t *palette_rgb, size_t palette_count,
                   uint32_t foreground_rgb, MusaScene2dLayer *result) {
    if (cad->signed_aci==0 || cad->signed_aci < -255 ||
        cad->signed_aci > 255 || cad->frozen>1u ||
        cad->viewport_frozen>1u || cad->has_truecolor>1u) return 0;
    const int32_t signed_aci=(int32_t)cad->signed_aci;
    const MusaScene2dCadColor color={
        (uint16_t)(signed_aci<0 ? -signed_aci : signed_aci),
        cad->has_truecolor, cad->truecolor_rgb};
    MusaScene2dColorSelection selected={0,0};
    if (!musa_scene2d_cad_color_select(&color,palette_rgb,palette_count,
                                       foreground_rgb,&selected) ||
        selected.mode!=MUSA_SCENE2D_EXPLICIT) return 0;
    result->handle=cad->handle;
    result->rgba=selected.rgba;
    result->visible=(uint8_t)(signed_aci>0 && !cad->frozen &&
                              !cad->viewport_frozen);
    return 1;
}
int musa_scene2d_layer_from_cad(const MusaScene2dCadLayer *cad,
                                const uint32_t *palette_rgb,
                                size_t palette_count,
                                uint32_t foreground_rgb,
                                MusaScene2dLayer *out) {
    if (!cad || !out ||
        overlaps(cad,sizeof(*cad),out,sizeof(*out)) ||
        (palette_count>SIZE_MAX/sizeof(uint32_t)) ||
        overlaps(palette_rgb,palette_count*sizeof(uint32_t),out,sizeof(*out)))
        return 0;
    MusaScene2dLayer result={0,0,0};
    if (!convert(cad,palette_rgb,palette_count,foreground_rgb,&result)) return 0;
    *out=result;
    return 1;
}
int musa_scene2d_layers_from_cad(const MusaScene2dCadLayer *cad,
                                 size_t count,
                                 const uint32_t *palette_rgb,
                                 size_t palette_count,
                                 uint32_t foreground_rgb,
                                 MusaScene2dLayer *out, size_t capacity) {
    if ((!cad && count) || (!out && count) || capacity<count ||
        count>SIZE_MAX/sizeof(*cad) || count>SIZE_MAX/sizeof(*out) ||
        palette_count>SIZE_MAX/sizeof(uint32_t) ||
        overlaps(cad,count*sizeof(*cad),out,count*sizeof(*out)) ||
        overlaps(palette_rgb,palette_count*sizeof(uint32_t),out,
                 count*sizeof(*out))) return 0;
    MusaScene2dLayer scratch={0,0,0};
    for (size_t i=0;i<count;++i)
        if (!convert(&cad[i],palette_rgb,palette_count,foreground_rgb,&scratch))
            return 0;
    for (size_t i=0;i<count;++i) {
        (void)convert(&cad[i],palette_rgb,palette_count,foreground_rgb,&scratch);
        out[i]=scratch;
    }
    return 1;
}

/* Bounded, transactional page conversion of predecoded CAD layer records. */
int musa_scene2d_layers_from_cad_page(const MusaScene2dCadLayer *cad,
                                      size_t count, size_t start,
                                      const uint32_t *palette_rgb,
                                      size_t palette_count,
                                      uint32_t foreground_rgb,
                                      MusaScene2dLayer *out, size_t capacity,
                                      size_t *written, size_t *total) {
    if (!written || !total || (count && !cad) ||
        count>SIZE_MAX/sizeof(*cad) ||
        palette_count>SIZE_MAX/sizeof(*palette_rgb) ||
        ((palette_rgb==NULL)!=(palette_count==0u)) ||
        (palette_rgb && palette_count!=256u) ||
        foreground_rgb>0xffffffu ||
        overlaps(written,sizeof(*written),total,sizeof(*total)) ||
        overlaps(cad,count*sizeof(*cad),written,sizeof(*written)) ||
        overlaps(cad,count*sizeof(*cad),total,sizeof(*total)) ||
        overlaps(palette_rgb,palette_count*sizeof(*palette_rgb),
                 written,sizeof(*written)) ||
        overlaps(palette_rgb,palette_count*sizeof(*palette_rgb),
                 total,sizeof(*total))) return 0;
    const size_t remaining=start<count ? count-start : 0u;
    const size_t page_count=remaining<capacity ? remaining : capacity;
    if (page_count>SIZE_MAX/sizeof(*out) || (page_count && !out)) return 0;
    const size_t output_bytes=page_count*sizeof(*out);
    if (overlaps(cad,count*sizeof(*cad),out,output_bytes) ||
        overlaps(palette_rgb,palette_count*sizeof(*palette_rgb),
                 out,output_bytes) ||
        overlaps(out,output_bytes,written,sizeof(*written)) ||
        overlaps(out,output_bytes,total,sizeof(*total))) return 0;
    MusaScene2dLayer converted={0,0,0};
    for (size_t i=0;i<page_count;++i)
        if (!convert(&cad[start+i],palette_rgb,palette_count,
                     foreground_rgb,&converted)) return 0;
    for (size_t i=0;i<page_count;++i) {
        (void)convert(&cad[start+i],palette_rgb,palette_count,
                      foreground_rgb,&converted);
        out[i]=converted;
    }
    *written=page_count;
    *total=count;
    return 1;
}
