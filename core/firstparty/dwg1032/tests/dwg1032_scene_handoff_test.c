/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_scene_handoff.h"
#include <assert.h>
int main(void) {
    MusaDwg1032SceneEvidence e[2] = {
        {1, 0, 10, 0x00ff00u, 1, 1, 1, 1, 1},
        {2, 1, 10, 0x0000ffu, 2, 1, 1, 1, 1}
    };
    assert(musa_dwg1032_scene_handoff_validate(e, 2, 0));
    assert(!musa_dwg1032_scene_handoff_validate(e, 2, 1));
    assert(!musa_dwg1032_scene_handoff_validate(0, 2, 0));
    assert(musa_dwg1032_scene_handoff_validate(0, 0, 1));
    assert(!musa_dwg1032_scene_handoff_validate(0, 0, 0));
    e[1].source_handle = 1;
    assert(!musa_dwg1032_scene_handoff_validate(e, 2, 0));
    e[1].source_handle = 2;
    e[1].owner_verified = 0;
    assert(!musa_dwg1032_scene_handoff_validate(e, 2, 0));
    e[1].owner_verified = 1;
    e[1].layer_handle = 0;
    assert(!musa_dwg1032_scene_handoff_validate(e, 2, 0));
    e[1].layer_handle = 10;
    e[1].color_verified = 0;
    assert(!musa_dwg1032_scene_handoff_validate(e, 2, 0));
    e[1].color_verified = 1;
    e[1].geometry_verified = 0;
    assert(!musa_dwg1032_scene_handoff_validate(e, 2, 0));
    e[1].geometry_verified = 1;
    e[1].entity_kind = 0;
    assert(!musa_dwg1032_scene_handoff_validate(e, 2, 0));
    e[1].entity_kind = 2;
    e[1].owner_handle = 2;
    assert(!musa_dwg1032_scene_handoff_validate(e, 2, 0));
    return 0;
}
