/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original AC1032 R2018 TEXT/BLOCK raw-type absence made explicit.
 * This is NOT text decoding, block expansion or proof of viewport display.
 * Uses source H/CRC index exclusively; never links any external DWG codec.
 */
#include "musacad_dwg_ac1032_scene_index.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_FILE (64u*1024u*1024u)
static int load(const char *name,uint8_t **bytes,size_t *n){
    FILE *f=fopen(name,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    long len=ftell(f);
    if(len<256||len>(long)MAX_FILE||fseek(f,0,SEEK_SET)!=0){
        fclose(f);return 0;
    }
    uint8_t *p=(uint8_t*)malloc((size_t)len);
    if(!p){fclose(f);return 0;}
    int ok=fread(p,1u,(size_t)len,f)==(size_t)len;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(p);return 0;}
    *bytes=p;*n=(size_t)len;return 1;
}
static int inspect(const char *path){
    uint8_t *bytes=NULL,*copy=NULL;size_t n=0u;
    if(!load(path,&bytes,&n))return 0;
    copy=(uint8_t*)malloc(n);
    if(!copy){free(bytes);return 0;}
    memcpy(copy,bytes,n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(bytes,n,&scene)&&
           memcmp(bytes,copy,n)==0;
    free(bytes);free(copy);
    if(!ok)return 0;
    MusaDwgAc1032Unrendered2dInventory actual={0},other={0};
    ok=musa_dwg_ac1032_scene_index_unrendered_2d_inventory(
        scene,&actual)&&actual.indexed_records>0u&&
        actual.original_types_crc_verified==1u&&
        actual.complete_2d_display_accepted==0u;
    if(ok){
        /* Independent source type oracle from the same immutable index:
         * no assumptions about whether a fixture happens to include text. */
        other.indexed_records=musa_dwg_ac1032_scene_index_count(scene);
        uint64_t previous=0u;
        for(size_t i=0u;i<other.indexed_records&&ok;i++){
            MusaDwgAc1032HandleEntry entry={0};uint16_t t=0u;
            ok=musa_dwg_ac1032_scene_index_entry(scene,i,&entry)&&
                entry.handle>previous&&
                musa_dwg_ac1032_scene_index_object_type(
                    scene,entry.handle,&t);
            if(!ok)break;
            previous=entry.handle;
            switch(t){
            case 1u:other.text_records++;break;
            case 44u:other.mtext_records++;break;
            case 2u:other.attrib_records++;break;
            case 3u:other.attdef_records++;break;
            case 4u:other.block_begin_records++;break;
            case 5u:other.block_end_records++;break;
            case 7u:other.insert_records++;break;
            case 8u:other.minsert_records++;break;
            default:other.other_indexed_records++;break;
            }
        }
        other.original_types_crc_verified=1u;
        if(ok)ok=memcmp(&actual,&other,sizeof(actual))==0;
    }
    if(ok){
        MusaDwgAc1032Unrendered2dInventory previous=actual;
        ok=!musa_dwg_ac1032_scene_index_unrendered_2d_inventory(
             scene,NULL)&&
           !musa_dwg_ac1032_scene_index_unrendered_2d_inventory(
             NULL,&actual)&&
           memcmp(&actual,&previous,sizeof(actual))==0;
    }
    if(ok)ok=musa_dwg_ac1032_scene_index_unrendered_2d_inventory(
          scene,&actual)&&
          !actual.complete_2d_display_accepted;
    fprintf(stderr,"%s %s indexed=%zu text=%zu mtext=%zu "
      "attrib=%zu attdef=%zu inserts=%zu minserts=%zu blocks=%zu endblocks=%zu "
      "other=%zu FULL_2D_DISPLAY=UNVERIFIED\n",
      ok?"PASS":"FAIL",path,actual.indexed_records,
      actual.text_records,actual.mtext_records,
      actual.attrib_records,actual.attdef_records,
      actual.insert_records,actual.minsert_records,
      actual.block_begin_records,actual.block_end_records,
      actual.other_indexed_records);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    MusaDwgAc1032Unrendered2dInventory sentinel={0};
    sentinel.indexed_records=7717u;
    if(musa_dwg_ac1032_scene_index_unrendered_2d_inventory(
        NULL,&sentinel)||sentinel.indexed_records!=7717u||
       musa_dwg_ac1032_scene_index_unrendered_2d_inventory(NULL,NULL))
        return 1;
    if(argc==1)return 0;
    if(argc!=7)return 2;
    for(int i=1;i<argc;i++)if(!inspect(argv[i]))return 1;
    return 0;
}
