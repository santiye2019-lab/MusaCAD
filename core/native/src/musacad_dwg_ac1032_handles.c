/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_handles.h"
#include "musacad_dwg_ac1032_sections.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MUSA_AC1032_HANDLE_FILE_MAX (64u * 1024u * 1024u)
#define MUSA_AC1032_HANDLES_MAX (1000000u)
#define MUSA_AC1032_SECTION_MAX (64u * 1024u * 1024u)

static int overlap(const void *p, size_t n, const void *q, size_t m){
    if(!n||!m)return 0;
    const uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;
    return a<=b?b-a<n:a-b<m;
}
int musa_dwg_ac1032_handles_index(
    const uint8_t *file,size_t file_bytes,
    MusaDwgAc1032HandleEntry *entries,size_t capacity,
    size_t *out_count){
    if(!file||!out_count||file_bytes<256u||
       file_bytes>MUSA_AC1032_HANDLE_FILE_MAX||
       memcmp(file,"AC1032",6u)!=0||
       (!entries&&capacity)||
       overlap(file,file_bytes,out_count,sizeof(*out_count)))
        return 0;
    if(entries){
        if(capacity>SIZE_MAX/sizeof(*entries))return 0;
        const size_t output_bytes=capacity*sizeof(*entries);
        if(overlap(file,file_bytes,entries,output_bytes)||
           overlap(entries,output_bytes,out_count,sizeof(*out_count)))
            return 0;
    }

    /* Full authenticated original directory: do not trust a single
     * metadata field to validate an untrusted modern drawing. */
    size_t catalog_count=0u;
    if(!musa_dwg_ac1032_section_catalog(
          file,file_bytes,NULL,0u,&catalog_count)||
       !catalog_count||catalog_count>256u)return 0;
    MusaDwgAc1032SectionCatalogEntry *sections=
        calloc(catalog_count,sizeof(*sections));
    if(!sections)return 0;
    size_t got_catalog=0u;
    int ok=musa_dwg_ac1032_section_catalog(
         file,file_bytes,sections,catalog_count,&got_catalog)&&
         got_catalog==catalog_count;
    size_t object_bytes=0u,handle_bytes=0u;
    unsigned objects_found=0u,handles_found=0u;
    if(ok)for(size_t i=0u;i<catalog_count;i++){
        const MusaDwgAc1032SectionCatalogEntry *e=&sections[i];
        if(!e->all_physical_page_checksums_verified){ok=0;break;}
        if(!strcmp((const char*)e->name,"AcDb:AcDbObjects")){
            objects_found++;
            if(e->logical_bytes==0u||
               e->logical_bytes>MUSA_AC1032_SECTION_MAX)ok=0;
            else object_bytes=(size_t)e->logical_bytes;
        }else if(!strcmp((const char*)e->name,"AcDb:Handles")){
            handles_found++;
            if(e->logical_bytes<8u||
               e->logical_bytes>MUSA_AC1032_SECTION_MAX)ok=0;
            else handle_bytes=(size_t)e->logical_bytes;
        }
    }
    free(sections);
    if(!ok||objects_found!=1u||handles_found!=1u)return 0;

    /* Decode BOTH physical sections. An authenticated page directory
     * without actual valid LZ logical content is not sufficient. */
    uint8_t *objects=malloc(object_bytes);
    uint8_t *handles=malloc(handle_bytes);
    MusaDwgAc1032HandleEntry *temporary=NULL;
    size_t object_got=0u,handle_got=0u,needed=0u;
    ok=0;
    if(!objects||!handles)goto done;
    if(!musa_dwg_ac1032_extract_named_section(file,file_bytes,
          "AcDb:AcDbObjects",objects,object_bytes,&object_got)||
       object_got!=object_bytes||
       !musa_dwg_ac1032_extract_named_section(file,file_bytes,
          "AcDb:Handles",handles,handle_bytes,&handle_got)||
       handle_got!=handle_bytes||
       !musa_dwg_ac1018_handles_index_continuous_offsets(handles,handle_bytes,object_bytes,
                                     NULL,0u,&needed)||
       !needed||needed>MUSA_AC1032_HANDLES_MAX||
       needed>SIZE_MAX/sizeof(*entries))
        goto done;
    if(entries){
        if(capacity<needed)goto done;
        temporary=malloc(needed*sizeof(*temporary));
        if(!temporary)goto done;
        size_t filled=0u;
        if(!musa_dwg_ac1018_handles_index_continuous_offsets(
              handles,handle_bytes,object_bytes,
              temporary,needed,&filled)||filled!=needed)
            goto done;
        memcpy(entries,temporary,needed*sizeof(*entries));
    }
    *out_count=needed;
    ok=1;
done:
    free(temporary);
    free(handles);
    free(objects);
    return ok;
}
