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
/* Independent cached R2018 parent/child graph; original source file is
 * deliberately not needed and no guessed H-order shortcut is accepted. */
#include "musacad_dwg_ac1015_handle_refs.h"
#include "musacad_dwg_ac1015_values.h"
static int cached_graph_ref(MusaDwgAc1015Bits *bits,uint64_t base,
                            uint8_t expected,uint64_t *h){
    MusaDwgAc1015HandleRef ref={0};
    if(!musa_dwg_ac1015_read_handle_ref(bits,base,&ref)||
       ref.code!=expected||!ref.target)return 0;
    *h=ref.target;return 1;
}
static int cached_graph_zero_padding(MusaDwgAc1015Bits *bits){
    if(!bits||bits->bit_pos>bits->bit_end||
       bits->bit_end-bits->bit_pos>7u)return 0;
    while(bits->bit_pos<bits->bit_end){
        uint32_t bit=1u;
        if(!musa_dwg_ac1015_bits_read_u(bits,1u,&bit)||bit)return 0;
    }
    return 1;
}
static int cached_reverse_owner(const Payload *p,
     uint64_t child,uint64_t parent,uint64_t layer){
    if(!p||p->layout.data_stream_bits>SIZE_MAX||
       p->layout.total_payload_bits>SIZE_MAX||
       p->layout.raw_object_type!=11u)return 0;
    MusaDwgAc1015Bits bits={0};
    if(!musa_dwg_ac1015_bits_init(&bits,p->raw,p->bytes,
           (size_t)p->layout.data_stream_bits,
           (size_t)p->layout.total_payload_bits))return 0;
    uint64_t got_owner=0u,got_layer=0u;
    return cached_graph_ref(&bits,child,4u,&got_owner)&&
           got_owner==parent&&
           cached_graph_ref(&bits,child,5u,&got_layer)&&
           got_layer==layer&&cached_graph_zero_padding(&bits);
}
/* Cached original R2018 LAYER H/CRC/type: unlike entity fetch(), this
 * deliberately never invokes ENTITY common decoding for table records. */
int musa_dwg_ac1032_scene_index_read_layer_identity(
    const MusaDwgAc1032SceneIndex *scene,uint64_t h,
    MusaDwgAc1032LayerIdentity *out){
    if(!h||!check_output(scene,out,sizeof(*out)))return 0;
    uint64_t off=0u;size_t needed=0u,copied=0u;
    MusaDwgAc1032PayloadInfo meta={0},again={0};
    MusaDwgAc1032ObjectStreams layout={0};
    if(!find(scene,h,&off)||
       !musa_dwg_ac1032_object_payload_bytes(
         scene->objects,scene->object_bytes,h,off,
         NULL,0u,&needed,&meta)||
       !meta.original_record_crc_verified||
       meta.raw_object_type!=51u||
       meta.indexed_handle!=h||meta.payload_bytes!=needed||
       !needed||needed>MAX_OBJECT_SECTION||
       !musa_dwg_ac1032_stream_layout_bytes(
         scene->objects,scene->object_bytes,h,off,&layout)||
       !layout.original_record_crc_verified||
       layout.indexed_handle!=h||layout.raw_object_type!=51u||
       layout.object_section_offset!=off||
       layout.total_payload_bits!=(uint64_t)needed*8u||
       layout.handle_stream_bits!=meta.handle_stream_bits)
        return 0;
    uint8_t *raw=(uint8_t*)malloc(needed);
    if(!raw)return 0;
    MusaDwgAc1032LayerIdentity found={0};
    int ok=musa_dwg_ac1032_object_payload_bytes(
         scene->objects,scene->object_bytes,h,off,
         raw,needed,&copied,&again)&&
       copied==needed&&again.original_record_crc_verified&&
       again.raw_object_type==51u&&again.indexed_handle==h&&
       again.object_section_offset==meta.object_section_offset&&
       musa_dwg_ac1032_layer_ot51_identity_at(
         raw,needed,&layout,&found)&&
       found.indexed_handle==h;
    free(raw);
    if(ok)*out=found;
    return ok;
}
/* Source-backed R2018 simple entity H-tail, not a guessed owner/layer
 * match based on nearby H or numerically small handles. The caller can
 * later resolve the LAYER OT=51 DATA record; this API certifies only the
 * entity-to-layer graph edge and both original indexed records' CRCs. */
int musa_dwg_ac1032_scene_index_entity_layer_ref(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032EntityLayerRef *out){
    if(!h||!check_output(s,out,sizeof(*out)))return 0;
    uint16_t type=0u;
    if(!musa_dwg_ac1032_scene_index_object_type(s,h,&type)||
       (type!=17u&&type!=18u&&type!=19u&&type!=27u&&type!=77u))
        return 0;
    Payload p={0};
    if(!fetch(s,h,type,&p))return 0;
    /* Source H stream grammar is unambiguous only in this restricted
     * common-entity form; reject any owner, reactor or additional H
     * attributes rather than guessing the layer position. */
    const int simple=p.common.common_decoded&&
        p.common.prefix.entity_mode==2u&&
        p.common.prefix.reactor_count==0u&&
        p.common.prefix.xdictionary_missing==1u&&
        !p.common.has_color_reference&&
        p.common.linetype_flags==0u&&
        p.common.plotstyle_flags==0u&&
        p.common.material_flags==0u&&
        p.common.visualstyle_flags==0u&&
        p.layout.data_stream_bits<=SIZE_MAX&&
        p.layout.total_payload_bits<=SIZE_MAX;
    uint64_t layer=0u;
    int ok=0;
    if(simple){
        MusaDwgAc1015Bits bits={0};
        ok=musa_dwg_ac1015_bits_init(&bits,p.raw,p.bytes,
              (size_t)p.layout.data_stream_bits,
              (size_t)p.layout.total_payload_bits)&&
           cached_graph_ref(&bits,h,5u,&layer)&&
           layer!=h&&cached_graph_zero_padding(&bits);
    }
    free(p.raw);
    uint16_t layer_type=0u;
    if(!ok||
       !musa_dwg_ac1032_scene_index_object_type(s,layer,&layer_type)||
       layer_type!=51u)return 0;
    MusaDwgAc1032EntityLayerRef evidence={0};
    evidence.entity_handle=h;
    evidence.layer_handle=layer;
    evidence.raw_entity_type=type;
    evidence.raw_layer_type=layer_type;
    evidence.original_entity_crc_verified=1u;
    evidence.original_layer_crc_verified=1u;
    evidence.source_handle_ref_verified=1u;
    *out=evidence;
    return 1;
}
int musa_dwg_ac1032_scene_index_read_polyline3d_vertices(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032Polyline3DOrderedVertex *vertices,size_t capacity,
    MusaDwgAc1032Polyline3DOrdered *out){
    if(!s||!h||!vertices||!out||capacity<2u||
       capacity>100000u||
       capacity>SIZE_MAX/sizeof(*vertices)||
       !check_output(s,vertices,capacity*sizeof(*vertices))||
       !check_output(s,out,sizeof(*out))||
       overlap(vertices,capacity*sizeof(*vertices),
               out,sizeof(*out)))return 0;
    Payload parent={0};
    if(!fetch(s,h,16u,&parent))return 0;
    MusaDwgAc1032Polyline3DHeader header={0};
    int ok=musa_dwg_ac1032_polyline3d_header_fields_at(
          parent.raw,parent.bytes,&parent.layout,
          parent.common.geometry_start_bit,&header)&&
          header.header_decoded&&header.indexed_handle==h&&
          header.geometry_end_bit<=parent.layout.data_stream_bits&&
          header.owned_object_count>=2u&&
          header.owned_object_count<=capacity&&
          header.owned_object_count<=100000u;
    header.common=parent.common;
    uint64_t *owned=NULL;
    MusaDwgAc1032Polyline3DOrderedVertex *stage=NULL;
    size_t count=ok?(size_t)header.owned_object_count:0u;
    if(ok){
        owned=(uint64_t*)calloc(count,sizeof(*owned));
        stage=(MusaDwgAc1032Polyline3DOrderedVertex*)
              calloc(count,sizeof(*stage));
        if(!owned||!stage)ok=0;
    }
    uint64_t layer=0u,seqend=0u;
    if(ok)ok=musa_dwg_ac1032_polyline3d_handle_fields_at(
         parent.raw,parent.bytes,&parent.layout,&header,
         owned,count,&layer,&seqend);
    free(parent.raw);
    uint16_t seqtype=0u;
    if(ok)ok=musa_dwg_ac1032_scene_index_object_type(
          s,seqend,&seqtype)&&seqtype==6u;
    for(size_t i=0u;ok&&i<count;i++){
        Payload child={0};
        if(!fetch(s,owned[i],11u,&child)){ok=0;break;}
        MusaDwgAc1032Vertex3D point={0};
        ok=musa_dwg_ac1032_vertex3d_fields_at(
             child.raw,child.bytes,&child.layout,
             child.common.geometry_start_bit,&point)&&
             point.coordinates_decoded&&
             point.indexed_handle==owned[i]&&
             child.common.prefix.indexed_handle==owned[i]&&
             child.common.prefix.raw_object_type==11u&&
             cached_reverse_owner(&child,owned[i],h,layer);
        if(ok){
            stage[i].indexed_handle=owned[i];
            stage[i].flags=point.flags;
            memcpy(stage[i].point,point.point,sizeof(point.point));
        }
        free(child.raw);
    }
    if(ok){
        MusaDwgAc1032Polyline3DOrdered result={0};
        result.header=header;
        result.layer_handle=layer;
        result.seqend_handle=seqend;
        result.vertex_count=(uint32_t)count;
        result.hard_owner_links_verified=1u;
        result.reverse_vertex_owners_verified=1u;
        result.seqend_type_verified=1u;
        memcpy(vertices,stage,count*sizeof(*stage));
        *out=result;
    }
    free(stage);
    free(owned);
    return ok;
}
/* Whole-index geometry evidence, built from the SAME cached original
 * indexed binary section, not a guessed model-space entity iterator.
 * Every entry, even proxy/nonentity, gets a CRC-backed raw type.
 * Partial accepted geometry is never promoted to a full-scene claim. */
int musa_dwg_ac1032_scene_index_scan_geometry(
    const MusaDwgAc1032SceneIndex *s,
    MusaDwgAc1032SceneGeometryRecord *rows,size_t capacity,
    MusaDwgAc1032SceneGeometryReport *out){
    if(!s||!out||!s->handles||!s->objects||
       !s->count||s->count>MAX_INDEX||
       (!rows&&capacity)||
       capacity>SIZE_MAX/sizeof(*rows)||
       (rows&&capacity<s->count)||
       !check_output(s,out,sizeof(*out))||
       (rows&&(!check_output(s,rows,capacity*sizeof(*rows))||
           overlap(rows,capacity*sizeof(*rows),out,sizeof(*out)))))
        return 0;
    MusaDwgAc1032SceneGeometryRecord *stage=NULL;
    MusaDwgAc1032LWVertex *lw=NULL;
    MusaDwgAc1032Polyline3DOrderedVertex *poly3d=NULL;
    int ok=1;
    if(rows){
        stage=(MusaDwgAc1032SceneGeometryRecord*)
              calloc(s->count,sizeof(*stage));
        if(!stage)return 0;
    }
    MusaDwgAc1032SceneGeometryReport report={0};
    report.indexed_records=s->count;
    uint64_t previous=0u;
    for(size_t i=0u;i<s->count&&ok;i++){
        MusaDwgAc1032HandleEntry entry={0};
        uint16_t type=0u;
        if(!musa_dwg_ac1032_scene_index_entry(s,i,&entry)||
           !entry.handle||entry.handle<=previous||
           !musa_dwg_ac1032_scene_index_object_type(
               s,entry.handle,&type)){
            ok=0;break;
        }
        previous=entry.handle;
        uint8_t category=MUSA_AC1032_SCENE_OPAQUE;
        uint32_t vertices=0u;
        int geometry=0;
        switch(type){
        case 19u: {
            MusaDwgAc1032NativeLine line={0};
            geometry=musa_dwg_ac1032_scene_index_read_line(
               s,entry.handle,&line);
            category=geometry?MUSA_AC1032_SCENE_GEOMETRY_DECODED:
                              MUSA_AC1032_SCENE_GEOMETRY_REJECTED;
            break;
        }
        case 17u: case 18u: {
            MusaDwgAc1032NativeCurve curve={0};
            geometry=musa_dwg_ac1032_scene_index_read_curve(
                 s,entry.handle,&curve);
            category=geometry?MUSA_AC1032_SCENE_GEOMETRY_DECODED:
                              MUSA_AC1032_SCENE_GEOMETRY_REJECTED;
            break;
        }
        case 27u: {
            MusaDwgAc1032NativePoint point={0};
            geometry=musa_dwg_ac1032_scene_index_read_point(
                s,entry.handle,&point);
            category=geometry?MUSA_AC1032_SCENE_GEOMETRY_DECODED:
                              MUSA_AC1032_SCENE_GEOMETRY_REJECTED;
            break;
        }
        case 77u: {
            if(!lw){
                lw=(MusaDwgAc1032LWVertex*)
                   malloc(MAX_LW_VERTICES*sizeof(*lw));
                if(!lw){ok=0;break;}
            }
            MusaDwgAc1032NativeLWPolyline pline={0};
            geometry=musa_dwg_ac1032_scene_index_read_lwpolyline(
                   s,entry.handle,lw,MAX_LW_VERTICES,&pline);
            if(geometry)vertices=pline.vertex_count;
            category=geometry?MUSA_AC1032_SCENE_GEOMETRY_DECODED:
                              MUSA_AC1032_SCENE_GEOMETRY_REJECTED;
            break;
        }
        case 16u: {
            if(!poly3d){
                poly3d=(MusaDwgAc1032Polyline3DOrderedVertex*)
                       malloc(100000u*sizeof(*poly3d));
                if(!poly3d){ok=0;break;}
            }
            MusaDwgAc1032Polyline3DOrdered value={0};
            geometry=musa_dwg_ac1032_scene_index_read_polyline3d_vertices(
                s,entry.handle,poly3d,100000u,&value);
            if(geometry)vertices=value.vertex_count;
            category=geometry?MUSA_AC1032_SCENE_GEOMETRY_DECODED:
                              MUSA_AC1032_SCENE_GEOMETRY_REJECTED;
            break;
        }
        case 6u: case 11u:
            category=MUSA_AC1032_SCENE_DEPENDENT;
            break;
        default:
            category=MUSA_AC1032_SCENE_OPAQUE;
            break;
        }
        if(!ok)break;
        if(category==MUSA_AC1032_SCENE_GEOMETRY_DECODED)
            report.decoded_geometry_objects++;
        else if(category==MUSA_AC1032_SCENE_GEOMETRY_REJECTED)
            report.rejected_supported_geometry++;
        else if(category==MUSA_AC1032_SCENE_DEPENDENT)
            report.dependent_objects++;
        else report.opaque_objects++;
        if(stage){
            stage[i].indexed_handle=entry.handle;
            stage[i].raw_object_type=type;
            stage[i].category=category;
            stage[i].original_record_crc_verified=1u;
            stage[i].decoded_vertex_count=vertices;
        }
    }
    if(ok){
        report.all_indexed_raw_types_crc_verified=1u;
        if(rows)memcpy(rows,stage,s->count*sizeof(*stage));
        *out=report;
    }
    free(poly3d);
    free(lw);
    free(stage);
    return ok;
}
