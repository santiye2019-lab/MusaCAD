/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_page_map.h"
#include "musacad_dwg_ac1024_section_envelope.h"
#include "musacad_dwg_ac1024_section_directory.h"
#include "musacad_crc16.h"
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

static void r2010_directory_shape_candidate_test(void){
    uint8_t decoded[132]={0},snapshot[132]={0};
    le32(decoded,1u); /* one record at 20, one 16-byte page link */
    uint8_t *record=decoded+20u;
    le32(record,64u); /* 64 logical bytes */
    le32(record+8u,1u); /* one source page candidate */
    memcpy(record+32u,"AcDb:Handles",13u);
    le32(decoded+116u,7u); /* physical page ID candidate */
    le32(decoded+120u,16u); /* packed byte count candidate */
    memcpy(snapshot,decoded,sizeof decoded);
    MusaDwgAc1024DirectoryShape shape={0};
    assert(musa_dwg_ac1024_directory_shape(
        decoded,sizeof decoded,&shape));
    assert(shape.sections==1u&&shape.referenced_pages==1u&&
           shape.handles_name_candidates==1u&&
           shape.objects_name_candidates==0u&&
           shape.duplicate_section_names==0u);
    assert(memcmp(decoded,snapshot,sizeof decoded)==0);
    const MusaDwgAc1024DirectoryShape before=shape;
    assert(!musa_dwg_ac1024_directory_shape(
        decoded,sizeof decoded,(MusaDwgAc1024DirectoryShape*)(void*)
            (decoded+8u)));
    assert(memcmp(decoded,snapshot,sizeof decoded)==0);
    decoded[116u]=0u;
    assert(!musa_dwg_ac1024_directory_shape(
        decoded,sizeof decoded,&shape));
    assert(memcmp(&shape,&before,sizeof shape)==0);
    memcpy(decoded,snapshot,sizeof decoded);
    le32(decoded+28u,2u); /* declared page refs exceed record boundary */
    assert(!musa_dwg_ac1024_directory_shape(
        decoded,sizeof decoded,&shape));
    assert(memcmp(&shape,&before,sizeof shape)==0);
    memcpy(decoded,snapshot,sizeof decoded);
    memset(decoded+52u,'X',64u); /* un-terminated class name */
    assert(!musa_dwg_ac1024_directory_shape(
        decoded,sizeof decoded,&shape));
    assert(memcmp(&shape,&before,sizeof shape)==0);
    memcpy(decoded,snapshot,sizeof decoded);
    assert(!musa_dwg_ac1024_directory_shape(
        decoded,sizeof decoded-1u,&shape));
    assert(memcmp(&shape,&before,sizeof shape)==0);
}

static void r2010_directory_original_page_link_test(void){
    uint8_t decoded[132]={0};
    le32(decoded,1u);
    le32(decoded+20u,64u); /* declared logical section bytes */
    le32(decoded+28u,1u); /* one page reference */
    memcpy(decoded+52u,"AcDb:AcDbObjects",17u);
    le32(decoded+116u,7u); /* original physical page ID */
    le32(decoded+120u,16u); /* compressed source payload bytes */
    MusaDwgAc1018PageLocation locations[1]={{7u,64u,256u}};
    MusaDwgAc1024DirectoryPageLinkEvidence evidence={0};
    assert(musa_dwg_ac1024_directory_physical_links(
        decoded,sizeof decoded,locations,1u,&evidence));
    assert(evidence.matched_page_references==1u&&
           evidence.matched_section_records==1u);
    const MusaDwgAc1024DirectoryPageLinkEvidence saved=evidence;
    locations[0].file_span_bytes=47u; /* < header 32 + payload 16 */
    assert(!musa_dwg_ac1024_directory_physical_links(
        decoded,sizeof decoded,locations,1u,&evidence));
    assert(memcmp(&evidence,&saved,sizeof saved)==0);
    locations[0].file_span_bytes=64u;
    locations[0].page_id=8u; /* non-existent original ID */
    assert(!musa_dwg_ac1024_directory_physical_links(
        decoded,sizeof decoded,locations,1u,&evidence));
    assert(memcmp(&evidence,&saved,sizeof saved)==0);
    locations[0].page_id=7u;
    assert(!musa_dwg_ac1024_directory_physical_links(
        decoded,sizeof decoded,locations,1u,
        (MusaDwgAc1024DirectoryPageLinkEvidence*)
            (void*)(decoded+4u)));
}

static void r2010_data_page_crc_candidate_test(void){
    uint8_t decoded[132]={0},source[64]={0};
    le32(decoded,1u);
    le32(decoded+20u,4u);   /* section logical length */
    le32(decoded+28u,1u);   /* one physical page */
    le32(decoded+32u,4u);   /* declared logical page bytes */
    le32(decoded+40u,2u);   /* source section LZ compression */
    le32(decoded+44u,9u);   /* section id */
    memcpy(decoded+52u,"AcDb:Handles",13u);
    le32(decoded+116u,7u);
    le32(decoded+120u,6u);
    const uint8_t compressed[]={1u,'A','B','C','D',0x11u};
    memcpy(source+32u,compressed,sizeof compressed);
    const uint32_t data_crc=checksum(0u,source+32u,sizeof compressed);
    uint32_t words[8]={UINT32_C(0x4163043b),9u,6u,6u,
                       0u,0u,0u,data_crc};
    uint8_t raw[32];
    for(size_t k=0;k<8u;k++)le32(raw+k*4u,words[k]);
    words[6]=checksum(data_crc,raw,sizeof raw);
    for(size_t k=0;k<8u;k++)
        le32(source+k*4u,words[k]^UINT32_C(0x4164536b));
    uint8_t immutable[64];memcpy(immutable,source,sizeof source);
    MusaDwgAc1018PageLocation loc[1]={{7u,64u,0u}};
    MusaDwgAc1024PageChecksumEvidence e={0};
    assert(musa_dwg_ac1024_directory_page_checksums(
        source,sizeof source,decoded,sizeof decoded,loc,1u,&e));
    assert(e.referenced_pages==1u&&e.masked_headers_verified==1u&&
           e.payload_adler_verified==1u&&e.header_adler_verified==1u&&
           e.strict_lz_eligible_pages==1u&&e.strict_lz_verified==1u);
    assert(memcmp(immutable,source,sizeof source)==0);
    source[33u]^=1u; /* source payload CRC corruption must refuse LZ */
    assert(musa_dwg_ac1024_directory_page_checksums(
        source,sizeof source,decoded,sizeof decoded,loc,1u,&e));
    assert(e.referenced_pages==1u&&e.masked_headers_verified==1u&&
           !e.payload_adler_verified&&!e.header_adler_verified&&
           !e.strict_lz_eligible_pages&&!e.strict_lz_verified);
    memcpy(source,immutable,sizeof source);
    source[24u]^=1u; /* masked header checksum corrupted */
    assert(musa_dwg_ac1024_directory_page_checksums(
        source,sizeof source,decoded,sizeof decoded,loc,1u,&e));
    assert(e.masked_headers_verified==1u&&e.payload_adler_verified==1u&&
           !e.header_adler_verified&&!e.strict_lz_verified);
    memcpy(source,immutable,sizeof source);
    source[0u]^=1u; /* wrong DATA page envelope magic */
    assert(musa_dwg_ac1024_directory_page_checksums(
        source,sizeof source,decoded,sizeof decoded,loc,1u,&e));
    assert(!e.masked_headers_verified&&!e.payload_adler_verified&&
           !e.header_adler_verified&&!e.strict_lz_verified);
    /* Authenticated checksums do not make malformed LZ valid. */
    memcpy(source,immutable,sizeof source);
    source[32u]=0x11u; /* terminates with zero output instead of four */
    words[7]=checksum(0u,source+32u,sizeof compressed);
    words[6]=0u;
    for(size_t k=0;k<8u;k++)le32(raw+k*4u,words[k]);
    words[6]=checksum(words[7],raw,sizeof raw);
    for(size_t k=0;k<8u;k++)
        le32(source+k*4u,words[k]^UINT32_C(0x4164536b));
    assert(musa_dwg_ac1024_directory_page_checksums(
        source,sizeof source,decoded,sizeof decoded,loc,1u,&e));
    assert(e.header_adler_verified==1u&&e.strict_lz_eligible_pages==1u&&
           !e.strict_lz_verified);
    memcpy(source,immutable,sizeof source);
    le32(decoded+40u,1u); /* uncompressed section: never claim LZ */
    assert(musa_dwg_ac1024_directory_page_checksums(
        source,sizeof source,decoded,sizeof decoded,loc,1u,&e));
    assert(e.header_adler_verified==1u&&
           !e.strict_lz_eligible_pages&&!e.strict_lz_verified);
    le32(decoded+40u,2u);
    loc[0].page_id=8u;
    const MusaDwgAc1024PageChecksumEvidence prior=e;
    assert(!musa_dwg_ac1024_directory_page_checksums(
        source,sizeof source,decoded,sizeof decoded,loc,1u,&e));
    /* Invalid metadata is fail-atomic: preserve the caller's old result,
     * not zero it as in an accepted but individually refused DATA page. */
    assert(memcmp(&e,&prior,sizeof e)==0);
    loc[0].page_id=7u;
    assert(!musa_dwg_ac1024_directory_page_checksums(
        source,sizeof source,decoded,sizeof decoded,loc,1u,
        (MusaDwgAc1024PageChecksumEvidence*)(void*)(source+4u)));
    assert(memcmp(source,immutable,sizeof source)==0);
}

static void r2010_original_logical_section_join_test(void){
    uint8_t source[128]={0},directory[132]={0};
    memcpy(source,"AC1024",6u);
    le32(directory,1u);
    le32(directory+20u,4u);
    le32(directory+28u,1u);
    le32(directory+32u,4u);
    le32(directory+40u,2u);
    le32(directory+44u,9u);
    memcpy(directory+52u,"AcDb:Handles",13u);
    le32(directory+116u,7u);
    le32(directory+120u,6u);
    const uint8_t packed[]={1u,'A','B','C','D',0x11u};
    memcpy(source+96u,packed,sizeof packed);
    const uint32_t payload_crc=checksum(0u,source+96u,sizeof packed);
    uint32_t words[8]={UINT32_C(0x4163043b),9u,6u,6u,
                       0u,0u,0u,payload_crc};
    uint8_t clear[32];
    for(size_t i=0u;i<8u;i++)le32(clear+4u*i,words[i]);
    words[6]=checksum(payload_crc,clear,32u);
    const uint32_t mask=UINT32_C(0x4164536b)^64u;
    for(size_t i=0u;i<8u;i++)
        le32(source+64u+4u*i,words[i]^mask);
    const uint8_t original[128]={0}; (void)original;
    uint8_t before[128];memcpy(before,source,sizeof before);
    MusaDwgAc1018PageLocation loc[1]={{7u,64u,64u}};
    uint8_t out[4]={'Z','Z','Z','Z'};
    size_t bytes=999u;
    assert(musa_dwg_ac1024_directory_join_section(
        source,sizeof source,directory,sizeof directory,loc,1u,
        "AcDb:Handles",out,sizeof out,&bytes));
    assert(bytes==4u&&memcmp(out,"ABCD",4u)==0);
    assert(memcmp(source,before,sizeof source)==0);
    memcpy(out,"ZZZZ",4u);bytes=999u;
    source[98u]^=1u;
    assert(!musa_dwg_ac1024_directory_join_section(
        source,sizeof source,directory,sizeof directory,loc,1u,
        "AcDb:Handles",out,sizeof out,&bytes));
    assert(memcmp(out,"ZZZZ",4u)==0&&bytes==999u);
    memcpy(source,before,sizeof source);
    assert(!musa_dwg_ac1024_directory_join_section(
        source,sizeof source,directory,sizeof directory,loc,1u,
        "AcDb:AcDbObjects",out,sizeof out,&bytes));
    assert(memcmp(out,"ZZZZ",4u)==0&&bytes==999u);
    /* The same source-authenticated physical page may be used for a
     * differently named section only when the exact directory name
     * matches. No record parsing or geometry is inferred here. */
    memset(directory+52u,0u,64u);
    memcpy(directory+52u,"AcDb:AcDbObjects",16u);
    assert(musa_dwg_ac1024_directory_join_section(
        source,sizeof source,directory,sizeof directory,loc,1u,
        "AcDb:AcDbObjects",out,sizeof out,&bytes));
    assert(bytes==4u&&memcmp(out,"ABCD",4u)==0);
    assert(memcmp(source,before,sizeof source)==0);
    memcpy(out,"ZZZZ",4u);bytes=999u;
    source[97u]^=1u; /* source checksum refusal for Objects */
    assert(!musa_dwg_ac1024_directory_join_section(
        source,sizeof source,directory,sizeof directory,loc,1u,
        "AcDb:AcDbObjects",out,sizeof out,&bytes));
    assert(bytes==999u&&memcmp(out,"ZZZZ",4u)==0);
    memcpy(source,before,sizeof source);
    memset(directory+52u,0u,64u);
    memcpy(directory+52u,"AcDb:Handles",13u);
    assert(!musa_dwg_ac1024_directory_join_section(
        source,sizeof source,directory,sizeof directory,loc,1u,
        "AcDb:Handles",out,3u,&bytes));
    assert(memcmp(out,"ZZZZ",4u)==0&&bytes==999u);
    /* Source/output overlap is forbidden, even on authentic data. */
    assert(!musa_dwg_ac1024_directory_join_section(
        source,sizeof source,directory,sizeof directory,loc,1u,
        "AcDb:Handles",source+96u,4u,&bytes));
    assert(memcmp(source,before,sizeof source)==0);
    le32(directory+48u,2u); /* encrypted section */
    assert(!musa_dwg_ac1024_directory_join_section(
        source,sizeof source,directory,sizeof directory,loc,1u,
        "AcDb:Handles",out,sizeof out,&bytes));
    le32(directory+48u,0u);
    le32(directory+124u,1u); /* invalid logical offset */
    assert(!musa_dwg_ac1024_directory_join_section(
        source,sizeof source,directory,sizeof directory,loc,1u,
        "AcDb:Handles",out,sizeof out,&bytes));
    assert(memcmp(out,"ZZZZ",4u)==0&&bytes==999u);
}


static void r2010_zero_page_size_source_lz_join_test(void){
    uint8_t source[4096]={0},directory[132]={0};
    uint8_t joined[29696]={0};
    memcpy(source,"AC1024",6u);
    le32(directory,1u);                /* exactly one named section */
    le32(directory+20u,29696u);        /* exact logical byte count */
    le32(directory+28u,1u);            /* one page */
    /* directory+32: zero declared page span; strict 0x7400 hypothesis */
    le32(directory+40u,2u);            /* compressed, not encrypted */
    le32(directory+44u,9u);
    memcpy(directory+52u,"AcDb:Handles",13u);
    le32(directory+116u,7u);
    size_t at=96u;
    source[at++]=1u;
    memcpy(source+at,"ABCD",4u);at+=4u;
    /* Long-run backreferences to the four already decoded bytes.
     * 899*33 + 25 + first four bytes = exactly 29,696. */
    for(unsigned i=0u;i<899u;i++){
        source[at++]=0x3fu;source[at++]=12u;source[at++]=0u;
    }
    source[at++]=0x37u;source[at++]=12u;source[at++]=0u;
    source[at++]=0x11u;
    const uint32_t packed=(uint32_t)(at-96u);
    le32(directory+120u,packed);
    const uint32_t payload_crc=checksum(0u,source+96u,packed);
    uint32_t words[8]={UINT32_C(0x4163043b),9u,packed,packed,
                       0u,0u,0u,payload_crc};
    uint8_t clear[32]={0};
    for(size_t k=0u;k<8u;k++)le32(clear+4u*k,words[k]);
    words[6]=checksum(payload_crc,clear,32u);
    const uint32_t mask=UINT32_C(0x4164536b)^64u;
    for(size_t k=0u;k<8u;k++)
        le32(source+64u+4u*k,words[k]^mask);
    uint8_t immutable[4096];memcpy(immutable,source,sizeof source);
    MusaDwgAc1018PageLocation loc[1]={{7u,4096u-64u,64u}};
    size_t produced=123u;
    assert(musa_dwg_ac1024_directory_join_section(
        source,sizeof source,directory,sizeof directory,loc,1u,
        "AcDb:Handles",joined,sizeof joined,&produced));
    assert(produced==29696u);
    for(size_t i=0u;i<sizeof joined;i++)
        assert(joined[i]=="ABCD"[i%4u]);
    assert(memcmp(immutable,source,sizeof source)==0);
    /* Authenticated DATA checksum corruption cannot publish partial data. */
    source[98u]^=1u;produced=123u;memset(joined,0xa5,sizeof joined);
    assert(!musa_dwg_ac1024_directory_join_section(
        source,sizeof source,directory,sizeof directory,loc,1u,
        "AcDb:Handles",joined,sizeof joined,&produced));
    assert(produced==123u&&joined[0]==0xa5&&joined[sizeof joined-1u]==0xa5);
}


/* Original R2010 Handles pages: validate bytes/CRC only, never decode
 * a handle ID, object offset, owner, or geometry in this test. */
static void r2010_handles_crc_page_frame_test(void){
    uint8_t pages[32]={0};
    pages[0]=0u;pages[1]=4u;pages[2]='A';pages[3]='B';
    pages[6]=0u;pages[7]=5u;
    pages[8]='C';pages[9]='D';pages[10]='E';
    pages[13]=0u;pages[14]=2u;
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,pages,4u,&crc));
    pages[4]=(uint8_t)(crc>>8u);pages[5]=(uint8_t)crc;
    assert(musa_core_crc16_arc(0xc0c1u,pages+6u,5u,&crc));
    pages[11]=(uint8_t)(crc>>8u);pages[12]=(uint8_t)crc;
    assert(musa_core_crc16_arc(0xc0c1u,pages+13u,2u,&crc));
    pages[15]=(uint8_t)(crc>>8u);pages[16]=(uint8_t)crc;
    uint8_t original[32];memcpy(original,pages,sizeof pages);
    MusaDwgAc1024HandlesPageFrames frames={0};
    assert(musa_dwg_ac1024_handles_page_frames(
        pages,17u,&frames));
    assert(frames.data_pages==2u&&frames.total_pages==3u&&
           frames.data_bytes==5u&&frames.terminal_crc_verified==1u);
    assert(memcmp(original,pages,sizeof pages)==0);
    const MusaDwgAc1024HandlesPageFrames before=frames;
    /* A truncated terminal CRC, CRC-correct data but missing final
     * terminal, corrupted data CRC, extra tail and an invalid page length
     * must all refuse atomically. */
    assert(!musa_dwg_ac1024_handles_page_frames(pages,16u,&frames));
    assert(!musa_dwg_ac1024_handles_page_frames(pages,13u,&frames));
    assert(!musa_dwg_ac1024_handles_page_frames(pages,18u,&frames));
    pages[9u]^=1u;
    assert(!musa_dwg_ac1024_handles_page_frames(pages,17u,&frames));
    memcpy(pages,original,sizeof pages);
    pages[1u]=1u;
    assert(!musa_dwg_ac1024_handles_page_frames(pages,17u,&frames));
    memcpy(pages,original,sizeof pages);
    assert(!musa_dwg_ac1024_handles_page_frames(
        pages,17u,(MusaDwgAc1024HandlesPageFrames*)(void*)(pages+4u)));
    assert(!musa_dwg_ac1024_handles_page_frames(pages,0u,&frames));
    assert(memcmp(original,pages,sizeof pages)==0);
    assert(memcmp(&frames,&before,sizeof frames)==0);
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
    r2010_directory_shape_candidate_test();
    r2010_directory_original_page_link_test();
    r2010_data_page_crc_candidate_test();
    r2010_original_logical_section_join_test();
    r2010_zero_page_size_source_lz_join_test();
    r2010_handles_crc_page_frame_test();
    return 0;
}
