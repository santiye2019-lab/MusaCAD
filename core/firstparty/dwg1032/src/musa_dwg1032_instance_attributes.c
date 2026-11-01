/* SPDX-License-Identifier: MIT
 * Two-phase first-party C11 INSERT-viewport color/visibility publication.
 */
#include "musa_dwg1032_instance_attributes.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
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
    uint8_t *visibility_scratch, size_t visibility_scratch_capacity) {
    if (!count || !layer_zero_handle || insert_visible>1u ||
        count>color_scratch_capacity ||
        count>visibility_scratch_capacity ||
        count>SIZE_MAX/sizeof(*definition_colors) ||
        count>SIZE_MAX/sizeof(*source_colors) ||
        count>SIZE_MAX/sizeof(*source_layer_handles) ||
        count>SIZE_MAX/sizeof(*colors_out) ||
        color_scratch_capacity>SIZE_MAX/sizeof(*colors_scratch) ||
        layer_count>SIZE_MAX/sizeof(*layers) ||
        override_count>SIZE_MAX/sizeof(*overrides) ||
        !definition_colors || !source_colors || !source_layer_handles ||
        !definition_owner_visible || !insert_color ||
        !insert_layer_color || !layers || !layer_count ||
        (override_count && !overrides) || !colors_out || !visible_out ||
        !colors_scratch || !visibility_scratch ||
        insert_color->byblock || insert_layer_color->byblock)
        return 0;
    const void *sources[]={
        definition_colors,source_colors,source_layer_handles,
        definition_owner_visible,insert_color,insert_layer_color,
        layers,overrides
    };
    const size_t source_bytes[]={
        count*sizeof(*definition_colors),count*sizeof(*source_colors),
        count*sizeof(*source_layer_handles),count,
        sizeof(*insert_color),sizeof(*insert_layer_color),
        layer_count*sizeof(*layers),override_count*sizeof(*overrides)
    };
    const void *destinations[]={
        colors_out,visible_out,colors_scratch,visibility_scratch
    };
    const size_t destination_bytes[]={
        count*sizeof(*colors_out),count,
        color_scratch_capacity*sizeof(*colors_scratch),
        visibility_scratch_capacity
    };
    for (size_t i=0u;i<4u;++i) {
        for (size_t j=i+1u;j<4u;++j)
            if (overlaps(destinations[i],destination_bytes[i],
                         destinations[j],destination_bytes[j])) return 0;
        for (size_t j=0u;j<8u;++j)
            if (overlaps(destinations[i],destination_bytes[i],
                         sources[j],source_bytes[j])) return 0;
    }
    /* Both calculations publish into scratch first. A later malformed
     * viewport/owner flag cannot leak correct-looking partial colors. */
    if (!musa_dwg1032_layer_color_apply_insert_context(
            definition_colors,source_colors,source_layer_handles,count,
            layer_zero_handle,insert_color,insert_layer_color,
            colors_scratch))
        return 0;
    if (!musa_dwg1032_viewport_insert_visibility(
            source_layer_handles,definition_owner_visible,count,
            layer_zero_handle,insert_visible,layers,layer_count,
            overrides,override_count,visibility_scratch))
        return 0;
    /* The instance must be independently resolved, never a definition's
     * unresolved ByBlock fallback masquerading as drawable color. */
    for (size_t i=0u;i<count;++i)
        if (colors_scratch[i].byblock) return 0;
    memcpy(colors_out,colors_scratch,count*sizeof(*colors_out));
    memcpy(visible_out,visibility_scratch,count);
    return 1;
}
