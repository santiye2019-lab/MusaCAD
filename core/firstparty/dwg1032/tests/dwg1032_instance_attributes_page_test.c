/* SPDX-License-Identifier: MIT
 * Independent synthetic post-decode INSERT paging regression.
 * Does not claim to parse or authenticate a real AC1032 DWG.
 */
#include "musa_dwg1032_instance_attributes.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void page_parity_and_atomic_rollback(void) {
    enum { N=20000, PAGE=257 };
    MusaDwg1032ResolvedColor *definition=calloc(N,sizeof(*definition));
    MusaDwg1032CadColor *raw=calloc(N,sizeof(*raw));
    uint64_t *handles=calloc(N,sizeof(*handles));
    uint8_t *owners=calloc(N,sizeof(*owners));
    MusaDwg1032ResolvedColor *full=calloc(N,sizeof(*full));
    MusaDwg1032ResolvedColor *paged=calloc(N,sizeof(*paged));
    MusaDwg1032ResolvedColor *full_scratch=calloc(N,sizeof(*full_scratch));
    MusaDwg1032ResolvedColor *page_scratch=calloc(PAGE,sizeof(*page_scratch));
    uint8_t *full_visible=calloc(N,1u);
    uint8_t *paged_visible=calloc(N,1u);
    uint8_t *full_vscratch=calloc(N,1u);
    uint8_t *page_vscratch=calloc(PAGE,1u);
    assert(definition && raw && handles && owners && full && paged &&
           full_scratch && page_scratch && full_visible && paged_visible &&
           full_vscratch && page_vscratch);
    for (size_t i=0u;i<N;++i) {
        handles[i]=10u+(uint64_t)(i%3u)*10u;
        owners[i]=(uint8_t)(i%13u!=0u);
        if (i%3u==0u) {
            raw[i]=(MusaDwg1032CadColor){0,0,0};
            definition[i]=(MusaDwg1032ResolvedColor){{0,0,0},1};
        } else if (i%3u==1u) {
            raw[i]=(MusaDwg1032CadColor){256,0,0};
            definition[i]=(MusaDwg1032ResolvedColor){{7,0,0},0};
        } else {
            raw[i]=(MusaDwg1032CadColor){256,1,0xabcdefu};
            definition[i]=(MusaDwg1032ResolvedColor){{256,1,0xabcdefu},0};
        }
    }
    const MusaDwg1032LayerState layers[3]={
        {10,0,0},{20,1,1},{30,1,1}
    };
    const MusaDwg1032ViewportFreeze frozen[1]={{20,1}};
    const MusaDwg1032ResolvedColor insert={{1,0,0},0};
    const MusaDwg1032ResolvedColor insert_layer={{5,0,0},0};
#define PAGE_CALL(FIRST,COUNT,COUT,VOUT,CS,CC,VS,VC) \
    musa_dwg1032_instance_attributes_page( \
        definition,raw,handles,owners,N,FIRST,COUNT,10,1, \
        &insert,&insert_layer,layers,3,frozen,1, \
        COUT,VOUT,CS,CC,VS,VC)
    assert(musa_dwg1032_instance_attributes(
        definition,raw,handles,owners,N,10,1,&insert,&insert_layer,
        layers,3,frozen,1,full,full_visible,
        full_scratch,N,full_vscratch,N));
    for (size_t first=0u;first<N;first+=PAGE) {
        const size_t n=N-first<PAGE?N-first:PAGE;
        assert(PAGE_CALL(first,n,paged+first,paged_visible+first,
                         page_scratch,PAGE,page_vscratch,PAGE));
    }
    for (size_t i=0u;i<N;++i) {
        assert(paged_visible[i]==full_visible[i]);
        assert(paged[i].byblock==full[i].byblock);
        assert(paged[i].color.aci==full[i].color.aci);
        assert(paged[i].color.has_truecolor==full[i].color.has_truecolor);
        assert(paged[i].color.truecolor_rgb==full[i].color.truecolor_rgb);
    }
    /* Late invalid owner cannot overwrite a previously published page. */
    MusaDwg1032ResolvedColor old_colors[17];
    uint8_t old_visible[17];
    memcpy(old_colors,paged+N-17u,sizeof(old_colors));
    memcpy(old_visible,paged_visible+N-17u,sizeof(old_visible));
    owners[N-1u]=2u;
    assert(!PAGE_CALL(N-17u,17u,paged+N-17u,paged_visible+N-17u,
                      page_scratch,PAGE,page_vscratch,PAGE));
    assert(!memcmp(old_colors,paged+N-17u,sizeof(old_colors)));
    assert(!memcmp(old_visible,paged_visible+N-17u,sizeof(old_visible)));
    owners[N-1u]=1u;
    /* An output/scratch pointer into a future input page must be rejected,
     * even when disjoint from the currently selected child page. */
    assert(!PAGE_CALL(0u,17u,
        (MusaDwg1032ResolvedColor *)(void *)(raw+1000u),paged_visible,
        page_scratch,PAGE,page_vscratch,PAGE));
    assert(!PAGE_CALL(0u,17u,paged,paged_visible,
        (MusaDwg1032ResolvedColor *)(void *)(raw+1000u),17u,
        page_vscratch,PAGE));
    assert(!PAGE_CALL(0u,17u,paged,owners+1000u,
        page_scratch,PAGE,page_vscratch,PAGE));
    assert(!PAGE_CALL(0u,0u,paged,paged_visible,
        page_scratch,PAGE,page_vscratch,PAGE));
    assert(!PAGE_CALL(N,1u,paged,paged_visible,
        page_scratch,PAGE,page_vscratch,PAGE));
    assert(!PAGE_CALL(N-1u,2u,paged,paged_visible,
        page_scratch,PAGE,page_vscratch,PAGE));
    assert(!PAGE_CALL(0u,SIZE_MAX,paged,paged_visible,
        page_scratch,PAGE,page_vscratch,PAGE));
#undef PAGE_CALL
    free(page_vscratch);free(full_vscratch);
    free(paged_visible);free(full_visible);
    free(page_scratch);free(full_scratch);
    free(paged);free(full);free(owners);free(handles);free(raw);
    free(definition);
}
int main(void) {
    page_parity_and_atomic_rollback();
    return 0;
}
