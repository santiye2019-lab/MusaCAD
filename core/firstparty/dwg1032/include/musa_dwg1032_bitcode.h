/* SPDX-License-Identifier: MIT
 * Independently implemented R2018 DWG scalar bitcodes from published field
 * definitions. Only operates on caller-validated bounded bitstreams.
 */
#ifndef MUSA_DWG1032_BITCODE_H
#define MUSA_DWG1032_BITCODE_H
#include "musa_dwg1032_cursor.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Reads BS, BL, BD and R2010+ object type. A signed integer's original
 * two's-complement bit pattern is returned in the unsigned output.
 * All operations preserve the cursor and output on failure, including
 * reserved opcodes, truncation, invalid cursor state and aliasing. */
int musa_dwg1032_bitcode_short(MusaDwg1032Cursor *cursor, uint16_t *value);
int musa_dwg1032_bitcode_long(MusaDwg1032Cursor *cursor, uint32_t *value);
/* Requires IEEE-754 binary64; rejects non-finite values to prevent unsafe
 * geometry propagation. This is a validation policy, not DWG file recovery. */
int musa_dwg1032_bitcode_double(MusaDwg1032Cursor *cursor, double *value);
/* R2010+ (including AC1032) object type, not an entity decoder. */
int musa_dwg1032_bitcode_object_type(MusaDwg1032Cursor *cursor,
                                     uint16_t *value);
#ifdef __cplusplus
}
#endif
#endif
