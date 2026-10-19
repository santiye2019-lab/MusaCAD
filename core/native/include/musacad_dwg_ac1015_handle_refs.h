/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1015_HANDLE_REFS_H
#define MUSACAD_DWG_AC1015_HANDLE_REFS_H
#include "musacad_dwg_ac1015_values.h"
#include "musacad_dwg_ac1015_object.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* First-party DWG R2000 code:4,count:4,[big-endian handle bytes].
 * This decoder is independent of the entity's semantic role.
 * It does not assert layer/owner/linetype membership or graph validity.
 */
typedef struct MusaDwgAc1015HandleRef {
    uint64_t target;
    uint64_t encoded_value;
    uint8_t code;
    uint8_t count;
    uint8_t relative;
} MusaDwgAc1015HandleRef;
/* Decode H, preserving both *bits and *out byte-for-byte on error.
 * base is the referring object's validated map handle.
 * Strict canonical nonzero IDs; unsupported codes and malformed/overflow
 * offsets are rejected. Absolute codes 0,2-5; relative 6,8,A,C.
 */
int musa_dwg_ac1015_read_handle_ref(
    MusaDwgAc1015Bits *bits,uint64_t base,MusaDwgAc1015HandleRef *out);
/* Deliberately accepts an EXTERNALLY KNOWN expected handle count; does not
 * guess semantic roles. Record must be CRC-verified, and handle stream
 * begins at pre_handle_bits. Remaining 0..7 end-padding bits must be zero.
 * refs MUST have >= expected_count capacity. Transactional 2-pass.
 * Source bytes are borrowed read-only. Not a full object-reference graph.
 */
int musa_dwg_ac1015_read_exact_handle_tail(
    const MusaDwgAc1015ObjectRecord *record,
    size_t expected_count,MusaDwgAc1015HandleRef *refs,size_t capacity);
/* Strict, read-only in-file reference lookup. The caller MUST provide a
 * CRC-validated object record, refs decoded by the H reader, and a map
 * returned by musa_dwg_ac1015_handles_index. It checks map membership and
 * file positions, NOT semantic roles (owner/layer/reactor) or geometry.
 * Requiring every target to exist is intentionally conservative: apply
 * only to reference layouts known to be internal to this DWG file.
 * Inputs are immutable; on failure preserve the entire output array.
 */
typedef struct MusaDwgAc1015ResolvedRef {
    MusaDwgAc1015HandleRef ref;
    uint64_t file_offset;
} MusaDwgAc1015ResolvedRef;
int musa_dwg_ac1015_resolve_refs_in_map(
    const MusaDwgAc1015ObjectRecord *record,
    const MusaDwgAc1015HandleRef *refs,size_t ref_count,
    const MusaDwgAc1015HandleEntry *map,size_t map_count,
    size_t file_bytes,MusaDwgAc1015ResolvedRef *out,size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
