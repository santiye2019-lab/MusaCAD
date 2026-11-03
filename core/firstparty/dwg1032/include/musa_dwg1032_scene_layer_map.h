/* SPDX-License-Identifier: MIT
 * Explicit source-layer to scene-layer mapping for predecoded records.
 * No AC1032 decoding, no inferred layer-zero handles.
 */
#ifndef MUSA_DWG1032_SCENE_LAYER_MAP_H
#define MUSA_DWG1032_SCENE_LAYER_MAP_H
#include <stddef.h>
#include <stdint.h>
typedef struct MusaDwg1032SceneLayerMap {
    uint64_t source_handle; /* authenticated DWG layer record, never zero */
    uint64_t scene_layer;   /* zero is reserved for authenticated layer zero */
    uint8_t is_layer_zero;  /* strictly 0 or 1 */
} MusaDwg1032SceneLayerMap;
/* Caller supplies ascending unique source handles. Reject duplicate scene
 * IDs and ambiguous zero designation. This is not layer-table parsing. */
static inline int musa_dwg1032_scene_layer_map_valid(
    const MusaDwg1032SceneLayerMap *maps, size_t count) {
    if (!maps || !count) return 0;
    uint64_t previous = 0;
    unsigned zero_count = 0;
    for (size_t i = 0; i < count; ++i) {
        const MusaDwg1032SceneLayerMap *m = &maps[i];
        if (!m->source_handle || m->source_handle <= previous ||
            m->is_layer_zero > 1 ||
            ((m->scene_layer == 0) != (m->is_layer_zero == 1))) return 0;
        for (size_t j = 0; j < i; ++j)
            if (maps[j].scene_layer == m->scene_layer) return 0;
        zero_count += m->is_layer_zero;
        if (zero_count > 1) return 0;
        previous = m->source_handle;
    }
    return 1;
}
/* Atomic output: scene_layer remains unchanged on any failure. */
static inline int musa_dwg1032_scene_layer_map_find(
    const MusaDwg1032SceneLayerMap *maps, size_t count,
    uint64_t source_handle, uint64_t *scene_layer) {
    if (!scene_layer || !source_handle ||
        !musa_dwg1032_scene_layer_map_valid(maps, count)) return 0;
    size_t lo = 0, hi = count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (maps[mid].source_handle < source_handle) lo = mid + 1;
        else hi = mid;
    }
    if (lo == count || maps[lo].source_handle != source_handle) return 0;
    *scene_layer = maps[lo].scene_layer;
    return 1;
}
#endif
