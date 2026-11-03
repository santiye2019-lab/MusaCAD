/* SPDX-License-Identifier: MIT
 * Original C11 bounded coordinate batches for caller-verified DWG fields.
 */
#ifndef MUSA_DWG1032_POINT_BATCH_H
#define MUSA_DWG1032_POINT_BATCH_H
#include "musa_dwg1032_point.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Decode exactly count consecutive 2BD or 3BD coordinate tuples from an
 * independently authenticated entity field. The entire destination and
 * cursor are committed together only after every tuple is valid. No heap,
 * O(count) time and O(count) caller-owned scratch. Scratch may be partially
 * written on failure; public output and cursor remain unchanged.
 *
 * All of destination[0..count), scratch[0..scratch_capacity), the cursor,
 * and its complete source byte buffer must be mutually disjoint wherever
 * writes occur. The source buffer must remain immutable throughout the call.
 * Reject zero count, insufficient scratch, invalid bitcodes, nonfinite
 * coordinates, truncated input, overlap and byte-count overflow.
 * These routines do NOT locate or authenticate DWG sections/entities. */
int musa_dwg1032_points2bd_batch(MusaDwg1032Cursor *cursor,
    MusaDwg1032Point2 *destination, size_t count,
    MusaDwg1032Point2 *scratch, size_t scratch_capacity);
int musa_dwg1032_points3bd_batch(MusaDwg1032Cursor *cursor,
    MusaDwg1032Point3 *destination, size_t count,
    MusaDwg1032Point3 *scratch, size_t scratch_capacity);
#ifdef __cplusplus
}
#endif
#endif
