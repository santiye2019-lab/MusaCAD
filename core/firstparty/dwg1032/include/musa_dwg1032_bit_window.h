/* SPDX-License-Identifier: MIT
 * Independently authored bounded bit windows over caller-verified payloads.
 * No AC1032 section, page, entity or compression layout is inferred here.
 */
#ifndef MUSA_DWG1032_BIT_WINDOW_H
#define MUSA_DWG1032_BIT_WINDOW_H
#include "musa_dwg1032_cursor.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaDwg1032BitWindow {
    MusaDwg1032Cursor cursor;
    size_t end_bit;
} MusaDwg1032BitWindow;
/* Absolute start and length in bits, relative to the borrowed source buffer.
 * Every operation is transactional and no memory is allocated. */
int musa_dwg1032_bit_window_init(MusaDwg1032BitWindow *window,
                                 const MusaDwg1032Cursor *source,
                                 size_t start_bit, size_t bit_length);
int musa_dwg1032_bit_window_remaining(const MusaDwg1032BitWindow *window,
                                      size_t *remaining);
int musa_dwg1032_bit_window_read32(MusaDwg1032BitWindow *window,
                                   unsigned bits, uint32_t *value);
int musa_dwg1032_bit_window_read64(MusaDwg1032BitWindow *window,
                                   unsigned bits, uint64_t *value);
int musa_dwg1032_bit_window_skip(MusaDwg1032BitWindow *window,
                                 size_t bits);
/* Consume a nested bounded bit range; child borrows the same source bytes.
 * On failure neither parent nor child is modified. */
int musa_dwg1032_bit_window_take(MusaDwg1032BitWindow *parent,
                                 size_t bits, MusaDwg1032BitWindow *child);
#ifdef __cplusplus
}
#endif
#endif
