/* SPDX-License-Identifier: GPL-3.0-or-later */
/* MC1032 v1 independent binary codec: no Libredwg, ODA, libc file I/O,
 * OS SDK, platform byte order assumptions or binary DWG compatibility.
 */
#include "musacad_mc1032.h"
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MC_HEADER 32u
#define MC_RECORD 72u
#define MC_MAX MUSA_MC1032_MAX_ENTITIES
static const unsigned char magic[6]={'M','C','1','0','3','2'};

static int platform_compatible(void){
    return sizeof(double)==8u&&sizeof(uint64_t)==8u &&
           DBL_MANT_DIG==53&&DBL_MAX_EXP==1024&&FLT_RADIX==2;
}
static void put32(unsigned char *p,uint32_t v){
    for(unsigned i=0;i<4u;i++)p[i]=(unsigned char)(v>>(i*8u));
}
static void put64(unsigned char *p,uint64_t v){
    for(unsigned i=0;i<8u;i++)p[i]=(unsigned char)(v>>(i*8u));
}
static uint32_t get32(const unsigned char *p){
    uint32_t v=0;
    for(unsigned i=0;i<4u;i++)v|=(uint32_t)p[i]<<(8u*i);
    return v;
}
static uint64_t get64(const unsigned char *p){
    uint64_t v=0;
    for(unsigned i=0;i<8u;i++)v|=(uint64_t)p[i]<<(8u*i);
    return v;
}
static void put_double(unsigned char *p,double value){
    uint64_t bits=0;memcpy(&bits,&value,8u);put64(p,bits);
}
static double get_double(const unsigned char *p){
    uint64_t bits=get64(p);double value=0;
    memcpy(&value,&bits,8u);return value;
}
/* CRC-32/ISO-HDLC: reflected poly EDB88320, init/final xor FFFFFFFF. */
static uint32_t crc32(const unsigned char *bytes,size_t n){
    uint32_t crc=0xffffffffu;
    for(size_t i=0;i<n;i++){
        crc^=(uint32_t)bytes[i];
        for(unsigned b=0;b<8u;b++){
            const uint32_t mask=0u-(crc&1u);
            crc=(crc>>1u)^(0xedb88320u&mask);
        }
    }
    return ~crc;
}
static int valid_entity(const MusaDocumentEntity *e){
    if(!e||!e->handle||!isfinite(e->x1)||!isfinite(e->y1)||
       !isfinite(e->x2)||!isfinite(e->y2)||
       !isfinite(e->radius)||!isfinite(e->start_deg)||
       !isfinite(e->end_deg))return 0;
    switch(e->kind){
      case MUSA_DOCUMENT_LINE:
        return e->radius==0&&e->start_deg==0&&e->end_deg==0;
      case MUSA_DOCUMENT_POINT:
        return e->x2==0&&e->y2==0&&e->radius==0&&
               e->start_deg==0&&e->end_deg==0;
      case MUSA_DOCUMENT_CIRCLE:
        return e->x2==0&&e->y2==0&&e->radius>0&&
               e->start_deg==0&&e->end_deg==0;
      case MUSA_DOCUMENT_ARC:
        return e->x2==0&&e->y2==0&&e->radius>0;
      default:return 0;
    }
}
size_t musa_mc1032_encoded_size(const MusaDocument *doc){
    if(!doc||!platform_compatible())return 0u;
    size_t n=musa_document_count(doc);
    if(n>MC_MAX)return 0u;
    return MC_HEADER+MC_RECORD*n;
}
int musa_mc1032_encode(const MusaDocument *doc,unsigned char *output,
                       size_t capacity,size_t *written){
    if(!written)return MUSA_MC1032_INVALID;
    *written=0;
    if(!doc||(!output&&capacity!=0u)||!platform_compatible())
        return MUSA_MC1032_INVALID;
    const size_t count=musa_document_count(doc);
    if(count>MC_MAX)return MUSA_MC1032_LIMIT;
    const size_t required=MC_HEADER+MC_RECORD*count;
    *written=required;
    if(!output)return MUSA_MC1032_OK;
    if(capacity<required)return MUSA_MC1032_LIMIT;
    unsigned char *private_data=(unsigned char *)calloc(required,1u);
    if(!private_data)return MUSA_MC1032_NO_MEMORY;
    memcpy(private_data,magic,6u);
    private_data[6]=1u; /* MC1032 schema revision 1 */
    private_data[7]=0u; /* required flags=0 */
    put32(private_data+8u,0x01020304u); /* portable endian sentinel */
    put32(private_data+12u,MC_HEADER);
    put32(private_data+16u,MC_RECORD);
    put32(private_data+20u,(uint32_t)count);
    int status=MUSA_MC1032_OK;
    for(size_t i=0;i<count;i++){
        MusaDocumentEntity entity={0};
        if(musa_document_get_index(doc,i,&entity)!=MUSA_DOCUMENT_OK||
           !valid_entity(&entity)){
            status=MUSA_MC1032_UNSUPPORTED;break;
        }
        unsigned char *p=private_data+MC_HEADER+i*MC_RECORD;
        put64(p,entity.handle);
        put32(p+8u,(uint32_t)entity.kind);
        /* p[12..15] = zero flags */
        put_double(p+16u,entity.x1);
        put_double(p+24u,entity.y1);
        put_double(p+32u,entity.x2);
        put_double(p+40u,entity.y2);
        put_double(p+48u,entity.radius);
        put_double(p+56u,entity.start_deg);
        put_double(p+64u,entity.end_deg);
    }
    if(status==MUSA_MC1032_OK){
        put32(private_data+24u,crc32(private_data+MC_HEADER,
                                      required-MC_HEADER));
        put32(private_data+28u,crc32(private_data,28u));
        memcpy(output,private_data,required); /* publish only on success */
    }
    memset(private_data,0,required);
    free(private_data);
    return status;
}
int musa_mc1032_decode(const unsigned char *data,size_t n,
                       MusaDocument **out){
    if(!out)return MUSA_MC1032_INVALID;
    *out=NULL;
    if(!data||n<MC_HEADER||!platform_compatible())
        return MUSA_MC1032_INVALID;
    if(n>MUSA_MC1032_MAX_BYTES)return MUSA_MC1032_LIMIT;
    if(memcmp(data,magic,6u)!=0)return MUSA_MC1032_INVALID;
    if(data[6]!=1u||data[7]!=0u)return MUSA_MC1032_UNSUPPORTED;
    if(get32(data+8u)!=0x01020304u||get32(data+12u)!=MC_HEADER||
       get32(data+16u)!=MC_RECORD)return MUSA_MC1032_UNSUPPORTED;
    const uint32_t count=get32(data+20u);
    if(count>MC_MAX)return MUSA_MC1032_LIMIT;
    if(n!=MC_HEADER+(size_t)count*MC_RECORD)
        return MUSA_MC1032_CORRUPT; /* truncated or trailing objects */
    if(get32(data+28u)!=crc32(data,28u)||
       get32(data+24u)!=crc32(data+MC_HEADER,n-MC_HEADER))
        return MUSA_MC1032_CORRUPT;
    MusaDocument *doc=musa_document_create(count?count:1u);
    if(!doc)return MUSA_MC1032_NO_MEMORY;
    int status=MUSA_MC1032_OK;
    for(uint32_t i=0;i<count;i++){
        const unsigned char *p=data+MC_HEADER+(size_t)i*MC_RECORD;
        if(get32(p+12u)!=0u){
            status=MUSA_MC1032_UNSUPPORTED;break;
        }
        MusaDocumentEntity e={0};
        e.handle=get64(p);
        e.kind=(int)get32(p+8u);
        e.x1=get_double(p+16u);
        e.y1=get_double(p+24u);
        e.x2=get_double(p+32u);
        e.y2=get_double(p+40u);
        e.radius=get_double(p+48u);
        e.start_deg=get_double(p+56u);
        e.end_deg=get_double(p+64u);
        if(!valid_entity(&e)){
            status=MUSA_MC1032_UNSUPPORTED;break;
        }
        uint64_t actual=0u;
        int result=musa_document_load_entity(doc,&e,&actual);
        if(result==MUSA_DOCUMENT_NO_MEMORY)status=MUSA_MC1032_NO_MEMORY;
        else if(result!=MUSA_DOCUMENT_OK||actual!=e.handle)
            status=MUSA_MC1032_CORRUPT;
        if(status!=MUSA_MC1032_OK)break;
    }
    if(status==MUSA_MC1032_OK){
        *out=doc;
        return status;
    }
    musa_document_destroy(doc);
    return status;
}
