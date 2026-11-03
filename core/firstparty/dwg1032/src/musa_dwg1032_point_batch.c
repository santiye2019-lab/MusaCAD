/* SPDX-License-Identifier: MIT
 * First-party atomic 2BD/3BD batches; no external CAD implementation.
 */
#include "musa_dwg1032_point_batch.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int validate(const MusaDwg1032Cursor *cursor,
                    const void *destination, size_t bytes,
                    const void *scratch, size_t scratch_bytes) {
    if (!cursor || !destination || !scratch ||
        (cursor->byte_count && !cursor->bytes) ||
        cursor->byte_count>PTRDIFF_MAX/8u ||
        cursor->bit_position>cursor->byte_count*8u)
        return 0;
    if (overlaps(cursor,sizeof(*cursor),cursor->bytes,cursor->byte_count) ||
        overlaps(destination,bytes,cursor,sizeof(*cursor)) ||
        overlaps(scratch,scratch_bytes,cursor,sizeof(*cursor)) ||
        overlaps(destination,bytes,cursor->bytes,cursor->byte_count) ||
        overlaps(scratch,scratch_bytes,cursor->bytes,cursor->byte_count) ||
        overlaps(destination,bytes,scratch,scratch_bytes))
        return 0;
    return 1;
}
int musa_dwg1032_points2bd_batch(MusaDwg1032Cursor *cursor,
    MusaDwg1032Point2 *destination, size_t count,
    MusaDwg1032Point2 *scratch, size_t scratch_capacity) {
    if (!count || count>SIZE_MAX/sizeof(*destination) ||
        scratch_capacity<count ||
        scratch_capacity>SIZE_MAX/sizeof(*scratch))
        return 0;
    const size_t bytes=count*sizeof(*destination);
    if (!validate(cursor,destination,bytes,scratch,
                  scratch_capacity*sizeof(*scratch)))
        return 0;
    /* Every bitcoded double needs at least two bits. */
    if (count>(cursor->byte_count*8u-cursor->bit_position)/4u)
        return 0;
    MusaDwg1032Cursor trial=*cursor;
    for (size_t i=0;i<count;++i)
        if (!musa_dwg1032_point2bd(&trial,&scratch[i])) return 0;
    memcpy(destination,scratch,bytes);
    *cursor=trial;
    return 1;
}
int musa_dwg1032_points3bd_batch(MusaDwg1032Cursor *cursor,
    MusaDwg1032Point3 *destination, size_t count,
    MusaDwg1032Point3 *scratch, size_t scratch_capacity) {
    if (!count || count>SIZE_MAX/sizeof(*destination) ||
        scratch_capacity<count ||
        scratch_capacity>SIZE_MAX/sizeof(*scratch))
        return 0;
    const size_t bytes=count*sizeof(*destination);
    if (!validate(cursor,destination,bytes,scratch,
                  scratch_capacity*sizeof(*scratch)))
        return 0;
    if (count>(cursor->byte_count*8u-cursor->bit_position)/6u)
        return 0;
    MusaDwg1032Cursor trial=*cursor;
    for (size_t i=0;i<count;++i)
        if (!musa_dwg1032_point3bd(&trial,&scratch[i])) return 0;
    memcpy(destination,scratch,bytes);
    *cursor=trial;
    return 1;
}
