/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_page_map.h"
#include "musacad_dwg_ac1024_section_envelope.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
/* Purely synthetic two-page AC1018 file. Never copies a private DWG. */
static void le32(uint8_t *p,uint32_t v){
    for(unsigned i=0u;i<4u;++i)p[i]=(uint8_t)(v>>(8u*i));
}
static uint32_t checksum(uint32_t seed,const uint8_t *p,size_t n){
    uint32_t lo=seed&65535u,hi=seed>>16u;
    for(size_t i=0u;i<n;++i){
        lo=(lo+p[i])%65521u;hi=(hi+lo)%65521u;
    }
    return (hi<<16u)|lo;
}
static void fixture(uint8_t file[380]){
    memset(file,0,380u);
    memcpy(file,"AC1018",6u);
    uint8_t hdr[0x6c]={0};
    memcpy(hdr,"AcFssFcAJMB",11u);
    le32(hdr+0x10u,0x6cu);
    le32(hdr+0x40u,2u);
    le32(hdr+0x50u,2u);
    le32(hdr+0x5cu,1u);
    le32(hdr+0x54u,64u);
    uint32_t state=1u;
    for(size_t i=0u;i<sizeof(hdr);++i){
        state=state*UINT32_C(0x343fd)+UINT32_C(0x269ec3);
        file[0x80u+i]=(uint8_t)(hdr[i]^(uint8_t)(state>>16u));
    }
    const size_t pos=320u;
    le32(file+pos,UINT32_C(0x41630e3b));
    le32(file+pos+4u,16u);
    le32(file+pos+8u,18u);
    le32(file+pos+12u,2u);
    uint8_t *packed=file+pos+20u;
    packed[0]=13u; /* R2004 literal 16 bytes */
    le32(packed+1u,1u);le32(packed+5u,64u);
    le32(packed+9u,2u);le32(packed+13u,64u);
    packed[17u]=0x11u;
    le32(file+pos+16u,checksum(
        checksum(0u,file+pos,20u),packed,18u));
}

/* Synthetic R2010 physical-map shared-structure proof. Real AC1024 page
 * auth and the logical DWG section still require independent evidence. */
static void ac1024_page_map_test(void){
    uint8_t file[380];
    fixture(file);
    memcpy(file,"AC1024",6u);
    uint8_t source_before[380];memcpy(source_before,file,sizeof(file));
    MusaDwgAc1018FileMapHeader header={0};
    size_t count=111u;
    assert(musa_dwg_ac1024_page_map_header(file,sizeof(file),&header));
    assert(header.page_count==2u&&header.page_map_id==2u&&
           header.section_map_id==1u);
    assert(musa_dwg_ac1024_page_locations(
        file,sizeof(file),NULL,0u,&count)&&count==2u);
    MusaDwgAc1018PageLocation rows[2]={{0}};
    assert(musa_dwg_ac1024_page_locations(
        file,sizeof(file),rows,2u,&count));
    assert(rows[0].file_offset==256u&&rows[0].file_span_bytes==64u&&
           rows[1].file_offset==320u&&rows[1].file_span_bytes==64u);
    assert(memcmp(source_before,file,sizeof(file))==0);
    /* A R2010 source is NEVER silently interpreted as R2004. */
    assert(!musa_dwg_ac1018_page_map_header(
        file,sizeof(file),&header));
    assert(!musa_dwg_ac1018_page_locations(
        file,sizeof(file),NULL,0u,&count));
    MusaDwgAc1018PageLocation saved[2];memcpy(saved,rows,sizeof rows);
    const size_t saved_count=count;
    assert(!musa_dwg_ac1024_page_locations(
        file,sizeof(file),rows,1u,&count));
    assert(count==saved_count&&memcmp(rows,saved,sizeof rows)==0);
    file[320u+20u+9u]^=1u; /* corrupt original map checksum */
    assert(!musa_dwg_ac1024_page_locations(
        file,sizeof(file),rows,2u,&count));
    assert(count==saved_count&&memcmp(rows,saved,sizeof rows)==0);
    memcpy(file,source_before,sizeof(file));
    file[0x80u]^=1u; /* corrupt original encrypted map header */
    assert(!musa_dwg_ac1024_page_map_header(file,sizeof(file),&header));
    memcpy(file,source_before,sizeof(file));
    file[0u]='X';
    assert(!musa_dwg_ac1024_page_map_header(file,sizeof(file),&header));
    assert(!musa_dwg_ac1024_page_locations(
        file,sizeof(file),rows,2u,&count));
    assert(count==saved_count&&memcmp(rows,saved,sizeof rows)==0);
}
/* Synthetic R2010 exact 44-byte suffix: all values constructed here. */
static void footer44(uint8_t file[486]){
    memset(file,0,486u);
    fixture(file);
    memcpy(file,"AC1024",6u);
    le32(file+340u+13u,122u);
    uint8_t hdr[20];memcpy(hdr,file+320u,sizeof hdr);
    memset(hdr+16u,0,4u);
    le32(file+336u,checksum(checksum(0u,hdr,20u),
                             file+340u,18u));
    static const uint8_t marker[20]={
        0x3bu,0x0eu,0x63u,0x41u,0u,0u,0u,0u,
        0u,0u,0u,0u,2u,0u,0u,0u,0u,0u,0u,0u
    };
    memcpy(file+358u,marker,20u);
    memcpy(file+378u,file+0x80u,108u);
}
static void test_footer44(void){
    uint8_t file[486];footer44(file);
    uint8_t before[486];memcpy(before,file,sizeof file);
    size_t n=17u;
    MusaDwgAc1018PageLocation pages[2]={{0}},saved[2]={{0}};
    assert(musa_dwg_ac1024_page_locations(
        file,sizeof file,NULL,0u,&n)&&n==2u);
    assert(musa_dwg_ac1024_page_locations(
        file,sizeof file,pages,2u,&n));
    assert(pages[0].file_offset==256u&&
           pages[1].file_offset==320u&&
           pages[1].file_span_bytes==122u);
    assert(memcmp(file,before,sizeof file)==0);
    memcpy(saved,pages,sizeof pages);
    const size_t saved_n=n;
    assert(!musa_dwg_ac1018_page_locations(
        file,sizeof file,pages,2u,&n));
    assert(n==saved_n&&memcmp(pages,saved,sizeof saved)==0);
    file[359u]^=1u; /* exact SYSTEM marker is mandatory */
    assert(!musa_dwg_ac1024_page_locations(
        file,sizeof file,pages,2u,&n));
    assert(n==saved_n&&memcmp(pages,saved,sizeof saved)==0);
    memcpy(file,before,sizeof file);
    file[388u]^=1u; /* copied encrypted header must match */
    assert(!musa_dwg_ac1024_page_locations(
        file,sizeof file,pages,2u,&n));
    assert(n==saved_n&&memcmp(pages,saved,sizeof saved)==0);
    memcpy(file,before,sizeof file);
    le32(file+340u+13u,123u); /* even checksum-correct 43-byte tail fails */
    uint8_t hdr[20];memcpy(hdr,file+320u,20u);
    memset(hdr+16u,0,4u);
    le32(file+336u,checksum(checksum(0u,hdr,20u),file+340u,18u));
    MusaDwgAc1018FileMapHeader verified={0};
    assert(musa_dwg_ac1024_page_map_header(
        file,sizeof file,&verified));
    assert(!musa_dwg_ac1024_page_locations(
        file,sizeof file,pages,2u,&n));
    assert(n==saved_n&&memcmp(pages,saved,sizeof saved)==0);
}

/* A self-contained, synthetic AC1024 SYSTEM page envelope fixture; never
 * includes any bytes from the original user drawings in Projeler/. */
static void r2010_section_envelope_test(void){
    uint8_t file[380];fixture(file);
    memcpy(file,"AC1024",6u);
    const size_t pos=256u;
    le32(file+pos,UINT32_C(0x4163003b));
    le32(file+pos+4u,4u);
    le32(file+pos+8u,6u);
    le32(file+pos+12u,2u);
    file[pos+20u]=1u; /* literal 4 bytes */
    le32(file+pos+21u,3u); /* diagnostic-only candidate count */
    file[pos+25u]=0x11u;
    uint8_t clean[20];memcpy(clean,file+pos,20u);
    memset(clean+16u,0,4u);
    le32(file+pos+16u,checksum(
         checksum(0u,clean,20u),file+pos+20u,6u));
    uint8_t original[380];memcpy(original,file,sizeof(file));
    MusaDwgAc1024SectionEnvelopeEvidence result={0};
    assert(musa_dwg_ac1024_section_envelope_probe(
        file,sizeof(file),&result));
    assert(result.adler_verified&&result.strict_lz_completed);
    assert(result.section_map_page_id==1u&&
           result.original_physical_offset==256u&&
           result.declared_expanded_bytes==4u&&
           result.declared_section_count_candidate==3u);
    assert(memcmp(file,original,sizeof(file))==0);
    MusaDwgAc1024SectionEnvelopeEvidence saved=result;
    assert(!musa_dwg_ac1024_section_envelope_probe(
        file,sizeof(file),
        (MusaDwgAc1024SectionEnvelopeEvidence*)(void*)(file+4u)));
    assert(memcmp(file,original,sizeof(file))==0);
    file[pos+24u]^=1u; /* source checksum and caller evidence protected */
    assert(!musa_dwg_ac1024_section_envelope_probe(
        file,sizeof(file),&result));
    assert(memcmp(&result,&saved,sizeof(result))==0);
    memcpy(file,original,sizeof(file));
    file[pos+12u]=3u; /* unsupported source compression codec */
    assert(!musa_dwg_ac1024_section_envelope_probe(
        file,sizeof(file),&result));
    assert(memcmp(&result,&saved,sizeof(result))==0);
    memcpy(file,original,sizeof(file));
    file[0]='X'; /* wrong DWG signature is not interpreted as R2010 */
    assert(!musa_dwg_ac1024_section_envelope_probe(
        file,sizeof(file),&result));
    assert(memcmp(&result,&saved,sizeof(result))==0);
}
int main(void){
    uint8_t file[380];fixture(file);
    MusaDwgAc1018FileMapHeader h={0};
    assert(musa_dwg_ac1018_page_map_header(file,sizeof(file),&h));
    assert(h.page_count==2u&&h.page_map_id==2u&&h.section_map_id==1u);
    assert(h.page_map_offset==320u&&h.map_decoded_bytes==16u);
    size_t n=999u;
    assert(musa_dwg_ac1018_page_locations(file,sizeof(file),NULL,0u,&n));
    assert(n==2u);
    MusaDwgAc1018PageLocation entries[2]={{0}};
    assert(musa_dwg_ac1018_page_locations(
        file,sizeof(file),entries,2u,&n));
    assert(entries[0].page_id==1u&&entries[0].file_offset==256u&&
           entries[0].file_span_bytes==64u);
    assert(entries[1].page_id==2u&&entries[1].file_offset==320u&&
           entries[1].file_span_bytes==64u);
    MusaDwgAc1018PageLocation before[2];memcpy(before,entries,sizeof(before));
    const size_t n_before=n;
    assert(!musa_dwg_ac1018_page_locations(
        file,sizeof(file),entries,1u,&n));
    file[320u+20u+9u]^=1u; /* bad checksum */
    assert(!musa_dwg_ac1018_page_locations(
        file,sizeof(file),entries,2u,&n));
    assert(n==n_before&&memcmp(before,entries,sizeof(before))==0);
    fixture(file);
    file[0x80u]^=1u; /* corrupt encrypted fixed header */
    assert(!musa_dwg_ac1018_page_map_header(file,sizeof(file),&h));
    fixture(file);
    file[320u+12u]=3u; /* unsupported codec */
    assert(!musa_dwg_ac1018_page_map_header(file,sizeof(file),&h));
    fixture(file);
    assert(!musa_dwg_ac1018_page_locations(file,337u,entries,2u,&n));
    assert(n==n_before&&memcmp(before,entries,sizeof(before))==0);
    ac1024_page_map_test();
    test_footer44();
    r2010_section_envelope_test();
    return 0;
}
