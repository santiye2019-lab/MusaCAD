/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_file_line.h"
#include "musacad_dwg_ac1018_sections.h"
#include "musacad_dwg_ac1018_handles.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#define MUSA_FILE_LINE_MAX_HANDLES 1000000u
static int intersects(const void *a,size_t na,const void *b,size_t nb){
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return na&&nb&&((x<=y&&y-x<na)||(y<x&&x-y<nb));
}
int musa_dwg_ac1018_file_read_line(const uint8_t *file,size_t file_bytes,
    uint64_t handle,MusaDwgAc1018Line *out){
    if(!file||!out||!handle||file_bytes<256u||
       intersects(file,file_bytes,out,sizeof(*out)))return 0;
    MusaDwgAc1018SectionDescriptor obj={0},hmap={0};
    size_t np=0u,nh=0u;
    if(!musa_dwg_ac1018_section_pages(file,file_bytes,
           MUSA_DWG_AC1018_OBJECTS,NULL,0u,&obj,&np)||
       !musa_dwg_ac1018_section_pages(file,file_bytes,
           MUSA_DWG_AC1018_HANDLES,NULL,0u,&hmap,&nh)||
       !np||!nh||obj.logical_bytes==0u||hmap.logical_bytes==0u||
       obj.logical_bytes>SIZE_MAX||hmap.logical_bytes>SIZE_MAX)return 0;
    const size_t obj_len=(size_t)obj.logical_bytes;
    const size_t map_len=(size_t)hmap.logical_bytes;
    uint8_t *objects=(uint8_t*)malloc(obj_len);
    uint8_t *map=(uint8_t*)malloc(map_len);
    MusaDwgAc1018HandleEntry *entries=NULL;
    int ok=0;
    if(!objects||!map)goto done;
    size_t decoded=0u;
    if(!musa_dwg_ac1018_extract_section(file,file_bytes,
           MUSA_DWG_AC1018_OBJECTS,objects,obj_len,&decoded)||
       decoded!=obj_len||
       !musa_dwg_ac1018_extract_section(file,file_bytes,
           MUSA_DWG_AC1018_HANDLES,map,map_len,&decoded)||
       decoded!=map_len)goto done;
    size_t count=0u;
    if(!musa_dwg_ac1018_handles_index(map,map_len,obj_len,
         NULL,0u,&count)||!count||count>MUSA_FILE_LINE_MAX_HANDLES||
       count>SIZE_MAX/sizeof(*entries))goto done;
    entries=(MusaDwgAc1018HandleEntry*)malloc(count*sizeof(*entries));
    if(!entries||!musa_dwg_ac1018_handles_index(map,map_len,obj_len,
         entries,count,&decoded)||decoded!=count)goto done;
    /* Handle deltas are strictly positive. Binary search avoids an O(N)
     * scan of all indexed objects for a selected LINE record. */
    size_t lo=0u,hi=count;
    while(lo<hi){
        const size_t mid=lo+(hi-lo)/2u;
        if(entries[mid].handle<handle)lo=mid+1u;
        else hi=mid;
    }
    if(lo>=count||entries[lo].handle!=handle||
       entries[lo].object_offset>SIZE_MAX)goto done;
    MusaDwgAc1018Line checked={0};
    if(!musa_dwg_ac1018_read_line_record(objects,obj_len,
         (size_t)entries[lo].object_offset,handle,&checked))goto done;
    *out=checked;ok=1;
done:
    free(entries);free(map);free(objects);return ok;
}
