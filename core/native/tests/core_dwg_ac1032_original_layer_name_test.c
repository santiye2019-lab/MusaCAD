/* SPDX-License-Identifier: GPL-3.0-or-later */
/* First-party real R2018 original LAYER UTF16 TU string stream oracle,
 * using genuine immutable Line.dwg and Point.dwg H=0x10. */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int load(const char *path,uint8_t **data,size_t *n){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    const long z=ftell(f);
    if(z<256||z>64L*1024*1024||fseek(f,0,SEEK_SET)!=0){
       fclose(f);return 0;
    }
    uint8_t *bytes=(uint8_t*)malloc((size_t)z);
    if(!bytes){fclose(f);return 0;}
    int ok=fread(bytes,1u,(size_t)z,f)==(size_t)z;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(bytes);return 0;}
    *n=(size_t)z;*data=bytes;return 1;
}
static void setbits(uint8_t *dst,size_t bitpos,unsigned count,
                    uint32_t value){
    for(unsigned i=0;i<count;i++){
        const size_t p=bitpos+i;
        const uint8_t mask=(uint8_t)(1u<<(7u-p%8u));
        if((value>>(count-1u-i))&1u)dst[p/8u]|=mask;
        else dst[p/8u]&=(uint8_t)~mask;
    }
}
static int run(const char *path,int expanded){
    uint8_t *file=NULL,*copy=NULL,*raw=NULL;size_t n=0;
    if(!load(path,&file,&n))return 0;
    copy=(uint8_t*)malloc(n);
    if(!copy){free(file);return 0;}
    memcpy(copy,file,n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(file,n,&scene);
    MusaDwgAc1032LayerName name={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_read_layer_name(
         scene,UINT64_C(0x10),&name)&&
         name.indexed_handle==UINT64_C(0x10)&&
         name.original_record_crc_verified&&
         name.utf16_code_units==1u&&name.utf8_bytes==1u&&
         strcmp(name.utf8,"0")==0;
    if(ok&&expanded)ok=name.original_string_start_bit==96u;
    MusaDwgAc1032LayerIdentity layer={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_read_layer_identity(
         scene,UINT64_C(0x10),&layer)&&
         layer.data_fields_start_bit<name.original_string_start_bit;
    if(ok){
        MusaDwgAc1032LayerName saved=name;
        ok=!musa_dwg_ac1032_scene_index_read_layer_name(
             scene,UINT64_C(0x1ca),&name)&&
           !memcmp(&name,&saved,sizeof(name))&&
           !musa_dwg_ac1032_scene_index_read_layer_name(
             scene,UINT64_MAX,&name)&&
           !memcmp(&name,&saved,sizeof(name));
    }
    size_t wanted=0u,got=0u;
    MusaDwgAc1032PayloadInfo info={0},again={0};
    MusaDwgAc1032ObjectStreams layout={0};
    if(ok)ok=musa_dwg_ac1032_object_payload_file(
        file,n,UINT64_C(0x10),NULL,0u,&wanted,&info)&&
        musa_dwg_ac1032_stream_layout_file(
        file,n,UINT64_C(0x10),&layout)&&
        wanted>0u&&wanted<65536u;
    if(ok){
        raw=(uint8_t*)malloc(wanted);
        ok=raw&&musa_dwg_ac1032_object_payload_file(
             file,n,UINT64_C(0x10),raw,wanted,&got,&again)&&
             got==wanted;
    }
    if(ok){
        MusaDwgAc1032LayerName same={0};
        ok=musa_dwg_ac1032_layer_ot51_name_at(
            raw,wanted,&layout,&same)&&
            !memcmp(&same,&name,sizeof(name));
    }
    if(ok&&expanded){
        MusaDwgAc1032LayerName modified={0},original=name;
        /* Synthetic codepoints edit only a detached RECORD copy to
         * validate the independent UTF16->UTF8 decoder. Not a real DWG. */
        setbits(raw,106u,8u,0xe9u); /* TU U+00E9 little-endian */
        setbits(raw,114u,8u,0u);
        ok=musa_dwg_ac1032_layer_ot51_name_at(
           raw,wanted,&layout,&modified)&&
           modified.utf16_code_units==1u&&
           modified.utf8_bytes==2u&&
           strcmp(modified.utf8,"\xc3\xa9")==0;
        if(ok){
            /* Lone high surrogate U+D800 must reject without writing out. */
            setbits(raw,106u,8u,0u);
            setbits(raw,114u,8u,0xd8u);
            MusaDwgAc1032LayerName before=modified;
            ok=!musa_dwg_ac1032_layer_ot51_name_at(
                  raw,wanted,&layout,&modified)&&
               !memcmp(&before,&modified,sizeof(before));
        }
        if(ok){
            setbits(raw,106u,8u,0x30u);
            setbits(raw,114u,8u,0u);
            const size_t flag=(size_t)layout.data_stream_bits-1u;
            setbits(raw,flag,1u,0u);
            MusaDwgAc1032LayerName before=modified;
            ok=!musa_dwg_ac1032_layer_ot51_name_at(
                  raw,wanted,&layout,&modified)&&
               !memcmp(&before,&modified,sizeof(before));
            setbits(raw,flag,1u,1u);
        }
        if(ok){
            /* Source-CRC-bound stream layout is not allowed to lie about
               actual bytes or OT; failures preserve prior outputs. */
            MusaDwgAc1032LayerName before=modified;
            ok=!musa_dwg_ac1032_layer_ot51_name_at(
                  raw,wanted-1u,&layout,&modified)&&
               !memcmp(&before,&modified,sizeof(before));
        }
        (void)original;
    }
    ok=ok&&!memcmp(file,copy,n);
    fprintf(stderr,"%s original R2018 LAYER UTF16 TU name='0'"
       " sourceH=0x10 and fail atomic, input=%s\n",
       ok?"PASS":"FAIL",path);
    free(raw);free(file);free(copy);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc!=3)return argc==1?0:2;
    return run(argv[1],1)&&run(argv[2],0)?0:1;
}
