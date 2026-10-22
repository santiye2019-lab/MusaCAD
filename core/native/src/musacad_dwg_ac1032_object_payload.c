/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1032_object_type.h"
#include "musacad_dwg_ac1032_record.h"
#include "musacad_dwg_ac1032_sections.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define PAYLOAD_MAX_FILE (64u*1024u*1024u)
#define PAYLOAD_MAX_CATALOG 256u
static int overlap(const void *a,size_t an,const void *b,size_t bn){
    if(!an||!bn)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<an:x-y<bn;
}
static int valid_outputs(const uint8_t *source,size_t bytes,
    uint8_t *dst,size_t cap,size_t *required,
    MusaDwgAc1032PayloadInfo *info){
    if(!required||!info||(!dst&&cap)||
       (dst&&(!cap||cap>PAYLOAD_MAX_FILE))||
       overlap(source,bytes,required,sizeof(*required))||
       overlap(source,bytes,info,sizeof(*info))||
       overlap(required,sizeof(*required),info,sizeof(*info)))
        return 0;
    if(dst&&(overlap(source,bytes,dst,cap)||
             overlap(dst,cap,required,sizeof(*required))||
             overlap(dst,cap,info,sizeof(*info))))return 0;
    return 1;
}
int musa_dwg_ac1032_object_payload_bytes(
    const uint8_t *objects,size_t objects_size,
    uint64_t handle,uint64_t offset,uint8_t *dst,size_t cap,
    size_t *required,MusaDwgAc1032PayloadInfo *info){
    if(!objects||!handle||objects_size<8u||
       objects_size>PAYLOAD_MAX_FILE||offset>=objects_size||
       !valid_outputs(objects,objects_size,dst,cap,required,info))return 0;
    MusaDwgAc1032ObjectType typed={0};
    MusaDwgAc1032RecordEnvelope record={0};
    if(!musa_dwg_ac1032_object_type_bytes(
          objects,objects_size,handle,offset,&typed)||
       !musa_dwg_ac1032_record_envelope_bytes(
          objects,objects_size,handle,offset,&record)||
       !typed.record_crc_verified||!record.record_crc_verified||
       typed.indexed_handle!=record.indexed_handle||
       typed.object_section_offset!=record.object_section_offset||
       record.payload_bytes<2u)return 0;
    const size_t at=(size_t)offset;
    const size_t start=at+record.ms_field_bytes+record.mc_field_bytes;
    if(start>objects_size||record.payload_bytes>objects_size-start||
       (dst&&cap<record.payload_bytes))return 0;
    MusaDwgAc1032PayloadInfo value={0};
    value.indexed_handle=handle;
    value.object_section_offset=offset;
    value.payload_bytes=record.payload_bytes;
    value.handle_stream_bits=record.handle_stream_bits;
    value.raw_object_type=typed.type_code;
    value.object_type_field_bits=typed.type_field_bits;
    value.original_record_crc_verified=1u;
    /* No further failure after this point: commit outputs atomically. */
    if(dst)memcpy(dst,objects+start,record.payload_bytes);
    *required=record.payload_bytes;
    *info=value;
    return 1;
}
int musa_dwg_ac1032_object_payload_file(
    const uint8_t *file,size_t file_bytes,uint64_t handle,
    uint8_t *dst,size_t cap,size_t *required,
    MusaDwgAc1032PayloadInfo *info){
    if(!file||!handle||file_bytes<256u||file_bytes>PAYLOAD_MAX_FILE||
       memcmp(file,"AC1032",6u)!=0||
       !valid_outputs(file,file_bytes,dst,cap,required,info))return 0;
    /* This source-backed entrypoint first authenticates the complete
     * original physical directory and original handles through the
     * established portable R2018 object type file reader. */
    MusaDwgAc1032ObjectType typed={0};
    if(!musa_dwg_ac1032_object_type_file(
          file,file_bytes,handle,&typed)||!typed.record_crc_verified)
        return 0;
    size_t n=0u;
    if(!musa_dwg_ac1032_section_catalog(
        file,file_bytes,NULL,0u,&n)||!n||n>PAYLOAD_MAX_CATALOG)
        return 0;
    MusaDwgAc1032SectionCatalogEntry *entries=calloc(n,sizeof(*entries));
    if(!entries)return 0;
    size_t got=0u,bytes=0u,match=0u;
    int ok=musa_dwg_ac1032_section_catalog(
              file,file_bytes,entries,n,&got)&&got==n;
    if(ok)for(size_t i=0u;i<n;i++){
        if(!strcmp((const char*)entries[i].name,"AcDb:AcDbObjects")){
            match++;
            if(!entries[i].all_physical_page_checksums_verified||
               !entries[i].logical_bytes||
               entries[i].logical_bytes>PAYLOAD_MAX_FILE)ok=0;
            else bytes=(size_t)entries[i].logical_bytes;
        }
    }
    free(entries);
    if(!ok||match!=1u)return 0;
    uint8_t *original=malloc(bytes);
    if(!original)return 0;
    size_t read=0u,needed=0u;
    MusaDwgAc1032PayloadInfo candidate={0};
    /* Copy goes into the caller only AFTER all consistency checks;
     * no destination mutations if a stage fails. */
    uint8_t *temporary=NULL;
    if(dst){temporary=malloc(cap);if(!temporary){free(original);return 0;}}
    ok=musa_dwg_ac1032_extract_named_section(
             file,file_bytes,"AcDb:AcDbObjects",original,bytes,&read)&&
       read==bytes&&
       musa_dwg_ac1032_object_payload_bytes(
             original,bytes,handle,typed.object_section_offset,
             temporary,cap,&needed,&candidate)&&
       candidate.raw_object_type==typed.type_code&&
       candidate.object_type_field_bits==typed.type_field_bits&&
       candidate.object_section_offset==typed.object_section_offset&&
       candidate.indexed_handle==typed.indexed_handle;
    if(ok){
        if(dst)memcpy(dst,temporary,needed);
        *required=needed;
        *info=candidate;
    }
    free(temporary);
    free(original);
    return ok;
}
