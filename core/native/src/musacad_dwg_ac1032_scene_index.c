/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_sections.h"
#include "musacad_dwg_ac1032_object_type.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1032_object_streams.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define MAX_ORIGINAL_DWG (64u*1024u*1024u)
#define MAX_OBJECT_SECTION (64u*1024u*1024u)
#define MAX_INDEX (1000000u)
#define MAX_CATALOG 256u
#define MAX_LW_VERTICES 100000u
struct MusaDwgAc1032SceneIndex {
    uint8_t *objects;
    size_t object_bytes;
    MusaDwgAc1032HandleEntry *handles;
    size_t count;
};
static int overlap(const void *a,size_t an,const void *b,size_t bn){
    if(!an||!bn)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<an:x-y<bn;
}
int musa_dwg_ac1032_scene_index_open(
    const uint8_t *file,size_t n,MusaDwgAc1032SceneIndex **out){
    if(!file||!out||n<256u||n>MAX_ORIGINAL_DWG||
       overlap(file,n,out,sizeof(*out))||
       memcmp(file,"AC1032",6u)!=0)return 0;
    /* An original 2018 file is validated in full by the existing
     * checksum-backed catalog+H reader BEFORE committing a cached index.
     * Do not cache an unsupported/partially decrypted drawing. */
    size_t cat_n=0u,h_n=0u;
    if(!musa_dwg_ac1032_section_catalog(file,n,NULL,0u,&cat_n)||
       !cat_n||cat_n>MAX_CATALOG||
       !musa_dwg_ac1032_handles_index(file,n,NULL,0u,&h_n)||
       !h_n||h_n>MAX_INDEX||
       h_n>SIZE_MAX/sizeof(MusaDwgAc1032HandleEntry))return 0;
    MusaDwgAc1032SectionCatalogEntry *cat=
      (MusaDwgAc1032SectionCatalogEntry*)calloc(cat_n,sizeof(*cat));
    if(!cat)return 0;
    size_t got=0u,object_n=0u,found=0u;
    int ok=musa_dwg_ac1032_section_catalog(
               file,n,cat,cat_n,&got)&&got==cat_n;
    if(ok)for(size_t i=0u;i<cat_n;i++){
       if(!strcmp((const char*)cat[i].name,"AcDb:AcDbObjects")){
          found++;
          if(!cat[i].all_physical_page_checksums_verified||
             !cat[i].logical_bytes||
             cat[i].logical_bytes>MAX_OBJECT_SECTION)ok=0;
          else object_n=(size_t)cat[i].logical_bytes;
       }
    }
    free(cat);
    if(!ok||found!=1u)return 0;
    MusaDwgAc1032SceneIndex *temporary=
      (MusaDwgAc1032SceneIndex*)calloc(1u,sizeof(*temporary));
    if(!temporary)return 0;
    temporary->handles=(MusaDwgAc1032HandleEntry*)
            calloc(h_n,sizeof(*temporary->handles));
    temporary->objects=(uint8_t*)malloc(object_n);
    if(!temporary->handles||!temporary->objects){
       musa_dwg_ac1032_scene_index_close(temporary);return 0;
    }
    size_t indexed=0u,read=0u;
    ok=musa_dwg_ac1032_handles_index(file,n,
            temporary->handles,h_n,&indexed)&&indexed==h_n&&
       musa_dwg_ac1032_extract_named_section(
            file,n,"AcDb:AcDbObjects",temporary->objects,
            object_n,&read)&&read==object_n;
    if(ok)for(size_t i=0u;i<h_n;i++){
        const MusaDwgAc1032HandleEntry *e=&temporary->handles[i];
        if(!e->handle||e->object_offset>=object_n||
           (i&&temporary->handles[i-1u].handle>=e->handle)){
            ok=0;break;
        }
    }
    if(!ok){
       musa_dwg_ac1032_scene_index_close(temporary);
       return 0;
    }
    temporary->object_bytes=object_n;
    temporary->count=h_n;
    *out=temporary;
    return 1;
}
void musa_dwg_ac1032_scene_index_close(MusaDwgAc1032SceneIndex *s){
    if(!s)return;
    free(s->handles);
    free(s->objects);
    free(s);
}
size_t musa_dwg_ac1032_scene_index_count(
    const MusaDwgAc1032SceneIndex *s){
    return s?s->count:0u;
}
int musa_dwg_ac1032_scene_index_entry(
    const MusaDwgAc1032SceneIndex *s,size_t at,
    MusaDwgAc1032HandleEntry *out){
    if(!s||!out||at>=s->count||
       overlap(s,sizeof(*s),out,sizeof(*out))||
       overlap(s->handles,s->count*sizeof(*s->handles),
               out,sizeof(*out))||
       overlap(s->objects,s->object_bytes,out,sizeof(*out)))return 0;
    *out=s->handles[at];return 1;
}
static int find(const MusaDwgAc1032SceneIndex *s,
                uint64_t h,uint64_t *offset){
    if(!s||!h||!offset||!s->handles||!s->objects||
       !s->count||!s->object_bytes)return 0;
    size_t lo=0u,hi=s->count;
    while(lo<hi){
       const size_t mid=lo+(hi-lo)/2u;
       if(s->handles[mid].handle<h)lo=mid+1u;else hi=mid;
    }
    if(lo==s->count||s->handles[lo].handle!=h)return 0;
    *offset=s->handles[lo].object_offset;
    return 1;
}
int musa_dwg_ac1032_scene_index_object_type(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    uint16_t *out){
    if(!s||!out||
       overlap(s,sizeof(*s),out,sizeof(*out))||
       overlap(s->handles,s->count*sizeof(*s->handles),
               out,sizeof(*out))||
       overlap(s->objects,s->object_bytes,out,sizeof(*out)))return 0;
    uint64_t off=0u;
    MusaDwgAc1032ObjectType type={0};
    if(!find(s,h,&off)||
       !musa_dwg_ac1032_object_type_bytes(
          s->objects,s->object_bytes,h,off,&type)||
       !type.record_crc_verified||type.indexed_handle!=h)return 0;
    *out=type.type_code;return 1;
}
typedef struct Payload {
    uint8_t *raw;
    size_t bytes;
    MusaDwgAc1032ObjectStreams layout;
    MusaDwgAc1032EntityCommon common;
} Payload;
static int fetch(const MusaDwgAc1032SceneIndex *s,uint64_t h,
                 uint16_t type,Payload *out){
    uint64_t off=0u;size_t needed=0u,copied=0u;
    MusaDwgAc1032PayloadInfo meta={0},again={0};
    MusaDwgAc1032ObjectStreams layout={0};
    if(!s||!out||!find(s,h,&off)||
       !musa_dwg_ac1032_object_payload_bytes(
           s->objects,s->object_bytes,h,off,
           NULL,0u,&needed,&meta)||
       !meta.original_record_crc_verified||
       meta.raw_object_type!=type||meta.object_type_field_bits!=10u||
       !needed||needed>MAX_OBJECT_SECTION||
       meta.payload_bytes!=needed||
       !musa_dwg_ac1032_stream_layout_bytes(
           s->objects,s->object_bytes,h,off,&layout)||
       !layout.original_record_crc_verified||
       layout.indexed_handle!=h||
       layout.object_section_offset!=off||
       layout.raw_object_type!=type||
       layout.object_type_field_bits!=meta.object_type_field_bits||
       layout.handle_stream_bits!=meta.handle_stream_bits||
       layout.total_payload_bits!=(uint64_t)needed*8u)return 0;
    uint8_t *raw=(uint8_t*)malloc(needed);
    if(!raw)return 0;
    MusaDwgAc1032EntityCommon common={0};
    int ok=musa_dwg_ac1032_object_payload_bytes(
       s->objects,s->object_bytes,h,off,
       raw,needed,&copied,&again)&&
       copied==needed&&again.original_record_crc_verified&&
       again.indexed_handle==h&&
       again.object_section_offset==off&&
       again.raw_object_type==type&&
       again.handle_stream_bits==meta.handle_stream_bits&&
       musa_dwg_ac1032_entity_common_bytes(
          raw,needed,&layout,type,&common)&&
       common.common_decoded&&
       common.prefix.indexed_handle==h&&
       common.prefix.raw_object_type==type;
    if(!ok){free(raw);return 0;}
    const Payload valid={raw,needed,layout,common};
    *out=valid;return 1;
}
static int check_output(const MusaDwgAc1032SceneIndex *s,
                        const void *out,size_t n){
    return s&&out&&!overlap(s,sizeof(*s),out,n)&&
       !overlap(s->objects,s->object_bytes,out,n)&&
       !overlap(s->handles,s->count*sizeof(*s->handles),out,n);
}
int musa_dwg_ac1032_scene_index_read_line(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032NativeLine *out){
    if(!check_output(s,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,19u,&p))return 0;
    MusaDwgAc1032NativeLine value={0};
    value.common=p.common;
    int ok=musa_dwg_ac1032_line_fields_at(
        p.raw,p.bytes,&p.layout,p.common.geometry_start_bit,
        &value.geometry)&&value.geometry.coordinates_decoded&&
        value.geometry.indexed_handle==h;
    free(p.raw);
    if(ok){*out=value;}
    return ok;
}
int musa_dwg_ac1032_scene_index_read_curve(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032NativeCurve *out){
    if(!check_output(s,out,sizeof(*out)))return 0;
    uint16_t type=0u;
    if(!musa_dwg_ac1032_scene_index_object_type(s,h,&type)||
       (type!=17u&&type!=18u))return 0;
    Payload p={0};
    if(!fetch(s,h,type,&p))return 0;
    MusaDwgAc1032NativeCurve value={0};
    int ok=musa_dwg_ac1032_curve_fields_at(
          p.raw,p.bytes,&p.layout,p.common.geometry_start_bit,&value)&&
          value.geometry_decoded&&value.indexed_handle==h&&
          value.object_type==type;
    value.common=p.common;
    free(p.raw);
    if(ok){*out=value;}
    return ok;
}
int musa_dwg_ac1032_scene_index_read_point(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032NativePoint *out){
    if(!check_output(s,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,27u,&p))return 0;
    MusaDwgAc1032NativePoint value={0};
    int ok=musa_dwg_ac1032_point_fields_at(
         p.raw,p.bytes,&p.layout,p.common.geometry_start_bit,&value)&&
         value.geometry_decoded&&value.indexed_handle==h;
    value.common=p.common;
    free(p.raw);
    if(ok){*out=value;}
    return ok;
}
int musa_dwg_ac1032_scene_index_read_lwpolyline(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032LWVertex *vertices,size_t capacity,
    MusaDwgAc1032NativeLWPolyline *out){
    if(!s||!vertices||!out||
       !capacity||capacity>MAX_LW_VERTICES||
       capacity>SIZE_MAX/sizeof(*vertices)||
       !check_output(s,vertices,capacity*sizeof(*vertices))||
       !check_output(s,out,sizeof(*out))||
       overlap(vertices,capacity*sizeof(*vertices),
               out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,77u,&p))return 0;
    MusaDwgAc1032NativeLWPolyline value={0};
    int ok=musa_dwg_ac1032_lwpolyline_fields_at(
          p.raw,p.bytes,&p.layout,p.common.geometry_start_bit,
          vertices,capacity,&value)&&value.geometry_decoded&&
          value.indexed_handle==h;
    value.common=p.common;
    free(p.raw);
    if(ok){*out=value;}
    return ok;
}
int musa_dwg_ac1032_scene_index_read_vertex3d(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032Vertex3D *out){
    if(!check_output(s,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,11u,&p))return 0;
    MusaDwgAc1032Vertex3D value={0};
    int ok=musa_dwg_ac1032_vertex3d_fields_at(
           p.raw,p.bytes,&p.layout,p.common.geometry_start_bit,&value)&&
           value.coordinates_decoded&&value.indexed_handle==h;
    value.common=p.common;
    free(p.raw);
    if(ok){*out=value;}
    return ok;
}
