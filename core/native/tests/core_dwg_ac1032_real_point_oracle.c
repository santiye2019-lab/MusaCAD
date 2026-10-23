/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Immutable original AutoCAD R2018 POINT, external matching Point.txt:
 * handle 0x1C0, coordinates 27.928 8.65308 0, default normal/thickness.
 * This binary probe links ONLY the MusaCAD-owned shared C11 engine.
 */
#include "musacad_dwg_ac1032_point.h"
#include "musacad_dwg_ac1032_object_type.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1032_object_streams.h"
#include "musacad_dwg_ac1032_handles.h"
#include "musacad_dwg_ac1032_page_map.h"
#include "musacad_dwg_ac1032_sections.h"
#include "musacad_dwg_ac1018_lz.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_DWG (64u*1024u*1024u)
static int load(const char *path,uint8_t **out,size_t *n){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    const long length=ftell(f);
    if(length<256||length>(long)MAX_DWG||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
    uint8_t *p=(uint8_t*)malloc((size_t)length);
    if(!p){fclose(f);return 0;}
    int ok=fread(p,1u,(size_t)length,f)==(size_t)length;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(p);return 0;}
    *out=p;*n=(size_t)length;return 1;
}
static void stage_diagnostics(const uint8_t *file,size_t n,
                              uint64_t handle){
    MusaDwgAc1032FileMapHeader hdr={0};
    size_t map_n=0u,handle_n=0u,section_n=0u;
    int header=musa_dwg_ac1032_page_map_header(file,n,&hdr);
    int map=musa_dwg_ac1032_page_locations(
        file,n,NULL,0u,&map_n);
    int sections=musa_dwg_ac1032_section_catalog(
        file,n,NULL,0u,&section_n);
    int handles=musa_dwg_ac1032_handles_index(
        file,n,NULL,0u,&handle_n);
    fprintf(stderr,"Point native file levels: bytes=%zu header=%d"
      " page_map=%d pages=%zu sections_ok=%d sections=%zu"
      " handle_index_ok=%d handles=%zu\n",
      n,header,map,map_n,sections,section_n,handles,handle_n);
    if(header){
        fprintf(stderr,"Point physical header: count=%u map_id=%u"
            " secmap_id=%u map_offset=%llu pack=%u expanded=%u\n",
            hdr.page_count,hdr.page_map_id,hdr.section_map_id,
            (unsigned long long)hdr.page_map_offset,
            hdr.map_compressed_bytes,hdr.map_decoded_bytes);
        if(hdr.map_decoded_bytes>0u&&hdr.map_decoded_bytes<=65536u&&
           hdr.page_map_offset+20u+hdr.map_compressed_bytes<=n){
           uint8_t *plain=(uint8_t*)malloc(hdr.map_decoded_bytes);
           size_t read=0u;
           int decoded=plain&&musa_dwg_ac1018_lz_decode(
              file+(size_t)hdr.page_map_offset+20u,
              hdr.map_compressed_bytes,plain,hdr.map_decoded_bytes,&read);
           fprintf(stderr,"Point physical map: LZ=%d consumed=%zu\n",
              decoded,read);
           if(decoded){
             size_t at=0u;uint64_t pos=256u;
             while(at+8u<=hdr.map_decoded_bytes){
                uint32_t raw=(uint32_t)plain[at]|
                    ((uint32_t)plain[at+1u]<<8u)|
                    ((uint32_t)plain[at+2u]<<16u)|
                    ((uint32_t)plain[at+3u]<<24u);
                uint32_t span=(uint32_t)plain[at+4u]|
                    ((uint32_t)plain[at+5u]<<8u)|
                    ((uint32_t)plain[at+6u]<<16u)|
                    ((uint32_t)plain[at+7u]<<24u);
                int gap=(raw&UINT32_C(0x80000000))!=0;
                size_t stride=gap?24u:8u;
                fprintf(stderr,"Point physical idx: raw_id=%08X"
                    " span=%u from=%llu gap=%d last=%d\n",
                    raw,span,(unsigned long long)pos,gap,
                    (at+stride==hdr.map_decoded_bytes));
                if(hdr.map_decoded_bytes-at<stride)break;
                pos+=span;at+=stride;
             }
             fprintf(stderr,"Point physical end=%llu size=%zu"
                 " delta=%lld copy108_at_end=%d\n",
                 (unsigned long long)pos,n,
                 (long long)n-(long long)pos,
                 n>=108u&&memcmp(file+0x80u,file+n-108u,108u)==0);
             if(n>=128u){
                fprintf(stderr,"Point original last 128 bytes:");
                for(size_t i=n-128u;i<n;i++)
                   fprintf(stderr," %02X",file[i]);
                fputc('\n',stderr);
             }
           }
           free(plain);
        }
    }
    if(handles&&handle_n>0u&&handle_n<=100000u){
        MusaDwgAc1032HandleEntry *entries=calloc(handle_n,sizeof(*entries));
        size_t got=0u;
        if(entries&&musa_dwg_ac1032_handles_index(
           file,n,entries,handle_n,&got)){
            uint64_t closest=0u;
            size_t matched=0u;
            for(size_t i=0;i<got;i++){
                if(entries[i].handle==handle)matched++;
                if(entries[i].handle<=handle &&
                   entries[i].handle>closest)closest=entries[i].handle;
            }
            fprintf(stderr,"Point native index: desired=%llX"
                " matches=%zu closest_below=%llX count=%zu\n",
                (unsigned long long)handle,matched,
                (unsigned long long)closest,got);
        }
        free(entries);
    }
    MusaDwgAc1032ObjectType ot={0};
    MusaDwgAc1032PayloadInfo info={0};
    MusaDwgAc1032ObjectStreams layout={0};
    size_t wanted=0u;
    int o=musa_dwg_ac1032_object_type_file(file,n,handle,&ot);
    int p=musa_dwg_ac1032_object_payload_file(
          file,n,handle,NULL,0u,&wanted,&info);
    int l=musa_dwg_ac1032_stream_layout_file(file,n,handle,&layout);
    fprintf(stderr,"Point original stages: OT_ok=%d OT=%u"
      " CRC=%u payload_ok=%d wanted=%zu"
      " metaOT=%u indexed=%llX layout_ok=%d"
      " payload_bits=%llu DATA_bits=%llu HANDLES_bits=%llu\n",
      o,ot.type_code,(unsigned)ot.record_crc_verified,p,wanted,
      info.raw_object_type,(unsigned long long)info.indexed_handle,l,
      (unsigned long long)layout.total_payload_bits,
      (unsigned long long)layout.data_stream_bits,
      (unsigned long long)layout.handle_stream_bits);
    if(!p||!l||wanted==0u||wanted>2048u)return;
    uint8_t *raw=(uint8_t*)malloc(wanted);
    if(!raw)return;
    size_t got=0u;
    MusaDwgAc1032PayloadInfo copied={0};
    int copy=musa_dwg_ac1032_object_payload_file(
        file,n,handle,raw,wanted,&got,&copied);
    MusaDwgAc1032EntityCommon common={0};
    int pref=copy&&musa_dwg_ac1032_entity_common_bytes(
           raw,got,&layout,27u,&common);
    MusaDwgAc1032NativePoint point={0};
    int field=pref&&musa_dwg_ac1032_point_fields_at(
           raw,got,&layout,common.geometry_start_bit,&point);
    fprintf(stderr,"Point original parser: copied=%d prefix=%d"
       " geometry_start=%zu field=%d XYZ=(%.9g,%.9g,%.9g)"
       " endbit=%llu\n",copy,pref,common.geometry_start_bit,field,
       point.point[0],point.point[1],point.point[2],
       (unsigned long long)point.geometry_end_bit);
    fprintf(stderr,"Point original payload first 64 byte hex:");
    for(size_t i=0u;i<got&&i<64u;i++)
        fprintf(stderr," %02X",raw[i]);
    fputc('\n',stderr);
    free(raw);
}
static int near(double x,double y){
    return isfinite(x)&&fabs(x-y)<=0.0001;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    uint8_t *file=NULL;size_t n=0u;
    if(!load(argv[1],&file,&n))return 2;
    const uint64_t h=UINT64_C(0x1c0);
    MusaDwgAc1032NativePoint point={0};
    int ok=musa_dwg_ac1032_read_point_file(file,n,h,&point);
    fprintf(stderr,"Original R2018 POINT read=%d H=%llX"
      " XYZ=(%.9g,%.9g,%.9g) thickness=%.9g normalZ=%.9g"
      " x_angle=%.9g geometry_at=%zu\n",
      ok,(unsigned long long)point.indexed_handle,
      point.point[0],point.point[1],point.point[2],
      point.thickness,point.extrusion[2],point.x_axis_angle,
      point.common.geometry_start_bit);
    ok=ok&&point.geometry_decoded&&point.common.common_decoded&&
       point.indexed_handle==h&&
       point.common.prefix.raw_object_type==27u&&
       point.common.prefix.indexed_handle==h&&
       near(point.point[0],27.928)&&
       near(point.point[1],8.65308)&&
       near(point.point[2],0.0)&&
       near(point.thickness,0.0)&&
       near(point.extrusion[0],0.0)&&
       near(point.extrusion[1],0.0)&&
       near(point.extrusion[2],1.0);
    if(ok){
        const MusaDwgAc1032NativePoint saved=point;
        const uint8_t first=file[0],last=file[n-1u];
        file[0]^=1u;
        ok=!musa_dwg_ac1032_read_point_file(file,n,h,&point)&&
           !memcmp(&point,&saved,sizeof(point));
        file[0]=first;
        if(ok){
           file[n-1u]^=1u;
           ok=!musa_dwg_ac1032_read_point_file(file,n,h,&point)&&
              !memcmp(&point,&saved,sizeof(point));
           file[n-1u]=last;
        }
        /* The original empty SYSTEM footer introduced by this R2018
         * sample is an exact 20-byte structural record, not padding.
         * Tamper there must never be treated as valid geometry. */
        if(ok&&n>=128u){
           const uint8_t first_footer=file[n-128u];
           file[n-128u]^=1u;
           size_t old_count=43u;
           ok=!musa_dwg_ac1032_page_locations(
               file,n,NULL,0u,&old_count)&&old_count==43u&&
              !musa_dwg_ac1032_read_point_file(
               file,n,h,&point)&&
              !memcmp(&point,&saved,sizeof(point));
           file[n-128u]=first_footer;
        }
        if(ok){
           ok=!musa_dwg_ac1032_read_point_file(
               file,n,UINT64_MAX,&point)&&
              !memcmp(&point,&saved,sizeof(point));
        }
        if(ok)ok=!musa_dwg_ac1032_read_point_file(
             file,n,h,(MusaDwgAc1032NativePoint*)(void*)file);
    }
    if(!ok)stage_diagnostics(file,n,h);
    fprintf(stderr,"%s authentic original R2018 POINT XYZ"
       " with full source / CRC / handle proof\n",ok?"PASS":"FAIL");
    free(file);
    return ok?0:1;
}
