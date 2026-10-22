/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Structural fixture exercising independent published R2018 page-map
 * organization, NOT a genuine AutoCAD R2018 drawing/objects oracle.
 * The physical layout of synthetic R2004 map is shared with R2018;
 * the program MUST NOT accept it as R2004 geometry. */
#define main ac1018_existing_page_map_tests
#include "core_dwg_ac1018_page_map_test.c"
#undef main
#include "musacad_dwg_ac1032_page_map.h"
int main(void){
    uint8_t file[380];fixture(file);
    MusaDwgAc1032FileMapHeader h={0};
    assert(!musa_dwg_ac1032_page_map_header(file,sizeof(file),&h));
    memcpy(file,"AC1032",6u);
    assert(musa_dwg_ac1032_page_map_header(file,sizeof(file),&h));
    assert(h.page_count==2u&&h.page_map_id==2u&&
           h.section_map_id==1u&&h.page_map_offset==320u&&
           h.map_decoded_bytes==16u&&h.map_compressed_bytes==18u);
    assert(!musa_dwg_ac1018_page_map_header(
                         file,sizeof(file),&h));
    size_t count=777u;
    MusaDwgAc1032PageLocation loc[2]={{0}};
    assert(musa_dwg_ac1032_page_locations(
             file,sizeof(file),loc,2u,&count));
    assert(count==2u&&loc[0].page_id==1u&&
           loc[0].file_offset==256u&&loc[0].file_span_bytes==64u&&
           loc[1].page_id==2u&&loc[1].file_offset==320u&&
           loc[1].file_span_bytes==64u);
    assert(!musa_dwg_ac1018_page_locations(
                      file,sizeof(file),loc,2u,&count));
    const MusaDwgAc1032FileMapHeader good=h;
    const size_t count_before=count;
    MusaDwgAc1032PageLocation before[2];
    memcpy(before,loc,sizeof(before));
    file[320u+20u+9u]^=0x01u; /* tamper data: checksum fails */
    assert(!musa_dwg_ac1032_page_map_header(file,sizeof(file),&h));
    assert(!musa_dwg_ac1032_page_locations(
                    file,sizeof(file),loc,2u,&count));
    assert(count==count_before&&
           !memcmp(loc,before,sizeof(loc))&&
           !memcmp(&good,&h,sizeof(h)));
    fixture(file);memcpy(file,"AC1032",6u);
    file[0x80u]^=1u; /* decrypted header sentinel */
    assert(!musa_dwg_ac1032_page_map_header(file,sizeof(file),&h));
    assert(!memcmp(&good,&h,sizeof(h)));
    fixture(file);memcpy(file,"AC1032",6u);
    assert(!musa_dwg_ac1032_page_locations(
                   file,337u,loc,2u,&count));
    assert(!musa_dwg_ac1032_page_locations(
                   file,sizeof(file),loc,1u,&count));
    assert(!musa_dwg_ac1032_page_map_header(
                   file,sizeof(file),NULL));
    assert(!musa_dwg_ac1032_page_locations(
                   file,sizeof(file),NULL,1u,&count));
    assert(!musa_dwg_ac1032_page_locations(
                   file,sizeof(file),loc,2u,NULL));
    assert(count==count_before&&
           !memcmp(loc,before,sizeof(loc))&&
           !memcmp(&good,&h,sizeof(h)));
    return 0;
}
