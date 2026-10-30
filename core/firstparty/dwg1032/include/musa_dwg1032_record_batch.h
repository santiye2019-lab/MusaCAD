/* SPDX-License-Identifier: MIT
 * First-party validation of already located, MS-framed object candidates.
 * This is NOT a complete AC1032 object or entity decoder.
 */
#ifndef MUSA_DWG1032_RECORD_BATCH_H
#define MUSA_DWG1032_RECORD_BATCH_H
#include <stddef.h>
#include "musa_dwg1032_object_index.h"
#include "musa_dwg1032_cursor.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Convert caller-supplied candidate spans into exact MS payload spans.
 * Candidates may contain trailing padding. Input entries must be sorted
 * by unique nonzero handle; their physical byte ranges must not overlap.
 * section is the complete, caller-verified, already decompressed object
 * section at bit position zero. The caller supplies sorting scratch.
 *
 * On success, payloads[i] has the same handle as candidates[i] and
 * references only its bounded MS payload, excluding the size prefix and
 * any candidate padding. A zero-length payload is rejected.
 * On failure, payloads is completely unchanged; order is scratch and
 * may be changed. No allocation or payload copies. O(n log n + bytes).
 * Input, output and scratch buffers must not overlap.
 *
 * The MS prefix is an assumed record envelope, NOT proof that any
 * candidate is an authentic AC1032 entity. Do not enable JNI import.
 */
int musa_dwg1032_record_batch_extract(
    const MusaDwg1032ObjectIndex *candidates,
    const MusaDwg1032Cursor *section,
    size_t max_payload_bytes,
    MusaDwg1032ObjectLocation *payloads, size_t capacity,
    size_t *order, size_t order_capacity);
#ifdef __cplusplus
}
#endif
#endif
