/* SPDX-License-Identifier: MIT
 * First-party validation of already located, MS-framed object candidates.
 * This is NOT a complete AC1032 object or entity decoder.
 */
#ifndef MUSA_DWG1032_RECORD_BATCH_H
#define MUSA_DWG1032_RECORD_BATCH_H
#include <stddef.h>
#include "musa_dwg1032_object_index.h"
#include "musa_dwg1032_object_map.h"
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
/* Extract only the requested page of ALREADY VALIDATED candidate spans.
 * Before the first call, the caller MUST successfully validate the entire
 * immutable index with musa_dwg1032_object_spans_prepare. This fast path
 * deliberately does not repeat full-index sorting or overlap checks:
 * modifying the index after validation invalidates this precondition.
 * start >= count returns an empty page; capacity counts records. Only the
 * requested page is parsed. No allocation, O(page_count) time and O(1)
 * auxiliary memory. A failure leaves payloads, *written and *total intact.
 * The section is a caller-verified, already decompressed object section.
 * MS frames are only candidates, NOT verified AC1032 entities.
 */
int musa_dwg1032_record_batch_extract_page(
    const MusaDwg1032ObjectIndex *validated_candidates,
    const MusaDwg1032Cursor *section,
    size_t start, size_t max_payload_bytes,
    MusaDwg1032ObjectLocation *payloads, size_t capacity,
    size_t *written, size_t *total);
/* Combine a caller-VERIFIED, already decompressed object-map payload with
 * a caller-VERIFIED, already decompressed object section. Decodes exactly
 * object_count map pairs, derives non-overlapping candidate spans, and
 * validates each bounded MS-prefixed record before publishing any payload.
 * map_entries, candidates and order are caller-owned scratch and may change
 * on failure; payloads is unchanged on failure. All scratch, output, source
 * bytes and the section descriptor must be disjoint (the two source byte
 * buffers may overlap, as both are read-only). No allocation or JNI import.
 * O(n log n + bytes). This is NOT an AC1032 page locator, decompressor,
 * CRC validator, entity decoder, or proof of authentic DWG record framing. */
int musa_dwg1032_record_batch_from_verified_map(
    const uint8_t *map_bytes, size_t map_byte_count, size_t object_count,
    const MusaDwg1032Cursor *section, size_t max_payload_bytes,
    MusaDwg1032MapEntry *map_entries, size_t map_capacity,
    MusaDwg1032ObjectLocation *candidates, size_t candidate_capacity,
    MusaDwg1032ObjectLocation *payloads, size_t payload_capacity,
    size_t *order, size_t order_capacity);
#ifdef __cplusplus
}
#endif
#endif
