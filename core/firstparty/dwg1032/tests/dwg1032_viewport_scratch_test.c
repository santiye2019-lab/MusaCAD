/* SPDX-License-Identifier: MIT
 * Independent synthetic 20k-entity viewport-mask differential test.
 * Post-decode only; NOT proof of real AC1032 compressed DWG reading.
 */
#include "musa_dwg1032_viewport_visibility.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

enum { ENTITY_COUNT=20000, LAYER_COUNT=1024, OVERRIDE_COUNT=128 };
static uint64_t entity_layers[ENTITY_COUNT];
static uint8_t flags[ENTITY_COUNT], reference[ENTITY_COUNT];
static uint8_t result[ENTITY_COUNT], scratch[ENTITY_COUNT+8];
static MusaDwg1032LayerState layers[LAYER_COUNT];
static MusaDwg1032ViewportFreeze overrides[OVERRIDE_COUNT];

static void verify_large_masks(void) {
    for (size_t i=0;i<LAYER_COUNT;++i) {
        layers[i]=(MusaDwg1032LayerState){
            (uint64_t)i+1u,(uint8_t)(i%13u!=0u),
            (uint8_t)(i%19u!=0u)};
    }
    for (size_t i=0;i<OVERRIDE_COUNT;++i) {
        overrides[i]=(MusaDwg1032ViewportFreeze){
            (uint64_t)(i*8u+1u),(uint8_t)(i%3u==0u)};
    }
    for (size_t i=0;i<ENTITY_COUNT;++i) {
        entity_layers[i]=(uint64_t)((i*37u)%LAYER_COUNT)+1u;
        flags[i]=(uint8_t)(i%17u!=0u);
        result[i]=0xa5u;
    }
    assert(musa_dwg1032_viewport_local_visibility(
        entity_layers,flags,ENTITY_COUNT,layers,LAYER_COUNT,
        overrides,OVERRIDE_COUNT,reference));
    assert(musa_dwg1032_viewport_local_visibility_scratch(
        entity_layers,flags,ENTITY_COUNT,layers,LAYER_COUNT,
        overrides,OVERRIDE_COUNT,result,scratch,sizeof(scratch)));
    assert(memcmp(result,reference,sizeof(result))==0);
    /* A different viewport must not reuse a stale prior mask. */
    for (size_t i=0;i<OVERRIDE_COUNT;++i)
        overrides[i].frozen=(uint8_t)(i%5u==0u);
    assert(musa_dwg1032_viewport_local_visibility(
        entity_layers,flags,ENTITY_COUNT,layers,LAYER_COUNT,
        overrides,OVERRIDE_COUNT,reference));
    assert(musa_dwg1032_viewport_local_visibility_scratch(
        entity_layers,flags,ENTITY_COUNT,layers,LAYER_COUNT,
        overrides,OVERRIDE_COUNT,result,scratch,sizeof(scratch)));
    assert(memcmp(result,reference,sizeof(result))==0);
}

static void verify_atomic_rejection(void) {
    uint8_t saved[ENTITY_COUNT];
    memcpy(saved,result,sizeof(saved));
    /* A late bad handle must not publish the earlier 19,999 results. */
    entity_layers[ENTITY_COUNT-1u]=UINT64_MAX;
    assert(!musa_dwg1032_viewport_local_visibility_scratch(
        entity_layers,flags,ENTITY_COUNT,layers,LAYER_COUNT,
        overrides,OVERRIDE_COUNT,result,scratch,sizeof(scratch)));
    assert(memcmp(result,saved,sizeof(saved))==0);
    entity_layers[ENTITY_COUNT-1u]=
        (uint64_t)(((ENTITY_COUNT-1u)*37u)%LAYER_COUNT)+1u;
    flags[ENTITY_COUNT-1u]=2u;
    assert(!musa_dwg1032_viewport_local_visibility_scratch(
        entity_layers,flags,ENTITY_COUNT,layers,LAYER_COUNT,
        overrides,OVERRIDE_COUNT,result,scratch,sizeof(scratch)));
    assert(memcmp(result,saved,sizeof(saved))==0);
    flags[ENTITY_COUNT-1u]=(uint8_t)((ENTITY_COUNT-1u)%17u!=0u);
    overrides[OVERRIDE_COUNT-1u].layer_handle=UINT64_MAX;
    assert(!musa_dwg1032_viewport_local_visibility_scratch(
        entity_layers,flags,ENTITY_COUNT,layers,LAYER_COUNT,
        overrides,OVERRIDE_COUNT,result,scratch,sizeof(scratch)));
    assert(memcmp(result,saved,sizeof(saved))==0);
    overrides[OVERRIDE_COUNT-1u].layer_handle=
        (uint64_t)((OVERRIDE_COUNT-1u)*8u+1u);
    assert(!musa_dwg1032_viewport_local_visibility_scratch(
        entity_layers,flags,ENTITY_COUNT,layers,LAYER_COUNT,
        overrides,OVERRIDE_COUNT,result,scratch,ENTITY_COUNT-1u));
    assert(!musa_dwg1032_viewport_local_visibility_scratch(
        entity_layers,flags,ENTITY_COUNT,layers,LAYER_COUNT,
        overrides,OVERRIDE_COUNT,result,result,ENTITY_COUNT));
    assert(!musa_dwg1032_viewport_local_visibility_scratch(
        entity_layers,flags,ENTITY_COUNT,layers,LAYER_COUNT,
        overrides,OVERRIDE_COUNT,flags,scratch,sizeof(scratch)));
    assert(!musa_dwg1032_viewport_local_visibility_scratch(
        entity_layers,flags,ENTITY_COUNT,layers,LAYER_COUNT,
        overrides,OVERRIDE_COUNT,result,(uint8_t *)(void *)entity_layers,
        ENTITY_COUNT));
    assert(!musa_dwg1032_viewport_local_visibility_scratch(
        entity_layers,flags,ENTITY_COUNT,layers,LAYER_COUNT,
        overrides,SIZE_MAX,result,scratch,sizeof(scratch)));
    assert(memcmp(result,saved,sizeof(saved))==0);
    assert(musa_dwg1032_viewport_local_visibility_scratch(
        entity_layers,flags,ENTITY_COUNT,layers,LAYER_COUNT,
        overrides,OVERRIDE_COUNT,result,scratch,sizeof(scratch)));
    assert(memcmp(result,saved,sizeof(saved))==0);
}
int main(void) {
    verify_large_masks();
    verify_atomic_rejection();
    return 0;
}
