/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_handles.h"
#include "musacad_dwg_ac1032_sections.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1015_values.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MUSA_AC1032_HANDLE_FILE_MAX (64u * 1024u * 1024u)
#define MUSA_AC1032_HANDLES_MAX (1000000u)
#define MUSA_AC1032_SECTION_MAX (64u * 1024u * 1024u)

static int overlap(const void *p, size_t n, const void *q, size_t m){
    if(!n||!m)return 0;
    const uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;
    return a<=b?b-a<n:a-b<m;
}
/* Both original map profiles are CRC-validated before selection.
 * A candidate's index H is NOT proof of the record's embedded self H.
 * Inspect a bounded, distributed sample of independently CRC-checked
 * original entity records. Unsupported types contribute no evidence. */
typedef struct HandleMapEvidence {
    size_t exact,foreign;
} HandleMapEvidence;
static HandleMapEvidence handle_map_evidence(
    const uint8_t *objects,size_t object_bytes,
    const MusaDwgAc1032HandleEntry *entries,size_t count){
    HandleMapEvidence evidence={0};
    if(!objects||!entries||!count)return evidence;
    const size_t samples=count<512u?count:512u;
    for(size_t k=0u;k<samples;k++){
        const size_t at=samples==1u?0u:
            (k*(count-1u))/(samples-1u);
        const MusaDwgAc1032HandleEntry *entry=&entries[at];
        MusaDwgAc1032PayloadInfo info={0};
        size_t needed=0u,copied=0u;
        if(!musa_dwg_ac1032_object_payload_bytes(
             objects,object_bytes,entry->handle,entry->object_offset,
             NULL,0u,&needed,&info)||!info.original_record_crc_verified||
           !needed||needed>object_bytes||info.object_type_field_bits!=10u)
            continue;
        const uint16_t type=info.raw_object_type;
        if(type!=1u&&type!=2u&&type!=3u&&type!=4u&&type!=5u&&
           type!=7u&&type!=17u&&type!=18u&&type!=19u&&
           type!=27u&&type!=35u&&type!=44u&&type!=77u)continue;
        uint8_t *raw=(uint8_t*)malloc(needed);
        if(!raw)continue;
        MusaDwgAc1032PayloadInfo copied_info={0};
        int ok=musa_dwg_ac1032_object_payload_bytes(
              objects,object_bytes,entry->handle,entry->object_offset,
              raw,needed,&copied,&copied_info)&&copied==needed&&
              copied_info.original_record_crc_verified;
        if(ok){
            MusaDwgAc1015Bits bits={0};
            uint32_t header=0u;
            ok=musa_dwg_ac1015_bits_init(
                 &bits,raw,needed,10u,needed*8u)&&
               musa_dwg_ac1015_bits_read_u(&bits,8u,&header)&&
               !(header>>4u)&&(header&15u)>0u&&(header&15u)<=8u;
            uint64_t self=0u;
            for(unsigned j=0u;ok&&j<(header&15u);j++){
                uint32_t byte=0u;
                ok=musa_dwg_ac1015_bits_read_u(&bits,8u,&byte);
                if(ok)self=(self<<8u)|byte;
            }
            if(ok){
                if(self==entry->handle)evidence.exact++;
                else evidence.foreign++;
            }
        }
        free(raw);
    }
    return evidence;
}
int musa_dwg_ac1032_handles_index(
    const uint8_t *file,size_t file_bytes,
    MusaDwgAc1032HandleEntry *entries,size_t capacity,
    size_t *out_count){
    if(!file||!out_count||file_bytes<256u||
       file_bytes>MUSA_AC1032_HANDLE_FILE_MAX||
       memcmp(file,"AC1032",6u)!=0||
       (!entries&&capacity)||
       overlap(file,file_bytes,out_count,sizeof(*out_count)))
        return 0;
    if(entries){
        if(capacity>SIZE_MAX/sizeof(*entries))return 0;
        const size_t output_bytes=capacity*sizeof(*entries);
        if(overlap(file,file_bytes,entries,output_bytes)||
           overlap(entries,output_bytes,out_count,sizeof(*out_count)))
            return 0;
    }

    /* Full authenticated original directory: do not trust a single
     * metadata field to validate an untrusted modern drawing. */
    size_t catalog_count=0u;
    if(!musa_dwg_ac1032_section_catalog(
          file,file_bytes,NULL,0u,&catalog_count)||
       !catalog_count||catalog_count>256u)return 0;
    MusaDwgAc1032SectionCatalogEntry *sections=
        calloc(catalog_count,sizeof(*sections));
    if(!sections)return 0;
    size_t got_catalog=0u;
    int ok=musa_dwg_ac1032_section_catalog(
         file,file_bytes,sections,catalog_count,&got_catalog)&&
         got_catalog==catalog_count;
    size_t object_bytes=0u,handle_bytes=0u;
    unsigned objects_found=0u,handles_found=0u;
    if(ok)for(size_t i=0u;i<catalog_count;i++){
        const MusaDwgAc1032SectionCatalogEntry *e=&sections[i];
        if(!e->all_physical_page_checksums_verified){ok=0;break;}
        if(!strcmp((const char*)e->name,"AcDb:AcDbObjects")){
            objects_found++;
            if(e->logical_bytes==0u||
               e->logical_bytes>MUSA_AC1032_SECTION_MAX)ok=0;
            else object_bytes=(size_t)e->logical_bytes;
        }else if(!strcmp((const char*)e->name,"AcDb:Handles")){
            handles_found++;
            if(e->logical_bytes<8u||
               e->logical_bytes>MUSA_AC1032_SECTION_MAX)ok=0;
            else handle_bytes=(size_t)e->logical_bytes;
        }
    }
    free(sections);
    if(!ok||objects_found!=1u||handles_found!=1u)return 0;

    /* Decode BOTH physical sections. An authenticated page directory
     * without actual valid LZ logical content is not sufficient. */
    uint8_t *objects=malloc(object_bytes);
    uint8_t *handles=malloc(handle_bytes);
    MusaDwgAc1032HandleEntry *temporary=NULL,*alternate=NULL;
    size_t object_got=0u,handle_got=0u,needed=0u;
    ok=0;
    if(!objects||!handles)goto done;
    if(!musa_dwg_ac1032_extract_named_section(file,file_bytes,
          "AcDb:AcDbObjects",objects,object_bytes,&object_got)||
       object_got!=object_bytes||
       !musa_dwg_ac1032_extract_named_section(file,file_bytes,
          "AcDb:Handles",handles,handle_bytes,&handle_got)||
       handle_got!=handle_bytes)goto done;
    size_t legacy_count=0u,continuous_count=0u,absolute_count=0u;
    const int legacy_ok=musa_dwg_ac1018_handles_index(
        handles,handle_bytes,object_bytes,NULL,0u,&legacy_count);
    const int continuous_ok=
        musa_dwg_ac1018_handles_index_continuous_offsets(
        handles,handle_bytes,object_bytes,NULL,0u,&continuous_count);
    const int absolute_ok=
        musa_dwg_ac1018_handles_index_page_absolute_handles(
        handles,handle_bytes,object_bytes,NULL,0u,&absolute_count);
    if(!legacy_ok&&!continuous_ok&&!absolute_ok)goto done;
    needed=legacy_ok?legacy_count:(continuous_ok?continuous_count:absolute_count);
    if(!needed||needed>MUSA_AC1032_HANDLES_MAX||
       needed>SIZE_MAX/sizeof(*entries)||
       (legacy_ok&&continuous_ok&&legacy_count!=continuous_count)||
       (legacy_ok&&absolute_ok&&legacy_count!=absolute_count)||
       (!legacy_ok&&continuous_ok&&absolute_ok&&continuous_count!=absolute_count))
        goto done;
    if(entries){
        if(capacity<needed)goto done;
        temporary=malloc(needed*sizeof(*temporary));
        if(!temporary)goto done;
        size_t filled=0u;
        if(!(legacy_ok?musa_dwg_ac1018_handles_index(
              handles,handle_bytes,object_bytes,
              temporary,needed,&filled):
              (continuous_ok?
               musa_dwg_ac1018_handles_index_continuous_offsets(
                   handles,handle_bytes,object_bytes,
                   temporary,needed,&filled):
               musa_dwg_ac1018_handles_index_page_absolute_handles(
                   handles,handle_bytes,object_bytes,
                   temporary,needed,&filled)))||filled!=needed)goto done;
        /* Only switch to the continuous profile when independent original
         * CRC-backed entity self handles positively prove it and the
         * legacy profile is demonstrably mismatched. Otherwise preserve
         * the established profile. Never synthesize a missing handle. */
        if(legacy_ok&&continuous_ok){
            alternate=malloc(needed*sizeof(*alternate));
            if(!alternate)goto done;
            size_t alt_count=0u;
            if(musa_dwg_ac1018_handles_index_continuous_offsets(
                   handles,handle_bytes,object_bytes,
                   alternate,needed,&alt_count)&&alt_count==needed){
                const HandleMapEvidence a=handle_map_evidence(
                    objects,object_bytes,temporary,needed);
                const HandleMapEvidence b=handle_map_evidence(
                    objects,object_bytes,alternate,needed);
                if(b.exact>=4u&&b.foreign==0u&&
                   b.exact>a.exact&&a.foreign>0u){
                    MusaDwgAc1032HandleEntry *swap=temporary;
                    temporary=alternate;
                    alternate=swap;
                }
            }
        }
        /* Another CRC-checked candidate: each page starts with an
         * absolute handle. Only select with positive embedded-self
         * evidence and demonstrable mismatch in the previous profile. */
        if(absolute_ok){
            if(!alternate){
                alternate=malloc(needed*sizeof(*alternate));
                if(!alternate)goto done;
            }
            size_t alt_count=0u;
            if(musa_dwg_ac1018_handles_index_page_absolute_handles(
                   handles,handle_bytes,object_bytes,
                   alternate,needed,&alt_count)&&alt_count==needed){
                const HandleMapEvidence a=handle_map_evidence(
                    objects,object_bytes,temporary,needed);
                const HandleMapEvidence b=handle_map_evidence(
                    objects,object_bytes,alternate,needed);
                if(b.exact>=4u&&b.foreign==0u&&
                   b.exact>a.exact&&a.foreign>0u){
                    MusaDwgAc1032HandleEntry *swap=temporary;
                    temporary=alternate;
                    alternate=swap;
                }
            }
        }
        memcpy(entries,temporary,needed*sizeof(*entries));
    }
    *out_count=needed;
    ok=1;
done:
    free(alternate);
    free(temporary);
    free(handles);
    free(objects);
    return ok;
}
