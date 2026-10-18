/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_handle_index.h"
#include "musacad_mc1032_v2.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
int main(void){
    assert(musa_handle_index_create(0)==NULL);
    assert(musa_handle_index_create(1000001u)==NULL);
    MusaHandleIndex *index=musa_handle_index_create(50000);
    assert(index);
    uint32_t untouched=12345;
    assert(musa_handle_index_get(index,42,&untouched)==0&&untouched==12345);
    assert(musa_handle_index_get(index,0,&untouched)==-1);
    for(uint64_t k=1;k<=50000;k++){
        uint64_t handle=UINT64_C(0x100000000)+k*UINT64_C(0x9e3779b97f4a7c15);
        assert(handle!=0);
        assert(musa_handle_index_put(index,handle,(uint32_t)k)==1);
    }
    assert(musa_handle_index_count(index)==50000);
    for(uint64_t k=1;k<=50000;k++){
        uint64_t handle=UINT64_C(0x100000000)+k*UINT64_C(0x9e3779b97f4a7c15);
        uint32_t slot=0;
        assert(musa_handle_index_get(index,handle,&slot)==1&&slot==(uint32_t)k);
        assert(musa_handle_index_put(index,handle,0)==0);
    }
    assert(musa_handle_index_put(index,12345,0)==-2);
    for(uint64_t k=1;k<=50000;k+=2){
        uint64_t h=UINT64_C(0x100000000)+k*UINT64_C(0x9e3779b97f4a7c15);
        assert(musa_handle_index_remove(index,h)==1);
        assert(musa_handle_index_remove(index,h)==0);
    }
    assert(musa_handle_index_count(index)==25000);
    for(uint64_t k=1;k<=50000;k+=2){
        uint64_t h=UINT64_C(0x100000000)+k*UINT64_C(0x9e3779b97f4a7c15);
        assert(musa_handle_index_get(index,h,&untouched)==0);
    }
    for(uint64_t k=1;k<=25000;k++)
        assert(musa_handle_index_put(index,k,((uint32_t)k)*2u)==1);
    assert(musa_handle_index_count(index)==50000);
    for(uint64_t k=1;k<=25000;k++){
        uint32_t value=0;
        assert(musa_handle_index_get(index,k,&value)==1&&value==(uint32_t)k*2u);
        assert(musa_handle_index_set(index,k,(uint32_t)k*3u)==1);
        assert(musa_handle_index_get(index,k,&value)==1&&value==(uint32_t)k*3u);
    }
    musa_handle_index_destroy(index);
    /* Actual scene integration: delete compacts array but HASH always maps
     * remaining immutable handles to the correct new ordered slot index.
     */
    MusaMC2Scene *scene=musa_mc2_scene_create(4096);
    assert(scene);
    uint64_t h=0;
    for(uint32_t i=0;i<4096;i++){
        MusaMC2Entity e={0};
        e.kind=MUSA_MC2_LINE3D;e.layer=0;
        e.x1=(double)i;e.y1=(double)(i*2u);e.z1=100.0;
        e.x2=(double)(i+1u);e.y2=(double)(i*2u+1u);e.z2=200.0;
        assert(musa_mc2_scene_add_entity(scene,&e,&h)==MUSA_MC2_OK);
        assert(h==(uint64_t)i+1u);
    }
    assert(musa_mc2_scene_entity_count(scene)==4096);
    for(uint64_t handle=4096;handle>1;handle-=3)
        assert(musa_mc2_scene_delete(scene,handle)==MUSA_MC2_OK);
    uint32_t count=musa_mc2_scene_entity_count(scene);
    assert(count<4096);
    for(uint32_t i=0;i<count;i++){
        MusaMC2Entity at={0},by_handle={0};
        assert(musa_mc2_scene_get_index(scene,i,&at)==MUSA_MC2_OK);
        assert(musa_mc2_scene_get(scene,at.handle,&by_handle)==MUSA_MC2_OK);
        assert(at.handle==by_handle.handle&&at.x1==by_handle.x1);
    }
    /* Deleted handles must NEVER reappear automatically. */
    MusaMC2Entity added={0};added.kind=MUSA_MC2_POINT3D;
    added.x1=12.5;added.y1=13.5;added.z1=14.5;
    assert(musa_mc2_scene_add_entity(scene,&added,&h)==MUSA_MC2_OK);
    assert(h==4097u);
    assert(musa_mc2_scene_get(scene,4096,&added)==MUSA_MC2_NOT_FOUND);
    MusaMC2Entity got={0};
    assert(musa_mc2_scene_get(scene,h,&got)==MUSA_MC2_OK&&got.z1==14.5);
    musa_mc2_scene_destroy(scene);
    puts("PASS: first-party indexed CAD handles 50k stress, tombstones and 3D identity");
    return 0;
}
