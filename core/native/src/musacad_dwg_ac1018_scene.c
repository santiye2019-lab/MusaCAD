/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_scene.h"
#include "musacad_dwg_ac1018_sections.h"
#include "musacad_dwg_ac1018_handles.h"
#include "musacad_dwg_ac1018_reactor_links.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define MUSA_AC1018_SCENE_MAX_HANDLES 1000000u
struct MusaDwgAc1018Scene {
    uint8_t *objects;
    size_t object_bytes;
    MusaDwgAc1018HandleEntry *index;
    size_t index_count;
};
static int overlaps(const void *a,size_t a_bytes,
                    const void *b,size_t b_bytes){
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return a_bytes&&b_bytes&&
           ((x<=y&&y-x<a_bytes)||(y<x&&x-y<b_bytes));
}
void musa_dwg_ac1018_scene_close(MusaDwgAc1018Scene *s){
    if(!s)return;
    free(s->index);
    free(s->objects);
    free(s);
}
int musa_dwg_ac1018_scene_open(const uint8_t *file,size_t file_bytes,
                              MusaDwgAc1018Scene **out){
    if(!file||!out||file_bytes<256u||
       overlaps(file,file_bytes,out,sizeof(*out)))return 0;
    MusaDwgAc1018SectionDescriptor obj={0},hmap={0};
    size_t obj_pages=0u,handle_pages=0u;
    if(!musa_dwg_ac1018_section_pages(file,file_bytes,
           MUSA_DWG_AC1018_OBJECTS,NULL,0u,&obj,&obj_pages)||
       !musa_dwg_ac1018_section_pages(file,file_bytes,
           MUSA_DWG_AC1018_HANDLES,NULL,0u,&hmap,&handle_pages)||
       !obj_pages||!handle_pages||!obj.logical_bytes||
       !hmap.logical_bytes||obj.logical_bytes>SIZE_MAX||
       hmap.logical_bytes>SIZE_MAX)return 0;
    const size_t obj_len=(size_t)obj.logical_bytes;
    const size_t map_len=(size_t)hmap.logical_bytes;
    MusaDwgAc1018Scene *tmp=
        (MusaDwgAc1018Scene*)calloc(1u,sizeof(*tmp));
    uint8_t *handles=(uint8_t*)malloc(map_len);
    int ok=0;
    if(!tmp||!handles)goto done;
    tmp->objects=(uint8_t*)malloc(obj_len);
    if(!tmp->objects)goto done;
    size_t expanded=0u;
    if(!musa_dwg_ac1018_extract_section(file,file_bytes,
              MUSA_DWG_AC1018_OBJECTS,tmp->objects,obj_len,&expanded)||
       expanded!=obj_len||
       !musa_dwg_ac1018_extract_section(file,file_bytes,
              MUSA_DWG_AC1018_HANDLES,handles,map_len,&expanded)||
       expanded!=map_len)goto done;
    size_t count=0u;
    if(!musa_dwg_ac1018_handles_index(handles,map_len,obj_len,
          NULL,0u,&count)||!count||
       count>MUSA_AC1018_SCENE_MAX_HANDLES||
       count>SIZE_MAX/sizeof(*tmp->index))goto done;
    tmp->index=(MusaDwgAc1018HandleEntry*)malloc(
                 count*sizeof(*tmp->index));
    if(!tmp->index||
       !musa_dwg_ac1018_handles_index(handles,map_len,obj_len,
                     tmp->index,count,&expanded)||
       expanded!=count)goto done;
    tmp->object_bytes=obj_len;
    tmp->index_count=count;
    *out=tmp;ok=1;
done:
    free(handles);
    if(!ok)musa_dwg_ac1018_scene_close(tmp);
    return ok;
}
size_t musa_dwg_ac1018_scene_handle_count(const MusaDwgAc1018Scene *s){
    return s?s->index_count:0u;
}
/* Stable, sorted 64-bit self handles from CRC-checked page deltas. */
static int get_offset(const MusaDwgAc1018Scene *s,uint64_t handle,
                      size_t *offset){
    if(!s||!s->objects||!s->index||!s->index_count||
       !handle||!offset)return 0;
    size_t l=0u,r=s->index_count;
    while(l<r){
        const size_t m=l+(r-l)/2u;
        if(s->index[m].handle<handle)l=m+1u;
        else r=m;
    }
    if(l>=s->index_count||s->index[l].handle!=handle||
       s->index[l].object_offset>=s->object_bytes)return 0;
    *offset=(size_t)s->index[l].object_offset;
    return 1;
}
int musa_dwg_ac1018_scene_read_line(const MusaDwgAc1018Scene *s,
      uint64_t handle,MusaDwgAc1018Line *out){
    if(!out)return 0;
    size_t at=0u;
    if(!get_offset(s,handle,&at))return 0;
    return musa_dwg_ac1018_read_line_record(s->objects,
                                s->object_bytes,at,handle,out);
}
int musa_dwg_ac1018_scene_read_lwpolyline(
      const MusaDwgAc1018Scene *s,uint64_t handle,
      MusaDwgAc1018LWVertex *vertices,size_t capacity,
      MusaDwgAc1018LWPolyline *out){
    if(!vertices||!out||!capacity||capacity>100000u||
       capacity>SIZE_MAX/sizeof(*vertices)||
       overlaps(vertices,capacity*sizeof(*vertices),out,sizeof(*out)))
       return 0;
    size_t at=0u;
    if(!get_offset(s,handle,&at))return 0;
    return musa_dwg_ac1018_read_lwpolyline_record(s->objects,
                            s->object_bytes,at,handle,vertices,capacity,out);
}


/* An indexed H target's presence is NOT proof of its DWG CLASSES role.
 * Existing strict common-handle-tail scanner supplies the only acceptance
 * path for R2004 link membership (zero or positive reactors).
 * Temporary map materialization is explicit; no unsafe struct aliasing. */
int musa_dwg_ac1018_scene_audit_links(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    uint64_t *reactor_targets,size_t capacity,
    MusaDwgAc1018ReactorAudit *out){
    if(!s||!out||!handle||capacity>64u||
       (capacity&&!reactor_targets)||
       s->index_count>SIZE_MAX/sizeof(MusaDwgAc1015HandleEntry))
       return 0;
    if(overlaps(s->objects,s->object_bytes,out,sizeof(*out))||
       overlaps(s->index,s->index_count*sizeof(*s->index),
                out,sizeof(*out))||
       (capacity&&(overlaps(s->objects,s->object_bytes,
                           reactor_targets,capacity*sizeof(*reactor_targets))||
                   overlaps(s->index,s->index_count*sizeof(*s->index),
                            reactor_targets,capacity*sizeof(*reactor_targets))||
                   overlaps(out,sizeof(*out),reactor_targets,
                            capacity*sizeof(*reactor_targets)))))
        return 0;
    size_t offset=0u;
    if(!get_offset(s,handle,&offset))return 0;
    MusaDwgAc1015HandleEntry *converted=
        (MusaDwgAc1015HandleEntry*)malloc(
             s->index_count*sizeof(*converted));
    if(!converted)return 0;
    for(size_t i=0u;i<s->index_count;i++){
        converted[i].handle=s->index[i].handle;
        converted[i].file_offset=s->index[i].object_offset;
    }
    MusaDwgAc1018ReactorAudit checked={0};
    uint64_t found[64]={0};
    int ok=musa_dwg_ac1018_read_zero_reactor_links(
           s->objects,s->object_bytes,offset,handle,
           converted,s->index_count,&checked);
    if(!ok)ok=musa_dwg_ac1018_read_reactor_links(
           s->objects,s->object_bytes,offset,handle,
           converted,s->index_count,found,64u,&checked);
    if(ok){
        if(checked.reactor_count>capacity||
           (checked.reactor_count&&!reactor_targets))ok=0;
        else{
            if(checked.reactor_count)
                memcpy(reactor_targets,found,
                       (size_t)checked.reactor_count*sizeof(found[0]));
            *out=checked;
        }
    }
    free(converted);
    return ok;
}
