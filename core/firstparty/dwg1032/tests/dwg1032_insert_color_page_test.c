/* SPDX-License-Identifier: MIT
 * Synthetic per-INSERT layer-zero color paging; NOT real DWG acceptance.
 */
#include "musa_dwg1032_layer_color.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

enum { N=20000, PAGE=127 };
static MusaDwg1032ResolvedColor definition[N], reference[N], paged[N];
static MusaDwg1032CadColor source[N];
static uint64_t child_layers[N];

static void prepare(void) {
    for (size_t i=0u;i<N;++i) {
        if (i%3u==0u) {
            child_layers[i]=100u;
            source[i]=(MusaDwg1032CadColor){256u,0u,0u};
            definition[i]=(MusaDwg1032ResolvedColor){{4u,0u,0u},0u};
        } else if (i%3u==1u) {
            child_layers[i]=200u;
            source[i]=(MusaDwg1032CadColor){0u,0u,0u};
            definition[i]=(MusaDwg1032ResolvedColor){{0u,0u,0u},1u};
        } else {
            child_layers[i]=200u;
            source[i]=(MusaDwg1032CadColor){6u,0u,0u};
            definition[i]=(MusaDwg1032ResolvedColor){{6u,0u,0u},0u};
        }
    }
}
static void compare_instance(uint16_t insert_aci, uint16_t layer_aci) {
    const MusaDwg1032ResolvedColor insert={ {insert_aci,0u,0u},0u };
    const MusaDwg1032ResolvedColor layer={ {layer_aci,0u,0u},0u };
    assert(musa_dwg1032_layer_color_apply_insert_context(
        definition,source,child_layers,N,100u,&insert,&layer,reference));
    memset(paged,0xa5,sizeof(paged));
    for (size_t first=0u;first<N;first+=PAGE) {
        const size_t count=N-first<PAGE ? N-first : PAGE;
        assert(musa_dwg1032_layer_color_apply_insert_context_page(
            definition,source,child_layers,N,first,count,100u,
            &insert,&layer,paged+first));
        assert(memcmp(paged+first,reference+first,
                      count*sizeof(*paged))==0);
    }
    assert(memcmp(paged,reference,sizeof(paged))==0);
    for (size_t i=0u;i<N;++i) {
        const uint16_t expected=i%3u==0u ? layer_aci :
                                i%3u==1u ? insert_aci : 6u;
        assert(paged[i].color.aci==expected);
        assert(paged[i].byblock==0u);
    }
}
static void late_failure_and_full_source_alias(void) {
    const MusaDwg1032ResolvedColor insert={{1u,0u,0u},0u};
    const MusaDwg1032ResolvedColor layer={{5u,0u,0u},0u};
    MusaDwg1032ResolvedColor guard[PAGE];
    memset(guard,0x7b,sizeof(guard));
    MusaDwg1032ResolvedColor saved[PAGE];
    memcpy(saved,guard,sizeof(saved));
    source[N-1u].has_truecolor=2u;
    assert(!musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,N-PAGE,PAGE,100u,
        &insert,&layer,guard));
    assert(memcmp(guard,saved,sizeof(guard))==0);
    source[N-1u].has_truecolor=0u;
    assert(!musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,0u,1u,100u,
        &insert,&layer,&definition[N-1u]));
    assert(!musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,0u,1u,100u,
        &insert,&layer,(MusaDwg1032ResolvedColor *)(void *)&source[N-1u]));
    assert(!musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,N,1u,100u,
        &insert,&layer,guard));
    assert(!musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,SIZE_MAX,1u,100u,
        &insert,&layer,guard));
    assert(!musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,0u,0u,100u,
        &insert,&layer,guard));
    assert(memcmp(guard,saved,sizeof(guard))==0);
}
static void last_record_singleton_page(void) {
    const MusaDwg1032ResolvedColor insert={{2u,0u,0u},0u};
    const MusaDwg1032ResolvedColor layer={{3u,0u,0u},0u};
    MusaDwg1032ResolvedColor output[3];
    MusaDwg1032ResolvedColor before[3];
    memset(output,0x6d,sizeof(output));
    memcpy(before,output,sizeof(before));
    assert(musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,N-1u,1u,100u,
        &insert,&layer,&output[1]));
    assert(memcmp(&output[0],&before[0],sizeof(output[0]))==0);
    assert(memcmp(&output[2],&before[2],sizeof(output[2]))==0);
    /* N-1 is 19999: modulo 3 equals 1, so BYBLOCK inherits INSERT ACI 2. */
    assert((N-1u)%3u==1u);
    assert(output[1].color.aci==insert.color.aci);
    assert(output[1].byblock==0u);
    memcpy(output,before,sizeof(output));
    assert(!musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,N-1u,2u,100u,
        &insert,&layer,&output[1]));
    assert(memcmp(output,before,sizeof(output))==0);
}
static void unaligned_page_with_canaries(void) {
    const MusaDwg1032ResolvedColor insert={{7u,0u,0u},0u};
    const MusaDwg1032ResolvedColor layer={{3u,0u,0u},0u};
    MusaDwg1032ResolvedColor guarded[PAGE+2u];
    MusaDwg1032ResolvedColor untouched[PAGE+2u];
    const size_t first=PAGE-1u;
    assert(musa_dwg1032_layer_color_apply_insert_context(
        definition,source,child_layers,N,100u,&insert,&layer,reference));
    memset(guarded,0x49,sizeof(guarded));
    memcpy(untouched,guarded,sizeof(guarded));
    assert(musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,first,PAGE,100u,
        &insert,&layer,guarded+1u));
    assert(memcmp(&guarded[0],&untouched[0],sizeof(guarded[0]))==0);
    assert(memcmp(&guarded[PAGE+1u],&untouched[PAGE+1u],
                  sizeof(guarded[0]))==0);
    assert(memcmp(guarded+1u,reference+first,
                  PAGE*sizeof(guarded[0]))==0);
}
static void unaligned_late_failure_preserves_canaries(void) {
    const MusaDwg1032ResolvedColor insert={{7u,0u,0u},0u};
    const MusaDwg1032ResolvedColor layer={{3u,0u,0u},0u};
    MusaDwg1032ResolvedColor guarded[PAGE+2u];
    MusaDwg1032ResolvedColor untouched[PAGE+2u];
    const size_t first=PAGE-1u;
    const size_t bad=first+PAGE-1u;
    memset(guarded,0x53,sizeof(guarded));
    memcpy(untouched,guarded,sizeof(guarded));
    source[bad].has_truecolor=2u;
    assert(!musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,first,PAGE,100u,
        &insert,&layer,guarded+1u));
    assert(memcmp(guarded,untouched,sizeof(guarded))==0);
    source[bad].has_truecolor=0u;
}
static void first_record_failure_preserves_page(void) {
    const MusaDwg1032ResolvedColor insert={{7u,0u,0u},0u};
    const MusaDwg1032ResolvedColor layer={{3u,0u,0u},0u};
    MusaDwg1032ResolvedColor guarded[PAGE+2u];
    MusaDwg1032ResolvedColor untouched[PAGE+2u];
    const size_t first=PAGE+1u;
    memset(guarded,0x61,sizeof(guarded));
    memcpy(untouched,guarded,sizeof(guarded));
    source[first].has_truecolor=2u;
    assert(!musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,first,PAGE,100u,
        &insert,&layer,guarded+1u));
    assert(memcmp(guarded,untouched,sizeof(guarded))==0);
    source[first].has_truecolor=0u;
}
static void middle_record_failure_preserves_page(void) {
    const MusaDwg1032ResolvedColor insert={{7u,0u,0u},0u};
    const MusaDwg1032ResolvedColor layer={{3u,0u,0u},0u};
    MusaDwg1032ResolvedColor guarded[PAGE+2u];
    MusaDwg1032ResolvedColor untouched[PAGE+2u];
    const size_t first=PAGE+3u;
    const size_t bad=first+PAGE/2u;
    memset(guarded,0x37,sizeof(guarded));
    memcpy(untouched,guarded,sizeof(guarded));
    source[bad].has_truecolor=2u;
    assert(!musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,first,PAGE,100u,
        &insert,&layer,guarded+1u));
    assert(memcmp(guarded,untouched,sizeof(guarded))==0);
    source[bad].has_truecolor=0u;
}
int main(void) {
    prepare();
    compare_instance(1u,5u);
    compare_instance(2u,3u);
    late_failure_and_full_source_alias();
    last_record_singleton_page();
    unaligned_page_with_canaries();
    unaligned_late_failure_preserves_canaries();
    first_record_failure_preserves_page();
    middle_record_failure_preserves_page();
    return 0;
}
