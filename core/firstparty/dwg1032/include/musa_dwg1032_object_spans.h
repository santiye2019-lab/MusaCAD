/* SPDX-License-Identifier: MIT
 * Independent post-decode object-span validation. No DWG page decoding.
 */
#ifndef MUSA_DWG1032_OBJECT_SPANS_H
#define MUSA_DWG1032_OBJECT_SPANS_H
#include "musa_dwg1032_object_index.h"
#include "musa_dwg1032_cursor.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Validate all caller-decoded object ranges and build an ascending offset
 * index in caller-owned workspace. Reject overlapping nonempty spans, even
 * if handles are sorted but offsets are not. O(n log n), O(1) extra space.
 * On failure the input index/entries are unchanged; workspace contents may
 * have changed. On success order[i] contains an entry index. No allocation.
 * Caller must keep entries and workspace unchanged while querying. */
int musa_dwg1032_object_spans_prepare(const MusaDwg1032ObjectIndex *index,
                                      size_t *order, size_t capacity);
/* Find the object containing byte_offset (half-open ranges). A gap or an
 * exact end boundary is not an object. Requires successful prepare().
 * On failure *position is unchanged. O(log n). */
int musa_dwg1032_object_spans_find(const MusaDwg1032ObjectIndex *index,
                                   const size_t *order, uint64_t byte_offset,
                                   size_t *position);
/* Return a borrowed bounded cursor for one validated object span.
 * The input section must begin at bit 0 and have exactly section_bytes.
 * This does not decode object records or establish their correctness.
 * On failure *object remains unchanged; no source memory is copied. */
int musa_dwg1032_object_span_cursor(const MusaDwg1032ObjectIndex *index,
                                    const MusaDwg1032Cursor *section,
                                    size_t position, MusaDwg1032Cursor *object);
#ifdef __cplusplus
}
#endif
#endif
