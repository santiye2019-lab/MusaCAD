/* SPDX-License-Identifier: GPL-3.0-or-later */
/* True independent original AutoCAD 2018/PolyLine3D.dwg + .txt.
 * Polyline OT16 H 0x1C1; VERTEX3D OT11 coordinates (real source objects).
 * Separate object coordinates DO NOT prove ownership or vertex order
 * until the original R2018 HANDLES reference graph is independently read.
 */
#include "musacad_dwg_ac1032_polyline3d.h"
#include "musacad_dwg_ac1032_handles.h"
#include "musacad_dwg_ac1032_sections.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FILE_MAX (64u*1024u*1024u)
#define LOGICAL_MAX (16u*1024u*1024u)
#define H_MAX 10000u
static int load(const char *path,uint8_t **file,size_t *bytes){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256||n>(long)FILE_MAX||fseek(f,0,SEEK_SET)!=0){
        fclose(f);return 0;
    }
    uint8_t *p=(uint8_t*)malloc((size_t)n);
    if(!p){fclose(f);return 0;}
    int ok=fread(p,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(p);return 0;}
    *file=p;*bytes=(size_t)n;return 1;
}
static int near(double a,double b){
    return isfinite(a)&&fabs(a-b)<0.0001;
}
static int verify_vertices(const uint8_t *file,size_t n){
    size_t count=0u;
    if(!musa_dwg_ac1032_handles_index(file,n,NULL,0u,&count)||
       !count||count>H_MAX)return 0;
    MusaDwgAc1032HandleEntry *index=
         (MusaDwgAc1032HandleEntry*)calloc(count,sizeof(*index));
    if(!index)return 0;
    size_t filled=0u;
    int ok=musa_dwg_ac1032_handles_index(
            file,n,index,count,&filled)&&filled==count;
    size_t cat_count=0u,cat_out=0u,obj_bytes=0u;
    MusaDwgAc1032SectionCatalogEntry *sections=NULL;
    if(ok)ok=musa_dwg_ac1032_section_catalog(
           file,n,NULL,0u,&cat_count)&&cat_count>0u&&
           cat_count<=256u;
    if(ok){
       sections=(MusaDwgAc1032SectionCatalogEntry*)
              calloc(cat_count,sizeof(*sections));
       ok=sections&&musa_dwg_ac1032_section_catalog(
             file,n,sections,cat_count,&cat_out)&&
             cat_out==cat_count;
    }
    if(ok)for(size_t i=0u;i<cat_count;i++)
       if(!strcmp((const char*)sections[i].name,"AcDb:AcDbObjects")){
          obj_bytes=(size_t)sections[i].logical_bytes;
       }
    if(!obj_bytes||obj_bytes>LOGICAL_MAX)ok=0;
    uint8_t *objects=NULL;
    if(ok){
       objects=(uint8_t*)malloc(obj_bytes);
       size_t read=0u;
       ok=objects&&musa_dwg_ac1032_extract_named_section(
         file,n,"AcDb:AcDbObjects",objects,obj_bytes,&read)&&
         read==obj_bytes;
    }
    uint8_t matches=0u;
    if(ok)for(size_t k=0u;k<count;k++){
       size_t needed=0u;
       MusaDwgAc1032PayloadInfo p={0};
       if(!musa_dwg_ac1032_object_payload_bytes(
         objects,obj_bytes,index[k].handle,index[k].object_offset,
         NULL,0u,&needed,&p)||p.raw_object_type!=11u||
         needed==0u||needed>8192u)continue;
       MusaDwgAc1032ObjectStreams l={0};
       if(!musa_dwg_ac1032_stream_layout_bytes(
          objects,obj_bytes,index[k].handle,index[k].object_offset,&l))
          continue;
       uint8_t *payload=(uint8_t*)malloc(needed);
       size_t copied=0u;
       MusaDwgAc1032PayloadInfo cp={0};
       MusaDwgAc1032EntityCommon co={0};
       MusaDwgAc1032Vertex3D v={0};
       int good=payload&&musa_dwg_ac1032_object_payload_bytes(
          objects,obj_bytes,index[k].handle,index[k].object_offset,
          payload,needed,&copied,&cp)&&copied==needed&&
          musa_dwg_ac1032_entity_common_bytes(
             payload,needed,&l,11u,&co)&&
          musa_dwg_ac1032_vertex3d_fields_at(
             payload,needed,&l,co.geometry_start_bit,&v);
       free(payload);
       if(!good)continue;
       if(near(v.point[0],21.4409)&&
          near(v.point[1],14.6498)&&near(v.point[2],0.0))
          matches|=1u;
       if(near(v.point[0],20.8962)&&
          near(v.point[1],5.65378)&&near(v.point[2],0.0))
          matches|=2u;
       fprintf(stderr,"Native original 3D VERTEX"
           " H=%llX XYZ=(%.9g,%.9g,%.9g) match_mask=%u\n",
           (unsigned long long)v.indexed_handle,
           v.point[0],v.point[1],v.point[2],matches);
       if(matches==3u)break;
    }
    fprintf(stderr,"Original independent 3D vertex witness:"
       " matches=%u handle_index_count=%zu\n",matches,count);
    free(objects);free(sections);free(index);
    return ok&&matches==3u;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    uint8_t *file=NULL;size_t n=0u;
    if(!load(argv[1],&file,&n))return 2;
    const uint64_t h=UINT64_C(0x1c1);
    MusaDwgAc1032Polyline3DHeader header={0};
    int ok=musa_dwg_ac1032_read_polyline3d_header_file(
         file,n,h,&header);
    fprintf(stderr,"Authentic R2018 3D POLYLINE header read=%d"
       " H=%llX count=%u spline_flags=%u flags=%u"
       " geom_start=%zu geom_end=%llu\n",
       ok,(unsigned long long)header.indexed_handle,
       header.owned_object_count,header.spline_flags,
       header.polyline_flags,header.common.geometry_start_bit,
       (unsigned long long)header.geometry_end_bit);
    ok=ok&&header.header_decoded&&
       header.common.common_decoded&&
       header.common.prefix.indexed_handle==h&&
       header.common.prefix.raw_object_type==16u&&
       header.owned_object_count>=2u&&
       header.owned_object_count<=H_MAX&&
       header.closed==0u&&header.spline_flags==0u;
    if(ok)ok=verify_vertices(file,n);
    if(ok){
       const MusaDwgAc1032Polyline3DHeader saved=header;
       const uint8_t first=file[0],last=file[n-1u];
       file[0]^=1u;
       ok=!musa_dwg_ac1032_read_polyline3d_header_file(
         file,n,h,&header)&&!memcmp(&saved,&header,sizeof(header));
       file[0]=first;
       if(ok){
          file[n-1u]^=1u;
          ok=!musa_dwg_ac1032_read_polyline3d_header_file(
            file,n,h,&header)&&!memcmp(&saved,&header,sizeof(header));
          file[n-1u]=last;
       }
    }
    fprintf(stderr,"%s original AutoCAD R2018 3D POLYLINE"
       " header and independent vertex records (NOT owned graph)\n",
       ok?"PASS":"FAIL");
    free(file);return ok?0:1;
}
