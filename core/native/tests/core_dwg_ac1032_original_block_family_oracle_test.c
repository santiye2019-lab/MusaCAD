/* SPDX-License-Identifier: GPL-3.0-or-later */
/* New independent genuine R2018 example_2018.dwg vs example_2018.dxf
 * BLOCK/INSERT/ATTRIB source-index oracle. Only OT/H/CRC provenance:
 * NO INSERT placement, transform, ATTRIB content, block ownership,
 * visibility, recursion or v6 rendering is accepted by this test. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct ExpectedRaw {
    uint64_t handle;
    uint16_t type;
} ExpectedRaw;
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
    if(!ok){
        fprintf(stderr,"FAIL authentic example_2018.dwg physical Objects/H scene open\n");
        return 0;
    }
    /* Independent companion DXF: 0 BLOCK/ENDBLK/ATTDEF/INSERT/ATTRIB
     * with group 5 handles; raw DWG OT: 4/5/3/7/2 respectively.
     * This is not a parser for any block entity's numeric DATA. */
    const ExpectedRaw expected[]={
        {UINT64_C(0x20),4u}, {UINT64_C(0x21),5u},
        {UINT64_C(0x17e),4u}, {UINT64_C(0x17f),5u},
        {UINT64_C(0x18f),3u}, {UINT64_C(0x190),7u},
        {UINT64_C(0x192),2u}, {UINT64_C(0x1b9),4u},
        {UINT64_C(0x1ba),5u}, {UINT64_C(0x1c8),7u},
        {UINT64_C(0x756),7u}, {UINT64_C(0x757),2u}
    };
    for(size_t i=0u;ok&&i<sizeof(expected)/sizeof(expected[0]);i++){
        uint16_t actual=UINT16_MAX;
        ok=musa_dwg_ac1032_scene_index_object_type(
            scene,expected[i].handle,&actual)&&actual==expected[i].type;
    }
    MusaDwgAc1032Unrendered2dInventory census={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_unrendered_2d_inventory(
        scene,&census)&&census.original_types_crc_verified&&
        !census.complete_2d_display_accepted&&
        census.insert_records>=10u&&census.attrib_records>=2u&&
        census.attdef_records>=3u&&census.block_begin_records>=16u&&
        census.block_end_records>=16u&&
        census.indexed_records==musa_dwg_ac1032_scene_index_count(scene);
    if(ok){
        uint16_t unchanged=UINT16_C(0x55aa);
        ok=!musa_dwg_ac1032_scene_index_object_type(
             scene,UINT64_MAX,&unchanged)&&unchanged==UINT16_C(0x55aa);
    }
    fprintf(stderr,"%s original R2018 example_2018.dwg OT/H/CRC "
        "BLOCK=%zu ENDBLK=%zu INSERT=%zu ATTRIB=%zu ATTDEF=%zu; "
        "placement/owner/transform/rendering UNVERIFIED\n",
        ok?"PASS":"FAIL",census.block_begin_records,
        census.block_end_records,census.insert_records,
        census.attrib_records,census.attdef_records);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=2)return 2;
    return genuine(argv[1])?0:1;
}
