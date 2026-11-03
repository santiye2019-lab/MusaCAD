/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_scene_layer_map.h"
#include <assert.h>
#include <stdint.h>
int main(void) {
    MusaDwg1032SceneLayerMap m[] = {{11, 0, 1}, {23, 23, 0}, {44, 44, 0}};
    uint64_t out = UINT64_MAX;
    assert(musa_dwg1032_scene_layer_map_valid(m, 3));
    assert(musa_dwg1032_scene_layer_map_find(m, 3, 11, &out) && out == 0);
    assert(musa_dwg1032_scene_layer_map_find(m, 3, 44, &out) && out == 44);
    assert(!musa_dwg1032_scene_layer_map_find(m, 3, 99, &out) && out == 44);
    m[1].scene_layer = 0;
    assert(!musa_dwg1032_scene_layer_map_valid(m, 3));
    assert(!musa_dwg1032_scene_layer_map_find(m, 3, 11, &out) && out == 44);
    m[1].scene_layer = 23;
    m[2].source_handle = 11;
    assert(!musa_dwg1032_scene_layer_map_valid(m, 3));
    m[2].source_handle = 44;
    m[0].source_handle = 0;
    assert(!musa_dwg1032_scene_layer_map_valid(m, 3));
    return 0;
}
