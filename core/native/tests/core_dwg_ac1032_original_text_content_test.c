/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Source-only real AC1032 TEXT Unicode content, not glyph placement.
 * Independent synthetic bit-level TU profile + genuine immutable R2018 DWG. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void put(uint8_t *bytes,size_t start,unsigned n,uint32_t value){
    for(unsigned i=0u;i<n;i++){
        const size_t p=start+i;
        const uint8_t mask=(uint8_t)(1u<<(7u-p%8u));
        if((value>>(n-1u-i))&1u)bytes[p/8u]|=mask;
        else bytes[p/8u]&=(uint8_t)~mask;
    }
}
static int synthetic(void){
    uint8_t raw[16]={0};
    MusaDwgAc1032ObjectStreams layout={0};
    layout.indexed_handle=42u;
    layout.raw_object_type=1u;
    layout.object_type_field_bits=10u;
    layout.original_record_crc_verified=1u; /* upstream invariant for unit */
    layout.total_payload_bits=128u;
    layout.data_stream_bits=112u;
    layout.handle_stream_start_bit=112u;
    layout.handle_stream_bits=16u;
    /* A single R2018 TU string "AB" at DATA end:
     * BS units=2 => 01 00000010, two UTF16LE chars, 16-bit bit
     * length=42, terminal STRING-present bit=1. */
    const size_t start=112u-17u-42u;
    put(raw,start,2u,1u);
    put(raw,start+2u,8u,2u);
    put(raw,start+10u,8u,'A');put(raw,start+18u,8u,0u);
    put(raw,start+26u,8u,'B');put(raw,start+34u,8u,0u);
    put(raw,95u,8u,42u);put(raw,103u,8u,0u);
    put(raw,111u,1u,1u);
    MusaDwgAc1032NativeTextContent text={0};
    int ok=musa_dwg_ac1032_text_content_at(
         raw,sizeof(raw),&layout,20u,&text)&&
         text.indexed_handle==42u&&text.utf16_code_units==2u&&
         text.utf8_bytes==2u&&strcmp(text.utf8,"AB")==0&&
         text.original_record_crc_verified==1u&&
         text.geometry_and_style_accepted==0u;
    if(!ok)return 0;
    const MusaDwgAc1032NativeTextContent saved=text;
    put(raw,111u,1u,0u);
    if(musa_dwg_ac1032_text_content_at(
          raw,sizeof(raw),&layout,20u,&text)||
       memcmp(&text,&saved,sizeof(text))!=0)return 0;
    put(raw,111u,1u,1u);
    layout.raw_object_type=44u; /* MTEXT is NOT verified by TEXT parser */
    if(musa_dwg_ac1032_text_content_at(
         raw,sizeof(raw),&layout,20u,&text)||
       memcmp(&text,&saved,sizeof(text))!=0)return 0;
    return !musa_dwg_ac1032_text_content_at(
         raw,sizeof(raw),&layout,20u,NULL);
}
static int load(const char *name,uint8_t **out,size_t *length){
    FILE *f=fopen(name,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    long bytes=ftell(f);
    if(bytes<256||bytes>64L*1024L*1024L||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
    uint8_t *data=(uint8_t*)malloc((size_t)bytes);
    if(!data){fclose(f);return 0;}
    int ok=fread(data,1u,(size_t)bytes,f)==(size_t)bytes;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(data);return 0;}
    *out=data;*length=(size_t)bytes;return 1;
}
static int genuine(const char *path){
    uint8_t *file=NULL,*copy=NULL;size_t len=0u;
    if(!load(path,&file,&len)){fprintf(stderr,"TEXT_FIXTURE_LOAD_FAIL: %s\n",path);return 0;}
    copy=(uint8_t*)malloc(len);
    if(!copy){free(file);fprintf(stderr,"TEXT_FIXTURE_ALLOC_FAIL\n");return 0;}
    memcpy(copy,file,len);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(file,len,&scene)&&
           memcmp(file,copy,len)==0;
    free(copy);free(file);
    if(!ok){fprintf(stderr,"TEXT_SCENE_OPEN_FAIL: verified R2018 source unsupported, bytes=%zu\n",len);return 0;}
    const size_t n=musa_dwg_ac1032_scene_index_count(scene);
    size_t text_records=0u,valid_strings=0u;
    for(size_t i=0u;i<n&&ok;i++){
        MusaDwgAc1032HandleEntry e={0};uint16_t type=0u;
        ok=musa_dwg_ac1032_scene_index_entry(scene,i,&e)&&
           musa_dwg_ac1032_scene_index_object_type(
               scene,e.handle,&type);
        if(!ok)break;
        if(type!=1u)continue;
        text_records++;
        MusaDwgAc1032NativeTextContent content={0};
        if(!musa_dwg_ac1032_scene_index_read_text_content(
            scene,e.handle,&content)){
            fprintf(stderr,"R2018 original TEXT H=%llX TU unsupported\n",
                    (unsigned long long)e.handle);
            continue;
        }
        if(content.indexed_handle!=e.handle||
           !content.original_record_crc_verified||
           content.geometry_and_style_accepted||
           !content.utf16_code_units||!content.utf8_bytes||
           content.utf8_bytes>=sizeof(content.utf8)||
           strlen(content.utf8)!=content.utf8_bytes){ok=0;break;}
        valid_strings++;
        fprintf(stderr,"TEXT source UTF8 H=%llX: %s\n",
                (unsigned long long)e.handle,content.utf8);
        MusaDwgAc1032NativeTextContent before=content;
        if(musa_dwg_ac1032_scene_index_read_text_content(
             scene,UINT64_MAX,&content)||
           memcmp(&before,&content,sizeof(content))!=0){ok=0;break;}
    }
    ok=ok&&text_records>0u&&valid_strings>0u;
    fprintf(stderr,"%s authentic TEXT.dwg indexed=%zu parsedTU=%zu "
         "text-position/style=UNRESOLVED\n",
         ok?"PASS":"FAIL",text_records,valid_strings);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(!synthetic()){fprintf(stderr,"FAIL TEXT TU smoke\n");return 1;}
    if(argc==1)return 0;
    if(argc!=2)return 2;
    return genuine(argv[1])?0:1;
}
