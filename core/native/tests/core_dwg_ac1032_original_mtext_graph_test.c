/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real original 2018/Text.dwg H1C0 MTEXT H-tail source graph oracle.
 * AutoCAD Text.txt: Layer "0", StyleName "Standard"; do NOT claim the
 * style name/font is decoded merely because the H target exists. */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1032_object_streams.h"
#include "musacad_dwg_ac1032_entity_prefix.h"
#include "musacad_dwg_ac1015_handle_refs.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void diagnose(const uint8_t *file,size_t n){
    const uint64_t h=UINT64_C(0x1c0);
    MusaDwgAc1032ObjectStreams l={0};
    MusaDwgAc1032PayloadInfo info={0};
    size_t need=0u,got=0u;
    if(!musa_dwg_ac1032_object_payload_file(file,n,h,NULL,0u,&need,&info)||
       !musa_dwg_ac1032_stream_layout_file(file,n,h,&l)||
       !need||need>65536u)return;
    uint8_t *raw=(uint8_t*)malloc(need);
    if(!raw)return;
    MusaDwgAc1032EntityCommon c={0};
    if(musa_dwg_ac1032_object_payload_file(
         file,n,h,raw,need,&got,&info)&&got==need&&
       musa_dwg_ac1032_entity_common_bytes(raw,need,&l,44u,&c)){
        fprintf(stderr,"MTEXT source H: mode=%u Hbits=%llu ltype=%u plot=%u material=%u visual=%u\n",
           (unsigned)c.prefix.entity_mode,
           (unsigned long long)l.handle_stream_bits,
           (unsigned)c.linetype_flags,(unsigned)c.plotstyle_flags,
           (unsigned)c.material_flags,(unsigned)c.visualstyle_flags);
        MusaDwgAc1015Bits b={0};
        if(l.data_stream_bits<=SIZE_MAX&&l.total_payload_bits<=SIZE_MAX&&
           musa_dwg_ac1015_bits_init(&b,raw,need,
             (size_t)l.data_stream_bits,(size_t)l.total_payload_bits)){
            for(unsigned i=0;i<6u&&b.bit_end-b.bit_pos>=8u;i++){
                MusaDwgAc1015HandleRef ref={0};
                if(!musa_dwg_ac1015_read_handle_ref(&b,h,&ref)){
                    fprintf(stderr,"MTEXT H tail remaining=%zu bits: ",
                        b.bit_end-b.bit_pos);
                    MusaDwgAc1015Bits copy=b;
                    while(copy.bit_pos<copy.bit_end){
                        uint32_t bit=0u;
                        if(!musa_dwg_ac1015_bits_read_u(&copy,1u,&bit))break;
                        fputc(bit?'1':'0',stderr);
                    }
                    fputc('\\n',stderr);
                    break;
                }
                fprintf(stderr,"MTEXT H ref[%u] code=%u target=%llX\n",
                   i,(unsigned)ref.code,(unsigned long long)ref.target);
            }
        }
    }
    free(raw);
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");
    if(!f)return 1;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 1;}
    long len=ftell(f);
    if(len<256L||len>64L*1024L*1024L||fseek(f,0,SEEK_SET)!=0){
        fclose(f);return 1;
    }
    uint8_t *file=(uint8_t*)malloc((size_t)len);
    if(!file){fclose(f);return 1;}
    int ok=fread(file,1u,(size_t)len,f)==(size_t)len;
    if(fclose(f)!=0)ok=0;
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=!memcmp(file,"AC1032",6u)&&
       musa_dwg_ac1032_scene_index_open(file,(size_t)len,&scene);
    MusaDwgAc1032MTextGraph g={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_mtext_graph(
        scene,UINT64_C(0x1c0),&g);
    if(!ok)diagnose(file,(size_t)len);
    if(ok){
        MusaDwgAc1032LayerName name={0};
        ok=g.indexed_handle==UINT64_C(0x1c0)&&
           g.layer_handle!=0u&&g.style_handle!=0u&&
           g.layer_object_type==51u&&g.style_object_type!=0u&&
           g.graph_edges_verified&&
           g.original_entity_crc_verified&&
           g.original_layer_crc_verified&&
           g.original_style_crc_verified&&
           !g.font_and_display_accepted&&
           musa_dwg_ac1032_scene_index_read_layer_name(
             scene,g.layer_handle,&name)&&
           !strcmp(name.utf8,"0");
    }
    if(ok){
        const MusaDwgAc1032MTextGraph saved=g;
        if(musa_dwg_ac1032_scene_index_mtext_graph(
              scene,UINT64_MAX,&g)||
           memcmp(&saved,&g,sizeof(g))!=0)ok=0;
    }
    fprintf(stderr,"%s original MTEXT H1C0 source layer/style edges; font and final display UNRESOLVED\n",
         ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    free(file);
    return ok?0:1;
}
