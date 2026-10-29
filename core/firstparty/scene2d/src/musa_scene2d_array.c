/* SPDX-License-Identifier: MIT
 * Original C11 planar array transform; standard algebra, no CAD library.
 */
#include "musa_scene2d_array.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
static int array_overlap(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
int musa_scene2d_array_affine(const MusaScene2dInsertArray *array,
                               size_t column, size_t row, double out[6]) {
    if (!array || !out || !array->columns || !array->rows ||
        column>=array->columns || row>=array->rows ||
        !isfinite(array->column_spacing) || !isfinite(array->row_spacing) ||
        array_overlap(array,sizeof(*array),out,6u*sizeof(double)))
        return 0;
    double candidate[6];
    if (!musa_scene2d_insert_affine(&array->placement,candidate)) return 0;
    const double dx=(double)column*array->column_spacing;
    const double dy=(double)row*array->row_spacing;
    if (!isfinite(dx) || !isfinite(dy)) return 0;
    const double x=candidate[0]*dx + candidate[2]*dy;
    const double y=candidate[1]*dx + candidate[3]*dy;
    if (!isfinite(x) || !isfinite(y)) return 0;
    candidate[4]+=x;
    candidate[5]+=y;
    if (!isfinite(candidate[4]) || !isfinite(candidate[5])) return 0;
    memcpy(out,candidate,sizeof(candidate));
    return 1;
}

int musa_scene2d_array_fill(const MusaScene2dInsertArray *array,
                             double *out, size_t capacity, size_t *written) {
    if (!array || !out || !written || !array->columns || !array->rows ||
        array->columns>SIZE_MAX/array->rows) return 0;
    const size_t count=array->columns*array->rows;
    if (count>SIZE_MAX/(6u*sizeof(double)) || capacity<count ||
        array_overlap(array,sizeof(*array),out,count*6u*sizeof(double)) ||
        array_overlap(written,sizeof(*written),out,count*6u*sizeof(double)) ||
        array_overlap(array,sizeof(*array),written,sizeof(*written)))
        return 0;
    /* The first pass validates every result without touching the output. */
    double affine[6];
    for (size_t row=0;row<array->rows;++row)
        for (size_t col=0;col<array->columns;++col)
            if (!musa_scene2d_array_affine(array,col,row,affine)) return 0;
    /* Caller keeps the input immutable throughout both passes. */
    for (size_t row=0;row<array->rows;++row)
        for (size_t col=0;col<array->columns;++col) {
            const size_t i=row*array->columns+col;
            if (!musa_scene2d_array_affine(array,col,row,affine)) return 0;
            memcpy(out+6u*i,affine,sizeof(affine));
        }
    *written=count;
    return 1;
}
