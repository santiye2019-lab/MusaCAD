/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_mc1032_migrate.h"
#include "musacad_mc1032.h"
#include "musacad_document.h"
#include <stdint.h>

static int v1_error(int code){
    switch(code){
      case MUSA_MC1032_OK:return MUSA_MC2_OK;
      case MUSA_MC1032_UNSUPPORTED:return MUSA_MC2_UNSUPPORTED;
      case MUSA_MC1032_CORRUPT:return MUSA_MC2_CORRUPT;
      case MUSA_MC1032_LIMIT:return MUSA_MC2_LIMIT;
      case MUSA_MC1032_NO_MEMORY:return MUSA_MC2_NO_MEMORY;
      default:return MUSA_MC2_INVALID;
    }
}
int musa_mc1032_upgrade_v1_to_v2(
    const unsigned char *src,size_t src_len,unsigned char *dst,
    size_t capacity,size_t *written){
    if(!written)return MUSA_MC2_INVALID;
    *written=0;
    if(!src||(!dst&&capacity!=0u))return MUSA_MC2_INVALID;
    MusaDocument *old=NULL;
    const int read=musa_mc1032_decode(src,src_len,&old);
    if(read!=MUSA_MC1032_OK)return v1_error(read);
    const size_t count=musa_document_count(old);
    MusaMC2Scene *scene=musa_mc2_scene_create((uint32_t)(count?count:1u));
    if(!scene){
        musa_document_destroy(old);
        return MUSA_MC2_NO_MEMORY;
    }
    int status=MUSA_MC2_OK;
    for(size_t i=0;i<count;i++){
        MusaDocumentEntity e={0};
        if(musa_document_get_index(old,i,&e)!=MUSA_DOCUMENT_OK){
            status=MUSA_MC2_CORRUPT;break;
        }
        MusaMC2Entity neo={0};
        neo.handle=e.handle;neo.layer=0u;
        neo.x1=e.x1;neo.y1=e.y1;neo.z1=0.0;
        if(e.kind==MUSA_DOCUMENT_LINE){
            neo.kind=MUSA_MC2_LINE3D;
            neo.x2=e.x2;neo.y2=e.y2;
        }else if(e.kind==MUSA_DOCUMENT_POINT){
            neo.kind=MUSA_MC2_POINT3D;
        }else{
            status=MUSA_MC2_UNSUPPORTED;break; /* CIRCLE/ARC cannot be lost */
        }
        uint64_t actual=0;
        status=musa_mc2_scene_add_entity(scene,&neo,&actual);
        if(status!=MUSA_MC2_OK||actual!=neo.handle){
            if(status==MUSA_MC2_OK)status=MUSA_MC2_CORRUPT;
            break;
        }
    }
    musa_document_destroy(old);
    if(status==MUSA_MC2_OK)
        status=musa_mc2_encode(scene,dst,capacity,written);
    musa_mc2_scene_destroy(scene);
    return status;
}
int musa_mc1032_downgrade_v2_to_v1(
    const unsigned char *src,size_t src_len,unsigned char *dst,
    size_t capacity,size_t *written){
    if(!written)return MUSA_MC2_INVALID;
    *written=0;
    if(!src||(!dst&&capacity!=0u))return MUSA_MC2_INVALID;
    MusaMC2Scene *newer=NULL;
    int status=musa_mc2_decode(src,src_len,&newer);
    if(status!=MUSA_MC2_OK)return status;
    const uint32_t count=musa_mc2_scene_entity_count(newer);
    if(musa_mc2_scene_layer_count(newer)!=1u){
        musa_mc2_scene_destroy(newer);
        return MUSA_MC2_UNSUPPORTED; /* named/colored layers cannot be dropped */
    }
    MusaDocument *legacy=musa_document_create(count?count:1u);
    if(!legacy){
        musa_mc2_scene_destroy(newer);
        return MUSA_MC2_NO_MEMORY;
    }
    for(uint32_t i=0;i<count;i++){
        MusaMC2Entity e={0};
        if(musa_mc2_scene_get_index(newer,i,&e)!=MUSA_MC2_OK){
            status=MUSA_MC2_CORRUPT;break;
        }
        if(e.layer!=0u||e.z1!=0.0||e.z2!=0.0){
            status=MUSA_MC2_UNSUPPORTED;break;
        }
        MusaDocumentEntity old={0};
        old.handle=e.handle;old.x1=e.x1;old.y1=e.y1;
        if(e.kind==MUSA_MC2_LINE3D){
            old.kind=MUSA_DOCUMENT_LINE;
            old.x2=e.x2;old.y2=e.y2;
        }else if(e.kind==MUSA_MC2_POINT3D){
            old.kind=MUSA_DOCUMENT_POINT;
        }else{
            status=MUSA_MC2_UNSUPPORTED;break;
        }
        uint64_t actual=0;
        const int loaded=musa_document_load_entity(legacy,&old,&actual);
        if(loaded!=MUSA_DOCUMENT_OK||actual!=old.handle){
            status=loaded==MUSA_DOCUMENT_NO_MEMORY?
                MUSA_MC2_NO_MEMORY:MUSA_MC2_CORRUPT;
            break;
        }
    }
    musa_mc2_scene_destroy(newer);
    if(status==MUSA_MC2_OK)
        status=v1_error(musa_mc1032_encode(legacy,dst,capacity,written));
    musa_document_destroy(legacy);
    return status;
}
