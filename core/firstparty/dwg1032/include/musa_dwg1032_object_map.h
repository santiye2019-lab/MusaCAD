/* SPDX-License-Identifier: MIT
 * Bounded first-party decoder for a caller-verified DWG object-map payload.
 */
#ifndef MUSA_DWG1032_OBJECT_MAP_H
#define MUSA_DWG1032_OBJECT_MAP_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MusaDwg1032MapEntry {
    uint64_t handle;
    uint64_t offset;
} MusaDwg1032MapEntry;
/* Decode exactly count (unsigned MC handle delta, signed MC file-offset
 * delta) pairs from a verified, already decompressed object-map payload.
 * Enforces strictly increasing nonzero handles, in-section offsets, no
 * trailing bytes and canonical MC encodings. All output is unchanged on
 * failure; no allocation. Does NOT validate CRC, map page headers, object
 * lengths, section location, decompression or record/entity semantics. */
int musa_dwg1032_object_map_decode(const uint8_t *bytes, size_t byte_count,
                                   size_t count, uint64_t section_bytes,
                                   MusaDwg1032MapEntry *out, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
