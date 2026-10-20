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
