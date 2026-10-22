/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_class_resolution.h"
#include "musacad_dwg_ac1032_object_type.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CLASS_SOURCE_FILE (64u*1024u*1024u)
#define MAX_REGISTERED_CLASSES 4096u
static int overlaps(const void *a,size_t an,const void *b,size_t bn){
    if(!an||!bn)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<an:x-y<bn;
}
static int nonempty_terminated(const char *p,size_t len){
    /* Names must be nonempty and NUL-terminated in their fixed field. */
    return p&&p[0]&&memchr(p,0,len)!=NULL;
}
int musa_dwg_ac1032_class_join(
    uint16_t type,uint64_t handle,const MusaDwgAc1032ClassNumeric *nums,
    const MusaDwgAc1032ClassNames *names,size_t n,
    MusaDwgAc1032ClassResolution *out){
    if(!out||!type||!handle||n>MAX_REGISTERED_CLASSES||
       (n&&(!nums||!names)))return 0;
    if(n>SIZE_MAX/sizeof(*nums)||
       n>SIZE_MAX/sizeof(*names))return 0;
    if(n&&(overlaps(out,sizeof(*out),nums,n*sizeof(*nums))||
           overlaps(out,sizeof(*out),names,n*sizeof(*names))))
       return 0;
    MusaDwgAc1032ClassResolution value={0};
    value.indexed_handle=handle;
    value.raw_object_type=type;
    if(type<500u){*out=value;return 1;}
    if(!n)return 0;
    size_t pos=0u;uint16_t prev=0u;
    /* Fail CLOSED for duplicated, unsorted, unmatched ID or missing
     * Unicode strings; this is a source-join, not string heuristics. */
    for(size_t i=0;i<n;i++){
        const MusaDwgAc1032ClassNumeric *rec=&nums[i];
        const MusaDwgAc1032ClassNames *name=&names[i];
        if(rec->class_number<500u||
           (i&&rec->class_number<=prev)||
           name->class_number!=rec->class_number||
           (rec->item_class_id!=0x1f2u&&rec->item_class_id!=0x1f3u)||
           !nonempty_terminated(name->application,sizeof(name->application))||
           !nonempty_terminated(name->cpp_class,sizeof(name->cpp_class))||
           !nonempty_terminated(name->dxf_name,sizeof(name->dxf_name)))
            return 0;
        prev=rec->class_number;
        if(rec->class_number==type){pos=i+1u;}
    }
    if(!pos)return 0;
    const MusaDwgAc1032ClassNumeric *rec=&nums[pos-1u];
    const MusaDwgAc1032ClassNames *name=&names[pos-1u];
    value.class_number=rec->class_number;
    value.item_class_id=rec->item_class_id;
    value.proxy_flags=rec->proxy_flags;
    value.dwg_version=rec->dwg_version;
    value.maintenance_version=rec->maintenance_version;
    value.is_custom_class=1u;
    value.was_proxy=rec->was_proxy;
    value.source_class_metadata_verified=1u;
    memcpy(value.application,name->application,sizeof(value.application));
    memcpy(value.cpp_class,name->cpp_class,sizeof(value.cpp_class));
    memcpy(value.dxf_name,name->dxf_name,sizeof(value.dxf_name));
    *out=value;return 1;
}
static int load_classes(const uint8_t *file,size_t bytes,
    MusaDwgAc1032ClassNumeric **id_out,
    MusaDwgAc1032ClassNames **name_out,size_t *n_out){
    size_t n=0u;
    if(!musa_dwg_ac1032_classes_numeric_records(
         file,bytes,NULL,0u,&n)||
       !n||n>MAX_REGISTERED_CLASSES)return 0;
    MusaDwgAc1032ClassNumeric *id=calloc(n,sizeof(*id));
    MusaDwgAc1032ClassNames *names=calloc(n,sizeof(*names));
    if(!id||!names){free(id);free(names);return 0;}
    size_t nr=0u,nn=0u;
    if(!musa_dwg_ac1032_classes_numeric_records(
          file,bytes,id,n,&nr)||
       !musa_dwg_ac1032_classes_names(file,bytes,names,n,&nn)||
       nr!=n||nn!=n){
        free(id);free(names);return 0;
    }
    *id_out=id;*name_out=names;*n_out=n;return 1;
}
static int resolve(const uint8_t *file,size_t bytes,
     uint16_t type,uint64_t handle,uint8_t original_crc,
     MusaDwgAc1032ClassResolution *out){
    MusaDwgAc1032ClassNumeric *num=NULL;
    MusaDwgAc1032ClassNames *name=NULL;
    size_t n=0u;
    if(!load_classes(file,bytes,&num,&name,&n))return 0;
    MusaDwgAc1032ClassResolution found={0};
    int ok=musa_dwg_ac1032_class_join(type,handle,num,name,n,&found);
    free(num);free(name);
    if(!ok)return 0;
    found.original_record_crc_verified=original_crc;
    *out=found;return 1;
}
int musa_dwg_ac1032_resolve_object_class_file(
    const uint8_t *file,size_t bytes,uint64_t handle,
    MusaDwgAc1032ClassResolution *out){
    if(!file||!out||!handle||bytes<256u||bytes>MAX_CLASS_SOURCE_FILE||
       overlaps(file,bytes,out,sizeof(*out)))return 0;
    MusaDwgAc1032ObjectType type={0};
    if(!musa_dwg_ac1032_object_type_file(file,bytes,handle,&type)||
       !type.record_crc_verified||!type.type_code)return 0;
    return resolve(file,bytes,type.type_code,handle,1u,out);
}
int musa_dwg_ac1032_resolve_declared_class_file(
    const uint8_t *file,size_t bytes,uint16_t class_number,
    MusaDwgAc1032ClassResolution *out){
    if(!file||!out||class_number<500u||bytes<256u||
       bytes>MAX_CLASS_SOURCE_FILE||
       overlaps(file,bytes,out,sizeof(*out)))return 0;
    return resolve(file,bytes,class_number,1u,0u,out);
}
