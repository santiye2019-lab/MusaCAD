/* SPDX-License-Identifier: MIT */
#include "musa_scene2d_array.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static int close_to(double a,double b) { return fabs(a-b)<1e-9; }
int main(void) {
    MusaScene2dInsertArray array={{10,20,0,0,1,1,0},200,100,2,3};
    const size_t count=200u*100u;
    double *out=malloc(count*6u*sizeof(*out));
    assert(out);
    size_t written=77;
    assert(musa_scene2d_array_fill(&array,out,count,&written));
    assert(written==count);
    for (size_t row=0;row<array.rows;++row)
        for (size_t col=0;col<array.columns;++col) {
            const size_t i=6u*(row*array.columns+col);
            assert(close_to(out[i+4],10+2*(double)col));
            assert(close_to(out[i+5],20+3*(double)row));
            assert(out[i]==1 && out[i+1]==0 &&
                   out[i+2]==0 && out[i+3]==1);
        }
    const double first=out[4],last=out[6u*(count-1u)+5u];
    written=123;
    assert(!musa_scene2d_array_fill(&array,out,count-1u,&written));
    assert(written==123 && out[4]==first &&
           out[6u*(count-1u)+5u]==last);
    assert(!musa_scene2d_array_fill(&array,out,count,(size_t *)(void *)out));
    assert(!musa_scene2d_array_fill(&array,(double *)(void *)&array,count,&written));
    array.columns=SIZE_MAX; array.rows=2;
    assert(!musa_scene2d_array_fill(&array,out,count,&written));
    array.columns=200; array.rows=100;
    array.column_spacing=1e308;
    assert(!musa_scene2d_array_fill(&array,out,count,&written));
    assert(written==123 && out[4]==first &&
           out[6u*(count-1u)+5u]==last);
    array.column_spacing=-2;
    array.placement.scale_x=-1;
    assert(musa_scene2d_array_fill(&array,out,count,&written));
    assert(written==count);
    assert(close_to(out[6u*199u+4u],408));
    free(out);
    return 0;
}
