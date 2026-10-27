/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_text_v6_packet.h"
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
static int f32(double d){return isfinite(d)&&d<=FLT_MAX&&d>=-FLT_MAX;}
static int overlap(const void *a,size_t n,const void *b,size_t m){
    if(!n||!m)return 0;
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<n:x-y<m;
}
int musa_dwg_ac1032_text_v6_packet(
    const MusaDwgAc1032SceneIndex *scene,uint64_t handle,
    float *words,size_t capacity,MusaAc1032TextV6PacketStats *out){
    if(!scene||!handle||!out||(!words&&capacity)||
       capacity>SIZE_MAX/sizeof(float)||
       overlap(words,capacity*sizeof(float),out,sizeof(*out)))return 0;
    MusaDwgAc1032TextBundle b={0};
    MusaDwgAc1032TextPlacement p={0};
    MusaDwgAc1032EntityCommon c={0};
    if(!musa_dwg_ac1032_scene_text_bundle(scene,handle,&b)||
       !musa_dwg_ac1032_scene_index_read_text_placement(scene,handle,&p)||
       !musa_dwg_ac1032_scene_index_read_text_common(scene,handle,&c))
        return 0;
    const unsigned ha=b.world.horizontal_alignment;
    const unsigned va=b.world.vertical_alignment;
    const size_t bytes=b.unicode.utf8_bytes;
    /* Unsupported FIT/ALIGNED requires original font width, and unknown
     * generation/oblique/mirror cannot be approximated as a faithful glyph.
     * This intentionally accepts only simple text or centered/middle/right
     * source anchors as explicitly provisional font rendering. */
    if(!b.source_join_verified||b.font_glyph_layout_accepted||
       b.model_viewport_visibility_accepted||b.final_display_color_accepted||
       !b.layer_flags.source_crc_verified||!b.layer_bits.source_crc_verified||
       b.layer_bits.display_accepted||
       b.graph.layer_handle!=b.color.layer_handle||
       !b.color.entity_and_layer_crc_verified||
       b.color.entity_handle!=handle||b.color.raw_entity_type!=1u||
       !p.numeric_fields_decoded||!p.original_record_crc_verified||
       p.indexed_handle!=handle||p.generation!=0u||
       !isfinite(p.oblique)||p.oblique!=0.0||
       !isfinite(p.width_factor)||p.width_factor!=1.0||
       !c.common_decoded||c.prefix.indexed_handle!=handle||
       c.prefix.raw_object_type!=1u||c.invisibility!=0u||
       b.color.source_entity_invisible||
       c.color_index!=b.color.entity_aci||
       bytes==0u||bytes>=sizeof(b.unicode.utf8)||
       b.unicode.utf8[bytes]!='\0'||strlen(b.unicode.utf8)!=bytes||
       !((ha<=2u)||ha==4u)||va>3u||
       !f32(b.world.height)||b.world.height<=0.0||
       !b.world.wcs_math_decoded||b.world.font_and_display_accepted||
       b.world.indexed_handle!=handle||
       !b.world.original_record_crc_verified)return 0;
    const double *anchor=(ha||va)?b.world.alignment:b.world.insertion;
    const double *u=b.world.baseline_unit,*v=b.world.up_unit;
    if(!f32(anchor[0])||!f32(anchor[1])||
       !isfinite(anchor[2])||anchor[2]!=0.0||
       !f32(u[0])||!f32(u[1])||!isfinite(u[2])||u[2]!=0.0||
       !f32(v[0])||!f32(v[1])||!isfinite(v[2])||v[2]!=0.0||
       fabs(hypot(u[0],u[1])-1.0)>1e-6||
       fabs(hypot(v[0],v[1])-1.0)>1e-6||
       fabs(u[0]*v[0]+u[1]*v[1])>1e-6||
       fabs(u[0]*v[1]-u[1]*v[0]-1.0)>1e-6||
       !f32(u[0]*b.world.height)||!f32(u[1]*b.world.height)||
       !f32(v[0]*b.world.height)||!f32(v[1]*b.world.height))
       return 0;
    float code=0.f,rgb=0.f;
    uint8_t extra=0u,exact=0u;
    if(!musa_dwg_ac1032_preview_v6_entity_source_style_code(
        &c,6u,&code,&rgb,&extra,&exact))return 0;
    /* Independent original OT1 H/CRC entity->LAYER source evidence must
     * agree with the first-frame v6 color. A source direct RGB/ACI is
     * NOT proof of owner/model-space, frozen/off, viewport or glyph paint.
     * BYLAYER/BYBLOCK and complex ENC stay provisional, never guessed. */
    const int plain=!c.has_rgb&&!c.has_color_reference&&
        !c.has_transparency;
    const int direct_rgb=c.has_rgb&&!c.has_color_reference&&
        !c.has_transparency&&c.rgb<=UINT32_C(0x00ffffff);
    if(direct_rgb){
        if(!b.color.entity_has_direct_rgb24||
           b.color.entity_source_rgb24!=c.rgb||!exact||
           extra!=1u||code!=-1538.f||rgb!=(float)c.rgb)return 0;
    }else if(plain&&c.color_index>=1u&&c.color_index<=255u){
        if(!b.color.entity_has_direct_aci||!exact||extra!=0u||
           code!=(float)(1536u+c.color_index))return 0;
    }else if(plain&&c.color_index==256u){
        if(!b.color.entity_by_layer||exact||code!=1543.f)return 0;
    }else if(plain&&c.color_index==0u){
        if(!b.color.entity_by_block||exact||code!=1543.f)return 0;
    }else if(!b.color.entity_complex_color_or_transparency||
             exact||code!=1543.f)return 0;
    /* Type6: [code,(RGB),x,y,ux,uy,vx,vy,hAlign,vAlign,byteCount,
     * packed UTF8 in 24-bit exact float words]. v-axis is negative
     * source WCS up, matching the established native Canvas grammar. */
    const size_t packed=(bytes+2u)/3u;
    if(packed>SIZE_MAX-10u-extra)return 0;
    const size_t need=10u+packed+extra;
    if(words&&capacity<need)return 0;
    MusaAc1032TextV6PacketStats result={0};
    result.source_handle=handle;
    result.source_layer_handle=b.graph.layer_handle;
    result.float_words=need;
    result.utf8_bytes=(uint32_t)bytes;
    result.source_crc_and_graph_verified=1u;
    result.source_direct_style_exact=exact;
    result.provisional_font_and_visibility=1u;
    if(words){
        words[0]=code;
        if(extra)words[1]=rgb;
        const size_t at=extra;
        words[at+1u]=(float)anchor[0];
        words[at+2u]=(float)anchor[1];
        words[at+3u]=(float)(u[0]*b.world.height);
        words[at+4u]=(float)(u[1]*b.world.height);
        words[at+5u]=(float)(-v[0]*b.world.height);
        words[at+6u]=(float)(-v[1]*b.world.height);
        words[at+7u]=(float)ha;
        words[at+8u]=(float)va;
        words[at+9u]=(float)bytes;
        for(size_t i=0u;i<packed;i++){
            uint32_t w=0u;
            for(size_t k=0u;k<3u;k++){
                const size_t atbyte=i*3u+k;
                if(atbyte<bytes)
                    w|=(uint32_t)(uint8_t)b.unicode.utf8[atbyte]<<(8u*k);
            }
            words[at+10u+i]=(float)w;
        }
    }
    *out=result;return 1;
}
