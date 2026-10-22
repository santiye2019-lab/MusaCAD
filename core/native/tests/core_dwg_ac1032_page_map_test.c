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
    /* New real-file gap behavior: one negative-ID physical gap node
     * consumes 24 map bytes and real map/section page IDs are sparse.
     * Unlike older synthetic fixture, there is an actual unused page. */
    uint8_t sparse[448]={0};
    memcpy(sparse,"AC1032",6u);
    uint8_t inner[0x6c]={0};
    memcpy(inner,"AcFssFcAJMB",11u);
    le32(inner+0x10u,0x6cu);
    le32(inner+0x40u,2u); /* two actual stored pages */
    le32(inner+0x50u,3u); /* map ID > page count */
    le32(inner+0x5cu,1u);
    le32(inner+0x54u,128u); /* map physical offset 384 */
    uint32_t state=1u;
    for(size_t i=0u;i<sizeof(inner);i++){
        state=state*UINT32_C(0x343fd)+UINT32_C(0x269ec3);
        sparse[0x80u+i]=(uint8_t)(inner[i]^(uint8_t)(state>>16u));
    }
    le32(sparse+384u,UINT32_C(0x41630e3b));
    le32(sparse+388u,40u); /* 8+24+8 */
    le32(sparse+392u,43u); /* 0x00 0x16 + 40 literal + 0x11 */
    le32(sparse+396u,2u);
    uint8_t *lz=sparse+404u;
    lz[0]=0u;lz[1]=22u;
    le32(lz+2u,1u);le32(lz+6u,64u);
    le32(lz+10u,UINT32_C(0xfffffffe)); /* signed gap -2 */
    le32(lz+14u,64u);
    /* lz[18..33] is the all-zero gap-tree extension. */
    le32(lz+34u,3u);le32(lz+38u,64u);
    lz[42u]=0x11u;
    le32(sparse+400u,checksum(checksum(0u,sparse+384u,20u),
                                lz,43u));
    MusaDwgAc1032FileMapHeader g={0};
    assert(musa_dwg_ac1032_page_map_header(sparse,sizeof(sparse),&g));
    assert(g.page_count==2u&&g.page_map_id==3u&&
           g.map_decoded_bytes==40u);
    MusaDwgAc1032PageLocation gp[2]={{0}};
    size_t ng=0u;
    assert(musa_dwg_ac1032_page_locations(
                   sparse,sizeof(sparse),gp,2u,&ng));
    assert(ng==2u&&gp[0].page_id==1u&&
           gp[0].file_offset==256u&&gp[0].file_span_bytes==64u&&
           gp[1].page_id==3u&&gp[1].file_offset==384u&&
           gp[1].file_span_bytes==64u);
    MusaDwgAc1032PageLocation saved_g[2];
    memcpy(saved_g,gp,sizeof(saved_g));
    const size_t old_ng=ng;
    lz[10u]=1u; /* corrupt signed gap node without updating CRC */
    assert(!musa_dwg_ac1032_page_locations(
                 sparse,sizeof(sparse),gp,2u,&ng));
    assert(ng==old_ng&&!memcmp(gp,saved_g,sizeof(saved_g)));
    return 0;
}
