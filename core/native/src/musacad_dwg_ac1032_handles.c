#include "musacad_dwg_ac1032_handles.h"
#include "musacad_dwg_ac1032_sections.h"
#include <stdlib.h>
#include <string.h>

#define MUSA_AC1032_HANDLE_CAP 1000000u
static int disjoint(const void *a,size_t n,const void *b,size_t m){
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return !n||!m||!((x<=y&&y-x<n)||(y<x&&x-y<m));
}
int musa_dwg_ac1032_file_handle_index(
    const uint8_t *file,size_t bytes,MusaDwgAc1032HandleLocation *entries,
    size_t capacity,size_t *out_count){
    if(!file||!out_count||bytes<256u||bytes>64u*1024u*1024u||
       (!entries&&capacity)||
       !disjoint(file,bytes,out_count,sizeof(*out_count)))return 0;
    if(entries){
        if(capacity>SIZE_MAX/sizeof(*entries))return 0;
        const size_t n=capacity*sizeof(*entries);
        if(!disjoint(file,bytes,entries,n)||
           !disjoint(entries,n,out_count,sizeof(*out_count)))return 0;
    }
    size_t count=0u;
    if(!musa_dwg_ac1032_section_catalog(
                file,bytes,NULL,0u,&count)||
       !count||count>256u)return 0;
    MusaDwgAc1032SectionCatalogEntry *meta=
        calloc(count,sizeof(*meta));
    if(!meta)return 0;
    int ok=musa_dwg_ac1032_section_catalog(
                file,bytes,meta,count,&count);
