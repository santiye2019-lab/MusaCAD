/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Pure host test: reject forged inputs without altering caller pointers. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
int main(void){
    uint8_t malformed[256]={0};
    memcpy(malformed,"AC1032",6u);
    MusaDwgAc1032SceneIndex *scene=(MusaDwgAc1032SceneIndex*)(uintptr_t)0x1234u;
    assert(!musa_dwg_ac1032_scene_index_open(
       malformed,sizeof(malformed),&scene));
    assert(scene==(MusaDwgAc1032SceneIndex*)(uintptr_t)0x1234u);
    assert(musa_dwg_ac1032_scene_index_count(NULL)==0u);
    MusaDwgAc1032HandleEntry e={0};
    assert(!musa_dwg_ac1032_scene_index_entry(NULL,0u,&e));
    MusaDwgAc1032NativeLine line={0};
    assert(!musa_dwg_ac1032_scene_index_read_line(NULL,0x1cau,&line));
    uint16_t type=123u;
    assert(!musa_dwg_ac1032_scene_index_object_type(NULL,0x1cau,&type));
    assert(type==123u);
    MusaDwgAc1032LWVertex vertices[2]={{0}};
    MusaDwgAc1032NativeLWPolyline poly={0};
    assert(!musa_dwg_ac1032_scene_index_read_lwpolyline(
        NULL,0x1bdu,vertices,2u,&poly));
    musa_dwg_ac1032_scene_index_close(NULL);
    return 0;
}
