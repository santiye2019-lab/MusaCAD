/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_line_file.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1032_object_streams.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define MUSA_AC1032_FILE_MAX (256u*1024u*1024u)
#define MUSA_AC1032_PAYLOAD_MAX (64u*1024u*1024u)
static int overlap(const void *a,size_t an,const void *b,size_t bn){
    if(!an||!bn)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<an:x-y<bn;
}
int musa_dwg_ac1032_read_line_file(
    const uint8_t *file,size_t file_bytes,uint64_t indexed_handle,
    MusaDwgAc1032NativeLine *out){
    if(!file||!out||!indexed_handle||file_bytes<256u||
       file_bytes>MUSA_AC1032_FILE_MAX||
       overlap(file,file_bytes,out,sizeof(*out))||
       memcmp(file,"AC1032",6u)!=0)return 0;

    /* All first-party authenticated original-section operations run on the
     * caller's immutable DWG: never rewrite source, never skip unknown data.
     * No caller-supplied or plausible-looking geometry_start_bit exists. */
    size_t needed=0u;
    MusaDwgAc1032PayloadInfo meta={0};
    if(!musa_dwg_ac1032_object_payload_file(
            file,file_bytes,indexed_handle,NULL,0u,&needed,&meta)||
       !meta.original_record_crc_verified||
       meta.indexed_handle!=indexed_handle||
       meta.raw_object_type!=19u||
       meta.object_type_field_bits!=10u||
       needed==0u||needed>MUSA_AC1032_PAYLOAD_MAX||
       needed!=meta.payload_bytes)return 0;

    MusaDwgAc1032ObjectStreams layout={0};
    if(!musa_dwg_ac1032_stream_layout_file(
            file,file_bytes,indexed_handle,&layout)||
       !layout.original_record_crc_verified||
       layout.indexed_handle!=indexed_handle||
       layout.object_section_offset!=meta.object_section_offset||
       layout.raw_object_type!=meta.raw_object_type||
       layout.object_type_field_bits!=meta.object_type_field_bits||
       layout.total_payload_bits!=(uint64_t)needed*8u||
       layout.handle_stream_bits!=meta.handle_stream_bits)return 0;

    uint8_t *payload=(uint8_t*)malloc(needed);
    if(!payload)return 0;
    size_t copied_bytes=0u;
    MusaDwgAc1032PayloadInfo copied={0};
    MusaDwgAc1032NativeLine candidate={0};
    int ok=musa_dwg_ac1032_object_payload_file(
          file,file_bytes,indexed_handle,payload,needed,
          &copied_bytes,&copied)&&
        copied_bytes==needed&&
        copied.indexed_handle==meta.indexed_handle&&
        copied.object_section_offset==meta.object_section_offset&&
        copied.payload_bytes==meta.payload_bytes&&
        copied.handle_stream_bits==meta.handle_stream_bits&&
        copied.raw_object_type==19u&&copied.original_record_crc_verified&&
        musa_dwg_ac1032_line_common_bytes(
          payload,needed,&layout,&candidate.common)&&
        candidate.common.common_decoded&&
        candidate.common.prefix.indexed_handle==indexed_handle&&
        musa_dwg_ac1032_line_fields_at(
          payload,needed,&layout,candidate.common.geometry_start_bit,
          &candidate.geometry)&&
        candidate.geometry.coordinates_decoded&&
        candidate.geometry.indexed_handle==indexed_handle&&
        candidate.geometry.geometry_start_bit==
            (uint64_t)candidate.common.geometry_start_bit&&
        candidate.geometry.geometry_end_bit<=layout.data_stream_bits;
    free(payload);
    if(!ok)return 0;
    *out=candidate;
    return 1;
}
