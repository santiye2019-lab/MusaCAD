/* SPDX-License-Identifier: GPL-3.0-or-later */
/* P0 REAL original R2018 LINE source: object handle, OT, CRC and streams.
 * Does NOT assert decoded coordinates: true R2018 ENT common-prefix logic
 * and its independently determined geometry_start_bit still pending.
 * AutoCAD reference coordinates are in same upstream 2018/Line.txt.
 */
#include "musacad_dwg_ac1032_handles.h"
#include "musacad_dwg_ac1032_object_type.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1032_object_streams.h"
#include "musacad_dwg_ac1032_sections.h"
#include "musacad_dwg_ac1032_page_map.h"
#include "musacad_dwg_ac1018_lz.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILE (64u*1024u*1024u)
#define ORIGINAL_LINE_HANDLE UINT64_C(0x1ca)
static int read_file(const char *path,uint8_t **out,size_t *n){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    const long len=ftell(f);
    if(len<256||len>(long)MAX_FILE||fseek(f,0,SEEK_SET)!=0){
        fclose(f);return 0;
    }
    uint8_t *mem=malloc((size_t)len);
    if(!mem){fclose(f);return 0;}
    int ok=fread(mem,1u,(size_t)len,f)==(size_t)len;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(mem);return 0;}
    *out=mem;*n=(size_t)len;return 1;
}
static void diagnose_line(const uint8_t *f,size_t n){
    MusaDwgAc1032FileMapHeader hdr={0};
    const int hdr_ok=musa_dwg_ac1032_page_map_header(f,n,&hdr);
    size_t phys=0u;
    const int loc_ok=musa_dwg_ac1032_page_locations(
         f,n,NULL,0u,&phys);
    fprintf(stderr,"Original LINE physical header_ok=%d locations_ok=%d"
         " page_count=%u physical=%zu map_id=%u secmap_id=%u"
         " offset=%llu packed=%u expanded=%u\n",
         hdr_ok,loc_ok,hdr.page_count,phys,hdr.page_map_id,
         hdr.section_map_id,(unsigned long long)hdr.page_map_offset,
         hdr.map_compressed_bytes,hdr.map_decoded_bytes);
    if(hdr_ok&&hdr.map_decoded_bytes>0u&&
       hdr.map_decoded_bytes<=65536u&&
       hdr.page_map_offset+20u+hdr.map_compressed_bytes<=n){
        uint8_t *plain=(uint8_t*)malloc(hdr.map_decoded_bytes);
        size_t used=0u;
        const int ok=plain&&musa_dwg_ac1018_lz_decode(
             f+(size_t)hdr.page_map_offset+20u,
             hdr.map_compressed_bytes,plain,hdr.map_decoded_bytes,&used);
        fprintf(stderr,"Original LINE map LZ=%d used=%zu expanded=%u\n",
                ok,used,hdr.map_decoded_bytes);
        if(ok){
            uint64_t offset=256u;
            for(size_t pos=0u;pos+8u<=hdr.map_decoded_bytes;){
                const uint32_t id=(uint32_t)plain[pos]|
                  ((uint32_t)plain[pos+1u]<<8u)|
                  ((uint32_t)plain[pos+2u]<<16u)|
                  ((uint32_t)plain[pos+3u]<<24u);
                const uint32_t span=(uint32_t)plain[pos+4u]|
                  ((uint32_t)plain[pos+5u]<<8u)|
                  ((uint32_t)plain[pos+6u]<<16u)|
                  ((uint32_t)plain[pos+7u]<<24u);
                const int gap=(id&UINT32_C(0x80000000))!=0;
                fprintf(stderr,"Original LINE physical entry offset=%zu"
                     " id=%08X span=%u physical=%llu gap=%d\n",
                     pos,id,span,(unsigned long long)offset,gap);
                const size_t stride=gap?24u:8u;
                if(hdr.map_decoded_bytes-pos<stride)break;
                pos+=stride;offset+=span;
            }
        }
        free(plain);
    }
    if(hdr_ok&&n>256u&&hdr.page_map_offset+160u<=n){
        const size_t trailer=(size_t)hdr.page_map_offset+160u;
        fprintf(stderr,"Original LINE last SYSTEM page end=%zu trailing=%zu"
             " header_copy_equal_at_EOFminus108=%d\n",
             trailer,n-trailer,
             memcmp(f+0x80u,f+n-108u,108u)==0);
        if(n-trailer<=256u){
            fprintf(stderr,"Original LINE trailer hex:");
            for(size_t i=trailer;i<n;i++)
               fprintf(stderr," %02X",f[i]);
            fputc('\n',stderr);
        }
    }
    fprintf(stderr,"Original LINE encrypted header at 0x80 first 24 bytes:");
    if(n>=0x98u)for(size_t k=0x80u;k<0x98u;k++)
        fprintf(stderr," %02X",f[k]);
    fputc('\n',stderr);
    size_t count=0u;
    const int index_count_ok=musa_dwg_ac1032_handles_index(f,n,NULL,0u,&count);
    fprintf(stderr,"Original LINE diagnostic: handle_index_ok=%d count=%zu file_bytes=%zu\n",
            index_count_ok,count,n);
    size_t cat=0u;
    const int cat_ok=musa_dwg_ac1032_section_catalog(
          f,n,NULL,0u,&cat);
    fprintf(stderr,"Original LINE diagnostic: section_catalog_ok=%d sections=%zu\n",
            cat_ok,cat);
    if(cat_ok&&cat>0u&&cat<=256u){
        MusaDwgAc1032SectionCatalogEntry *sect=calloc(cat,sizeof(*sect));
        size_t got=0u;
        if(sect&&musa_dwg_ac1032_section_catalog(f,n,sect,cat,&got)){
            for(size_t i=0u;i<got;i++)
                fprintf(stderr,"Original LINE section[%zu]: '%s' logical=%llu CRCs=%u\n",
                     i,sect[i].name,
                     (unsigned long long)sect[i].logical_bytes,
                     (unsigned)sect[i].all_physical_page_checksums_verified);
        }
        free(sect);
    }
    if(!index_count_ok||count==0u||count>1000000u)return;
    MusaDwgAc1032HandleEntry *idx=calloc(count,sizeof(*idx));
    if(!idx)return;
    if(!musa_dwg_ac1032_handles_index(f,n,idx,count,&count)){
        free(idx);return;
    }
    size_t sec_count=0u,object_bytes=0u,found=0u;
    if(!musa_dwg_ac1032_section_catalog(
          f,n,NULL,0u,&sec_count)||sec_count>256u||!sec_count){
        free(idx);return;
    }
    MusaDwgAc1032SectionCatalogEntry *entries=
        calloc(sec_count,sizeof(*entries));
    if(!entries){free(idx);return;}
    if(musa_dwg_ac1032_section_catalog(
             f,n,entries,sec_count,&sec_count)){
        for(size_t i=0u;i<sec_count;i++){
            if(!strcmp((const char*)entries[i].name,"AcDb:AcDbObjects")){
                found++;object_bytes=(size_t)entries[i].logical_bytes;
            }
        }
    }
    free(entries);
    if(found!=1u||!object_bytes||object_bytes>MAX_FILE){
        free(idx);return;
    }
    uint8_t *plain=malloc(object_bytes);
    size_t got=0u;
    if(!plain){free(idx);return;}
    if(musa_dwg_ac1032_extract_named_section(
         f,n,"AcDb:AcDbObjects",plain,object_bytes,&got)&&got==object_bytes){
        size_t seen=0u,lines=0u,errors=0u;
        uint64_t first=0u;
        for(size_t i=0;i<count;i++){
            MusaDwgAc1032ObjectType obj={0};
            if(!musa_dwg_ac1032_object_type_bytes(
                   plain,object_bytes,idx[i].handle,
                   idx[i].object_offset,&obj)){errors++;continue;}
            seen++;
            if(obj.type_code==19u){
                lines++;
                if(!first)first=idx[i].handle;
            }
        }
        fprintf(stderr,"Original source indexed OT summary:"
             " handles=%zu CRC-valid=%zu LINE_OT19=%zu first_LINE=%llX"
             " unhandled=%zu\n",count,seen,lines,
             (unsigned long long)first,errors);
    }
    free(plain);free(idx);
}
int main(int argc,char **argv){
    if(argc!=2){fprintf(stderr,"usage: %s 2018/Line.dwg\n",argv[0]);return 2;}
    uint8_t *f=NULL;size_t bytes=0u;
    if(!read_file(argv[1],&f,&bytes))return 2;
    int accepted=0;
    if(memcmp(f,"AC1032",6u)!=0){
        fprintf(stderr,"FAIL expected original AC1032 file\n");goto done;
    }
    MusaDwgAc1032ObjectType obj={0};
    if(!musa_dwg_ac1032_object_type_file(
          f,bytes,ORIGINAL_LINE_HANDLE,&obj)||
       !obj.record_crc_verified||
       obj.indexed_handle!=ORIGINAL_LINE_HANDLE||
       obj.type_code!=19u||obj.type_field_bits!=10u){
        fprintf(stderr,"FAIL original AutoCAD LINE handle 0x1CA"
               " type/CRC: type=%u offset=%llu\n",
               obj.type_code,(unsigned long long)obj.object_section_offset);
        diagnose_line(f,bytes);goto done;
    }
    MusaDwgAc1032PayloadInfo info={0};
    MusaDwgAc1032ObjectStreams layout={0};
    size_t needed=0u;
    if(!musa_dwg_ac1032_object_payload_file(
          f,bytes,ORIGINAL_LINE_HANDLE,NULL,0u,&needed,&info)||
       !musa_dwg_ac1032_stream_layout_file(
          f,bytes,ORIGINAL_LINE_HANDLE,&layout)||
       !needed||needed!=info.payload_bytes||
       layout.raw_object_type!=19u||
       layout.indexed_handle!=ORIGINAL_LINE_HANDLE||
       layout.data_stream_bits<layout.object_type_field_bits||
       layout.total_payload_bits!=(uint64_t)needed*8u||
       layout.handle_stream_bits!=info.handle_stream_bits||
       layout.object_section_offset!=obj.object_section_offset){
        fprintf(stderr,"FAIL original AutoCAD LINE data/handle stream metadata\n");
        goto done;
    }
    uint8_t *payload=malloc(needed);
    if(!payload){goto done;}
    uint32_t type=0u;
    const int copied=musa_dwg_ac1032_object_payload_file(
         f,bytes,ORIGINAL_LINE_HANDLE,payload,needed,&needed,&info)&&
         musa_dwg_ac1032_stream_read_bits(
         payload,needed,&layout,MUSA_AC1032_STREAM_OBJECT_DATA,
         0u,10u,&type)&&type==19u;
    free(payload);
    if(!copied){
        fprintf(stderr,"FAIL original R2018 LINE payload copied OT bits\n");
        goto done;
    }
    const MusaDwgAc1032ObjectType preserved=obj;
    const uint8_t first=f[0];
    f[0]^=1u;
    if(musa_dwg_ac1032_object_type_file(
           f,bytes,ORIGINAL_LINE_HANDLE,&obj)||
       memcmp(&obj,&preserved,sizeof(obj))){
        fprintf(stderr,"FAIL tampered original must preserve output\n");
        f[0]=first;goto done;
    }
    f[0]=first;
    fprintf(stderr,"PASS original AC1032 AutoCAD LINE:"
        " indexed_handle=0x%llX raw_OT=%u payload=%zu"
        " DATA_bits=%llu HANDLES_bits=%llu source_CRC=verified\n",
        (unsigned long long)ORIGINAL_LINE_HANDLE,type,needed,
        (unsigned long long)layout.data_stream_bits,
        (unsigned long long)layout.handle_stream_bits);
    fprintf(stderr,"Reference 2018/Line.txt:"
        " start=(21.6078,2.81192,0) end=(11.0307,16.6375,0);"
        " coordinates NOT decoded yet\n");
    accepted=1;
done:
    free(f);
    return accepted?0:1;
}
