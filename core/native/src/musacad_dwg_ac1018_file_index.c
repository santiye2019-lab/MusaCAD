/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_file_index.h"
#include "musacad_dwg_ac1018_sections.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define MUSA_FILE_MAX_HANDLES 1000000u
static int overlaps(const void *first,size_t first_len,
                    const void *second,size_t second_len){
    const uintptr_t a=(uintptr_t)first,b=(uintptr_t)second;
    return first_len&&second_len&&
           ((a<=b && b-a<first_len)||(b<a && a-b<second_len));
}
int musa_dwg_ac1018_file_handle_index(const uint8_t *file,
    size_t file_bytes,MusaDwgAc1018HandleEntry *entries,
    size_t capacity,size_t *out_count){
    if(!file||!out_count||file_bytes<256u||(!entries&&capacity))
        return 0;
    MusaDwgAc1018SectionDescriptor object_info={0},handle_info={0};
    size_t pages=0u,handles_pages=0u;
    if(!musa_dwg_ac1018_section_pages(file,file_bytes,
           MUSA_DWG_AC1018_OBJECTS,NULL,0u,&object_info,&pages)||
       !musa_dwg_ac1018_section_pages(file,file_bytes,
           MUSA_DWG_AC1018_HANDLES,NULL,0u,&handle_info,&handles_pages)||
       !pages||!handles_pages||!object_info.logical_bytes||
       !handle_info.logical_bytes||object_info.logical_bytes>SIZE_MAX||
       handle_info.logical_bytes>SIZE_MAX)return 0;
    const size_t object_bytes=(size_t)object_info.logical_bytes;
    const size_t handle_bytes=(size_t)handle_info.logical_bytes;
    uint8_t *objects=(uint8_t*)malloc(object_bytes);
    uint8_t *handles=(uint8_t*)malloc(handle_bytes);
    MusaDwgAc1018HandleEntry *temporary=NULL;
    size_t produced_objects=0u,produced_handles=0u,needed=0u;
    int ok=0;
    if(!objects||!handles)goto done;
    /* Decode both sections fully; the index must never be accepted
     * solely because its metadata announces a plausible length. */
    if(!musa_dwg_ac1018_extract_section(file,file_bytes,
        MUSA_DWG_AC1018_OBJECTS,objects,object_bytes,&produced_objects)||
       produced_objects!=object_bytes||
       !musa_dwg_ac1018_extract_section(file,file_bytes,
        MUSA_DWG_AC1018_HANDLES,handles,handle_bytes,&produced_handles)||
       produced_handles!=handle_bytes||
       !musa_dwg_ac1018_handles_index(handles,handle_bytes,
                    object_bytes,NULL,0u,&needed)||
       !needed||needed>MUSA_FILE_MAX_HANDLES||
       needed>SIZE_MAX/sizeof(*entries))goto done;
    if(entries){
        const size_t bytes=needed*sizeof(*entries);
        if(needed>capacity||overlaps(file,file_bytes,entries,bytes))
            goto done;
        temporary=(MusaDwgAc1018HandleEntry*)malloc(bytes);
        if(!temporary)goto done;
        size_t again=0u;
        if(!musa_dwg_ac1018_handles_index(handles,handle_bytes,
             object_bytes,temporary,needed,&again)||again!=needed)
             goto done;
        memcpy(entries,temporary,bytes);
    }
    *out_count=needed;
    ok=1;
done:
    free(temporary);
    free(objects);
    free(handles);
    return ok;
}
