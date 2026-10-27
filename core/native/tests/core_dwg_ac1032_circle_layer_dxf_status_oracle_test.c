/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Genuine R2018 original H1C8 INSERT/H1BB CIRCLE/H8A LAYER source status.
 * DXF group 70 is separately read, NOT equated to DWG packed 70/290/370.
 * Unknown viewport, frozen/off and overrides continue to veto paint. */
#include "musacad_dwg_ac1032_circle_adaptive_diagnostic_v6.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct DxfOracle {
    unsigned layer_count,circle_count,insert_count,layer70_count,layer62_count;
    unsigned circle62_count,circle420_count,circle60_count;
    long group70,aci;
} DxfOracle;
static void trim(char *s){
    size_t n=strlen(s);
    while(n&&(s[n-1]=='\n'||s[n-1]=='\r'||s[n-1]==' '||
              s[n-1]=='\t'))s[--n]='\0';
    size_t i=0u;
    while(s[i]==' '||s[i]=='\t')i++;
    if(i)memmove(s,s+i,strlen(s+i)+1u);
}
static void finish(DxfOracle *o,const char *type,uint64_t h,
                   unsigned has70,long group70,unsigned has62,long aci,
                   unsigned has420,unsigned has60){
    if(!strcmp(type,"INSERT")&&h==UINT64_C(0x1c8))o->insert_count++;
    if(!strcmp(type,"CIRCLE")&&h==UINT64_C(0x1bb)){
        o->circle_count++;
        o->circle62_count+=has62;o->circle420_count+=has420;
        o->circle60_count+=has60;
    }
    if(!strcmp(type,"LAYER")&&h==UINT64_C(0x8a)){
        o->layer_count++;
        o->layer70_count+=has70;o->layer62_count+=has62;
        o->group70=group70;o->aci=aci;
    }
}
static int independent_dxf(const char *path,DxfOracle *o){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    char code[128],value[1024],type[64]={0};
    uint64_t h=0u;
    unsigned has70=0u,has62=0u,has420=0u,has60=0u;
    long group70=0L,aci=0L;
    int ok=1;
    while(fgets(code,sizeof(code),f)&&fgets(value,sizeof(value),f)){
        trim(code);trim(value);
        char *end=NULL;
        long group=strtol(code,&end,10);
        if(end==code||*end){ok=0;break;}
        if(group==0){
            finish(o,type,h,has70,group70,has62,aci,has420,has60);
            if(strlen(value)>=sizeof(type)){ok=0;break;}
            strcpy(type,value);
            h=0u;has70=has62=has420=has60=0u;
            group70=aci=0L;
        }else if(group==5){
            char *tail=NULL;
            unsigned long long v=strtoull(value,&tail,16);
            if(tail!=value&&!*tail)h=(uint64_t)v;
        }else if(group==70&&!strcmp(type,"LAYER")){
            char *tail=NULL;long v=strtol(value,&tail,10);
            if(tail==value||*tail||v<0L||v>255L){ok=0;break;}
            has70=1u;group70=v;
        }else if(group==62){
            char *tail=NULL;long v=strtol(value,&tail,10);
            if(tail==value||*tail||v<-32768L||v>32767L){ok=0;break;}
            has62=1u;aci=v;
        }else if(group==420&&!strcmp(type,"CIRCLE")){
            has420=1u;
        }else if(group==60&&!strcmp(type,"CIRCLE")){
            has60=1u;
        }
    }
    if(ok)finish(o,type,h,has70,group70,has62,aci,has420,has60);
    if(ferror(f))ok=0;
    if(fclose(f)!=0)ok=0;
    return ok&&o->insert_count==1u&&o->circle_count==1u&&
           o->layer_count==1u&&o->layer70_count==1u&&
           o->layer62_count==1u&&o->aci==4L&&
           !o->circle62_count&&!o->circle420_count&&!o->circle60_count;
}
static int run(const char *dwg,const char *dxf){
    DxfOracle oracle={0};
    if(!independent_dxf(dxf,&oracle))return 0;
    FILE *f=fopen(dwg,"rb");
    if(!f)return 0;
    int ok=fseek(f,0,SEEK_END)==0;
    long n=ok?ftell(f):-1L;
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)!=0)ok=0;
    uint8_t *bytes=ok?(uint8_t*)malloc((size_t)n):NULL;
    uint8_t *copy=bytes?(uint8_t*)malloc((size_t)n):NULL;
    if(!bytes||!copy)ok=0;
    if(ok)ok=fread(bytes,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(ok)memcpy(copy,bytes,(size_t)n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=!memcmp(bytes,"AC1032",6u)&&
       musa_dwg_ac1032_scene_index_open(bytes,(size_t)n,&scene)&&
       !memcmp(bytes,copy,(size_t)n);
    free(bytes);free(copy);
    if(!ok){musa_dwg_ac1032_scene_index_close(scene);return 0;}
    /* Independent original R2018 H1C8 INSERT golden fixture. Verify that
     * the new source-only join agrees exactly with the two independently
     * authenticated H/CRC readers; never infer reverse owner or paint.
     * If the OT49 child prefix is unsupported for this genuine fixture,
     * fail closed and retain the explicit negative oracle instead. */
    if(ok){
        MusaDwgAc1032InsertSourceGraph graph={0};
        ok=musa_dwg_ac1032_scene_index_insert_source_graph(
             scene,UINT64_C(0x1c8),&graph)&&
           graph.indexed_handle==UINT64_C(0x1c8)&&
           graph.block_header_handle!=0u&&
           graph.original_entity_and_targets_crc_verified&&
           graph.source_edges_verified&&
           !graph.block_expansion_and_display_accepted;
        if(ok){
            MusaDwgAc1032BlockHeaderTypedChildren children={0};
            const int direct=
                musa_dwg_ac1032_scene_index_block_header_typed_children(
                    scene,graph.block_header_handle,&children);
            MusaDwgAc1032InsertBlockChildInventory joined={0};
            const int admitted=
                musa_dwg_ac1032_scene_index_insert_block_child_inventory(
                    scene,UINT64_C(0x1c8),&joined);
            ok=direct==admitted;
            if(ok&&admitted)ok=
                !memcmp(&graph,&joined.insert,sizeof(graph))&&
                !memcmp(&children,&joined.block,sizeof(children))&&
                joined.insert_to_block_source_edge_verified&&
                !joined.reverse_owner_and_nested_transforms_verified&&
                !joined.final_display_accepted;
            if(ok&&!admitted){
                fprintf(stderr,"Original R2018 H1C8 OT49 typed children "
                    "unsupported: join correctly rejects; no block paint\\n");
            }
            MusaDwgAc1032InsertBlockChildInventory untouched;
            memset(&untouched,0xa5,sizeof(untouched));
            const MusaDwgAc1032InsertBlockChildInventory before=untouched;
            const uint64_t invalid[]={0u,UINT64_C(0x1bb),UINT64_MAX};
            for(size_t i=0u;ok&&i<sizeof(invalid)/sizeof(invalid[0]);i++)
                ok=!musa_dwg_ac1032_scene_index_insert_block_child_inventory(
                        scene,invalid[i],&untouched)&&
                   !memcmp(&untouched,&before,sizeof(before));
            if(ok)ok=!musa_dwg_ac1032_scene_index_insert_block_child_inventory(
                       scene,UINT64_C(0x1c8),NULL);
        }
    }
    MusaDwgAc1032CircleLayerStatusPreflight v={0};
    ok=musa_dwg_ac1032_circle_layer_status_preflight(
           scene,UINT64_C(0x1c8),UINT64_C(0x1bb),&v)&&
       v.source.chain.layer_handle==UINT64_C(0x8a)&&
       v.source.chain.original_layer_aci==(uint8_t)oracle.aci&&
       v.candidate_bits.layer_handle==UINT64_C(0x8a)&&
       v.candidate_bits.source_crc_verified&&
       v.candidate_bits.raw_values==
         v.source.original_layer_flags.raw_values_70_290_370&&
       v.candidate_bits.frozen_bit==
         ((v.candidate_bits.raw_values>>0u)&1u)&&
       v.candidate_bits.on_bit==
         ((v.candidate_bits.raw_values>>1u)&1u)&&
       v.candidate_bits.locked_bit==
         ((v.candidate_bits.raw_values>>3u)&1u)&&
       v.original_layer_status_bits_crc_verified&&
       !v.dxf_group70_and_viewport_semantics_verified&&
       !v.candidate_bits.display_accepted&&
       !v.effective_color_and_visible_paint_accepted;
    if(ok){
        MusaDwgAc1032CircleLayerStatusPreflight again={0};
        ok=musa_dwg_ac1032_circle_layer_status_preflight(
              scene,UINT64_C(0x1c8),UINT64_C(0x1bb),&again)&&
           !memcmp(&again,&v,sizeof(v));
    }
    if(ok){
        MusaDwgAc1032CircleLayerStatusPreflight untouched;
        memset(&untouched,0xa5,sizeof(untouched));
        const MusaDwgAc1032CircleLayerStatusPreflight before=untouched;
        const uint64_t bad[][2]={
            {0u,UINT64_C(0x1bb)},
            {UINT64_C(0x1c8),0u},
            {UINT64_C(0x1b8),UINT64_C(0x1bb)},
            {UINT64_C(0x1c8),UINT64_C(0x8a)},
            {UINT64_MAX,UINT64_C(0x1bb)}
        };
        for(size_t i=0u;ok&&i<sizeof(bad)/sizeof(bad[0]);i++)
            ok=!musa_dwg_ac1032_circle_layer_status_preflight(
                  scene,bad[i][0],bad[i][1],&untouched)&&
               !memcmp(&before,&untouched,sizeof(before));
        if(ok)ok=!musa_dwg_ac1032_circle_layer_status_preflight(
                  scene,UINT64_C(0x1c8),UINT64_C(0x1bb),NULL);
    }
    if(ok){
        MusaDwgAc1032CircleLayerDxfOracle external={
            UINT64_C(0x8a),(int32_t)oracle.group70,(int32_t)oracle.aci,
            1u,1u,1u
        };
        MusaDwgAc1032CircleLayerDxfStatusEvidence evidence={0};
        ok=musa_dwg_ac1032_circle_layer_dxf_status_oracle(
             scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
             &external,&evidence)&&
           evidence.layer_handle==UINT64_C(0x8a)&&
           evidence.dxf_layer_aci_matches_source&&
           evidence.original_dwg_crc_and_chain_verified&&
           evidence.external_dxf_frozen==((oracle.group70&1L)!=0L)&&
           evidence.external_dxf_frozen_in_new_viewports==
               ((oracle.group70&2L)!=0L)&&
           evidence.external_dxf_locked==((oracle.group70&4L)!=0L)&&
           !evidence.external_dxf_off&&
           !evidence.dwg_layer_visibility_semantics_verified&&
           !evidence.viewport_and_final_paint_accepted;
        if(ok){
            MusaDwgAc1032CircleLayerDxfStatusEvidence repeat={0};
            ok=musa_dwg_ac1032_circle_layer_dxf_status_oracle(
                 scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
                 &external,&repeat)&&
               !memcmp(&repeat,&evidence,sizeof(evidence));
        }
        if(ok){
            MusaDwgAc1032CircleLayerDxfStatusEvidence untouched;
            memset(&untouched,0xa5,sizeof(untouched));
            const MusaDwgAc1032CircleLayerDxfStatusEvidence before=untouched;
            const MusaDwgAc1032CircleLayerDxfOracle invalid[]={
                {UINT64_C(0x8b),0,4,1,1,1},
                {UINT64_C(0x8a),8,4,1,1,1},
                {UINT64_C(0x8a),0,5,1,1,1},
                {UINT64_C(0x8a),0,0,1,1,1},
                {UINT64_C(0x8a),0,4,0,1,1},
                {UINT64_C(0x8a),0,4,1,0,1},
                {UINT64_C(0x8a),0,4,1,1,0},
                {UINT64_C(0x8a),128,4,1,1,1},
                {UINT64_C(0x8a),0,-256,1,1,1}
            };
            for(size_t i=0u;ok&&i<sizeof(invalid)/sizeof(invalid[0]);i++)
                ok=!musa_dwg_ac1032_circle_layer_dxf_status_oracle(
                    scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
                    &invalid[i],&untouched)&&
                   !memcmp(&before,&untouched,sizeof(before));
            if(ok){
                MusaDwgAc1032CircleLayerDxfOracle synthetic=external;
                synthetic.dxf_signed_group62=-4;
                MusaDwgAc1032CircleLayerDxfStatusEvidence off={0};
                ok=musa_dwg_ac1032_circle_layer_dxf_status_oracle(
                     scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
                     &synthetic,&off)&&off.external_dxf_off&&
                   !off.viewport_and_final_paint_accepted;
            }
        }
    }
    /* Authentic immutable R2018 DWG/DXF oracle: classification is NOT
     * a runtime visibility permission, even if the companion layer is ON. */
    if(ok){
        MusaDwgAc1032CircleLayerDxfOracle external={
            UINT64_C(0x8a),(int32_t)oracle.group70,(int32_t)oracle.aci,
            1u,1u,1u
        };
        MusaDwgAc1032CircleLayerVisibilityVeto evidence={0};
        ok=musa_dwg_ac1032_circle_layer_visibility_veto(
             scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
             &external,&evidence)&&
           evidence.original_layer_handle==UINT64_C(0x8a)&&
           evidence.original_dwg_chain_crc_verified&&
           !evidence.final_visible_paint_accepted&&
           evidence.unresolved_original_dwg_requirements==
             (MUSA_CIRCLE_DWG_UNRESOLVED_LAYER_BITS|
              MUSA_CIRCLE_DWG_UNRESOLVED_OWNER_SPACE|
              MUSA_CIRCLE_DWG_UNRESOLVED_NESTED_INSERT|
              MUSA_CIRCLE_DWG_UNRESOLVED_VIEWPORT|
              MUSA_CIRCLE_DWG_UNRESOLVED_XREF)&&
           evidence.external_unconditional_veto==
             (uint32_t)(((oracle.aci<0L)?
               MUSA_CIRCLE_DXF_VETO_LAYER_OFF:0u)|
              ((oracle.group70&1L)?
               MUSA_CIRCLE_DXF_VETO_LAYER_FROZEN:0u))&&
           evidence.external_dxf_locked_for_editing==
             (uint8_t)((oracle.group70&4L)!=0L);
        if(ok){
            MusaDwgAc1032CircleLayerVisibilityVeto again={0};
            ok=musa_dwg_ac1032_circle_layer_visibility_veto(
                scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
                &external,&again)&&
               !memcmp(&evidence,&again,sizeof(evidence));
        }
        /* These are synthetic DXF statuses, NOT claims about the fixture.
         * Exercise OFF, frozen, new-viewport, locked and xref flags. */
        if(ok){
            const int32_t flags[]={0,1,2,4,16,32,64,119};
            for(size_t i=0u;ok&&i<sizeof(flags)/sizeof(flags[0]);i++){
                MusaDwgAc1032CircleLayerDxfOracle synthetic=external;
                synthetic.dxf_group70=flags[i];
                synthetic.dxf_signed_group62=(i==0u)?-4:4;
                MusaDwgAc1032CircleLayerVisibilityVeto v={0};
                const uint32_t expected_veto=
                    (uint32_t)(((i==0u)?
                      MUSA_CIRCLE_DXF_VETO_LAYER_OFF:0u)|
                     ((flags[i]&1)?
                      MUSA_CIRCLE_DXF_VETO_LAYER_FROZEN:0u));
                const uint32_t expected_risks=
                    (uint32_t)(((flags[i]&2)?
                      MUSA_CIRCLE_DXF_RISK_NEW_VIEWPORT_FROZEN:0u)|
                     ((flags[i]&16)?
                      MUSA_CIRCLE_DXF_RISK_XREF_DEPENDENT:0u)|
                     ((flags[i]&32)?
                      MUSA_CIRCLE_DXF_RISK_XREF_RESOLVED:0u)|
                     ((flags[i]&64)?
                      MUSA_CIRCLE_DXF_RISK_REFERENCED:0u));
                ok=musa_dwg_ac1032_circle_layer_visibility_veto(
                    scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
                    &synthetic,&v)&&
                   v.external_unconditional_veto==expected_veto&&
                   v.external_conditional_risks==expected_risks&&
                   v.external_dxf_locked_for_editing==
                       (uint8_t)((flags[i]&4)!=0)&&
                   !v.final_visible_paint_accepted;
            }
        }
        if(ok){
            MusaDwgAc1032CircleLayerVisibilityVeto untouched;
            memset(&untouched,0xa5,sizeof(untouched));
            const MusaDwgAc1032CircleLayerVisibilityVeto before=untouched;
            MusaDwgAc1032CircleLayerDxfOracle invalid=external;
            invalid.dxf_group70=8; /* reserved DXF LAYER status */
            ok=!musa_dwg_ac1032_circle_layer_visibility_veto(
                   scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
                   &invalid,&untouched)&&
               !memcmp(&before,&untouched,sizeof(before));
            if(ok)ok=!musa_dwg_ac1032_circle_layer_visibility_veto(
                   scene,UINT64_C(0x1c8),UINT64_C(0x8a),
                   &external,&untouched)&&
               !memcmp(&before,&untouched,sizeof(before));
        }
    }
    /* Enumerate the ORIGINAL R2018 indexed scene, not just a hand-picked
     * H1C8 call. Companion DXF is the independent oracle for H1BB/H8A
     * and ACI4, never an input to the runtime inventory. */
    if(ok){
        MusaDwgAc1032CircleByLayerInventory inventory={0};
        ok=musa_dwg_ac1032_circle_bylayer_inventory(scene,&inventory)&&
           inventory.original_source_inventory_only&&
           !inventory.final_visible_paint_accepted&&
           inventory.indexed_record_count==
               musa_dwg_ac1032_scene_index_count(scene)&&
           inventory.insert_count>=1u&&
           inventory.source_indexed_insert_count>=1u&&
           inventory.circle_candidate_count>=1u&&
           inventory.verified_source_color_count>=1u&&
           inventory.verified_source_color_count+
             inventory.unresolved_circle_count==
               inventory.circle_candidate_count&&
           inventory.source_indexed_insert_count+
             inventory.unresolved_insert_count==inventory.insert_count;
        unsigned matches=0u;
        for(size_t i=0u;ok&&i<inventory.verified_source_color_count;i++){
            const MusaDwgAc1032CircleByLayerInventoryRow *row=
                &inventory.rows[i];
            ok=row->original_source_crc_and_wcs_verified&&
               !row->final_visible_paint_accepted&&
               row->insert_handle&&row->circle_handle&&
               row->block_header_handle&&row->layer_handle&&
               row->original_layer_aci;
            if(row->insert_handle==UINT64_C(0x1c8)&&
               row->circle_handle==UINT64_C(0x1bb)){
                matches++;
                ok=ok&&row->block_header_handle==UINT64_C(0x1b8)&&
                   row->layer_handle==UINT64_C(0x8a)&&
                   row->original_layer_aci==(uint8_t)oracle.aci;
            }
        }
        if(ok)ok=matches==1u;
        if(ok){
            MusaDwgAc1032CircleByLayerInventory repeat={0};
            ok=musa_dwg_ac1032_circle_bylayer_inventory(scene,&repeat)&&
               !memcmp(&repeat,&inventory,sizeof(inventory));
        }
        if(ok){
            MusaDwgAc1032CircleByLayerInventory untouched;
            memset(&untouched,0xa5,sizeof(untouched));
            const MusaDwgAc1032CircleByLayerInventory before=untouched;
            ok=!musa_dwg_ac1032_circle_bylayer_inventory(NULL,&untouched)&&
               !memcmp(&before,&untouched,sizeof(before))&&
               !musa_dwg_ac1032_circle_bylayer_inventory(scene,NULL);
        }
    }
    /* Genuine R2018 source-ACI and WCS analytic envelope, not paint.
     * Companion DXF independently supplies H1BB/H8A and ACI4.
     * The geometry extrema must contain all affine-circle samples. */
    if(ok){
        MusaDwgAc1032CircleByLayerInventory inventory={0};
        ok=musa_dwg_ac1032_circle_bylayer_inventory(scene,&inventory)&&
           inventory.verified_source_color_count>=1u;
        unsigned matches=0u;
        for(size_t i=0u;ok&&i<inventory.verified_source_color_count;i++){
            const MusaDwgAc1032CircleByLayerInventoryRow *row=
                &inventory.rows[i];
            MusaDwgAc1032CircleSourceEnvelope envelope={0};
            ok=musa_dwg_ac1032_circle_source_envelope(
                   &inventory,i,&envelope)&&
               envelope.insert_handle==row->insert_handle&&
               envelope.block_header_handle==row->block_header_handle&&
               envelope.circle_handle==row->circle_handle&&
               envelope.layer_handle==row->layer_handle&&
               envelope.original_layer_aci==row->original_layer_aci&&
               envelope.original_layer_packed_values==
                 row->original_layer_packed_values&&
               envelope.source_crc_wcs_and_layer_aci_verified&&
               !envelope.final_visible_paint_accepted&&
               envelope.unresolved_original_dwg_requirements==
                 (MUSA_CIRCLE_DWG_UNRESOLVED_LAYER_BITS|
                  MUSA_CIRCLE_DWG_UNRESOLVED_OWNER_SPACE|
                  MUSA_CIRCLE_DWG_UNRESOLVED_NESTED_INSERT|
                  MUSA_CIRCLE_DWG_UNRESOLVED_VIEWPORT|
                  MUSA_CIRCLE_DWG_UNRESOLVED_XREF);
            for(unsigned k=0u;ok&&k<2u;k++){
                const double r=hypot(row->world_axis_u[k],
                                     row->world_axis_v[k]);
                ok=isfinite(r)&&
                   envelope.min_xy[k]<=row->world_center[k]-r&&
                   envelope.max_xy[k]>=row->world_center[k]+r;
            }
            for(unsigned t=0u;ok&&t<128u;t++){
                const double a=6.2831853071795864769*(double)t/128.0;
                for(unsigned k=0u;ok&&k<2u;k++){
                    const double p=row->world_center[k]+
                        row->world_axis_u[k]*cos(a)+
                        row->world_axis_v[k]*sin(a);
                    ok=isfinite(p)&&p>=envelope.min_xy[k]-1e-7&&
                       p<=envelope.max_xy[k]+1e-7;
                }
            }
            if(ok){
                MusaDwgAc1032CircleSourceEnvelope again={0};
                ok=musa_dwg_ac1032_circle_source_envelope(
                     &inventory,i,&again)&&
                   !memcmp(&again,&envelope,sizeof(again));
            }
            if(row->insert_handle==UINT64_C(0x1c8)&&
               row->circle_handle==UINT64_C(0x1bb)){
                matches++;
                ok=ok&&row->block_header_handle==UINT64_C(0x1b8)&&
                   row->layer_handle==UINT64_C(0x8a)&&
                   envelope.original_layer_aci==(uint8_t)oracle.aci;
            }
        }
        if(ok)ok=matches==1u;
        if(ok){
            MusaDwgAc1032CircleSourceEnvelope untouched;
            memset(&untouched,0xa5,sizeof(untouched));
            const MusaDwgAc1032CircleSourceEnvelope before=untouched;
            MusaDwgAc1032CircleByLayerInventory invalid=inventory;
            ok=!musa_dwg_ac1032_circle_source_envelope(
                    NULL,0u,&untouched)&&
               !memcmp(&untouched,&before,sizeof(before))&&
               !musa_dwg_ac1032_circle_source_envelope(
                    &inventory,inventory.verified_source_color_count,
                    &untouched)&&
               !memcmp(&untouched,&before,sizeof(before))&&
               !musa_dwg_ac1032_circle_source_envelope(
                    &inventory,0u,NULL);
            invalid.rows[0].original_layer_aci=0u;
            if(ok)ok=!musa_dwg_ac1032_circle_source_envelope(
                         &invalid,0u,&untouched)&&
                      !memcmp(&untouched,&before,sizeof(before));
            invalid=inventory;
            invalid.rows[0].world_axis_u[0]=NAN;
            if(ok)ok=!musa_dwg_ac1032_circle_source_envelope(
                         &invalid,0u,&untouched)&&
                      !memcmp(&untouched,&before,sizeof(before));
            invalid=inventory;
            invalid.rows[0].world_center[1]=INFINITY;
            if(ok)ok=!musa_dwg_ac1032_circle_source_envelope(
                         &invalid,0u,&untouched)&&
                      !memcmp(&untouched,&before,sizeof(before));
            invalid=inventory;
            invalid.rows[0].final_visible_paint_accepted=1u;
            if(ok)ok=!musa_dwg_ac1032_circle_source_envelope(
                         &invalid,0u,&untouched)&&
                      !memcmp(&untouched,&before,sizeof(before));
            invalid=inventory;
            invalid.original_source_inventory_only=0u;
            if(ok)ok=!musa_dwg_ac1032_circle_source_envelope(
                         &invalid,0u,&untouched)&&
                      !memcmp(&untouched,&before,sizeof(before));
        }
    }
    /* Per-source-layer WCS unions from the SAME genuine immutable R2018
     * scene; independent companion DXF verifies H8A ACI4, never visibility. */
    if(ok){
        MusaDwgAc1032CircleByLayerInventory inventory={0};
        MusaDwgAc1032CircleSourceLayerSpatialIndex spatial={0};
        ok=musa_dwg_ac1032_circle_bylayer_inventory(scene,&inventory)&&
           musa_dwg_ac1032_circle_source_layer_spatial_index(
             &inventory,&spatial)&&spatial.source_only&&
           !spatial.final_visible_paint_accepted&&
           spatial.source_layer_count>=1u&&
           spatial.verified_instance_count==
             inventory.verified_source_color_count&&
           spatial.unresolved_circle_count==
             inventory.unresolved_circle_count&&
           spatial.unresolved_insert_count==
             inventory.unresolved_insert_count&&
           spatial.unresolved_original_dwg_requirements==
             (MUSA_CIRCLE_DWG_UNRESOLVED_LAYER_BITS|
              MUSA_CIRCLE_DWG_UNRESOLVED_OWNER_SPACE|
              MUSA_CIRCLE_DWG_UNRESOLVED_NESTED_INSERT|
              MUSA_CIRCLE_DWG_UNRESOLVED_VIEWPORT|
              MUSA_CIRCLE_DWG_UNRESOLVED_XREF);
        unsigned match=0u;
        size_t total=0u;
        for(size_t j=0u;ok&&j<spatial.source_layer_count;j++){
            const MusaDwgAc1032CircleSourceLayerSpatialRow *r=
                &spatial.rows[j];
            ok=r->layer_handle&&r->original_layer_aci&&
               r->verified_instance_count>0u;
            total+=r->verified_instance_count;
            if(r->layer_handle==UINT64_C(0x8a)){
                match++;
                ok=ok&&r->original_layer_aci==(uint8_t)oracle.aci;
            }
            for(size_t i=0u;ok&&i<inventory.verified_source_color_count;i++){
                MusaDwgAc1032CircleSourceEnvelope e={0};
                ok=musa_dwg_ac1032_circle_source_envelope(
                       &inventory,i,&e);
                if(ok&&e.layer_handle==r->layer_handle){
                    ok=e.original_layer_aci==r->original_layer_aci&&
                       e.original_layer_packed_values==
                         r->original_layer_packed_values;
                    for(unsigned k=0u;ok&&k<2u;k++)
                        ok=r->min_xy[k]<=e.min_xy[k]&&
                           r->max_xy[k]>=e.max_xy[k];
                }
            }
        }
        if(ok)ok=match==1u&&total==spatial.verified_instance_count;
        if(ok){
            MusaDwgAc1032CircleSourceLayerSpatialIndex again={0};
            ok=musa_dwg_ac1032_circle_source_layer_spatial_index(
                 &inventory,&again)&&
               !memcmp(&again,&spatial,sizeof(again));
        }
        if(ok){
            MusaDwgAc1032CircleSourceLayerSpatialIndex untouched;
            memset(&untouched,0xa5,sizeof(untouched));
            const MusaDwgAc1032CircleSourceLayerSpatialIndex before=
                untouched;
            MusaDwgAc1032CircleByLayerInventory invalid=inventory;
            ok=!musa_dwg_ac1032_circle_source_layer_spatial_index(
                    NULL,&untouched)&&
               !memcmp(&before,&untouched,sizeof(before))&&
               !musa_dwg_ac1032_circle_source_layer_spatial_index(
                    &inventory,NULL);
            invalid.rows[0].original_layer_aci=0u;
            if(ok)ok=!musa_dwg_ac1032_circle_source_layer_spatial_index(
                        &invalid,&untouched)&&
                      !memcmp(&before,&untouched,sizeof(before));
            invalid=inventory;
            invalid.rows[0].world_center[0]=NAN;
            if(ok)ok=!musa_dwg_ac1032_circle_source_layer_spatial_index(
                        &invalid,&untouched)&&
                      !memcmp(&before,&untouched,sizeof(before));
            if(ok&&inventory.verified_source_color_count<
                    MUSACAD_AC1032_CIRCLE_BYLAYER_INVENTORY_MAX){
                invalid=inventory;
                const size_t n=invalid.verified_source_color_count;
                invalid.rows[n]=invalid.rows[0];
                invalid.rows[n].original_layer_aci=
                    invalid.rows[0].original_layer_aci==4u?5u:4u;
                invalid.verified_source_color_count++;
                invalid.circle_candidate_count++;
                ok=!musa_dwg_ac1032_circle_source_layer_spatial_index(
                     &invalid,&untouched)&&
                   !memcmp(&before,&untouched,sizeof(before));
            }
        }
    }

    /* Adaptive first-party C11 geometry from ORIGINAL R2018 H1C8/H1BB.
     * Independent DXF supplies the ACI4 oracle, not visibility authority.
     * Check true affine-curve midpoints against every emitted chord. */
    if(ok){
        MusaDwgAc1032CircleByLayerInventory inventory={0};
        ok=musa_dwg_ac1032_circle_bylayer_inventory(scene,&inventory);
        unsigned matches=0u;
        for(size_t i=0u;ok&&i<inventory.verified_source_color_count;i++){
            const MusaDwgAc1032CircleByLayerInventoryRow *row=
                &inventory.rows[i];
            MusaDwgAc1032CircleAdaptiveSourcePolyline poly={0};
            MusaDwgAc1032CircleAdaptiveSourcePolyline finer={0};
            ok=musa_dwg_ac1032_circle_adaptive_source_polyline(
                &inventory,i,0.5,&poly)&&
               musa_dwg_ac1032_circle_adaptive_source_polyline(
                &inventory,i,0.125,&finer)&&
               poly.source_only&&!poly.final_visible_paint_accepted&&
               poly.segment_count>=8u&&
               poly.segment_count<=MUSA_AC1032_CIRCLE_ADAPTIVE_MAX_SEGMENTS&&
               finer.segment_count>=poly.segment_count&&
               poly.conservative_xy_chord_error_bound<=0.5&&
               finer.conservative_xy_chord_error_bound<=0.125&&
               poly.unresolved_original_dwg_requirements==
                 (MUSA_CIRCLE_DWG_UNRESOLVED_LAYER_BITS|
                  MUSA_CIRCLE_DWG_UNRESOLVED_OWNER_SPACE|
                  MUSA_CIRCLE_DWG_UNRESOLVED_NESTED_INSERT|
                  MUSA_CIRCLE_DWG_UNRESOLVED_VIEWPORT|
                  MUSA_CIRCLE_DWG_UNRESOLVED_XREF)&&
               poly.xy[0][0]==poly.xy[poly.segment_count][0]&&
               poly.xy[0][1]==poly.xy[poly.segment_count][1];
            for(size_t j=0u;ok&&j<poly.segment_count;j++){
                const double a=6.2831853071795864769*
                    ((double)j+0.5)/(double)poly.segment_count;
                const double x=row->world_center[0]+
                    row->world_axis_u[0]*cos(a)+
                    row->world_axis_v[0]*sin(a);
                const double y=row->world_center[1]+
                    row->world_axis_u[1]*cos(a)+
                    row->world_axis_v[1]*sin(a);
                const double midx=(poly.xy[j][0]+poly.xy[j+1u][0])*0.5;
                const double midy=(poly.xy[j][1]+poly.xy[j+1u][1])*0.5;
                ok=hypot(x-midx,y-midy)<=
                    poly.conservative_xy_chord_error_bound+1e-7;
                for(unsigned k=0u;ok&&k<2u;k++)
                    ok=poly.xy[j][k]>=poly.min_xy[k]&&
                       poly.xy[j][k]<=poly.max_xy[k];
            }
            if(row->insert_handle==UINT64_C(0x1c8)&&
               row->circle_handle==UINT64_C(0x1bb)){
                matches++;
                ok=ok&&poly.layer_handle==UINT64_C(0x8a)&&
                   poly.original_layer_aci==(uint8_t)oracle.aci;
            }
            if(ok){
                MusaDwgAc1032CircleAdaptiveSourcePolyline repeat={0};
                ok=musa_dwg_ac1032_circle_adaptive_source_polyline(
                    &inventory,i,0.5,&repeat)&&
                   !memcmp(&repeat,&poly,sizeof(poly));
            }
        }
        if(ok)ok=matches==1u;
        if(ok){
            MusaDwgAc1032CircleAdaptiveSourcePolyline untouched;
            memset(&untouched,0xa5,sizeof(untouched));
            const MusaDwgAc1032CircleAdaptiveSourcePolyline before=
                untouched;
            MusaDwgAc1032CircleByLayerInventory invalid=inventory;
            ok=!musa_dwg_ac1032_circle_adaptive_source_polyline(
                    &inventory,0u,0.0,&untouched)&&
               !memcmp(&before,&untouched,sizeof(before))&&
               !musa_dwg_ac1032_circle_adaptive_source_polyline(
                    &inventory,0u,NAN,&untouched)&&
               !memcmp(&before,&untouched,sizeof(before))&&
               !musa_dwg_ac1032_circle_adaptive_source_polyline(
                    &inventory,0u,1e-12,&untouched)&&
               !memcmp(&before,&untouched,sizeof(before))&&
               !musa_dwg_ac1032_circle_adaptive_source_polyline(
                    &inventory,0u,0.5,NULL);
            invalid.rows[0].original_layer_aci=0u;
            if(ok)ok=!musa_dwg_ac1032_circle_adaptive_source_polyline(
                        &invalid,0u,0.5,&untouched)&&
                      !memcmp(&before,&untouched,sizeof(before));
            invalid=inventory;
            invalid.rows[0].world_axis_u[0]=INFINITY;
            if(ok)ok=!musa_dwg_ac1032_circle_adaptive_source_polyline(
                        &invalid,0u,0.5,&untouched)&&
                      !memcmp(&before,&untouched,sizeof(before));
        }
    }

    /* Authentic R2018 adaptive v6 packet: geometry is original affine WCS,
     * but v6 ACI6 is explicitly DIAGNOSTIC, not original source ACI4. */
    if(ok){
        MusaDwgAc1032CircleByLayerInventory inventory={0};
        ok=musa_dwg_ac1032_circle_bylayer_inventory(scene,&inventory);
        unsigned matches=0u;
        for(size_t i=0u;ok&&i<inventory.verified_source_color_count;i++){
            float words[MUSA_AC1032_CIRCLE_ADAPTIVE_V6_MAX_WORDS]={0};
            float again[MUSA_AC1032_CIRCLE_ADAPTIVE_V6_MAX_WORDS]={0};
            MusaDwgAc1032CircleAdaptiveV6Evidence proof={0},repeat={0};
            ok=musa_dwg_ac1032_circle_adaptive_diagnostic_v6(
                 &inventory,i,0.5,words,
                 MUSA_AC1032_CIRCLE_ADAPTIVE_V6_MAX_WORDS,&proof)&&
               musa_dwg_ac1032_circle_adaptive_diagnostic_v6(
                 &inventory,i,0.5,again,
                 MUSA_AC1032_CIRCLE_ADAPTIVE_V6_MAX_WORDS,&repeat)&&
               !memcmp(&proof,&repeat,sizeof(proof))&&
               !memcmp(words,again,proof.float_word_count*sizeof(float))&&
               proof.source_only&&!proof.original_color_or_final_paint_accepted&&
               proof.diagnostic_packet_aci==6u&&
               proof.original_layer_aci==inventory.rows[i].original_layer_aci&&
               proof.float_word_count==7u+2u+2u*(proof.segment_count+1u)&&
               words[0]==6.0f&&words[1]==1.0f&&words[6]==1.0f&&
               words[7]==518.0f&&words[8]==(float)(proof.segment_count+1u)&&
               proof.segment_count>=3u&&
               words[9]==words[9u+2u*proof.segment_count]&&
               words[10]==words[10u+2u*proof.segment_count];
            for(size_t j=0u;ok&&j<=proof.segment_count;j++)
                ok=isfinite(words[9u+2u*j])&&
                   isfinite(words[10u+2u*j])&&
                   words[9u+2u*j]>=words[2]&&
                   words[9u+2u*j]<=words[4]&&
                   words[10u+2u*j]>=words[3]&&
                   words[10u+2u*j]<=words[5];
            /* Independently verify all float32 vertices lie inside the
             * outward source bounds of the genuine R2018 v6 packet. */
            for(size_t j=0u;ok&&j<=proof.segment_count;j++){
                const float x=words[9u+2u*j],y=words[10u+2u*j];
                ok=x>=words[2]&&x<=words[4]&&
                   y>=words[3]&&y<=words[5];
            }
            /* Verify the float32 transport budget independently from the
             * source R2018 WCS tessellation and its DXF-backed oracle. */
            if(ok){
                MusaDwgAc1032CircleAdaptiveSourcePolyline source={0};
                ok=musa_dwg_ac1032_circle_adaptive_source_polyline(
                    &inventory,i,0.5,&source)&&
                   source.segment_count==proof.segment_count;
                double max_displacement=0.0;
                for(size_t j=0u;ok&&j<=source.segment_count;j++){
                    const double dx=(double)words[9u+2u*j]-source.xy[j][0];
                    const double dy=(double)words[10u+2u*j]-source.xy[j][1];
                    const double distance=hypot(dx,dy);
                    if(!isfinite(distance))ok=0;
                    else if(distance>max_displacement)
                        max_displacement=distance;
                }
                /* Compare the *emitted* closing seam against the source
                 * final double vertex, not only the source start point. */
                if(ok){
                    const size_t last=source.segment_count;
                    const double sx=(double)words[9u+2u*last]-
                                    source.xy[last][0];
                    const double sy=(double)words[10u+2u*last]-
                                    source.xy[last][1];
                    const double seam_displacement=hypot(sx,sy);
                    ok=words[9u+2u*last]==words[9u]&&
                       words[10u+2u*last]==words[10u]&&
                       isfinite(seam_displacement)&&
                       proof.max_float32_vertex_displacement>=
                       seam_displacement;
                }
                if(ok)ok=proof.max_float32_vertex_displacement>=
                           max_displacement&&
                          proof.combined_world_xy_error_bound>=
                           proof.conservative_world_xy_error_bound+
                           proof.max_float32_vertex_displacement&&
                          proof.combined_world_xy_error_bound<=0.5;
            }
            /* Independent authentic R2018 gold: transported geometry must
             * retain at least two distinct float32 vertices. */
            if(ok){
                int distinct=0;
                for(size_t j=1u;j<proof.segment_count;j++)
                    if(words[9u+2u*j]!=words[9u]||
                       words[10u+2u*j]!=words[10u])distinct=1;
                ok=distinct;
            }
            if(inventory.rows[i].insert_handle==UINT64_C(0x1c8)&&
               inventory.rows[i].circle_handle==UINT64_C(0x1bb)){
                matches++;
                ok=ok&&proof.layer_handle==UINT64_C(0x8a)&&
                   proof.original_layer_aci==(uint8_t)oracle.aci&&
                   proof.diagnostic_packet_aci!=proof.original_layer_aci;
            }
            if(ok){
                float untouched[MUSA_AC1032_CIRCLE_ADAPTIVE_V6_MAX_WORDS];
                memset(untouched,0xa5,sizeof(untouched));
                float before[MUSA_AC1032_CIRCLE_ADAPTIVE_V6_MAX_WORDS];
                memcpy(before,untouched,sizeof(before));
                MusaDwgAc1032CircleAdaptiveV6Evidence metadata;
                memset(&metadata,0xa5,sizeof(metadata));
                const MusaDwgAc1032CircleAdaptiveV6Evidence meta_before=
                    metadata;
                ok=!musa_dwg_ac1032_circle_adaptive_diagnostic_v6(
                      &inventory,i,0.5,untouched,proof.float_word_count-1u,
                      &metadata)&&
                   !memcmp(untouched,before,sizeof(before))&&
                   !memcmp(&metadata,&meta_before,sizeof(metadata))&&
                   !musa_dwg_ac1032_circle_adaptive_diagnostic_v6(
                      &inventory,i,0.0,untouched,
                      MUSA_AC1032_CIRCLE_ADAPTIVE_V6_MAX_WORDS,&metadata)&&
                   !memcmp(untouched,before,sizeof(before))&&
                   !memcmp(&metadata,&meta_before,sizeof(metadata));
                MusaDwgAc1032CircleByLayerInventory invalid=inventory;
                invalid.rows[i].original_layer_aci=0u;
                if(ok)ok=!musa_dwg_ac1032_circle_adaptive_diagnostic_v6(
                           &invalid,i,0.5,untouched,
                           MUSA_AC1032_CIRCLE_ADAPTIVE_V6_MAX_WORDS,
                           &metadata)&&
                          !memcmp(untouched,before,sizeof(before))&&
                          !memcmp(&metadata,&meta_before,sizeof(metadata));
                /* Synthetic negative, NOT authentic fixture evidence:
                 * at 1e9, float32 loses radius-1 geometry. Fail atomically. */
                MusaDwgAc1032CircleByLayerInventory shifted=inventory;
                for(unsigned k=0u;k<3u;k++){
                    shifted.rows[i].world_center[k]=k<2u?1000000000.25:0.0;
                    shifted.rows[i].world_axis_u[k]=k==0u?1.0:0.0;
                    shifted.rows[i].world_axis_v[k]=k==1u?1.0:0.0;
                }
                if(ok)ok=!musa_dwg_ac1032_circle_adaptive_diagnostic_v6(
                           &shifted,i,0.5,untouched,
                           MUSA_AC1032_CIRCLE_ADAPTIVE_V6_MAX_WORDS,
                           &metadata)&&
                          !memcmp(untouched,before,sizeof(before))&&
                          !memcmp(&metadata,&meta_before,sizeof(metadata));
            }
        }
        if(ok)ok=matches==1u;
    }
    fprintf(stderr,"%s original DWG vs independent DXF signed LAYER oracle, group70=%ld "
            "is separate; viewport/paint UNKNOWN\n",
            ok?"PASS":"FAIL",oracle.group70);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=3)return 2;
    return run(argv[1],argv[2])?0:1;
}
