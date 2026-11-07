/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1024_section_directory.h"
#include "musacad_dwg_ac1018_lz.h"
#include "musacad_crc16.h"
#include "musacad_dwg_ac1018_handles.h"
#include "musacad_dwg_ac1024_self_index.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#define MAX_SECTION_BYTES 1048576u
#define MAX_SECTIONS 256u
#define MAX_REFERENCES 65536u
static uint32_t read32(const uint8_t *p){
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8u) |
           ((uint32_t)p[2]<<16u) | ((uint32_t)p[3]<<24u);
}
static uint64_t read64(const uint8_t *p){
    return (uint64_t)read32(p) | ((uint64_t)read32(p+4u)<<32u);
}
static int overlap(const void *a,size_t na,const void *b,size_t nb){
    if(!a||!b||!na||!nb)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<na:x-y<nb;
}
int musa_dwg_ac1024_directory_shape(
    const uint8_t *decoded,size_t bytes,MusaDwgAc1024DirectoryShape *out){
    if(!decoded||!out||bytes<20u||bytes>MAX_SECTION_BYTES||
       overlap(decoded,bytes,out,sizeof(*out)))return 0;
    const uint32_t n=read32(decoded);
    if(n==0u||n>MAX_SECTIONS)return 0;
    /* The remaining 16 bytes precede the R2004-family records. This is a
     * layout hypothesis, so failure is diagnostic and never enables an
     * alternate full-DWG opening path. */
    uint8_t names[MAX_SECTIONS][64]={{0}};
    MusaDwgAc1024DirectoryShape evidence={0};
    evidence.sections=n;
    size_t at=20u;
    for(uint32_t i=0;i<n;i++){
        if(at>bytes||bytes-at<96u)return 0;
        const uint8_t *record=decoded+at;
        const uint64_t logical=read64(record);
        const uint32_t pages=read32(record+8u);
        const uint8_t *name=record+32u;
        if(logical>128u*1024u*1024u||
           memchr(name,0,64u)==NULL)return 0;
        if(pages && (!logical||!name[0]))return 0;
        if(i && name[0]){
            for(uint32_t j=0;j<i;j++){
                if(!strcmp((const char*)names[j],(const char*)name)){
                    evidence.duplicate_section_names=1u;
                    break;
                }
            }
        }
        memcpy(names[i],name,64u);
        if(!strcmp((const char*)name,"AcDb:AcDbObjects"))
            ++evidence.objects_name_candidates;
        if(!strcmp((const char*)name,"AcDb:Handles"))
            ++evidence.handles_name_candidates;
        at+=96u;
        if(pages>MAX_REFERENCES-evidence.referenced_pages||
           pages>(bytes-at)/16u)return 0;
        for(uint32_t j=0;j<pages;j++){
            const uint8_t *entry=decoded+at+(size_t)j*16u;
            if(!read32(entry)||!read32(entry+4u))return 0;
        }
        evidence.referenced_pages+=pages;
        at+=(size_t)pages*16u;
    }
    if(at!=bytes)return 0;
    *out=evidence;
    return 1;
}

int musa_dwg_ac1024_directory_physical_links(
    const uint8_t *decoded,size_t bytes,
    const MusaDwgAc1018PageLocation *locations,size_t physical_count,
    MusaDwgAc1024DirectoryPageLinkEvidence *out){
    if(!decoded||!locations||!out||!physical_count||
       physical_count>MAX_REFERENCES||
       physical_count>SIZE_MAX/sizeof(*locations)||
       overlap(decoded,bytes,out,sizeof(*out))||
       overlap(locations,physical_count*sizeof(*locations),
               out,sizeof(*out)))return 0;
    MusaDwgAc1024DirectoryShape shape={0};
    if(!musa_dwg_ac1024_directory_shape(decoded,bytes,&shape))return 0;
    for(size_t i=0;i<physical_count;i++){
        if(!locations[i].page_id ||
           (i&&locations[i-1u].page_id>=locations[i].page_id))
            return 0;
    }
    MusaDwgAc1024DirectoryPageLinkEvidence evidence={0};
    evidence.matched_section_records=shape.sections;
    size_t at=20u;
    for(uint32_t i=0;i<shape.sections;i++){
        const uint32_t npage=read32(decoded+at+8u);
        at+=96u;
        for(uint32_t j=0;j<npage;j++){
            const uint8_t *entry=decoded+at+(size_t)j*16u;
            const uint32_t id=read32(entry),packed=read32(entry+4u);
            size_t lo=0u,hi=physical_count;
            while(lo<hi){
                const size_t mid=lo+(hi-lo)/2u;
                if(locations[mid].page_id<id)lo=mid+1u;
                else hi=mid;
            }
            if(lo==physical_count||locations[lo].page_id!=id||
               locations[lo].file_span_bytes<32u||
               packed>locations[lo].file_span_bytes-32u)
                return 0;
            ++evidence.matched_page_references;
        }
        at+=(size_t)npage*16u;
    }
    if(at!=bytes||
       evidence.matched_page_references!=shape.referenced_pages)
        return 0;
    *out=evidence;
    return 1;
}

/* Chunked Adler32, identical to first-party authenticated R2004 page
 * checksums. Uses integer arithmetic only; never writes original source. */
static uint32_t adler32(uint32_t seed,const uint8_t *p,size_t n){
    uint64_t lo=seed&65535u,hi=seed>>16u;
    for(size_t at=0u;at<n;){
        const size_t count=n-at<5552u?n-at:5552u;
        for(size_t i=0u;i<count;i++){lo+=p[at+i];hi+=lo;}
        lo%=65521u;hi%=65521u;at+=count;
    }
    return ((uint32_t)hi<<16u)|(uint32_t)lo;
}
static void put32(uint8_t *p,uint32_t n){
    p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8u);
    p[2]=(uint8_t)(n>>16u);p[3]=(uint8_t)(n>>24u);
}
int musa_dwg_ac1024_directory_page_checksums(
    const uint8_t *file,size_t file_bytes,
    const uint8_t *decoded,size_t decoded_bytes,
    const MusaDwgAc1018PageLocation *locations,size_t physical_count,
    MusaDwgAc1024PageChecksumEvidence *out){
    if(!file||!decoded||!locations||!out||
       physical_count>SIZE_MAX/sizeof(*locations)||
       overlap(file,file_bytes,out,sizeof(*out))||
       overlap(decoded,decoded_bytes,out,sizeof(*out))||
       overlap(locations,physical_count*sizeof(*locations),
               out,sizeof(*out)))return 0;
    MusaDwgAc1024DirectoryPageLinkEvidence links={0};
    if(!musa_dwg_ac1024_directory_physical_links(
           decoded,decoded_bytes,locations,physical_count,&links))
        return 0;
    MusaDwgAc1024PageChecksumEvidence evidence={0};
    evidence.referenced_pages=links.matched_page_references;
    const uint32_t sections=read32(decoded);
    size_t at=20u;
    for(uint32_t i=0;i<sections;i++){
        const uint8_t *rec=decoded+at;
        const uint32_t npage=read32(rec+8u);
        const uint32_t section_id=read32(rec+24u);
        const uint32_t page_output_bytes=read32(rec+12u);
        const uint32_t compression=read32(rec+20u);
        const uint32_t encryption=read32(rec+28u);
        const uint64_t section_logical_bytes=read64(rec);
        const int handles_section=
            strcmp((const char*)(rec+32u),"AcDb:Handles")==0;
        if(handles_section)
            evidence.handles_page_references=npage;
        at+=96u;
        for(uint32_t j=0;j<npage;j++){
            const uint8_t *entry=decoded+at+(size_t)j*16u;
            const uint32_t id=read32(entry),packed=read32(entry+4u);
            const uint64_t logical=read64(entry+8u);
            size_t lo=0u,hi=physical_count;
            while(lo<hi){
                const size_t mid=lo+(hi-lo)/2u;
                if(locations[mid].page_id<id)lo=mid+1u;
                else hi=mid;
            }
            /* Physical link success checked above. A refused source page
             * contributes no geometry and never gets silently skipped. */
            if(lo>=physical_count||locations[lo].page_id!=id)
                continue;
            const MusaDwgAc1018PageLocation *loc=&locations[lo];
            if(!section_id||logical>UINT32_MAX||
               loc->file_offset>file_bytes||
               file_bytes-(size_t)loc->file_offset<32u||
               packed>file_bytes-(size_t)loc->file_offset-32u)
                continue;
            const size_t pos=(size_t)loc->file_offset;
            const uint32_t mask=UINT32_C(0x4164536b)^(uint32_t)pos;
            uint32_t words[8];
            for(size_t k=0u;k<8u;k++)
                words[k]=read32(file+pos+k*4u)^mask;
            if(words[0]!=UINT32_C(0x4163043b)||
               words[1]!=section_id||
               words[2]!=packed||
               words[3]<packed||words[3]-packed>63u||
               words[4]!=(uint32_t)logical||words[5]!=0u)
                continue;
            ++evidence.masked_headers_verified;
            const uint32_t payload_crc=adler32(0u,file+pos+32u,packed);
            if(payload_crc!=words[7])continue;
            ++evidence.payload_adler_verified;
            uint8_t clear[32];
            for(size_t k=0u;k<8u;k++)
                put32(clear+k*4u,k==6u?0u:words[k]);
            if(adler32(payload_crc,clear,sizeof clear)!=words[6])
                continue;
            ++evidence.header_adler_verified;
            /* Only checksum-authenticated original compressed DATA pages
             * enter this read-only LZ diagnostic. Never publish decoded
             * bytes, infer object identities, or admit an Android scene. */
            if(compression!=2u||encryption!=0u||
               !page_output_bytes||page_output_bytes>1048576u||
               !section_logical_bytes||
               (section_logical_bytes-1u)/page_output_bytes+1u!=npage||
               logical!=(uint64_t)j*page_output_bytes)
                continue;
            ++evidence.strict_lz_eligible_pages;
            if(handles_section)++evidence.handles_strict_lz_eligible;
            MusaDwgAc1018LzDiagnostic diag={0};
            if(musa_dwg_ac1018_lz_diagnose(
                   file+pos+32u,packed,page_output_bytes,&diag)&&
               diag.completed&&diag.input_consumed>0u&&
               diag.input_consumed<=packed&&
               diag.output_produced==page_output_bytes){
                ++evidence.strict_lz_verified;
                if(handles_section)++evidence.handles_strict_lz_verified;
            }
        }
        at+=(size_t)npage*16u;
    }
    if(at!=decoded_bytes)return 0;
    *out=evidence;
    return 1;
}


/* This helper joins only independently source-verified, unencrypted
 * AC1024 logical sections. It is not a scene or DWG object parser. */
static int join_section_inner(
    const uint8_t *file,size_t file_bytes,
    const uint8_t *directory,size_t directory_bytes,
    const MusaDwgAc1018PageLocation *locations,size_t physical_count,
    const char *section_name,
    uint8_t *output,size_t capacity,size_t *out_bytes,
    uint32_t *stage){
    if(stage)*stage=0u;
    if(!file||!directory||!locations||!section_name||!output||
       !out_bytes||file_bytes<6u||
       memcmp(file,"AC1024",6u)!=0||
       physical_count>SIZE_MAX/sizeof(*locations)||
       overlap(file,file_bytes,output,capacity)||
       overlap(directory,directory_bytes,output,capacity)||
       overlap(locations,physical_count*sizeof(*locations),
               output,capacity)||
       overlap(file,file_bytes,out_bytes,sizeof(*out_bytes))||
       overlap(directory,directory_bytes,out_bytes,sizeof(*out_bytes))||
       overlap(locations,physical_count*sizeof(*locations),
               out_bytes,sizeof(*out_bytes))||
       overlap(output,capacity,out_bytes,sizeof(*out_bytes))){
        if(stage)*stage=1u;
        return 0;
    }
    /* Never offer an arbitrary section extraction route to Android
     * without separately verified record/graph semantics. */
    if(strcmp(section_name,"AcDb:Handles")!=0 &&
       strcmp(section_name,"AcDb:AcDbObjects")!=0){
        if(stage)*stage=2u;
        return 0;
    }
    MusaDwgAc1024DirectoryShape shape={0};
    if(!musa_dwg_ac1024_directory_shape(
           directory,directory_bytes,&shape)||
       shape.duplicate_section_names){
        if(stage)*stage=3u;
        return 0;
    }
    MusaDwgAc1024PageChecksumEvidence crc={0};
    if(!musa_dwg_ac1024_directory_page_checksums(
           file,file_bytes,directory,directory_bytes,
           locations,physical_count,&crc)||
       crc.header_adler_verified!=crc.referenced_pages){
        if(stage)*stage=4u;
        return 0;
    }
    size_t at=20u,chosen_at=0u;
    uint64_t logical_bytes=0u;
    uint32_t npage=0u,page_bytes=0u;
    int found=0;
    for(uint32_t i=0u;i<shape.sections;i++){
        const uint8_t *rec=directory+at;
        const uint32_t count=read32(rec+8u);
        const char *name=(const char*)(rec+32u);
        if(strcmp(name,section_name)==0){
            if(found){if(stage)*stage=5u;return 0;}
            found=1;
            chosen_at=at+96u;
            logical_bytes=read64(rec);
            npage=count;
            page_bytes=read32(rec+12u);
            /* The authenticated R2004-family section directory may
             * declare page size zero for the conventional 0x7400 DATA
             * logical block. This candidate is only accepted when every
             * source logical offset, declared section span, checksum and
             * strict original LZ stream independently agree. */
            if(!page_bytes)page_bytes=29696u;
            if(read32(rec+20u)!=2u||read32(rec+28u)!=0u||
               !read32(rec+24u)){
                if(stage)*stage=6u;
                return 0;
            }
        }
        at+=96u+(size_t)count*16u;
    }
    if(!found||!npage){if(stage)*stage=7u;return 0;}
    if(!page_bytes||page_bytes>1048576u){if(stage)*stage=8u;return 0;}
    if(!logical_bytes||logical_bytes>128u*1024u*1024u||
       logical_bytes>capacity){if(stage)*stage=9u;return 0;}
    if((logical_bytes-1u)/page_bytes+1u!=npage){
        if(stage)*stage=10u;
        return 0;
    }
    uint8_t *staged=(uint8_t*)malloc((size_t)logical_bytes);
    uint8_t *block=(uint8_t*)malloc(page_bytes);
    if(!staged||!block){
        free(staged);free(block);
        if(stage)*stage=11u;
        return 0;
    }
    size_t covered=0u;
    int ok=1;
    for(uint32_t j=0u;j<npage;j++){
        const uint8_t *entry=directory+chosen_at+(size_t)j*16u;
        const uint32_t id=read32(entry);
        const uint32_t packed=read32(entry+4u);
        const uint64_t offset=read64(entry+8u);
        if(offset!=(uint64_t)j*page_bytes){
            if(stage)*stage=12u;
            ok=0;break;
        }
        size_t lo=0u,hi=physical_count;
        while(lo<hi){
            const size_t mid=lo+(hi-lo)/2u;
            if(locations[mid].page_id<id)lo=mid+1u;
            else hi=mid;
        }
        if(lo==physical_count||locations[lo].page_id!=id||
           locations[lo].file_offset>file_bytes||
           file_bytes-(size_t)locations[lo].file_offset<32u||
           packed>file_bytes-(size_t)locations[lo].file_offset-32u){
            if(stage)*stage=13u;
            ok=0;break;
        }
        const size_t pos=(size_t)locations[lo].file_offset;
        size_t consumed=0u;
        if(!musa_dwg_ac1018_lz_decode(
                file+pos+32u,packed,block,page_bytes,&consumed)||
           !consumed||consumed>packed){
            if(stage)*stage=14u;
            ok=0;break;
        }
        const size_t remaining=(size_t)logical_bytes-covered;
        const size_t take=remaining<page_bytes?remaining:page_bytes;
        memcpy(staged+covered,block,take);
        covered+=take;
    }
    if(covered!=(size_t)logical_bytes){
        if(stage&&!*stage)*stage=15u;
        ok=0;
    }
    if(ok){
        memcpy(output,staged,(size_t)logical_bytes);
        *out_bytes=(size_t)logical_bytes;
    }
    free(block);free(staged);
    return ok;
}

int musa_dwg_ac1024_directory_join_section(
    const uint8_t *file,size_t file_bytes,
    const uint8_t *directory,size_t directory_bytes,
    const MusaDwgAc1018PageLocation *locations,size_t physical_count,
    const char *section_name,
    uint8_t *output,size_t capacity,size_t *out_bytes){
    return join_section_inner(file,file_bytes,directory,directory_bytes,
                              locations,physical_count,section_name,
                              output,capacity,out_bytes,NULL);
}
int musa_dwg_ac1024_directory_join_section_diagnose(
    const uint8_t *file,size_t file_bytes,
    const uint8_t *directory,size_t directory_bytes,
    const MusaDwgAc1018PageLocation *locations,size_t physical_count,
    const char *section_name,
    uint8_t *output,size_t capacity,size_t *out_bytes,
    uint32_t *reason){
    if(!reason||physical_count>SIZE_MAX/sizeof(*locations)||
       overlap(reason,sizeof(*reason),file,file_bytes)||
       overlap(reason,sizeof(*reason),directory,directory_bytes)||
       overlap(reason,sizeof(*reason),locations,
               physical_count*sizeof(*locations))||
       overlap(reason,sizeof(*reason),output,capacity)||
       overlap(reason,sizeof(*reason),out_bytes,sizeof(*out_bytes)))
        return 0;
    uint32_t stage=0u;
    const int ok=join_section_inner(file,file_bytes,directory,directory_bytes,
                                    locations,physical_count,section_name,
                                    output,capacity,out_bytes,&stage);
    *reason=stage;
    return ok;
}


/* Diagnostic-only AC1024 Handles CRC page-frame structure. These framing
 * bytes may resemble R2004, but are NOT evidence that R2004 numeric handle
 * delta or object-offset semantics apply to R2010. */
int musa_dwg_ac1024_handles_page_frames(
    const uint8_t *joined_handles,size_t bytes,
    MusaDwgAc1024HandlesPageFrames *out){
    if(!joined_handles||!out||bytes<8u||bytes>16u*1024u*1024u||
       overlap(joined_handles,bytes,out,sizeof(*out)))
        return 0;
    MusaDwgAc1024HandlesPageFrames next={0};
    size_t at=0u;
    while(at<bytes){
        if(bytes-at<4u)return 0;
        const uint8_t *p=joined_handles+at;
        const uint32_t size=((uint32_t)p[0]<<8u)|(uint32_t)p[1];
        if(size<2u||size>2040u||size>bytes-at-2u)
            return 0;
        const size_t page_end=at+(size_t)size;
        const uint16_t stored=((uint16_t)joined_handles[page_end]<<8u)|
                              (uint16_t)joined_handles[page_end+1u];
        uint16_t verified=0u;
        if(!musa_core_crc16_arc(0xc0c1u,p,size,&verified)||
           verified!=stored)return 0;
        if(next.total_pages==UINT32_MAX)return 0;
        ++next.total_pages;
        if(size==2u){
            if(page_end+2u!=bytes||!next.data_pages)return 0;
            next.terminal_crc_verified=1u;
            *out=next;
            return 1;
        }
        if(next.data_pages==UINT32_MAX||
           size-2u>UINT32_MAX-next.data_bytes)return 0;
        ++next.data_pages;
        next.data_bytes+=size-2u;
        at=page_end+2u;
    }
    /* Missing terminal CRC is never accepted, even if all data pages pass. */
    return 0;
}


/* A borrowed delta profile is a bounded candidate grammar, not a semantic
 * R2010 handle map. Only source-record CRC/self-H can advance evidence. */
static int candidate_handle_mode(unsigned mode,
    const uint8_t *handles,size_t handle_bytes,size_t object_bytes,
    MusaDwgAc1018HandleEntry *entries,size_t capacity,size_t *count){
    if(mode==1u)
        return musa_dwg_ac1018_handles_index(
            handles,handle_bytes,object_bytes,entries,capacity,count);
    if(mode==2u)
        return musa_dwg_ac1018_handles_index_continuous_offsets(
            handles,handle_bytes,object_bytes,entries,capacity,count);
    if(mode==3u)
        return musa_dwg_ac1018_handles_index_page_absolute_handles(
            handles,handle_bytes,object_bytes,entries,capacity,count);
    return 0;
}

int musa_dwg_ac1024_handle_object_crosscheck(
    const uint8_t *handles,size_t handle_bytes,
    const uint8_t *objects,size_t object_bytes,
    MusaDwgAc1024HandleObjectCrosscheck *out){
    if(!handles||!objects||!out||handle_bytes>16u*1024u*1024u||
       !object_bytes||object_bytes>64u*1024u*1024u||
       overlap(handles,handle_bytes,objects,object_bytes)||
       overlap(handles,handle_bytes,out,sizeof(*out))||
       overlap(objects,object_bytes,out,sizeof(*out)))
        return 0;
    /* Do not parse a single original offset until EVERY Handles page has
     * passed CRC16/ARC and its exact terminal sentinel was verified. */
    MusaDwgAc1024HandlesPageFrames frames={0};
    if(!musa_dwg_ac1024_handles_page_frames(
           handles,handle_bytes,&frames)||
       !frames.terminal_crc_verified)
        return 0;
    MusaDwgAc1024HandleObjectCrosscheck next={0};
    for(unsigned mode=1u;mode<=3u;mode++){
        size_t count=0u;
        if(!candidate_handle_mode(mode,handles,handle_bytes,object_bytes,
                                  NULL,0u,&count)||
           !count||count>400000u||
           count>SIZE_MAX/sizeof(MusaDwgAc1018HandleEntry)||
           count>SIZE_MAX/sizeof(uint64_t)||
           count>SIZE_MAX/sizeof(MusaDwgAc1024SelfIdentity))
            continue;
        MusaDwgAc1018HandleEntry *entries=
            (MusaDwgAc1018HandleEntry*)malloc(count*sizeof(*entries));
        uint64_t *offsets=(uint64_t*)malloc(count*sizeof(*offsets));
        if(!entries||!offsets){
            free(entries);free(offsets);
            return 0;
        }
        size_t reproduced=0u;
        if(!candidate_handle_mode(mode,handles,handle_bytes,object_bytes,
                                  entries,count,&reproduced)||
           reproduced!=count){
            free(entries);free(offsets);
            continue;
        }
        ++next.candidate_profiles;
        uint32_t prefix=0u,matching=0u;
        uint64_t last_handle=0u;
        for(size_t k=0u;k<count;k++){
            offsets[k]=entries[k].object_offset;
            MusaDwgAc1024SelfPrefix self={0};
            if(!musa_dwg_ac1024_record_self_prefix(
                   objects,object_bytes,offsets[k],&self)||
               self.embedded_absolute_handle<=last_handle)
                break;
            last_handle=self.embedded_absolute_handle;
            ++prefix;
            if(entries[k].handle==self.embedded_absolute_handle)
                ++matching;
        }
        int entire=0;
        if(prefix==count){
            MusaDwgAc1024SelfIdentity *recovered=
                (MusaDwgAc1024SelfIdentity*)malloc(
                    count*sizeof(*recovered));
            if(!recovered){
                free(entries);free(offsets);
                return 0;
            }
            entire=musa_dwg_ac1024_recover_self_index(
                objects,object_bytes,offsets,count,recovered,count);
            free(recovered);
        }
        const int replace=(!next.candidate_profile ||
            (entire&&!next.complete_self_index_verified)||
            (entire==next.complete_self_index_verified&&
                (prefix>next.crc_self_prefix_pairs||
                 (prefix==next.crc_self_prefix_pairs&&
                  matching>next.candidate_numeric_matches))));
        if(replace){
            next.candidate_profile=(uint8_t)mode;
            next.candidate_pairs=(uint32_t)count;
            next.crc_self_prefix_pairs=prefix;
            next.candidate_numeric_matches=matching;
            next.complete_self_index_verified=(uint8_t)(entire!=0);
            next.complete_numeric_agreement=(uint8_t)(
                entire&&matching==(uint32_t)count);
        }
        free(entries);free(offsets);
    }
    *out=next;
    return 1;
}
