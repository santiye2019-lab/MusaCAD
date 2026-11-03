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
/* Reuse the INSERT base transform for large MINSERT batches. Only the
 * per-cell offsets differ; no trigonometry is needed per instance. */
static int instance_from_base(const MusaScene2dInsertArray *array,
                              const double base[6], size_t column,
                              size_t row, double out[6]) {
    double candidate[6];
    memcpy(candidate,base,sizeof(candidate));
    const double dx=(double)column*array->column_spacing;
    const double dy=(double)row*array->row_spacing;
    if (!isfinite(dx) || !isfinite(dy)) return 0;
    /* Keep the existing translation inside the fused sum: intermediate
     * scaled offsets may overflow even when the final placement is finite.
     * This also avoids two extra rounding steps on large CAD coordinates. */
    double tx=fma(candidate[0],dx,
                  fma(candidate[2],dy,candidate[4]));
    double ty=fma(candidate[1],dx,
                  fma(candidate[3],dy,candidate[5]));
    /* Either inner product may overflow before an opposite-signed outer
     * term cancels it. Retry with the other term evaluated first; preserve
     * the established finite fast path and reject genuine overflow. */
    if (!isfinite(tx))
        tx=fma(candidate[2],dy,fma(candidate[0],dx,candidate[4]));
    if (!isfinite(ty))
        ty=fma(candidate[3],dy,fma(candidate[1],dx,candidate[5]));
    if (!isfinite(tx) || !isfinite(ty)) return 0;
    candidate[4]=tx;
    candidate[5]=ty;
    memcpy(out,candidate,sizeof(candidate));
    return 1;
}

int musa_scene2d_array_affine(const MusaScene2dInsertArray *array,
                               size_t column, size_t row, double out[6]) {
    if (!array || !out || !array->columns || !array->rows ||
        column>=array->columns || row>=array->rows ||
        !isfinite(array->column_spacing) || !isfinite(array->row_spacing) ||
        array_overlap(array,sizeof(*array),out,6u*sizeof(double)))
        return 0;
    double base[6];
    if (!musa_scene2d_insert_affine(&array->placement,base)) return 0;
    return instance_from_base(array,base,column,row,out);
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
    double base[6],affine[6];
    if (!musa_scene2d_array_affine(array,0u,0u,base)) return 0;
    for (size_t row=0;row<array->rows;++row)
        for (size_t col=0;col<array->columns;++col)
            if (!instance_from_base(array,base,col,row,affine)) return 0;
    /* Caller keeps the input immutable throughout both passes. */
    for (size_t row=0;row<array->rows;++row)
        for (size_t col=0;col<array->columns;++col) {
            const size_t i=row*array->columns+col;
            if (!instance_from_base(array,base,col,row,affine)) return 0;
            memcpy(out+6u*i,affine,sizeof(affine));
        }
    *written=count;
    return 1;
}

/* One checked evaluation per instance, followed by an atomic publication
 * from caller-owned scratch. No allocation, recursion, or CAD dependencies. */
int musa_scene2d_array_fill_scratch(
    const MusaScene2dInsertArray *array,
    double *out, size_t capacity, double *scratch,
    size_t scratch_capacity, size_t *written) {
    if (!array || !out || !scratch || !written ||
        !array->columns || !array->rows ||
        array->columns>SIZE_MAX/array->rows) return 0;
    const size_t count=array->columns*array->rows;
    if (count>SIZE_MAX/(6u*sizeof(double)) ||
        count>capacity || count>scratch_capacity) return 0;
    const size_t bytes=count*6u*sizeof(double);
    if (array_overlap(array,sizeof(*array),out,bytes) ||
        array_overlap(array,sizeof(*array),scratch,bytes) ||
        array_overlap(array,sizeof(*array),written,sizeof(*written)) ||
        array_overlap(out,bytes,scratch,bytes) ||
        array_overlap(out,bytes,written,sizeof(*written)) ||
        array_overlap(scratch,bytes,written,sizeof(*written)))
        return 0;
    double base[6],affine[6];
    if (!musa_scene2d_array_affine(array,0u,0u,base)) return 0;
    for (size_t row=0;row<array->rows;++row) {
        for (size_t col=0;col<array->columns;++col) {
            if (!instance_from_base(array,base,col,row,affine)) return 0;
            const size_t i=row*array->columns+col;
            memcpy(scratch+6u*i,affine,sizeof(affine));
        }
    }
    memcpy(out,scratch,bytes);
    *written=count;
    return 1;
}

/* Page only the requested row-major instances: O(page_size), independent
 * of the number of preceding instances in a very large block array. */
int musa_scene2d_array_fill_page(const MusaScene2dInsertArray *array,
                                  size_t start, double *out, size_t capacity,
                                  size_t *written, size_t *total) {
    if (!array || !written || !total ||
        array_overlap(array,sizeof(*array),written,sizeof(*written)) ||
        array_overlap(array,sizeof(*array),total,sizeof(*total)) ||
        array_overlap(written,sizeof(*written),total,sizeof(*total)) ||
        !array->columns || !array->rows ||
        array->columns>SIZE_MAX/array->rows) return 0;
    const size_t count=array->columns*array->rows;
    const size_t remaining=start<count ? count-start : 0u;
    const size_t page_count=remaining<capacity ? remaining:capacity;
    if (page_count>SIZE_MAX/(6u*sizeof(double)) ||
        (page_count && !out)) return 0;
    const size_t output_bytes=page_count*6u*sizeof(double);
    if (array_overlap(array,sizeof(*array),out,output_bytes) ||
        array_overlap(written,sizeof(*written),out,output_bytes) ||
        array_overlap(total,sizeof(*total),out,output_bytes)) return 0;
    double base[6],affine[6];
    /* Validate even an empty page's base placement and spacings. */
    if (!musa_scene2d_array_affine(array,0u,0u,base)) return 0;
    for (size_t i=0;i<page_count;++i) {
        const size_t at=start+i; /* <= count-1; checked by remaining. */
        if (!instance_from_base(array,base,at%array->columns,
                                        at/array->columns,affine)) return 0;
    }
    for (size_t i=0;i<page_count;++i) {
        const size_t at=start+i;
        (void)instance_from_base(array,base,at%array->columns,
                                         at/array->columns,affine);
        memcpy(out+6u*i,affine,sizeof(affine));
    }
    *written=page_count;
    *total=count;
    return 1;
}
