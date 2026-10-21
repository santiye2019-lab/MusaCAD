/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_scene.h"
#include "musacad_ocs.h"
#include "musacad_dwg_ac1018_sections.h"
#include "musacad_dwg_ac1018_handles.h"
#include "musacad_dwg_ac1018_reactor_links.h"
#include "musacad_dwg_ac1018_reactor_links_internal.h"
#include "musacad_dwg_ac1015_values.h"
#include "musacad_crc16.h"
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
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
int musa_dwg_ac1018_scene_read_layer_prefix(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    MusaDwgAc1018LayerPrefix *out){
    if(!s||!out||!handle||
       scene_output_overlaps(s,out,sizeof(*out)))return 0;
    size_t offset=0u;
    if(!get_offset(s,handle,&offset))return 0;
    return musa_dwg_ac1018_layer_prefix_record(
               s->objects,s->object_bytes,offset,handle,out);
}
int musa_dwg_ac1018_scene_read_layer_color(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    MusaDwgAc1018LayerColor *out){
    if(!s||!out||!handle||
       scene_output_overlaps(s,out,sizeof(*out)))return 0;
    size_t offset=0u;
    if(!get_offset(s,handle,&offset))return 0;
    return musa_dwg_ac1018_layer_color_record(
               s->objects,s->object_bytes,offset,handle,out);
}
static int compare_layer_handle_u64(const void *left,const void *right){
    const uint64_t a=*(const uint64_t*)left;
    const uint64_t b=*(const uint64_t*)right;
    return (a>b)-(a<b);
}
int musa_dwg_ac1018_scene_read_layer_control_refs(
    const MusaDwgAc1018Scene *s,uint64_t control_handle,
    uint64_t *entry_handles,size_t capacity,
    MusaDwgAc1018LayerControlRefs *out){
    if(!s||!out||!control_handle||!entry_handles||
       !capacity||capacity>4096u||capacity>SIZE_MAX/sizeof(uint64_t)||
       scene_output_overlaps(s,out,sizeof(*out))||
       scene_output_overlaps(s,entry_handles,capacity*sizeof(uint64_t))||
       overlaps(out,sizeof(*out),entry_handles,
                capacity*sizeof(uint64_t)))return 0;
    size_t offset=0u;
    if(!get_offset(s,control_handle,&offset))return 0;
    MusaDwgAc1018LayerControlRefs checked={0};
    if(!musa_dwg_ac1018_layer_control_refs_record(
           s->objects,s->object_bytes,offset,control_handle,
           NULL,0u,&checked)||
       !checked.entry_count||checked.entry_count>capacity||
       checked.entry_count>SIZE_MAX/sizeof(uint64_t))return 0;
    const size_t n=(size_t)checked.entry_count;
    uint64_t *resolved=(uint64_t*)malloc(n*sizeof(*resolved));
    uint64_t *sorted=(uint64_t*)malloc(n*sizeof(*sorted));
    int valid=resolved!=NULL&&sorted!=NULL;
    if(valid){
        MusaDwgAc1018LayerControlRefs tested={0};
        if(!musa_dwg_ac1018_layer_control_refs_record(
               s->objects,s->object_bytes,offset,control_handle,
               resolved,n,&tested)||
           tested.entry_count!=checked.entry_count)valid=0;
    }
    if(valid){
        memcpy(sorted,resolved,n*sizeof(*sorted));
        qsort(sorted,n,sizeof(*sorted),compare_layer_handle_u64);
        for(size_t i=0u;i<n;i++){
            MusaDwgAc1018NonEntityIdentity layer={0};
            if((i&&sorted[i]==sorted[i-1u])||
               !musa_dwg_ac1018_scene_probe_nonentity_identity(
                        s,sorted[i],&layer)||
               layer.declared_type!=51u||
               layer.indexed_handle!=sorted[i]||
               !layer.record_crc_verified||!layer.embedded_self_verified){
                valid=0;break;
            }
        }
    }
    if(valid){
        memcpy(entry_handles,resolved,n*sizeof(*resolved));
        *out=checked;
    }
    free(sorted);
    free(resolved);
    return valid;
}
/* A validated table->layer H link permits reverse MEMBERSHIP
 * lookup; it is not a decoded LAYER->owner H field. Refuse unsupported
 * tables rather than silently skipping them. No session modifications.
 */
int musa_dwg_ac1018_scene_find_layer_control_membership(
    const MusaDwgAc1018Scene *s,uint64_t layer_handle,
    uint64_t *out_control_handle){
    if(!s||!out_control_handle||!layer_handle||
       scene_output_overlaps(s,out_control_handle,sizeof(*out_control_handle)))
        return 0;
    MusaDwgAc1018NonEntityIdentity checked_layer={0};
    if(!musa_dwg_ac1018_scene_probe_nonentity_identity(
             s,layer_handle,&checked_layer)||
       checked_layer.declared_type!=51u||
       checked_layer.indexed_handle!=layer_handle||
       !checked_layer.record_crc_verified||
       !checked_layer.embedded_self_verified)return 0;
    uint64_t *refs=(uint64_t*)malloc(4096u*sizeof(*refs));
    if(!refs)return 0;
    uint64_t found=0u;
    int valid=1;
    for(size_t i=0u;i<s->index_count;i++){
        MusaDwgAc1018RecordProbe probe={0};
        if(!musa_dwg_ac1018_scene_probe_record_at(s,i,&probe)){
            valid=0;break;
        }
        if(probe.declared_type!=50u)continue;
        MusaDwgAc1018LayerControlRefs table={0};
        if(!musa_dwg_ac1018_scene_read_layer_control_refs(
                 s,probe.handle,refs,4096u,&table)){
            valid=0;break;
        }
        for(uint32_t j=0u;j<table.entry_count;j++){
            if(refs[j]!=layer_handle)continue;
            if(found!=0u){valid=0;break;}
            found=probe.handle;
        }
        if(!valid)break;
    }
    free(refs);
    if(!valid||found==0u)return 0;
    *out_control_handle=found;
    return 1;
}
/* Verify TOTAL numeric LAYER membership for the bounded supported
 * table grammar. Indexed handles are already ascending, so keep LAYER
 * handles sorted and use binary lookup for every table H target.
 * Not a reverse decoded owner-H reference or AcDb class certification.
 */
int musa_dwg_ac1018_scene_audit_layer_table_coverage(
    const MusaDwgAc1018Scene *s,
    MusaDwgAc1018LayerTableCoverage *out){
    if(!s||!out||!s->index||!s->objects||
       scene_output_overlaps(s,out,sizeof(*out)))return 0;
    size_t layer_count=0u,control_count=0u;
    for(size_t i=0u;i<s->index_count;i++){
        MusaDwgAc1018RecordProbe probe={0};
        if(!musa_dwg_ac1018_scene_probe_record_at(s,i,&probe))
            return 0;
        if(probe.declared_type==51u)layer_count++;
        else if(probe.declared_type==50u)control_count++;
    }
    if(!layer_count||!control_count||
       layer_count>SIZE_MAX/sizeof(uint64_t))return 0;
    uint64_t *layers=(uint64_t*)malloc(layer_count*sizeof(*layers));
    uint8_t *seen=(uint8_t*)calloc(layer_count,sizeof(*seen));
    uint64_t *refs=(uint64_t*)malloc(4096u*sizeof(*refs));
    if(!layers||!seen||!refs){
        free(layers);free(seen);free(refs);return 0;
    }
    int valid=1;
    size_t next=0u,matched=0u;
    for(size_t i=0u;i<s->index_count&&valid;i++){
        MusaDwgAc1018RecordProbe probe={0};
        if(!musa_dwg_ac1018_scene_probe_record_at(s,i,&probe)){
            valid=0;break;
        }
        if(probe.declared_type!=51u)continue;
        MusaDwgAc1018NonEntityIdentity identity={0};
        if(!musa_dwg_ac1018_scene_probe_nonentity_identity(
                 s,probe.handle,&identity)||
           identity.declared_type!=51u||
           identity.indexed_handle!=probe.handle||
           !identity.record_crc_verified||
           !identity.embedded_self_verified||next>=layer_count){
            valid=0;break;
        }
        layers[next++]=probe.handle;
    }
    if(next!=layer_count)valid=0;
    for(size_t i=0u;i<s->index_count&&valid;i++){
        MusaDwgAc1018RecordProbe probe={0};
        if(!musa_dwg_ac1018_scene_probe_record_at(s,i,&probe)){
            valid=0;break;
        }
        if(probe.declared_type!=50u)continue;
        MusaDwgAc1018LayerControlRefs table={0};
        if(!musa_dwg_ac1018_scene_read_layer_control_refs(
                 s,probe.handle,refs,4096u,&table)||
           table.indexed_handle!=probe.handle||
           !table.crc_verified||!table.embedded_self_verified||
           !table.supported_handle_roles_verified){
            valid=0;break;
        }
        for(uint32_t j=0u;j<table.entry_count;j++){
            const uint64_t h=refs[j];
            size_t lo=0u,hi=layer_count;
            while(lo<hi){
                const size_t mid=lo+(hi-lo)/2u;
                if(layers[mid]<h)lo=mid+1u;
                else hi=mid;
            }
            if(lo==layer_count||layers[lo]!=h||seen[lo]){
                valid=0;break;
            }
            seen[lo]=1u;
            matched++;
        }
    }
    if(valid&&matched!=layer_count)valid=0;
    if(valid){
        MusaDwgAc1018LayerTableCoverage checked={0};
        checked.indexed_layers=layer_count;
        checked.indexed_controls=control_count;
        checked.resolved_layer_references=matched;
        checked.all_indexed_layers_have_unique_table=1u;
        *out=checked;
    }
    free(refs);free(seen);free(layers);
    return valid;
}
typedef struct MusaDwgAc1018PaletteNameRef {
    const char *name;
} MusaDwgAc1018PaletteNameRef;
/* Fixed ASCII case folding avoids locale-dependent layer collisions. */
static int compare_palette_ascii_names(const void *a,const void *b){
    const unsigned char *left=(const unsigned char*)
        ((const MusaDwgAc1018PaletteNameRef*)a)->name;
    const unsigned char *right=(const unsigned char*)
        ((const MusaDwgAc1018PaletteNameRef*)b)->name;
    for(;;){
        unsigned l=*left++,r=*right++;
        if(l>='A'&&l<='Z')l+=('a'-'A');
        if(r>='A'&&r<='Z')r+=('a'-'A');
        if(l!=r)return (l>r)-(l<r);
        if(!l)return 0;
    }
}
/* All-or-nothing metadata extraction: require verified global table graph
 * before accepting ANY LAYER color/name. No guessed screen presentation.
 * Out-of-memory and unsupported codepages/CMC never produce partial results.
 */
int musa_dwg_ac1018_scene_read_verified_layer_palette(
    const MusaDwgAc1018Scene *s,
    MusaDwgAc1018LayerColor *layers,size_t capacity,
    MusaDwgAc1018LayerPaletteReport *out){
    if(!s||!layers||!out||!capacity||capacity>65536u||
       capacity>SIZE_MAX/sizeof(*layers)||
       scene_output_overlaps(s,layers,capacity*sizeof(*layers))||
       scene_output_overlaps(s,out,sizeof(*out))||
       overlaps(layers,capacity*sizeof(*layers),out,sizeof(*out)))
        return 0;
    MusaDwgAc1018LayerTableCoverage coverage={0};
    if(!musa_dwg_ac1018_scene_audit_layer_table_coverage(s,&coverage)||
       !coverage.all_indexed_layers_have_unique_table||
       !coverage.indexed_layers||
       coverage.indexed_layers>capacity||
       coverage.indexed_layers>65536u||
       coverage.indexed_layers>
           SIZE_MAX/sizeof(MusaDwgAc1018PaletteNameRef))
        return 0;
    const size_t n=coverage.indexed_layers;
    MusaDwgAc1018LayerColor *checked=
        (MusaDwgAc1018LayerColor*)calloc(n,sizeof(*checked));
    MusaDwgAc1018PaletteNameRef *names=
        (MusaDwgAc1018PaletteNameRef*)malloc(n*sizeof(*names));
    if(!checked||!names){free(checked);free(names);return 0;}
    size_t found=0u;
    int valid=1;
    for(size_t i=0u;i<s->index_count&&valid;i++){
        MusaDwgAc1018RecordProbe probe={0};
        if(!musa_dwg_ac1018_scene_probe_record_at(s,i,&probe)){
            valid=0;break;
        }
        if(probe.declared_type!=51u)continue;
        if(found>=n||
           !musa_dwg_ac1018_scene_read_layer_color(
                    s,probe.handle,&checked[found])||
           checked[found].prefix.indexed_handle!=probe.handle||
           !checked[found].prefix.record_crc_verified||
           !checked[found].prefix.embedded_self_verified||
           !checked[found].cmc_data_bits_verified||
           !checked[found].prefix.name_bytes||
           !checked[found].prefix.name[0]){
            valid=0;break;
        }
        names[found].name=checked[found].prefix.name;
        found++;
    }
    if(found!=n)valid=0;
    if(valid){
        qsort(names,n,sizeof(*names),compare_palette_ascii_names);
        for(size_t i=1u;i<n;i++){
            if(compare_palette_ascii_names(&names[i-1u],&names[i])==0){
                valid=0;break;
            }
        }
    }
    if(valid){
        MusaDwgAc1018LayerPaletteReport report={0};
        report.indexed_layers=n;
        report.indexed_layer_controls=coverage.indexed_controls;
        report.unique_ascii_names_verified=1u;
        report.raw_cmc_fields_verified=1u;
        memcpy(layers,checked,n*sizeof(*checked));
        *out=report;
    }
    free(names);free(checked);
    return valid;
}
/* Conservatively resolve a single ASCII name against the COMPLETE
 * already-supported graph/palette, not against a guessed subset. No global
 * locale, static scratch buffer, scene mutation or appearance assumptions.
 */
int musa_dwg_ac1018_scene_find_verified_layer_ascii_name(
    const MusaDwgAc1018Scene *s,const char *ascii_name,
    MusaDwgAc1018LayerColor *out){
    if(!s||!ascii_name||!out||
       scene_output_overlaps(s,out,sizeof(*out)))return 0;
    char query[256]={0};
    size_t len=0u;
    for(;len<sizeof(query);len++){
        const unsigned char c=(unsigned char)ascii_name[len];
        if(!c)break;
        if(c<32u||c>126u)return 0;
        query[len]=(char)c;
    }
    if(!len||len==sizeof(query))return 0;
    MusaDwgAc1018LayerTableCoverage coverage={0};
    if(!musa_dwg_ac1018_scene_audit_layer_table_coverage(s,&coverage)||
       !coverage.indexed_layers||
       coverage.indexed_layers>65536u||
       coverage.indexed_layers>SIZE_MAX/sizeof(MusaDwgAc1018LayerColor))
        return 0;
    const size_t count=coverage.indexed_layers;
    MusaDwgAc1018LayerColor *palette=
        (MusaDwgAc1018LayerColor*)calloc(count,sizeof(*palette));
    if(!palette)return 0;
    MusaDwgAc1018LayerPaletteReport report={0};
    int matched=0;
    if(musa_dwg_ac1018_scene_read_verified_layer_palette(
              s,palette,count,&report)&&
       report.indexed_layers==count&&
       report.unique_ascii_names_verified&&report.raw_cmc_fields_verified){
        const MusaDwgAc1018PaletteNameRef desired={query};
        for(size_t i=0u;i<count;i++){
            const MusaDwgAc1018PaletteNameRef actual={
                palette[i].prefix.name};
            if(compare_palette_ascii_names(&desired,&actual)==0){
                *out=palette[i];
                matched=1;
                break;
            }
        }
    }
    free(palette);
    return matched;
}
/* All-or-nothing entity->LAYER lookup from original indexed R2004 bytes.
 * Never claim verified primitive coordinates from a header-only fixture.
 */
int musa_dwg_ac1018_scene_read_verified_entity_layer(
    const MusaDwgAc1018Scene *s,uint64_t entity_handle,
    MusaDwgAc1018EntityLayerEvidence *out){
    if(!s||!out||!entity_handle||
       scene_output_overlaps(s,out,sizeof(*out)))return 0;
    MusaDwgAc1018RecordProbe entity={0};
    if(!musa_dwg_ac1018_scene_probe_record_handle(
            s,entity_handle,&entity)||
       (entity.declared_type!=17u&&entity.declared_type!=18u&&
        entity.declared_type!=19u&&entity.declared_type!=77u))
        return 0;
    MusaDwgAc1018FixedLinkIdentities links={0};
    if(!musa_dwg_ac1018_scene_audit_fixed_link_identities(
            s,entity_handle,&links)||
       !links.verified_fixed_identity_links||
       links.roles.link_roles.handle!=entity_handle||
       !links.roles.link_roles.layer_handle||
       links.layer.indexed_handle!=links.roles.link_roles.layer_handle||
       !links.layer.embedded_self_verified||
       !links.layer.record_crc_verified)return 0;

    MusaDwgAc1018LayerTableCoverage coverage={0};
    if(!musa_dwg_ac1018_scene_audit_layer_table_coverage(s,&coverage)||
       !coverage.all_indexed_layers_have_unique_table||
       !coverage.indexed_layers||coverage.indexed_layers>65536u||
       coverage.indexed_layers>SIZE_MAX/sizeof(MusaDwgAc1018LayerColor))
        return 0;
    const size_t count=coverage.indexed_layers;
    MusaDwgAc1018LayerColor *layers=
        (MusaDwgAc1018LayerColor*)calloc(count,sizeof(*layers));
    if(!layers)return 0;
    MusaDwgAc1018LayerPaletteReport palette={0};
    MusaDwgAc1018EntityLayerEvidence checked={0};
    int valid=musa_dwg_ac1018_scene_read_verified_layer_palette(
                     s,layers,count,&palette)&&
              palette.indexed_layers==count&&
              palette.unique_ascii_names_verified&&
              palette.raw_cmc_fields_verified;
    if(valid){
        /* Palette is sorted by numeric handle; enforce exact H identity. */
        size_t lo=0u,hi=count;
        const uint64_t target=links.roles.link_roles.layer_handle;
        while(lo<hi){
            const size_t m=lo+(hi-lo)/2u;
            if(layers[m].prefix.indexed_handle<target)lo=m+1u;
            else hi=m;
        }
        if(lo==count||layers[lo].prefix.indexed_handle!=target)valid=0;
        else checked.layer=layers[lo];
    }
    if(valid){
        if(!musa_dwg_ac1018_scene_find_layer_control_membership(
                 s,links.roles.link_roles.layer_handle,
                 &checked.layer_control_handle)||
           !checked.layer_control_handle)valid=0;
    }
    if(valid){
        checked.entity_handle=entity_handle;
        checked.fixed_entity_links_verified=1u;
        checked.complete_layer_table_verified=1u;
        *out=checked;
    }
    free(layers);
    return valid;
}
/* Strict batch validation: one entire supported palette/table pass,
 * then per-entity O(log N) handle resolution and checked link identities.
 * This avoids repeatedly rebuilding the layer graph for every drawable.
 */
int musa_dwg_ac1018_scene_read_entity_layer_batch(
    const MusaDwgAc1018Scene *s,
    const uint64_t *entity_handles,size_t count,
    MusaDwgAc1018EntityLayerLink *links,size_t capacity,
    MusaDwgAc1018EntityLayerBatchReport *out){
    if(!s||!s->index||!s->objects||!entity_handles||!links||!out||
       !count||count>262144u||capacity<count||capacity>262144u||
       count>s->index_count||
       count>SIZE_MAX/sizeof(*entity_handles)||
       count>SIZE_MAX/sizeof(*links)||
       capacity>SIZE_MAX/sizeof(*links)||
       scene_output_overlaps(s,links,capacity*sizeof(*links))||
       scene_output_overlaps(s,out,sizeof(*out))||
       scene_output_overlaps(s,entity_handles,count*sizeof(*entity_handles))||
       overlaps(entity_handles,count*sizeof(*entity_handles),
                links,capacity*sizeof(*links))||
       overlaps(entity_handles,count*sizeof(*entity_handles),
                out,sizeof(*out))||
       overlaps(links,capacity*sizeof(*links),out,sizeof(*out)))
        return 0;
    MusaDwgAc1018LayerTableCoverage coverage={0};
    if(!musa_dwg_ac1018_scene_audit_layer_table_coverage(s,&coverage)||
       !coverage.all_indexed_layers_have_unique_table||
       !coverage.indexed_layers||coverage.indexed_layers>65536u||
       coverage.indexed_layers>SIZE_MAX/sizeof(MusaDwgAc1018LayerColor)||
       coverage.indexed_layers>SIZE_MAX/sizeof(uint64_t))
        return 0;
    const size_t n=coverage.indexed_layers;
    MusaDwgAc1018LayerColor *palette=
        (MusaDwgAc1018LayerColor*)calloc(n,sizeof(*palette));
    uint64_t *controls=(uint64_t*)calloc(n,sizeof(*controls));
    uint64_t *refs=(uint64_t*)malloc(4096u*sizeof(*refs));
    uint64_t *sorted=(uint64_t*)malloc(count*sizeof(*sorted));
    MusaDwgAc1018EntityLayerLink *staged=
        (MusaDwgAc1018EntityLayerLink*)calloc(count,sizeof(*staged));
    int valid=palette&&controls&&refs&&sorted&&staged;
    if(!valid)goto batch_done;
    MusaDwgAc1018LayerPaletteReport report={0};
    if(!musa_dwg_ac1018_scene_read_verified_layer_palette(
             s,palette,n,&report)||
       report.indexed_layers!=n||
       report.indexed_layer_controls!=coverage.indexed_controls||
       !report.unique_ascii_names_verified||
       !report.raw_cmc_fields_verified){
        valid=0;goto batch_done;
    }
    memcpy(sorted,entity_handles,count*sizeof(*sorted));
    qsort(sorted,count,sizeof(*sorted),compare_layer_handle_u64);
    for(size_t i=0u;i<count;i++){
        if(!sorted[i]||(i&&sorted[i]==sorted[i-1u])){
            valid=0;goto batch_done;
        }
    }
    /* Derive per-layer table H ONCE from exact original table H tails. */
    size_t matched=0u,table_count=0u;
    for(size_t i=0u;i<s->index_count;i++){
        MusaDwgAc1018RecordProbe probe={0};
        if(!musa_dwg_ac1018_scene_probe_record_at(s,i,&probe)){
            valid=0;goto batch_done;
        }
        if(probe.declared_type!=50u)continue;
        MusaDwgAc1018LayerControlRefs control={0};
        if(!musa_dwg_ac1018_scene_read_layer_control_refs(
                s,probe.handle,refs,4096u,&control)||
           control.indexed_handle!=probe.handle||
           !control.crc_verified||!control.embedded_self_verified||
           !control.supported_handle_roles_verified){
            valid=0;goto batch_done;
        }
        table_count++;
        for(uint32_t j=0u;j<control.entry_count;j++){
            const uint64_t h=refs[j];
            size_t lo=0u,hi=n;
            while(lo<hi){
                const size_t m=lo+(hi-lo)/2u;
                if(palette[m].prefix.indexed_handle<h)lo=m+1u;
                else hi=m;
            }
            if(lo==n||palette[lo].prefix.indexed_handle!=h||
               controls[lo]!=0u){
                valid=0;goto batch_done;
            }
            controls[lo]=probe.handle;
            matched++;
        }
    }
    if(matched!=n||table_count!=coverage.indexed_controls){
        valid=0;goto batch_done;
    }
    for(size_t i=0u;i<count;i++){
        const uint64_t h=entity_handles[i];
        MusaDwgAc1018RecordProbe record={0};
        if(!musa_dwg_ac1018_scene_probe_record_handle(s,h,&record)||
           (record.declared_type!=17u&&record.declared_type!=18u&&
            record.declared_type!=19u&&record.declared_type!=77u)){
            valid=0;goto batch_done;
        }
        MusaDwgAc1018FixedLinkIdentities checked={0};
        if(!musa_dwg_ac1018_scene_audit_fixed_link_identities(
                 s,h,&checked)||
           !checked.verified_fixed_identity_links||
           checked.roles.link_roles.handle!=h||
           !checked.layer.record_crc_verified||
           !checked.layer.embedded_self_verified){
            valid=0;goto batch_done;
        }
        const uint64_t target=checked.roles.link_roles.layer_handle;
        size_t lo=0u,hi=n;
        while(lo<hi){
            const size_t m=lo+(hi-lo)/2u;
            if(palette[m].prefix.indexed_handle<target)lo=m+1u;
            else hi=m;
        }
        if(lo==n||palette[lo].prefix.indexed_handle!=target||
           checked.layer.indexed_handle!=target||!controls[lo]){
            valid=0;goto batch_done;
        }
        staged[i].entity_handle=h;
        staged[i].layer_handle=target;
        staged[i].layer_control_handle=controls[lo];
        staged[i].raw_cmc_rgb=palette[lo].raw_cmc_rgb;
    }
    {
        MusaDwgAc1018EntityLayerBatchReport summary={0};
        summary.verified_entities=count;
        summary.verified_layers=n;
        summary.verified_layer_controls=table_count;
        summary.verified_unique_entity_handles=1u;
        summary.complete_layer_membership_verified=1u;
        memcpy(links,staged,count*sizeof(*links));
        *out=summary;
    }
batch_done:
    free(staged);free(sorted);free(refs);free(controls);free(palette);
    return valid;
}
/* Original protected H-index strict coverage. Unlike a user-selected
 * batch, enumeration is comprehensive inside this LIMITED code vocabulary.
 * Any unknown object means the scene CANNOT be represented by this API:
 * return failure, not a plausible-looking incomplete layer scene.
 */
int musa_dwg_ac1018_scene_collect_strict_indexed_layer_links(
    const MusaDwgAc1018Scene *s,
    MusaDwgAc1018EntityLayerLink *links,size_t capacity,
    MusaDwgAc1018IndexedLayerLinkCoverage *out){
    if(!s||!links||!out||!s->index||!s->objects||
       !capacity||capacity>262144u||
       capacity>SIZE_MAX/sizeof(*links)||
       scene_output_overlaps(s,links,capacity*sizeof(*links))||
       scene_output_overlaps(s,out,sizeof(*out))||
       overlaps(links,capacity*sizeof(*links),out,sizeof(*out)))
        return 0;
    size_t entities=0u,fixed=0u;
    for(size_t i=0u;i<s->index_count;i++){
        MusaDwgAc1018RecordProbe probe={0};
        if(!musa_dwg_ac1018_scene_probe_record_at(s,i,&probe))
            return 0;
        switch(probe.declared_type){
            case 17u: case 18u: case 19u: case 77u:
                entities++;
                if(entities>capacity)return 0;
                break;
            case 42u: case 49u: case 50u: case 51u: {
                MusaDwgAc1018NonEntityIdentity item={0};
                if(!musa_dwg_ac1018_scene_probe_nonentity_identity(
                         s,probe.handle,&item)||
                   item.indexed_handle!=probe.handle||
                   item.declared_type!=probe.declared_type||
                   !item.record_crc_verified||
                   !item.embedded_self_verified)return 0;
                fixed++;
                break;
            }
            default:
                /* Explicit refusal of TEXT/INSERT/XREF/unknown data. */
                return 0;
        }
    }
    if(!entities||entities>262144u||
       entities>SIZE_MAX/sizeof(uint64_t))return 0;
    uint64_t *requested=(uint64_t*)malloc(entities*sizeof(*requested));
    MusaDwgAc1018EntityLayerLink *staged=
        (MusaDwgAc1018EntityLayerLink*)malloc(
              entities*sizeof(*staged));
    if(!requested||!staged){
        free(staged);free(requested);return 0;
    }
    size_t cursor=0u;
    int valid=1;
    for(size_t i=0u;i<s->index_count;i++){
        MusaDwgAc1018RecordProbe probe={0};
        if(!musa_dwg_ac1018_scene_probe_record_at(s,i,&probe)){
            valid=0;break;
        }
        switch(probe.declared_type){
            case 17u: case 18u: case 19u: case 77u:
                if(cursor>=entities){valid=0;break;}
                requested[cursor++]=probe.handle;
                break;
            default: break;
        }
        if(!valid)break;
    }
    MusaDwgAc1018EntityLayerBatchReport batch={0};
    if(!valid||cursor!=entities||
       !musa_dwg_ac1018_scene_read_entity_layer_batch(
             s,requested,entities,staged,entities,&batch)||
       batch.verified_entities!=entities||
       !batch.verified_unique_entity_handles||
       !batch.complete_layer_membership_verified)
        valid=0;
    if(valid){
        MusaDwgAc1018IndexedLayerLinkCoverage summary={0};
        summary.indexed_records_checked=s->index_count;
        summary.fixed_nonentity_prefixes_checked=fixed;
        summary.supported_entity_links_checked=entities;
        summary.verified_layers=batch.verified_layers;
        summary.verified_layer_controls=batch.verified_layer_controls;
        summary.no_unclassified_record_types=1u;
        summary.all_supported_entity_links_verified=1u;
        memcpy(links,staged,entities*sizeof(*links));
        *out=summary;
    }
    free(staged);
    free(requested);
    return valid;
}
/* Evidence counters are deliberately independent: do not mark an
 * arbitrary header-only LINE as geometry or a geometric LINE as linked.
 * Whole indexed scene CRC + non-overlapping record envelopes are fatal
 * integrity requirements. Unsupported valid records remain opaque counts.
 */
int musa_dwg_ac1018_scene_audit_2d_geometry_vs_links(
    const MusaDwgAc1018Scene *s,
    MusaDwgAc1018GeometryLinkEvidence *out){
    if(!s||!out||scene_output_overlaps(s,out,sizeof(*out)))
        return 0;
    MusaDwgAc1018SceneInventory inv={0};
    MusaDwgAc1018RecordRangeAudit ranges={0};
    if(!musa_dwg_ac1018_scene_inventory(s,0,&inv)||
       !musa_dwg_ac1018_scene_audit_record_ranges(s,&ranges)||
       !inv.all_indexed_record_crcs_verified||
       !ranges.ranges_nonoverlapping||
       inv.indexed_record_count!=ranges.indexed_records||
       inv.indexed_record_count!=s->index_count)
        return 0;

    MusaDwgAc1018GeometryLinkEvidence checked={0};
    checked.indexed_records=inv.indexed_record_count;
    checked.other_declared_records=inv.other_declared_records;
    checked.all_record_ranges_nonoverlapping=1u;
    checked.all_indexed_record_crcs_verified=1u;
    MusaDwgAc1018LWVertex *scratch=NULL;
    int valid=1;
    for(size_t i=0u;i<s->index_count;i++){
        MusaDwgAc1018RecordProbe record={0};
        if(!musa_dwg_ac1018_scene_probe_record_at(s,i,&record)){
            valid=0;break;
        }
        if(record.declared_type!=17u&&record.declared_type!=18u&&
           record.declared_type!=19u&&record.declared_type!=77u)
            continue;
        checked.declared_supported_2d_entities++;
        int decoded=0;
        if(record.declared_type==19u){
            MusaDwgAc1018Line line={0};
            decoded=musa_dwg_ac1018_scene_read_line(
                        s,record.handle,&line)&&line.handle==record.handle;
        }else if(record.declared_type==17u||record.declared_type==18u){
            MusaDwgAc1018Curve curve={0};
            decoded=musa_dwg_ac1018_scene_read_curve(
                        s,record.handle,&curve)&&
                    curve.handle==record.handle&&
                    curve.type_code==record.declared_type;
        }else{
            if(!scratch){
                scratch=(MusaDwgAc1018LWVertex*)malloc(
                       100000u*sizeof(*scratch));
                if(!scratch){valid=0;break;}
            }
            MusaDwgAc1018LWPolyline poly={0};
            decoded=musa_dwg_ac1018_scene_read_lwpolyline(
                        s,record.handle,scratch,100000u,&poly)&&
                    poly.handle==record.handle;
        }
        MusaDwgAc1018FixedLinkIdentities link={0};
        const int linked=musa_dwg_ac1018_scene_audit_fixed_link_identities(
                           s,record.handle,&link)&&
                           link.verified_fixed_identity_links&&
                           link.roles.link_roles.handle==record.handle&&
                           link.layer.indexed_handle==
                                  link.roles.link_roles.layer_handle&&
                           link.layer.embedded_self_verified;
        if(decoded)checked.geometry_decoded++;
        if(linked)checked.fixed_link_identities_verified++;
        if(decoded&&linked)checked.geometry_and_links_verified++;
        else if(decoded)checked.geometry_only++;
        else if(linked)checked.links_only++;
        else checked.neither_verified++;
    }
    free(scratch);
    if(!valid||
       checked.geometry_decoded!=inv.decoded_lines+
          inv.decoded_arcs+inv.decoded_circles+inv.decoded_lwpolylines||
       checked.declared_supported_2d_entities!=
          checked.geometry_decoded+inv.undecodable_supported_records||
       checked.declared_supported_2d_entities!=
          checked.geometry_and_links_verified+checked.geometry_only+
          checked.links_only+checked.neither_verified||
       checked.geometry_decoded!=
          checked.geometry_and_links_verified+checked.geometry_only||
       checked.fixed_link_identities_verified!=
          checked.geometry_and_links_verified+checked.links_only)
        return 0;
    *out=checked;
    return 1;
}
/* Page every original indexed H, including unsupported records, without
 * publishing a partial result. The complete evidence audit is deliberately
 * repeated before each page; this is diagnostic integrity, not rendering.
 */
/* Parse a page against the already globally audited scene. Not public:
 * caller MUST preflight CRC/range coverage before invoking this helper. */
static int fill_diagnosis_range(
    const MusaDwgAc1018Scene *s,size_t first_index,size_t count,
    MusaDwgAc1018RecordDiagnosis *rows,
    MusaDwgAc1018RecordDiagnosisPage *out){
    MusaDwgAc1018LWVertex *vertices=NULL;
    MusaDwgAc1018RecordDiagnosisPage page={0};
    page.total_indexed_records=s->index_count;
    page.first_index=first_index;
    page.records_written=count;
    page.next_index=first_index+count;
    int valid=1;
    for(size_t j=0u;j<count;j++){
        MusaDwgAc1018RecordProbe record={0};
        if(!musa_dwg_ac1018_scene_probe_record_at(
                s,first_index+j,&record)||
           !record.record_crc_verified||
           !record.handle){
            valid=0;break;
        }
        MusaDwgAc1018RecordDiagnosis *row=&rows[j];
        row->indexed_handle=record.handle;
        row->declared_type=record.declared_type;
        if(record.declared_type==17u||record.declared_type==18u||
           record.declared_type==19u||record.declared_type==77u){
            row->category=MUSA_DWG_AC1018_DIAG_2D_ENTITY;
            page.supported_2d_in_page++;
            int geometric=0;
            if(record.declared_type==19u){
                MusaDwgAc1018Line line={0};
                geometric=musa_dwg_ac1018_scene_read_line(
                            s,record.handle,&line)&&
                          line.handle==record.handle;
            }else if(record.declared_type==17u||
                     record.declared_type==18u){
                MusaDwgAc1018Curve curve={0};
                geometric=musa_dwg_ac1018_scene_read_curve(
                            s,record.handle,&curve)&&
                          curve.handle==record.handle&&
                          curve.type_code==record.declared_type;
            }else{
                if(!vertices){
                    vertices=(MusaDwgAc1018LWVertex*)malloc(
                             100000u*sizeof(*vertices));
                    if(!vertices){valid=0;break;}
                }
                MusaDwgAc1018LWPolyline poly={0};
                geometric=musa_dwg_ac1018_scene_read_lwpolyline(
                            s,record.handle,vertices,100000u,&poly)&&
                          poly.handle==record.handle;
            }
            MusaDwgAc1018FixedLinkIdentities link={0};
            const int linked=
                musa_dwg_ac1018_scene_audit_fixed_link_identities(
                    s,record.handle,&link)&&
                link.verified_fixed_identity_links&&
                link.roles.link_roles.handle==record.handle&&
                link.layer.indexed_handle==
                    link.roles.link_roles.layer_handle&&
                link.layer.embedded_self_verified;
            row->geometry_decoded=(uint8_t)(geometric!=0);
            row->fixed_link_identities_verified=(uint8_t)(linked!=0);
            if(!geometric){
                row->evidence_issues|=
                    MUSA_DWG_AC1018_DIAG_GEOMETRY_REJECTED;
                page.geometry_rejected_in_page++;
            }
            if(!linked){
                row->evidence_issues|=
                    MUSA_DWG_AC1018_DIAG_LINK_AUDIT_REJECTED;
                page.link_audit_rejected_in_page++;
            }
        }else if(record.declared_type==42u||
                 record.declared_type==49u||
                 record.declared_type==50u||
                 record.declared_type==51u){
            MusaDwgAc1018NonEntityIdentity identity={0};
            if(!musa_dwg_ac1018_scene_probe_nonentity_identity(
                     s,record.handle,&identity)||
               identity.indexed_handle!=record.handle||
               identity.declared_type!=record.declared_type||
               !identity.record_crc_verified||
               !identity.embedded_self_verified){
                valid=0;break;
            }
            row->category=MUSA_DWG_AC1018_DIAG_FIXED_NONENTITY;
        }else{
            row->category=MUSA_DWG_AC1018_DIAG_UNCLASSIFIED;
            row->evidence_issues=MUSA_DWG_AC1018_DIAG_UNCLASSIFIED_TYPE;
            page.unclassified_in_page++;
        }
    }
    free(vertices);
    if(valid)*out=page;
    return valid;
}
/* Legacy page API retains its fully audited fail-closed behavior. */
int musa_dwg_ac1018_scene_read_record_diagnosis_page(
    const MusaDwgAc1018Scene *s,size_t first_index,
    MusaDwgAc1018RecordDiagnosis *rows,size_t capacity,
    MusaDwgAc1018RecordDiagnosisPage *out){
    if(!s||!s->index||!s->objects||!rows||!out||
       !capacity||capacity>4096u||first_index>=s->index_count||
       capacity>SIZE_MAX/sizeof(*rows)||
       scene_output_overlaps(s,rows,capacity*sizeof(*rows))||
       scene_output_overlaps(s,out,sizeof(*out))||
       overlaps(rows,capacity*sizeof(*rows),out,sizeof(*out)))
        return 0;
    MusaDwgAc1018GeometryLinkEvidence coverage={0};
    if(!musa_dwg_ac1018_scene_audit_2d_geometry_vs_links(
             s,&coverage)||
       coverage.indexed_records!=s->index_count||
       !coverage.all_record_ranges_nonoverlapping||
       !coverage.all_indexed_record_crcs_verified)
        return 0;
    const size_t remaining=s->index_count-first_index;
    const size_t count=remaining<capacity?remaining:capacity;
    MusaDwgAc1018RecordDiagnosis *staged=
        (MusaDwgAc1018RecordDiagnosis*)calloc(count,sizeof(*staged));
    if(!staged)return 0;
    MusaDwgAc1018RecordDiagnosisPage page={0};
    const int valid=fill_diagnosis_range(
              s,first_index,count,staged,&page);
    if(valid){
        memcpy(rows,staged,count*sizeof(*staged));
        *out=page;
    }
    free(staged);
    return valid;
}
/* Immutable diagnostic snapshot owns its entire materialized H page set.
 * It never borrows source scene memory after successful creation. */
struct MusaDwgAc1018DiagnosisSnapshot {
    size_t count;
    MusaDwgAc1018RecordDiagnosis *records;
};
void musa_dwg_ac1018_diagnosis_snapshot_close(
    MusaDwgAc1018DiagnosisSnapshot *snapshot){
    if(!snapshot)return;
    free(snapshot->records);
    free(snapshot);
}
size_t musa_dwg_ac1018_diagnosis_snapshot_handle_count(
    const MusaDwgAc1018DiagnosisSnapshot *snapshot){
    return snapshot?snapshot->count:0u;
}
int musa_dwg_ac1018_scene_create_diagnosis_snapshot(
    const MusaDwgAc1018Scene *s,
    MusaDwgAc1018DiagnosisSnapshot **out){
    if(!s||!s->index||!s->objects||!out||!s->index_count||
       s->index_count>1000000u||
       s->index_count>SIZE_MAX/sizeof(MusaDwgAc1018RecordDiagnosis)||
       scene_output_overlaps(s,out,sizeof(*out)))
        return 0;
    MusaDwgAc1018GeometryLinkEvidence coverage={0};
    if(!musa_dwg_ac1018_scene_audit_2d_geometry_vs_links(s,&coverage)||
       coverage.indexed_records!=s->index_count||
       !coverage.all_record_ranges_nonoverlapping||
       !coverage.all_indexed_record_crcs_verified)
        return 0;
    MusaDwgAc1018DiagnosisSnapshot *snapshot=
        (MusaDwgAc1018DiagnosisSnapshot*)calloc(1u,sizeof(*snapshot));
    if(!snapshot)return 0;
    snapshot->count=s->index_count;
    snapshot->records=(MusaDwgAc1018RecordDiagnosis*)calloc(
                    snapshot->count,sizeof(*snapshot->records));
    MusaDwgAc1018RecordDiagnosisPage unused={0};
    if(!snapshot->records||
       !fill_diagnosis_range(s,0u,snapshot->count,
                             snapshot->records,&unused)){
        musa_dwg_ac1018_diagnosis_snapshot_close(snapshot);
        return 0;
    }
    *out=snapshot;
    return 1;
}
/* Pages are copied from immutable stored records in O(page_size).
 * No file, stream, mutable scene, repeated audit or heap allocation. */
int musa_dwg_ac1018_diagnosis_snapshot_read_page(
    const MusaDwgAc1018DiagnosisSnapshot *snapshot,
    size_t first_index,MusaDwgAc1018RecordDiagnosis *rows,
    size_t capacity,MusaDwgAc1018RecordDiagnosisPage *out){
    if(!snapshot||!snapshot->records||!snapshot->count||
       !rows||!out||!capacity||capacity>4096u||
       first_index>=snapshot->count||
       capacity>SIZE_MAX/sizeof(*rows)||
       snapshot->count>SIZE_MAX/sizeof(*snapshot->records)||
       overlaps(snapshot,sizeof(*snapshot),rows,capacity*sizeof(*rows))||
       overlaps(snapshot,sizeof(*snapshot),out,sizeof(*out))||
       overlaps(snapshot->records,
                snapshot->count*sizeof(*snapshot->records),
                rows,capacity*sizeof(*rows))||
       overlaps(snapshot->records,
                snapshot->count*sizeof(*snapshot->records),
                out,sizeof(*out))||
       overlaps(rows,capacity*sizeof(*rows),out,sizeof(*out)))
        return 0;
    const size_t remain=snapshot->count-first_index;
    const size_t n=remain<capacity?remain:capacity;
    MusaDwgAc1018RecordDiagnosisPage page={0};
    page.total_indexed_records=snapshot->count;
    page.first_index=first_index;
    page.records_written=n;
    page.next_index=first_index+n;
    for(size_t i=0u;i<n;i++){
        const MusaDwgAc1018RecordDiagnosis *d=
             &snapshot->records[first_index+i];
        if(d->category==MUSA_DWG_AC1018_DIAG_2D_ENTITY){
            page.supported_2d_in_page++;
            if(!d->geometry_decoded)page.geometry_rejected_in_page++;
            if(!d->fixed_link_identities_verified)
                page.link_audit_rejected_in_page++;
        }else if(d->category==MUSA_DWG_AC1018_DIAG_UNCLASSIFIED){
            page.unclassified_in_page++;
        }
    }
    memcpy(rows,snapshot->records+first_index,n*sizeof(*rows));
    *out=page;
    return 1;
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
        case 50u: /* LAYER_CONTROL declared type */
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

/* Exact centerline vertex extrema only. No synthetic bounds for arc
 * bulges, variable-width strokes or non-planar thickness. */
static int finite_vec3(const double xyz[3]){
    return isfinite(xyz[0])&&isfinite(xyz[1])&&isfinite(xyz[2]);
}
static void bounds_include(MusaDwgAc1018LinearWcsBounds *box,
                           const double xyz[3]){
    for(size_t axis=0u;axis<3u;axis++){
        if(xyz[axis]<box->minimum[axis])box->minimum[axis]=xyz[axis];
        if(xyz[axis]>box->maximum[axis])box->maximum[axis]=xyz[axis];
    }
}
int musa_dwg_ac1018_scene_read_linear_wcs_bounds(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    MusaDwgAc1018LinearWcsBounds *out){
    if(!s||!handle||!out||
       scene_output_overlaps(s,out,sizeof(*out)))return 0;
    MusaDwgAc1018RecordProbe record={0};
    if(!musa_dwg_ac1018_scene_probe_record_handle(s,handle,&record)||
       (record.declared_type!=19u&&record.declared_type!=77u))
        return 0;
    MusaDwgAc1018LinearWcsBounds checked={0};
    checked.handle=handle;
    checked.type_code=record.declared_type;
    if(record.declared_type==19u){
        MusaDwgAc1018Line line={0};
        if(!musa_dwg_ac1018_scene_read_line(s,handle,&line)||
           line.handle!=handle||!isfinite(line.thickness)||
           line.thickness!=0.0||
           !finite_vec3(line.start)||!finite_vec3(line.end))
            return 0;
        checked.vertex_count=2u;
        checked.segment_count=1u;
        memcpy(checked.minimum,line.start,sizeof(checked.minimum));
        memcpy(checked.maximum,line.start,sizeof(checked.maximum));
        bounds_include(&checked,line.end);
    }else{
        MusaDwgAc1018LWVertex *vertices=
            (MusaDwgAc1018LWVertex*)malloc(
                    100000u*sizeof(MusaDwgAc1018LWVertex));
        if(!vertices)return 0;
        MusaDwgAc1018LWPolyline poly={0};
        int valid=musa_dwg_ac1018_scene_read_lwpolyline(
                    s,handle,vertices,100000u,&poly)&&
                  poly.handle==handle&&
                  poly.vertex_count>=2u&&poly.vertex_count<=100000u&&
                  isfinite(poly.thickness)&&poly.thickness==0.0&&
                  isfinite(poly.constant_width)&&poly.constant_width==0.0;
        if(valid){
            checked.vertex_count=poly.vertex_count;
            checked.segment_count=poly.vertex_count-(poly.closed?0u:1u);
            for(uint32_t i=0u;i<poly.vertex_count;i++){
                const MusaDwgAc1018LWVertex *v=&vertices[i];
                if(!isfinite(v->x)||!isfinite(v->y)||
                   !isfinite(v->bulge)||v->bulge!=0.0||
                   !isfinite(v->start_width)||v->start_width!=0.0||
                   !isfinite(v->end_width)||v->end_width!=0.0){
                    valid=0;break;
                }
                double wcs[3]={0};
                if(!musa_dwg_ac1018_lwpolyline_vertex_to_wcs(
                        &poly,v,wcs)||!finite_vec3(wcs)){
                    valid=0;break;
                }
                if(i==0u){
                    memcpy(checked.minimum,wcs,sizeof(wcs));
                    memcpy(checked.maximum,wcs,sizeof(wcs));
                }else bounds_include(&checked,wcs);
            }
        }
        free(vertices);
        if(!valid)return 0;
    }
    *out=checked;
    return 1;
}

/* Parametric original OCS circular centerline in WCS:
 * P(t) = OCS_to_WCS(center) +
 *        radius * (basis.x * cos(t) + basis.y * sin(t)).
 * Its coordinate-axis extrema occur where theta = atan2(Yaxis,Xaxis)
 * (max) and theta+PI (min), independently of axis-aligned orientation.
 */
static double curve_wrapped_angle(double a,double turn){
    double t=fmod(a,turn);
    return t<0.0?t+turn:t;
}
static int curve_point_wcs(
    const MusaCoreOcsBasis *basis,const double center[3],
    double radius,double theta,double out[3]){
    const double ct=cos(theta),st=sin(theta);
    if(!isfinite(ct)||!isfinite(st))return 0;
    for(size_t k=0u;k<3u;k++){
        out[k]=center[k]+radius*(basis->x[k]*ct+basis->y[k]*st);
        if(!isfinite(out[k]))return 0;
    }
    return 1;
}
static void curve_bounds_include(
    MusaDwgAc1018CurveWcsBounds *bounds,const double xyz[3]){
    for(size_t k=0u;k<3u;k++){
        if(xyz[k]<bounds->minimum[k])bounds->minimum[k]=xyz[k];
        if(xyz[k]>bounds->maximum[k])bounds->maximum[k]=xyz[k];
    }
}
int musa_dwg_ac1018_scene_read_curve_wcs_bounds(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    MusaDwgAc1018CurveWcsBounds *out){
    if(!s||!out||!handle||scene_output_overlaps(s,out,sizeof(*out)))
        return 0;
    MusaDwgAc1018Curve curve={0};
    if(!musa_dwg_ac1018_scene_read_curve(s,handle,&curve)||
       curve.handle!=handle||
       (curve.type_code!=17u&&curve.type_code!=18u)||
       !isfinite(curve.radius)||!(curve.radius>0.0)||
       !isfinite(curve.thickness))
        return 0;
    MusaCoreOcsBasis basis={0};
    const double xy[2]={curve.center[0],curve.center[1]};
    double center[3]={0};
    if(!musa_core_ocs_basis(curve.extrusion,&basis)||
       !musa_core_ocs_to_world(&basis,xy,curve.center[2],center)||
       !finite_vec3(center))
        return 0;
    const double pi=acos(-1.0),turn=2.0*pi;
    if(!isfinite(pi)||!isfinite(turn))return 0;
    const int is_arc=curve.type_code==17u;
    double start=0.0,sweep=turn;
    if(is_arc){
        /* Reject a full-turn ARC masquerading as two coincident angles,
         * and unbounded/repeated turns outside the standard single sweep.
         * Numeric angle reduction is limited for stable precision. */
        if(!isfinite(curve.start_angle)||
           !isfinite(curve.end_angle)||
           fabs(curve.start_angle)>1e8||
           fabs(curve.end_angle)>1e8||
           fabs(curve.end_angle-curve.start_angle)>turn+1e-10)
            return 0;
        start=curve_wrapped_angle(curve.start_angle,turn);
        sweep=curve_wrapped_angle(
                    curve.end_angle-curve.start_angle,turn);
        if(!isfinite(start)||!isfinite(sweep)||sweep<=1e-12||
           sweep>=turn-1e-12)return 0;
    }
    MusaDwgAc1018CurveWcsBounds checked={0};
    checked.handle=handle;
    checked.type_code=curve.type_code;
    checked.radius=curve.radius;
    checked.thickness=curve.thickness;
    checked.start_angle=start;
    checked.sweep_angle=sweep;
    memcpy(checked.center_wcs,center,sizeof(center));
    if(!is_arc){
        for(size_t k=0u;k<3u;k++){
            /* Analytic full-circle bounds, no tessellation/sample error. */
            const double amp=curve.radius*hypot(basis.x[k],basis.y[k]);
            checked.minimum[k]=center[k]-amp;
            checked.maximum[k]=center[k]+amp;
            if(!isfinite(amp)||!isfinite(checked.minimum[k])||
               !isfinite(checked.maximum[k]))return 0;
        }
    }else{
        double point[3]={0};
        if(!curve_point_wcs(&basis,center,curve.radius,start,point))
            return 0;
        memcpy(checked.minimum,point,sizeof(point));
        memcpy(checked.maximum,point,sizeof(point));
        if(!curve_point_wcs(&basis,center,curve.radius,
                            start+sweep,point))return 0;
        curve_bounds_include(&checked,point);
        for(size_t k=0u;k<3u;k++){
            const double a=curve_wrapped_angle(
                                 atan2(basis.y[k],basis.x[k]),turn);
            for(size_t j=0u;j<2u;j++){
                const double candidate=curve_wrapped_angle(a+j*pi,turn);
                const double dist=curve_wrapped_angle(candidate-start,turn);
                if(!isfinite(candidate)||!isfinite(dist))return 0;
                if(dist<=sweep+1e-12){
                    if(!curve_point_wcs(&basis,center,curve.radius,
                                        candidate,point))return 0;
                    curve_bounds_include(&checked,point);
                }
            }
        }
    }
    /* Exact bounds of the extruded circular sweep over 0..thickness,
     * including negative extrusion. OCS Z is a UNIT WORLD vector. */
    for(size_t k=0u;k<3u;k++){
        const double delta=curve.thickness*basis.z[k];
        if(!isfinite(delta))return 0;
        if(delta<0.0)checked.minimum[k]+=delta;
        else checked.maximum[k]+=delta;
        if(!isfinite(checked.minimum[k])||
           !isfinite(checked.maximum[k]))return 0;
    }
    *out=checked;
    return 1;
}

/* Exact original signed LW bulge geometry (b=tan(sweep/4)).
 * Center offset from chord midpoint along its OCS left normal is
 * (chord/4)*(1-b*b)/b. Use the SAME native arbitrary-axis basis as
 * all other R2004 geometric coordinates; never conflate OCS and WCS. */
int musa_dwg_ac1018_bulge_segment_wcs_bounds(
    const MusaDwgAc1018LWVertex *start,
    const MusaDwgAc1018LWVertex *end,
    const double extrusion[3],double elevation,double thickness,
    MusaDwgAc1018BulgeSegmentWcsBounds *out){
    if(!start||!end||!extrusion||!out||
       overlaps(start,sizeof(*start),out,sizeof(*out))||
       overlaps(end,sizeof(*end),out,sizeof(*out))||
       overlaps(extrusion,3u*sizeof(double),out,sizeof(*out))||
       !isfinite(start->x)||!isfinite(start->y)||
       !isfinite(end->x)||!isfinite(end->y)||
       !isfinite(start->bulge)||!isfinite(elevation)||
       !isfinite(thickness))return 0;
    const double bulge=start->bulge;
    if(fabs(bulge)>1e6||
       (bulge!=0.0&&fabs(bulge)<1e-12))
        return 0;
    MusaCoreOcsBasis basis={0};
    if(!musa_core_ocs_basis(extrusion,&basis))return 0;
    double p0[3]={0},p1[3]={0};
    const double xy0[2]={start->x,start->y};
    const double xy1[2]={end->x,end->y};
    if(!musa_core_ocs_to_world(&basis,xy0,elevation,p0)||
       !musa_core_ocs_to_world(&basis,xy1,elevation,p1)||
       !finite_vec3(p0)||!finite_vec3(p1))return 0;
    MusaDwgAc1018BulgeSegmentWcsBounds checked={0};
    memcpy(checked.minimum,p0,sizeof(p0));
    memcpy(checked.maximum,p0,sizeof(p0));
    for(size_t k=0u;k<3u;k++){
        if(p1[k]<checked.minimum[k])checked.minimum[k]=p1[k];
        if(p1[k]>checked.maximum[k])checked.maximum[k]=p1[k];
    }
    if(bulge!=0.0){
        const double dx=end->x-start->x,dy=end->y-start->y;
        const double chord=hypot(dx,dy);
        if(!isfinite(dx)||!isfinite(dy)||
           !isfinite(chord)||!(chord>0.0))return 0;
        const double center_offset=
             (chord*0.25)*((1.0-bulge*bulge)/bulge);
        const double radius=(chord*0.25)*
                                   (1.0+bulge*bulge)/fabs(bulge);
        const double cx=0.5*start->x+0.5*end->x-
                        (dy/chord)*center_offset;
        const double cy=0.5*start->y+0.5*end->y+
                        (dx/chord)*center_offset;
        const double sweep=4.0*atan(bulge);
        const double pi=acos(-1.0),turn=2.0*pi;
        if(!isfinite(center_offset)||!isfinite(radius)||
           !isfinite(cx)||!isfinite(cy)||!isfinite(sweep)||
           !isfinite(turn)||!(radius>0.0)||
           fabs(sweep)<1e-12||fabs(sweep)>=turn-1e-12)
            return 0;
        const double center_xy[2]={cx,cy};
        double center[3]={0};
        if(!musa_core_ocs_to_world(&basis,center_xy,elevation,center)||
           !finite_vec3(center))return 0;
        const double start_angle=curve_wrapped_angle(
                 atan2(start->y-cy,start->x-cx),turn);
        if(!isfinite(start_angle))return 0;
        for(size_t k=0u;k<3u;k++){
            const double a=curve_wrapped_angle(
                                  atan2(basis.y[k],basis.x[k]),turn);
            if(!isfinite(a))return 0;
            for(size_t j=0u;j<2u;j++){
                const double candidate=curve_wrapped_angle(a+j*pi,turn);
                const double distance=curve_wrapped_angle(
                   sweep>0.0 ? candidate-start_angle:
                               start_angle-candidate,turn);
                if(!isfinite(candidate)||!isfinite(distance))return 0;
                if(distance<=fabs(sweep)+1e-12){
                    double point[3]={0};
                    if(!curve_point_wcs(
                            &basis,center,radius,candidate,point))
                        return 0;
                    for(size_t axis=0u;axis<3u;axis++){
                        if(point[axis]<checked.minimum[axis])
                           checked.minimum[axis]=point[axis];
                        if(point[axis]>checked.maximum[axis])
                           checked.maximum[axis]=point[axis];
                    }
                }
            }
        }
        checked.curved=1u;
        checked.radius=radius;
        checked.sweep_angle=sweep;
    }
    /* Include the signed 3D extrusion of the WHOLE WCS curve; geometric
     * stroke width/variable widths are validated by caller separately. */
    for(size_t k=0u;k<3u;k++){
        const double delta=thickness*basis.z[k];
        if(!isfinite(delta))return 0;
        if(delta<0.0)checked.minimum[k]+=delta;
        else checked.maximum[k]+=delta;
        if(!isfinite(checked.minimum[k])||
           !isfinite(checked.maximum[k]))return 0;
    }
    *out=checked;
    return 1;
}
/* Shared original-DWG parser: width admission changes ONLY the nature of
 * the published EVIDENCE, never the actual bulge/segment decode. */
static int read_bulge_centerline_impl(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    int reject_nonzero_width,
    MusaDwgAc1018BulgeCenterlineAudit *out){
    if(!s||!handle||!out)return 0;
    MusaDwgAc1018RecordProbe record={0};
    if(!musa_dwg_ac1018_scene_probe_record_handle(s,handle,&record)||
       record.declared_type!=77u)return 0;
    MusaDwgAc1018LWVertex *vertices=
        (MusaDwgAc1018LWVertex*)malloc(100000u*sizeof(*vertices));
    if(!vertices)return 0;
    MusaDwgAc1018LWPolyline poly={0};
    int valid=musa_dwg_ac1018_scene_read_lwpolyline(
               s,handle,vertices,100000u,&poly)&&
              poly.handle==handle&&poly.vertex_count>=2u&&
              poly.vertex_count<=100000u&&
              isfinite(poly.constant_width)&&
              isfinite(poly.thickness)&&isfinite(poly.elevation);
    MusaDwgAc1018BulgeCenterlineAudit audit={0};
    MusaDwgAc1018BulgedPolylineWcsBounds checked={0};
    if(valid){
        audit.nonzero_constant_width=
                    (uint8_t)(poly.constant_width!=0.0);
        if(reject_nonzero_width&&audit.nonzero_constant_width)valid=0;
        checked.handle=handle;
        checked.vertex_count=poly.vertex_count;
        checked.segment_count=poly.vertex_count-(poly.closed?0u:1u);
        for(uint32_t i=0u;i<poly.vertex_count;i++){
            const MusaDwgAc1018LWVertex *v=&vertices[i];
            if(!isfinite(v->start_width)||
               !isfinite(v->end_width)||
               !isfinite(v->x)||!isfinite(v->y)||
               !isfinite(v->bulge)){
                valid=0;break;
            }
            if(v->start_width!=0.0||v->end_width!=0.0){
                audit.nonzero_variable_width_vertices++;
                if(reject_nonzero_width){valid=0;break;}
            }
        }
        if(!poly.closed&&vertices[poly.vertex_count-1u].bulge!=0.0)
            valid=0;
        for(uint32_t i=0u;valid&&i<checked.segment_count;i++){
            const uint32_t next=i+1u==poly.vertex_count?0u:i+1u;
            MusaDwgAc1018BulgeSegmentWcsBounds segment={0};
            if(!musa_dwg_ac1018_bulge_segment_wcs_bounds(
                  &vertices[i],&vertices[next],poly.extrusion,
                  poly.elevation,poly.thickness,&segment)){
                valid=0;break;
            }
            checked.curved_segment_count+=(uint32_t)segment.curved;
            if(i==0u){
                memcpy(checked.minimum,segment.minimum,
                       sizeof(checked.minimum));
                memcpy(checked.maximum,segment.maximum,
                       sizeof(checked.maximum));
            }else{
                for(size_t k=0u;k<3u;k++){
                    if(segment.minimum[k]<checked.minimum[k])
                        checked.minimum[k]=segment.minimum[k];
                    if(segment.maximum[k]>checked.maximum[k])
                        checked.maximum[k]=segment.maximum[k];
                }
            }
        }
    }
    free(vertices);
    if(!valid)return 0;
    audit.centerline=checked;
    audit.excluded_width_geometry=(uint8_t)(
                audit.nonzero_constant_width||
                audit.nonzero_variable_width_vertices!=0u);
    *out=audit;
    return 1;
}
int musa_dwg_ac1018_scene_read_bulged_polyline_wcs_bounds(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    MusaDwgAc1018BulgedPolylineWcsBounds *out){
    if(!s||!out||scene_output_overlaps(s,out,sizeof(*out)))return 0;
    MusaDwgAc1018BulgeCenterlineAudit checked={0};
    if(!read_bulge_centerline_impl(s,handle,1,&checked)||
       checked.excluded_width_geometry)return 0;
    *out=checked.centerline;
    return 1;
}
int musa_dwg_ac1018_scene_audit_bulge_centerline_wcs(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    MusaDwgAc1018BulgeCenterlineAudit *out){
    if(!s||!out||scene_output_overlaps(s,out,sizeof(*out)))return 0;
    MusaDwgAc1018BulgeCenterlineAudit checked={0};
    if(!read_bulge_centerline_impl(s,handle,0,&checked))return 0;
    *out=checked;
    return 1;
}

/* Width is permitted for EVIDENCE of the OCS radial neighborhood, not
 * interpreted as a complete DWG stroke mesh / join/cap equivalence.
 * For every source centerline point and interpolated width <= maxWidth,
 * its cross-sectional OCS offset lies within a disk of radius maxWidth/2.
 * The world axis projection of that disk is norm((OCS Xk, OCS Yk))*r.
 * An AABB of the centerline + this disk is a provable conservative bound
 * for this mathematical neighborhood, with tilted extrusion and signed
 * thickness already covered in the centerline input. */
int musa_dwg_ac1018_scene_audit_bulge_width_neighborhood(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    MusaDwgAc1018BulgeWidthNeighborhood *out){
    if(!s||!handle||!out||scene_output_overlaps(s,out,sizeof(*out)))
        return 0;
    MusaDwgAc1018BulgeWidthNeighborhood checked={0};
    if(!musa_dwg_ac1018_scene_audit_bulge_centerline_wcs(
                    s,handle,&checked.evidence))
        return 0;
    MusaDwgAc1018LWVertex *vertices=
        (MusaDwgAc1018LWVertex*)malloc(100000u*sizeof(*vertices));
    if(!vertices)return 0;
    MusaDwgAc1018LWPolyline poly={0};
    int valid=musa_dwg_ac1018_scene_read_lwpolyline(
                   s,handle,vertices,100000u,&poly)&&
              poly.handle==handle&&
              poly.vertex_count==checked.evidence.centerline.vertex_count&&
              poly.vertex_count<=100000u&&
              isfinite(poly.constant_width)&&poly.constant_width>=0.0;
    double largest=0.0;
    MusaCoreOcsBasis basis={0};
    if(valid){
        largest=poly.constant_width;
        valid=musa_core_ocs_basis(poly.extrusion,&basis);
        for(uint32_t i=0u;valid&&i<poly.vertex_count;i++){
            const double a=vertices[i].start_width;
            const double b=vertices[i].end_width;
            if(!isfinite(a)||!isfinite(b)||a<0.0||b<0.0){
                valid=0;break;
            }
            if(a>largest)largest=a;
            if(b>largest)largest=b;
        }
    }
    free(vertices);
    if(!valid||!isfinite(largest))return 0;
    checked.source_max_width=largest;
    for(size_t k=0u;k<3u;k++){
        const double axisnorm=hypot(basis.x[k],basis.y[k]);
        const double pad=0.5*largest*axisnorm;
        const double min=checked.evidence.centerline.minimum[k]-pad;
        const double max=checked.evidence.centerline.maximum[k]+pad;
        if(!isfinite(axisnorm)||!isfinite(pad)||!isfinite(min)||
           !isfinite(max)||min>max)return 0;
        checked.radial_pad_wcs[k]=pad;
        checked.padded_minimum[k]=min;
        checked.padded_maximum[k]=max;
    }
    checked.complete_visible_stroke_verified=0u;
    *out=checked;
    return 1;
}


/* Straight source LW ribbon is a ruled OCS quadrilateral, not the
 * oversized circular-neighborhood proxy of the complete DWG polyline.
 * This deliberately does NOT include join/cap/miter/lineweight semantics.
 */
int musa_dwg_ac1018_scene_read_straight_taper_wcs(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    uint32_t segment_index,MusaDwgAc1018StraightTaperWcs *out){
    if(!s||!handle||!out||scene_output_overlaps(s,out,sizeof(*out)))
        return 0;
    MusaDwgAc1018RecordProbe record={0};
    if(!musa_dwg_ac1018_scene_probe_record_handle(s,handle,&record)||
       record.declared_type!=77u)return 0;
    MusaDwgAc1018LWVertex *vertices=
        (MusaDwgAc1018LWVertex*)malloc(100000u*sizeof(*vertices));
    if(!vertices)return 0;
    MusaDwgAc1018LWPolyline poly={0};
    int good=musa_dwg_ac1018_scene_read_lwpolyline(
                 s,handle,vertices,100000u,&poly)&&
             poly.handle==handle&&poly.vertex_count>=2u&&
             poly.vertex_count<=100000u&&
             isfinite(poly.elevation)&&isfinite(poly.thickness)&&
             isfinite(poly.constant_width)&&poly.constant_width>=0.0;
    MusaDwgAc1018StraightTaperWcs checked={0};
    if(good){
        const uint32_t count=
             poly.vertex_count-(poly.closed?0u:1u);
        if(segment_index>=count||
           (poly.width_count!=0u&&
            poly.width_count!=poly.vertex_count))good=0;
    }
    if(good){
        const uint32_t j=(segment_index+1u==poly.vertex_count)?
                         0u:segment_index+1u;
        const MusaDwgAc1018LWVertex *a=&vertices[segment_index];
        const MusaDwgAc1018LWVertex *b=&vertices[j];
        const double dx=b->x-a->x,dy=b->y-a->y;
        const double len=hypot(dx,dy);
        if(!isfinite(a->x)||!isfinite(a->y)||
           !isfinite(b->x)||!isfinite(b->y)||
           !isfinite(a->bulge)||a->bulge!=0.0||
           !isfinite(dx)||!isfinite(dy)||
           !isfinite(len)||!(len>0.0))good=0;
        if(good){
            const int per_vertex=poly.width_count!=0u;
            const double sw=per_vertex?a->start_width:poly.constant_width;
            const double ew=per_vertex?a->end_width:poly.constant_width;
            MusaCoreOcsBasis basis={0};
            if(!isfinite(sw)||sw<0.0||!isfinite(ew)||ew<0.0||
               !musa_core_ocs_basis(poly.extrusion,&basis))good=0;
            if(good){
                checked.handle=handle;
                checked.segment_index=segment_index;
                checked.widths_from_vertices=(uint8_t)per_vertex;
                checked.start_width=sw;
                checked.end_width=ew;
                checked.thickness=poly.thickness;
                const double left_x=-dy/len,left_y=dx/len;
                for(size_t side=0u;side<4u;side++){
                    const int at_end=side>=2u;
                    const int is_left=(side%2u)==0u;
                    const MusaDwgAc1018LWVertex *v=at_end?b:a;
                    const double half=0.5*(at_end?ew:sw);
                    const double sign=is_left?1.0:-1.0;
                    const double xy[2]={
                         v->x+sign*left_x*half,
                         v->y+sign*left_y*half};
                    if(!isfinite(xy[0])||!isfinite(xy[1])||
                       !musa_core_ocs_to_world(
                             &basis,xy,poly.elevation,
                             checked.corners[side])||
                       !finite_vec3(checked.corners[side])){
                        good=0;break;
                    }
                    for(size_t axis=0u;axis<3u;axis++){
                        const double value=checked.corners[side][axis];
                        if(side==0u){
                            checked.minimum[axis]=value;
                            checked.maximum[axis]=value;
                        }else{
                            if(value<checked.minimum[axis])
                                checked.minimum[axis]=value;
                            if(value>checked.maximum[axis])
                                checked.maximum[axis]=value;
                        }
                    }
                }
                if(good){
                    for(size_t axis=0u;axis<3u;axis++){
                        const double delta=poly.thickness*basis.z[axis];
                        if(!isfinite(delta)){good=0;break;}
                        if(delta<0.0)checked.minimum[axis]+=delta;
                        else checked.maximum[axis]+=delta;
                        if(!isfinite(checked.minimum[axis])||
                           !isfinite(checked.maximum[axis])){
                            good=0;break;
                        }
                    }
                }
            }
        }
    }
    free(vertices);
    if(!good)return 0;
    *out=checked;
    return 1;
}


/* Immutable original DWG segment geometry, no pointer into the scene.
 * Fractional edge evaluation is independent of DWG source lifetime.
 * The original H is kept as evidence, not resolved on every sample. */
struct MusaDwgAc1018BulgeEdgeSnapshot {
    uint64_t handle;
    uint32_t segment_index;
    uint8_t widths_from_vertices;
    double center_ocs[2];
    double start_angle;
    double radius;
    double sweep;
    double elevation;
    double thickness;
    double start_width;
    double end_width;
    MusaCoreOcsBasis basis;
};
void musa_dwg_ac1018_bulge_edge_snapshot_close(
    MusaDwgAc1018BulgeEdgeSnapshot *snapshot){
    free(snapshot);
}
int musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
    const MusaDwgAc1018BulgeEdgeSnapshot *snapshot,double fraction,
    MusaDwgAc1018BulgeWidthEdgePoint *out){
    if(!snapshot||!out||!isfinite(fraction)||
       fraction<0.0||fraction>1.0||
       overlaps(snapshot,sizeof(*snapshot),out,sizeof(*out)))
        return 0;
    const double theta=snapshot->start_angle+snapshot->sweep*fraction;
    const double ct=cos(theta),st=sin(theta);
    const double width=(1.0-fraction)*snapshot->start_width+
                          fraction*snapshot->end_width;
    if(!isfinite(theta)||!isfinite(ct)||!isfinite(st)||
       !isfinite(width)||width<0.0)return 0;
    MusaDwgAc1018BulgeWidthEdgePoint checked={0};
    checked.handle=snapshot->handle;
    checked.segment_index=snapshot->segment_index;
    checked.widths_from_vertices=snapshot->widths_from_vertices;
    checked.fraction=fraction;
    checked.start_width=snapshot->start_width;
    checked.end_width=snapshot->end_width;
    checked.interpolated_width=width;
    checked.signed_sweep=snapshot->sweep;
    checked.radius=snapshot->radius;
    checked.thickness=snapshot->thickness;
    const double normal_sign=snapshot->sweep>0.0?-1.0:1.0;
    const double x=snapshot->center_ocs[0]+snapshot->radius*ct;
    const double y=snapshot->center_ocs[1]+snapshot->radius*st;
    const double half=width*0.5;
    const double xy_center[2]={x,y};
    const double xy_left[2]={x+normal_sign*half*ct,
                             y+normal_sign*half*st};
    const double xy_right[2]={x-normal_sign*half*ct,
                              y-normal_sign*half*st};
    if(!isfinite(x)||!isfinite(y)||
       !isfinite(xy_left[0])||!isfinite(xy_left[1])||
       !isfinite(xy_right[0])||!isfinite(xy_right[1])||
       !musa_core_ocs_to_world(&snapshot->basis,xy_center,
                               snapshot->elevation,checked.center_wcs)||
       !musa_core_ocs_to_world(&snapshot->basis,xy_left,
                               snapshot->elevation,checked.left_wcs)||
       !musa_core_ocs_to_world(&snapshot->basis,xy_right,
                               snapshot->elevation,checked.right_wcs)||
       !finite_vec3(checked.center_wcs)||
       !finite_vec3(checked.left_wcs)||
       !finite_vec3(checked.right_wcs))return 0;
    for(size_t axis=0u;axis<3u;axis++){
        const double delta=snapshot->thickness*snapshot->basis.z[axis];
        checked.left_thickness_wcs[axis]=checked.left_wcs[axis]+delta;
        checked.right_thickness_wcs[axis]=checked.right_wcs[axis]+delta;
        if(!isfinite(delta)||
           !isfinite(checked.left_thickness_wcs[axis])||
           !isfinite(checked.right_thickness_wcs[axis]))return 0;
    }
    *out=checked;
    return 1;
}
int musa_dwg_ac1018_scene_create_bulge_edge_snapshot(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    uint32_t segment_index,MusaDwgAc1018BulgeEdgeSnapshot **out){
    if(!s||!handle||!out||scene_output_overlaps(s,out,sizeof(*out)))
        return 0;
    MusaDwgAc1018RecordProbe record={0};
    if(!musa_dwg_ac1018_scene_probe_record_handle(s,handle,&record)||
       record.declared_type!=77u)return 0;
    MusaDwgAc1018LWVertex *vertices=
        (MusaDwgAc1018LWVertex*)malloc(
            100000u*sizeof(MusaDwgAc1018LWVertex));
    if(!vertices)return 0;
    MusaDwgAc1018LWPolyline poly={0};
    int valid=musa_dwg_ac1018_scene_read_lwpolyline(
                      s,handle,vertices,100000u,&poly)&&
              poly.handle==handle&&poly.vertex_count>=2u&&
              poly.vertex_count<=100000u&&
              isfinite(poly.elevation)&&isfinite(poly.thickness)&&
              isfinite(poly.constant_width)&&poly.constant_width>=0.0;
    if(valid){
        const uint32_t segments=
            poly.vertex_count-(poly.closed?0u:1u);
        valid=segment_index<segments&&
              (poly.width_count==0u||
               poly.width_count==poly.vertex_count);
    }
    MusaDwgAc1018BulgeEdgeSnapshot prepared={0};
    if(valid){
        const uint32_t next=segment_index+1u==poly.vertex_count?
                           0u:segment_index+1u;
        const MusaDwgAc1018LWVertex *a=&vertices[segment_index];
        const MusaDwgAc1018LWVertex *b=&vertices[next];
        const int per_vertex=poly.width_count!=0u;
        const double sw=per_vertex?a->start_width:poly.constant_width;
        const double ew=per_vertex?a->end_width:poly.constant_width;
        const double bulge=a->bulge;
        const double dx=b->x-a->x,dy=b->y-a->y;
        const double chord=hypot(dx,dy);
        if(!isfinite(a->x)||!isfinite(a->y)||
           !isfinite(b->x)||!isfinite(b->y)||
           !isfinite(sw)||sw<0.0||!isfinite(ew)||ew<0.0||
           !isfinite(bulge)||bulge==0.0||
           fabs(bulge)<1e-12||fabs(bulge)>1e6||
           !isfinite(dx)||!isfinite(dy)||
           !isfinite(chord)||!(chord>0.0))valid=0;
        if(valid){
            const double offset=(chord*0.25)*
                                ((1.0-bulge*bulge)/bulge);
            const double radius=(chord*0.25)*
                                ((1.0+bulge*bulge)/fabs(bulge));
            const double cx=0.5*a->x+0.5*b->x-(dy/chord)*offset;
            const double cy=0.5*a->y+0.5*b->y+(dx/chord)*offset;
            const double sweep=4.0*atan(bulge);
            const double turn=2.0*acos(-1.0);
            const double angle=atan2(a->y-cy,a->x-cx);
            if(!isfinite(offset)||!isfinite(radius)||
               !(radius>0.0)||!isfinite(cx)||!isfinite(cy)||
               !isfinite(sweep)||fabs(sweep)>=turn-1e-12||
               !isfinite(angle)||
               !musa_core_ocs_basis(poly.extrusion,&prepared.basis)){
                valid=0;
            }else{
                prepared.handle=handle;
                prepared.segment_index=segment_index;
                prepared.widths_from_vertices=(uint8_t)per_vertex;
                prepared.center_ocs[0]=cx;
                prepared.center_ocs[1]=cy;
                prepared.start_angle=angle;
                prepared.radius=radius;
                prepared.sweep=sweep;
                prepared.elevation=poly.elevation;
                prepared.thickness=poly.thickness;
                prepared.start_width=sw;
                prepared.end_width=ew;
                MusaDwgAc1018BulgeWidthEdgePoint test={0};
                valid=musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                        &prepared,0.0,&test)&&
                      musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                        &prepared,1.0,&test);
            }
        }
    }
    free(vertices);
    if(!valid)return 0;
    MusaDwgAc1018BulgeEdgeSnapshot *snapshot=
        (MusaDwgAc1018BulgeEdgeSnapshot*)malloc(sizeof(*snapshot));
    if(!snapshot)return 0;
    *snapshot=prepared;
    *out=snapshot;
    return 1;
}
/* Legacy convenient one-shot ABI shares exact same arithmetic with the
 * immutable snapshot; repeated viewport evaluations should use the
 * dedicated snapshot instead of repeatedly decoding the original DWG. */
int musa_dwg_ac1018_scene_evaluate_bulge_width_edges(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    uint32_t segment_index,double fraction,
    MusaDwgAc1018BulgeWidthEdgePoint *out){
    if(!s||!out||!isfinite(fraction)||
       fraction<0.0||fraction>1.0||
       scene_output_overlaps(s,out,sizeof(*out)))return 0;
    MusaDwgAc1018BulgeEdgeSnapshot *snapshot=NULL;
    if(!musa_dwg_ac1018_scene_create_bulge_edge_snapshot(
                    s,handle,segment_index,&snapshot))return 0;
    const int result=musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                         snapshot,fraction,out);
    musa_dwg_ac1018_bulge_edge_snapshot_close(snapshot);
    return result;
}


/* Conservative interval enclosure of original variable-width arc edges.
 * Each WCS axis component is Ck + q(t)*(Uxk*cos(theta(t))+
 * Uyk*sin(theta(t))), with q(t)=r +/- width(t)/2 LINEAR and theta
 * MONOTONE over each subinterval. Bound the harmonic by its analytic
 * extrema and multiply with the independent radial interval. This
 * cannot silently miss interior extrema as point-only sampling can.
 *
 * Rounded finite doubles are padded with a scaled floating guard.
 * Results are numerical enclosures, not formal real-arithmetic proofs. */
static int arc_axis_harmonic_range(
    double ux,double uy,double theta0,double theta1,
    double *low,double *high){
    if(!isfinite(ux)||!isfinite(uy)||
       !isfinite(theta0)||!isfinite(theta1)||
       !low||!high)return 0;
    const double a=fmin(theta0,theta1),b=fmax(theta0,theta1);
    const double v0=ux*cos(a)+uy*sin(a);
    const double v1=ux*cos(b)+uy*sin(b);
    const double amplitude=hypot(ux,uy);
    const double period=2.0*acos(-1.0);
    const double phase=atan2(uy,ux);
    if(!isfinite(v0)||!isfinite(v1)||!isfinite(amplitude)||
       !isfinite(period)||!isfinite(phase))return 0;
    double lo=fmin(v0,v1),hi=fmax(v0,v1);
    /* A small outward angular guard avoids missing a critical angle
     * exactly on an interval boundary due to double phase rounding. */
    const double tol=1e-12;
    if(ceil((a-phase-tol)/period)<=
       floor((b-phase+tol)/period))hi=amplitude;
    if(ceil((a-(phase+0.5*period)-tol)/period)<=
       floor((b-(phase+0.5*period)+tol)/period))lo=-amplitude;
    if(!isfinite(lo)||!isfinite(hi))return 0;
    *low=lo;*high=hi;
    return 1;
}
static void bounds_include_edge_sample(
    MusaDwgAc1018BulgeEdgeWcsEnvelope *box,
    const MusaDwgAc1018BulgeWidthEdgePoint *v){
    const double *pts[4]={
        v->left_wcs,v->right_wcs,
        v->left_thickness_wcs,v->right_thickness_wcs};
    for(size_t j=0u;j<4u;j++){
        for(size_t k=0u;k<3u;k++){
            const double x=pts[j][k];
            if(x<box->inner_minimum[k])box->inner_minimum[k]=x;
            if(x>box->inner_maximum[k])box->inner_maximum[k]=x;
        }
    }
}
int musa_dwg_ac1018_bulge_edge_snapshot_enclose(
    const MusaDwgAc1018BulgeEdgeSnapshot *s,
    double requested_tolerance,uint32_t max_intervals,
    MusaDwgAc1018BulgeEdgeWcsEnvelope *out){
    if(!s||!out||overlaps(s,sizeof(*s),out,sizeof(*out))||
       !isfinite(requested_tolerance)||!(requested_tolerance>0.0)||
       max_intervals<16u||max_intervals>65536u)
        return 0;
    const double center_xy[2]={s->center_ocs[0],s->center_ocs[1]};
    double center[3]={0};
    if(!musa_core_ocs_to_world(
               &s->basis,center_xy,s->elevation,center)||
       !finite_vec3(center))return 0;
    uint32_t n=16u;
    for(;;){
        MusaDwgAc1018BulgeEdgeWcsEnvelope box={0};
        box.handle=s->handle;
        box.segment_index=s->segment_index;
        box.intervals_used=n;
        box.requested_tolerance=requested_tolerance;
        for(size_t k=0u;k<3u;k++){
            box.minimum[k]=DBL_MAX;
            box.maximum[k]=-DBL_MAX;
            box.inner_minimum[k]=DBL_MAX;
            box.inner_maximum[k]=-DBL_MAX;
        }
        int good=1;
        for(uint32_t i=0u;i<n&&good;i++){
            const double t0=(double)i/(double)n;
            const double t1=(double)(i+1u)/(double)n;
            const double angle0=s->start_angle+s->sweep*t0;
            const double angle1=s->start_angle+s->sweep*t1;
            const double w0=(1.0-t0)*s->start_width+
                                          t0*s->end_width;
            const double w1=(1.0-t1)*s->start_width+
                                          t1*s->end_width;
            if(!isfinite(angle0)||!isfinite(angle1)||
               !isfinite(w0)||!isfinite(w1)||w0<0.0||w1<0.0){
                good=0;break;
            }
            /* A centerline and two true edge curves have the SAME
             * sin/cos basis. Only edge curves contribute to this box. */
            for(size_t k=0u;k<3u;k++){
                double harmonic_lo=0,harmonic_hi=0;
                if(!arc_axis_harmonic_range(
                        s->basis.x[k],s->basis.y[k],
                        angle0,angle1,&harmonic_lo,&harmonic_hi)){
                    good=0;break;
                }
                const double delta=s->thickness*s->basis.z[k];
                if(!isfinite(delta)){good=0;break;}
                const double bottom=fmin(0.0,delta);
                const double top=fmax(0.0,delta);
                for(int side=-1;side<=1;side+=2){
                    const double dir=s->sweep>0.0?-1.0:1.0;
                    const double q0=s->radius+
                            (double)side*dir*0.5*w0;
                    const double q1=s->radius+
                            (double)side*dir*0.5*w1;
                    const double qlo=fmin(q0,q1),qhi=fmax(q0,q1);
                    const double p[4]={qlo*harmonic_lo,
                                       qlo*harmonic_hi,
                                       qhi*harmonic_lo,
                                       qhi*harmonic_hi};
                    double lo=p[0],hi=p[0];
                    for(size_t j=0u;j<4u;j++){
                        if(!isfinite(p[j])){good=0;break;}
                        if(p[j]<lo)lo=p[j];
                        if(p[j]>hi)hi=p[j];
                    }
                    if(!good)break;
                    const double raw_min=center[k]+lo+bottom;
                    const double raw_max=center[k]+hi+top;
                    /* Widen, never shrink interval endpoints. */
                    const double guard=256.0*DBL_EPSILON*
                        (1.0+fabs(center[k])+fabs(qlo)+fabs(qhi)+
                         fabs(harmonic_lo)+fabs(harmonic_hi)+
                         fabs(delta)+fabs(raw_min)+fabs(raw_max));
                    const double lower=raw_min-guard;
                    const double upper=raw_max+guard;
                    if(!isfinite(lower)||!isfinite(upper)){
                        good=0;break;
                    }
                    if(lower<box.minimum[k])box.minimum[k]=lower;
                    if(upper>box.maximum[k])box.maximum[k]=upper;
                }
                if(!good)break;
            }
            if(!good)break;
            MusaDwgAc1018BulgeWidthEdgePoint sample={0};
            if(!musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                        s,t0,&sample)){good=0;break;}
            bounds_include_edge_sample(&box,&sample);
            if(!musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                        s,(t0+t1)*0.5,&sample)){good=0;break;}
            bounds_include_edge_sample(&box,&sample);
            if(i+1u==n){
                if(!musa_dwg_ac1018_bulge_edge_snapshot_evaluate(
                            s,1.0,&sample)){good=0;break;}
                bounds_include_edge_sample(&box,&sample);
            }
        }
        if(!good)return 0;
        double gap=0.0;
        for(size_t k=0u;k<3u;k++){
            if(!isfinite(box.minimum[k])||
               !isfinite(box.maximum[k])||
               !isfinite(box.inner_minimum[k])||
               !isfinite(box.inner_maximum[k])||
               box.minimum[k]>box.inner_minimum[k]||
               box.maximum[k]<box.inner_maximum[k])return 0;
            const double gl=box.inner_minimum[k]-box.minimum[k];
            const double gr=box.maximum[k]-box.inner_maximum[k];
            if(gl>gap)gap=gl;
            if(gr>gap)gap=gr;
        }
        box.max_outward_gap=gap;
        if(isfinite(gap)&&gap<=requested_tolerance){
            *out=box;return 1;
        }
        if(n==max_intervals)return 0;
        n=n<=max_intervals/2u?n*2u:max_intervals;
    }
}


/* Exact source width-edge endpoint with the derivative of its true
 * variable-width ribbon curve. A straight segment's OCS derivative is
 * chord + d(width)/dt * half-left-normal. For signed circular bulges,
 * theta'=sweep and r_edge(t)=radius +/- width(t)/2, hence the edge
 * derivative contains BOTH angular AND radial-width contributions.
 * Use genuine original DWG OCS->WCS basis, never viewer UCS.
 */
static int lw_join_segment_endpoint(
    const MusaDwgAc1018LWVertex *a,
    const MusaDwgAc1018LWVertex *b,
    const MusaCoreOcsBasis *basis,
    double elevation,double thickness,double sw,double ew,int end,
    double positions[2][3],double directions[2][3],
    double translated[2][3],uint8_t *curved){
    if(!a||!b||!basis||!positions||!directions||!translated||!curved||
       !isfinite(a->x)||!isfinite(a->y)||!isfinite(b->x)||!isfinite(b->y)||
       !isfinite(a->bulge)||!isfinite(sw)||!isfinite(ew)||
       sw<0.0||ew<0.0||!isfinite(elevation)||!isfinite(thickness))
        return 0;
    const double dx=b->x-a->x,dy=b->y-a->y;
    const double chord=hypot(dx,dy);
    if(!isfinite(dx)||!isfinite(dy)||
       !isfinite(chord)||!(chord>0.0))return 0;
    double tanx=dx/chord,tany=dy/chord;
    double radialx=0.0,radialy=0.0;
    double radius=0.0,sweep=0.0;
    const double bulge=a->bulge;
    if(bulge!=0.0){
        if(fabs(bulge)<1e-12||fabs(bulge)>1e6)return 0;
        const double offset=(chord*0.25)*
                            ((1.0-bulge*bulge)/bulge);
        radius=(chord*0.25)*
                            ((1.0+bulge*bulge)/fabs(bulge));
        const double cx=0.5*a->x+0.5*b->x-(dy/chord)*offset;
        const double cy=0.5*a->y+0.5*b->y+(dx/chord)*offset;
        const MusaDwgAc1018LWVertex *at=end?b:a;
        const double vx=at->x-cx,vy=at->y-cy;
        const double rn=hypot(vx,vy);
        sweep=4.0*atan(bulge);
        if(!isfinite(offset)||!isfinite(radius)||!(radius>0.0)||
           !isfinite(cx)||!isfinite(cy)||!isfinite(rn)||
           !(rn>0.0)||!isfinite(sweep)||
           fabs(sweep)>=2.0*acos(-1.0)-1e-12)
            return 0;
        radialx=vx/rn;radialy=vy/rn;
        const double dir=bulge>0.0?1.0:-1.0;
        tanx=-dir*radialy;
        tany= dir*radialx;
    }
    const double nx=-tany,ny=tanx;
    const MusaDwgAc1018LWVertex *v=end?b:a;
    const double width=end?ew:sw;
    const double delta_width=ew-sw;
    const double center_xy[2]={v->x,v->y};
    double center[3]={0};
    if(!musa_core_ocs_to_world(basis,center_xy,elevation,center)||
       !finite_vec3(center))return 0;
    for(size_t i=0u;i<2u;i++){
        const double side=i==0u?1.0:-1.0;
        const double x=v->x+side*0.5*width*nx;
        const double y=v->y+side*0.5*width*ny;
        const double xy[2]={x,y};
        double derivx=0.0,derivy=0.0;
        if(bulge==0.0){
            derivx=dx+side*0.5*delta_width*nx;
            derivy=dy+side*0.5*delta_width*ny;
        }else{
            const double radial_dir=bulge>0.0?-1.0:1.0;
            const double q=radius+side*radial_dir*0.5*width;
            const double dq=side*radial_dir*0.5*delta_width;
            derivx=dq*radialx-q*sweep*radialy;
            derivy=dq*radialy+q*sweep*radialx;
        }
        const double norm=hypot(derivx,derivy);
        if(!isfinite(x)||!isfinite(y)||!isfinite(norm)||
           !(norm>0.0))return 0;
        if(!musa_core_ocs_to_world(basis,xy,elevation,positions[i])||
           !finite_vec3(positions[i]))return 0;
        for(size_t k=0u;k<3u;k++){
            directions[i][k]=(basis->x[k]*derivx+
                               basis->y[k]*derivy)/norm;
            translated[i][k]=positions[i][k]+
                               thickness*basis->z[k];
            if(!isfinite(directions[i][k])||
               !isfinite(translated[i][k]))return 0;
        }
    }
    *curved=(uint8_t)(bulge!=0.0);
    return 1;
}
int musa_dwg_ac1018_scene_audit_lw_join_edges(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    uint32_t vertex_index,MusaDwgAc1018LwJoinEdgeAudit *out){
    if(!s||!handle||!out||scene_output_overlaps(s,out,sizeof(*out)))
        return 0;
    MusaDwgAc1018RecordProbe rec={0};
    if(!musa_dwg_ac1018_scene_probe_record_handle(s,handle,&rec)||
       rec.declared_type!=77u)return 0;
    MusaDwgAc1018LWVertex *vertices=
        (MusaDwgAc1018LWVertex*)malloc(
                        100000u*sizeof(*vertices));
    if(!vertices)return 0;
    MusaDwgAc1018LWPolyline poly={0};
    int valid=musa_dwg_ac1018_scene_read_lwpolyline(
                      s,handle,vertices,100000u,&poly)&&
              poly.handle==handle&&poly.vertex_count>=2u&&
              poly.vertex_count<=100000u&&
              vertex_index<poly.vertex_count&&
              (poly.closed||
               (vertex_index>0u&&
                vertex_index+1u<poly.vertex_count))&&
              (poly.width_count==0u||
               poly.width_count==poly.vertex_count)&&
              isfinite(poly.elevation)&&isfinite(poly.thickness)&&
              isfinite(poly.constant_width)&&poly.constant_width>=0.0;
    MusaDwgAc1018LwJoinEdgeAudit checked={0};
    if(valid){
        const uint32_t n=poly.vertex_count;
        const uint32_t before=vertex_index==0u?n-1u:
                                        vertex_index-1u;
        const uint32_t next=vertex_index+1u==n?0u:
                                        vertex_index+1u;
        const MusaDwgAc1018LWVertex *last=&vertices[before];
        const MusaDwgAc1018LWVertex *joint=&vertices[vertex_index];
        const MusaDwgAc1018LWVertex *following=&vertices[next];
        const int per_vertex=poly.width_count!=0u;
        const double in_sw=per_vertex?last->start_width:
                                      poly.constant_width;
        const double in_ew=per_vertex?last->end_width:
                                      poly.constant_width;
        const double out_sw=per_vertex?joint->start_width:
                                       poly.constant_width;
        const double out_ew=per_vertex?joint->end_width:
                                       poly.constant_width;
        MusaCoreOcsBasis basis={0};
        valid=musa_core_ocs_basis(poly.extrusion,&basis);
        if(valid){
            valid=lw_join_segment_endpoint(
                 last,joint,&basis,poly.elevation,poly.thickness,
                 in_sw,in_ew,1,checked.incoming_edge_wcs,
                 checked.incoming_edge_tangent_wcs,
                 checked.incoming_edge_thickness_wcs,
                 &checked.incoming_curved)&&
                 lw_join_segment_endpoint(
                 joint,following,&basis,poly.elevation,poly.thickness,
                 out_sw,out_ew,0,checked.outgoing_edge_wcs,
                 checked.outgoing_edge_tangent_wcs,
                 checked.outgoing_edge_thickness_wcs,
                 &checked.outgoing_curved);
        }
        if(valid){
            const double xy[2]={joint->x,joint->y};
            valid=musa_core_ocs_to_world(&basis,xy,poly.elevation,
                                         checked.center_wcs)&&
                  finite_vec3(checked.center_wcs);
            checked.handle=handle;
            checked.vertex_index=vertex_index;
            checked.incoming_segment_index=before;
            checked.outgoing_segment_index=vertex_index;
            checked.incoming_end_width=in_ew;
            checked.outgoing_start_width=out_sw;
            const double scale=1.0+fabs(in_ew)+fabs(out_sw)+
                    fabs(joint->x)+fabs(joint->y);
            const double limit=1e-10*scale;
            if(!isfinite(scale)||!isfinite(limit))valid=0;
            checked.source_widths_match=(uint8_t)(
                isfinite(in_ew)&&isfinite(out_sw)&&
                fabs(in_ew-out_sw)<=limit);
            double largest=0.0;
            for(size_t side=0u;side<2u&&valid;side++){
                double difference[3]={0};
                for(size_t k=0u;k<3u;k++)
                    difference[k]=checked.incoming_edge_wcs[side][k]-
                                  checked.outgoing_edge_wcs[side][k];
                const double gap=hypot(hypot(difference[0],
                                             difference[1]),
                                             difference[2]);
                if(!isfinite(gap)){valid=0;break;}
                checked.edge_gap[side]=gap;
                if(gap>largest)largest=gap;
            }
            if(valid)checked.edge_endpoints_match=
                           (uint8_t)(largest<=limit);
            /* A coincident endpoint does NOT prove a visible CAD join:
             * no miter/round/bevel construction was done here. */
            checked.complete_visible_join_verified=0u;
        }
    }
    free(vertices);
    if(!valid)return 0;
    *out=checked;
    return 1;
}


/* Standalone WCS bevel and finite, tangent-line miter CANDIDATES.
 * The underlying source-curve endpoints and true width-dependent edge
 * derivatives come from the CRC-checked original R2004 LW join audit.
 * No claim that an AutoCAD-style visible joining policy uses them. */
int musa_dwg_ac1018_scene_lw_join_candidates(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    uint32_t vertex_index,double miter_limit,
    MusaDwgAc1018LwJoinCandidates *out){
    if(!s||!handle||!out||!isfinite(miter_limit)||
       miter_limit<1.0||miter_limit>100.0||
       scene_output_overlaps(s,out,sizeof(*out)))return 0;
    MusaDwgAc1018LwJoinCandidates checked={0};
    if(!musa_dwg_ac1018_scene_audit_lw_join_edges(
                 s,handle,vertex_index,&checked.source))return 0;
    checked.miter_limit=miter_limit;
    const double width=fmax(checked.source.incoming_end_width,
                            checked.source.outgoing_start_width);
    const double half=width*0.5;
    const double allowed=miter_limit*half;
    if(!isfinite(width)||!isfinite(allowed)||width<0.0)return 0;
    for(size_t side=0u;side<2u;side++){
        double in[3]={0},outgoing[3]={0},tin[3]={0},tout[3]={0};
        double delta[3]={0};
        for(size_t k=0u;k<3u;k++){
            in[k]=checked.source.incoming_edge_wcs[side][k];
            outgoing[k]=checked.source.outgoing_edge_wcs[side][k];
            tin[k]=checked.source.incoming_edge_tangent_wcs[side][k];
            tout[k]=checked.source.outgoing_edge_tangent_wcs[side][k];
            checked.bevel_wcs[side][0][k]=in[k];
            checked.bevel_wcs[side][1][k]=outgoing[k];
            checked.bevel_thickness_wcs[side][0][k]=
                checked.source.incoming_edge_thickness_wcs[side][k];
            checked.bevel_thickness_wcs[side][1][k]=
                checked.source.outgoing_edge_thickness_wcs[side][k];
            delta[k]=outgoing[k]-in[k];
            if(!isfinite(in[k])||!isfinite(outgoing[k])||
               !isfinite(tin[k])||!isfinite(tout[k])||
               !isfinite(delta[k]))return 0;
        }
        const double dot=tin[0]*tout[0]+tin[1]*tout[1]+
                         tin[2]*tout[2];
        const double denominator=1.0-dot*dot;
        /* Parallel or near parallel: bevel geometry remains correct;
         * intersection is not well-defined or would be unstable. */
        if(!isfinite(dot)||!isfinite(denominator)||
           denominator<=1e-10)continue;
        const double p=delta[0]*tin[0]+delta[1]*tin[1]+
                       delta[2]*tin[2];
        const double q=delta[0]*tout[0]+delta[1]*tout[1]+
                       delta[2]*tout[2];
        const double v=(dot*p-q)/denominator;
        const double u=p+dot*v;
        if(!isfinite(u)||!isfinite(v))continue;
        double candidate[3]={0},other[3]={0};
        double diff2=0.0;
        double scale=1.0+fabs(u)+fabs(v)+fabs(width);
        for(size_t k=0u;k<3u;k++){
            candidate[k]=in[k]+u*tin[k];
            other[k]=outgoing[k]+v*tout[k];
            if(!isfinite(candidate[k])||!isfinite(other[k])){
                scale=INFINITY;break;
            }
            const double error=candidate[k]-other[k];
            diff2+=error*error;
            scale+=fabs(in[k])+fabs(outgoing[k]);
        }
        if(!isfinite(scale)||!isfinite(diff2))continue;
        const double consistency=1e-10*scale;
        const double radius_guard=1e-10*scale;
        if(sqrt(diff2)>consistency||
           fabs(u)>allowed+radius_guard||
           fabs(v)>allowed+radius_guard)continue;
        for(size_t k=0u;k<3u;k++){
            const double shift=
              checked.source.incoming_edge_thickness_wcs[side][k]-in[k];
            const double outgoing_shift=
              checked.source.outgoing_edge_thickness_wcs[side][k]-
              outgoing[k];
            const double translated=candidate[k]+shift;
            if(!isfinite(shift)||!isfinite(outgoing_shift)||
               !isfinite(translated)||
               fabs(shift-outgoing_shift)>consistency){
                scale=INFINITY;break;
            }
            checked.miter_wcs[side][k]=candidate[k];
            checked.miter_thickness_wcs[side][k]=translated;
        }
        if(!isfinite(scale))continue;
        checked.tangent_miter_valid[side]=1u;
        checked.incoming_extension[side]=u;
        checked.outgoing_extension[side]=v;
    }
    checked.complete_visible_join_verified=0u;
    *out=checked;
    return 1;
}


/* Original R2004 polyline ribbon-bevel envelope: one CRC-backed decode.
 * Segment computation and junction bridges are O(N), apart from bounded
 * interval arithmetic per curved edge. The bridge between two source
 * edge endpoints cannot extend an axis-aligned box enclosing them.
 * Genuinely different miter/round/cap strokes are NOT included.
 */
/* Work exclusively on once-decoded edge points and true WCS edge
 * tangents. Reject unstable or over-limit tangent-line intersections.
 * These are finite candidate points, not a DWG stroke style promise. */
static int lw_miter_accumulate(
    MusaDwgAc1018LwRibbonMiterWcsEnvelope *sum,
    const double in[2][3],const double ti[2][3],double wi,
    const double out[2][3],const double to[2][3],double wo,
    const MusaCoreOcsBasis *basis,double thickness){
    if(!sum||!in||!ti||!out||!to||!basis||
       !isfinite(wi)||!isfinite(wo)||wi<0.0||wo<0.0||
       !isfinite(thickness))return 0;
    const double w=fmax(wi,wo);
    const double limit=sum->miter_limit*0.5*w;
    if(!isfinite(limit))return 0;
    for(size_t side=0;side<2;side++){
        const double *a=in[side],*b=out[side];
        const double *uvec=ti[side],*vvec=to[side];
        double d[3]={0};
        for(size_t k=0;k<3;k++){
            d[k]=b[k]-a[k];
            if(!isfinite(d[k])||!isfinite(uvec[k])||
               !isfinite(vvec[k]))return 0;
        }
        const double dot=uvec[0]*vvec[0]+uvec[1]*vvec[1]+
                         uvec[2]*vvec[2];
        const double den=1.0-dot*dot;
        if(!isfinite(den)||den<=1e-10){
            sum->rejected_miter_candidates++;continue;
        }
        const double p=d[0]*uvec[0]+d[1]*uvec[1]+d[2]*uvec[2];
        const double q=d[0]*vvec[0]+d[1]*vvec[1]+d[2]*vvec[2];
        const double v=(dot*p-q)/den,u=p+dot*v;
        if(!isfinite(u)||!isfinite(v)){
            sum->rejected_miter_candidates++;continue;
        }
        double point[3]={0},extruded[3]={0},err2=0;
        double scale=1.0+fabs(u)+fabs(v)+fabs(w);
        for(size_t k=0;k<3;k++){
            point[k]=a[k]+u*uvec[k];
            const double other=b[k]+v*vvec[k];
            const double diff=point[k]-other;
            extruded[k]=point[k]+thickness*basis->z[k];
            err2+=diff*diff;
            scale+=fabs(a[k])+fabs(b[k]);
            if(!isfinite(point[k])||!isfinite(other)||
               !isfinite(extruded[k])||!isfinite(err2)||
               !isfinite(scale)){scale=INFINITY;break;}
        }
        if(!isfinite(scale)||sqrt(err2)>1e-10*scale||
           fabs(u)>limit+1e-10*scale||
           fabs(v)>limit+1e-10*scale){
            sum->rejected_miter_candidates++;continue;
        }
        for(size_t k=0;k<3;k++){
            if(point[k]<sum->minimum[k])sum->minimum[k]=point[k];
            if(point[k]>sum->maximum[k])sum->maximum[k]=point[k];
            if(extruded[k]<sum->minimum[k])
                sum->minimum[k]=extruded[k];
            if(extruded[k]>sum->maximum[k])
                sum->maximum[k]=extruded[k];
        }
        sum->valid_miter_candidates++;
    }
    return 1;
}
/* Conservative full continuous WCS coverage of one mathematical
 * radial-arc bridge between actual adjacent source edge endpoints.
 * Variable widths interpolate linearly; angles take shortest OCS path.
 * No assertion of Autodesk-selected round join or painted fill. */
static int lw_radial_bridge_accumulate(
    MusaDwgAc1018LwRadialBridgeWcsEnvelope *box,
    const double incoming[2][3],double incoming_width,
    const double following[2][3],double following_width,
    const double vertex_xy[2],const MusaCoreOcsBasis *basis,
    double elevation,double thickness){
    if(!box||!incoming||!following||!vertex_xy||!basis||
       !isfinite(incoming_width)||!isfinite(following_width)||
       incoming_width<0.0||following_width<0.0)return 0;
    double center[3]={0};
    if(!musa_core_ocs_to_world(basis,vertex_xy,elevation,center)||
       !finite_vec3(center))return 0;
    const double r0=0.5*incoming_width,r1=0.5*following_width;
    for(size_t side=0u;side<2u;side++){
        if(!(r0>0.0)||!(r1>0.0)){
            box->radial_bridges_rejected++;continue;
        }
        double dr[2][2]={{0}};
        const double *p[2]={incoming[side],following[side]};
        const double radii[2]={r0,r1};
        for(size_t j=0u;j<2u;j++){
            double wcs[3]={0};
            for(size_t k=0u;k<3u;k++)wcs[k]=p[j][k]-center[k];
            dr[j][0]=wcs[0]*basis->x[0]+wcs[1]*basis->x[1]+
                     wcs[2]*basis->x[2];
            dr[j][1]=wcs[0]*basis->y[0]+wcs[1]*basis->y[1]+
                     wcs[2]*basis->y[2];
            const double rr=hypot(dr[j][0],dr[j][1]);
            const double tol=1e-9*
                        (1.0+radii[j]+fabs(center[0])+
                         fabs(center[1])+fabs(center[2]));
            if(!isfinite(rr)||!isfinite(tol)||
               fabs(rr-radii[j])>tol)return 0;
        }
        const double angle0=atan2(dr[0][1],dr[0][0]);
        const double angle1=atan2(dr[1][1],dr[1][0]);
        const double delta=angle1-angle0;
        const double sweep=atan2(sin(delta),cos(delta));
        if(!isfinite(sweep)||
           fabs(sweep)>=acos(-1.0)-1e-12){
            box->radial_bridges_rejected++;continue;
        }
        if(fabs(r1-r0)>1e-10*(1.0+r0+r1))
            box->variable_radius_bridges++;
        /* Each independent harmonic×radial interval product encloses
         * EVERY intermediate point; 64 pieces sharpen the conservative
         * box and never substitute sampled points for true extrema. */
        for(uint32_t i=0u;i<64u;i++){
            const double t0=(double)i/64.0,t1=(double)(i+1u)/64.0;
            const double a0=angle0+sweep*t0;
            const double a1=angle0+sweep*t1;
            const double rt0=(1.0-t0)*r0+t0*r1;
            const double rt1=(1.0-t1)*r0+t1*r1;
            const double rl=fmin(rt0,rt1),rh=fmax(rt0,rt1);
            for(size_t k=0u;k<3u;k++){
                double hlo=0.0,hhi=0.0;
                if(!arc_axis_harmonic_range(
                        basis->x[k],basis->y[k],a0,a1,&hlo,&hhi))
                    return 0;
                const double v[4]={rl*hlo,rl*hhi,rh*hlo,rh*hhi};
                double lo=v[0],hi=v[0];
                for(size_t j=0u;j<4u;j++){
                    if(!isfinite(v[j]))return 0;
                    if(v[j]<lo)lo=v[j];
                    if(v[j]>hi)hi=v[j];
                }
                const double shift=thickness*basis->z[k];
                const double mn=center[k]+lo+fmin(0.0,shift);
                const double mx=center[k]+hi+fmax(0.0,shift);
                const double guard=256.0*DBL_EPSILON*
                          (1.0+fabs(center[k])+fabs(mn)+fabs(mx)+
                           fabs(rh)+fabs(shift));
                if(!isfinite(mn)||!isfinite(mx)||
                   !isfinite(guard)||!isfinite(mn-guard)||
                   !isfinite(mx+guard))return 0;
                if(mn-guard<box->minimum[k])
                    box->minimum[k]=mn-guard;
                if(mx+guard>box->maximum[k])
                    box->maximum[k]=mx+guard;
            }
        }
        box->radial_bridges_enclosed++;
    }
    return 1;
}
static int lw_envelope_include(
    MusaDwgAc1018LwRibbonBevelWcsEnvelope *dst,
    const double v[3]){
    if(!dst||!v||!finite_vec3(v))return 0;
    for(size_t k=0u;k<3u;k++){
        if(v[k]<dst->minimum[k])dst->minimum[k]=v[k];
        if(v[k]>dst->maximum[k])dst->maximum[k]=v[k];
    }
    return 1;
}
static int lw_envelope_bridge(
    MusaDwgAc1018LwRibbonBevelWcsEnvelope *dst,
    const double prior[2][3],double prior_width,
    const double following[2][3],double following_width){
    if(!dst||!prior||!following||
       !isfinite(prior_width)||!isfinite(following_width)||
       prior_width<0.0||following_width<0.0)return 0;
    const double tolerance=1e-10*
            (1.0+fabs(prior_width)+fabs(following_width));
    if(!isfinite(tolerance))return 0;
    if(fabs(prior_width-following_width)>tolerance)
        dst->discontinuous_width_junctions++;
    for(size_t side=0u;side<2u;side++){
        if(!lw_envelope_include(dst,prior[side])||
           !lw_envelope_include(dst,following[side]))return 0;
        const double dx=prior[side][0]-following[side][0];
        const double dy=prior[side][1]-following[side][1];
        const double dz=prior[side][2]-following[side][2];
        const double gap=hypot(hypot(dx,dy),dz);
        if(!isfinite(gap))return 0;
        if(gap>dst->maximum_bevel_edge_gap)
            dst->maximum_bevel_edge_gap=gap;
    }
    dst->bevel_bridge_count++;
    return 1;
}
static int lw_enclose_model_impl(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    double requested_tolerance,uint32_t max_intervals_per_curve,
    MusaDwgAc1018LwRibbonMiterWcsEnvelope *miter,
    double miter_limit,
    MusaDwgAc1018LwRadialBridgeWcsEnvelope *radial,
    MusaDwgAc1018LwRibbonBevelWcsEnvelope *out){
    if(!s||!handle||!out||
       scene_output_overlaps(s,out,sizeof(*out))||
       !isfinite(requested_tolerance)||!(requested_tolerance>0.0)||
       max_intervals_per_curve<16u||
       max_intervals_per_curve>65536u)return 0;
    MusaDwgAc1018RecordProbe record={0};
    if(!musa_dwg_ac1018_scene_probe_record_handle(s,handle,&record)||
       record.declared_type!=77u)return 0;
    MusaDwgAc1018LWVertex *vertices=
        (MusaDwgAc1018LWVertex*)malloc(100000u*sizeof(*vertices));
    if(!vertices)return 0;
    MusaDwgAc1018LWPolyline poly={0};
    int valid=musa_dwg_ac1018_scene_read_lwpolyline(
                 s,handle,vertices,100000u,&poly)&&
              poly.handle==handle&&
              poly.vertex_count>=2u&&poly.vertex_count<=100000u&&
              (poly.width_count==0u||
               poly.width_count==poly.vertex_count)&&
              isfinite(poly.elevation)&&isfinite(poly.thickness)&&
              isfinite(poly.constant_width)&&
              poly.constant_width>=0.0;
    MusaCoreOcsBasis basis={0};
    if(valid)valid=musa_core_ocs_basis(poly.extrusion,&basis);
    MusaDwgAc1018LwRibbonBevelWcsEnvelope checked={0};
    if(miter){
        miter->miter_limit=miter_limit;
        for(size_t k=0;k<3;k++){
            miter->minimum[k]=DBL_MAX;
            miter->maximum[k]=-DBL_MAX;
        }
    }
    if(radial){
        for(size_t k=0u;k<3u;k++){
            radial->minimum[k]=DBL_MAX;
            radial->maximum[k]=-DBL_MAX;
        }
    }
    if(valid){
        checked.handle=handle;
        checked.vertex_count=poly.vertex_count;
        checked.segment_count=poly.vertex_count-
                                   (poly.closed?0u:1u);
        checked.requested_tolerance=requested_tolerance;
        for(size_t k=0u;k<3u;k++){
            checked.minimum[k]=DBL_MAX;
            checked.maximum[k]=-DBL_MAX;
        }
        double first_start[2][3]={{0}},first_tangent[2][3]={{0}};
        double first_vertex_xy[2]={0};
        double previous_end[2][3]={{0}},previous_tangent[2][3]={{0}};
        double first_width=0.0,prior_width=0.0;
        const int per_vertex=poly.width_count!=0u;
        const uint32_t n=poly.vertex_count;
        for(uint32_t i=0u;i<checked.segment_count&&valid;i++){
            const uint32_t next=i+1u==n?0u:i+1u;
            const MusaDwgAc1018LWVertex *a=&vertices[i];
            const MusaDwgAc1018LWVertex *b=&vertices[next];
            const double sw=per_vertex?a->start_width:
                                       poly.constant_width;
            const double ew=per_vertex?a->end_width:
                                       poly.constant_width;
            double from[2][3]={{0}},to[2][3]={{0}};
            double tangent[2][3]={{0}},from_tangent[2][3]={{0}};
            double extruded[2][3]={{0}};
            uint8_t curved=0u;
            valid=lw_join_segment_endpoint(
                a,b,&basis,poly.elevation,poly.thickness,sw,ew,0,
                from,tangent,extruded,&curved);
            if(!valid)break;
            memcpy(from_tangent,tangent,sizeof(from_tangent));
            for(size_t side=0u;side<2u&&valid;side++){
                if(!lw_envelope_include(&checked,from[side])||
                   !lw_envelope_include(&checked,extruded[side]))
                    valid=0;
            }
            if(!valid)break;
            if(i==0u){
                memcpy(first_start,from,sizeof(first_start));
                memcpy(first_tangent,from_tangent,sizeof(first_tangent));
                first_vertex_xy[0]=a->x;
                first_vertex_xy[1]=a->y;
                first_width=sw;
            }else{
                valid=lw_envelope_bridge(&checked,previous_end,
                                         prior_width,from,sw);
                if(valid&&radial){
                    const double xy[2]={a->x,a->y};
                    valid=lw_radial_bridge_accumulate(radial,
                               previous_end,prior_width,from,sw,
                               xy,&basis,poly.elevation,poly.thickness);
                }
                if(valid&&miter)
                    valid=lw_miter_accumulate(miter,previous_end,
                           previous_tangent,prior_width,
                           from,from_tangent,sw,&basis,poly.thickness);
                if(!valid)break;
            }
            uint8_t end_curved=0u;
            valid=lw_join_segment_endpoint(
                a,b,&basis,poly.elevation,poly.thickness,sw,ew,1,
                to,tangent,extruded,&end_curved);
            if(!valid||end_curved!=curved){valid=0;break;}
            for(size_t side=0u;side<2u&&valid;side++){
                if(!lw_envelope_include(&checked,to[side])||
                   !lw_envelope_include(&checked,extruded[side]))
                    valid=0;
            }
            if(!valid)break;
            if(!curved){
                checked.straight_segments++;
            }else{
                checked.curved_segments++;
                /* A local immutable snapshot uses the ONE already
                 * verified DWG read. No per-segment scene re-open, no
                 * source data re-decompression or O(N^2) lookup. */
                const double dx=b->x-a->x,dy=b->y-a->y;
                const double chord=hypot(dx,dy),bulge=a->bulge;
                if(!isfinite(chord)||!(chord>0.0)||
                   !isfinite(bulge)||fabs(bulge)<1e-12||
                   fabs(bulge)>1e6){valid=0;break;}
                const double off=(chord*0.25)*
                                     ((1.0-bulge*bulge)/bulge);
                const double radius=(chord*0.25)*
                                     ((1.0+bulge*bulge)/fabs(bulge));
                const double cx=0.5*a->x+0.5*b->x-
                                                 (dy/chord)*off;
                const double cy=0.5*a->y+0.5*b->y+
                                                 (dx/chord)*off;
                const double sweep=4.0*atan(bulge);
                if(!isfinite(off)||!isfinite(radius)||
                   !(radius>0.0)||!isfinite(cx)||!isfinite(cy)||
                   !isfinite(sweep)||
                   fabs(sweep)>=2.0*acos(-1.0)-1e-12){
                    valid=0;break;
                }
                MusaDwgAc1018BulgeEdgeSnapshot curve={0};
                curve.handle=handle;
                curve.segment_index=i;
                curve.widths_from_vertices=(uint8_t)per_vertex;
                curve.center_ocs[0]=cx;curve.center_ocs[1]=cy;
                curve.start_angle=atan2(a->y-cy,a->x-cx);
                curve.radius=radius;curve.sweep=sweep;
                curve.elevation=poly.elevation;
                curve.thickness=poly.thickness;
                curve.start_width=sw;curve.end_width=ew;
                curve.basis=basis;
                MusaDwgAc1018BulgeEdgeWcsEnvelope bound={0};
                if(!musa_dwg_ac1018_bulge_edge_snapshot_enclose(
                            &curve,requested_tolerance,
                            max_intervals_per_curve,&bound)){
                    valid=0;break;
                }
                for(size_t k=0u;k<3u;k++){
                    const double lo=bound.minimum[k];
                    const double hi=bound.maximum[k];
                    if(!isfinite(lo)||!isfinite(hi)||lo>hi){
                        valid=0;break;
                    }
                    if(lo<checked.minimum[k])
                        checked.minimum[k]=lo;
                    if(hi>checked.maximum[k])
                        checked.maximum[k]=hi;
                }
                if(!valid)break;
                checked.curve_intervals_total+=
                                        bound.intervals_used;
                if(bound.max_outward_gap>checked.maximum_curve_gap)
                    checked.maximum_curve_gap=bound.max_outward_gap;
            }
            memcpy(previous_end,to,sizeof(previous_end));
            memcpy(previous_tangent,tangent,sizeof(previous_tangent));
            prior_width=ew;
        }
        if(valid&&poly.closed){
            valid=lw_envelope_bridge(&checked,previous_end,
                                    prior_width,first_start,first_width);
            if(valid&&radial)
                valid=lw_radial_bridge_accumulate(radial,
                           previous_end,prior_width,
                           first_start,first_width,first_vertex_xy,
                           &basis,poly.elevation,poly.thickness);
            if(valid&&miter)
                valid=lw_miter_accumulate(miter,previous_end,
                           previous_tangent,prior_width,
                           first_start,first_tangent,first_width,
                           &basis,poly.thickness);
        }
        if(valid){
            valid=checked.straight_segments+
                        checked.curved_segments==
                                  checked.segment_count&&
                  checked.bevel_bridge_count==
                   (poly.closed?checked.segment_count:
                          checked.segment_count-1u);
            for(size_t k=0u;k<3u;k++){
                if(!isfinite(checked.minimum[k])||
                   !isfinite(checked.maximum[k])||
                   checked.minimum[k]>checked.maximum[k])
                    valid=0;
            }
            checked.bevel_bridge_endpoints_enclosed=(uint8_t)valid;
            checked.all_miters_enclosed=0u;
            checked.all_round_joins_enclosed=0u;
            checked.complete_visible_stroke_verified=0u;
        }
    }
    free(vertices);
    if(!valid)return 0;
    if(miter){
        miter->bevel=checked;
        for(size_t k=0;k<3;k++){
            if(checked.minimum[k]<miter->minimum[k])
                miter->minimum[k]=checked.minimum[k];
            if(checked.maximum[k]>miter->maximum[k])
                miter->maximum[k]=checked.maximum[k];
            if(!isfinite(miter->minimum[k])||
               !isfinite(miter->maximum[k]))return 0;
        }
        miter->bounded_tangent_candidates_enclosed=1u;
        miter->complete_visible_stroke_verified=0u;
    }
    if(radial){
        radial->bevel=checked;
        for(size_t k=0u;k<3u;k++){
            if(checked.minimum[k]<radial->minimum[k])
                radial->minimum[k]=checked.minimum[k];
            if(checked.maximum[k]>radial->maximum[k])
                radial->maximum[k]=checked.maximum[k];
            if(!isfinite(radial->minimum[k])||
               !isfinite(radial->maximum[k]))return 0;
        }
        radial->all_admitted_radial_bridges_enclosed=1u;
        radial->complete_visible_stroke_verified=0u;
    }
    *out=checked;
    return 1;
}
int musa_dwg_ac1018_scene_enclose_lw_ribbon_bevel_wcs(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    double requested_tolerance,uint32_t max_intervals_per_curve,
    MusaDwgAc1018LwRibbonBevelWcsEnvelope *out){
    return lw_enclose_model_impl(s,handle,requested_tolerance,
                  max_intervals_per_curve,NULL,0.0,NULL,out);
}
int musa_dwg_ac1018_scene_enclose_lw_ribbon_miter_wcs(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    double requested_tolerance,uint32_t max_intervals_per_curve,
    double miter_limit,MusaDwgAc1018LwRibbonMiterWcsEnvelope *out){
    if(!s||!out||scene_output_overlaps(s,out,sizeof(*out))||
       !isfinite(miter_limit)||miter_limit<1.0||miter_limit>100.0)
        return 0;
    MusaDwgAc1018LwRibbonMiterWcsEnvelope result={0};
    MusaDwgAc1018LwRibbonBevelWcsEnvelope baseline={0};
    if(!lw_enclose_model_impl(s,handle,requested_tolerance,
                   max_intervals_per_curve,&result,miter_limit,
                   NULL,&baseline))return 0;
    *out=result;
    return 1;
}

int musa_dwg_ac1018_scene_enclose_lw_radial_bridges_wcs(
    const MusaDwgAc1018Scene *s,uint64_t handle,
    double requested_tolerance,uint32_t max_intervals_per_curve,
    MusaDwgAc1018LwRadialBridgeWcsEnvelope *out){
    if(!s||!out||scene_output_overlaps(s,out,sizeof(*out)))return 0;
    MusaDwgAc1018LwRadialBridgeWcsEnvelope evaluated={0};
    MusaDwgAc1018LwRibbonBevelWcsEnvelope base={0};
    if(!lw_enclose_model_impl(s,handle,requested_tolerance,
                  max_intervals_per_curve,NULL,0.0,&evaluated,
                  &base))return 0;
    *out=evaluated;
    return 1;
}
