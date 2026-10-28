#include "mc2_layers.h"
#include <stddef.h>

int mc2_resolve_appearance_on_layer(const Mc2Entity2 *entity,
                                    Mc2Color color, const Mc2Layer *layer,
                                    uint32_t parent_block_rgb, int parent_visible,
                                    Mc2EntityAppearance *out) {
    if (!entity || !layer || !out ||
        layer->id != entity->layer_id ||
        layer->color_rgb > 0xffffffu || parent_block_rgb > 0xffffffu ||
        color.rgb > 0xffffffu || !mc2_entity_validate(entity)) return 0;
    if (color.mode != MC2_COLOR_EXPLICIT &&
        color.mode != MC2_COLOR_BY_LAYER &&
        color.mode != MC2_COLOR_BY_BLOCK) return 0;
    Mc2EntityAppearance result;
    result.effective_rgb = color.mode == MC2_COLOR_BY_LAYER ? layer->color_rgb :
                           color.mode == MC2_COLOR_BY_BLOCK ? parent_block_rgb : color.rgb;
    result.visible = !!parent_visible && !!layer->is_on && !layer->is_frozen;
    *out = result;
    return 1;
}

int mc2_resolve_appearance(const Mc2Entity2 *entity,
                           Mc2Color color,
                           const Mc2Layer *layers, size_t layer_count,
                           uint32_t parent_block_rgb,
                           int parent_visible,
                           Mc2EntityAppearance *out) {
    if (!entity || !out || (layer_count && !layers)) return 0;
    const Mc2Layer *found = NULL;
    for (size_t i = 0; i < layer_count; ++i) {
        if (layers[i].color_rgb > 0xffffffu) return 0;
        if (layers[i].id == entity->layer_id) {
            if (found) return 0;
            found = &layers[i];
        }
    }
    if (!found) return 0;
    return mc2_resolve_appearance_on_layer(entity,color,found,
                                           parent_block_rgb,parent_visible,out);
}
