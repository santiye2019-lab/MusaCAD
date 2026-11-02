/* SPDX-License-Identifier: MIT
 * Synthetic 20k occurrence post-decode viewport regression.
 * This does NOT demonstrate actual AC1032 entity decoding.
 */
#include "musa_dwg1032_affine2d.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

enum { N=20000 };
static MusaDwg1032Affine2 transforms[N];
static MusaDwg1032Bounds2 bounds[N];
static uint8_t flags[N], expected[N], actual[N], scratch[N+8];

static void setup(void) {
    for (size_t i=0u;i<N;++i) {
        const double x=(double)(int)(i%251u)-125.0;
        const double y=(double)(int)(i%199u)-99.0;
        transforms[i]=(MusaDwg1032Affine2){
            i%7u==0u?-1.0:1.0,0.125,x,
            -0.25,1.0,y
        };
        bounds[i]=(MusaDwg1032Bounds2){
            0.0,0.0,i%13u==0u?0.0:2.0,i%17u==0u?0.0:3.0
        };
        flags[i]=(uint8_t)(i%11u!=0u);
    }
}
static void compare_all_pages(void) {
    const MusaDwg1032Bounds2 viewports[]={
        {-25.0,-25.0,25.0,25.0},
        {0.0,0.0,0.0,0.0},
        {-1000.0,-1000.0,1000.0,1000.0},
        {500.0,500.0,501.0,501.0}
    };
    const size_t pages[]={1u,7u,257u,1024u,N};
    for (size_t v=0u;v<sizeof(viewports)/sizeof(viewports[0]);++v) {
        for (size_t k=0u;k<sizeof(pages)/sizeof(pages[0]);++k) {
            memset(expected,0xa5,sizeof(expected));
            memset(actual,0xa5,sizeof(actual));
            for (size_t first=0u;first<N;) {
                const size_t left=N-first;
                const size_t count=pages[k]<left?pages[k]:left;
                assert(musa_dwg1032_affine2_cull_page(
                    transforms,bounds,flags,N,first,count,
                    &viewports[v],expected+first));
                assert(musa_dwg1032_affine2_cull_page_scratch(
                    transforms,bounds,flags,N,first,count,
                    &viewports[v],actual+first,scratch,sizeof(scratch)));
                first+=count;
            }
            assert(memcmp(expected,actual,sizeof(expected))==0);
        }
    }
}
static void reject_atomically(void) {
    const MusaDwg1032Bounds2 viewport={-10.0,-10.0,10.0,10.0};
    uint8_t page[257];
    memset(page,0xa5,sizeof(page));
    flags[N-1u]=2u;
    assert(!musa_dwg1032_affine2_cull_page_scratch(
        transforms,bounds,flags,N,N-257u,257u,&viewport,
        page,scratch,sizeof(scratch)));
    flags[N-1u]=0u; /* Hidden invalid geometry must still be rejected. */
    bounds[N-1u].max_x=NAN;
    assert(!musa_dwg1032_affine2_cull_page_scratch(
        transforms,bounds,flags,N,N-257u,257u,&viewport,
        page,scratch,sizeof(scratch)));
    bounds[N-1u].max_x=2.0;
    transforms[N-1u].tx=INFINITY;
    assert(!musa_dwg1032_affine2_cull_page_scratch(
        transforms,bounds,flags,N,N-257u,257u,&viewport,
        page,scratch,sizeof(scratch)));
    transforms[N-1u].tx=(double)(int)((N-1u)%251u)-125.0;
    flags[N-1u]=(uint8_t)((N-1u)%11u!=0u);
    MusaDwg1032Bounds2 bad_viewport=viewport;
    bad_viewport.max_y=-11.0;
    assert(!musa_dwg1032_affine2_cull_page_scratch(
        transforms,bounds,flags,N,0u,257u,&bad_viewport,
        page,scratch,sizeof(scratch)));
    assert(!musa_dwg1032_affine2_cull_page_scratch(
        transforms,bounds,flags,N,N,1u,&viewport,
        page,scratch,sizeof(scratch)));
    assert(!musa_dwg1032_affine2_cull_page_scratch(
        transforms,bounds,flags,N,SIZE_MAX,1u,&viewport,
        page,scratch,sizeof(scratch)));
    assert(!musa_dwg1032_affine2_cull_page_scratch(
        transforms,bounds,flags,SIZE_MAX,0u,257u,&viewport,
        page,scratch,sizeof(scratch)));
    assert(!musa_dwg1032_affine2_cull_page_scratch(
        transforms,bounds,flags,N,0u,257u,&viewport,
        page,scratch,256u));
    assert(!musa_dwg1032_affine2_cull_page_scratch(
        transforms,bounds,flags,N,0u,257u,&viewport,
        page,page,257u));
    assert(!musa_dwg1032_affine2_cull_page_scratch(
        transforms,bounds,flags,N,0u,257u,&viewport,
        page,flags+1000u,257u));
    assert(!musa_dwg1032_affine2_cull_page_scratch(
        transforms,bounds,flags,N,0u,257u,&viewport,
        flags+1000u,scratch,sizeof(scratch)));
    assert(!musa_dwg1032_affine2_cull_page_scratch(
        transforms,bounds,flags,N,0u,257u,&viewport,
        page,(uint8_t *)(void *)(transforms+1000u),257u));
    assert(!musa_dwg1032_affine2_cull_page_scratch(
        transforms,bounds,flags,N,0u,257u,&viewport,
        page,(uint8_t *)(void *)&viewport,257u));
    assert(!musa_dwg1032_affine2_cull_page_scratch(
        transforms,bounds,flags,N,0u,257u,&viewport,
        page,scratch,SIZE_MAX));
    for (size_t i=0u;i<sizeof(page);++i) assert(page[i]==0xa5u);
    assert(musa_dwg1032_affine2_cull_page_scratch(
        transforms,bounds,flags,N,N-257u,257u,&viewport,
        page,scratch,sizeof(scratch)));
    assert(musa_dwg1032_affine2_cull_page(
        transforms,bounds,flags,N,N-257u,257u,&viewport,expected));
    assert(memcmp(page,expected,257u)==0);
}
int main(void) {
    setup();
    compare_all_pages();
    reject_atomically();
    return 0;
}
