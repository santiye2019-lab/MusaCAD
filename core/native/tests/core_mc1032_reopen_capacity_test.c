/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_mc1032_v2.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Once reopened, a valid MC1032 document must remain editable. The wire
 * entity count is NOT a permanent maximum scene size. This regression runs
 * unchanged in the shared C engine on Windows, Linux, macOS, iOS and Android.
 */
static unsigned char *write_scene(const MusaMC2Scene *scene,size_t *length) {
    assert(scene && length);
    *length=0;
    assert(musa_mc2_encode(scene,NULL,0,length)==MUSA_MC2_OK);
    assert(*length>0 && *length<=MUSA_MC2_MAX_BYTES);
    unsigned char *bytes=(unsigned char *)malloc(*length);
    assert(bytes);
    size_t written=0;
    assert(musa_mc2_encode(scene,bytes,*length,&written)==MUSA_MC2_OK);
    assert(written==*length);
    return bytes;
}
int main(void) {
    MusaMC2Scene *source=musa_mc2_scene_create(2);
    assert(source);
    MusaMC2Entity entity={0};
    entity.kind=MUSA_MC2_POINT3D;
    entity.x1=12.25;entity.y1=-8.5;entity.z1=100.75;
    uint64_t handle=0;
    assert(musa_mc2_scene_add_entity(source,&entity,&handle)==MUSA_MC2_OK);
    assert(handle==1u);
    entity.x1=-33.25;entity.z1=200.75;
    assert(musa_mc2_scene_add_entity(source,&entity,&handle)==MUSA_MC2_OK);
    assert(handle==2u);
    size_t original_length=0;
    unsigned char *original=write_scene(source,&original_length);
    MusaMC2Scene *reopened=NULL;
    assert(musa_mc2_decode(original,original_length,&reopened)==MUSA_MC2_OK);
    assert(reopened && musa_mc2_scene_entity_count(reopened)==2u);
    /* The old decoder allocated exactly two entity slots here and incorrectly
     * reported LIMIT on the first additional entity in an opened file.
     */
    for(uint32_t n=3u;n<=MUSA_MC2_MAX_ENTITIES;n++) {
        entity.handle=0;
        entity.x1=(double)n;entity.y1=-(double)n;entity.z1=10.0;
        assert(musa_mc2_scene_add_entity(reopened,&entity,&handle)==MUSA_MC2_OK);
        assert(handle==(uint64_t)n);
    }
    assert(musa_mc2_scene_entity_count(reopened)==MUSA_MC2_MAX_ENTITIES);
    uint64_t untouched=UINT64_C(987654321);
    assert(musa_mc2_scene_add_entity(reopened,&entity,&untouched)==MUSA_MC2_LIMIT);
    assert(untouched==UINT64_C(987654321));
    MusaMC2Entity first={0},second={0},last={0};
    assert(musa_mc2_scene_get(reopened,1u,&first)==MUSA_MC2_OK);
    assert(musa_mc2_scene_get(reopened,2u,&second)==MUSA_MC2_OK);
    assert(musa_mc2_scene_get(reopened,MUSA_MC2_MAX_ENTITIES,&last)==MUSA_MC2_OK);
    assert(first.x1==12.25 && second.x1==-33.25);
    assert(last.x1==(double)MUSA_MC2_MAX_ENTITIES);
    /* The original byte source is still intact and independently readable. */
    MusaMC2Scene *untouched_source=NULL;
    assert(musa_mc2_decode(original,original_length,&untouched_source)==MUSA_MC2_OK);
    assert(musa_mc2_scene_entity_count(untouched_source)==2u);
    musa_mc2_scene_destroy(untouched_source);
    size_t full_length=0;
    unsigned char *full=write_scene(reopened,&full_length);
    MusaMC2Scene *full_reopened=NULL;
    assert(musa_mc2_decode(full,full_length,&full_reopened)==MUSA_MC2_OK);
    assert(musa_mc2_scene_entity_count(full_reopened)==MUSA_MC2_MAX_ENTITIES);
    assert(musa_mc2_scene_get(full_reopened,MUSA_MC2_MAX_ENTITIES,&last)==MUSA_MC2_OK);
    assert(last.x1==(double)MUSA_MC2_MAX_ENTITIES);
    musa_mc2_scene_destroy(full_reopened);
    free(full);
    musa_mc2_scene_destroy(reopened);
    musa_mc2_scene_destroy(source);
    free(original);

    /* Empty wire records also reopen as editable, never a one-slot scene. */
    MusaMC2Scene *empty=musa_mc2_scene_create(1);
    assert(empty);
    size_t empty_length=0;
    unsigned char *empty_bytes=write_scene(empty,&empty_length);
    MusaMC2Scene *opened_empty=NULL;
    assert(musa_mc2_decode(empty_bytes,empty_length,&opened_empty)==MUSA_MC2_OK);
    assert(musa_mc2_scene_entity_count(opened_empty)==0u);
    memset(&entity,0,sizeof(entity));
    entity.kind=MUSA_MC2_POINT3D;
    for(uint32_t i=1;i<=4;i++) {
        entity.x1=(double)i;
        assert(musa_mc2_scene_add_entity(opened_empty,&entity,&handle)==MUSA_MC2_OK);
        assert(handle==(uint64_t)i);
    }
    musa_mc2_scene_destroy(opened_empty);
    musa_mc2_scene_destroy(empty);
    free(empty_bytes);
    puts("PASS: MC1032 rev2 reopened 0/2/4096 entities remain fully editable");
    return 0;
}
