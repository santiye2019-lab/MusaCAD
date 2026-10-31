/* SPDX-License-Identifier: MIT
 * First-party DWG handle field decoder. Does not resolve object ownership.
 */
#ifndef MUSA_DWG1032_HANDLE_H
#define MUSA_DWG1032_HANDLE_H
#include "musa_dwg1032_cursor.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaDwg1032Handle {
    uint64_t payload;
    uint8_t code;
    uint8_t length;
} MusaDwg1032Handle;
/* Reads the four-bit reference code, four-bit byte count and 0..8 payload
 * bytes in most-significant-first order, from an unaligned bit position.
 * Only documented absolute (2..5) and relative (6,8,A,C) forms accepted.
 * No object-map lookup or handle-stream positioning is performed. */
int musa_dwg1032_handle_read(MusaDwg1032Cursor *cursor,
                             MusaDwg1032Handle *handle);
/* Resolves absolute and base-relative references without overflow. The
 * caller must supply the correct owning object's base handle and must still
 * check that the resolved target exists and has the expected relationship. */
int musa_dwg1032_handle_resolve(const MusaDwg1032Handle *handle,
                                uint64_t base, uint64_t *resolved);
#ifdef __cplusplus
}
#endif
#endif
