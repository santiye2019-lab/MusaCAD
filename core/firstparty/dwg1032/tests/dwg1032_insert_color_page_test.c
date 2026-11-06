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
static void retry_after_invalid_middle_record(void) {
    const MusaDwg1032ResolvedColor insert={{2u,0u,0u},0u};
    const MusaDwg1032ResolvedColor layer={{5u,0u,0u},0u};
    MusaDwg1032ResolvedColor output[PAGE+2u];
    MusaDwg1032ResolvedColor saved[PAGE+2u];
    const size_t first=PAGE+3u;
    const size_t bad=first+PAGE/2u;
    assert(musa_dwg1032_layer_color_apply_insert_context(
        definition,source,child_layers,N,100u,&insert,&layer,reference));
    memset(output,0x29,sizeof(output));
    memcpy(saved,output,sizeof(saved));
    source[bad].has_truecolor=2u;
    assert(!musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,first,PAGE,100u,
        &insert,&layer,output+1u));
    assert(memcmp(output,saved,sizeof(output))==0);
    source[bad].has_truecolor=0u;
    assert(musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,first,PAGE,100u,
        &insert,&layer,output+1u));
    assert(memcmp(output+1u,reference+first,PAGE*sizeof(output[0]))==0);
    assert(memcmp(output,saved,sizeof(output[0]))==0);
    assert(memcmp(output+PAGE+1u,saved+PAGE+1u,sizeof(output[0]))==0);
}
static void failed_page_does_not_poison_other_insert_context(void) {
    const MusaDwg1032ResolvedColor first_insert={{2u,0u,0u},0u};
    const MusaDwg1032ResolvedColor second_insert={{7u,0u,0u},0u};
    const MusaDwg1032ResolvedColor layer={{3u,0u,0u},0u};
    MusaDwg1032ResolvedColor output[PAGE+2u], before[PAGE+2u];
    const size_t start=PAGE*3u+1u;
    const size_t bad=start+PAGE-1u;
    memset(output,0x4c,sizeof(output));
    memcpy(before,output,sizeof(before));
    source[bad].has_truecolor=2u;
    assert(!musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,start,PAGE,100u,
        &first_insert,&layer,output+1u));
    assert(memcmp(output,before,sizeof(output))==0);
    source[bad].has_truecolor=0u;
    assert(musa_dwg1032_layer_color_apply_insert_context(
        definition,source,child_layers,N,100u,
        &second_insert,&layer,reference));
    assert(musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,start,PAGE,100u,
        &second_insert,&layer,output+1u));
    assert(memcmp(output+1u,reference+start,PAGE*sizeof(output[0]))==0);
    assert(memcmp(output,before,sizeof(output[0]))==0);
    assert(memcmp(output+PAGE+1u,before+PAGE+1u,sizeof(output[0]))==0);
}
static void independent_insert_contexts_do_not_mutate_source(void) {
    const MusaDwg1032ResolvedColor red={{1u,0u,0u},0u};
    const MusaDwg1032ResolvedColor blue={{5u,0u,0u},0u};
    const MusaDwg1032ResolvedColor layer={{3u,0u,0u},0u};
    const size_t first=PAGE*4u+2u;
    MusaDwg1032ResolvedColor first_page[PAGE], second_page[PAGE];
    MusaDwg1032CadColor original_source[PAGE];
    MusaDwg1032ResolvedColor original_definition[PAGE];
    uint64_t original_layers[PAGE];
    memcpy(original_source,source+first,sizeof(original_source));
    memcpy(original_definition,definition+first,sizeof(original_definition));
    memcpy(original_layers,child_layers+first,sizeof(original_layers));
    assert(musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,first,PAGE,100u,
        &red,&layer,first_page));
    assert(musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,first,PAGE,100u,
        &blue,&layer,second_page));
    for (size_t i=0u;i<PAGE;++i) {
        const size_t index=first+i;
        if (index%3u==1u) {
            assert(first_page[i].color.aci==1u);
            assert(second_page[i].color.aci==5u);
        } else {
            assert(memcmp(first_page+i,second_page+i,sizeof(first_page[i]))==0);
        }
    }
    assert(memcmp(source+first,original_source,sizeof(original_source))==0);
    assert(memcmp(definition+first,original_definition,sizeof(original_definition))==0);
    assert(memcmp(child_layers+first,original_layers,sizeof(original_layers))==0);
}
/* Reject output that aliases a future definition page, even if the
 * requested page itself is disjoint. Future INSERTs must see pristine data. */
static void future_page_alias_is_rejected(void) {
    const MusaDwg1032ResolvedColor insert={{2u,0u,0u},0u};
    const MusaDwg1032ResolvedColor layer={{5u,0u,0u},0u};
    MusaDwg1032ResolvedColor saved[PAGE];
    const size_t future=PAGE*8u;
    memcpy(saved,definition+future,sizeof(saved));
    assert(!musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,0u,PAGE,100u,
        &insert,&layer,definition+future));
    assert(memcmp(saved,definition+future,sizeof(saved))==0);
}
/* TrueColor and explicit ACI must survive an INSERT context unchanged;
 * only layer-zero ByLayer and ByBlock may inherit instance colors. */
static void truecolor_and_provenance_page(void) {
    const size_t first=PAGE*5u+1u;
    const MusaDwg1032ResolvedColor insert={{0u,0x123456u,1u},0u};
    const MusaDwg1032ResolvedColor layer={{0u,0xabcdefu,1u},0u};
    MusaDwg1032ResolvedColor output[5], before[5];
    MusaDwg1032CadColor old_raw[3];
    MusaDwg1032ResolvedColor old_def[3];
    uint64_t old_layers[3];
    memcpy(old_raw,source+first,sizeof(old_raw));
    memcpy(old_def,definition+first,sizeof(old_def));
    memcpy(old_layers,child_layers+first,sizeof(old_layers));
    /* Three distinct semantics: explicit RGB, layer-zero ByLayer, ByBlock. */
    source[first]=(MusaDwg1032CadColor){0u,0x112233u,1u};
    definition[first]=(MusaDwg1032ResolvedColor){{0u,0x112233u,1u},0u};
    source[first+1u]=(MusaDwg1032CadColor){256u,0u,0u};
    definition[first+1u]=(MusaDwg1032ResolvedColor){{4u,0u,0u},0u};
    child_layers[first+1u]=100u;
    source[first+2u]=(MusaDwg1032CadColor){0u,0u,0u};
    /* ByBlock raw provenance requires a canonical unresolved definition. */
    definition[first+2u]=(MusaDwg1032ResolvedColor){{0u,0u,0u},1u};
    /* Isolate provenance failures before checking the combined page. */
    for (size_t offset=0u;offset<3u;++offset) {
        MusaDwg1032ResolvedColor single;
        assert(musa_dwg1032_layer_color_apply_insert_context_page(
            definition,source,child_layers,N,first+offset,1u,100u,
            &insert,&layer,&single));
    }
    memset(output,0x5a,sizeof(output));
    memcpy(before,output,sizeof(before));
    assert(musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,first,3u,100u,
        &insert,&layer,output+1u));
    assert(output[1].color.has_truecolor==1u &&
           output[1].color.truecolor_rgb==0x112233u);
    assert(output[2].color.has_truecolor==1u &&
           output[2].color.truecolor_rgb==0xabcdefu);
    assert(output[3].color.has_truecolor==1u &&
           output[3].color.truecolor_rgb==0x123456u);
    assert(memcmp(output,before,sizeof(output[0]))==0);
    assert(memcmp(output+4u,before+4u,sizeof(output[0]))==0);
    /* A definition contradicting its original RGB must fail atomically. */
    definition[first].color.truecolor_rgb=0x445566u;
    memcpy(output,before,sizeof(output));
    assert(!musa_dwg1032_layer_color_apply_insert_context_page(
        definition,source,child_layers,N,first,3u,100u,
        &insert,&layer,output+1u));
    assert(memcmp(output,before,sizeof(output))==0);
    memcpy(source+first,old_raw,sizeof(old_raw));
    memcpy(definition+first,old_def,sizeof(old_def));
    memcpy(child_layers+first,old_layers,sizeof(old_layers));
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
    retry_after_invalid_middle_record();
    failed_page_does_not_poison_other_insert_context();
    independent_insert_contexts_do_not_mutate_source();
    future_page_alias_is_rejected();
    truecolor_and_provenance_page();
    return 0;
}
