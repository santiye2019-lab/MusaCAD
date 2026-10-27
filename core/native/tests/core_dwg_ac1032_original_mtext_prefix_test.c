/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original AutoCAD 2018/Text.dwg contains MTEXT H=1C0, never OT1 TEXT.
 * First-party C11 source evidence only, NOT a text/glyph renderer. */
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1032_object_streams.h"
#include "musacad_dwg_ac1032_entity_prefix.h"
#include "musacad_dwg_ac1032_mtext.h"
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_source_utf8.h"
#include "musacad_dwg_ac1032_text_source_bundle.h"
#include "musacad_dwg_ac1015_values.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int load(const char *path,uint8_t **out,size_t *n){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    long bytes=ftell(f);
    if(bytes<256L||bytes>64L*1024L*1024L||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
    uint8_t *b=(uint8_t*)malloc((size_t)bytes);
    if(!b){fclose(f);return 0;}
    int ok=fread(b,1u,(size_t)bytes,f)==(size_t)bytes;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(b);return 0;}
    *out=b;*n=(size_t)bytes;return 1;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    uint8_t *file=NULL,*raw=NULL;size_t n=0u,need=0u,got=0u;
    if(!load(argv[1],&file,&n))return 1;
    MusaDwgAc1032PayloadInfo meta={0};
    MusaDwgAc1032ObjectStreams layout={0};
    const uint64_t h=UINT64_C(0x1c0);
    int ok=!memcmp(file,"AC1032",6u)&&
       musa_dwg_ac1032_object_payload_file(
           file,n,h,NULL,0u,&need,&meta)&&
       meta.original_record_crc_verified&&meta.raw_object_type==44u&&
       musa_dwg_ac1032_stream_layout_file(file,n,h,&layout)&&
       layout.original_record_crc_verified&&
       layout.raw_object_type==44u&&
       layout.total_payload_bits==(uint64_t)need*8u;
    if(!ok){fprintf(stderr,"FAIL original MTEXT H1C0 authenticated payload/layout\n");free(file);return 1;}
    raw=(uint8_t*)malloc(need);
    if(!raw){free(file);return 1;}
    ok=musa_dwg_ac1032_object_payload_file(
         file,n,h,raw,need,&got,&meta)&&got==need;
    MusaDwgAc1032EntityCommon common={0};
    int common_ok=ok&&musa_dwg_ac1032_entity_common_bytes(
         raw,need,&layout,44u,&common);
    const size_t end=(size_t)layout.data_stream_bits;
    uint32_t flag=0u,low=0u,high=0u;
    MusaDwgAc1015Bits b={0};
    int tu_frame=ok&&end>=17u&&
       musa_dwg_ac1015_bits_init(&b,raw,need,end-1u,end)&&
       musa_dwg_ac1015_bits_read_u(&b,1u,&flag)&&
       musa_dwg_ac1015_bits_init(&b,raw,need,end-17u,end-1u)&&
       musa_dwg_ac1015_bits_read_u(&b,8u,&low)&&
       musa_dwg_ac1015_bits_read_u(&b,8u,&high);
    const uint32_t bits=low|(high<<8u);
    fprintf(stderr,"ORIGINAL MTEXT H1C0: payload=%zu data_bits=%llu H_bits=%llu common=%d geom_start=%zu TU_frame=%d TU_flag=%u TU_bits=%u\n",
         need,(unsigned long long)layout.data_stream_bits,
         (unsigned long long)layout.handle_stream_bits,
         common_ok,common.geometry_start_bit,tu_frame,flag,bits);
    static const char expected[]=
       "This a multiline text to check if libredwg is able to read this in the current file format version";
    MusaDwgAc1032MTextSource source={0};
    MusaDwgAc1032SceneIndex *scene=NULL;
    ok=ok&&common_ok&&common.common_decoded&&
       common.prefix.raw_object_type==44u&&
       common.prefix.indexed_handle==h&&
       common.geometry_start_bit<end&&
       tu_frame&&flag==1u&&bits==1578u&&
       musa_dwg_ac1032_mtext_single_tu_at(
          raw,need,&layout,common.geometry_start_bit,&source)&&
       source.indexed_handle==h&&
       source.utf16_code_units==98u&&
       source.utf8_bytes==strlen(expected)&&
       musa_dwg_ac1032_source_utf8_valid(
           source.utf8,source.utf8_bytes,sizeof(source.utf8),1)&&
       !strcmp(source.utf8,expected)&&
       source.original_record_crc_verified&&
       !source.geometry_style_and_glyphs_accepted&&
       musa_dwg_ac1032_scene_index_open(file,n,&scene);
    if(ok){
        MusaDwgAc1032MTextSource cached={0};
        ok=musa_dwg_ac1032_scene_index_read_mtext_source(
               scene,h,&cached)&&
           !memcmp(&source,&cached,sizeof(source));
        const MusaDwgAc1032MTextSource saved=source;
        const size_t bit=end-1u;
        raw[bit/8u]^=(uint8_t)(1u<<(7u-bit%8u));
        if(musa_dwg_ac1032_mtext_single_tu_at(
               raw,need,&layout,common.geometry_start_bit,&source)||
           memcmp(&source,&saved,sizeof(source))!=0)ok=0;
        raw[bit/8u]^=(uint8_t)(1u<<(7u-bit%8u));
        layout.raw_object_type=1u; /* OT1 TEXT is a DIFFERENT decoder */
        if(musa_dwg_ac1032_mtext_single_tu_at(
               raw,need,&layout,common.geometry_start_bit,&source)||
           memcmp(&source,&saved,sizeof(source))!=0)ok=0;
    }
    if(ok){
        MusaDwgAc1032MTextGraph graph={0};
        ok=musa_dwg_ac1032_scene_index_mtext_graph(scene,h,&graph)&&
           graph.indexed_handle==h&&graph.graph_edges_verified&&
           graph.original_entity_crc_verified&&
           graph.original_layer_crc_verified&&
           graph.original_style_crc_verified&&
           graph.layer_object_type==51u&&graph.style_object_type==53u&&
           graph.layer_handle!=0u&&graph.style_handle!=0u&&
           !graph.font_and_display_accepted;
    }
    if(ok){
        MusaDwgAc1032TextSourceBundle bundle={0};
        ok=musa_dwg_ac1032_text_source_bundle(scene,h,44u,&bundle)&&
           bundle.indexed_handle==h&&bundle.raw_object_type==44u&&
           bundle.original_unicode_numeric_h_graph_verified&&
           bundle.original_layer_name_verified&&
           !bundle.font_layout_visibility_color_and_paint_accepted&&
           bundle.layer_handle!=0u&&bundle.style_handle!=0u&&
           bundle.utf8_bytes==strlen(expected)&&
           !strcmp(bundle.utf8,expected);
        MusaDwgAc1032LayerName original_layer={0};
        if(ok)ok=musa_dwg_ac1032_scene_index_read_layer_name(
                       scene,bundle.layer_handle,&original_layer)&&
                  original_layer.original_record_crc_verified&&
                  original_layer.indexed_handle==bundle.layer_handle&&
                  bundle.layer_utf8_bytes==original_layer.utf8_bytes&&
                  bundle.layer_utf8_bytes>0u&&
                  bundle.layer_utf8_bytes<sizeof(bundle.layer_utf8)&&
                  !strcmp(bundle.layer_utf8,original_layer.utf8);
        /* Independent OT44 H graph must agree with the exported source
         * packet; a style handle is not a verified font or glyph layout. */
        MusaDwgAc1032MTextGraph verified_graph={0};
        if(ok)ok=musa_dwg_ac1032_scene_index_mtext_graph(
                      scene,h,&verified_graph)&&
                  verified_graph.style_object_type==53u&&
                  verified_graph.layer_object_type==51u&&
                  bundle.style_handle==verified_graph.style_handle&&
                  bundle.layer_handle==verified_graph.layer_handle;
        if(ok){
            MusaDwgAc1032TextSourceBundle rejected;
            memset(&rejected,0x5a,sizeof(rejected));
            const MusaDwgAc1032TextSourceBundle before=rejected;
            ok=!musa_dwg_ac1032_text_source_bundle(
                   scene,h,1u,&rejected)&&
               !musa_dwg_ac1032_text_source_bundle(
                   scene,UINT64_MAX,44u,&rejected)&&
               !musa_dwg_ac1032_text_source_bundle(
                   scene,0u,44u,&rejected)&&
               !musa_dwg_ac1032_text_source_bundle(
                   scene,h,0u,&rejected)&&
               !musa_dwg_ac1032_text_source_bundle(
                   NULL,h,44u,&rejected)&&
               !musa_dwg_ac1032_text_source_bundle(
                   scene,h,44u,NULL)&&
               !musa_dwg_ac1032_text_source_bundle(
                   scene,h,UINT16_MAX,&rejected)&&
               !memcmp(&rejected,&before,sizeof(rejected));
        }
    }
    fprintf(stderr,"%s original MTEXT TU source Unicode H1C0 exact oracle; NOT full MTEXT glyph/format/placement\\n",ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    free(raw);free(file);
    return ok?0:1;
}
