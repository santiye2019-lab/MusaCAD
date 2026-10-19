/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_scene.h"
#include "musacad_dwg_ac1018_sections.h"
#include "musacad_dwg_ac1018_handles.h"
#include "musacad_dwg_ac1018_reactor_links.h"
#include "musacad_dwg_ac1018_reactor_links_internal.h"
#include "musacad_dwg_ac1015_values.h"
#include "musacad_crc16.h"
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

/* Record probe intentionally does not infer class, owner, layer or geometry.
 * A handle-index entry is not proof the referenced record self-ID matches:
 * dedicated geometry readers still validate their own self handles. */
int musa_dwg_ac1018_scene_probe_record_at(
    const MusaDwgAc1018Scene *s,size_t ordinal,
    MusaDwgAc1018RecordProbe *out){
    if(!s||!out||!s->objects||!s->index||
       ordinal>=s->index_count)return 0;
    const MusaDwgAc1018HandleEntry *entry=&s->index[ordinal];
    if(!entry->handle||entry->object_offset>=s->object_bytes)return 0;
    const size_t offset=(size_t)entry->object_offset;
    const size_t n=s->object_bytes-offset;
    const uint8_t *p=s->objects+offset;
    if(n<4u)return 0;
    const uint16_t first=(uint16_t)p[0]|((uint16_t)p[1]<<8u);
    size_t head=2u,payload=0u;
    if((first&UINT16_C(0x8000))==0u){
        payload=first;
    }else{
        if(n<6u)return 0;
        const uint16_t second=(uint16_t)p[2]|((uint16_t)p[3]<<8u);
        if(!second||(second&UINT16_C(0x8000)))return 0;
        payload=(size_t)(first&UINT16_C(0x7fff))|
                ((size_t)second<<15u);
        head=4u;
    }
    if(payload<8u||n-head<2u||payload>n-head-2u||
       payload>SIZE_MAX/8u)return 0;
    uint16_t computed=0u;
    if(!musa_core_crc16_arc(0xc0c1u,p,head+payload,&computed))
        return 0;
    const size_t crc_at=head+payload;
    const uint16_t stored=(uint16_t)p[crc_at]|
                          ((uint16_t)p[crc_at+1u]<<8u);
    if(stored!=computed)return 0;
    MusaDwgAc1015Bits bits={0};
    uint16_t declared=0u;
    if(!musa_dwg_ac1015_bits_init(&bits,p+head,payload,0u,
                                 payload*8u)||
       !musa_dwg_ac1015_bits_read_bs(&bits,&declared))return 0;
    MusaDwgAc1018RecordProbe checked={0};
    checked.handle=entry->handle;
    checked.object_offset=entry->object_offset;
    checked.record_bytes=head+payload+2u;
    checked.declared_type=declared;
    checked.record_crc_verified=1u;
    *out=checked;
    return 1;
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
int musa_dwg_ac1018_scene_read_curve(const MusaDwgAc1018Scene *s,
      uint64_t handle,MusaDwgAc1018Curve *out){
    if(!out)return 0;
    size_t at=0u;
    if(!get_offset(s,handle,&at))return 0;
    return musa_dwg_ac1018_read_curve_record(s->objects,
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
 * The validated scene's original CRC-backed R2004 index is immutable.
 * Reuse it directly: no allocation, no O(N) conversion/verification on
 * each H role query; individual roles use binary-search membership. */
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
    MusaDwgAc1018ReactorAudit checked={0};
    uint64_t found[64]={0};
    int ok=musa_dwg_ac1018_read_links_verified_scene_index(
           s->objects,s->object_bytes,offset,handle,
           s->index,s->index_count,found,64u,&checked,1);
    if(!ok)ok=musa_dwg_ac1018_read_links_verified_scene_index(
           s->objects,s->object_bytes,offset,handle,
           s->index,s->index_count,found,64u,&checked,0);
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
    return ok;
}
