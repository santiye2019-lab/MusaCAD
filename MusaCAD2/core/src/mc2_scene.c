#include "mc2_scene.h"
int mc2_scene_resolve(const Mc2SceneItem *items, size_t count,
                      size_t item_index, const Mc2Layer *layers, size_t layer_count,
                      uint32_t parent_block_rgb, Mc2EntityAppearance *out) {
    if (!items || !out || item_index >= count) return 0;
    /* Validate every handle, owner, and ancestor before exposing any appearance. */
    for (size_t i = 0; i < count; ++i) {
        if (!items[i].entity.handle || !mc2_entity_validate(&items[i].entity)) return 0;
        for (size_t j = 0; j < i; ++j)
            if (items[j].entity.handle == items[i].entity.handle) return 0;
        uint64_t current = items[i].parent_handle;
        size_t steps = 0;
        while (current) {
            size_t found = count;
            for (size_t j = 0; j < count; ++j)
                if (items[j].entity.handle == current) { found = j; break; }
            if (found == count || ++steps > count) return 0;
            current = items[found].parent_handle;
        }
    }
    int visible = items[item_index].own_visible != 0;
    uint64_t parent = items[item_index].parent_handle;
    while (parent) {
        size_t index = 0;
        while (index < count && items[index].entity.handle != parent) ++index;
        if (index == count) return 0;
        if (!items[index].own_visible) visible = 0;
        parent = items[index].parent_handle;
    }
    return mc2_resolve_appearance(&items[item_index].entity,items[item_index].color,
                                  layers,layer_count,parent_block_rgb,visible,out);
}
