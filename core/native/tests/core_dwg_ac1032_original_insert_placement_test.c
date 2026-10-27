/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original unchanged R2018 example_2018.dwg OT7 H1C8 INSERT numeric DATA
 * against independent companion DXF AcDbBlockReference H1C8 group 10/20.
 * No BLOCK_HEADER H graph, owner, recursion or final display claim. */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_insert_original_planar_affine.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int near(double a,double b){return isfinite(a)&&fabs(a-b)<0.00002;}
static int genuine(const char *file){
    FILE *f=fopen(file,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *src=(uint8_t*)malloc((size_t)n);
    uint8_t *saved=(uint8_t*)malloc((size_t)n);
    if(!src||!saved){free(src);free(saved);fclose(f);return 0;}
    int ok=fread(src,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f))ok=0;
    if(ok)memcpy(saved,src,(size_t)n);
    ok=ok&&!memcmp(src,"AC1032",6u);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
        src,(size_t)n,&scene)&&!memcmp(src,saved,(size_t)n);
    free(src);free(saved);
    if(!ok)return 0;
    /* Two additional unchanged R2018 OT7 sources with an ATTRIB count.
     * Only source DATA is accepted, not the child H/owner relationship. */
    int probe_ok=1;
    const uint64_t diagnostic_handles[]={UINT64_C(0x190),UINT64_C(0x756)};
    for(size_t i=0u;i<sizeof(diagnostic_handles)/sizeof(diagnostic_handles[0]);i++){
        MusaDwgAc1032InsertPlacement probe={0};
        const int parsed=musa_dwg_ac1032_scene_index_read_insert_placement(
            scene,diagnostic_handles[i],&probe);
        probe_ok=probe_ok&&parsed&&probe.original_record_crc_verified&&
            probe.numeric_fields_decoded&&!probe.block_graph_and_display_accepted&&
            probe.indexed_handle==diagnostic_handles[i]&&
            probe.has_attribs==1u&&probe.owned_attrib_count==1u&&
            near(probe.extrusion[0],0.0)&&near(probe.extrusion[1],0.0)&&
            near(probe.extrusion[2],1.0);
        fprintf(stderr,"MC1032 INSERT original H%llx accepted=%d attrib=%u count=%u\n",
            (unsigned long long)diagnostic_handles[i],parsed,
            (unsigned)probe.has_attribs,(unsigned)probe.owned_attrib_count);
    }
    const uint64_t h=UINT64_C(0x1c8);
    MusaDwgAc1032InsertPlacement p={0};
    ok=probe_ok&&
       musa_dwg_ac1032_scene_index_read_insert_placement(scene,h,&p)&&
       p.indexed_handle==h&&p.original_record_crc_verified&&
       p.numeric_fields_decoded&&!p.block_graph_and_display_accepted&&
       near(p.insertion[0],8482.030567364651)&&
       near(p.insertion[1],-1381.341268226861)&&
       near(p.insertion[2],0.0)&&
       near(p.scale[0],1.0)&&near(p.scale[1],1.0)&&
       near(p.scale[2],1.0)&&near(p.rotation,0.0)&&
       near(p.extrusion[0],0.0)&&near(p.extrusion[1],0.0)&&
       near(p.extrusion[2],1.0)&&!p.has_attribs&&
       p.owned_attrib_count==0u;
    if(ok){
        /* Independent companion example_2018.dxf: INSERT H1C8 at
         * (8482.030567364651,-1381.341268226861), scale 1, angle 0;
         * referenced BLOCK origin is (0,0). Read all source fields from
         * unchanged AC1032 bytes, never derive expected values from them. */
        double affine[6]={NAN,NAN,NAN,NAN,NAN,NAN};
        ok=musa_dwg_ac1032_insert_original_planar_affine(
               scene,h,affine)&&
           near(affine[0],1.0)&&near(affine[1],0.0)&&
           near(affine[2],8482.030567364651)&&
           near(affine[3],0.0)&&near(affine[4],1.0)&&
           near(affine[5],-1381.341268226861);
        if(ok){
            const double unchanged[6]={affine[0],affine[1],affine[2],
                                       affine[3],affine[4],affine[5]};
            ok=!musa_dwg_ac1032_insert_original_planar_affine(
                    scene,UINT64_MAX,affine)&&
               !memcmp(affine,unchanged,sizeof(affine))&&
               !musa_dwg_ac1032_insert_original_planar_affine(
                    scene,UINT64_C(0x192),affine)&&
               !memcmp(affine,unchanged,sizeof(affine));
        }
        fprintf(stderr,"%s genuine R2018 INSERT H1C8 original source "
                "affine vs independent DXF; block paint NOT accepted\\n",
                ok?"PASS":"FAIL");
    }
    if(ok){
        MusaDwgAc1032InsertPlacement unchanged;
        memset(&unchanged,0x5a,sizeof(unchanged));
        const MusaDwgAc1032InsertPlacement saved=unchanged;
        ok=!musa_dwg_ac1032_scene_index_read_insert_placement(
            scene,UINT64_MAX,&unchanged)&&
           !memcmp(&unchanged,&saved,sizeof(unchanged))&&
           !musa_dwg_ac1032_scene_index_read_insert_placement(
            scene,UINT64_C(0x192),&unchanged)&&
           !memcmp(&unchanged,&saved,sizeof(unchanged));
    }
    fprintf(stderr,"%s original R2018 OT7 H1C8 OCS insertion=(%.10f,%.10f,%.10f)"
        " flags=%u scale=(%.3f,%.3f,%.3f) rotation=%.6f; "
        "BLOCK/ATTRIB H graph and paint UNVERIFIED\n",
        ok?"PASS":"FAIL",p.insertion[0],p.insertion[1],p.insertion[2],
        (unsigned)p.scale_data_flags,p.scale[0],p.scale[1],p.scale[2],
        p.rotation);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=2)return 2;
    return genuine(argv[1])?0:1;
}
