/* SPDX-License-Identifier: MIT
 * Independent, synthetic row-major INSERT array pagination regression.
 */
#include "musa_scene2d_array.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Large rotated/reflected arrays must match the single-instance path
 * bit-for-bit after sharing the base affine across batch/page processing. */
static void rotated_mirrored_20k(void) {
    const size_t count=250u*80u;
    MusaScene2dInsertArray a={
        {17.25,-90.5,3.0,-7.0,-2.5,0.75,acos(-1.0)/7.0},
        250u,80u,-1.25,3.75
    };
    double *full=malloc(count*6u*sizeof(*full));
    assert(full);
    size_t written=0,total=0;
    assert(musa_scene2d_array_fill(&a,full,count,&written));
    assert(written==count);
    for (size_t i=0;i<count;i+=97u) {
        double single[6];
        assert(musa_scene2d_array_affine(&a,i%a.columns,i/a.columns,single));
        assert(memcmp(single,full+6u*i,sizeof(single))==0);
    }
    double page[127u*6u];
    for (size_t start=0;start<count;start+=127u) {
        assert(musa_scene2d_array_fill_page(
            &a,start,page,127u,&written,&total));
        const size_t expected=(count-start)<127u ? count-start : 127u;
        assert(total==count && written==expected);
        assert(memcmp(page,full+6u*start,expected*6u*sizeof(double))==0);
    }
    free(full);
}
int main(void) {
    rotated_mirrored_20k();
    MusaScene2dInsertArray array={{10,20,0,0,1,1,0},200,100,2,3};
    const size_t count=200u*100u;
    double *full=malloc(count*6u*sizeof(*full));
    assert(full);
    size_t written=0, total=0;
    assert(musa_scene2d_array_fill(&array,full,count,&written));
    assert(written==count);
    double page[127u*6u];
    size_t next=0, pages=0;
    while (next<count) {
        memset(page,0xa5,sizeof(page));
        written=SIZE_MAX; total=SIZE_MAX;
        assert(musa_scene2d_array_fill_page(&array,next,page,127,
                                              &written,&total));
        assert(total==count && written>0 && written<=127);
        assert(memcmp(page,full+6u*next,written*6u*sizeof(double))==0);
        if (written<127) {
            const unsigned char *bytes=(const unsigned char *)page;
            for (size_t i=written*6u*sizeof(double);i<sizeof(page);++i)
                assert(bytes[i]==0xa5u);
        }
        next+=written;
        ++pages;
    }
    assert(next==count && pages>100);
    memset(page,0xa5,sizeof(page));
    written=123; total=456;
    assert(musa_scene2d_array_fill_page(&array,count,page,127,
                                          &written,&total));
    assert(written==0 && total==count);
    for (size_t i=0;i<sizeof(page);++i)
        assert(((const unsigned char *)page)[i]==0xa5u);
    written=123; total=456;
    assert(musa_scene2d_array_fill_page(&array,SIZE_MAX,NULL,127,
                                          &written,&total));
    assert(written==0 && total==count);
    assert(musa_scene2d_array_fill_page(&array,0,NULL,0,
                                          &written,&total));
    assert(written==0 && total==count);
    written=123; total=456;
    assert(!musa_scene2d_array_fill_page(&array,0,NULL,1,
                                           &written,&total));
    assert(written==123 && total==456);
    assert(!musa_scene2d_array_fill_page(&array,0,page,127,
                                           &written,&written));
    assert(!musa_scene2d_array_fill_page(&array,0,page,127,
                                           (size_t *)(void *)page,&total));
    assert(!musa_scene2d_array_fill_page(&array,0,page,127,
                                           &written,(size_t *)(void *)page));
    assert(!musa_scene2d_array_fill_page(&array,0,
                                           (double *)(void *)&array,1,
                                           &written,&total));
    assert(written==123 && total==456);
    memset(page,0xa5,sizeof(page));
    array.column_spacing=1e308;
    assert(!musa_scene2d_array_fill_page(&array,1,page,2,
                                           &written,&total));
    assert(written==123 && total==456);
    for (size_t i=0;i<sizeof(page);++i)
        assert(((const unsigned char *)page)[i]==0xa5u);
    array.column_spacing=INFINITY;
    assert(!musa_scene2d_array_fill_page(&array,0,NULL,0,
                                           &written,&total));
    array.column_spacing=2;
    array.columns=SIZE_MAX; array.rows=2;
    assert(!musa_scene2d_array_fill_page(&array,0,NULL,0,
                                           &written,&total));
    array.columns=0;
    assert(!musa_scene2d_array_fill_page(&array,0,NULL,0,
                                           &written,&total));
    assert(written==123 && total==456);
    free(full);
    return 0;
}
