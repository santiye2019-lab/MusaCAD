/* SPDX-License-Identifier: MIT
 * Independently implemented, bounded DWG modular integer primitives.
 * This module does not locate AC1032 sections or decode object maps.
 */
#ifndef MUSA_DWG1032_MODULAR_H
#define MUSA_DWG1032_MODULAR_H
#include "musa_dwg1032_cursor.h"
#ifdef __cplusplus
extern "C" {
#endif
/* MC handle delta: unsigned base-128 little-endian groups, bit 7 continues.
 * Accepts up to 64 bits. Rejects overlong encodings and overflow.
 * Requires byte alignment. Failure preserves cursor and output. */
int musa_dwg1032_modular_unsigned(MusaDwg1032Cursor *cursor, uint64_t *value);
/* MC signed offset: sign in bit 6 of the last byte, preceding groups
 * carry seven bits. Rejects negative zero, noncanonical forms and overflow.
 * Supports magnitudes up to INT64_MAX. Failure is transactional. */
int musa_dwg1032_modular_signed(MusaDwg1032Cursor *cursor, int64_t *value);
/* MS size: one or two little-endian 16-bit words, bit 15 continues.
 * The two 15-bit payloads form a value of at most 30 bits.
 * Rejects noncanonical second words and truncated input. */
int musa_dwg1032_modular_short(MusaDwg1032Cursor *cursor, uint32_t *value);
#ifdef __cplusplus
}
#endif
#endif
