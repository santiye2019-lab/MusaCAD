/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_sections.h"
#include "musacad_dwg_ac1018_page_map.h"
#include "musacad_dwg_ac1032_page_map.h"
#include "musacad_dwg_ac1032_sections.h"
#include "musacad_dwg_ac1018_lz.h"
#include "musacad_dwg_ac1018_section_assemble.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define R2004_BLOCK 29696u
#define R2004_MAX_PAGES 65536u
#define R2004_MAX_SECTIONS 256u
#define R2004_MAX_EXPANDED 1048576u
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8u)|
           ((uint32_t)p[2]<<16u)|((uint32_t)p[3]<<24u);
}
static uint64_t get64(const uint8_t *p) {
    return (uint64_t)get32(p)|((uint64_t)get32(p+4u)<<32u);
}
static void put32(uint8_t *p,uint32_t x) {
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8u);
    p[2]=(uint8_t)(x>>16u);p[3]=(uint8_t)(x>>24u);
}
/* Adler with the DWG R2004 initial value and chunked bounded arithmetic. */
static uint32_t check_adler(uint32_t seed,const uint8_t *p,size_t n) {
    uint64_t low=seed&65535u,high=seed>>16u;
    size_t at=0u;
    while(at<n) {
        const size_t count=n-at<0x15b0u?n-at:0x15b0u;
        for(size_t i=0u;i<count;i++){low+=p[at+i];high+=low;}
        low%=65521u;high%=65521u;at+=count;
    }
    return ((uint32_t)high<<16u)|(uint32_t)low;
}
static int scan(const uint8_t *file,size_t file_bytes,
                MusaDwgAc1018SectionKind kind,int modern,
                MusaDwgAc1018SectionPage *pages,
                MusaDwgAc1018SectionDescriptor *desc,
                size_t *out_count,
                MusaDwgAc1032SectionCatalogEntry *catalog_entries,
                size_t catalog_capacity,size_t *out_catalog_count,
                const char *explicit_section_name) {
    const int catalog_mode=(out_catalog_count!=NULL);
    if(explicit_section_name && (!modern||catalog_mode))
        return 0;
    if(!file||!desc||!out_count||
       (kind!=MUSA_DWG_AC1018_OBJECTS&&kind!=MUSA_DWG_AC1018_HANDLES))
        return 0;
    MusaDwgAc1018FileMapHeader hdr={0};
    const int header_ok=modern?
            musa_dwg_ac1032_page_map_header(file,file_bytes,&hdr):
            musa_dwg_ac1018_page_map_header(file,file_bytes,&hdr);
    if(!header_ok||hdr.page_count>R2004_MAX_PAGES)return 0;
    size_t num=0u;
    const int map_ok=modern?
            musa_dwg_ac1032_page_locations(file,file_bytes,NULL,0u,&num):
            musa_dwg_ac1018_page_locations(file,file_bytes,NULL,0u,&num);
    if(!map_ok||num!=hdr.page_count||num>R2004_MAX_PAGES)return 0;
    MusaDwgAc1018PageLocation *locations=
        (MusaDwgAc1018PageLocation*)calloc(num,sizeof(*locations));
    if(!locations)return 0;
    uint8_t *decoded=NULL;
    MusaDwgAc1032SectionCatalogEntry *catalog=NULL;
    uint8_t *seen_pages=NULL;
    int ok=0;
    const int entries_ok=modern?
        musa_dwg_ac1032_page_locations(file,file_bytes,locations,num,&num):
        musa_dwg_ac1018_page_locations(file,file_bytes,locations,num,&num);
    if(!entries_ok)goto finish;
    const size_t sid=(size_t)hdr.section_map_id-1u;
    if(sid>=num||locations[sid].page_id!=hdr.section_map_id||
       locations[sid].file_offset>file_bytes||
       file_bytes-(size_t)locations[sid].file_offset<20u||
       locations[sid].file_span_bytes<20u)goto finish;
    const size_t start=(size_t)locations[sid].file_offset;
    const uint8_t *header=file+start;
    const uint32_t kind_code=get32(header),expanded=get32(header+4u),
        packed=get32(header+8u),codec=get32(header+12u),
        stored=get32(header+16u);
    if(kind_code!=UINT32_C(0x4163003b)||codec!=2u||
       expanded<20u||expanded>R2004_MAX_EXPANDED||
       !packed||packed>locations[sid].file_span_bytes-20u||
       packed>file_bytes-start-20u)goto finish;
    uint8_t clean_head[20];
    memcpy(clean_head,header,20u);
    memset(clean_head+16u,0,4u);
    if(check_adler(check_adler(0u,clean_head,20u),
                   header+20u,packed)!=stored)goto finish;
    decoded=(uint8_t*)malloc(expanded);
    if(!decoded)goto finish;
    size_t consumed=0u;
    if(!musa_dwg_ac1018_lz_decode(header+20u,packed,decoded,expanded,
                                   &consumed)||consumed>packed)goto finish;
    const uint32_t nsections=get32(decoded);
    if(!nsections||nsections>R2004_MAX_SECTIONS||
       (catalog_mode&&catalog_entries&&nsections>catalog_capacity))
        goto finish;
    if(catalog_mode){
        catalog=(MusaDwgAc1032SectionCatalogEntry*)calloc(
                  nsections,sizeof(*catalog));
        seen_pages=(uint8_t*)calloc(num,sizeof(*seen_pages));
        if(!catalog||!seen_pages)goto finish;
    }
    const char *wanted=explicit_section_name?explicit_section_name:
         (kind==MUSA_DWG_AC1018_OBJECTS?
         "AcDb:AcDbObjects":"AcDb:Handles");
    /* Name is externally pre-validated by named API; protect scan too. */
    size_t wanted_size=0u;
    while(wanted_size<64u&&wanted[wanted_size])++wanted_size;
    if(!wanted_size||wanted_size>=64u)goto finish;
    size_t at=20u,page_count=0u;
    int found=0;
    MusaDwgAc1018SectionDescriptor selected={0};
    for(uint32_t i=0u;i<nsections;i++) {
        if(at>expanded||expanded-at<96u)goto finish;
        const uint8_t *record=decoded+at;
        const uint64_t logical=get64(record);
        const uint32_t npage=get32(record+8u);
        const uint32_t compression=get32(record+20u);
        const uint32_t section_id=get32(record+24u);
        const uint32_t encrypted=get32(record+28u);
        const uint8_t *name=record+32u;
        at+=96u;
        if(npage>num||(size_t)npage>(expanded-at)/16u)goto finish;
        const int match=!memcmp(name,wanted,wanted_size)&&
                        name[wanted_size]==0u;
        if(catalog_mode){
            if(!memchr(name,0,64u)||!name[0])goto finish;
            for(uint32_t prev=0u;prev<i;prev++){
                /* A section name is NUL-terminated. Different trailing
                 * padding must not bypass duplicate-name rejection. Keep
                 * the entire 64-byte raw spelling in the saved catalog. */
                if(catalog[prev].section_id==section_id||
                   !strcmp((const char*)catalog[prev].name,
                           (const char*)name))
                    goto finish;
            }
            memcpy(catalog[i].name,name,64u);
            catalog[i].logical_bytes=logical;
            catalog[i].section_id=section_id;
            catalog[i].page_count=npage;
            catalog[i].compression=compression;
            catalog[i].encryption=encrypted;
            catalog[i].all_physical_page_checksums_verified=1u;
        }
        if(match||catalog_mode) {
            if((match&&found)||!logical||logical>128u*1024u*1024u||
               logical>SIZE_MAX||!section_id||compression!=2u||
               encrypted!=0u||!npage||
               npage!=(logical-1u)/R2004_BLOCK+1u)goto finish;
            if(match)found=1;
            selected.section_id=section_id;
            selected.compression=compression;
            selected.encryption=encrypted;
            selected.page_count=npage;
            selected.logical_bytes=logical;
            selected.map_checksum_verified=1u;
            selected.data_page_checksums_verified=1u;
            page_count=npage;
            for(uint32_t j=0u;j<npage;j++) {
                const uint8_t *entry=decoded+at+(size_t)j*16u;
                const uint32_t pid=get32(entry);
                const uint32_t packed_bytes=get32(entry+4u);
                const uint64_t logical_offset=get64(entry+8u);
                if(!pid||pid>num||!packed_bytes||
                   logical_offset!=(uint64_t)j*R2004_BLOCK)
                    goto finish;
                if(catalog_mode){
                    if(seen_pages[pid-1u])goto finish;
                    seen_pages[pid-1u]=1u;
                }
                const MusaDwgAc1018PageLocation *loc=&locations[pid-1u];
                if(loc->page_id!=pid||loc->file_offset>file_bytes||
                   loc->file_span_bytes<32u||
                   file_bytes-(size_t)loc->file_offset<32u)goto finish;
                const uint64_t physical=loc->file_offset;
                const size_t pos=(size_t)physical;
                const uint32_t mask=UINT32_C(0x4164536b)^(uint32_t)physical;
                uint32_t words[8];
                for(size_t k=0u;k<8u;k++)
                    words[k]=get32(file+pos+k*4u)^mask;
                if(words[0]!=UINT32_C(0x4163043b)||
                   words[1]!=section_id||words[2]!=packed_bytes||
                   words[3]<packed_bytes||words[3]-packed_bytes>63u||
                   words[4]!=logical_offset||words[5]!=0u||
                   packed_bytes>loc->file_span_bytes-32u||
                   packed_bytes>file_bytes-pos-32u)goto finish;
                const uint32_t payload_crc=check_adler(
                    0u,file+pos+32u,packed_bytes);
                if(payload_crc!=words[7])goto finish;
                uint8_t decoded_head[32];
                for(size_t k=0u;k<8u;k++)
                    put32(decoded_head+k*4u,k==6u?0u:words[k]);
                if(check_adler(payload_crc,decoded_head,32u)!=words[6])
                    goto finish;
                if(pages) {
                    pages[j].page_id=pid;
                    pages[j].compressed_bytes=packed_bytes;
                    pages[j].physical_span_bytes=loc->file_span_bytes;
                    pages[j].masked_header_decompressed_bytes=words[3];
                    pages[j].physical_offset=physical;
                    pages[j].payload_offset=physical+32u;
                    pages[j].logical_offset=logical_offset;
                    pages[j].data_checksum_verified=1u;
                    pages[j].header_checksum_verified=1u;
                }
            }
        }
        at+=(size_t)npage*16u;
    }
    if((!catalog_mode&&!found)||at!=expanded)goto finish;
    if(catalog_mode){
        if(catalog_entries)
            memcpy(catalog_entries,catalog,
                   (size_t)nsections*sizeof(*catalog_entries));
        *out_catalog_count=(size_t)nsections;
    }
    *desc=selected;*out_count=page_count;ok=1;
finish:
    free(seen_pages);free(catalog);
    free(decoded);free(locations);return ok;
}
static int shared_section_pages(const uint8_t *file,size_t file_bytes,
    MusaDwgAc1018SectionKind kind,int modern,const char *name,
    MusaDwgAc1018SectionPage *pages,
    size_t capacity,MusaDwgAc1018SectionDescriptor *desc,
    size_t *out_page_count) {
    if(!file||!desc||!out_page_count||(!pages&&capacity))return 0;
    MusaDwgAc1018SectionDescriptor checked={0};
    size_t count=0u;
    if(!scan(file,file_bytes,kind,modern,NULL,&checked,&count,
             NULL,0u,NULL,name))return 0;
    if(pages) {
        if(count>capacity)return 0;
        MusaDwgAc1018SectionPage *tmp=
            (MusaDwgAc1018SectionPage*)calloc(count,sizeof(*tmp));
        if(!tmp)return 0;
        MusaDwgAc1018SectionDescriptor twice={0};
        size_t again=0u;
        const int ok=scan(file,file_bytes,kind,modern,tmp,&twice,&again,
                          NULL,0u,NULL,name)&&
                     again==count;
        if(ok)memcpy(pages,tmp,count*sizeof(*pages));
        free(tmp);
        if(!ok)return 0;
    }
    *desc=checked;
    *out_page_count=count;
    return 1;
}
static int shared_extract_section(const uint8_t *file,size_t file_bytes,
    MusaDwgAc1018SectionKind kind,int modern,const char *name,
    uint8_t *output,size_t capacity,
    size_t *out_bytes) {
    if(!file||!output||!out_bytes)return 0;
    const uintptr_t src=(uintptr_t)file,dst=(uintptr_t)output;
    if((dst>=src&&dst-src<file_bytes)||
       (src>=dst&&src-dst<capacity))return 0;
    MusaDwgAc1018SectionDescriptor info={0};
    size_t n=0u;
    if(!shared_section_pages(file,file_bytes,kind,modern,name,NULL,0u,
                             &info,&n)||
       info.logical_bytes>capacity||info.logical_bytes>SIZE_MAX||!n)
        return 0;
    MusaDwgAc1018SectionPage *pages=
        (MusaDwgAc1018SectionPage*)calloc(n,sizeof(*pages));
    MusaDwgAc1018PageInput *inputs=
        (MusaDwgAc1018PageInput*)calloc(n,sizeof(*inputs));
    if(!pages||!inputs){free(pages);free(inputs);return 0;}
    MusaDwgAc1018SectionDescriptor again={0};
    size_t n2=0u;
    int ok=shared_section_pages(file,file_bytes,kind,modern,name,pages,n,
                                &again,&n2)&&n2==n&&
           again.logical_bytes==info.logical_bytes&&
           again.data_page_checksums_verified;
    if(ok) {
        for(size_t i=0u;i<n;i++) {
            if(!pages[i].header_checksum_verified||
               !pages[i].data_checksum_verified||
               pages[i].payload_offset>SIZE_MAX||
               pages[i].logical_offset>SIZE_MAX) {ok=0;break;}
            inputs[i].packed_offset=(size_t)pages[i].payload_offset;
            inputs[i].packed_length=pages[i].compressed_bytes;
            inputs[i].logical_offset=(size_t)pages[i].logical_offset;
            inputs[i].header_verified=1u;
        }
    }
    if(ok)ok=musa_dwg_ac1018_assemble_section(file,file_bytes,
                          inputs,n,output,(size_t)info.logical_bytes);
    if(ok)*out_bytes=(size_t)info.logical_bytes;
    free(pages);free(inputs);return ok;
}

/* Two public entry points share ONLY the physical DWG page envelope.
 * AC1032 objects are opaque; AC1018 entity parsers are not involved. */
int musa_dwg_ac1018_section_pages(const uint8_t *file,size_t file_bytes,
    MusaDwgAc1018SectionKind kind,MusaDwgAc1018SectionPage *pages,
    size_t capacity,MusaDwgAc1018SectionDescriptor *desc,
    size_t *out_page_count){
    return shared_section_pages(file,file_bytes,kind,0,NULL,pages,capacity,
                                desc,out_page_count);
}
int musa_dwg_ac1032_section_pages(const uint8_t *file,size_t file_bytes,
    MusaDwgAc1032SectionKind kind,MusaDwgAc1032SectionPage *pages,
    size_t capacity,MusaDwgAc1032SectionDescriptor *desc,
    size_t *out_page_count){
    return shared_section_pages(file,file_bytes,kind,1,NULL,pages,capacity,
                                desc,out_page_count);
}
int musa_dwg_ac1018_extract_section(const uint8_t *file,size_t file_bytes,
    MusaDwgAc1018SectionKind kind,uint8_t *output,size_t capacity,
    size_t *out_bytes){
    return shared_extract_section(file,file_bytes,kind,0,NULL,
                                  output,capacity,out_bytes);
}
int musa_dwg_ac1032_extract_section(const uint8_t *file,size_t file_bytes,
    MusaDwgAc1032SectionKind kind,uint8_t *output,size_t capacity,
    size_t *out_bytes){
    return shared_extract_section(file,file_bytes,kind,1,NULL,
                                  output,capacity,out_bytes);
}

/* All-section catalog: reuses the already validated R2018 protected
 * section-map and physical page checksum logic. Unknown names are kept,
 * not assigned an invented AcDb class. Caller output remains atomic. */
int musa_dwg_ac1032_section_catalog(const uint8_t *file,size_t n,
    MusaDwgAc1032SectionCatalogEntry *entries,size_t capacity,
    size_t *out_count){
    if(!file||!out_count||(!entries&&capacity)||
       (uintptr_t)out_count>UINTPTR_MAX-sizeof(*out_count))
        return 0;
    const uintptr_t src=(uintptr_t)file,cnt=(uintptr_t)out_count;
    if((cnt>=src&&cnt-src<n)||
       (src>=cnt&&src-cnt<sizeof(*out_count)))
        return 0;
    if(entries){
        if(capacity>SIZE_MAX/sizeof(*entries))return 0;
        const size_t bytes=capacity*sizeof(*entries);
        const uintptr_t dst=(uintptr_t)entries;
        if((dst>=src&&dst-src<n)||
           (src>=dst&&src-dst<bytes)||
           (dst>=cnt&&dst-cnt<sizeof(*out_count))||
           (cnt>=dst&&cnt-dst<bytes))
            return 0;
    }
    MusaDwgAc1018SectionDescriptor unused={0};
    size_t ignored_pages=0u;
    return scan(file,n,MUSA_DWG_AC1018_OBJECTS,1,
                NULL,&unused,&ignored_pages,
                entries,capacity,out_count,NULL);
}

/* Named-section extraction fills a real gap: class/metadata and other
 * previously catalog-only AC1032 sections can now be extracted as opaque
 * original logical bytes. No object/class grammar is guessed or decoded. */
static int ranges_overlap(const void *a,size_t na,const void *b,size_t nb){
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return na&&nb&&((x<=y&&y-x<na)||(y<x&&x-y<nb));
}
int musa_dwg_ac1032_extract_named_section(
    const uint8_t *file,size_t file_bytes,const char *section_name,
    uint8_t *output,size_t capacity,size_t *out_bytes){
    if(!file||!section_name||!output||!out_bytes)return 0;
    size_t len=0u;
    while(len<64u&&section_name[len])++len;
    if(!len||len>=64u||!capacity||
       ranges_overlap(file,file_bytes,output,capacity)||
       ranges_overlap(file,file_bytes,out_bytes,sizeof(*out_bytes))||
       ranges_overlap(output,capacity,out_bytes,sizeof(*out_bytes)))
        return 0;
    /* Unlike fixed-section extraction, a corrupt or duplicate *other*
     * section invalidates the WHOLE physical catalog. Never silently
     * hide unknown but present sections in a claimed complete transfer. */
    size_t all_sections=0u;
    if(!musa_dwg_ac1032_section_catalog(
             file,file_bytes,NULL,0u,&all_sections)||
       !all_sections)return 0;
    return shared_extract_section(file,file_bytes,
                                  MUSA_DWG_AC1018_OBJECTS,1,
                                  section_name,output,capacity,out_bytes);
}
