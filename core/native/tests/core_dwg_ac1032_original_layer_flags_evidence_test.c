/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Immutable original AutoCAD 2018 LAYER H=0x10 numeric fields.
 * Verifies RAW provenance only: visibility, plotting, color override and
 * true BYLAYER display are NOT certified by this test. */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static int load(const char *name,uint8_t **out,size_t *len){
    FILE *f=fopen(name,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    const long n=ftell(f);
    if(n<256||n>64L*1024*1024||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *raw=(uint8_t*)malloc((size_t)n);
    if(!raw){fclose(f);return 0;}
    int ok=fread(raw,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(raw);return 0;}
    *out=raw;*len=(size_t)n;return 1;
}
static int check(const char *file){
    uint8_t *raw=NULL,*backup=NULL,*payload=NULL;size_t n=0;
    if(!load(file,&raw,&n))return 0;
    backup=(uint8_t*)malloc(n);
    if(!backup){free(raw);return 0;}
    memcpy(backup,raw,n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(raw,n,&scene);
    MusaDwgAc1032LayerFlagsEvidence flags={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_read_layer_flags_evidence(
         scene,UINT64_C(0x10),&flags)&&
         flags.indexed_handle==UINT64_C(0x10)&&
         flags.source_crc_verified&&
         flags.numeric_flags_start_bit==31u&&
         flags.numeric_flags_end_bit==45u&&
         flags.entry_64_flag==0u&&
         flags.xref_index_plus_one==0u&&
         flags.xref_dependent==0u&&
         flags.raw_values_70_290_370==224u;
    if(ok){
        const MusaDwgAc1032LayerFlagsEvidence old=flags;
        ok=!musa_dwg_ac1032_scene_index_read_layer_flags_evidence(
             scene,UINT64_C(0x1ca),&flags)&&
           !memcmp(&flags,&old,sizeof(flags))&&
           !musa_dwg_ac1032_scene_index_read_layer_flags_evidence(
             scene,UINT64_MAX,&flags)&&
           !memcmp(&flags,&old,sizeof(flags));
    }
    size_t needed=0u,read=0u;
    MusaDwgAc1032PayloadInfo meta={0},again={0};
    MusaDwgAc1032ObjectStreams layout={0};
    if(ok)ok=musa_dwg_ac1032_object_payload_file(
         raw,n,UINT64_C(0x10),NULL,0u,&needed,&meta)&&
         needed>0u&&needed<65536u&&
         musa_dwg_ac1032_stream_layout_file(
         raw,n,UINT64_C(0x10),&layout);
    if(ok){
        payload=(uint8_t*)malloc(needed);
        ok=payload&&musa_dwg_ac1032_object_payload_file(
            raw,n,UINT64_C(0x10),payload,needed,&read,&again)&&
            read==needed;
    }
    if(ok){
        MusaDwgAc1032LayerFlagsEvidence found={0};
        ok=musa_dwg_ac1032_layer_ot51_flags_evidence_at(
             payload,needed,&layout,&found)&&
           !memcmp(&found,&flags,sizeof(found));
        if(ok){
            const MusaDwgAc1032LayerFlagsEvidence unchanged=found;
            MusaDwgAc1032ObjectStreams invalid=layout;
            invalid.raw_object_type=19u;
            ok=!musa_dwg_ac1032_layer_ot51_flags_evidence_at(
                 payload,needed,&invalid,&found)&&
                !memcmp(&found,&unchanged,sizeof(found))&&
               !musa_dwg_ac1032_layer_ot51_flags_evidence_at(
                 payload,needed-1u,&layout,&found)&&
                !memcmp(&found,&unchanged,sizeof(found))&&
               !musa_dwg_ac1032_layer_ot51_flags_evidence_at(
                 payload,needed,&layout,NULL);
        }
    }
    ok=ok&&!memcmp(raw,backup,n);
    fprintf(stderr,"%s ORIGINAL DWG LAYER H0x10 source raw "
       "flags=0x%X index+1=%u status-bits [%zu,%zu) "
       "NOT resolved color or visibility, file=%s\n",
       ok?"PASS":"FAIL",(unsigned)flags.raw_values_70_290_370,
       (unsigned)flags.xref_index_plus_one,
       flags.numeric_flags_start_bit,flags.numeric_flags_end_bit,file);
    free(payload);free(raw);free(backup);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1){
        MusaDwgAc1032LayerFlagsEvidence untouched={0};
        untouched.raw_values_70_290_370=321u;
        return !musa_dwg_ac1032_scene_index_read_layer_flags_evidence(
                 NULL,UINT64_C(0x10),&untouched)&&
               untouched.raw_values_70_290_370==321u?0:1;
    }
    if(argc!=3)return 2;
    const int line=check(argv[1]);
    const int point=check(argv[2]);
    return line&&point?0:1;
}
