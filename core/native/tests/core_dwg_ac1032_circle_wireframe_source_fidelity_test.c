/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent companion example_2018.dxf: H1C8 INSERT -> H1BB CIRCLE,
 * center (8482.030567364651,-1381.341268226861), R130.0490046078758.
 * This is geometry-only, NEVER source color, owner or layer visibility. */
#include "musacad_dwg_ac1032_circle_wireframe_source_fidelity.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int original(const char *path){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    int ok=fseek(f,0,SEEK_END)==0;
    long n=ok?ftell(f):-1L;
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)!=0)ok=0;
    uint8_t *bytes=ok?(uint8_t*)malloc((size_t)n):NULL;
    if(!bytes)ok=0;
    if(ok)ok=fread(bytes,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=!memcmp(bytes,"AC1032",6u)&&
             musa_dwg_ac1032_scene_index_open(bytes,(size_t)n,&scene);
    free(bytes);
    if(!ok){musa_dwg_ac1032_scene_index_close(scene);return 0;}
    MusaDwgAc1032CircleWireframeEvidence evidence={0};
    ok=musa_dwg_ac1032_circle_wireframe_v6_diagnostic(
           scene,NULL,0u,&evidence)&&
       evidence.emitted_numeric_wireframes>0u&&
       evidence.unresolved_h4_h5_circles<evidence.emitted_numeric_wireframes;
    float *packet=ok?(float*)malloc(evidence.required_float_words*sizeof(float)):NULL;
    if(!packet)ok=0;
    if(ok)ok=musa_dwg_ac1032_circle_wireframe_v6_diagnostic(
            scene,packet,evidence.required_float_words,&evidence);
    MusaDwgAc1032CircleWireframeFidelity fidelity={0};
    if(ok)ok=musa_dwg_ac1032_circle_wireframe_source_fidelity(
            scene,packet,evidence.required_float_words,&fidelity)&&
          fidelity.geometry_only_verified&&
          !fidelity.source_color_or_visibility_accepted&&
          fidelity.verified_wireframes==evidence.emitted_numeric_wireframes&&
          fidelity.unresolved_h4_h5_circles==
              evidence.unresolved_h4_h5_circles;
    /* A fixed independent DXF oracle, not a roundtrip of our own packet:
     * H1C8 is the sole CIRCLE in the 13-child CIRKLO_PUNKTOJ block. */
    MusaDwgAc1032InsertCircleSourceAxes h1c8={0};
    if(ok)ok=musa_dwg_ac1032_insert_circle_source_axes(
             scene,UINT64_C(0x1c8),&h1c8)&&
           h1c8.block_header_handle==UINT64_C(0x1b8)&&
           h1c8.circle_candidate_count==1u&&
           h1c8.verified_circle_count==1u&&
           h1c8.h4_h5_unresolved_count==0u&&
           h1c8.h4_h5_source_verified_count==1u&&
           h1c8.rows[0].verified_block_owner_handle==UINT64_C(0x1b8)&&
           h1c8.rows[0].source_layer_handle==UINT64_C(0x8a)&&
           h1c8.rows[0].child_handle==UINT64_C(0x1bb)&&
           fabs(h1c8.rows[0].world_center[0]-8482.030567364651)<1e-6&&
           fabs(h1c8.rows[0].world_center[1]+1381.341268226861)<1e-6&&
           fabs(h1c8.rows[0].source_radius-130.0490046078758)<1e-6;
    /* Real binary source must yield an ACI6 diagnostic sampled vertex
     * at (cx+radius,cy); it is NOT the original DWG color. */
    if(ok){
        int found=0;
        for(size_t at=7u;at+132u<=evidence.required_float_words;at+=132u){
            if(fabs((double)packet[at+2u]-
                    (8482.030567364651+130.0490046078758))<0.002&&
               fabs((double)packet[at+3u]+1381.341268226861)<0.002)
                found=1;
        }
        ok=found;
    }
    /* Reject packet tampering and preserve the caller's previous evidence
     * on failure. Neither packet nor original DWG may be modified. */
    if(ok){
        const MusaDwgAc1032CircleWireframeFidelity saved=fidelity;
        const float first=packet[7],vertex=packet[9],bounds=packet[2];
        packet[7]=519.0f;
        ok=!musa_dwg_ac1032_circle_wireframe_source_fidelity(
               scene,packet,evidence.required_float_words,&fidelity)&&
           !memcmp(&saved,&fidelity,sizeof(saved));
        packet[7]=first;
        packet[9]=vertex+1.0f;
        if(ok)ok=!musa_dwg_ac1032_circle_wireframe_source_fidelity(
                    scene,packet,evidence.required_float_words,&fidelity)&&
                  !memcmp(&saved,&fidelity,sizeof(saved));
        packet[9]=vertex;
        packet[2]=bounds+1000.0f;
        if(ok)ok=!musa_dwg_ac1032_circle_wireframe_source_fidelity(
                    scene,packet,evidence.required_float_words,&fidelity)&&
                  !memcmp(&saved,&fidelity,sizeof(saved));
        packet[2]=bounds;
        if(ok)ok=!musa_dwg_ac1032_circle_wireframe_source_fidelity(
                    scene,packet,evidence.required_float_words-1u,&fidelity)&&
                  !memcmp(&saved,&fidelity,sizeof(saved));
        if(ok)ok=musa_dwg_ac1032_circle_wireframe_source_fidelity(
                    scene,packet,evidence.required_float_words,&fidelity);
    }
    fprintf(stderr,"%s genuine R2018 H1C8/H1BB independent DXF WCS "
            "CIRCLE packet fidelity; H4/H5 unknown, paint NOT accepted\n",
            ok?"PASS":"FAIL");
    free(packet);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    return original(argv[1])?0:1;
}
