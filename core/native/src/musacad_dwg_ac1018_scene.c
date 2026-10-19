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
/* Reject caller outputs overlapping the opaque scene header or its cached
 * verified object/index data. Every exposed read remains read-only even if
 * the caller passes a pointer into previously returned session memory. */
static int scene_output_overlaps(const MusaDwgAc1018Scene *s,
                                 const void *dst,size_t bytes){
    if(!s||!s->objects||!s->index||
       s->index_count>SIZE_MAX/sizeof(*s->index))return 1;
    return overlaps(s,sizeof(*s),dst,bytes)||
           overlaps(s->objects,s->object_bytes,dst,bytes)||
           overlaps(s->index,s->index_count*sizeof(*s->index),dst,bytes);
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
    if(!s||!out||ordinal>=s->index_count||
       scene_output_overlaps(s,out,sizeof(*out)))return 0;
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

/* Return independent record envelope evidence for an indexed handle.
 * This is not a self-H/class check; no guessed object role admission. */
int musa_dwg_ac1018_scene_probe_record_handle(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    MusaDwgAc1018RecordProbe *out){
    if(!s||!out||!handle||
       scene_output_overlaps(s,out,sizeof(*out)))return 0;
    size_t lo=0u,hi=s->index_count;
    while(lo<hi){
        const size_t mid=lo+(hi-lo)/2u;
        if(s->index[mid].handle<handle)lo=mid+1u;
        else hi=mid;
    }
    if(lo>=s->index_count||s->index[lo].handle!=handle)return 0;
    return musa_dwg_ac1018_scene_probe_record_at(s,lo,out);
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
int musa_dwg_ac1018_scene_probe_nonentity_identity(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    MusaDwgAc1018NonEntityIdentity *out){
    if(!s||!out||!handle||!s->objects||!s->index||
       overlaps(s,sizeof(*s),out,sizeof(*out))||
       overlaps(s->objects,s->object_bytes,out,sizeof(*out))||
       overlaps(s->index,s->index_count*sizeof(*s->index),out,sizeof(*out)))
       return 0;
    size_t offset=0u;
    if(!get_offset(s,handle,&offset))return 0;
    return musa_dwg_ac1018_nonentity_identity_record(
               s->objects,s->object_bytes,offset,handle,out);
}
int musa_dwg_ac1018_scene_read_line(const MusaDwgAc1018Scene *s,
      uint64_t handle,MusaDwgAc1018Line *out){
    if(!out||scene_output_overlaps(s,out,sizeof(*out)))return 0;
    size_t at=0u;
    if(!get_offset(s,handle,&at))return 0;
    return musa_dwg_ac1018_read_line_record(s->objects,
                                s->object_bytes,at,handle,out);
}
int musa_dwg_ac1018_scene_read_curve(const MusaDwgAc1018Scene *s,
      uint64_t handle,MusaDwgAc1018Curve *out){
    if(!out||scene_output_overlaps(s,out,sizeof(*out)))return 0;
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
       overlaps(vertices,capacity*sizeof(*vertices),out,sizeof(*out))||
       scene_output_overlaps(s,out,sizeof(*out))||
       scene_output_overlaps(s,vertices,capacity*sizeof(*vertices)))
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
    if(scene_output_overlaps(s,out,sizeof(*out))||
       (capacity&&(scene_output_overlaps(
                         s,reactor_targets,capacity*sizeof(*reactor_targets))||
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

/* Transactional two-stage H-link verification. The first stage checks
 * typed H role grammar and target membership, the second checks the
 * individual target object's MS length, CRC16 and declared BS type.
 * A valid declared type cannot establish an AcDb layer/owner CLASS. */
int musa_dwg_ac1018_scene_audit_link_record_envelopes(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    MusaDwgAc1018RecordProbe *reactor_records,size_t capacity,
    MusaDwgAc1018LinkedRecordAudit *out){
    if(!s||!out||!handle||capacity>64u||
       (capacity&&!reactor_records)||
       scene_output_overlaps(s,out,sizeof(*out))||
       (capacity&&(overlaps(out,sizeof(*out),reactor_records,
                            capacity*sizeof(*reactor_records))||
                   scene_output_overlaps(s,reactor_records,
                            capacity*sizeof(*reactor_records)))))
        return 0;
    MusaDwgAc1018LinkedRecordAudit staged={0};
    MusaDwgAc1018RecordProbe inspected[64]={{0}};
    uint64_t reactor_handles[64]={0};
    if(!musa_dwg_ac1018_scene_audit_links(s,handle,reactor_handles,
                        64u,&staged.link_roles)||
       staged.link_roles.reactor_count>capacity||
       !staged.link_roles.layer_handle||
       !musa_dwg_ac1018_scene_probe_record_handle(
                s,staged.link_roles.layer_handle,&staged.layer_record))
       return 0;
    if(staged.link_roles.owner_handle&&
       !musa_dwg_ac1018_scene_probe_record_handle(
                s,staged.link_roles.owner_handle,&staged.owner_record))
       return 0;
    if(staged.link_roles.xdictionary_handle&&
       !musa_dwg_ac1018_scene_probe_record_handle(
                s,staged.link_roles.xdictionary_handle,
                &staged.xdictionary_record))
       return 0;
    for(uint32_t i=0u;i<staged.link_roles.reactor_count;i++)
       if(!musa_dwg_ac1018_scene_probe_record_handle(
                    s,reactor_handles[i],&inspected[i]))return 0;
    if(staged.link_roles.reactor_count)
        memcpy(reactor_records,inspected,
             (size_t)staged.link_roles.reactor_count*sizeof(inspected[0]));
    *out=staged;
    return 1;
}

/* Fixed R2004 BS role-code filter on top of CRC+indexed H-link evidence.
 * These are DECLARED codes only, not decoded AcDb CLASSES, a target
 * embedded self-handle check, or a proven block ownership graph.
 * Standard entity owner mode zero points to BLOCK_HEADER (49);
 * entity layers to LAYER (51), extension dictionaries to DICTIONARY (42).
 * Reactor classes remain unclassified. Fail closed and atomic.
 */
int musa_dwg_ac1018_scene_audit_fixed_role_type_claims(
    const MusaDwgAc1018Scene *s,uint64_t entity_handle,
    MusaDwgAc1018LinkedRecordAudit *out){
    if(!s||!out||!entity_handle||
       overlaps(s,sizeof(*s),out,sizeof(*out))||
       overlaps(s->objects,s->object_bytes,out,sizeof(*out))||
       overlaps(s->index,s->index_count*sizeof(*s->index),
                out,sizeof(*out)))return 0;
    MusaDwgAc1018LinkedRecordAudit staged={0};
    MusaDwgAc1018RecordProbe reactors[64]={{0}};
    if(!musa_dwg_ac1018_scene_audit_link_record_envelopes(
             s,entity_handle,reactors,64u,&staged)||
       staged.layer_record.declared_type!=51u||
       (staged.link_roles.owner_handle &&
           staged.owner_record.declared_type!=49u)||
       (staged.link_roles.xdictionary_handle &&
           staged.xdictionary_record.declared_type!=42u))
       return 0;
    *out=staged;
    return 1;
}

/* Only the supported fixed H roles: strict numeric BS type claims
 * plus independently checked embedded self-H on each indexed target.
 * Does not overclaim CLASSES or target-specific AcDb fields. */
int musa_dwg_ac1018_scene_audit_fixed_link_identities(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    MusaDwgAc1018FixedLinkIdentities *out){
    if(!s||!out||!handle||!s->objects||!s->index||
       overlaps(s,sizeof(*s),out,sizeof(*out))||
       overlaps(s->objects,s->object_bytes,out,sizeof(*out))||
       overlaps(s->index,s->index_count*sizeof(*s->index),
                out,sizeof(*out)))return 0;
    MusaDwgAc1018FixedLinkIdentities checked={0};
    if(!musa_dwg_ac1018_scene_audit_fixed_role_type_claims(
               s,handle,&checked.roles)||
       !musa_dwg_ac1018_scene_probe_nonentity_identity(
               s,checked.roles.link_roles.layer_handle,&checked.layer)||
       checked.layer.declared_type!=51u)return 0;
    if(checked.roles.link_roles.owner_handle&&
       (!musa_dwg_ac1018_scene_probe_nonentity_identity(
             s,checked.roles.link_roles.owner_handle,&checked.owner)||
        checked.owner.declared_type!=49u))return 0;
    if(checked.roles.link_roles.xdictionary_handle&&
       (!musa_dwg_ac1018_scene_probe_nonentity_identity(
             s,checked.roles.link_roles.xdictionary_handle,
             &checked.xdictionary)||
        checked.xdictionary.declared_type!=42u))return 0;
    checked.verified_fixed_identity_links=1u;
    *out=checked;
    return 1;
}

/* Deterministic original binary DWG inventory: no selective skipping,
 * no full-scene materialization, no O(index_count) side copy. Indexed
 * record types are untrusted until their separate CRC and primitive
 * reader have been verified. A success with unclassified records is
 * deliberately marked incomplete (rather than pretending a full CAD
 * display/export can be made from the subset). */
int musa_dwg_ac1018_scene_inventory(
    const MusaDwgAc1018Scene *s,int strict_supported_only,
    MusaDwgAc1018SceneInventory *out){
    if(!s||!out||(strict_supported_only!=0&&strict_supported_only!=1)||
       scene_output_overlaps(s,out,sizeof(*out)))return 0;
    MusaDwgAc1018SceneInventory checked={0};
    MusaDwgAc1018LWVertex *poly_vertices=NULL;
    int ok=1;
    for(size_t i=0u;i<s->index_count;i++){
        MusaDwgAc1018RecordProbe probe={0};
        if(!musa_dwg_ac1018_scene_probe_record_at(s,i,&probe)){
            ok=0;break;
        }
        checked.indexed_record_count++;
        switch(probe.declared_type){
        case 19u: {
            MusaDwgAc1018Line line={0};
            if(musa_dwg_ac1018_scene_read_line(s,probe.handle,&line) &&
               line.handle==probe.handle)checked.decoded_lines++;
            else checked.undecodable_supported_records++;
            break;
        }
        case 17u:
        case 18u: {
            MusaDwgAc1018Curve curve={0};
            if(musa_dwg_ac1018_scene_read_curve(s,probe.handle,&curve) &&
               curve.handle==probe.handle &&
               curve.type_code==probe.declared_type){
                if(probe.declared_type==17u)checked.decoded_arcs++;
                else checked.decoded_circles++;
            }else checked.undecodable_supported_records++;
            break;
        }
        case 77u: {
            /* Allocate the bounded vertex storage ONCE for the entire
             * scan, never one scratch slab for every indexed polyline. */
            if(!poly_vertices){
                poly_vertices=(MusaDwgAc1018LWVertex*)malloc(
                    100000u*sizeof(*poly_vertices));
                if(!poly_vertices){ok=0;break;}
            }
            MusaDwgAc1018LWPolyline poly={0};
            if(musa_dwg_ac1018_scene_read_lwpolyline(
                    s,probe.handle,poly_vertices,100000u,&poly) &&
               poly.handle==probe.handle)
                checked.decoded_lwpolylines++;
            else checked.undecodable_supported_records++;
            break;
        }
        case 42u: /* DICTIONARY declared type */
        case 49u: /* BLOCK_HEADER declared type */
        case 51u: { /* LAYER declared type */
            /* A CRC-correct target with a wrong embedded self-H cannot
             * be treated as a harmless opaque record. Fail the entire
             * inventory atomically; this is NOT an AcDb class proof. */
            MusaDwgAc1018NonEntityIdentity fixed={0};
            if(!musa_dwg_ac1018_scene_probe_nonentity_identity(
                   s,probe.handle,&fixed)||
               fixed.indexed_handle!=probe.handle||
               fixed.declared_type!=probe.declared_type||
               !fixed.record_crc_verified||!fixed.embedded_self_verified){
                ok=0;break;
            }
            /* The semantic fields are still opaque: strict display and
             * export must continue rejecting this incomplete scene. */
            checked.other_declared_records++;
            break;
        }
        default:
            checked.other_declared_records++;
            break;
        }
        if(!ok)break;
    }
    free(poly_vertices);
    if(!ok)return 0;
    checked.all_indexed_record_crcs_verified=1u;
    checked.only_supported_primitives_decoded=(uint8_t)(
        checked.other_declared_records==0u &&
        checked.undecodable_supported_records==0u);
    if(strict_supported_only&&!checked.only_supported_primitives_decoded)
        return 0;
    *out=checked;
    return 1;
}

/* Physical offsets are independent of strictly ascending H handles.
 * Never assume that adjacent handles imply adjacent object records. */
typedef struct MusaDwgAc1018ByteRange {
    size_t start;
    size_t length;
} MusaDwgAc1018ByteRange;
static int compare_byte_ranges(const void *a,const void *b){
    const MusaDwgAc1018ByteRange *x=(const MusaDwgAc1018ByteRange*)a;
    const MusaDwgAc1018ByteRange *y=(const MusaDwgAc1018ByteRange*)b;
    if(x->start<y->start)return -1;
    if(x->start>y->start)return 1;
    return 0;
}
int musa_dwg_ac1018_scene_audit_record_ranges(
    const MusaDwgAc1018Scene *s,MusaDwgAc1018RecordRangeAudit *out){
    if(!s||!out||!s->objects||!s->index||!s->index_count||
       s->index_count>SIZE_MAX/sizeof(MusaDwgAc1018ByteRange)||
       scene_output_overlaps(s,out,sizeof(*out)))return 0;
    MusaDwgAc1018ByteRange *ranges=(MusaDwgAc1018ByteRange*)
        malloc(s->index_count*sizeof(*ranges));
    if(!ranges)return 0;
    int ok=1;
    for(size_t i=0u;i<s->index_count;i++){
        MusaDwgAc1018RecordProbe r={0};
        if(!musa_dwg_ac1018_scene_probe_record_at(s,i,&r)||
           r.object_offset>(uint64_t)SIZE_MAX){
            ok=0;break;
        }
        ranges[i].start=(size_t)r.object_offset;
        ranges[i].length=r.record_bytes;
        if(ranges[i].start>s->object_bytes||
           ranges[i].length>s->object_bytes-ranges[i].start){
            ok=0;break;
        }
    }
    size_t cursor=0u,covered=0u;
    if(ok){
        qsort(ranges,s->index_count,sizeof(*ranges),
              compare_byte_ranges);
        for(size_t i=0u;i<s->index_count;i++){
            const size_t start=ranges[i].start;
            const size_t length=ranges[i].length;
            if(start<cursor||length>s->object_bytes-start||
               length>SIZE_MAX-covered){
                ok=0;break;
            }
            covered+=length;
            cursor=start+length;
        }
    }
    free(ranges);
    if(!ok)return 0;
    MusaDwgAc1018RecordRangeAudit checked={0};
    checked.indexed_records=s->index_count;
    checked.covered_record_bytes=covered;
    checked.unindexed_section_bytes=s->object_bytes-covered;
    checked.ranges_nonoverlapping=1u;
    *out=checked;
    return 1;
}
