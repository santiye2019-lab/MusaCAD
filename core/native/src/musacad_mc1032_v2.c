/* SPDX-License-Identifier: GPL-3.0-or-later */
/* MusaCAD-owned MC1032 rev2 layered 3D document + binary codec.
 * No DWG parser, no external CAD SDK, no platform-specific headers.
 */
#include "musacad_mc1032_v2.h"
#include "musacad_handle_index.h"
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct MusaMC2Scene {
    uint32_t maximum,count,layer_count;
    uint64_t next_handle;
    MusaMC2Layer layers[MUSA_MC2_MAX_LAYERS];
    MusaMC2Entity *entities;
    MusaHandleIndex *handles;
};
static const unsigned char magic[6]={'M','C','1','0','3','2'};
static int ieee64(void) {
    return sizeof(double)==8u&&sizeof(uint64_t)==8u &&
           FLT_RADIX==2&&DBL_MANT_DIG==53&&DBL_MAX_EXP==1024;
}
static int good_name(const char *name,size_t *length){
    if(!name)return 0;
    size_t n=0;
    while(n<=MUSA_MC2_LAYER_NAME_BYTES && name[n]){
        unsigned char c=(unsigned char)name[n];
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
             (c>='0'&&c<='9')||c=='_'||c=='-'))return 0;
        n++;
    }
    if(n==0||n>MUSA_MC2_LAYER_NAME_BYTES)return 0;
    if(length)*length=n;
    return 1;
}
static int entity_valid(const MusaMC2Scene *s,const MusaMC2Entity *e){
    if(!s||!e||e->layer>=s->layer_count||
       !isfinite(e->x1)||!isfinite(e->y1)||!isfinite(e->z1)||
       !isfinite(e->x2)||!isfinite(e->y2)||!isfinite(e->z2))return 0;
    if(e->kind==MUSA_MC2_POINT3D)
        return e->x2==0&&e->y2==0&&e->z2==0;
    return e->kind==MUSA_MC2_LINE3D;
}
MusaMC2Scene *musa_mc2_scene_create(uint32_t maximum){
    if(!maximum||maximum>MUSA_MC2_MAX_ENTITIES||!ieee64())return NULL;
    MusaMC2Scene *s=(MusaMC2Scene *)calloc(1,sizeof(*s));
    if(!s)return NULL;
    s->entities=(MusaMC2Entity *)calloc(maximum,sizeof(*s->entities));
    if(!s->entities){free(s);return NULL;}
    s->handles=musa_handle_index_create(maximum);
    if(!s->handles){free(s->entities);free(s);return NULL;}
    s->maximum=maximum;s->next_handle=1;
    s->layer_count=1;
    strcpy(s->layers[0].name,"0");
    s->layers[0].rgba=0xffffffffu;
    s->layers[0].visible=1u;
    return s;
}
void musa_mc2_scene_destroy(MusaMC2Scene *s){
    if(!s)return;
    if(s->entities){
        memset(s->entities,0,(size_t)s->maximum*sizeof(*s->entities));
        free(s->entities);
    }
    musa_handle_index_destroy(s->handles);
    memset(s,0,sizeof(*s));free(s);
}
uint32_t musa_mc2_scene_entity_count(const MusaMC2Scene *s){return s?s->count:0;}
uint32_t musa_mc2_scene_layer_count(const MusaMC2Scene *s){return s?s->layer_count:0;}
int musa_mc2_scene_add_layer(MusaMC2Scene *s,const char *name,
                             uint32_t rgba,uint32_t *id){
    size_t n=0;
    if(!s||!id||!good_name(name,&n))return MUSA_MC2_INVALID;
    if(s->layer_count>=MUSA_MC2_MAX_LAYERS)return MUSA_MC2_LIMIT;
    for(uint32_t i=0;i<s->layer_count;i++)
        if(strcmp(s->layers[i].name,name)==0)return MUSA_MC2_INVALID;
    MusaMC2Layer *l=&s->layers[s->layer_count];
    memset(l,0,sizeof(*l));
    memcpy(l->name,name,n);
    l->rgba=rgba;l->visible=1;
    *id=s->layer_count++;
    return MUSA_MC2_OK;
}
int musa_mc2_scene_get_layer(const MusaMC2Scene *s,uint32_t id,MusaMC2Layer *out){
    if(!s||!out)return MUSA_MC2_INVALID;
    if(id>=s->layer_count)return MUSA_MC2_NOT_FOUND;
    *out=s->layers[id];return MUSA_MC2_OK;
}
static uint32_t locate(const MusaMC2Scene *s,uint64_t handle){
    if(!s||!handle)return UINT32_MAX;
    uint32_t slot=UINT32_MAX;
    return musa_handle_index_get(s->handles,handle,&slot)==1
       ?slot:UINT32_MAX;
}
int musa_mc2_scene_add_entity(MusaMC2Scene *s,const MusaMC2Entity *in,
                              uint64_t *actual_handle){
    if(!s||!actual_handle||!entity_valid(s,in))return MUSA_MC2_INVALID;
    if(s->count>=s->maximum)return MUSA_MC2_LIMIT;
    uint64_t id=in->handle?in->handle:s->next_handle;
    if(id==0u||id==UINT64_MAX)return MUSA_MC2_LIMIT;
    if(locate(s,id)!=UINT32_MAX)return MUSA_MC2_INVALID;
    const int indexed=musa_handle_index_put(s->handles,id,s->count);
    if(indexed!=1)return indexed==-2?MUSA_MC2_LIMIT:MUSA_MC2_INVALID;
    MusaMC2Entity e=*in;e.handle=id;
    s->entities[s->count++]=e;
    if(id>=s->next_handle)s->next_handle=id+1u;
    *actual_handle=id;
    return MUSA_MC2_OK;
}
int musa_mc2_scene_get_index(const MusaMC2Scene *s,uint32_t index,
                             MusaMC2Entity *out){
    if(!s||!out)return MUSA_MC2_INVALID;
    if(index>=s->count)return MUSA_MC2_NOT_FOUND;
    *out=s->entities[index];
    return MUSA_MC2_OK;
}
int musa_mc2_scene_get(const MusaMC2Scene *s,uint64_t id,MusaMC2Entity *out){
    if(!s||!out||id==0u)return MUSA_MC2_INVALID;
    uint32_t i=locate(s,id);
    if(i==UINT32_MAX)return MUSA_MC2_NOT_FOUND;
    *out=s->entities[i];return MUSA_MC2_OK;
}
int musa_mc2_scene_translate(MusaMC2Scene *s,uint64_t id,
                             double dx,double dy,double dz){
    if(!s||!id||!isfinite(dx)||!isfinite(dy)||!isfinite(dz))
        return MUSA_MC2_INVALID;
    uint32_t i=locate(s,id);
    if(i==UINT32_MAX)return MUSA_MC2_NOT_FOUND;
    MusaMC2Entity e=s->entities[i];
    e.x1+=dx;e.y1+=dy;e.z1+=dz;
    if(e.kind==MUSA_MC2_LINE3D){
        e.x2+=dx;e.y2+=dy;e.z2+=dz;
    }
    if(!entity_valid(s,&e))return MUSA_MC2_LIMIT;
    s->entities[i]=e;
    return MUSA_MC2_OK;
}
int musa_mc2_scene_delete(MusaMC2Scene *s,uint64_t id){
    if(!s||!id)return MUSA_MC2_INVALID;
    uint32_t i=locate(s,id);
    if(i==UINT32_MAX)return MUSA_MC2_NOT_FOUND;
    if(musa_handle_index_remove(s->handles,id)!=1)
        return MUSA_MC2_CORRUPT;
    if(i+1u<s->count)
        memmove(s->entities+i,s->entities+i+1u,
                (size_t)(s->count-i-1u)*sizeof(*s->entities));
    --s->count;
    memset(s->entities+s->count,0,sizeof(*s->entities));
    for(uint32_t k=i;k<s->count;k++)
        (void)musa_handle_index_set(s->handles,s->entities[k].handle,k);
    return MUSA_MC2_OK;
}
static void u32put(unsigned char *p,uint32_t v){
    for(unsigned j=0;j<4;j++)p[j]=(unsigned char)(v>>(8u*j));
}
static void u64put(unsigned char *p,uint64_t v){
    for(unsigned j=0;j<8;j++)p[j]=(unsigned char)(v>>(8u*j));
}
static uint32_t u32get(const unsigned char *p){
    uint32_t v=0u;
    for(unsigned j=0;j<4;j++)v|=(uint32_t)p[j]<<(8u*j);
    return v;
}
static uint64_t u64get(const unsigned char *p){
    uint64_t v=0u;
    for(unsigned j=0;j<8;j++)v|=(uint64_t)p[j]<<(8u*j);
    return v;
}
static void fput(unsigned char *p,double f){
    uint64_t bits;memcpy(&bits,&f,8u);u64put(p,bits);
}
static double fget(const unsigned char *p){
    uint64_t bits=u64get(p);double f;memcpy(&f,&bits,8u);return f;
}
static uint32_t crc(const unsigned char *p,size_t n){
    uint32_t c=0xffffffffu;
    for(size_t i=0;i<n;i++){
        c^=p[i];
        for(unsigned j=0;j<8u;j++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));
    }
    return ~c;
}
static size_t byte_count(const MusaMC2Scene *s){
    return (size_t)MUSA_MC2_HEADER_SIZE+
        (size_t)s->layer_count*MUSA_MC2_LAYER_SIZE+
        (size_t)s->count*MUSA_MC2_RECORD_SIZE;
}
int musa_mc2_encode(const MusaMC2Scene *s,unsigned char *dst,
                    size_t capacity,size_t *written){
    if(!written)return MUSA_MC2_INVALID;
    *written=0;
    if(!s||!ieee64()||(!dst&&capacity!=0u))
        return MUSA_MC2_INVALID;
    if(s->count>MUSA_MC2_MAX_ENTITIES||s->layer_count<1u||
       s->layer_count>MUSA_MC2_MAX_LAYERS)return MUSA_MC2_LIMIT;
    const size_t n=byte_count(s);
    *written=n;
    if(!dst)return MUSA_MC2_OK;
    if(capacity<n)return MUSA_MC2_LIMIT;
    unsigned char *stage=(unsigned char *)calloc(n,1);
    if(!stage)return MUSA_MC2_NO_MEMORY;
    int status=MUSA_MC2_OK;
    memcpy(stage,magic,6u);
    stage[6]=2; /* own v2 schema */
    u32put(stage+8,0x01020304u);
    u32put(stage+12,MUSA_MC2_HEADER_SIZE);
    u32put(stage+16,MUSA_MC2_LAYER_SIZE);
    u32put(stage+20,MUSA_MC2_RECORD_SIZE);
    u32put(stage+24,s->layer_count);
    u32put(stage+28,s->count);
    u32put(stage+32,(uint32_t)(n-MUSA_MC2_HEADER_SIZE));
    for(uint32_t i=0;i<s->layer_count;i++){
        const MusaMC2Layer *l=&s->layers[i];
        size_t len=0;
        if(!good_name(l->name,&len)||l->visible!=1u){
            status=MUSA_MC2_UNSUPPORTED;break;
        }
        unsigned char *p=stage+MUSA_MC2_HEADER_SIZE+
                         (size_t)i*MUSA_MC2_LAYER_SIZE;
        u32put(p,i);u32put(p+4,l->rgba);
        u32put(p+8,l->visible);u32put(p+12,(uint32_t)len);
        memcpy(p+16,l->name,len);
    }
    if(status==MUSA_MC2_OK){
        for(uint32_t i=0;i<s->count;i++){
            const MusaMC2Entity *e=&s->entities[i];
            if(!entity_valid(s,e)||!e->handle){
                status=MUSA_MC2_UNSUPPORTED;break;
            }
            unsigned char *p=stage+MUSA_MC2_HEADER_SIZE+
                  (size_t)s->layer_count*MUSA_MC2_LAYER_SIZE+
                  (size_t)i*MUSA_MC2_RECORD_SIZE;
            u64put(p,e->handle);u32put(p+8,e->kind);u32put(p+12,e->layer);
            const double fields[6]={e->x1,e->y1,e->z1,e->x2,e->y2,e->z2};
            for(unsigned k=0;k<6;k++)fput(p+16+8*k,fields[k]);
        }
    }
    if(status==MUSA_MC2_OK){
        u32put(stage+36,crc(stage+MUSA_MC2_HEADER_SIZE,
                            n-MUSA_MC2_HEADER_SIZE));
        u32put(stage+40,crc(stage,40));
        memcpy(dst,stage,n);
    }
    memset(stage,0,n);free(stage);
    return status;
}
int musa_mc2_decode(const unsigned char *data,size_t n,MusaMC2Scene **out){
    if(!out)return MUSA_MC2_INVALID;
    *out=NULL;
    if(!data||n<MUSA_MC2_HEADER_SIZE||!ieee64())return MUSA_MC2_INVALID;
    if(n>MUSA_MC2_MAX_BYTES)return MUSA_MC2_LIMIT;
    if(memcmp(data,magic,6u))return MUSA_MC2_INVALID;
    if(data[6]!=2u)return MUSA_MC2_UNSUPPORTED;
    if(data[7]!=0u||u32get(data+8)!=0x01020304u||
       u32get(data+12)!=MUSA_MC2_HEADER_SIZE||
       u32get(data+16)!=MUSA_MC2_LAYER_SIZE||
       u32get(data+20)!=MUSA_MC2_RECORD_SIZE||
       u32get(data+44)!=0u)return MUSA_MC2_UNSUPPORTED;
    const uint32_t layers=u32get(data+24);
    const uint32_t count=u32get(data+28);
    if(layers<1u||layers>MUSA_MC2_MAX_LAYERS||count>MUSA_MC2_MAX_ENTITIES)
        return MUSA_MC2_LIMIT;
    const size_t wanted=(size_t)MUSA_MC2_HEADER_SIZE+
       (size_t)layers*MUSA_MC2_LAYER_SIZE+
       (size_t)count*MUSA_MC2_RECORD_SIZE;
    if(n!=wanted||u32get(data+32)!=(uint32_t)(wanted-MUSA_MC2_HEADER_SIZE))
        return MUSA_MC2_CORRUPT;
    if(u32get(data+40)!=crc(data,40)||
       u32get(data+36)!=crc(data+MUSA_MC2_HEADER_SIZE,
                           n-MUSA_MC2_HEADER_SIZE))return MUSA_MC2_CORRUPT;
    /* The on-disk record count is current occupancy, not a scene capacity.
     * Opened documents must remain editable up to the bounded v2 limit.
     * Fixed-capacity allocation also keeps handle-index slot coverage valid. */
    MusaMC2Scene *s=musa_mc2_scene_create(MUSA_MC2_MAX_ENTITIES);
    if(!s)return MUSA_MC2_NO_MEMORY;
    int status=MUSA_MC2_OK;
    for(uint32_t i=0;i<layers;i++){
        const unsigned char *p=data+MUSA_MC2_HEADER_SIZE+
                               (size_t)i*MUSA_MC2_LAYER_SIZE;
        const uint32_t id=u32get(p);
        const uint32_t name_len=u32get(p+12);
        if(id!=i||name_len==0u||name_len>MUSA_MC2_LAYER_NAME_BYTES||
           u32get(p+8)!=1u){
            status=MUSA_MC2_UNSUPPORTED;break;
        }
        char name[MUSA_MC2_LAYER_NAME_BYTES+1u]={0};
        memcpy(name,p+16,name_len);
        size_t valid_len=0;
        if(!good_name(name,&valid_len)||valid_len!=name_len){
            status=MUSA_MC2_UNSUPPORTED;break;
        }
        for(size_t j=name_len;j<MUSA_MC2_LAYER_NAME_BYTES;j++){
            if(p[16+j]!=0){status=MUSA_MC2_UNSUPPORTED;break;}
        }
        if(status!=MUSA_MC2_OK)break;
        if(i==0u){
            if(strcmp(name,"0")||u32get(p+4)!=0xffffffffu){
                status=MUSA_MC2_UNSUPPORTED;break;
            }
        }else{
            uint32_t actual=0;
            if(musa_mc2_scene_add_layer(s,name,u32get(p+4),&actual)
               !=MUSA_MC2_OK||actual!=i){
                status=MUSA_MC2_UNSUPPORTED;break;
            }
        }
    }
    if(status==MUSA_MC2_OK){
        for(uint32_t i=0;i<count;i++){
            const unsigned char *p=data+MUSA_MC2_HEADER_SIZE+
                (size_t)layers*MUSA_MC2_LAYER_SIZE+
                (size_t)i*MUSA_MC2_RECORD_SIZE;
            MusaMC2Entity e={0};
            e.handle=u64get(p);
            e.kind=u32get(p+8);e.layer=u32get(p+12);
            double *fields[6]={&e.x1,&e.y1,&e.z1,&e.x2,&e.y2,&e.z2};
            for(unsigned k=0;k<6;k++)*fields[k]=fget(p+16+8*k);
            for(unsigned k=64;k<MUSA_MC2_RECORD_SIZE;k++){
                if(p[k]!=0){status=MUSA_MC2_UNSUPPORTED;break;}
            }
            if(status!=MUSA_MC2_OK)break;
            if(!e.handle||!entity_valid(s,&e)){
                status=MUSA_MC2_UNSUPPORTED;break;
            }
            uint64_t got=0;
            int r=musa_mc2_scene_add_entity(s,&e,&got);
            if(r!=MUSA_MC2_OK||got!=e.handle){
                status=r==MUSA_MC2_NO_MEMORY?MUSA_MC2_NO_MEMORY:MUSA_MC2_CORRUPT;
                break;
            }
        }
    }
    if(status==MUSA_MC2_OK){
        *out=s;return MUSA_MC2_OK;
    }
    musa_mc2_scene_destroy(s);
    return status;
}
