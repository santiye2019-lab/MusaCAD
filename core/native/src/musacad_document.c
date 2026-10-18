/* SPDX-License-Identifier: GPL-3.0-or-later */
/* MusaCAD first-party document storage and editing implementation. */
#include "musacad_document.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MUSA_DOCUMENT_MAX_ENTITIES 1000000u
struct MusaDocument {
    MusaDocumentEntity *entities;
    size_t count,capacity,maximum;
    uint64_t next_handle;
};

unsigned int musa_document_abi_version(void){return MUSA_DOCUMENT_ABI_VERSION;}

MusaDocument *musa_document_create(size_t max_entities){
    if(max_entities==0||max_entities>MUSA_DOCUMENT_MAX_ENTITIES)return NULL;
    MusaDocument *doc=(MusaDocument *)calloc(1,sizeof(*doc));
    if(!doc)return NULL;
    doc->maximum=max_entities;
    doc->next_handle=1u;
    return doc;
}
void musa_document_destroy(MusaDocument *doc){
    if(!doc)return;
    if(doc->entities){
        memset(doc->entities,0,doc->capacity*sizeof(*doc->entities));
        free(doc->entities);
    }
    memset(doc,0,sizeof(*doc));
    free(doc);
}
size_t musa_document_count(const MusaDocument *doc){return doc?doc->count:0u;}

static int valid_point(double x,double y){
    return isfinite(x)&&isfinite(y);
}
static int valid_radius(double radius){
    return isfinite(radius)&&radius>0.0;
}
static int ensure_slot(MusaDocument *doc){
    if(doc->count>=doc->maximum)return MUSA_DOCUMENT_LIMIT;
    if(doc->next_handle==0u || doc->next_handle==UINT64_MAX)
        return MUSA_DOCUMENT_OVERFLOW;
    if(doc->count<doc->capacity)return MUSA_DOCUMENT_OK;
    size_t cap=doc->capacity?doc->capacity*2u:16u;
    if(cap>doc->maximum)cap=doc->maximum;
    if(cap>SIZE_MAX/sizeof(*doc->entities))
        return MUSA_DOCUMENT_NO_MEMORY;
    void *grown=realloc(doc->entities,cap*sizeof(*doc->entities));
    if(!grown)return MUSA_DOCUMENT_NO_MEMORY;
    doc->entities=(MusaDocumentEntity *)grown;
    doc->capacity=cap;
    return MUSA_DOCUMENT_OK;
}
static int insert(MusaDocument *doc,MusaDocumentEntity item,
                  uint64_t *out_handle){
    if(!doc||!out_handle)return MUSA_DOCUMENT_INVALID;
    int status=ensure_slot(doc);
    if(status!=MUSA_DOCUMENT_OK)return status;
    item.handle=doc->next_handle;
    doc->entities[doc->count]=item;
    ++doc->count;
    ++doc->next_handle;
    *out_handle=item.handle;
    return MUSA_DOCUMENT_OK;
}
int musa_document_add_line(MusaDocument *doc,double x1,double y1,
                           double x2,double y2,uint64_t *out_handle){
    if(!valid_point(x1,y1)||!valid_point(x2,y2))
        return MUSA_DOCUMENT_INVALID;
    MusaDocumentEntity item={0};
    item.kind=MUSA_DOCUMENT_LINE;
    item.x1=x1;item.y1=y1;item.x2=x2;item.y2=y2;
    return insert(doc,item,out_handle);
}
int musa_document_add_circle(MusaDocument *doc,double cx,double cy,
                             double radius,uint64_t *out_handle){
    if(!valid_point(cx,cy)||!valid_radius(radius))
        return MUSA_DOCUMENT_INVALID;
    MusaDocumentEntity item={0};
    item.kind=MUSA_DOCUMENT_CIRCLE;
    item.x1=cx;item.y1=cy;item.radius=radius;
    return insert(doc,item,out_handle);
}
int musa_document_add_arc(MusaDocument *doc,double cx,double cy,
                          double radius,double start_deg,double end_deg,
                          uint64_t *out_handle){
    if(!valid_point(cx,cy)||!valid_radius(radius)||
       !isfinite(start_deg)||!isfinite(end_deg))
        return MUSA_DOCUMENT_INVALID;
    MusaDocumentEntity item={0};
    item.kind=MUSA_DOCUMENT_ARC;
    item.x1=cx;item.y1=cy;item.radius=radius;
    item.start_deg=start_deg;item.end_deg=end_deg;
    return insert(doc,item,out_handle);
}
int musa_document_add_point(MusaDocument *doc,double x,double y,
                            uint64_t *out_handle){
    if(!valid_point(x,y))return MUSA_DOCUMENT_INVALID;
    MusaDocumentEntity item={0};
    item.kind=MUSA_DOCUMENT_POINT;item.x1=x;item.y1=y;
    return insert(doc,item,out_handle);
}
static size_t locate(const MusaDocument *doc,uint64_t handle){
    if(!doc||handle==0u)return SIZE_MAX;
    for(size_t i=0;i<doc->count;i++)
        if(doc->entities[i].handle==handle)return i;
    return SIZE_MAX;
}
int musa_document_get(const MusaDocument *doc,uint64_t handle,
                      MusaDocumentEntity *out_entity){
    if(!doc||!out_entity||handle==0u)return MUSA_DOCUMENT_INVALID;
    size_t at=locate(doc,handle);
    if(at==SIZE_MAX)return MUSA_DOCUMENT_NOT_FOUND;
    *out_entity=doc->entities[at];return MUSA_DOCUMENT_OK;
}
int musa_document_get_index(const MusaDocument *doc,size_t index,
                            MusaDocumentEntity *out_entity){
    if(!doc||!out_entity)return MUSA_DOCUMENT_INVALID;
    if(index>=doc->count)return MUSA_DOCUMENT_NOT_FOUND;
    *out_entity=doc->entities[index];return MUSA_DOCUMENT_OK;
}
int musa_document_translate(MusaDocument *doc,uint64_t handle,
                            double dx,double dy){
    if(!doc||handle==0u||!valid_point(dx,dy))
        return MUSA_DOCUMENT_INVALID;
    size_t at=locate(doc,handle);
    if(at==SIZE_MAX)return MUSA_DOCUMENT_NOT_FOUND;
    MusaDocumentEntity candidate=doc->entities[at];
    candidate.x1+=dx;candidate.y1+=dy;
    if(candidate.kind==MUSA_DOCUMENT_LINE){
        candidate.x2+=dx;candidate.y2+=dy;
    }
    if(!valid_point(candidate.x1,candidate.y1)||
       !valid_point(candidate.x2,candidate.y2))
        return MUSA_DOCUMENT_OVERFLOW;
    doc->entities[at]=candidate;
    return MUSA_DOCUMENT_OK;
}
int musa_document_delete(MusaDocument *doc,uint64_t handle){
    if(!doc||handle==0u)return MUSA_DOCUMENT_INVALID;
    size_t at=locate(doc,handle);
    if(at==SIZE_MAX)return MUSA_DOCUMENT_NOT_FOUND;
    if(at+1u<doc->count)
        memmove(doc->entities+at,doc->entities+at+1u,
                (doc->count-at-1u)*sizeof(*doc->entities));
    --doc->count;
    memset(doc->entities+doc->count,0,sizeof(*doc->entities));
    return MUSA_DOCUMENT_OK;
}
