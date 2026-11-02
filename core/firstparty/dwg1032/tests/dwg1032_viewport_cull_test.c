/* SPDX-License-Identifier: MIT
 * Independently authored synthetic post-decode 2D viewport regression.
 * This does not validate AC1032 DWG file parsing or real-world fixtures.
 */
#include "musa_dwg1032_affine2d.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

static void rotated_reflected_and_hidden(void) {
    const MusaDwg1032Affine2 transforms[4]={
        {1,0,0,0,1,0},       /* outside */
        {0,-1,10,1,0,20},   /* rotated rectangle */
        {-1,0,10,0,1,20},   /* reflected but hidden */
        {1,0,9,0,1,21}      /* point touching viewport */
    };
    const MusaDwg1032Bounds2 bounds[4]={
        {0,0,1,1}, {0,0,2,4}, {0,0,2,4}, {0,0,0,0}
    };
    const uint8_t flags[4]={1,1,0,1};
    const MusaDwg1032Bounds2 view={9,21,11,25};
    uint8_t out[4]={9,9,9,9};
    assert(musa_dwg1032_affine2_cull_page(
        transforms,bounds,flags,4,0,4,&view,out));
    assert(out[0]==0 && out[1]==1 && out[2]==0 && out[3]==1);
    const MusaDwg1032Bounds2 touching={10,22,10,22};
    assert(musa_dwg1032_affine2_cull_page(
        transforms,bounds,flags,4,1,1,&touching,out));
    assert(out[0]==1);
}

static void large_sheet_and_late_failure(void) {
    enum { N=20000, FIRST=500, PAGE=1500 };
    static MusaDwg1032Affine2 transforms[N];
    static MusaDwg1032Bounds2 bounds[N];
    static uint8_t flags[N],out[PAGE],saved[PAGE];
    for (size_t i=0;i<N;++i) {
        transforms[i]=(MusaDwg1032Affine2){
            1,0,(double)i*10,0,1,0
        };
        bounds[i]=(MusaDwg1032Bounds2){0,0,2,2};
        flags[i]=1;
    }
    const MusaDwg1032Bounds2 view={10000,0,10012,2};
    memset(out,0xa5,sizeof(out));
    assert(musa_dwg1032_affine2_cull_page(
        transforms,bounds,flags,N,FIRST,PAGE,&view,out));
    for (size_t i=0;i<PAGE;++i) {
        const size_t global=FIRST+i;
        assert(out[i]==(uint8_t)(global==1000 || global==1001));
    }
    memcpy(saved,out,sizeof(out));
    /* Hidden malformed records must also fail; otherwise an INSERT can
     * silently become visible after a subsequent viewport/layer change. */
    flags[FIRST+PAGE-1]=0;
    transforms[FIRST+PAGE-1].tx=NAN;
    assert(!musa_dwg1032_affine2_cull_page(
        transforms,bounds,flags,N,FIRST,PAGE,&view,out));
    assert(memcmp(saved,out,sizeof(out))==0);
    transforms[FIRST+PAGE-1].tx=1e308;
    transforms[FIRST+PAGE-1].xx=1e308;
    bounds[FIRST+PAGE-1]=(MusaDwg1032Bounds2){2,0,3,1};
    assert(!musa_dwg1032_affine2_cull_page(
        transforms,bounds,flags,N,FIRST,PAGE,&view,out));
    assert(memcmp(saved,out,sizeof(out))==0);
    transforms[FIRST+PAGE-1]=(MusaDwg1032Affine2){
        1,0,(double)(FIRST+PAGE-1)*10,0,1,0
    };
    bounds[FIRST+PAGE-1]=(MusaDwg1032Bounds2){0,0,2,2};
    flags[FIRST+PAGE-1]=2;
    assert(!musa_dwg1032_affine2_cull_page(
        transforms,bounds,flags,N,FIRST,PAGE,&view,out));
    assert(memcmp(saved,out,sizeof(out))==0);
    flags[FIRST+PAGE-1]=1;

    const MusaDwg1032Bounds2 bad_view={10,0,0,2};
    assert(!musa_dwg1032_affine2_cull_page(
        transforms,bounds,flags,N,FIRST,PAGE,&bad_view,out));
    assert(!musa_dwg1032_affine2_cull_page(
        transforms,bounds,flags,N,N-1,2,&view,out));
    assert(!musa_dwg1032_affine2_cull_page(
        transforms,bounds,flags,N,0,0,&view,out));
    assert(!musa_dwg1032_affine2_cull_page(
        transforms,bounds,flags,N,0,PAGE,&view,flags+N-PAGE));
    assert(!musa_dwg1032_affine2_cull_page(
        transforms,bounds,flags,N,0,PAGE,&view,
        (uint8_t *)(void *)&transforms[N-1]));
    assert(!musa_dwg1032_affine2_cull_page(
        transforms,bounds,flags,N,0,PAGE,&view,
        (uint8_t *)(void *)&bounds[N-1]));
    assert(!musa_dwg1032_affine2_cull_page(
        transforms,bounds,flags,N,0,PAGE,&view,
        (uint8_t *)(void *)&view));
    assert(memcmp(saved,out,sizeof(out))==0);
}

int main(void) {
    rotated_reflected_and_hidden();
    large_sheet_and_late_failure();
    return 0;
}
