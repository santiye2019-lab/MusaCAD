/* SPDX-License-Identifier: MIT
 * Synthetic post-decode viewport evidence; not a real DWG fixture.
 */
#include "musa_dwg1032_viewport_visibility.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>

static void viewport_contexts(void) {
    const MusaDwg1032LayerState layers[]={
        {10,1,1},{20,1,1},{30,0,1},{40,1,0}
    };
    uint64_t handles[]={10,20,30,40,10,20};
    uint8_t entity[]={1,1,1,1,0,1};
    MusaDwg1032ViewportFreeze vp_a[]={{20,1},{40,0}};
    MusaDwg1032ViewportFreeze vp_b[]={{10,1},{20,0}};
    uint8_t out[6]={9,9,9,9,9,9};
    assert(musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_a,2,out));
    const uint8_t expected_a[]={1,0,0,0,0,0};
    assert(!memcmp(out,expected_a,sizeof(out)));
    assert(musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_b,2,out));
    const uint8_t expected_b[]={0,1,0,0,0,1};
    assert(!memcmp(out,expected_b,sizeof(out)));
    assert(musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,NULL,0,out));
    const uint8_t expected_default[]={1,1,0,0,0,1};
    assert(!memcmp(out,expected_default,sizeof(out)));
    uint8_t saved[6]; memcpy(saved,out,sizeof(saved));
    handles[5]=999;
    assert(!musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_a,2,out));
    assert(!memcmp(out,saved,sizeof(out)));
    handles[5]=20;
    entity[5]=2;
    assert(!musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_a,2,out));
    assert(!memcmp(out,saved,sizeof(out)));
    entity[5]=1;
    vp_a[1].frozen=2;
    assert(!musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_a,2,out));
    assert(!memcmp(out,saved,sizeof(out)));
    vp_a[1].frozen=0;
    vp_a[1].layer_handle=20;
    assert(!musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_a,2,out));
    vp_a[1].layer_handle=40;
    assert(!musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_a,2,(uint8_t *)(void *)handles));
    assert(!musa_dwg1032_viewport_local_visibility(
        handles,entity,6,layers,4,vp_a,2,entity));
    assert(!memcmp(out,saved,sizeof(out)));
}
int main(void) { viewport_contexts(); return 0; }
