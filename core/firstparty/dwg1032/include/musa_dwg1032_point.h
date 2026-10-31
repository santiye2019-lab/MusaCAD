/* SPDX-License-Identifier: MIT
 * First-party, bounded DWG coordinate bitcodes. Not an entity decoder.
 */
#ifndef MUSA_DWG1032_POINT_H
#define MUSA_DWG1032_POINT_H
#include "musa_dwg1032_cursor.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct MusaDwg1032Point2 { double x, y; } MusaDwg1032Point2;
typedef struct MusaDwg1032Point3 { double x, y, z; } MusaDwg1032Point3;

/* 2RD/3RD: consecutive raw IEEE-754 little-endian doubles; 2BD/3BD:
 * consecutive DWG bitcoded doubles. Fields can begin at any bit offset.
 * Rejects nonfinite coordinates, truncation and aliased output. Cursor and
 * output are unchanged on failure. Caller must independently validate the
 * enclosing AC1032 section and entity schema before using these values. */
int musa_dwg1032_point2rd(MusaDwg1032Cursor *cursor, MusaDwg1032Point2 *point);
int musa_dwg1032_point3rd(MusaDwg1032Cursor *cursor, MusaDwg1032Point3 *point);
int musa_dwg1032_point2bd(MusaDwg1032Cursor *cursor, MusaDwg1032Point2 *point);
int musa_dwg1032_point3bd(MusaDwg1032Cursor *cursor, MusaDwg1032Point3 *point);
#ifdef __cplusplus
}
#endif
#endif
