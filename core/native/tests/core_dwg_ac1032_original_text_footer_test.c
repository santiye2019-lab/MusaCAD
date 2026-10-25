/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent immutable original AC1032 physical-footer regression.
 * LibreDWG supplies only pinned AutoCAD-produced static DWG fixture bytes;
 * no LibreDWG code is compiled, linked or executed by this C11 test.
 * 2018/Text.txt describes AcDbMText H=1C0, NOT an OT1 TEXT oracle.
 */
#include "musacad_dwg_ac1032_page_map.h"
#include "musacad_dwg_ac1032_sections.h"
#include "musacad_dwg_ac1032_handles.h"
#include "musacad_dwg_ac1032_scene_index.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int load(const char *path,uint8_t **out,size_t *size){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    const long n=ftell(f);
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)!=0){
        fclose(f);return 0;
    }
    uint8_t *b=(uint8_t*)malloc((size_t)n);
    if(!b){fclose(f);return 0;}
    int ok=fread(b,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(b);return 0;}
    *out=b;*size=(size_t)n;return 1;
}
static int original(const char *path,size_t expected_trailer,
                    int require_mtext){
    uint8_t *file=NULL,*snapshot=NULL;
    size_t bytes=0u;
    if(!load(path,&file,&bytes))return 0;
    snapshot=(uint8_t*)malloc(bytes);
    if(!snapshot){free(file);return 0;}
    memcpy(snapshot,file,bytes);
    MusaDwgAc1032FileMapHeader header={0};
    size_t pages=0u,catalog=0u,handles=0u;
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=!memcmp(file,"AC1032",6u)&&
        musa_dwg_ac1032_page_map_header(file,bytes,&header)&&
        musa_dwg_ac1032_page_locations(file,bytes,NULL,0u,&pages)&&
        pages==header.page_count&&pages>0u&&
        musa_dwg_ac1032_section_catalog(file,bytes,NULL,0u,&catalog)&&
        catalog>0u&&
        musa_dwg_ac1032_handles_index(file,bytes,NULL,0u,&handles)&&
        handles>0u&&
        musa_dwg_ac1032_scene_index_open(file,bytes,&scene)&&
        scene!=NULL&&
        musa_dwg_ac1032_scene_index_count(scene)==handles;
    if(ok&&require_mtext){
        uint16_t type=0u;
        /* Original 2018/Text.txt: AcDbMText, Handle 1C0.
         * Numeric R2018 OT44 must NOT be passed to OT1 TEXT decoder. */
        ok=musa_dwg_ac1032_scene_index_object_type(
               scene,UINT64_C(0x1c0),&type)&&type==44u;
    }
    if(ok){
        /* This independent physical profile is for the final MAP page:
         * exact 108-byte duplicate header with a 20-byte empty SYSTEM
         * marker immediately after the compressed page-map payload. */
        const uint64_t map_end=header.page_map_offset+20u+
                                header.map_compressed_bytes;
        const size_t physical_end=bytes-expected_trailer;
        static const uint8_t empty_system[20]={
            0x3bu,0x0eu,0x63u,0x41u,0u,0u,0u,0u,
            0u,0u,0u,0u,2u,0u,0u,0u,0u,0u,0u,0u
        };
        if(expected_trailer==108u){
            ok=map_end+20u==physical_end&&
               !memcmp(file+(size_t)map_end,empty_system,20u)&&
               !memcmp(file+physical_end,file+0x80u,108u);
            if(ok){
                /* Corrupt the duplicated footer: fail closed, do not
                 * modify count or caller-owned outputs on rejection. */
                size_t sentinel=UINT32_C(0x5a5a5a5a);
                file[physical_end]^=1u;
                ok=!musa_dwg_ac1032_page_locations(
                         file,bytes,NULL,0u,&sentinel)&&
                   sentinel==UINT32_C(0x5a5a5a5a)&&
                   !musa_dwg_ac1032_section_catalog(
                         file,bytes,NULL,0u,&sentinel)&&
                   sentinel==UINT32_C(0x5a5a5a5a);
                file[physical_end]^=1u;
            }
        }
    }
    if(memcmp(file,snapshot,bytes)!=0)ok=0;
    fprintf(stderr,"%s original AC1032 %s: bytes=%zu pages=%zu sections=%zu handles=%zu trailer=%zu MTEXT_H1C0=%s\n",
        ok?"PASS":"FAIL",path,bytes,pages,catalog,handles,
        expected_trailer,require_mtext?"OT44":"not asserted");
    musa_dwg_ac1032_scene_index_close(scene);
    free(snapshot);free(file);
    return ok;
}
int main(int argc,char **argv){
    if(argc!=4){
        fprintf(stderr,"usage: test Text.dwg Line.dwg Ellipse.dwg\n");
        return 2;
    }
    return original(argv[1],108u,1)&&
           original(argv[2],110u,0)&&
           original(argv[3],107u,0)?0:1;
}
