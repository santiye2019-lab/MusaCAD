/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Genuine unchanged R2018 example_2018.dwg vs independent companion DXF:
 * INSERT H1C8/H190/H756, LAYER/BLOCK_RECORD, ATTRIB and SEQEND handles.
 * Source H/CRC and insertion-point math only. No nested block expansion or visible scene claim. */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_insert_binding.h"
#include "musacad_dwg_ac1032_block_header_identity.h"
#include "musacad_dwg_ac1032_insert_transform.h"
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct Expected {
    uint64_t h,layer,block,attrib,seqend;
    uint32_t count;
} Expected;
static int genuine(const char *file){
    FILE *f=fopen(file,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    const long n=ftell(f);
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *src=(uint8_t*)malloc((size_t)n);
    uint8_t *saved=(uint8_t*)malloc((size_t)n);
    if(!src||!saved){free(src);free(saved);fclose(f);return 0;}
    int ok=fread(src,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f))ok=0;
    if(ok)memcpy(saved,src,(size_t)n);
    ok=ok&&!memcmp(src,"AC1032",6u);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
        src,(size_t)n,&scene)&&!memcmp(src,saved,(size_t)n);
    free(src);free(saved);
    if(!ok)return 0;
    /* Handles from independent DXF AcDbBlockReference / BLOCK_RECORD,
     * ATTRIB and SEQEND group 5, never guessed from adjacent offsets. */
    const Expected expected[]={
        {UINT64_C(0x1c8),UINT64_C(0x8a),UINT64_C(0x1b8),0u,0u,0u},
        {UINT64_C(0x190),UINT64_C(0x8a),UINT64_C(0x17d),UINT64_C(0x192),UINT64_C(0x191),1u},
        {UINT64_C(0x756),UINT64_C(0x8a),UINT64_C(0x17d),UINT64_C(0x757),UINT64_C(0x759),1u}
    };
    for(size_t i=0u;ok&&i<sizeof(expected)/sizeof(expected[0]);i++){
        MusaDwgAc1032InsertSourceGraph g={0};
        const Expected *e=&expected[i];
        ok=musa_dwg_ac1032_scene_index_insert_source_graph(
            scene,e->h,&g)&&g.indexed_handle==e->h&&
            g.layer_handle==e->layer&&
            g.block_header_handle==e->block&&
            g.attrib_handle==e->attrib&&g.seqend_handle==e->seqend&&
            g.owned_attrib_count==e->count&&
            g.original_entity_and_targets_crc_verified&&
            g.source_edges_verified&&!g.block_expansion_and_display_accepted;
        if(ok){
            MusaDwgAc1032InsertBinding binding={0};
            ok=musa_dwg_ac1032_insert_bind_source(scene,e->h,&binding)&&
               binding.source_binding_verified&&
               !binding.block_expansion_accepted&&
               binding.graph.block_header_handle==e->block&&
               binding.graph.layer_handle==e->layer&&
               binding.placement.indexed_handle==e->h;
        }
        if(ok){
            MusaDwgAc1032InsertBinding binding={0};
            MusaDwgAc1032BlockBaseEvidence missing={0};
            double world[3]={123.0,456.0,789.0};
            const double original_world[3]={123.0,456.0,789.0};
            const double local[3]={0.0,0.0,0.0};
            ok=musa_dwg_ac1032_insert_bind_source(
                   scene,e->h,&binding)&&
               !musa_dwg_ac1032_insert_map_verified_base(
                   &binding,&missing,local,world)&&
               !memcmp(world,original_world,sizeof(world));
            /* A foreign BLOCK_HEADER H must never authorize geometry.
             * This evidence is synthetic and deliberately not accepted. */
            missing.block_header_handle=e->block+1u;
            missing.original_block_header_crc_verified=1u;
            missing.base_point_decoded=1u;
            if(ok)ok=!musa_dwg_ac1032_insert_map_verified_base(
                         &binding,&missing,local,world)&&
                     !memcmp(world,original_world,sizeof(world));
        }
        if(ok){
            MusaDwgAc1032BlockHeaderIdentity block={0};
            ok=musa_dwg_ac1032_scene_index_read_block_header_identity(
                   scene,e->block,&block)&&
               block.indexed_handle==e->block&&
               block.original_record_crc_verified&&
               block.embedded_self_handle_verified&&
               block.empty_eed_verified&&
               block.no_reactors_verified&&
               !block.base_point_decoded;
        }
        if(ok){
            MusaDwgAc1032InsertBlockHeaderSource joined={0};
            ok=musa_dwg_ac1032_insert_bind_block_header_source(
                   scene,e->h,&joined)&&
               joined.source_join_verified&&
               joined.insert.graph.indexed_handle==e->h&&
               joined.block_header.indexed_handle==e->block&&
               joined.block_header.original_record_crc_verified&&
               !joined.block_base_decoded&&
               !joined.block_header.base_point_decoded;
            /* The wrong object type must not mutate the caller output. */
            if(ok){
                const MusaDwgAc1032InsertBlockHeaderSource before=joined;
                ok=!musa_dwg_ac1032_insert_bind_block_header_source(
                       scene,e->block,&joined)&&
                   !memcmp(&before,&joined,sizeof(joined));
            }
        }
        if(ok){
            MusaDwgAc1032BlockHeaderPrefixEvidence prefix={0};
            ok=musa_dwg_ac1032_scene_index_read_block_header_prefix_evidence(
                   scene,e->block,&prefix)&&
               prefix.indexed_handle==e->block&&
               prefix.original_record_crc_verified&&
               prefix.fields_start_bit>10u&&
               prefix.prefix_fnv1a64!=0u&&
               !prefix.base_point_decoded;
            /* Repeated extraction from immutable original bytes is
             * deterministic, with the same verified handle and prefix. */
            if(ok){
                MusaDwgAc1032BlockHeaderPrefixEvidence repeat={0};
                ok=musa_dwg_ac1032_scene_index_read_block_header_prefix_evidence(
                       scene,e->block,&repeat)&&
                   repeat.indexed_handle==prefix.indexed_handle&&
                   repeat.fields_start_bit==prefix.fields_start_bit&&
                   repeat.prefix_fnv1a64==prefix.prefix_fnv1a64&&
                   repeat.original_record_crc_verified&&
                   !repeat.base_point_decoded;
            }
            if(ok){
                const MusaDwgAc1032BlockHeaderPrefixEvidence before=prefix;
                ok=!musa_dwg_ac1032_scene_index_read_block_header_prefix_evidence(
                        scene,e->h,&prefix)&&
                   !memcmp(&prefix,&before,sizeof(prefix));
            }
        }
        if(ok){
            MusaDwgAc1032BlockHeaderFieldSpan span={0};
            ok=musa_dwg_ac1032_scene_index_read_block_header_field_span(
                   scene,e->block,&span)&&
               span.indexed_handle==e->block&&
               span.original_record_crc_verified&&
               span.first_bit>10u&&
               span.first_bit<span.end_bit&&
               !span.base_point_decoded;
            if(ok){
                const MusaDwgAc1032BlockHeaderFieldSpan before=span;
                ok=!musa_dwg_ac1032_scene_index_read_block_header_field_span(
                        scene,e->h,&span)&&
                   !memcmp(&before,&span,sizeof(span));
            }
        }
        if(ok){
            /* Genuine R2018: first authenticated field bits must be
             * reproducible, without claiming that they encode XYZ. */
            MusaDwgAc1032BlockHeaderFieldSpan span={0};
            if(ok)ok=musa_dwg_ac1032_scene_index_read_block_header_field_span(
                   scene,e->block,&span);
            if(ok){
                MusaDwgAc1032BlockHeaderFieldSpan repeat={0};
                ok=musa_dwg_ac1032_scene_index_read_block_header_field_span(
                       scene,e->block,&repeat)&&
                   repeat.indexed_handle==span.indexed_handle&&
                   repeat.first_bit==span.first_bit&&
                   repeat.end_bit==span.end_bit&&
                   repeat.original_record_crc_verified&&
                   !repeat.base_point_decoded;
            }
        }
        if(ok){
            /* Original R2018 H1B8/H17D: the same source DATA bits must
             * agree for 16-bit and two 8-bit reads. This is an exact
             * binary extraction oracle, NOT a decoded block base. */
            MusaDwgAc1032BlockHeaderFieldSpan span={0};
            uint8_t pair[2]={0xa5u,0x5au};
            uint8_t hi=0u,lo=0u;
            ok=musa_dwg_ac1032_scene_index_read_block_header_field_span(
                   scene,e->block,&span)&&
               span.end_bit-span.first_bit>=16u&&
               musa_dwg_ac1032_scene_index_copy_block_header_field_bits(
                   scene,e->block,span.first_bit,16u,pair,sizeof(pair))&&
               musa_dwg_ac1032_scene_index_copy_block_header_field_bits(
                   scene,e->block,span.first_bit,8u,&hi,1u)&&
               musa_dwg_ac1032_scene_index_copy_block_header_field_bits(
                   scene,e->block,span.first_bit+8u,8u,&lo,1u)&&
               pair[0]==hi&&pair[1]==lo;
            if(ok){
                const uint8_t before[2]={pair[0],pair[1]};
                ok=!musa_dwg_ac1032_scene_index_copy_block_header_field_bits(
                       scene,e->h,span.first_bit,16u,pair,sizeof(pair))&&
                   !memcmp(pair,before,sizeof(pair))&&
                   !musa_dwg_ac1032_scene_index_copy_block_header_field_bits(
                       scene,e->block,span.end_bit,16u,pair,sizeof(pair))&&
                   !memcmp(pair,before,sizeof(pair));
            }
        }
        /* A verified source H graph must agree with the independently
         * decoded INSERT DATA attribute flag/count for the same handle.
         * This is source consistency, not block ownership or rendering. */
        if(ok){
            MusaDwgAc1032InsertPlacement p={0};
            ok=musa_dwg_ac1032_scene_index_read_insert_placement(
                 scene,e->h,&p)&&p.indexed_handle==e->h&&
               p.original_record_crc_verified&&p.numeric_fields_decoded&&
               p.owned_attrib_count==g.owned_attrib_count&&
               p.has_attribs==(uint8_t)(e->count!=0u)&&
               !p.block_graph_and_display_accepted;
            /* Genuine source INSERT + independent transform oracle:
             * identity block base is synthetic; not yet a BLOCK_HEADER
             * base-point or final scene-geometry verification. */
            if(ok&&e->h==UINT64_C(0x1c8)){
                const double base[3]={0.0,0.0,0.0};
                const double origin[3]={0.0,0.0,0.0};
                double mapped[3]={NAN,NAN,NAN};
                ok=musa_dwg_ac1032_insert_xy_world_z(
                    &p,base,origin,mapped)&&
                    fabs(mapped[0]-p.insertion[0])<1e-9&&
                    fabs(mapped[1]-p.insertion[1])<1e-9&&
                    fabs(mapped[2]-p.insertion[2])<1e-9;
            }
        }
    }
    if(ok){
        MusaDwgAc1032InsertBinding unchanged_binding;
        memset(&unchanged_binding,0x5a,sizeof(unchanged_binding));
        const MusaDwgAc1032InsertBinding original_binding=unchanged_binding;
        ok=!musa_dwg_ac1032_insert_bind_source(
                scene,UINT64_MAX,&unchanged_binding)&&
           !memcmp(&unchanged_binding,&original_binding,
                   sizeof(unchanged_binding))&&
           !musa_dwg_ac1032_insert_bind_source(
                scene,UINT64_C(0x192),&unchanged_binding)&&
           !memcmp(&unchanged_binding,&original_binding,
                   sizeof(unchanged_binding))&&
           !musa_dwg_ac1032_insert_bind_source(
                scene,UINT64_C(0x1b8),&unchanged_binding)&&
           !memcmp(&unchanged_binding,&original_binding,
                   sizeof(unchanged_binding));
    }
    /* An OT49 parser must reject an unauthenticated payload before
     * attempting any BLOCK_HEADER base-point interpretation. */
    if(ok){
        const uint8_t fake[2]={0,0};
        MusaDwgAc1032ObjectStreams layout={0};
        MusaDwgAc1032BlockHeaderIdentity identity;
        memset(&identity,0x5a,sizeof(identity));
        const MusaDwgAc1032BlockHeaderIdentity saved_identity=identity;
        layout.indexed_handle=UINT64_C(0x1b8);
        layout.raw_object_type=49u;
        layout.object_type_field_bits=10u;
        layout.total_payload_bits=16u;
        layout.data_stream_bits=12u;
        layout.handle_stream_start_bit=12u;
        layout.handle_stream_bits=4u;
        ok=!musa_dwg_ac1032_block_header_ot49_identity(
              fake,sizeof(fake),&layout,&identity)&&
           !memcmp(&identity,&saved_identity,sizeof(identity));
        /* A CRC bit alone must not authorize an OT49 identity: a
         * mismatched object type and malformed prefix both fail closed. */
        layout.original_record_crc_verified=1u;
        if(ok)ok=!musa_dwg_ac1032_block_header_ot49_identity(
                    fake,sizeof(fake),&layout,&identity)&&
                  !memcmp(&identity,&saved_identity,sizeof(identity));
        layout.raw_object_type=51u;
        if(ok)ok=!musa_dwg_ac1032_block_header_ot49_identity(
                    fake,sizeof(fake),&layout,&identity)&&
                  !memcmp(&identity,&saved_identity,sizeof(identity));
        layout.raw_object_type=49u;
        /* Aliased output/payload is never accepted, even with a CRC
         * claim, and no bytes of the original buffer may be changed. */
        {
            uint8_t shared[sizeof(MusaDwgAc1032BlockHeaderIdentity)+2u];
            memset(shared,0x3c,sizeof(shared));
            uint8_t saved_shared[sizeof(shared)];
            memcpy(saved_shared,shared,sizeof(shared));
            if(ok)ok=!musa_dwg_ac1032_block_header_ot49_identity(
                 shared,sizeof(shared),&layout,
                 (MusaDwgAc1032BlockHeaderIdentity *)(void *)shared)&&
                 !memcmp(shared,saved_shared,sizeof(shared));
        }
        {
            MusaDwgAc1032BlockHeaderPrefixEvidence prefix;
            memset(&prefix,0x5a,sizeof(prefix));
            const MusaDwgAc1032BlockHeaderPrefixEvidence original=prefix;
            layout.original_record_crc_verified=0u;
            if(ok)ok=!musa_dwg_ac1032_block_header_ot49_prefix_evidence(
                  fake,sizeof(fake),&layout,&prefix)&&
                  !memcmp(&prefix,&original,sizeof(prefix));
            layout.original_record_crc_verified=1u;
            if(ok)ok=!musa_dwg_ac1032_block_header_ot49_prefix_evidence(
                  fake,sizeof(fake),&layout,&prefix)&&
                  !memcmp(&prefix,&original,sizeof(prefix));
        }
        {
            MusaDwgAc1032BlockHeaderFieldSpan span;
            memset(&span,0x5a,sizeof(span));
            const MusaDwgAc1032BlockHeaderFieldSpan before=span;
            if(ok)ok=!musa_dwg_ac1032_block_header_ot49_field_span(
                  fake,sizeof(fake),&layout,&span)&&
                  !memcmp(&span,&before,sizeof(span));
        }
        {
            uint8_t output[2]={0xa5u,0x5au};
            if(ok)ok=!musa_dwg_ac1032_block_header_ot49_copy_field_bits(
                   fake,sizeof(fake),&layout,10u,8u,output,sizeof(output))&&
                   output[0]==0xa5u&&output[1]==0x5au;
        }
        layout.handle_stream_start_bit=11u;
        if(ok)ok=!musa_dwg_ac1032_block_header_ot49_identity(
                    fake,sizeof(fake),&layout,&identity)&&
                  !memcmp(&identity,&saved_identity,sizeof(identity));
    }
    if(ok){
        MusaDwgAc1032InsertSourceGraph unchanged;
        memset(&unchanged,0x5a,sizeof(unchanged));
        const MusaDwgAc1032InsertSourceGraph saved=unchanged;
        ok=!musa_dwg_ac1032_scene_index_insert_source_graph(
             scene,UINT64_MAX,&unchanged)&&
           !memcmp(&unchanged,&saved,sizeof(unchanged))&&
           !musa_dwg_ac1032_scene_index_insert_source_graph(
             scene,UINT64_C(0x192),&unchanged)&&
           !memcmp(&unchanged,&saved,sizeof(unchanged))&&
           /* Reject valid, CRC-authenticated non-INSERT source types.
            * Every failed read must leave the caller's output intact. */
           !musa_dwg_ac1032_scene_index_insert_source_graph(
             scene,UINT64_C(0x8a),&unchanged)&&
           !memcmp(&unchanged,&saved,sizeof(unchanged))&&
           !musa_dwg_ac1032_scene_index_insert_source_graph(
             scene,UINT64_C(0x1b8),&unchanged)&&
           !memcmp(&unchanged,&saved,sizeof(unchanged))&&
           !musa_dwg_ac1032_scene_index_insert_source_graph(
             scene,UINT64_C(0x191),&unchanged)&&
           !memcmp(&unchanged,&saved,sizeof(unchanged));
    }
    fprintf(stderr,"%s genuine R2018 INSERT H1C8/H190/H756 source H graph and H1C8 origin transform; "
        "block expansion, owner, viewport, final paint UNVERIFIED\n",
        ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=2)return 2;
    return genuine(argv[1])?0:1;
}
