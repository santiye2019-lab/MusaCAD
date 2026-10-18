/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_mc1032_migrate.h"
#include "musacad_mc1032.h"
#include "musacad_document.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void identical(const MusaDocumentEntity *a,const MusaDocumentEntity *b){
    assert(a->handle==b->handle&&a->kind==b->kind);
    assert(a->x1==b->x1&&a->y1==b->y1&&a->x2==b->x2&&a->y2==b->y2);
    assert(a->radius==b->radius&&a->start_deg==b->start_deg&&
           a->end_deg==b->end_deg);
}
int main(void){
    MusaDocument *doc=musa_document_create(4);
    assert(doc);
    MusaDocumentEntity old={0};
    uint64_t actual=0;
    old.kind=MUSA_DOCUMENT_LINE;old.handle=100;
    old.x1=-11.5;old.y1=2.25;old.x2=300.5;old.y2=-4;
    assert(musa_document_load_entity(doc,&old,&actual)==MUSA_DOCUMENT_OK);
    old.kind=MUSA_DOCUMENT_POINT;old.handle=20;
    old.x1=3.5;old.y1=4;old.x2=0;old.y2=0;
    assert(musa_document_load_entity(doc,&old,&actual)==MUSA_DOCUMENT_OK);
    const size_t old_size=musa_mc1032_encoded_size(doc);
    unsigned char *old_data=(unsigned char *)malloc(old_size);
    assert(old_data);
    size_t wrote=0;
    assert(musa_mc1032_encode(doc,old_data,old_size,&wrote)==MUSA_MC1032_OK);
    assert(wrote==old_size);
    size_t target_size=0;
    assert(musa_mc1032_upgrade_v1_to_v2(old_data,old_size,NULL,0,&target_size)
           ==MUSA_MC2_OK);
    assert(target_size==48+64+2*80);
    unsigned char *new_data=(unsigned char *)malloc(target_size);
    assert(new_data);
    memset(new_data,0x77,target_size);
    assert(musa_mc1032_upgrade_v1_to_v2(old_data,old_size,
          new_data,target_size-1,&wrote)==MUSA_MC2_LIMIT);
    for(size_t i=0;i<target_size;i++)assert(new_data[i]==0x77);
    assert(musa_mc1032_upgrade_v1_to_v2(old_data,old_size,
          new_data,target_size,&wrote)==MUSA_MC2_OK);
    assert(wrote==target_size&&new_data[6]==2);
    MusaMC2Scene *scene=NULL;
    assert(musa_mc2_decode(new_data,target_size,&scene)==MUSA_MC2_OK);
    assert(musa_mc2_scene_entity_count(scene)==2);
    MusaMC2Entity a={0};
    assert(musa_mc2_scene_get_index(scene,0,&a)==MUSA_MC2_OK);
    assert(a.kind==MUSA_MC2_LINE3D&&a.handle==100&&
           a.z1==0&&a.z2==0&&a.x2==300.5);
    assert(musa_mc2_scene_get_index(scene,1,&a)==MUSA_MC2_OK);
    assert(a.kind==MUSA_MC2_POINT3D&&a.handle==20&&
           a.layer==0&&a.z1==0);
    size_t downgraded_size=0;
    assert(musa_mc1032_downgrade_v2_to_v1(
          new_data,target_size,NULL,0,&downgraded_size)==MUSA_MC2_OK);
    assert(downgraded_size==old_size);
    unsigned char *down=(unsigned char *)malloc(old_size);
    assert(down);
    assert(musa_mc1032_downgrade_v2_to_v1(new_data,target_size,
          down,old_size,&wrote)==MUSA_MC2_OK);
    assert(wrote==old_size&&memcmp(old_data,down,old_size)==0);
    MusaDocument *reopen=NULL;
    assert(musa_mc1032_decode(down,old_size,&reopen)==MUSA_MC1032_OK);
    for(size_t i=0;i<2;i++){
        MusaDocumentEntity expected={0},got={0};
        assert(musa_document_get_index(doc,i,&expected)==MUSA_DOCUMENT_OK);
        assert(musa_document_get_index(reopen,i,&got)==MUSA_DOCUMENT_OK);
        identical(&expected,&got);
    }
    musa_document_destroy(reopen);
    /* Geometry unsupported by the target version MUST reject, not drop. */
    assert(musa_document_add_circle(doc,0,0,5,&actual)==MUSA_DOCUMENT_OK);
    size_t with_circle=musa_mc1032_encoded_size(doc);
    unsigned char *v1_circle=(unsigned char *)malloc(with_circle);
    assert(v1_circle);
    assert(musa_mc1032_encode(doc,v1_circle,with_circle,&wrote)==MUSA_MC1032_OK);
    memset(new_data,0x58,target_size);
    assert(musa_mc1032_upgrade_v1_to_v2(v1_circle,with_circle,
            new_data,target_size,&wrote)==MUSA_MC2_UNSUPPORTED);
    for(size_t i=0;i<target_size;i++)assert(new_data[i]==0x58);
    free(v1_circle);
    /* Z information may not disappear by coercion into v1 XY. */
    assert(musa_mc2_scene_translate(scene,100,0,0,1.25)==MUSA_MC2_OK);
    size_t modified_size=0;
    assert(musa_mc2_encode(scene,NULL,0,&modified_size)==MUSA_MC2_OK);
    unsigned char *modified=(unsigned char *)malloc(modified_size);
    assert(modified);
    assert(musa_mc2_encode(scene,modified,modified_size,&wrote)==MUSA_MC2_OK);
    memset(down,0x4b,old_size);
    assert(musa_mc1032_downgrade_v2_to_v1(modified,modified_size,
          down,old_size,&wrote)==MUSA_MC2_UNSUPPORTED);
    for(size_t i=0;i<old_size;i++)assert(down[i]==0x4b);
    /* Even 2D entities on named layers cannot be flattened silently. */
    assert(musa_mc2_scene_translate(scene,100,0,0,-1.25)==MUSA_MC2_OK);
    uint32_t colored=0;
    assert(musa_mc2_scene_add_layer(scene,"MECHANICAL",0x223344ff,&colored)==MUSA_MC2_OK);
    modified_size=0;
    assert(musa_mc2_encode(scene,NULL,0,&modified_size)==MUSA_MC2_OK);
    free(modified);modified=(unsigned char *)malloc(modified_size);
    assert(modified);
    assert(musa_mc2_encode(scene,modified,modified_size,&wrote)==MUSA_MC2_OK);
    assert(musa_mc1032_downgrade_v2_to_v1(modified,modified_size,
            down,old_size,&wrote)==MUSA_MC2_UNSUPPORTED);
    musa_mc2_scene_destroy(scene);
    /* Different revision/malformed bytes must not be rebranded. */
    assert(musa_mc1032_upgrade_v1_to_v2(
        (const unsigned char *)"AC1032",6,NULL,0,&wrote)==MUSA_MC2_INVALID);
    assert(musa_mc1032_downgrade_v2_to_v1(old_data,old_size,NULL,0,&wrote)
           ==MUSA_MC2_UNSUPPORTED);
    /* Empty v1 documents can upgrade without losing their identity. */
    MusaDocument *empty=musa_document_create(1);
    assert(empty);
    unsigned char empty_v1[32]={0};
    assert(musa_mc1032_encode(empty,empty_v1,sizeof(empty_v1),&wrote)
           ==MUSA_MC1032_OK);
    unsigned char empty_v2[112]={0};
    assert(musa_mc1032_upgrade_v1_to_v2(empty_v1,32,empty_v2,sizeof(empty_v2),&wrote)
           ==MUSA_MC2_OK&&wrote==112);
    assert(musa_mc2_decode(empty_v2,112,&scene)==MUSA_MC2_OK);
    assert(musa_mc2_scene_entity_count(scene)==0);
    musa_mc2_scene_destroy(scene);musa_document_destroy(empty);
    musa_document_destroy(doc);
    free(modified);free(old_data);free(new_data);free(down);
    puts("PASS: MC1032 first-party lossless v1-v2 migration and rejection of data loss");
    return 0;
}
