/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_sections.h"
#include "musacad_dwg_ac1032_object_type.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1032_object_streams.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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
/* Real OT4 BLOCK begin source only. BLOCK marker is not drawable,
 * and no BLOCK_HEADER, nested INSERT or owned children are inferred. */
/* Original R2018 OT5 ENDBLK is a non-drawable marker. The source
 * CRC, handle and common are authenticated, but its BLOCK_HEADER
 * and owned-child sequence have NOT been traversed. */
int musa_dwg_ac1032_scene_index_read_endblk_common(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032EntityCommon *out){
    if(!h||!check_output(s,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,5u,&p))return 0;
    const MusaDwgAc1032EntityCommon v=p.common;
    free(p.raw);
    if(!v.common_decoded||!v.prefix.prefix_decoded||
       v.prefix.indexed_handle!=h||v.prefix.raw_object_type!=5u||
       v.geometry_start_bit<=v.prefix.next_color_bit)return 0;
    *out=v;return 1;
}
int musa_dwg_ac1032_scene_index_read_block_common(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032EntityCommon *out){
    if(!h||!check_output(s,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,4u,&p))return 0;
    const MusaDwgAc1032EntityCommon v=p.common;
    free(p.raw);
    if(!v.common_decoded||!v.prefix.prefix_decoded||
       v.prefix.indexed_handle!=h||v.prefix.raw_object_type!=4u||
       v.geometry_start_bit<=v.prefix.next_color_bit)return 0;
    *out=v;return 1;
}
/* Real OT3 ATTDEF authenticated common ENC/status; no tag/default text,
 * BLOCK ownership, attributes on INSERT or rendered geometry. */
int musa_dwg_ac1032_scene_index_read_attdef_common(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032EntityCommon *out){
    if(!h||!check_output(s,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,3u,&p))return 0;
    const MusaDwgAc1032EntityCommon v=p.common;
    free(p.raw);
    if(!v.common_decoded||!v.prefix.prefix_decoded||
       v.prefix.indexed_handle!=h||v.prefix.raw_object_type!=3u||
       v.geometry_start_bit<=v.prefix.next_color_bit)return 0;
    *out=v;return 1;
}
/* Real OT2 ATTRIB authenticated common ENC/status. The attribute DATA,
 * INSERT ownership and block transforms are intentionally NOT admitted. */
int musa_dwg_ac1032_scene_index_read_attrib_common(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032EntityCommon *out){
    if(!h||!check_output(s,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,2u,&p))return 0;
    const MusaDwgAc1032EntityCommon v=p.common;
    free(p.raw);
    if(!v.common_decoded||!v.prefix.prefix_decoded||
       v.prefix.indexed_handle!=h||v.prefix.raw_object_type!=2u||
       v.geometry_start_bit<=v.prefix.next_color_bit)return 0;
    *out=v;return 1;
}
/* Original R2018 OT7 numeric OCS placement. No block graph or paint. */
int musa_dwg_ac1032_scene_index_read_insert_placement(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032InsertPlacement *out){
    if(!h||!check_output(s,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,7u,&p))return 0;
    MusaDwgAc1032InsertPlacement value={0};
    int ok=musa_dwg_ac1032_insert_placement_at(
        p.raw,p.bytes,&p.layout,p.common.geometry_start_bit,&value)&&
        value.indexed_handle==h&&value.original_record_crc_verified&&
        value.numeric_fields_decoded&&!value.block_graph_and_display_accepted;
    free(p.raw);
    if(ok)*out=value;
    return ok;
}
/* Genuine original OT7 INSERT common source, NOT numeric geometry. */
int musa_dwg_ac1032_scene_index_read_insert_common(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032EntityCommon *out){
    if(!h||!check_output(s,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,7u,&p))return 0;
    const MusaDwgAc1032EntityCommon v=p.common;
    free(p.raw);
    if(!v.common_decoded||!v.prefix.prefix_decoded||
       v.prefix.indexed_handle!=h||v.prefix.raw_object_type!=7u||
       v.geometry_start_bit<=v.prefix.next_color_bit)return 0;
    *out=v;return 1;
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
int musa_dwg_ac1032_scene_index_read_ellipse(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032NativeEllipse *out){
    if(!check_output(s,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,35u,&p))return 0;
    MusaDwgAc1032NativeEllipse value={0};
    int ok=musa_dwg_ac1032_ellipse_fields_at(
        p.raw,p.bytes,&p.layout,p.common.geometry_start_bit,&value)&&
        value.geometry_decoded&&value.indexed_handle==h;
    value.common=p.common;
    free(p.raw);
    if(ok)*out=value;
    return ok;
}
/* OT1 TEXT is not OT44 MTEXT. Source-authenticate the common entity
 * fields only; no TU/string/layout or owner/style inference is permitted. */
/* Original R2018 OT1 STRING TU. Not a shortcut to MTEXT. */
int musa_dwg_ac1032_scene_index_read_text_source(
    const MusaDwgAc1032SceneIndex *scene,uint64_t h,
    MusaDwgAc1032TextSource *out){
    if(!h||!check_output(scene,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(scene,h,1u,&p))return 0;
    MusaDwgAc1032TextSource value={0};
    const int ok=musa_dwg_ac1032_text_single_tu_at(
        p.raw,p.bytes,&p.layout,p.common.geometry_start_bit,&value)&&
        value.indexed_handle==h&&value.original_record_crc_verified&&
        !value.geometry_font_and_display_accepted;
    free(p.raw);
    if(ok)*out=value;
    return ok;
}
int musa_dwg_ac1032_scene_index_read_text_placement(
    const MusaDwgAc1032SceneIndex *scene,uint64_t h,
    MusaDwgAc1032TextPlacement *out){
    if(!h||!check_output(scene,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(scene,h,1u,&p))return 0;
    MusaDwgAc1032TextPlacement value={0};
    const int ok=musa_dwg_ac1032_text_placement_at(
       p.raw,p.bytes,&p.layout,p.common.geometry_start_bit,&value)&&
       value.indexed_handle==h&&value.original_record_crc_verified&&
       value.numeric_fields_decoded&&!value.wcs_font_and_display_accepted;
    free(p.raw);
    if(ok)*out=value;
    return ok;
}
int musa_dwg_ac1032_scene_index_read_text_common(
    const MusaDwgAc1032SceneIndex *scene,uint64_t h,
    MusaDwgAc1032EntityCommon *out){
    if(!h||!check_output(scene,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(scene,h,1u,&p))return 0;
    const MusaDwgAc1032EntityCommon common=p.common;
    free(p.raw);
    if(!common.common_decoded||!common.prefix.prefix_decoded||
       common.prefix.indexed_handle!=h||
       common.prefix.raw_object_type!=1u||
       !common.geometry_start_bit)return 0;
    *out=common;
    return 1;
}
int musa_dwg_ac1032_scene_index_read_mtext_common(
    const MusaDwgAc1032SceneIndex *scene,uint64_t h,
    MusaDwgAc1032EntityCommon *out){
    if(!check_output(scene,out,sizeof(*out)))return 0;
    Payload payload={0};
    if(!fetch(scene,h,44u,&payload))return 0;
    const MusaDwgAc1032EntityCommon common=payload.common;
    free(payload.raw);
    if(!common.common_decoded||!common.prefix.prefix_decoded||
       common.prefix.indexed_handle!=h||
       common.prefix.raw_object_type!=44u)return 0;
    *out=common;return 1;
}
int musa_dwg_ac1032_scene_index_read_mtext_source(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032MTextSource *out){
    if(!check_output(s,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,44u,&p))return 0;
    MusaDwgAc1032MTextSource value={0};
    const int ok=musa_dwg_ac1032_mtext_single_tu_at(
        p.raw,p.bytes,&p.layout,p.common.geometry_start_bit,&value)&&
        value.indexed_handle==h&&
        value.original_record_crc_verified&&
        !value.geometry_style_and_glyphs_accepted;
    free(p.raw);
    if(ok)*out=value;
    return ok;
}
int musa_dwg_ac1032_scene_index_read_mtext_placement(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032MTextPlacement *out){
    if(!check_output(s,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,44u,&p))return 0;
    MusaDwgAc1032MTextPlacement value={0};
    const int ok=musa_dwg_ac1032_mtext_placement_at(
        p.raw,p.bytes,&p.layout,p.common.geometry_start_bit,&value)&&
        value.indexed_handle==h&&value.original_record_crc_verified&&
        value.geometry_fields_decoded&&
        !value.glyph_layout_and_display_accepted;
    free(p.raw);
    if(ok)*out=value;
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
/* Authentic R2018 INSERT handle tails end with one-bits (not zero
 * padding). Do not generalize this narrow verified profile to other OT. */
static int insert_one_padding(MusaDwgAc1015Bits *bits){
    if(!bits||bits->bit_pos>bits->bit_end||
       bits->bit_end-bits->bit_pos>7u)return 0;
    while(bits->bit_pos<bits->bit_end){
        uint32_t bit=0u;
        if(!musa_dwg_ac1015_bits_read_u(bits,1u,&bit)||bit!=1u)
            return 0;
    }
    return 1;
}
/* Source-only original INSERT H graph: H5 LAYER, H5 BLOCK_HEADER,
 * optional H3 ATTRIB + H3 SEQEND for the independently verified one-
 * attribute profile. All targets require their own indexed CRC/OT proof.
 * No block name, nested transforms, owner, viewport or drawing acceptance. */
int musa_dwg_ac1032_scene_index_insert_source_graph(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032InsertSourceGraph *out){
    if(!h||!check_output(s,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,7u,&p))return 0;
    MusaDwgAc1032InsertPlacement place={0};
    MusaDwgAc1032InsertSourceGraph v={0};
    v.indexed_handle=h;
    int ok=p.common.common_decoded&&
       p.common.prefix.entity_mode==2u&&
       p.common.prefix.reactor_count==0u&&
       p.common.prefix.xdictionary_missing==1u&&
       !p.common.has_color_reference&&
       p.common.linetype_flags==0u&&
       p.common.plotstyle_flags==0u&&
       p.common.material_flags==0u&&
       p.common.visualstyle_flags==0u&&
       p.layout.data_stream_bits<=SIZE_MAX&&
       p.layout.total_payload_bits<=SIZE_MAX&&
       musa_dwg_ac1032_insert_placement_at(
          p.raw,p.bytes,&p.layout,p.common.geometry_start_bit,&place)&&
       place.original_record_crc_verified&&place.numeric_fields_decoded&&
       place.indexed_handle==h&&!place.block_graph_and_display_accepted&&
       place.owned_attrib_count<=1u&&
       (place.has_attribs==(uint8_t)(place.owned_attrib_count==1u));
    if(ok){
        MusaDwgAc1015Bits bits={0};
        ok=musa_dwg_ac1015_bits_init(&bits,p.raw,p.bytes,
              (size_t)p.layout.data_stream_bits,
              (size_t)p.layout.total_payload_bits)&&
           cached_graph_ref(&bits,h,5u,&v.layer_handle)&&
           cached_graph_ref(&bits,h,5u,&v.block_header_handle)&&
           v.layer_handle!=h&&v.block_header_handle!=h&&
           v.layer_handle!=v.block_header_handle;
        if(ok&&place.has_attribs)
            ok=cached_graph_ref(&bits,h,3u,&v.attrib_handle)&&
               cached_graph_ref(&bits,h,3u,&v.seqend_handle)&&
               v.attrib_handle!=h&&v.seqend_handle!=h&&
               v.attrib_handle!=v.seqend_handle;
        if(ok)ok=insert_one_padding(&bits);
    }
    free(p.raw);
    if(!ok)return 0;
    uint16_t kind=0u;
    if(!musa_dwg_ac1032_scene_index_object_type(
            s,v.layer_handle,&kind)||kind!=51u||
       !musa_dwg_ac1032_scene_index_object_type(
            s,v.block_header_handle,&kind)||kind!=49u)return 0;
    if(place.has_attribs&&
       (!musa_dwg_ac1032_scene_index_object_type(
           s,v.attrib_handle,&kind)||kind!=2u||
        !musa_dwg_ac1032_scene_index_object_type(
           s,v.seqend_handle,&kind)||kind!=6u))return 0;
    v.owned_attrib_count=place.owned_attrib_count;
    v.original_entity_and_targets_crc_verified=1u;
    v.source_edges_verified=1u;
    *out=v;return 1;
}
/* Strict original R2018 INSERT layer provenance, not a style resolver.
 * Reuse the independently authenticated INSERT H graph rather than
 * guessing layer by numeric H adjacency or a default palette. */
int musa_dwg_ac1032_scene_index_insert_layer_provenance(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032InsertLayerProvenance *out){
    if(!h||!check_output(s,out,sizeof(*out)))return 0;
    MusaDwgAc1032InsertLayerProvenance v={0};
    if(!musa_dwg_ac1032_scene_index_insert_source_graph(
           s,h,&v.graph)||
       !v.graph.original_entity_and_targets_crc_verified||
       !v.graph.source_edges_verified||
       v.graph.block_expansion_and_display_accepted||
       v.graph.indexed_handle!=h||!v.graph.layer_handle||
       !musa_dwg_ac1032_scene_index_read_layer_name(
           s,v.graph.layer_handle,&v.layer_name)||
       !musa_dwg_ac1032_scene_index_read_layer_flags_evidence(
           s,v.graph.layer_handle,&v.raw_layer_flags)||
       !musa_dwg_ac1032_scene_index_read_insert_common(
           s,h,&v.insert_common)||
       v.layer_name.indexed_handle!=v.graph.layer_handle||
       !v.layer_name.original_record_crc_verified||
       !v.layer_name.utf8_bytes||
       v.layer_name.utf8_bytes>=sizeof(v.layer_name.utf8)||
       v.layer_name.utf8[v.layer_name.utf8_bytes]!='\0'||
       v.raw_layer_flags.indexed_handle!=v.graph.layer_handle||
       !v.raw_layer_flags.source_crc_verified||
       v.raw_layer_flags.numeric_flags_end_bit<=
           v.raw_layer_flags.numeric_flags_start_bit||
       !v.insert_common.common_decoded||
       !v.insert_common.prefix.prefix_decoded||
       v.insert_common.prefix.indexed_handle!=h||
       v.insert_common.prefix.raw_object_type!=7u)return 0;
    v.source_layer_and_entity_verified=1u;
    *out=v;return 1;
}
/* One source-backed original INSERT -> LAYER CMC join. Reuse the
 * independently verified H graph and Unicode/status source evidence.
 * Do not assign a drawable block color or silently default BYLAYER. */
int musa_dwg_ac1032_scene_index_insert_layer_cmc_source(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032InsertLayerCmcSource *out){
    if(!h||!check_output(s,out,sizeof(*out)))return 0;
    MusaDwgAc1032InsertLayerCmcSource v={0};
    if(!musa_dwg_ac1032_scene_index_insert_layer_provenance(
           s,h,&v.source)||
       !v.source.source_layer_and_entity_verified||
       v.source.final_visibility_color_and_paint_accepted||
       !musa_dwg_ac1032_scene_index_layer_anchored_cmc_evidence(
           s,v.source.graph.layer_handle,&v.cmc)||
       v.cmc.indexed_handle!=v.source.graph.layer_handle||
       !v.cmc.original_record_crc_verified||
       (v.cmc.packed_color_method>>24u)!=0xc3u||
       !v.cmc.aci_index||
       (v.cmc.packed_color_method&0xffu)!=v.cmc.aci_index||
       v.cmc.cmc_end_bit<=v.cmc.cmc_start_bit)return 0;
    v.source_aci_verified=1u;
    *out=v;return 1;
}
/* Original MTEXT H tail: optional hard owner, entity layer H5 and
 * MTEXT style H5. Reject unexpected extras, never infer a default layer. */
/* Source OT1 TEXT H tail is NOT MTEXT's H graph. For this narrow
 * genuine R2018 profile require explicit layer H5 and style H5, with
 * only an optional null 0x50 H reference. The independently CRC-checked
 * R2018 sample_2018.dwg OT1 H8E has two H5 refs (81, 11) followed by
 * exactly three one-bits (111), not zero padding. Accept this exact
 * terminal profile only; other trailing H layouts fail closed. */
int musa_dwg_ac1032_scene_index_text_graph(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032TextGraph *out){
    if(!h||!check_output(s,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,1u,&p))return 0;
    MusaDwgAc1032TextGraph v={0};
    v.indexed_handle=h;
    int ok=p.common.common_decoded&&
       p.common.prefix.reactor_count==0u&&
       p.common.prefix.xdictionary_missing==1u&&
       !p.common.has_color_reference&&
       p.common.linetype_flags==0u&&
       p.common.plotstyle_flags==0u&&
       p.common.material_flags==0u&&
       p.common.visualstyle_flags==0u&&
       p.layout.data_stream_bits<=SIZE_MAX&&
       p.layout.total_payload_bits<=SIZE_MAX;
    if(ok){
        MusaDwgAc1015Bits bits={0};
        ok=musa_dwg_ac1015_bits_init(&bits,p.raw,p.bytes,
            (size_t)p.layout.data_stream_bits,
            (size_t)p.layout.total_payload_bits);
        if(ok&&p.common.prefix.entity_mode==0u)
            ok=cached_graph_ref(&bits,h,4u,&v.owner_handle);
        else if(ok&&p.common.prefix.entity_mode!=2u)ok=0;
        if(ok)ok=cached_graph_ref(&bits,h,5u,&v.layer_handle)&&
                 cached_graph_ref(&bits,h,5u,&v.style_handle)&&
                 v.layer_handle!=h&&v.style_handle!=h&&
                 v.layer_handle!=v.style_handle;
        if(ok&&bits.bit_end-bits.bit_pos>=8u){
            uint32_t null_ref=0u;
            ok=musa_dwg_ac1015_bits_read_u(&bits,8u,&null_ref)&&
               null_ref==0x50u;
        }
        if(ok){
            /* Genuine OT1 H8E ends at bit 888: 853 data bits,
             * 32 H-reference bits, 3 terminal one-bits. Do not
             * generalize this narrow profile to other TEXT objects. */
            uint32_t terminal=0u;
            ok=bits.bit_end-bits.bit_pos==3u&&
               musa_dwg_ac1015_bits_read_u(&bits,3u,&terminal)&&
               terminal==7u&&bits.bit_pos==bits.bit_end;
        }
    }
    free(p.raw);
    if(!ok)return 0;
    uint16_t lt=0u,st=0u;
    if(!musa_dwg_ac1032_scene_index_object_type(
           s,v.layer_handle,&lt)||lt!=51u||
       !musa_dwg_ac1032_scene_index_object_type(
           s,v.style_handle,&st)||st!=53u)return 0;
    v.layer_object_type=lt;
    v.style_object_type=st;
    v.original_entity_crc_verified=1u;
    v.original_layer_crc_verified=1u;
    v.original_style_crc_verified=1u;
    v.graph_edges_verified=1u;
    v.font_and_display_accepted=0u;
    *out=v;return 1;
}
int musa_dwg_ac1032_scene_index_mtext_graph(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032MTextGraph *out){
    if(!h||!check_output(s,out,sizeof(*out)))return 0;
    Payload p={0};
    if(!fetch(s,h,44u,&p))return 0;
    MusaDwgAc1032MTextGraph value={0};
    value.indexed_handle=h;
    int ok=p.common.common_decoded&&
       p.common.prefix.reactor_count==0u&&
       p.common.prefix.xdictionary_missing==1u&&
       !p.common.has_color_reference&&
       p.common.linetype_flags==0u&&
       p.common.plotstyle_flags==0u&&
       p.common.material_flags==0u&&
       p.common.visualstyle_flags==0u&&
       p.layout.data_stream_bits<=SIZE_MAX&&
       p.layout.total_payload_bits<=SIZE_MAX;
    if(ok){
        MusaDwgAc1015Bits bits={0};
        ok=musa_dwg_ac1015_bits_init(&bits,p.raw,p.bytes,
            (size_t)p.layout.data_stream_bits,
            (size_t)p.layout.total_payload_bits);
        if(ok&&p.common.prefix.entity_mode==0u)
            ok=cached_graph_ref(&bits,h,4u,&value.owner_handle);
        else if(ok&&p.common.prefix.entity_mode!=2u)ok=0;
        if(ok)ok=cached_graph_ref(&bits,h,5u,&value.layer_handle)&&
                 cached_graph_ref(&bits,h,5u,&value.style_handle)&&
                 value.layer_handle!=h&&value.style_handle!=h&&
                 value.layer_handle!=value.style_handle;
        /* Original R2018 Text.dwg H1C0 has two valid H5 targets,
         * followed by one exact 0x50 null H5 optional reference and
         * six zero end-padding bits. Never interpret the null as a
         * third target, and never accept arbitrary unparsed H data. */
        if(ok&&bits.bit_end-bits.bit_pos>=8u){
            uint32_t null_ref=0u;
            ok=musa_dwg_ac1015_bits_read_u(&bits,8u,&null_ref)&&
               null_ref==0x50u;
        }
        if(ok)ok=cached_graph_zero_padding(&bits);
    }
    free(p.raw);
    if(!ok)return 0;
    uint16_t layer_type=0u,style_type=0u;
    if(!musa_dwg_ac1032_scene_index_object_type(
          s,value.layer_handle,&layer_type)||layer_type!=51u||
       !musa_dwg_ac1032_scene_index_object_type(
          s,value.style_handle,&style_type))return 0;
    value.layer_object_type=layer_type;
    value.style_object_type=style_type;
    value.original_entity_crc_verified=1u;
    value.original_layer_crc_verified=1u;
    value.original_style_crc_verified=1u;
    value.graph_edges_verified=1u;
    value.font_and_display_accepted=0u;
    *out=value;
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
/* Source-CRC authenticated cached LAYER TU name, never a substitute
 * for actual layer style/visibility or content Unicode outside the
 * bounded R2018 string stream grammar supported by layer.c. */
int musa_dwg_ac1032_scene_index_read_block_header_prefix_evidence(
    const MusaDwgAc1032SceneIndex *scene,uint64_t h,
    MusaDwgAc1032BlockHeaderPrefixEvidence *out){
    if(!h||!check_output(scene,out,sizeof(*out)))return 0;
    uint64_t off=0u;size_t needed=0u,copied=0u;
    MusaDwgAc1032PayloadInfo meta={0},again={0};
    MusaDwgAc1032ObjectStreams layout={0};
    if(!find(scene,h,&off)||
       !musa_dwg_ac1032_object_payload_bytes(
           scene->objects,scene->object_bytes,h,off,
           NULL,0u,&needed,&meta)||
       !meta.original_record_crc_verified||
       meta.raw_object_type!=49u||meta.indexed_handle!=h||
       !needed||needed>MAX_OBJECT_SECTION||
       meta.payload_bytes!=needed||
       !musa_dwg_ac1032_stream_layout_bytes(
           scene->objects,scene->object_bytes,h,off,&layout)||
       !layout.original_record_crc_verified||
       layout.indexed_handle!=h||layout.raw_object_type!=49u||
       layout.object_section_offset!=off||
       layout.total_payload_bits!=(uint64_t)needed*8u||
       layout.handle_stream_bits!=meta.handle_stream_bits)return 0;
    uint8_t *raw=(uint8_t*)malloc(needed);
    if(!raw)return 0;
    MusaDwgAc1032BlockHeaderPrefixEvidence value={0};
    int ok=musa_dwg_ac1032_object_payload_bytes(
           scene->objects,scene->object_bytes,h,off,
           raw,needed,&copied,&again)&&
       copied==needed&&again.original_record_crc_verified&&
       again.raw_object_type==49u&&again.indexed_handle==h&&
       again.object_section_offset==off&&
       musa_dwg_ac1032_block_header_ot49_prefix_evidence(
           raw,needed,&layout,&value);
    free(raw);
    if(!ok)return 0;
    *out=value;return 1;
}
int musa_dwg_ac1032_scene_index_copy_block_header_field_bits(
    const MusaDwgAc1032SceneIndex *scene,uint64_t h,
    size_t start_bit,size_t bit_count,uint8_t *out,size_t out_bytes){
    if(!h||!bit_count||bit_count>SIZE_MAX-7u||
       out_bytes>MAX_OBJECT_SECTION||
       out_bytes<(bit_count+7u)/8u||
       !check_output(scene,out,out_bytes))return 0;
    uint64_t off=0u;size_t needed=0u,copied=0u;
    MusaDwgAc1032PayloadInfo meta={0},again={0};
    MusaDwgAc1032ObjectStreams layout={0};
    if(!find(scene,h,&off)||
       !musa_dwg_ac1032_object_payload_bytes(
           scene->objects,scene->object_bytes,h,off,
           NULL,0u,&needed,&meta)||
       !meta.original_record_crc_verified||
       meta.raw_object_type!=49u||meta.indexed_handle!=h||
       !needed||needed>MAX_OBJECT_SECTION||
       meta.payload_bytes!=needed||
       !musa_dwg_ac1032_stream_layout_bytes(
           scene->objects,scene->object_bytes,h,off,&layout)||
       !layout.original_record_crc_verified||
       layout.indexed_handle!=h||layout.raw_object_type!=49u||
       layout.object_section_offset!=off||
       layout.total_payload_bits!=(uint64_t)needed*8u||
       layout.handle_stream_bits!=meta.handle_stream_bits)return 0;
    uint8_t *raw=(uint8_t*)malloc(needed);
    if(!raw)return 0;
    int ok=musa_dwg_ac1032_object_payload_bytes(
           scene->objects,scene->object_bytes,h,off,
           raw,needed,&copied,&again)&&
       copied==needed&&again.original_record_crc_verified&&
       again.raw_object_type==49u&&again.indexed_handle==h&&
       again.object_section_offset==off&&
       musa_dwg_ac1032_block_header_ot49_copy_field_bits(
           raw,needed,&layout,start_bit,bit_count,out,out_bytes);
    free(raw);
    return ok;
}
int musa_dwg_ac1032_scene_index_block_header_child_prefix(
    const MusaDwgAc1032SceneIndex *scene,uint64_t h,
    MusaDwgAc1032BlockHeaderChildPrefix *out){
    if(!h||!check_output(scene,out,sizeof(*out)))return 0;
    uint64_t off=0u;size_t needed=0u,copied=0u;
    MusaDwgAc1032PayloadInfo meta={0},again={0};
    MusaDwgAc1032ObjectStreams layout={0};
    if(!find(scene,h,&off)||
       !musa_dwg_ac1032_object_payload_bytes(
           scene->objects,scene->object_bytes,h,off,
           NULL,0u,&needed,&meta)||
       !meta.original_record_crc_verified||
       meta.raw_object_type!=49u||meta.indexed_handle!=h||
       !needed||needed>MAX_OBJECT_SECTION||
       meta.payload_bytes!=needed||
       !musa_dwg_ac1032_stream_layout_bytes(
           scene->objects,scene->object_bytes,h,off,&layout)||
       !layout.original_record_crc_verified||
       layout.indexed_handle!=h||layout.raw_object_type!=49u||
       layout.object_section_offset!=off||
       layout.total_payload_bits!=(uint64_t)needed*8u||
       layout.handle_stream_bits!=meta.handle_stream_bits)return 0;
    uint8_t *raw=(uint8_t*)malloc(needed);
    if(!raw)return 0;
    MusaDwgAc1032BlockHeaderChildPrefix value={0};
    int ok=musa_dwg_ac1032_object_payload_bytes(
           scene->objects,scene->object_bytes,h,off,
           raw,needed,&copied,&again)&&
       copied==needed&&again.original_record_crc_verified&&
       again.raw_object_type==49u&&again.indexed_handle==h&&
       again.object_section_offset==off&&
       musa_dwg_ac1032_block_header_ot49_child_prefix_at(
           raw,needed,&layout,&value);
    if(!ok&&(h==UINT64_C(0x1b8)||h==UINT64_C(0x17d))){
        /* TEMPORARY source-level diagnostic; remove before merge. */
        fprintf(stderr,"OT49 H prefix diagnostic h=%llx payload=%zu Hstart=%llu Hbits=%llu\\n",
            (unsigned long long)h,needed,
            (unsigned long long)layout.data_stream_bits,
            (unsigned long long)layout.handle_stream_bits);
        MusaDwgAc1015Bits dbg={0};
        if(musa_dwg_ac1015_bits_init(&dbg,raw,needed,
             (size_t)layout.data_stream_bits,(size_t)layout.total_payload_bits)){
            for(unsigned j=0;j<48u&&dbg.bit_pos+8u<=dbg.bit_end;j++){
                uint32_t v=0;
                if(!musa_dwg_ac1015_bits_read_u(&dbg,8u,&v))break;
                fprintf(stderr,"%02x%s",v,j%16u==15u?"\\n":" ");
            }
            fprintf(stderr,"\\n");
        }
    }
    free(raw);
    if(!ok)return 0;
    if(h==UINT64_C(0x1b8)||h==UINT64_C(0x17d))
        fprintf(stderr,"OT49 H decoded h=%llx control=%llx begin=%llx end=%llx count=%u\\n",
            (unsigned long long)h,(unsigned long long)value.block_control_handle,
            (unsigned long long)value.block_begin_handle,
            (unsigned long long)value.endblk_handle,value.child_count);
    /* Resolve every proposed edge against the original independently
     * CRC-verified H map. This is not reverse-owner authentication. */
    uint16_t kind=0u;
    if(!musa_dwg_ac1032_scene_index_object_type(
           scene,value.block_control_handle,&kind)||
       kind==4u||kind==5u||
       !musa_dwg_ac1032_scene_index_object_type(
           scene,value.block_begin_handle,&kind)||kind!=4u||
       !musa_dwg_ac1032_scene_index_object_type(
           scene,value.endblk_handle,&kind)||kind!=5u)return 0;
    for(uint32_t i=0u;i<value.child_count;i++){
        if(!musa_dwg_ac1032_scene_index_object_type(
               scene,value.child_handles[i],&kind)||
           kind==4u||kind==5u)return 0;
    }
    *out=value;return 1;
}
int musa_dwg_ac1032_scene_index_read_block_header_base(
    const MusaDwgAc1032SceneIndex *scene,uint64_t h,
    MusaDwgAc1032BlockHeaderBase *out){
    if(!h||!check_output(scene,out,sizeof(*out)))return 0;
    uint64_t off=0u;size_t needed=0u,copied=0u;
    MusaDwgAc1032PayloadInfo meta={0},again={0};
    MusaDwgAc1032ObjectStreams layout={0};
    if(!find(scene,h,&off)||
       !musa_dwg_ac1032_object_payload_bytes(
           scene->objects,scene->object_bytes,h,off,
           NULL,0u,&needed,&meta)||
       !meta.original_record_crc_verified||
       meta.raw_object_type!=49u||meta.indexed_handle!=h||
       !needed||needed>MAX_OBJECT_SECTION||
       meta.payload_bytes!=needed||
       !musa_dwg_ac1032_stream_layout_bytes(
           scene->objects,scene->object_bytes,h,off,&layout)||
       !layout.original_record_crc_verified||
       layout.indexed_handle!=h||layout.raw_object_type!=49u||
       layout.object_section_offset!=off||
       layout.total_payload_bits!=(uint64_t)needed*8u||
       layout.handle_stream_bits!=meta.handle_stream_bits)return 0;
    uint8_t *raw=(uint8_t*)malloc(needed);
    if(!raw)return 0;
    MusaDwgAc1032BlockHeaderBase value={0};
    int ok=musa_dwg_ac1032_object_payload_bytes(
           scene->objects,scene->object_bytes,h,off,
           raw,needed,&copied,&again)&&
       copied==needed&&again.original_record_crc_verified&&
       again.raw_object_type==49u&&again.indexed_handle==h&&
       again.object_section_offset==off&&
       musa_dwg_ac1032_block_header_ot49_base_at(
           raw,needed,&layout,&value);
    free(raw);
    if(!ok)return 0;
    *out=value;return 1;
}
int musa_dwg_ac1032_scene_index_read_block_header_field_span(
    const MusaDwgAc1032SceneIndex *scene,uint64_t h,
    MusaDwgAc1032BlockHeaderFieldSpan *out){
    if(!h||!check_output(scene,out,sizeof(*out)))return 0;
    uint64_t off=0u;size_t needed=0u,copied=0u;
    MusaDwgAc1032PayloadInfo meta={0},again={0};
    MusaDwgAc1032ObjectStreams layout={0};
    if(!find(scene,h,&off)||
       !musa_dwg_ac1032_object_payload_bytes(
           scene->objects,scene->object_bytes,h,off,
           NULL,0u,&needed,&meta)||
       !meta.original_record_crc_verified||
       meta.raw_object_type!=49u||meta.indexed_handle!=h||
       !needed||needed>MAX_OBJECT_SECTION||
       meta.payload_bytes!=needed||
       !musa_dwg_ac1032_stream_layout_bytes(
           scene->objects,scene->object_bytes,h,off,&layout)||
       !layout.original_record_crc_verified||
       layout.indexed_handle!=h||layout.raw_object_type!=49u||
       layout.object_section_offset!=off||
       layout.total_payload_bits!=(uint64_t)needed*8u||
       layout.handle_stream_bits!=meta.handle_stream_bits)return 0;
    uint8_t *raw=(uint8_t*)malloc(needed);
    if(!raw)return 0;
    MusaDwgAc1032BlockHeaderFieldSpan value={0};
    int ok=musa_dwg_ac1032_object_payload_bytes(
           scene->objects,scene->object_bytes,h,off,
           raw,needed,&copied,&again)&&
       copied==needed&&again.original_record_crc_verified&&
       again.raw_object_type==49u&&again.indexed_handle==h&&
       again.object_section_offset==off&&
       musa_dwg_ac1032_block_header_ot49_field_span(
           raw,needed,&layout,&value);
    free(raw);
    if(!ok)return 0;
    *out=value;return 1;
}
int musa_dwg_ac1032_scene_index_read_block_header_identity(
    const MusaDwgAc1032SceneIndex *scene,uint64_t h,
    MusaDwgAc1032BlockHeaderIdentity *out){
    if(!h||!check_output(scene,out,sizeof(*out)))return 0;
    uint64_t off=0u;size_t needed=0u,copied=0u;
    MusaDwgAc1032PayloadInfo meta={0},again={0};
    MusaDwgAc1032ObjectStreams layout={0};
    if(!find(scene,h,&off)||
       !musa_dwg_ac1032_object_payload_bytes(
           scene->objects,scene->object_bytes,h,off,
           NULL,0u,&needed,&meta)||
       !meta.original_record_crc_verified||
       meta.raw_object_type!=49u||meta.indexed_handle!=h||
       !needed||needed>MAX_OBJECT_SECTION||
       meta.payload_bytes!=needed||
       !musa_dwg_ac1032_stream_layout_bytes(
           scene->objects,scene->object_bytes,h,off,&layout)||
       !layout.original_record_crc_verified||
       layout.indexed_handle!=h||layout.raw_object_type!=49u||
       layout.object_section_offset!=off||
       layout.total_payload_bits!=(uint64_t)needed*8u||
       layout.handle_stream_bits!=meta.handle_stream_bits)return 0;
    uint8_t *raw=(uint8_t*)malloc(needed);
    if(!raw)return 0;
    MusaDwgAc1032BlockHeaderIdentity value={0};
    int ok=musa_dwg_ac1032_object_payload_bytes(
           scene->objects,scene->object_bytes,h,off,
           raw,needed,&copied,&again)&&
       copied==needed&&again.original_record_crc_verified&&
       again.raw_object_type==49u&&again.indexed_handle==h&&
       again.object_section_offset==off&&
       musa_dwg_ac1032_block_header_ot49_identity(
           raw,needed,&layout,&value);
    free(raw);
    if(!ok)return 0;
    *out=value;return 1;
}
int musa_dwg_ac1032_scene_index_read_layer_name(
    const MusaDwgAc1032SceneIndex *scene,uint64_t h,
    MusaDwgAc1032LayerName *out){
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
    MusaDwgAc1032LayerName value={0};
    int ok=musa_dwg_ac1032_object_payload_bytes(
         scene->objects,scene->object_bytes,h,off,
         raw,needed,&copied,&again)&&
       copied==needed&&again.original_record_crc_verified&&
       again.raw_object_type==51u&&again.indexed_handle==h&&
       again.object_section_offset==meta.object_section_offset&&
       musa_dwg_ac1032_layer_ot51_name_at(
         raw,needed,&layout,&value)&&
       value.indexed_handle==h;
    free(raw);
    if(ok)*out=value;
    return ok;
}
/* Same checked immutable cached source lookup as Unicode name; raw flags only. */
int musa_dwg_ac1032_scene_index_read_layer_flags_evidence(
    const MusaDwgAc1032SceneIndex *scene,uint64_t h,
    MusaDwgAc1032LayerFlagsEvidence *out){
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
    MusaDwgAc1032LayerFlagsEvidence value={0};
    int ok=musa_dwg_ac1032_object_payload_bytes(
         scene->objects,scene->object_bytes,h,off,
         raw,needed,&copied,&again)&&
       copied==needed&&again.original_record_crc_verified&&
       again.raw_object_type==51u&&again.indexed_handle==h&&
       again.object_section_offset==meta.object_section_offset&&
       musa_dwg_ac1032_layer_ot51_flags_evidence_at(
         raw,needed,&layout,&value)&&
       value.indexed_handle==h;
    free(raw);
    if(ok)*out=value;
    return ok;
}
/* Exact source H/CRC/OT51 lookup before narrow anchored CMC evidence. */
int musa_dwg_ac1032_scene_index_layer_anchored_cmc_evidence(
    const MusaDwgAc1032SceneIndex *scene,uint64_t h,
    MusaDwgAc1032LayerCmcEvidence *out){
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
    MusaDwgAc1032LayerCmcEvidence value={0};
    int ok=musa_dwg_ac1032_object_payload_bytes(
         scene->objects,scene->object_bytes,h,off,
         raw,needed,&copied,&again)&&
       copied==needed&&again.original_record_crc_verified&&
       again.raw_object_type==51u&&again.indexed_handle==h&&
       again.object_section_offset==meta.object_section_offset&&
       musa_dwg_ac1032_layer_ot51_anchored_cmc_evidence_at(
         raw,needed,&layout,&value)&&
       value.indexed_handle==h;
    free(raw);
    if(ok)*out=value;
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
       (type!=1u&&type!=17u&&type!=18u&&type!=19u&&type!=27u&&
         type!=35u&&type!=44u&&type!=77u))
        return 0;
    /* OT1 TEXT has two H5 targets (LAYER and STYLE) and a source-specific
     * terminal 111, not the single-H LINE grammar. Require the existing
     * independently authenticated H graph before exposing its layer. */
    if(type==1u){
        MusaDwgAc1032TextGraph graph={0};
        if(!musa_dwg_ac1032_scene_index_text_graph(s,h,&graph)||
           !graph.graph_edges_verified||
           !graph.original_entity_crc_verified||
           !graph.original_layer_crc_verified||
           !graph.original_style_crc_verified||
           graph.font_and_display_accepted||
           graph.indexed_handle!=h||
           !graph.layer_handle||graph.layer_object_type!=51u||
           !graph.style_handle||graph.style_object_type!=53u)return 0;
        MusaDwgAc1032EntityLayerRef linked={0};
        linked.entity_handle=h;
        linked.layer_handle=graph.layer_handle;
        linked.raw_entity_type=type;
        linked.raw_layer_type=51u;
        linked.original_entity_crc_verified=1u;
        linked.original_layer_crc_verified=1u;
        linked.source_handle_ref_verified=1u;
        *out=linked;
        return 1;
    }
    /* MTEXT uses its own already original-H/CRC verified owner/layer/
     * text-style graph, including the exact optional 0x50 null handle.
     * Reuse it instead of incorrectly applying the LINE single-H tail. */
    if(type==44u){
        MusaDwgAc1032MTextGraph graph={0};
        if(!musa_dwg_ac1032_scene_index_mtext_graph(
              s,h,&graph)||
           !graph.graph_edges_verified||
           !graph.original_entity_crc_verified||
           !graph.original_layer_crc_verified||
           !graph.original_style_crc_verified||
           graph.font_and_display_accepted||
           graph.indexed_handle!=h||
           !graph.layer_handle||
           graph.layer_object_type!=51u)return 0;
        MusaDwgAc1032EntityLayerRef linked={0};
        linked.entity_handle=h;
        linked.layer_handle=graph.layer_handle;
        linked.raw_entity_type=type;
        linked.raw_layer_type=51u;
        linked.original_entity_crc_verified=1u;
        linked.original_layer_crc_verified=1u;
        linked.source_handle_ref_verified=1u;
        *out=linked;
        return 1;
    }
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
/* No default '0' or palette guesses: the actual source entity H=5
 * points at an original CRC-verified OT51, whose independently parsed TU
 * name is returned with its original bit offset and Unicode evidence. */
int musa_dwg_ac1032_scene_index_entity_layer_name(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaDwgAc1032EntityLayerName *out){
    if(!h||!check_output(s,out,sizeof(*out)))return 0;
    MusaDwgAc1032EntityLayerRef link={0};
    MusaDwgAc1032LayerIdentity identity={0};
    MusaDwgAc1032LayerName layer={0};
    if(!musa_dwg_ac1032_scene_index_entity_layer_ref(s,h,&link)||
       link.entity_handle!=h||!link.layer_handle||
       link.raw_layer_type!=51u||
       !link.source_handle_ref_verified||
       !link.original_entity_crc_verified||
       !link.original_layer_crc_verified||
       !musa_dwg_ac1032_scene_index_read_layer_identity(
             s,link.layer_handle,&identity)||
       !identity.original_record_crc_verified||
       !identity.embedded_self_handle_verified||
       identity.indexed_handle!=link.layer_handle||
       !musa_dwg_ac1032_scene_index_read_layer_name(
             s,link.layer_handle,&layer)||
       !layer.original_record_crc_verified||
       layer.indexed_handle!=link.layer_handle||
       !layer.utf16_code_units||!layer.utf8_bytes||
       layer.utf8_bytes>=sizeof(layer.utf8)||
       layer.utf8[layer.utf8_bytes]!='\0')
        return 0;
    MusaDwgAc1032EntityLayerName result={0};
    result.edge=link;
    result.layer=layer;
    result.source_graph_and_name_verified=1u;
    *out=result;
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
        case 35u: {
            MusaDwgAc1032NativeEllipse ellipse={0};
            geometry=musa_dwg_ac1032_scene_index_read_ellipse(
                 s,entry.handle,&ellipse);
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

/* Explicit ORIGINAL 2018 raw types currently missing full text/block
 * geometry support in the native MC1032 preview. Counts are source
 * record evidence only, not visible-entity totals. No external codec. */
int musa_dwg_ac1032_scene_index_unrendered_2d_inventory(
    const MusaDwgAc1032SceneIndex *index,
    MusaDwgAc1032Unrendered2dInventory *out){
    if(!index||!out||!index->handles||!index->objects||
       !index->count||index->count>MAX_INDEX||
       !check_output(index,out,sizeof(*out)))
        return 0;
    MusaDwgAc1032Unrendered2dInventory local={0};
    local.indexed_records=index->count;
    uint64_t previous=0u;
    for(size_t i=0u;i<index->count;i++){
        MusaDwgAc1032HandleEntry entry={0};
        uint16_t type=0u;
        if(!musa_dwg_ac1032_scene_index_entry(index,i,&entry)||
           !entry.handle||entry.handle<=previous||
           !musa_dwg_ac1032_scene_index_object_type(
             index,entry.handle,&type))
            return 0;
        previous=entry.handle;
        switch(type){
        case 1u:local.text_records++;break;
        case 44u:local.mtext_records++;break;
        case 2u:local.attrib_records++;break;
        case 3u:local.attdef_records++;break;
        case 4u:local.block_begin_records++;break;
        case 5u:local.block_end_records++;break;
        case 7u:local.insert_records++;break;
        case 8u:local.minsert_records++;break;
        default:local.other_indexed_records++;break;
        }
    }
    if(local.indexed_records!=
       local.text_records+local.mtext_records+
       local.attrib_records+local.attdef_records+
       local.block_begin_records+local.block_end_records+
       local.insert_records+local.minsert_records+
       local.other_indexed_records)
        return 0;
    local.original_types_crc_verified=1u;
    local.complete_2d_display_accepted=0u;
    *out=local;
    return 1;
}
