/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_scene_layer_map.h"
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
int main(void) {
    MusaDwg1032SceneLayerMap m[] = {{11, 0, 1}, {23, 23, 0}, {44, 44, 0}};
    uint64_t out = UINT64_MAX;
    assert(musa_dwg1032_scene_layer_map_valid(m, 3));
    assert(musa_dwg1032_scene_layer_map_find(m, 3, 11, &out) && out == 0);
    assert(musa_dwg1032_scene_layer_map_find(m, 3, 44, &out) && out == 44);
    assert(!musa_dwg1032_scene_layer_map_find(m, 3, 99, &out) && out == 44);
    /* A lookup must not corrupt its own source map through output aliasing. */
    const MusaDwg1032SceneLayerMap before = m[1];
    assert(!musa_dwg1032_scene_layer_map_find(
        m, 3, 11, &m[1].scene_layer));
    assert(m[1].source_handle == before.source_handle &&
           m[1].scene_layer == before.scene_layer &&
           m[1].is_layer_zero == before.is_layer_zero);
    assert(!musa_dwg1032_scene_layer_map_valid(m, SIZE_MAX));
    m[1].scene_layer = 0;
    assert(!musa_dwg1032_scene_layer_map_valid(m, 3));
    assert(!musa_dwg1032_scene_layer_map_find(m, 3, 11, &out) && out == 44);
    m[1].scene_layer = 23;
    m[2].source_handle = 11;
    assert(!musa_dwg1032_scene_layer_map_valid(m, 3));
    m[2].source_handle = 44;
    m[0].source_handle = 0;
    assert(!musa_dwg1032_scene_layer_map_valid(m, 3));

    /* Large, deliberately permuted scene IDs exercise the O(n log n)
     * scratch path and one-pass atomic page resolution. */
    enum { BIG = 20000 };
    static MusaDwg1032SceneLayerMap large[BIG];
    static uint64_t source[BIG], resolved[BIG], scratch[BIG];
    for (size_t i = 0; i < BIG; ++i) {
        large[i].source_handle = (uint64_t)i + 1u;
        large[i].scene_layer = (uint64_t)(BIG - i);
        large[i].is_layer_zero = 0;
        source[i] = (uint64_t)i + 1u;
        resolved[i] = UINT64_MAX;
    }
    assert(musa_dwg1032_scene_layer_map_valid_scratch(
        large, BIG, scratch, BIG));
    assert(musa_dwg1032_scene_layer_map_find_many_scratch(
        large, BIG, source, BIG, resolved, scratch, BIG));
    for (size_t i = 0; i < BIG; ++i)
        assert(resolved[i] == (uint64_t)(BIG - i));
    /* An invalid final handle must not partially publish a page. */
    source[BIG - 1] = UINT64_MAX;
    assert(!musa_dwg1032_scene_layer_map_find_many_scratch(
        large, BIG, source, BIG, resolved, scratch, BIG));
    assert(resolved[0] == BIG && resolved[BIG - 1] == 1u);
    source[BIG - 1] = BIG;
    /* Nonadjacent duplicate scene IDs must be rejected. */
    large[BIG - 1].scene_layer = large[0].scene_layer;
    assert(!musa_dwg1032_scene_layer_map_valid_scratch(
        large, BIG, scratch, BIG));
    assert(!musa_dwg1032_scene_layer_map_find_many_scratch(
        large, BIG, source, BIG, resolved, scratch, BIG));
    assert(resolved[0] == BIG && resolved[BIG - 1] == 1u);
    large[BIG - 1].scene_layer = 1u;
    /* A zero scene ID requires an explicit layer-zero designation. */
    large[BIG - 1].scene_layer = 0u;
    assert(!musa_dwg1032_scene_layer_map_valid_scratch(
        large, BIG, scratch, BIG));
    large[BIG - 1].is_layer_zero = 1u;
    assert(musa_dwg1032_scene_layer_map_valid_scratch(
        large, BIG, scratch, BIG));
    assert(musa_dwg1032_scene_layer_map_find_many_scratch(
        large, BIG, source, BIG, resolved, scratch, BIG));
    assert(resolved[BIG - 1] == 0u);
    /* Reject aliases into the map, source handles or scratch capacity. */
    assert(!musa_dwg1032_scene_layer_map_find_many_scratch(
        large, BIG, source, BIG, source, scratch, BIG));
    assert(!musa_dwg1032_scene_layer_map_find_many_scratch(
        large, BIG, source, BIG, resolved, resolved, BIG));
    assert(!musa_dwg1032_scene_layer_map_valid_scratch(
        large, BIG, &large[0].scene_layer, BIG));
    assert(!musa_dwg1032_scene_layer_map_find_many_scratch(
        large, BIG, source, BIG, &large[0].scene_layer, scratch, BIG));
    assert(!musa_dwg1032_scene_layer_map_find_many_scratch(
        large, BIG, source, BIG, resolved, scratch, BIG - 1));
    assert(resolved[BIG - 1] == 0u);
    return 0;
}
