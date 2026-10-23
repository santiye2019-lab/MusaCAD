/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original R2018 AutoCAD 3D POLYLINE; independent full-source graph oracle
 * versus native cached-index owner traversal AFTER source bytes are freed. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_DWG (64u*1024u*1024u)
static int load(const char *path,uint8_t **dst,size_t *length){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256||n>(long)MAX_DWG||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *p=(uint8_t*)malloc((size_t)n);
    if(!p){fclose(f);return 0;}
    int ok=fread(p,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f))ok=0;
    if(!ok){free(p);return 0;}
    *dst=p;*length=(size_t)n;return 1;
}
static int eq(double x,double y){
    return isfinite(x)&&isfinite(y)&&fabs(x-y)<1e-7;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    uint8_t *original=NULL,*source_copy=NULL;
    size_t bytes=0u;
    if(!load(argv[1],&original,&bytes))return 2;
    source_copy=(uint8_t*)malloc(bytes);
    if(!source_copy){free(original);return 2;}
    memcpy(source_copy,original,bytes);
    MusaDwgAc1032Polyline3DOrdered full={0},cached={0};
    MusaDwgAc1032Polyline3DOrderedVertex expected[8]={0},actual[8]={0};
    const uint64_t h=UINT64_C(0x1c1);
    int ok=musa_dwg_ac1032_read_polyline3d_vertices_file(
        original,bytes,h,expected,8u,&full)&&
        full.vertex_count==6u&&
        full.seqend_handle==UINT64_C(0x1c2)&&
        full.layer_handle==UINT64_C(0x10);
    MusaDwgAc1032SceneIndex *session=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
                    original,bytes,&session)&&session!=NULL;
    if(ok)ok=!memcmp(original,source_copy,bytes);
    free(original);original=NULL;
    free(source_copy);source_copy=NULL;
    /* Distinct owned buffers prove no source-file lifetime dependency. */
    if(ok)ok=musa_dwg_ac1032_scene_index_read_polyline3d_vertices(
            session,h,actual,8u,&cached);
    if(ok){
        ok=cached.vertex_count==6u&&
           cached.header.owned_object_count==6u&&
           cached.header.indexed_handle==h&&
           cached.layer_handle==full.layer_handle&&
           cached.seqend_handle==full.seqend_handle&&
           cached.hard_owner_links_verified&&
           cached.reverse_vertex_owners_verified&&
           cached.seqend_type_verified;
    }
    for(unsigned i=0u;ok&&i<6u;i++){
        ok=actual[i].indexed_handle==expected[i].indexed_handle&&
           actual[i].indexed_handle==UINT64_C(0x1c3)+i&&
           actual[i].flags==expected[i].flags;
        for(unsigned j=0u;j<3u&&ok;j++)
            ok=eq(actual[i].point[j],expected[i].point[j]);
    }
    if(ok)ok=eq(actual[0].point[0],21.4409)&&
             eq(actual[0].point[1],14.6498)&&
             eq(actual[1].point[0],20.8962)&&
             eq(actual[1].point[1],5.65378);
    if(ok){
        const MusaDwgAc1032Polyline3DOrdered saved=cached;
        MusaDwgAc1032Polyline3DOrderedVertex saved_verts[8];
        memcpy(saved_verts,actual,sizeof(actual));
        ok=!musa_dwg_ac1032_scene_index_read_polyline3d_vertices(
             session,h,actual,5u,&cached)&&
           !memcmp(&cached,&saved,sizeof(saved))&&
           !memcmp(actual,saved_verts,sizeof(actual));
        if(ok)ok=!musa_dwg_ac1032_scene_index_read_polyline3d_vertices(
             session,UINT64_MAX,actual,8u,&cached)&&
           !memcmp(&cached,&saved,sizeof(saved))&&
           !memcmp(actual,saved_verts,sizeof(actual));
        /* No reuse of original source and two independent ordered reads. */
        if(ok)ok=musa_dwg_ac1032_scene_index_read_polyline3d_vertices(
             session,h,actual,8u,&cached)&&
           cached.vertex_count==saved.vertex_count&&
           cached.seqend_handle==saved.seqend_handle&&
           !memcmp(actual,saved_verts,sizeof(actual));
    }
    fprintf(stderr,"%s source-free cached R2018 3D POLYLINE:"
            " 6 exact ordered OT11 with verified owner/layer/SEQEND\n",
            ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(session);
    return ok?0:1;
}
