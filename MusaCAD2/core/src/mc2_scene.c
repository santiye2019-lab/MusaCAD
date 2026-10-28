#include "mc2_scene.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct { uint64_t handle; size_t index; } Mc2HandleIndex;
typedef struct { uint32_t id; size_t index; } Mc2LayerIndex;

static int compare_layers(const void *a, const void *b) {
    const uint32_t x=((const Mc2LayerIndex *)a)->id;
    const uint32_t y=((const Mc2LayerIndex *)b)->id;
    return (x>y)-(x<y);
}
static int find_layer(const Mc2LayerIndex *sorted, size_t count,
                      uint32_t id, size_t *index) {
    const Mc2LayerIndex key={id,0};
    const Mc2LayerIndex *entry=(const Mc2LayerIndex *)
        bsearch(&key,sorted,count,sizeof(*sorted),compare_layers);
    if (!entry) return 0;
    *index=entry->index;
    return 1;
}

static int compare_handles(const void *a, const void *b) {
    const uint64_t x=((const Mc2HandleIndex *)a)->handle;
    const uint64_t y=((const Mc2HandleIndex *)b)->handle;
    return (x>y)-(x<y);
}
static int find_handle(const Mc2HandleIndex *sorted, size_t count,
                       uint64_t handle, size_t *index) {
    Mc2HandleIndex key={handle,0};
    const Mc2HandleIndex *entry=(const Mc2HandleIndex *)
        bsearch(&key,sorted,count,sizeof(*sorted),compare_handles);
    if (!entry) return 0;
    *index=entry->index;
    return 1;
}
int mc2_scene_resolve_all(const Mc2SceneItem *items, size_t count,
                          const Mc2Layer *layers, size_t layer_count,
                          uint32_t parent_block_rgb, Mc2EntityAppearance *out) {
    if (!count) return 1;
    if (!items || !out || !layer_count || !layers ||
        parent_block_rgb>0xffffffu ||
        count>SIZE_MAX/sizeof(Mc2HandleIndex) ||
        count>SIZE_MAX/sizeof(Mc2EntityAppearance) ||
        count>SIZE_MAX/sizeof(size_t) ||
        layer_count>SIZE_MAX/sizeof(Mc2LayerIndex)) return 0;
    Mc2HandleIndex *sorted=(Mc2HandleIndex *)malloc(count*sizeof(*sorted));
    Mc2LayerIndex *sorted_layers=(Mc2LayerIndex *)malloc(layer_count*sizeof(*sorted_layers));
    Mc2EntityAppearance *resolved=(Mc2EntityAppearance *)malloc(count*sizeof(*resolved));
    size_t *path=(size_t *)malloc(count*sizeof(*path));
    uint8_t *state=(uint8_t *)calloc(count,sizeof(*state));
    int ok=0;
    if (!sorted || !sorted_layers || !resolved || !path || !state) goto done;
    for (size_t i=0;i<layer_count;++i) {
        if (layers[i].color_rgb>0xffffffu) goto done;
        sorted_layers[i].id=layers[i].id;
        sorted_layers[i].index=i;
    }
    qsort(sorted_layers,layer_count,sizeof(*sorted_layers),compare_layers);
    for (size_t i=1;i<layer_count;++i)
        if (sorted_layers[i-1].id==sorted_layers[i].id) goto done;
    for (size_t i=0;i<count;++i) {
        if (!items[i].entity.handle || !mc2_entity_validate(&items[i].entity)) goto done;
        sorted[i].handle=items[i].entity.handle;
        sorted[i].index=i;
    }
    qsort(sorted,count,sizeof(*sorted),compare_handles);
    for (size_t i=1;i<count;++i)
        if (sorted[i-1].handle==sorted[i].handle) goto done;
    for (size_t i=0;i<count;++i) {
        if (state[i]==2) continue;
        size_t current=i, depth=0;
        for (;;) {
            if (state[current]==2) break;
            if (state[current]==1) goto done; /* cycle */
            state[current]=1;
            path[depth++]=current;
            const uint64_t parent=items[current].parent_handle;
            if (!parent) break;
            if (!find_handle(sorted,count,parent,&current)) goto done;
        }
        while (depth) {
            const size_t k=path[--depth];
            const uint64_t parent=items[k].parent_handle;
            int inherited_visible=1;
            uint32_t inherited_rgb=parent_block_rgb;
            if (parent) {
                size_t parent_index=0;
                if (!find_handle(sorted,count,parent,&parent_index) ||
                    state[parent_index]!=2) goto done;
                inherited_visible=resolved[parent_index].visible;
                /* BYBLOCK inherits the actual resolved insertion color,
                 * including colors inherited by that insertion itself. */
                inherited_rgb=resolved[parent_index].effective_rgb;
            }
            size_t layer_index=0;
            if (!find_layer(sorted_layers,layer_count,items[k].entity.layer_id,&layer_index) ||
                !mc2_resolve_appearance_on_layer(&items[k].entity,items[k].color,
                    &layers[layer_index],inherited_rgb,
                    inherited_visible && items[k].own_visible,
                    &resolved[k])) goto done;
            state[k]=2;
        }
    }
    memcpy(out,resolved,count*sizeof(*out));
    ok=1;
done:
    free(sorted_layers);
    free(state);
    free(path);
    free(resolved);
    free(sorted);
    return ok;
}
int mc2_scene_resolve(const Mc2SceneItem *items, size_t count,
                      size_t item_index, const Mc2Layer *layers, size_t layer_count,
                      uint32_t parent_block_rgb, Mc2EntityAppearance *out) {
    if (!items || !out || item_index>=count ||
        count>SIZE_MAX/sizeof(Mc2EntityAppearance)) return 0;
    Mc2EntityAppearance *all=(Mc2EntityAppearance *)
        malloc(count*sizeof(*all));
    if (!all) return 0;
    const int ok=mc2_scene_resolve_all(items,count,layers,layer_count,
                                        parent_block_rgb,all);
    if (ok) *out=all[item_index];
    free(all);
    return ok;
}
