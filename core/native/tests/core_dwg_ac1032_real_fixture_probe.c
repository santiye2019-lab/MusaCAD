/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real-file P0 acceptance of the first-party AC1032 PHYSICAL container.
 * The public upstream fixture is only READ as binary input; LibreDWG
 * decoding/conversion/library must NOT be called by this test.
 *
 * Covers original R2018 signature, encrypted file header, protected
 * page map, entire physical section directory and all data-page Adler
 * checksums. No entity, class graph, XREF or edit/save claim. */
#include "musacad_dwg_ac1032_page_map.h"
#include "musacad_dwg_ac1032_sections.h"
#include "musacad_dwg_ac1018_lz.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FILE_LIMIT (64u*1024u*1024u)
#define SECTION_LIMIT (32u*1024u*1024u)

static int real_file(const char *name,uint8_t **out,size_t *n){
    FILE *f=fopen(name,"rb");
    if(!f){perror(name);return 0;}
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    const long bytes=ftell(f);
    if(bytes<256||bytes>(long)FILE_LIMIT||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
    uint8_t *p=(uint8_t*)malloc((size_t)bytes);
    if(!p){fclose(f);return 0;}
    int ok=fread(p,1,(size_t)bytes,f)==(size_t)bytes;
    if(fgetc(f)!=EOF)ok=0;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(p);return 0;}
    *out=p;*n=(size_t)bytes;return 1;
}
/* Diagnostic on a REAL public fixture only. Uses the SAME first-party
 * LZ decoder; no external CAD library or format reinterpretation. */
static uint32_t u32_probe(const uint8_t *p){
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8u) |
           ((uint32_t)p[2]<<16u) | ((uint32_t)p[3]<<24u);
}
static void explain_real_page_map(const uint8_t *file,size_t n,
    const MusaDwgAc1032FileMapHeader *h){
    const size_t pos=(size_t)h->page_map_offset+20u;
    if(h->page_map_offset>SIZE_MAX-20u||pos>n||
       h->map_compressed_bytes>n-pos||
       h->map_decoded_bytes>4u*1024u*1024u)return;
    uint8_t *plain=(uint8_t*)malloc(h->map_decoded_bytes);
    if(!plain)return;
    size_t consumed=0u;
    const int ok=musa_dwg_ac1018_lz_decode(file+pos,
        h->map_compressed_bytes,plain,h->map_decoded_bytes,&consumed);
    fprintf(stderr,"Real AC1032 page-map decode=%d expanded=%u"
           " packed=%u consumed=%zu\n",ok,h->map_decoded_bytes,
           h->map_compressed_bytes,consumed);
    if(ok){
        size_t at=0u;unsigned idx=0u;uint64_t position=256u;
        while(at+8u<=h->map_decoded_bytes && idx<2048u){
            const uint32_t raw=u32_probe(plain+at);
            const uint32_t len=u32_probe(plain+at+4u);
            const int negative=(raw&UINT32_C(0x80000000))!=0u;
            if(idx<8u||at+24u>=h->map_decoded_bytes)
                fprintf(stderr," map entry[%u] raw=%08X"
                  " size=%u file_off=%llu gap=%d\n",
                  idx,raw,len,(unsigned long long)position,negative);
            if(negative&&at+24u>h->map_decoded_bytes)break;
            at+=negative?24u:8u;
            position+=len;
            idx++;
        }
        fprintf(stderr," Real AC1032 page-map summary records=%u"
                " parsed_bytes=%zu file_end=%llu real_size=%zu\n",
                idx,at,(unsigned long long)position,n);
    }
    free(plain);
}
static void fail(const char *stage,const uint8_t *file,size_t n){
    fprintf(stderr,
        "FAIL first-party AC1032 real DWG physical acceptance: %s;"
        " source_size=%zu; signature=%c%c%c%c%c%c; source UNCHANGED\n",
        stage,n,n>=6?file[0]:'?',n>=6?file[1]:'?',
        n>=6?file[2]:'?',n>=6?file[3]:'?',
        n>=6?file[4]:'?',n>=6?file[5]:'?');
}
int main(int argc,char **argv){
    if(argc!=2){fprintf(stderr,"usage: %s public-ac1032.dwg\n",argv[0]);return 2;}
    uint8_t *file=NULL;size_t len=0u;int accepted=0;
    if(!real_file(argv[1],&file,&len))return 2;
    if(memcmp(file,"AC1032",6u)!=0){
        fail("wrong original signature",file,len);goto done;
    }
    MusaDwgAc1032FileMapHeader header={0};
    if(!musa_dwg_ac1032_page_map_header(file,len,&header)){
        fprintf(stderr,"Original protected 108-byte header at 0x80:");
        for(size_t i=0;i<0x6cu;i++)fprintf(stderr," %02X",file[0x80u+i]);
        fputc('\n',stderr);
        fail("protected R2018 header/page-map envelope",file,len);goto done;
    }
    fprintf(stderr,"PASS original AC1032 protected map header:"
            " physical_pages=%u, first_map_off=%llu\n",
            header.page_count,(unsigned long long)header.page_map_offset);
    size_t map_count=0u;
    if(!musa_dwg_ac1032_page_locations(
                 file,len,NULL,0u,&map_count)||
       !map_count||map_count!=header.page_count){
        explain_real_page_map(file,len,&header);
        fail("decoded original AC1032 page locations",file,len);goto done;
    }
    MusaDwgAc1032PageLocation *map=(MusaDwgAc1032PageLocation*)calloc(
                                      map_count,sizeof(*map));
    if(!map||!musa_dwg_ac1032_page_locations(
                 file,len,map,map_count,&map_count)){
        fail("bounded physical map locations",file,len);
        free(map);goto done;
    }
    free(map);
    fprintf(stderr,"PASS original AC1032 physical location map: %zu pages\n",map_count);
    size_t section_count=0u;
    if(!musa_dwg_ac1032_section_catalog(
                 file,len,NULL,0u,&section_count)||
       !section_count||section_count>256u){
        fail("FULL physical section directory and every page CRC",
             file,len);goto done;
    }
    MusaDwgAc1032SectionCatalogEntry *entry=
        (MusaDwgAc1032SectionCatalogEntry*)calloc(
                                      section_count,sizeof(*entry));
    if(!entry||!musa_dwg_ac1032_section_catalog(
                      file,len,entry,section_count,&section_count)){
        fail("bounded complete section metadata",file,len);
        free(entry);goto done;
    }
    int objects=0,handles=0;
    for(size_t i=0;i<section_count;i++){
        const MusaDwgAc1032SectionCatalogEntry *e=&entry[i];
        if(!e->all_physical_page_checksums_verified){
            fail("unverified physical page in catalog",file,len);
            free(entry);goto done;
        }
        fprintf(stderr,"AC1032 physical section[%zu] name='%s'"
                 " logical_bytes=%llu page_count=%u id=%u\n",
                 i,(const char*)e->name,(unsigned long long)e->logical_bytes,
                 e->page_count,e->section_id);
        if(!strcmp((const char*)e->name,"AcDb:AcDbObjects"))objects++;
        if(!strcmp((const char*)e->name,"AcDb:Handles"))handles++;
    }
    if(objects!=1||handles!=1){
        fail("missing unique AcDbObjects/Handles in complete R2018 map",
             file,len);
        free(entry);goto done;
    }
    const char *extracts[]={"AcDb:AcDbObjects","AcDb:Handles"};
    for(size_t i=0;i<2u;i++){
        const MusaDwgAc1032SectionCatalogEntry *e=NULL;
        for(size_t j=0;j<section_count;j++)
            if(!strcmp((const char*)entry[j].name,extracts[i]))e=&entry[j];
        if(!e||!e->logical_bytes||e->logical_bytes>SECTION_LIMIT){
            fail("unbounded AC1032 extraction size",file,len);
            free(entry);goto done;
        }
        uint8_t *plain=(uint8_t*)malloc((size_t)e->logical_bytes);
        size_t written=0u;
        if(!plain||!musa_dwg_ac1032_extract_named_section(
                       file,len,extracts[i],plain,(size_t)e->logical_bytes,
                       &written)||written!=e->logical_bytes){
            fail("original R2018 physical data-page decompression",
                 file,len);
            free(plain);free(entry);goto done;
        }
        fprintf(stderr,"PASS first-party opaque AC1032 section '%s': %zu bytes\n",
                extracts[i],written);
        free(plain);
    }
    free(entry);
    fprintf(stderr,
        "PASS independent AC1032 real public fixture: all protected"
        " pages/sections admitted by MusaCAD C11 ONLY;"
        " no R2018 object/class/editing parity claimed\n");
    accepted=1;
done:
    free(file);
    return accepted?0:1;
}
