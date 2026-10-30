/* SPDX-License-Identifier: MIT
 * Bounded first-party decoder for a caller-verified DWG object-map payload.
 */
#ifndef MUSA_DWG1032_OBJECT_MAP_H
#define MUSA_DWG1032_OBJECT_MAP_H
#include <stddef.h>
#include <stdint.h>
#include "musa_dwg1032_object_index.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaDwg1032MapEntry {
    uint64_t handle;
    uint64_t offset;
} MusaDwg1032MapEntry;
/* Incremental, allocation-free iterator over an ALREADY VERIFIED and
 * decompressed object-map payload. It does not authenticate DWG sections,
 * decompress pages, or decode object/entity records. Each successful next()
 * consumes exactly one handle/offset delta pair; a malformed or truncated
 * pair leaves both the iterator and caller's entry unchanged. The caller
 * must verify that the iterator consumed the expected number of pairs and
 * all bytes before trusting a complete map. */
typedef struct MusaDwg1032MapIterator {
    MusaDwg1032Cursor cursor;
    uint64_t handle;
    uint64_t offset;
    uint64_t section_bytes;
} MusaDwg1032MapIterator;
int musa_dwg1032_object_map_iterator_init(
    MusaDwg1032MapIterator *iterator, const uint8_t *bytes,
    size_t byte_count, uint64_t section_bytes);
int musa_dwg1032_object_map_iterator_next(
    MusaDwg1032MapIterator *iterator, MusaDwg1032MapEntry *entry);
/* Decode exactly count (unsigned MC handle delta, signed MC file-offset
 * delta) pairs from a verified, already decompressed object-map payload.
 * Enforces strictly increasing nonzero handles, in-section offsets, no
 * trailing bytes and canonical MC encodings. All output is unchanged on
 * failure; no allocation. Does NOT validate CRC, map page headers, object
 * lengths, section location, decompression or record/entity semantics. */
int musa_dwg1032_object_map_decode(const uint8_t *bytes, size_t byte_count,
                                   size_t count, uint64_t section_bytes,
                                   MusaDwg1032MapEntry *out, size_t capacity);
/* Given caller-verified map entries in strictly increasing handle order,
 * compute a bounded CANDIDATE region for each object: from its map offset
 * to the next higher offset (or section end). The region may contain padding,
 * metadata or bytes that are NOT part of the object record. Never treat the
 * inferred length as a decoded object length without record validation.
 * Offsets must be unique and in range; physical offset order may differ
 * from handle order. O(count log count), no allocation, order is scratch.
 * Failure preserves out; order may be modified. Buffers must not overlap. */
int musa_dwg1032_object_map_candidate_spans(
    const MusaDwg1032MapEntry *entries, size_t count, uint64_t section_bytes,
    MusaDwg1032ObjectLocation *out, size_t capacity,
    size_t *order, size_t order_capacity);
#ifdef __cplusplus
}
#endif
#endif
