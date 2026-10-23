/* SPDX-License-Identifier: GPL-3.0-or-later */
/* No external DWG decoder. Original immutable AutoCAD R2018
 * PolyLine3D.dwg: 6 hard owned VERTEX3D records in H-stream order,
 * each reverse OT11 owner H points to the OT16 H=0x1C1 parent,
 * followed by OT6 SEQEND H=0x1C2. Independent 2018 .txt pins XY.
 */
#include "musacad_dwg_ac1032_polyline3d.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_FILE (64u*1024u*1024u)
static int load(const char *name,uint8_t **out,size_t *bytes){
    FILE *f=fopen(name,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    const long n=ftell(f);
    if(n<256||n>(long)MAX_FILE||fseek(f,0,SEEK_SET)!=0){
       fclose(f);return 0;
    }
    uint8_t *p=(uint8_t*)malloc((size_t)n);
    if(!p){fclose(f);return 0;}
    int ok=fread(p,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(p);return 0;}
    *out=p;*bytes=(size_t)n;return 1;
}
static int near(double x,double v){
   return isfinite(x)&&fabs(x-v)<=0.0001;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    uint8_t *file=NULL;size_t n=0u;
    if(!load(argv[1],&file,&n))return 2;
    MusaDwgAc1032Polyline3DOrderedVertex vertices[8]={0};
    MusaDwgAc1032Polyline3DOrdered result={0};
    const uint64_t h=UINT64_C(0x1c1);
    int ok=musa_dwg_ac1032_read_polyline3d_vertices_file(
        file,n,h,vertices,8u,&result);
    fprintf(stderr,"Source R2018 ordered 3D POLYLINE read=%d"
       " n=%u seqend=%llX layer=%llX hard_owner=%u"
       " reverse_owner=%u seqend_verified=%u\n",
       ok,result.vertex_count,
       (unsigned long long)result.seqend_handle,
       (unsigned long long)result.layer_handle,
       (unsigned)result.hard_owner_links_verified,
       (unsigned)result.reverse_vertex_owners_verified,
       (unsigned)result.seqend_type_verified);
    ok=ok&&result.vertex_count==6u&&
       result.header.indexed_handle==h&&
       result.layer_handle==UINT64_C(0x10)&&
       result.seqend_handle==UINT64_C(0x1c2)&&
       result.hard_owner_links_verified&&
       result.reverse_vertex_owners_verified&&
       result.seqend_type_verified&&
       result.header.owned_object_count==6u;
    for(unsigned i=0u;ok&&i<6u;i++){
        fprintf(stderr,"Original ordered VERTEX3D[%u]"
          " H=%llX XYZ=(%.9g,%.9g,%.9g)\n",
          i,(unsigned long long)vertices[i].indexed_handle,
          vertices[i].point[0],vertices[i].point[1],
          vertices[i].point[2]);
        ok=vertices[i].indexed_handle==(uint64_t)(0x1c3u+i);
    }
    if(ok){
        ok=near(vertices[0].point[0],21.4409)&&
           near(vertices[0].point[1],14.6498)&&
           near(vertices[0].point[2],0.0)&&
           near(vertices[1].point[0],20.8962)&&
           near(vertices[1].point[1],5.65378)&&
           near(vertices[1].point[2],0.0);
    }
    if(ok){
        const MusaDwgAc1032Polyline3DOrdered saved_result=result;
        MusaDwgAc1032Polyline3DOrderedVertex saved_vertices[8];
        memcpy(saved_vertices,vertices,sizeof(vertices));
        ok=!musa_dwg_ac1032_read_polyline3d_vertices_file(
             file,n,h,vertices,5u,&result)&&
           !memcmp(&saved_result,&result,sizeof(result))&&
           !memcmp(saved_vertices,vertices,sizeof(vertices));
        const uint8_t first=file[0],last=file[n-1u];
        file[0]^=1u;
        if(ok)ok=!musa_dwg_ac1032_read_polyline3d_vertices_file(
             file,n,h,vertices,8u,&result)&&
           !memcmp(&saved_result,&result,sizeof(result))&&
           !memcmp(saved_vertices,vertices,sizeof(vertices));
        file[0]=first;
        file[n-1u]^=1u;
        if(ok)ok=!musa_dwg_ac1032_read_polyline3d_vertices_file(
             file,n,h,vertices,8u,&result)&&
           !memcmp(&saved_result,&result,sizeof(result))&&
           !memcmp(saved_vertices,vertices,sizeof(vertices));
        file[n-1u]=last;
    }
    fprintf(stderr,"%s original R2018 ordered 3D POLYLINE"
       " full hard-owner/OT11 reverse owner/SEQEND+XYZ graph\n",
       ok?"PASS":"FAIL");
    free(file);return ok?0:1;
}
