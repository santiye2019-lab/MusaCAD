/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1024_section_envelope.h"
#include "musacad_dwg_ac1018_page_map.h"
#include "musacad_dwg_ac1018_lz.h"
#include "musacad_dwg_ac1024_section_directory.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#define MUSA_AC1024_MAX_PHYSICAL 65536u
#define MUSA_AC1024_MAX_SECTION_BYTES 1048576u
static uint32_t read32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8u)|
           ((uint32_t)p[2]<<16u)|((uint32_t)p[3]<<24u);
}
static uint32_t adler(uint32_t seed,const uint8_t *p,size_t n){
    uint64_t lo=seed&65535u,hi=seed>>16u;
    for(size_t at=0u;at<n;){
        const size_t take=n-at<5552u?n-at:5552u;
        for(size_t i=0u;i<take;i++){lo+=p[at+i];hi+=lo;}
        lo%=65521u;hi%=65521u;at+=take;
    }
    return ((uint32_t)hi<<16u)|(uint32_t)lo;
}
static int overlap(const void *a,size_t na,const void *b,size_t nb){
    if(!a||!b||!na||!nb)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<na:x-y<nb;
}
int musa_dwg_ac1024_section_envelope_probe(const uint8_t *file,size_t n,
    MusaDwgAc1024SectionEnvelopeEvidence *out){
    if(!file||!out||overlap(file,n,out,sizeof(*out)))return 0;
    MusaDwgAc1018FileMapHeader header={0};
    if(!musa_dwg_ac1024_page_map_header(file,n,&header)||
       !header.page_count||header.page_count>MUSA_AC1024_MAX_PHYSICAL)
        return 0;
    size_t count=0u;
    if(!musa_dwg_ac1024_page_locations(file,n,NULL,0u,&count)||
       count!=header.page_count)return 0;
    MusaDwgAc1018PageLocation *pages=
        (MusaDwgAc1018PageLocation*)calloc(count,sizeof(*pages));
    if(!pages)return 0;
    int ok=0;
    if(!musa_dwg_ac1024_page_locations(
           file,n,pages,count,&count)||count!=header.page_count)
        goto done;
    for(size_t i=0u;i<count;i++){
        if(pages[i].page_id!=header.section_map_id)continue;
        if(pages[i].file_offset>n||
           pages[i].file_span_bytes<20u||
           n-(size_t)pages[i].file_offset<20u)break;
        const size_t pos=(size_t)pages[i].file_offset;
        const uint8_t *page=file+pos;
        const uint32_t kind=read32(page),expanded=read32(page+4u),
                       packed=read32(page+8u),codec=read32(page+12u),
                       stored=read32(page+16u);
        if(kind!=UINT32_C(0x4163003b)||codec!=2u||
           expanded<4u||expanded>MUSA_AC1024_MAX_SECTION_BYTES||
           !packed||packed>MUSA_AC1024_MAX_SECTION_BYTES||
           packed>pages[i].file_span_bytes-20u||
           packed>n-pos-20u)break;
        uint8_t clean[20];memcpy(clean,page,20u);
        memset(clean+16u,0,4u);
        if(adler(adler(0u,clean,20u),page+20u,packed)!=stored)break;
        MusaDwgAc1024SectionEnvelopeEvidence e={0};
        e.section_map_page_id=header.section_map_id;
        e.compressed_bytes=packed;
        e.declared_expanded_bytes=expanded;
        e.original_physical_offset=pages[i].file_offset;
        e.adler_verified=1u;
        uint8_t *decoded=(uint8_t*)malloc(expanded);
        if(!decoded)break;
        size_t consumed=0u;
        if(musa_dwg_ac1018_lz_decode(page+20u,packed,decoded,expanded,
                                     &consumed)&&consumed<=packed){
            e.strict_lz_completed=1u;
            e.declared_section_count_candidate=read32(decoded);
            MusaDwgAc1024DirectoryShape shape={0};
            if(musa_dwg_ac1024_directory_shape(decoded,expanded,&shape)){
                e.directory_shape_candidate=1u;
                e.source_handles_name_candidates=
                    shape.handles_name_candidates;
                e.source_directory_duplicate_names=
                    shape.duplicate_section_names;
                e.directory_sections_candidate=shape.sections;
                e.directory_referenced_pages_candidate=
                    shape.referenced_pages;
                MusaDwgAc1024DirectoryPageLinkEvidence linked={0};
                if(musa_dwg_ac1024_directory_physical_links(
                       decoded,expanded,pages,count,&linked)){
                    e.directory_physical_links_candidate=1u;
                    e.directory_matched_physical_refs_candidate=
                        linked.matched_page_references;
                    MusaDwgAc1024PageChecksumEvidence crc={0};
                    if(musa_dwg_ac1024_directory_page_checksums(
                           file,n,decoded,expanded,pages,count,&crc)){
                        e.source_data_masked_headers_verified=
                            crc.masked_headers_verified;
                        e.source_data_payload_adler_verified=
                            crc.payload_adler_verified;
                        e.source_data_header_adler_verified=
                            crc.header_adler_verified;
                        e.source_data_lz_eligible_pages=
                            crc.strict_lz_eligible_pages;
                        e.source_data_lz_verified=
                            crc.strict_lz_verified;
                        /* Only complete, original-source verified pages
                         * may be joined. Diagnostic-only: never emit the
                         * resulting Handles bytes or claim ownership/paint.
                         * Bound additional report-stage memory to 16 MiB. */
                        if(crc.header_adler_verified==crc.referenced_pages){
                            size_t pos=20u;
                            for(uint32_t si=0u;si<shape.sections;si++){
                                const uint8_t *rec=decoded+pos;
                                const uint32_t count=read32(rec+8u);
                                const uint64_t logical=
                                    (uint64_t)read32(rec)|
                                    ((uint64_t)read32(rec+4u)<<32u);
                                if(strcmp((const char*)(rec+32u),
                                          "AcDb:Handles")==0){
                                    e.source_handles_candidate_pages=count;
                                    e.source_handles_candidate_logical_bytes=
                                        (uint32_t)logical;
                                    e.source_handles_candidate_page_bytes=
                                        read32(rec+12u);
                                    e.source_handles_candidate_compression=
                                        read32(rec+20u);
                                    e.source_handles_candidate_encryption=
                                        read32(rec+28u);
                                }
                                if(strcmp((const char*)(rec+32u),
                                          "AcDb:Handles")==0&&
                                   logical>0u&&logical<=16u*1024u*1024u){
                                    uint8_t *joined=(uint8_t*)malloc(
                                        (size_t)logical);
                                    if(joined){
                                        size_t written=0u;
                                        if(musa_dwg_ac1024_directory_join_section(
                                               file,n,decoded,expanded,
                                               pages,count,"AcDb:Handles",
                                               joined,(size_t)logical,
                                               &written)&&written==logical){
                                            e.source_handles_logical_join_verified=1u;
                                            e.source_handles_logical_bytes=
                                                (uint32_t)written;
                                        }
                                        free(joined);
                                    }
                                }
                                pos+=96u+(size_t)count*16u;
                            }
                        }
                    }
                }
            }
        }
        free(decoded);
        *out=e;
        ok=1;
        break;
    }
done:
    free(pages);
    return ok;
}
