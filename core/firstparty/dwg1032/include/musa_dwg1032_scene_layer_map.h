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
    if (!maps || !count || count > SIZE_MAX / sizeof(*maps)) return 0;
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
    /* Never allow a successful lookup to overwrite the source map. */
    const uintptr_t source = (uintptr_t)maps;
    const uintptr_t destination = (uintptr_t)scene_layer;
    const size_t source_bytes = count * sizeof(*maps);
    if (source <= destination
            ? destination - source < source_bytes
            : source - destination < sizeof(*scene_layer)) return 0;
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
/* Optional caller-owned scratch fast path for large authenticated layer
 * tables. Sort scene IDs in scratch to detect duplicates in O(n log n)
 * rather than the O(n^2) pair scan of the small-table API above.
 * Scratch is modified even on failure and must not overlap the map.
 * Neither function parses DWG or establishes that a layer is visible. */
static inline int musa_dwg1032_scene_layer_map_ranges_overlap(
    const void *a, size_t a_bytes, const void *b, size_t b_bytes) {
    const uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < a_bytes : x - y < b_bytes;
}
static inline int musa_dwg1032_scene_layer_map_valid_scratch(
    const MusaDwg1032SceneLayerMap *maps, size_t count,
    uint64_t *scratch, size_t scratch_count) {
    if (!maps || !scratch || !count || scratch_count < count ||
        count > SIZE_MAX / sizeof(*maps) ||
        scratch_count > SIZE_MAX / sizeof(*scratch) ||
        musa_dwg1032_scene_layer_map_ranges_overlap(
            maps, count * sizeof(*maps), scratch,
            scratch_count * sizeof(*scratch))) return 0;
    uint64_t previous = 0;
    unsigned zero_count = 0;
    for (size_t i = 0; i < count; ++i) {
        const MusaDwg1032SceneLayerMap *m = &maps[i];
        if (!m->source_handle || m->source_handle <= previous ||
            m->is_layer_zero > 1 ||
            ((m->scene_layer == 0) != (m->is_layer_zero == 1)))
            return 0;
        zero_count += m->is_layer_zero;
        if (zero_count > 1) return 0;
        previous = m->source_handle;
        scratch[i] = m->scene_layer;
    }
    /* In-place max heapsort: bounded stack, deterministic O(n log n). */
    for (size_t start = count / 2; start > 0;) {
        size_t root = --start;
        while (root < count / 2) {
            size_t child = root * 2 + 1;
            if (child + 1 < count && scratch[child] < scratch[child + 1])
                ++child;
            if (scratch[root] >= scratch[child]) break;
            const uint64_t tmp = scratch[root];
            scratch[root] = scratch[child];
            scratch[child] = tmp;
            root = child;
        }
    }
    for (size_t end = count; end > 1;) {
        --end;
        const uint64_t tmp = scratch[0];
        scratch[0] = scratch[end];
        scratch[end] = tmp;
        size_t root = 0;
        while (root < end / 2) {
            size_t child = root * 2 + 1;
            if (child + 1 < end && scratch[child] < scratch[child + 1])
                ++child;
            if (scratch[root] >= scratch[child]) break;
            const uint64_t swap = scratch[root];
            scratch[root] = scratch[child];
            scratch[child] = swap;
            root = child;
        }
    }
    for (size_t i = 1; i < count; ++i)
        if (scratch[i - 1] == scratch[i]) return 0;
    return 1;
}
/* Atomic many-to-many source handle mapping for paged 2D scene assembly.
 * One O(n log n) table validation followed by O(q log n) lookups.
 * All output, full scratch capacity, source handles and map must be
 * mutually disjoint where writes are involved. Unknown handles reject
 * the entire page without changing any published output. */
static inline int musa_dwg1032_scene_layer_map_find_many_scratch(
    const MusaDwg1032SceneLayerMap *maps, size_t count,
    const uint64_t *source_handles, size_t query_count,
    uint64_t *scene_layers, uint64_t *scratch, size_t scratch_count) {
    if (!source_handles || !scene_layers || !scratch || !query_count ||
        query_count > SIZE_MAX / sizeof(*source_handles) ||
        scratch_count < query_count ||
        scratch_count > SIZE_MAX / sizeof(*scratch)) return 0;
    const size_t qbytes = query_count * sizeof(*source_handles);
    const size_t sbytes = scratch_count * sizeof(*scratch);
    if (musa_dwg1032_scene_layer_map_ranges_overlap(
            scratch, sbytes, source_handles, qbytes) ||
        musa_dwg1032_scene_layer_map_ranges_overlap(
            scratch, sbytes, scene_layers, qbytes) ||
        musa_dwg1032_scene_layer_map_ranges_overlap(
            scene_layers, qbytes, source_handles, qbytes) ||
        (maps && count <= SIZE_MAX / sizeof(*maps) &&
         musa_dwg1032_scene_layer_map_ranges_overlap(
            scene_layers, qbytes, maps, count * sizeof(*maps))) ||
        !musa_dwg1032_scene_layer_map_valid_scratch(
            maps, count, scratch, scratch_count)) return 0;
    for (size_t i = 0; i < query_count; ++i) {
        const uint64_t handle = source_handles[i];
        if (!handle) return 0;
        size_t lo = 0, hi = count;
        while (lo < hi) {
            const size_t mid = lo + (hi - lo) / 2;
            if (maps[mid].source_handle < handle) lo = mid + 1;
            else hi = mid;
        }
        if (lo == count || maps[lo].source_handle != handle) return 0;
        scratch[i] = maps[lo].scene_layer;
    }
    /* No writes to the destination until every handle is authenticated. */
    for (size_t i = 0; i < query_count; ++i)
        scene_layers[i] = scratch[i];
    return 1;
}

#endif
