/* SPDX-License-Identifier: GPL-3.0-or-later */
/* FIRST-PARTY CMC color parser: synthetic well-specified R2004+ fields
 * AND real AutoCAD 2018 negative evidence for unsafe color-offset guesses.
 * No LibreDWG parser, no graphics claims. */
#include "musacad_dwg_cmc.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1032_scene_index.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef struct Writer {uint8_t data[64];size_t bit;} Writer;
static int put(Writer *w,unsigned size,uint32_t v){
    if(!w||size>32u||w->bit+size>sizeof(w->data)*8u)return 0;
    for(unsigned i=0;i<size;i++){
        const size_t pos=w->bit++;
        const unsigned shift=size-1u-i;
        if((v>>shift)&1u)
            w->data[pos/8u]|=(uint8_t)(1u<<(7u-pos%8u));
    }
    return 1;
}
static size_t make(Writer *w,unsigned padding,
                   uint16_t index,uint32_t rgb,uint8_t flags,
                   unsigned rgb_form){
    memset(w,0,sizeof(*w));
    if(padding>7u)return 0;
    w->bit=padding;
    /* Supported CMC ordinary BS index 0: 2-bit 10 form. An invalid
     * nonzero index can be generated through 01 followed by one byte. */
    if(index==0u){if(!put(w,2u,2u))return 0;}
    else if(!put(w,2u,1u)||!put(w,8u,index))return 0;
    if(!put(w,2u,rgb_form))return 0;
    if(rgb_form==0u){
        for(unsigned i=0;i<4u;i++)
            if(!put(w,8u,(rgb>>(i*8u))&255u))return 0;
    }else if(rgb_form==1u){
        if(rgb>255u||!put(w,8u,rgb))return 0;
    }else if(rgb_form!=2u)return 0;
    if(!put(w,8u,flags))return 0;
    return w->bit;
}
static int synthetic(void){
    const struct Example{uint32_t value;unsigned form;} cases[]={
        {UINT32_C(0xC2FA6735),0u},
        {UINT32_C(0xC3000000),0u},
        {UINT32_C(0x00000000),0u},
        {UINT32_C(0x0000007F),1u},
        {UINT32_C(0x00000000),2u}};
    for(unsigned off=0u;off<8u;off++){
        for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);i++){
            Writer w={0};
            size_t end=make(&w,off,0u,cases[i].value,0u,cases[i].form);
            if(!end)return 0;
            MusaDwgCmcField out={0};
            if(!musa_dwg_cmc_read_numeric_field(
                    w.data,sizeof(w.data),off,end,&out)||
               out.indexed_color!=0u||
               out.packed_rgb!=cases[i].value||
               out.start_bit!=off||out.end_bit!=end||
               !out.supported_numeric_only||
               out.color_name_flag||out.book_name_flag)return 0;
            const MusaDwgCmcField prior=out;
            if(musa_dwg_cmc_read_numeric_field(
                w.data,sizeof(w.data),off,end-1u,&out)||
               memcmp(&prior,&out,sizeof(out))||
               musa_dwg_cmc_read_numeric_field(
                w.data,sizeof(w.data),end,off,&out)||
               memcmp(&prior,&out,sizeof(out))||
               musa_dwg_cmc_read_numeric_field(
                w.data,sizeof(w.data),off,end,NULL)||
               musa_dwg_cmc_read_numeric_field(
                w.data,sizeof(w.data),off,end,
                (MusaDwgCmcField*)(void*)w.data)||
               memcmp(&prior,&out,sizeof(out)))return 0;
        }
    }
    for(unsigned flag=1u;flag<=4u;flag++){
        Writer w={0};size_t end=make(&w,3u,0u,0u,(uint8_t)flag,2u);
        MusaDwgCmcField output={0};output.packed_rgb=123u;
        if(!end||musa_dwg_cmc_read_numeric_field(
              w.data,sizeof(w.data),3u,end,&output)||
           output.packed_rgb!=123u)return 0;
    }
    Writer w={0};
    size_t end=make(&w,0u,7u,0u,0u,2u);
    MusaDwgCmcField last={0};last.packed_rgb=777u;
    if(!end||musa_dwg_cmc_read_numeric_field(
          w.data,sizeof(w.data),0u,end,&last)||
       last.packed_rgb!=777u)return 0;
    return 1;
}
static int source_refusal(const char *file){
    FILE *f=fopen(file,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    const long length=ftell(f);
    if(length<256||length>64L*1024L*1024L||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
    uint8_t *src=(uint8_t*)malloc((size_t)length);
    uint8_t *old=(uint8_t*)malloc((size_t)length);
    if(!src||!old){free(src);free(old);fclose(f);return 0;}
    int ok=fread(src,1u,(size_t)length,f)==(size_t)length;
    if(fclose(f)!=0)ok=0;
    memcpy(old,src,(size_t)length);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
         src,(size_t)length,&scene);
    MusaDwgAc1032LayerFlagsEvidence evidence={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_read_layer_flags_evidence(
        scene,UINT64_C(0x10),&evidence)&&
        evidence.numeric_flags_end_bit==45u;
    MusaDwgAc1032ObjectStreams layout={0};
    MusaDwgAc1032PayloadInfo meta={0},again={0};
    size_t required=0u,got=0u;
    if(ok)ok=musa_dwg_ac1032_object_payload_file(
         src,(size_t)length,UINT64_C(0x10),
         NULL,0u,&required,&meta)&&
       musa_dwg_ac1032_stream_layout_file(
         src,(size_t)length,UINT64_C(0x10),&layout)&&
       meta.original_record_crc_verified&&required>0u&&
       required<65536u;
    uint8_t *raw=NULL;
    if(ok)raw=(uint8_t*)malloc(required);
    if(ok)ok=raw&&musa_dwg_ac1032_object_payload_file(
          src,(size_t)length,UINT64_C(0x10),
          raw,required,&got,&again)&&got==required;
    if(ok){
        MusaDwgCmcField out={0};out.packed_rgb=951u;
        /* This source offset is NOT certified to begin a valid CMC:
         * raw numerical fields ending at bit45 do not imply CMC starts
         * there. Refuse to fabricate a palette / BYLAYER color. */
        ok=!musa_dwg_cmc_read_numeric_field(
              raw,required,evidence.numeric_flags_end_bit,
              (size_t)layout.data_stream_bits,&out)&&
           out.packed_rgb==951u;
    }
    ok=ok&&!memcmp(src,old,(size_t)length);
    fprintf(stderr,"%s genuine R2018 original OT51 raw CMC offset"
       " must NOT be treated as verified layer color\n",
       ok?"PASS":"FAIL");
    free(src);free(old);free(raw);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(!synthetic())return 1;
    if(argc==1)return 0;
    if(argc!=2)return 2;
    return source_refusal(argv[1])?0:1;
}
