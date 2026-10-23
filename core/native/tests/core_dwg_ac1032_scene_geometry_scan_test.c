/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent AutoCAD 2018 source-derived full indexed geometry inventory:
 * every H retained, only verified native geometries counted as decoded. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_FILE (64u*1024u*1024u)
static int load(const char *path,uint8_t **p,size_t *n){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long len=ftell(f);
    if(len<256||len>(long)MAX_FILE||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *bytes=(uint8_t*)malloc((size_t)len);
    if(!bytes){fclose(f);return 0;}
    int good=fread(bytes,1u,(size_t)len,f)==(size_t)len;
    if(fclose(f))good=0;
    if(!good){free(bytes);return 0;}
    *p=bytes;*n=(size_t)len;return 1;
}
static int one(const char *path,uint64_t expected_handle,
               uint16_t expected_type,uint32_t expected_vertices){
    uint8_t *file=NULL;
    size_t n=0u;
    if(!load(path,&file,&n))return 0;
    MusaDwgAc1032SceneIndex *s=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(file,n,&s);
    free(file);file=NULL;
    if(!ok)return 0;
    MusaDwgAc1032SceneGeometryReport count={0},again={0};
    size_t entries=musa_dwg_ac1032_scene_index_count(s);
    ok=entries>0u&&entries<=1000000u&&
       musa_dwg_ac1032_scene_index_scan_geometry(s,NULL,0u,&count)&&
       count.indexed_records==entries&&
       count.all_indexed_raw_types_crc_verified&&
       count.decoded_geometry_objects>0u&&
       count.indexed_records==count.decoded_geometry_objects+
          count.rejected_supported_geometry+count.dependent_objects+
          count.opaque_objects;
    MusaDwgAc1032SceneGeometryRecord *rows=NULL,*saved=NULL;
    if(ok){
        rows=(MusaDwgAc1032SceneGeometryRecord*)calloc(
            entries,sizeof(*rows));
        saved=(MusaDwgAc1032SceneGeometryRecord*)calloc(
            entries,sizeof(*saved));
        if(!rows||!saved)ok=0;
    }
    if(ok)ok=musa_dwg_ac1032_scene_index_scan_geometry(
          s,rows,entries,&again)&&
          !memcmp(&count,&again,sizeof(count));
    int matched=0;
    for(size_t i=0u;ok&&i<entries;i++){
        if(i&&rows[i-1u].indexed_handle>=rows[i].indexed_handle)
            ok=0;
        if(!rows[i].original_record_crc_verified)ok=0;
        if(rows[i].indexed_handle==expected_handle){
            matched++;
            if(rows[i].raw_object_type!=expected_type||
               rows[i].category!=MUSA_AC1032_SCENE_GEOMETRY_DECODED||
               (expected_vertices==0u?rows[i].decoded_vertex_count!=0u:
                  rows[i].decoded_vertex_count<expected_vertices))ok=0;
        }
    }
    if(ok)ok=matched==1;
    if(ok){
        memcpy(saved,rows,entries*sizeof(*rows));
        const MusaDwgAc1032SceneGeometryReport saved_report=again;
        ok=!musa_dwg_ac1032_scene_index_scan_geometry(
              s,rows,entries-1u,&again)&&
           !memcmp(&saved_report,&again,sizeof(again))&&
           !memcmp(saved,rows,entries*sizeof(*rows));
        /* Disallow overlapping outputs before any bytes are modified. */
        if(ok)ok=!musa_dwg_ac1032_scene_index_scan_geometry(
              s,rows,entries,(MusaDwgAc1032SceneGeometryReport*)rows)&&
              !memcmp(saved,rows,entries*sizeof(*rows))&&
              !memcmp(&saved_report,&again,sizeof(again));
        /* Repeat successfully after rejecting both invalid requests. */
        if(ok)ok=musa_dwg_ac1032_scene_index_scan_geometry(
              s,rows,entries,&again)&&
              !memcmp(&saved_report,&again,sizeof(again))&&
              !memcmp(saved,rows,entries*sizeof(*rows));
    }
    fprintf(stderr,"%s indexed geometry audit %s total=%zu"
        " decoded=%zu rejected=%zu dependent=%zu opaque=%zu\n",
        ok?"PASS":"FAIL",path,count.indexed_records,
        count.decoded_geometry_objects,
        count.rejected_supported_geometry,count.dependent_objects,
        count.opaque_objects);
    free(saved);free(rows);
    musa_dwg_ac1032_scene_index_close(s);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1){
       MusaDwgAc1032SceneGeometryReport rep={0};
       rep.indexed_records=123u;
       if(musa_dwg_ac1032_scene_index_scan_geometry(
            NULL,NULL,0u,&rep)||rep.indexed_records!=123u)return 1;
       puts("PASS bounded scene geometry scan negative smoke");
       return 0;
    }
    if(argc!=7)return 2;
    return (one(argv[1],UINT64_C(0x1ca),19u,0u)&&
            one(argv[2],UINT64_C(0x1bd),17u,0u)&&
            one(argv[3],UINT64_C(0x1bd),18u,0u)&&
            one(argv[4],UINT64_C(0x1c0),27u,0u)&&
            one(argv[5],UINT64_C(0x1bd),77u,3u)&&
            one(argv[6],UINT64_C(0x1c1),16u,6u))?0:1;
}
